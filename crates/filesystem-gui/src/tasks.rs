use crate::apps::{load_app_registry, open_file_with_app};
use crate::icons::decorate_entries;
use crate::model::{
    DesktopApp, DisplayListing, DisplaySearchResults, HomeShortcut, Message, NewEntryKind,
};
use filesystem_core::{
    DirectoryListing, FsError, ScanOptions, create_file, create_folder, delete_entry, scan_dir,
    search_file_names,
};
use iced::{Task, window};
use std::env;
use std::fs;
use std::os::unix::fs::PermissionsExt;
use std::path::PathBuf;
use std::process::Command;

pub(crate) fn load_home_shortcuts(home: Option<PathBuf>) -> Task<Message> {
    Task::perform(
        async move { detect_home_shortcuts(home) },
        Message::HomeShortcutsLoaded,
    )
}

pub(crate) fn load_app_registry_task() -> Task<Message> {
    Task::perform(async { load_app_registry() }, Message::ApplicationsLoaded)
}

fn detect_home_shortcuts(home: Option<PathBuf>) -> Vec<HomeShortcut> {
    let Some(home) = home else {
        return Vec::new();
    };

    let definitions = [
        (
            include_bytes!("../../../icons/download.svg").as_slice(),
            "下载",
            ["下载", "Downloads"],
        ),
        (
            include_bytes!("../../../icons/picture.svg").as_slice(),
            "图片",
            ["图片", "Pictures"],
        ),
        (
            include_bytes!("../../../icons/desktop.svg").as_slice(),
            "桌面",
            ["桌面", "Desktop"],
        ),
        (
            include_bytes!("../../../icons/document.svg").as_slice(),
            "文档",
            ["文档", "Documents"],
        ),
        (
            include_bytes!("../../../icons/music.svg").as_slice(),
            "音乐",
            ["音乐", "Music"],
        ),
        (
            include_bytes!("../../../icons/videos.svg").as_slice(),
            "视频",
            ["视频", "Videos"],
        ),
    ];

    let mut shortcuts = Vec::new();

    for (icon, label, candidates) in definitions {
        if let Some(path) = candidates
            .into_iter()
            .map(|candidate| home.join(candidate))
            .find(|path| path.is_dir())
        {
            shortcuts.push(HomeShortcut { icon, label, path });
        }
    }

    shortcuts
}

pub(crate) fn load_directory(
    path: PathBuf,
    options: ScanOptions,
) -> Result<DisplayListing, FsError> {
    let DirectoryListing { path, entries } = scan_dir(path, options)?;

    Ok(DisplayListing {
        path,
        entries: decorate_entries(entries),
    })
}

pub(crate) fn load_search(
    root: PathBuf,
    query: String,
    options: ScanOptions,
) -> Result<DisplaySearchResults, FsError> {
    let results = search_file_names(root, query, options)?;

    Ok(DisplaySearchResults {
        root: results.root,
        query: results.query,
        entries: decorate_entries(results.entries),
    })
}

pub(crate) fn create_entry_task(parent: PathBuf, kind: NewEntryKind) -> Task<Message> {
    Task::perform(
        async move {
            match kind {
                NewEntryKind::File => create_file(parent, "新建文件"),
                NewEntryKind::Folder => create_folder(parent, "新建文件夹"),
            }
        },
        move |result| Message::CreateFinished(kind, result),
    )
}

pub(crate) fn delete_entry_task(path: PathBuf) -> Task<Message> {
    Task::perform(
        async move {
            delete_entry(&path)?;
            Ok(path)
        },
        Message::DeleteFinished,
    )
}

pub(crate) fn open_file_with_app_task(path: PathBuf, app: DesktopApp) -> Task<Message> {
    Task::perform(
        async move { open_file_with_app(path, app) },
        Message::OpenFileFinished,
    )
}

pub(crate) fn open_terminal(cwd: PathBuf) -> Result<String, String> {
    for terminal in ["terminator", "mate-terminal", "gnome-terminal"] {
        let Some(executable) = find_executable_in_path(terminal) else {
            continue;
        };

        Command::new(&executable)
            .arg("--working-directory")
            .arg(&cwd)
            .spawn()
            .map_err(|error| format!("Failed to open {terminal}: {error}"))?;

        return Ok(terminal.to_string());
    }

    Err("No supported terminal found in PATH".to_string())
}

fn find_executable_in_path(name: &str) -> Option<PathBuf> {
    let paths = env::var_os("PATH")?;

    env::split_paths(&paths)
        .map(|path| path.join(name))
        .find(|path| {
            fs::metadata(path)
                .map(|metadata| metadata.is_file() && metadata.permissions().mode() & 0o111 != 0)
                .unwrap_or(false)
        })
}

pub(crate) fn latest_window_task(
    action: impl Fn(window::Id) -> Task<Message> + Send + 'static,
) -> Task<Message> {
    window::latest().then(move |id| match id {
        Some(id) => action(id),
        None => Task::none(),
    })
}
