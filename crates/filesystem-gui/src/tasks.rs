use crate::apps::{load_app_registry, open_file_with_app, set_default_app_for_mime};
use crate::config::BlankMenuCommand;
use crate::icons::{basic_entries, decorate_entry_batch, decorated_entry};
use crate::model::{
    AutoRefreshRequest, CurrentFolderChange, CurrentFolderChangeKind, DefaultAppAssignment,
    DeleteEntriesOutcome, DesktopApp, DirectoryLoadEvent, DirectoryRequest, DisplayEntry,
    DisplaySearchResults, FileOperationEvent, HomeShortcut, Message, NewEntryKind, OpenFileOutcome,
    TemplateFile,
};
use filesystem_core::{
    create_file, create_file_from_template, create_folder, delete_entry, entry_for_path,
    paste_paths_with_progress, scan_dir, search_file_names, DirectoryScanner, FileEntry,
    FileOperationCancelToken, FsError, PasteAction, ScanOptions,
};
use iced::{
    futures::channel::mpsc as iced_mpsc, futures::SinkExt, stream, window, Subscription, Task,
};
use notify::event::{AccessKind, AccessMode, MetadataKind, ModifyKind};
use notify::{Event, EventKind, RecursiveMode, Watcher};
use std::env;
use std::fs;
use std::future;
use std::os::unix::fs::PermissionsExt;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::mpsc as std_mpsc;
use std::thread;
use std::time::Duration;

const DIRECTORY_BATCH_SIZE: usize = 128;
const DIRECTORY_STREAM_BUFFER: usize = 1024;
const CURRENT_FOLDER_WATCH_BUFFER: usize = 128;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct CurrentFolderState {
    path: PathBuf,
}

pub(crate) fn load_home_shortcuts(home: Option<PathBuf>) -> Task<Message> {
    Task::perform(
        async move { detect_home_shortcuts(home) },
        Message::HomeShortcutsLoaded,
    )
}

pub(crate) fn load_app_registry_task() -> Task<Message> {
    Task::perform(async { load_app_registry() }, Message::ApplicationsLoaded)
}

pub(crate) fn load_template_files_task(home: Option<PathBuf>) -> Task<Message> {
    Task::perform(
        async move { detect_template_files(home) },
        Message::TemplateFilesLoaded,
    )
}

pub(crate) fn watch_current_folder(path: PathBuf) -> Subscription<Message> {
    Subscription::run_with(CurrentFolderState { path }, current_folder_events)
}

fn current_folder_events(state: &CurrentFolderState) -> impl iced::futures::Stream<Item = Message> {
    let path = state.path.clone();

    stream::channel(
        CURRENT_FOLDER_WATCH_BUFFER,
        move |mut output: iced_mpsc::Sender<Message>| async move {
            let watch_path = path.clone();
            let event_path = path.clone();
            let mut event_output = output.clone();

            let mut watcher =
                match notify::recommended_watcher(move |event: notify::Result<Event>| match event {
                    Ok(event) => {
                        if let Some(change) = current_folder_change_for_event(&event_path, &event) {
                            let _ = event_output.try_send(Message::CurrentFolderChanged(change));
                        }
                    }
                    Err(error) => {
                        let _ = event_output.try_send(Message::CurrentFolderWatchFailed(
                            event_path.clone(),
                            error.to_string(),
                        ));
                    }
                }) {
                    Ok(watcher) => watcher,
                    Err(error) => {
                        let _ = output
                            .send(Message::CurrentFolderWatchFailed(
                                watch_path,
                                error.to_string(),
                            ))
                            .await;
                        future::pending::<()>().await;
                        return;
                    }
                };

            if let Err(error) = watcher.watch(&watch_path, RecursiveMode::NonRecursive) {
                let _ = output
                    .send(Message::CurrentFolderWatchFailed(
                        watch_path,
                        error.to_string(),
                    ))
                    .await;
                future::pending::<()>().await;
                return;
            }

            let _watcher = watcher;
            future::pending::<()>().await;
        },
    )
}

fn current_folder_change_for_event(cwd: &Path, event: &Event) -> Option<CurrentFolderChange> {
    let paths = direct_child_event_paths(cwd, &event.paths);

    if event.need_rescan() {
        return Some(CurrentFolderChange {
            path: cwd.to_path_buf(),
            kind: CurrentFolderChangeKind::Rescan,
            paths,
        });
    }

    let kind = match event.kind {
        EventKind::Any | EventKind::Other => CurrentFolderChangeKind::Rescan,
        EventKind::Create(_) | EventKind::Remove(_) => CurrentFolderChangeKind::Structure,
        EventKind::Modify(ModifyKind::Metadata(MetadataKind::AccessTime)) => return None,
        EventKind::Modify(ModifyKind::Name(_)) => CurrentFolderChangeKind::Structure,
        EventKind::Modify(ModifyKind::Data(_)) | EventKind::Modify(ModifyKind::Metadata(_)) => {
            CurrentFolderChangeKind::Entry
        }
        EventKind::Modify(ModifyKind::Any | ModifyKind::Other) => CurrentFolderChangeKind::Rescan,
        EventKind::Access(AccessKind::Close(AccessMode::Write)) => CurrentFolderChangeKind::Entry,
        EventKind::Access(_) => return None,
    };

    if kind != CurrentFolderChangeKind::Rescan && paths.is_empty() {
        return None;
    }

    let paths = if kind == CurrentFolderChangeKind::Entry {
        paths
            .into_iter()
            .filter(|path| !path.is_dir())
            .collect::<Vec<_>>()
    } else {
        paths
    };

    if kind == CurrentFolderChangeKind::Entry && paths.is_empty() {
        return None;
    }

    Some(CurrentFolderChange {
        path: cwd.to_path_buf(),
        kind,
        paths,
    })
}

fn direct_child_event_paths(cwd: &Path, paths: &[PathBuf]) -> Vec<PathBuf> {
    paths
        .iter()
        .filter(|path| path.parent() == Some(cwd))
        .cloned()
        .collect()
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

fn detect_template_files(home: Option<PathBuf>) -> Vec<TemplateFile> {
    let Some(home) = home else {
        return Vec::new();
    };

    template_files_in_dirs(template_directories(&home))
}

fn template_directories(home: &Path) -> Vec<PathBuf> {
    let mut directories = Vec::new();
    let user_dirs_config = user_dirs_config_path(home);

    if let Ok(contents) = fs::read_to_string(user_dirs_config) {
        if let Some(path) = parse_user_dirs_templates_dir(&contents, home) {
            push_template_directory(&mut directories, home, path);
        }
    }

    for name in ["Templates", "Template", "模板"] {
        push_template_directory(&mut directories, home, home.join(name));
    }

    directories
}

fn user_dirs_config_path(home: &Path) -> PathBuf {
    env::var_os("XDG_CONFIG_HOME")
        .map(PathBuf::from)
        .filter(|path| path.is_absolute())
        .unwrap_or_else(|| home.join(".config"))
        .join("user-dirs.dirs")
}

fn parse_user_dirs_templates_dir(contents: &str, home: &Path) -> Option<PathBuf> {
    contents.lines().find_map(|line| {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            return None;
        }

        let (key, value) = line.split_once('=')?;
        (key.trim() == "XDG_TEMPLATES_DIR")
            .then(|| parse_user_dirs_value(value))
            .flatten()
            .and_then(|value| expand_user_dir_value(&value, home))
    })
}

fn parse_user_dirs_value(value: &str) -> Option<String> {
    let value = value.trim();
    if value.is_empty() {
        return None;
    }

    if let Some(rest) = value.strip_prefix('"') {
        let end = rest.find('"')?;
        return Some(rest[..end].replace("\\\"", "\"").replace("\\\\", "\\"));
    }

    if let Some(rest) = value.strip_prefix('\'') {
        let end = rest.find('\'')?;
        return Some(rest[..end].to_string());
    }

    value
        .split_whitespace()
        .next()
        .map(|value| value.trim_end_matches(';').to_string())
        .filter(|value| !value.is_empty())
}

fn expand_user_dir_value(value: &str, home: &Path) -> Option<PathBuf> {
    if value == "$HOME" || value == "$HOME/" || value == "${HOME}" || value == "${HOME}/" {
        return Some(home.to_path_buf());
    }

    if let Some(rest) = value.strip_prefix("$HOME/") {
        return Some(home.join(rest));
    }

    if let Some(rest) = value.strip_prefix("${HOME}/") {
        return Some(home.join(rest));
    }

    let path = PathBuf::from(value);
    if path.is_absolute() {
        Some(path)
    } else {
        None
    }
}

fn push_template_directory(directories: &mut Vec<PathBuf>, home: &Path, path: PathBuf) {
    if path == home || directories.iter().any(|existing| existing == &path) {
        return;
    }

    if path.is_dir() {
        directories.push(path);
    }
}

fn template_files_in_dirs(directories: Vec<PathBuf>) -> Vec<TemplateFile> {
    let mut templates = Vec::new();

    for directory in directories {
        let Ok(entries) = fs::read_dir(directory) else {
            continue;
        };

        for entry in entries.flatten() {
            let path = entry.path();
            if templates
                .iter()
                .any(|template: &TemplateFile| template.path == path)
            {
                continue;
            }

            let label = entry.file_name().to_string_lossy().into_owned();
            if label.starts_with('.') {
                continue;
            }

            if fs::metadata(&path)
                .map(|metadata| metadata.is_file())
                .unwrap_or(false)
            {
                templates.push(TemplateFile {
                    label: template_file_label(&path, &label),
                    path,
                });
            }
        }
    }

    templates.sort_by(|left, right| {
        left.label
            .to_lowercase()
            .cmp(&right.label.to_lowercase())
            .then_with(|| left.label.cmp(&right.label))
            .then_with(|| left.path.cmp(&right.path))
    });
    templates
}

fn template_file_label(path: &Path, fallback: &str) -> String {
    path.file_stem()
        .map(|name| name.to_string_lossy().into_owned())
        .filter(|name| !name.is_empty())
        .unwrap_or_else(|| fallback.to_string())
}

pub(crate) fn load_directory_stream(
    request: DirectoryRequest,
    path: PathBuf,
    options: ScanOptions,
) -> Task<Message> {
    Task::stream(stream::channel(
        DIRECTORY_STREAM_BUFFER,
        move |mut output: iced_mpsc::Sender<Message>| async move {
            let mut scanner = match DirectoryScanner::new(path, options, DIRECTORY_BATCH_SIZE) {
                Ok(scanner) => scanner,
                Err(error) => {
                    let _ = output
                        .send(Message::DirectoryEvent(
                            request,
                            DirectoryLoadEvent::Failed(error),
                        ))
                        .await;
                    return;
                }
            };

            if output
                .send(Message::DirectoryEvent(
                    request.clone(),
                    DirectoryLoadEvent::Started,
                ))
                .await
                .is_err()
            {
                return;
            }

            loop {
                match scanner.next_batch() {
                    Ok(Some(entries)) => {
                        if output
                            .send(Message::DirectoryEvent(
                                request.clone(),
                                DirectoryLoadEvent::Batch(basic_entries(entries)),
                            ))
                            .await
                            .is_err()
                        {
                            return;
                        }
                    }
                    Ok(None) => break,
                    Err(error) => {
                        let _ = output
                            .send(Message::DirectoryEvent(
                                request,
                                DirectoryLoadEvent::Failed(error),
                            ))
                            .await;
                        return;
                    }
                }
            }

            let total = scanner.count();
            let _ = output
                .send(Message::DirectoryEvent(
                    request,
                    DirectoryLoadEvent::Finished { total },
                ))
                .await;
        },
    ))
}

pub(crate) fn refresh_entry_task(request_id: u64, path: PathBuf) -> Task<Message> {
    let message_path = path.clone();
    Task::perform(
        async move { entry_for_path(&path).map(decorated_entry) },
        move |result| Message::AutoRefreshEntryLoaded(request_id, message_path.clone(), result),
    )
}

pub(crate) fn load_auto_refresh_snapshot_task(
    request: AutoRefreshRequest,
    options: ScanOptions,
) -> Task<Message> {
    let path = request.path.clone();
    Task::perform(
        async move { load_directory_snapshot(path, options) },
        move |result| Message::AutoRefreshSnapshotLoaded(request.clone(), result),
    )
}

fn load_directory_snapshot(
    path: PathBuf,
    options: ScanOptions,
) -> Result<Vec<DisplayEntry>, FsError> {
    scan_dir(path, options).map(|listing| basic_entries(listing.entries))
}

pub(crate) fn decorate_auto_refresh_entries_task(
    request_id: u64,
    entries: Vec<FileEntry>,
) -> Task<Message> {
    Task::perform(
        async move { decorate_entry_batch(entries) },
        move |decorations| Message::AutoRefreshEntriesDecorated(request_id, decorations),
    )
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
        entries: basic_entries(results.entries),
    })
}

pub(crate) fn decorate_entries_task(request_id: u64, entries: Vec<FileEntry>) -> Task<Message> {
    Task::perform(
        async move { decorate_entry_batch(entries) },
        move |decorations| Message::EntriesDecorated(request_id, decorations),
    )
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

pub(crate) fn create_template_file_task(parent: PathBuf, template: PathBuf) -> Task<Message> {
    Task::perform(
        async move { create_file_from_template(parent, template) },
        Message::TemplateCreateFinished,
    )
}

pub(crate) fn delete_entries_task(paths: Vec<PathBuf>) -> Task<Message> {
    Task::perform(
        async move {
            let mut paths = paths;
            paths.sort_by(|left, right| {
                right
                    .components()
                    .count()
                    .cmp(&left.components().count())
                    .then_with(|| right.cmp(left))
            });

            let mut deleted = Vec::new();

            for path in paths {
                if let Err(error) = delete_entry(&path) {
                    return DeleteEntriesOutcome {
                        paths: deleted,
                        error: Some(error),
                    };
                }
                deleted.push(path);
            }

            DeleteEntriesOutcome {
                paths: deleted,
                error: None,
            }
        },
        Message::DeleteFinished,
    )
}

pub(crate) fn paste_paths_stream(
    operation_id: u64,
    sources: Vec<PathBuf>,
    destination: PathBuf,
    action: PasteAction,
    cancel_token: FileOperationCancelToken,
) -> Task<Message> {
    Task::stream(stream::channel(
        DIRECTORY_STREAM_BUFFER,
        move |mut output: iced_mpsc::Sender<Message>| async move {
            let (event_sender, event_receiver) = std_mpsc::channel::<FileOperationEvent>();
            let worker_cancel_token = cancel_token.clone();
            let output_cancel_token = cancel_token.clone();

            thread::spawn(move || {
                let progress_sender = event_sender.clone();
                let result = paste_paths_with_progress(
                    sources,
                    destination,
                    action,
                    worker_cancel_token,
                    |event| {
                        let _ = progress_sender.send(FileOperationEvent::Progress(event));
                    },
                );

                let _ = event_sender.send(FileOperationEvent::Finished(result));
            });

            while let Ok(event) = event_receiver.recv() {
                if output
                    .send(Message::FileOperationEvent(operation_id, event))
                    .await
                    .is_err()
                {
                    output_cancel_token.cancel();
                    break;
                }
            }
        },
    ))
}

pub(crate) fn remove_completed_file_operation_task(operation_id: u64) -> Task<Message> {
    Task::perform(
        async move {
            std::thread::sleep(Duration::from_secs(3));
            operation_id
        },
        Message::RemoveCompletedFileOperation,
    )
}

pub(crate) fn open_file_with_app_task(
    path: PathBuf,
    app: DesktopApp,
    default_mime: Option<String>,
) -> Task<Message> {
    Task::perform(
        async move {
            let app_id = app.id.clone();
            let app_name = open_file_with_app(path, app)?;
            let mut default_assignment = None;
            let mut default_error = None;

            if let Some(mime) = default_mime {
                match set_default_app_for_mime(&mime, &app_id) {
                    Ok(()) => {
                        default_assignment = Some(DefaultAppAssignment { mime, app_id });
                    }
                    Err(error) => {
                        default_error = Some(error);
                    }
                }
            }

            Ok(OpenFileOutcome {
                app_name,
                default_assignment,
                default_error,
            })
        },
        Message::OpenFileFinished,
    )
}

pub(crate) fn open_terminal(
    cwd: PathBuf,
    configured_terminal: Option<PathBuf>,
) -> Result<String, String> {
    if let Some(terminal) = configured_terminal {
        return open_configured_terminal(&terminal, &cwd);
    }

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

pub(crate) fn run_blank_menu_command(
    cwd: PathBuf,
    command: BlankMenuCommand,
) -> Result<String, String> {
    let args = expand_command_args(&command.args, &cwd);

    Command::new(&command.command)
        .args(args)
        .current_dir(&cwd)
        .spawn()
        .map_err(|error| format!("Failed to run {}: {error}", command.label))?;

    Ok(command.label)
}

fn expand_command_args(args: &[String], cwd: &Path) -> Vec<String> {
    let cwd = cwd.to_string_lossy();

    args.iter()
        .map(|arg| arg.replace("{cwd}", cwd.as_ref()))
        .collect()
}

fn open_configured_terminal(executable: &Path, cwd: &Path) -> Result<String, String> {
    let name = executable.to_string_lossy().into_owned();
    Command::new(executable)
        .current_dir(cwd)
        .spawn()
        .map_err(|error| format!("Failed to open {name}: {error}"))?;

    Ok(name)
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

#[cfg(test)]
mod tests {
    use super::*;
    use notify::event::{CreateKind, DataChange, Flag};
    use std::time::{SystemTime, UNIX_EPOCH};

    fn temp_dir(name: &str) -> TempDir {
        let id = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        let path =
            env::temp_dir().join(format!("filesystem-gui-{name}-{}-{id}", std::process::id()));
        fs::create_dir_all(&path).unwrap();
        TempDir { path }
    }

    struct TempDir {
        path: PathBuf,
    }

    impl TempDir {
        fn path(&self) -> &Path {
            &self.path
        }
    }

    impl Drop for TempDir {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.path);
        }
    }

    #[test]
    fn file_modify_event_maps_to_entry_change() {
        let fixture = temp_dir("file-modify");
        let path = fixture.path().join("file.txt");
        fs::write(&path, b"content").unwrap();
        let event = Event::new(EventKind::Modify(ModifyKind::Data(DataChange::Content)))
            .add_path(path.clone());

        let change = current_folder_change_for_event(fixture.path(), &event).unwrap();

        assert_eq!(change.kind, CurrentFolderChangeKind::Entry);
        assert_eq!(change.paths, vec![path]);
    }

    #[test]
    fn create_event_maps_to_structure_change() {
        let fixture = temp_dir("create");
        let path = fixture.path().join("file.txt");
        let event = Event::new(EventKind::Create(CreateKind::File)).add_path(path.clone());

        let change = current_folder_change_for_event(fixture.path(), &event).unwrap();

        assert_eq!(change.kind, CurrentFolderChangeKind::Structure);
        assert_eq!(change.paths, vec![path]);
    }

    #[test]
    fn nested_child_event_is_ignored() {
        let fixture = temp_dir("nested");
        let path = fixture.path().join("subdir/file.txt");
        let event = Event::new(EventKind::Create(CreateKind::File)).add_path(path);

        assert!(current_folder_change_for_event(fixture.path(), &event).is_none());
    }

    #[test]
    fn directory_modify_event_is_ignored() {
        let fixture = temp_dir("directory-modify");
        let path = fixture.path().join("subdir");
        fs::create_dir(&path).unwrap();
        let event = Event::new(EventKind::Modify(ModifyKind::Metadata(
            MetadataKind::WriteTime,
        )))
        .add_path(path);

        assert!(current_folder_change_for_event(fixture.path(), &event).is_none());
    }

    #[test]
    fn rescan_event_maps_to_rescan_even_without_paths() {
        let fixture = temp_dir("rescan");
        let event = Event::new(EventKind::Any).set_flag(Flag::Rescan);

        let change = current_folder_change_for_event(fixture.path(), &event).unwrap();

        assert_eq!(change.kind, CurrentFolderChangeKind::Rescan);
        assert!(change.paths.is_empty());
    }

    #[test]
    fn command_args_replace_cwd_placeholder() {
        let args = vec![
            "start".to_string(),
            "--cwd".to_string(),
            "{cwd}".to_string(),
            "path={cwd}".to_string(),
        ];

        assert_eq!(
            expand_command_args(&args, Path::new("/tmp/example")),
            vec!["start", "--cwd", "/tmp/example", "path=/tmp/example"]
        );
    }

    #[test]
    fn parse_user_dirs_templates_dir_expands_home() {
        let home = Path::new("/home/dingjing");
        let contents = r#"
            XDG_DESKTOP_DIR="$HOME/Desktop"
            XDG_TEMPLATES_DIR="$HOME/模板"
        "#;

        assert_eq!(
            parse_user_dirs_templates_dir(contents, home),
            Some(home.join("模板"))
        );
    }

    #[test]
    fn parse_user_dirs_templates_dir_accepts_absolute_paths() {
        let home = Path::new("/home/dingjing");

        assert_eq!(
            parse_user_dirs_templates_dir(r#"XDG_TEMPLATES_DIR="/data/templates""#, home),
            Some(PathBuf::from("/data/templates"))
        );
    }

    #[test]
    fn template_files_in_dirs_lists_regular_visible_files_sorted() {
        let fixture = temp_dir("template-files");
        let first = fixture.path().join("Templates");
        let second = fixture.path().join("模板");
        fs::create_dir(&first).unwrap();
        fs::create_dir(&second).unwrap();
        fs::write(first.join("B.txt"), b"b").unwrap();
        fs::write(first.join(".hidden.txt"), b"hidden").unwrap();
        fs::create_dir(first.join("Folder")).unwrap();
        fs::write(second.join("a.txt"), b"a").unwrap();

        let templates = template_files_in_dirs(vec![first.clone(), second.clone()]);

        assert_eq!(
            templates,
            vec![
                TemplateFile {
                    label: "a".to_string(),
                    path: second.join("a.txt"),
                },
                TemplateFile {
                    label: "B".to_string(),
                    path: first.join("B.txt"),
                },
            ]
        );
    }

    #[test]
    fn template_file_label_removes_only_last_extension() {
        assert_eq!(
            template_file_label(Path::new("/home/user/Templates/Report.docx"), "Report.docx"),
            "Report"
        );
        assert_eq!(
            template_file_label(
                Path::new("/home/user/Templates/archive.tar.gz"),
                "archive.tar.gz"
            ),
            "archive.tar"
        );
        assert_eq!(
            template_file_label(Path::new("/home/user/Templates/Makefile"), "Makefile"),
            "Makefile"
        );
    }
}
