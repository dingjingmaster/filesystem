use filesystem_core::{
    ChildPathLimits, ClipboardPaths, DirectoryListing, EntryKind, FileEntry, FolderProperties,
    FsError, ScanOptions, child_path_limits, create_file, create_folder, folder_properties,
    parse_clipboard_paths, paste_paths, rename_entry, scan_dir, search_file_names,
};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{
    button, checkbox, column, container, mouse_area, operation, row, scrollable, space, stack, svg,
    text, text_editor, text_input,
};
use iced::widget::{button as button_style, container as container_style, svg as svg_style};
use iced::{
    Background, Border, Color, Element, Fill, Length, Padding, Point, Rectangle,
    Result as IcedResult, Shadow, Size, Subscription, Task, Theme, mouse, window,
};
use std::collections::BTreeSet;
use std::env;
use std::fs;
use std::os::unix::ffi::OsStrExt;
use std::os::unix::fs::PermissionsExt;
use std::path::PathBuf;
use std::process::Command;
use std::time::{SystemTime, UNIX_EPOCH};

const APP_NAME_EN: &str = "File";
const APP_NAME_ZH: &str = "文件";
const WINDOW_ICON_SIZE: u32 = 128;
const WINDOW_INITIAL_WIDTH: f32 = 1220.0;
const WINDOW_INITIAL_HEIGHT: f32 = 760.0;
const WINDOW_MIN_WIDTH: f32 = 800.0;
const WINDOW_MIN_HEIGHT: f32 = 600.0;
const SIDEBAR_WIDTH: f32 = 240.0;
const TOOLBAR_HEIGHT: f32 = 54.0;
const RESIZE_HIT_SIZE: f32 = 6.0;
const TILE_WIDTH: f32 = 142.0;
const TILE_HEIGHT: f32 = 128.0;
const LIST_ROW_HEIGHT: f32 = 40.0;
const GRID_PADDING_TOP: f32 = 28.0;
const GRID_PADDING_LEFT: f32 = 36.0;
const GRID_ROW_SPACING: f32 = 22.0;
const GRID_COLUMN_SPACING: f32 = 28.0;
const LIST_PADDING_TOP: f32 = 18.0;
const LIST_PADDING_LEFT: f32 = 28.0;
const LIST_HEADER_HEIGHT: f32 = 34.0;
const SELECTION_DRAG_THRESHOLD: f32 = 3.0;
const CONTEXT_MENU_WIDTH: f32 = 184.0;
const PROPERTIES_DIALOG_WIDTH: f32 = 460.0;
const RENAME_INPUT_ID: &str = "file-rename-input";
const RENAME_TEXT_SIZE: f32 = 16.0;
const RENAME_LINE_HEIGHT: f32 = 1.5;
const RENAME_VERTICAL_PADDING: f32 = 14.0;
const RENAME_HORIZONTAL_PADDING: f32 = 10.0;
const RENAME_MIN_HEIGHT: f32 = 64.0;
const RENAME_AVERAGE_CHAR_WIDTH: f32 = 12.0;
const RENAME_MAX_WIDTH_MULTIPLIER: f32 = 3.0;
const LIST_RENAME_BASE_WIDTH: f32 = 340.0;
const GRID_RENAME_Y_OFFSET: f32 = 94.0;
const LIST_RENAME_X_OFFSET: f32 = 48.0;

pub fn main() -> IcedResult {
    iced::application(FileManager::new, FileManager::update, FileManager::view)
        .title(|manager: &FileManager| format!("{APP_NAME_EN} - {}", manager.cwd.display()))
        .theme(Theme::Dark)
        .window(window_settings())
        .subscription(FileManager::subscription)
        .run()
}

fn window_settings() -> window::Settings {
    let mut settings = window::Settings {
        size: Size::new(WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT),
        min_size: Some(Size::new(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT)),
        decorations: false,
        icon: load_window_icon(),
        ..window::Settings::default()
    };

    settings.platform_specific.application_id = APP_NAME_EN.to_string();
    settings
}

fn load_window_icon() -> Option<window::Icon> {
    let tree = resvg::usvg::Tree::from_data(
        include_bytes!("../../../icons/fs.svg"),
        &resvg::usvg::Options::default(),
    )
    .ok()?;

    let mut pixmap = resvg::tiny_skia::Pixmap::new(WINDOW_ICON_SIZE, WINDOW_ICON_SIZE)?;
    let size = tree.size();
    let transform = resvg::tiny_skia::Transform::from_scale(
        WINDOW_ICON_SIZE as f32 / size.width(),
        WINDOW_ICON_SIZE as f32 / size.height(),
    );

    resvg::render(&tree, transform, &mut pixmap.as_mut());

    let mut rgba = pixmap.take();
    unpremultiply_rgba(&mut rgba);

    window::icon::from_rgba(rgba, WINDOW_ICON_SIZE, WINDOW_ICON_SIZE).ok()
}

fn unpremultiply_rgba(rgba: &mut [u8]) {
    for pixel in rgba.chunks_exact_mut(4) {
        let alpha = pixel[3] as u32;

        if alpha == 0 {
            pixel[..3].fill(0);
        } else if alpha < 255 {
            for channel in &mut pixel[..3] {
                *channel = ((*channel as u32 * 255 + alpha / 2) / alpha).min(255) as u8;
            }
        }
    }
}

struct FileManager {
    cwd: PathBuf,
    entries: Vec<DisplayEntry>,
    show_hidden: bool,
    status: String,
    path_input: String,
    search_query: Option<String>,
    search_root: Option<PathBuf>,
    view_mode: ViewMode,
    menu_open: bool,
    view_submenu_open: bool,
    context_menu: Option<Point>,
    rename_state: Option<RenameState>,
    properties_dialog: Option<PropertiesDialog>,
    selected_paths: BTreeSet<PathBuf>,
    selection_drag: Option<SelectionDrag>,
    browser_pointer: Option<Point>,
    browser_scroll_y: f32,
    browser_width: f32,
    next_request_id: u64,
    active_request_id: Option<u64>,
    back_history: Vec<PathBuf>,
    forward_history: Vec<PathBuf>,
    home: Option<PathBuf>,
    home_shortcuts: Vec<HomeShortcut>,
}

#[derive(Debug, Clone)]
enum Message {
    Open(PathBuf, EntryKind),
    Go(PathBuf),
    Back,
    Forward,
    PathChanged(String),
    PathSubmit,
    SelectEntry(PathBuf),
    BrowserPointerMoved(Point),
    BrowserPressed,
    BrowserReleased,
    BrowserRightPressed,
    BrowserScrolled(f32),
    WindowResized(Size),
    ContextNewFile,
    ContextNewFolder,
    ContextPaste,
    ContextSelectAll,
    ContextOpenTerminal,
    ContextProperties,
    CreateFinished(NewEntryKind, Result<PathBuf, FsError>),
    RenameEditorAction(text_editor::Action),
    RenameSubmit,
    RenameFinished(Result<PathBuf, FsError>),
    PasteClipboardRead(Option<String>),
    PasteFinished(Result<Vec<PathBuf>, FsError>),
    TerminalOpened(Result<String, String>),
    PropertiesLoaded(Result<FolderProperties, FsError>),
    CloseProperties,
    SetPropertiesView(PropertiesView),
    ToggleHidden(bool),
    ToggleMenu,
    ToggleViewSubmenu,
    SetViewMode(ViewMode),
    DirectoryLoaded(DirectoryRequest, Result<DisplayListing, FsError>),
    SearchFinished(SearchRequest, Result<DisplaySearchResults, FsError>),
    HomeShortcutsLoaded(Vec<HomeShortcut>),
    WindowDrag,
    WindowClose,
    WindowMinimize,
    WindowToggleMaximize,
    WindowResize(window::Direction),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum NavKind {
    Home,
    Root,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum ViewMode {
    Icons,
    List,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum NewEntryKind {
    File,
    Folder,
}

#[derive(Debug, Clone)]
struct RenameState {
    path: PathBuf,
    fallback_name: String,
    value: String,
    content: text_editor::Content,
    limits: ChildPathLimits,
}

impl RenameState {
    fn new(path: PathBuf, value: String, limits: ChildPathLimits) -> Self {
        let mut content = text_editor::Content::with_text(&value);
        content.perform(text_editor::Action::SelectAll);

        Self {
            path,
            fallback_name: value.clone(),
            value,
            content,
            limits,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum PropertiesView {
    Summary,
    Permissions,
    Contents,
}

#[derive(Debug, Clone)]
enum PropertiesState {
    Loading(PathBuf),
    Loaded(FolderProperties),
    Error(String),
}

#[derive(Debug, Clone)]
struct PropertiesDialog {
    view: PropertiesView,
    state: PropertiesState,
}

impl ViewMode {
    fn label(self) -> &'static str {
        match self {
            Self::Icons => "图标视图",
            Self::List => "列表视图",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum DirectoryLoadMode {
    Replace,
    Visit,
    Back,
    Forward,
}

#[derive(Debug, Clone)]
struct DirectoryRequest {
    id: u64,
    path: PathBuf,
    mode: DirectoryLoadMode,
    previous: Option<PathBuf>,
}

#[derive(Debug, Clone)]
struct SearchRequest {
    id: u64,
}

#[derive(Debug, Clone)]
struct HomeShortcut {
    icon: &'static [u8],
    label: &'static str,
    path: PathBuf,
}

#[derive(Debug, Clone)]
struct DisplayEntry {
    file: FileEntry,
    icon: EntryIcon,
}

#[derive(Debug, Clone)]
enum EntryIcon {
    Embedded(&'static [u8]),
    Theme(Vec<u8>),
}

#[derive(Debug, Clone)]
struct DisplayListing {
    path: PathBuf,
    entries: Vec<DisplayEntry>,
}

#[derive(Debug, Clone)]
struct DisplaySearchResults {
    root: PathBuf,
    query: String,
    entries: Vec<DisplayEntry>,
}

#[derive(Debug, Clone, Copy)]
struct SelectionDrag {
    origin: Point,
    current: Point,
}

fn load_home_shortcuts(home: Option<PathBuf>) -> Task<Message> {
    Task::perform(
        async move { detect_home_shortcuts(home) },
        Message::HomeShortcutsLoaded,
    )
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

fn load_directory(path: PathBuf, options: ScanOptions) -> Result<DisplayListing, FsError> {
    let DirectoryListing { path, entries } = scan_dir(path, options)?;

    Ok(DisplayListing {
        path,
        entries: decorate_entries(entries),
    })
}

fn load_search(
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

fn decorate_entries(entries: Vec<FileEntry>) -> Vec<DisplayEntry> {
    let resolver = IconResolver::new();

    entries
        .into_iter()
        .map(|file| {
            let icon = resolver.entry_icon(&file);
            DisplayEntry { file, icon }
        })
        .collect()
}

struct IconResolver {
    roots: Vec<PathBuf>,
    themes: Vec<String>,
}

impl IconResolver {
    fn new() -> Self {
        let roots = icon_roots();
        let themes = icon_theme_names(&roots);

        Self { roots, themes }
    }

    fn entry_icon(&self, entry: &FileEntry) -> EntryIcon {
        match entry.kind {
            EntryKind::Directory => {
                EntryIcon::Embedded(include_bytes!("../../../icons/folder.svg"))
            }
            EntryKind::File => self
                .file_icon(entry)
                .map(EntryIcon::Theme)
                .unwrap_or_else(fallback_file_icon),
            EntryKind::Symlink | EntryKind::Other => fallback_file_icon(),
        }
    }

    fn file_icon(&self, entry: &FileEntry) -> Option<Vec<u8>> {
        let extension = entry
            .path
            .extension()
            .and_then(|value| value.to_str())
            .map(str::to_ascii_lowercase)?;
        let names = icon_names_for_extension(&extension);

        if names.is_empty() {
            return None;
        }

        self.find_icon(names)
    }

    fn find_icon(&self, names: &[&str]) -> Option<Vec<u8>> {
        for theme in &self.themes {
            for root in &self.roots {
                let theme_dir = root.join(theme);

                for directory in ICON_THEME_SUBDIRS {
                    let directory = theme_dir.join(directory);

                    for name in names {
                        for filename in icon_filenames(name) {
                            let path = directory.join(filename);
                            if path.is_file() {
                                if let Ok(bytes) = fs::read(path) {
                                    return Some(bytes);
                                }
                            }
                        }
                    }
                }
            }
        }

        None
    }
}

fn fallback_file_icon() -> EntryIcon {
    EntryIcon::Embedded(include_bytes!("../../../icons/file.svg"))
}

const ICON_THEME_SUBDIRS: &[&str] = &[
    "scalable/mimetypes",
    "symbolic/mimetypes",
    "mimetypes/scalable",
    "mimes/scalable",
    "mimes/scalable-light",
    "mimes/scalable-night",
    "scalable/apps",
    "apps/scalable",
    "scalable/devices",
    "symbolic/devices",
    "devices/scalable",
    "scalable/actions",
    "symbolic/actions",
    "actions/scalable",
    "actions/scalable-light",
    "actions/scalable-night",
];

fn icon_roots() -> Vec<PathBuf> {
    let mut roots = Vec::new();

    if let Some(home) = env::var_os("HOME").map(PathBuf::from) {
        push_unique_path(&mut roots, home.join(".local/share/icons"));
        push_unique_path(&mut roots, home.join(".icons"));
    }

    let data_dirs =
        env::var_os("XDG_DATA_DIRS").unwrap_or_else(|| "/usr/local/share:/usr/share".into());
    for directory in env::split_paths(&data_dirs) {
        push_unique_path(&mut roots, directory.join("icons"));
    }

    roots
}

fn icon_theme_names(roots: &[PathBuf]) -> Vec<String> {
    let mut themes = Vec::new();

    for variable in [
        "FILE_ICON_THEME",
        "XDG_ICON_THEME",
        "GTK_ICON_THEME",
        "ICON_THEME",
    ] {
        if let Ok(theme) = env::var(variable) {
            push_unique_string(&mut themes, theme);
        }
    }

    for theme in ["Adwaita", "hicolor", "HighContrast", "ContrastHigh"] {
        push_unique_string(&mut themes, theme.to_string());
    }

    let mut discovered = Vec::new();
    for root in roots {
        let Ok(entries) = fs::read_dir(root) else {
            continue;
        };

        for entry in entries.flatten() {
            if entry.path().is_dir() {
                if let Some(name) = entry.file_name().to_str() {
                    discovered.push(name.to_string());
                }
            }
        }
    }

    discovered.sort();
    for theme in discovered {
        push_unique_string(&mut themes, theme);
    }

    themes
}

fn icon_filenames(name: &str) -> [String; 2] {
    [format!("{name}.svg"), format!("{name}-symbolic.svg")]
}

fn icon_names_for_extension(extension: &str) -> &'static [&'static str] {
    match extension {
        "txt" | "md" | "markdown" | "log" | "ini" | "conf" | "toml" | "yaml" | "yml" | "json"
        | "xml" | "html" | "css" | "scss" | "rs" | "c" | "h" | "cpp" | "hpp" | "js" | "jsx"
        | "ts" | "tsx" | "py" | "go" | "java" | "sh" | "bash" | "zsh" => &["text-x-generic"],
        "png" | "jpg" | "jpeg" | "gif" | "webp" | "bmp" | "tif" | "tiff" | "svg" | "ico" => {
            &["image-x-generic"]
        }
        "mp3" | "flac" | "wav" | "ogg" | "opus" | "m4a" | "aac" => &["audio-x-generic"],
        "mp4" | "mkv" | "mov" | "webm" | "avi" | "flv" | "wmv" => &["video-x-generic"],
        "pdf" => &["application-pdf", "gnome-mime-application-pdf"],
        "zip" => &["application-zip", "package-x-generic", "media-zip"],
        "tar" | "gz" | "tgz" | "xz" | "bz2" | "7z" | "rar" => &[
            "package-x-generic",
            "application-zip",
            "application-x-gzip",
            "media-zip",
        ],
        "ods" | "xls" | "xlsx" | "csv" => &["x-office-spreadsheet"],
        "odp" | "ppt" | "pptx" => &["x-office-presentation"],
        "odt" | "doc" | "docx" => &["x-office-document", "text-x-generic"],
        _ => &[],
    }
}

fn push_unique_path(paths: &mut Vec<PathBuf>, path: PathBuf) {
    if path.is_dir() && !paths.iter().any(|existing| existing == &path) {
        paths.push(path);
    }
}

fn push_unique_string(values: &mut Vec<String>, value: String) {
    if !value.is_empty() && !values.iter().any(|existing| existing == &value) {
        values.push(value);
    }
}

impl FileManager {
    fn new() -> (Self, Task<Message>) {
        let cwd = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("/"));
        let home = std::env::var_os("HOME").map(PathBuf::from);
        let shortcuts_task = load_home_shortcuts(home.clone());
        let mut manager = Self {
            cwd,
            entries: Vec::new(),
            show_hidden: false,
            status: String::new(),
            path_input: String::new(),
            search_query: None,
            search_root: None,
            view_mode: ViewMode::Icons,
            menu_open: false,
            view_submenu_open: false,
            context_menu: None,
            rename_state: None,
            properties_dialog: None,
            selected_paths: BTreeSet::new(),
            selection_drag: None,
            browser_pointer: None,
            browser_scroll_y: 0.0,
            browser_width: browser_width_from_window(WINDOW_INITIAL_WIDTH),
            next_request_id: 0,
            active_request_id: None,
            back_history: Vec::new(),
            forward_history: Vec::new(),
            home,
            home_shortcuts: Vec::new(),
        };
        let task = manager.reload();
        (manager, Task::batch([task, shortcuts_task]))
    }

    fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Open(path, kind) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_path(path, kind)
            }
            Message::Go(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.visit_path(path)
            }
            Message::Back => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.go_back()
            }
            Message::Forward => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.go_forward()
            }
            Message::PathChanged(path) => {
                self.path_input = path;
                Task::none()
            }
            Message::PathSubmit => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_path_input()
            }
            Message::SelectEntry(path) => {
                if self
                    .rename_state
                    .as_ref()
                    .is_some_and(|rename| rename.path != path)
                {
                    return self.submit_rename();
                }
                self.close_menu();
                self.selection_drag = None;
                self.selected_paths.clear();
                self.selected_paths.insert(path);
                Task::none()
            }
            Message::BrowserPointerMoved(position) => {
                self.browser_pointer = Some(position);
                if let Some(drag) = &mut self.selection_drag {
                    drag.current = position;
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::BrowserPressed => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                if let Some(position) = self.browser_pointer {
                    self.selection_drag = Some(SelectionDrag {
                        origin: position,
                        current: position,
                    });
                    self.selected_paths.clear();
                }
                Task::none()
            }
            Message::BrowserReleased => {
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                    self.selection_drag = None;
                }
                Task::none()
            }
            Message::BrowserRightPressed => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.menu_open = false;
                self.view_submenu_open = false;
                self.selection_drag = None;

                if let Some(position) = self.browser_pointer {
                    if self.pointer_over_entry(position) {
                        self.context_menu = None;
                    } else {
                        self.context_menu = Some(position);
                    }
                }

                Task::none()
            }
            Message::BrowserScrolled(offset_y) => {
                self.browser_scroll_y = offset_y;
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::WindowResized(size) => {
                self.browser_width = browser_width_from_window(size.width);
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::ContextNewFile => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Creating file...".to_string();
                create_entry_task(self.cwd.clone(), NewEntryKind::File)
            }
            Message::ContextNewFolder => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Creating folder...".to_string();
                create_entry_task(self.cwd.clone(), NewEntryKind::Folder)
            }
            Message::ContextPaste => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Reading clipboard...".to_string();
                iced::clipboard::read().map(Message::PasteClipboardRead)
            }
            Message::ContextSelectAll => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.selected_paths = self
                    .entries
                    .iter()
                    .map(|entry| entry.file.path.clone())
                    .collect();
                Task::none()
            }
            Message::ContextOpenTerminal => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Opening terminal...".to_string();
                let cwd = self.cwd.clone();
                Task::perform(async move { open_terminal(cwd) }, Message::TerminalOpened)
            }
            Message::ContextProperties => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                let path = self.cwd.clone();
                self.properties_dialog = Some(PropertiesDialog {
                    view: PropertiesView::Summary,
                    state: PropertiesState::Loading(path.clone()),
                });
                Task::perform(
                    async move { folder_properties(path) },
                    Message::PropertiesLoaded,
                )
            }
            Message::CreateFinished(kind, result) => match result {
                Ok(path) => {
                    let value = path
                        .file_name()
                        .map(|name| name.to_string_lossy().into_owned())
                        .unwrap_or_default();
                    let limits = path
                        .parent()
                        .and_then(|parent| child_path_limits(parent).ok())
                        .unwrap_or_default();
                    self.rename_state = Some(RenameState::new(path.clone(), value, limits));
                    self.status = match kind {
                        NewEntryKind::File => "File created; enter a new name".to_string(),
                        NewEntryKind::Folder => "Folder created; enter a new name".to_string(),
                    };
                    self.reload()
                }
                Err(error) => {
                    self.status = error.to_string();
                    Task::none()
                }
            },
            Message::RenameEditorAction(action) => {
                if matches!(action, text_editor::Action::Edit(text_editor::Edit::Enter)) {
                    return self.submit_rename();
                }

                if let Some(rename) = &mut self.rename_state {
                    let previous_content = rename.content.clone();
                    let previous_value = rename.value.clone();
                    rename.content.perform(action);
                    let value = rename.content.text();

                    if let Some(message) = rename_limit_error(rename, &value) {
                        rename.content = previous_content;
                        rename.value = previous_value;
                        self.status = message;
                        return self.focus_rename_input(false);
                    }

                    rename.value = value;
                }
                Task::none()
            }
            Message::RenameSubmit => self.submit_rename(),
            Message::RenameFinished(result) => match result {
                Ok(path) => {
                    self.rename_state = None;
                    self.selected_paths.clear();
                    self.selected_paths.insert(path);
                    self.status = "Renamed".to_string();
                    self.reload()
                }
                Err(error) => {
                    if error.kind() == std::io::ErrorKind::AlreadyExists {
                        self.status = "Name already exists; enter another name".to_string();
                    } else if error.is_name_too_long()
                        && self.rename_state.as_ref().is_some_and(|rename| {
                            rename.limits.name_bytes.is_none() || rename.limits.path_bytes.is_none()
                        })
                    {
                        let Some(rename) = self.rename_state.take() else {
                            return Task::none();
                        };
                        self.selected_paths.clear();
                        self.selected_paths.insert(rename.path);
                        self.status = format!(
                            "Name is too long; kept default name {}",
                            rename.fallback_name
                        );
                        return Task::none();
                    } else {
                        self.status = error.to_string();
                    }
                    self.prepare_rename_retry()
                }
            },
            Message::PasteClipboardRead(contents) => {
                let Some(contents) = contents else {
                    self.status = "Clipboard does not contain local paths".to_string();
                    return Task::none();
                };

                let ClipboardPaths { action, paths } = parse_clipboard_paths(&contents);

                if paths.is_empty() {
                    self.status = "Clipboard does not contain local paths".to_string();
                    return Task::none();
                }

                let destination = self.cwd.clone();
                self.status = "Pasting...".to_string();
                Task::perform(
                    async move { paste_paths(paths, destination, action) },
                    Message::PasteFinished,
                )
            }
            Message::PasteFinished(result) => match result {
                Ok(paths) => {
                    self.status = format!("Pasted {} item(s)", paths.len());
                    self.reload()
                }
                Err(error) => {
                    self.status = error.to_string();
                    Task::none()
                }
            },
            Message::TerminalOpened(result) => {
                self.status = match result {
                    Ok(name) => format!("Opened {name}"),
                    Err(error) => error,
                };
                Task::none()
            }
            Message::PropertiesLoaded(result) => {
                if let Some(dialog) = &mut self.properties_dialog {
                    dialog.state = match result {
                        Ok(properties) => PropertiesState::Loaded(properties),
                        Err(error) => PropertiesState::Error(error.to_string()),
                    };
                }
                Task::none()
            }
            Message::CloseProperties => {
                self.properties_dialog = None;
                Task::none()
            }
            Message::SetPropertiesView(view) => {
                if let Some(dialog) = &mut self.properties_dialog {
                    dialog.view = view;
                }
                Task::none()
            }
            Message::ToggleHidden(show_hidden) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.show_hidden = show_hidden;
                if self.search_query.is_some() {
                    self.rerun_search()
                } else {
                    self.reload()
                }
            }
            Message::ToggleMenu => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.context_menu = None;
                self.menu_open = !self.menu_open;
                if !self.menu_open {
                    self.view_submenu_open = false;
                }
                Task::none()
            }
            Message::ToggleViewSubmenu => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.view_submenu_open = !self.view_submenu_open;
                Task::none()
            }
            Message::SetViewMode(view_mode) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.view_mode = view_mode;
                self.close_menu();
                Task::none()
            }
            Message::DirectoryLoaded(request, result) => {
                self.directory_loaded(request, result);
                self.focus_rename_input(true)
            }
            Message::SearchFinished(request, result) => {
                self.search_finished(request, result);
                Task::none()
            }
            Message::HomeShortcutsLoaded(shortcuts) => {
                self.home_shortcuts = shortcuts;
                Task::none()
            }
            Message::WindowDrag => latest_window_task(window::drag),
            Message::WindowClose => latest_window_task(window::close),
            Message::WindowMinimize => latest_window_task(|id| window::minimize(id, true)),
            Message::WindowToggleMaximize => latest_window_task(window::toggle_maximize),
            Message::WindowResize(direction) => {
                latest_window_task(move |id| window::drag_resize(id, direction))
            }
        }
    }

    fn subscription(&self) -> Subscription<Message> {
        window::resize_events().map(|(_id, size)| Message::WindowResized(size))
    }

    fn view(&self) -> Element<'_, Message> {
        let shell = row![self.sidebar(), self.main_area()]
            .height(Fill)
            .width(Fill);

        let shell = container(shell)
            .height(Fill)
            .width(Fill)
            .style(style::app_background);

        stack([shell.into(), resize_layer(), self.properties_overlay()])
            .height(Fill)
            .width(Fill)
            .into()
    }

    fn sidebar(&self) -> Element<'_, Message> {
        let top_drag = mouse_area(
            container(text(APP_NAME_ZH).size(16).style(style::primary_text))
                .height(TOOLBAR_HEIGHT)
                .width(Fill)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center)
                .style(style::sidebar_drag_area),
        )
        .on_press(Message::WindowDrag)
        .on_double_click(Message::WindowToggleMaximize);

        let quick = column![
            self.nav_item(
                NavKind::Home,
                include_bytes!("../../../icons/home.svg"),
                "主文件夹"
            ),
            self.nav_item(
                NavKind::Root,
                include_bytes!("../../../icons/root.svg"),
                "根目录"
            ),
        ]
        .spacing(4);

        let mut content = column![quick].spacing(8).padding([14, 10]);

        if !self.home_shortcuts.is_empty() {
            content = content.push(style::divider());

            for shortcut in &self.home_shortcuts {
                content = content.push(self.nav_path(
                    shortcut.icon,
                    shortcut.label,
                    shortcut.path.clone(),
                ));
            }
        }

        let sidebar = column![top_drag, container(content).height(Fill).width(Fill)].spacing(0);

        container(sidebar)
            .width(SIDEBAR_WIDTH)
            .height(Fill)
            .style(style::sidebar)
            .into()
    }

    fn main_area(&self) -> Element<'_, Message> {
        let content = column![self.toolbar(), self.browser_view()]
            .height(Fill)
            .width(Fill)
            .spacing(0);

        let content = container(content)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into();

        if self.menu_open {
            stack([content, self.toolbar_menu()])
                .height(Fill)
                .width(Fill)
                .into()
        } else {
            content
        }
    }

    fn toolbar(&self) -> Element<'_, Message> {
        let can_go_back = !self.back_history.is_empty();
        let back = button(toolbar_icon(
            include_bytes!("../../../icons/left.svg"),
            can_go_back,
        ))
        .height(36)
        .width(36)
        .padding(6)
        .style(style::toolbar_button)
        .on_press_maybe(can_go_back.then_some(Message::Back));

        let can_go_forward = !self.forward_history.is_empty();
        let forward = button(toolbar_icon(
            include_bytes!("../../../icons/right.svg"),
            can_go_forward,
        ))
        .height(36)
        .width(36)
        .padding(6)
        .style(style::toolbar_button)
        .on_press_maybe(can_go_forward.then_some(Message::Forward));

        let path_bar = text_input("输入绝对路径或文件名正则", &self.path_input)
            .on_input(Message::PathChanged)
            .on_submit(Message::PathSubmit)
            .size(15)
            .width(Fill)
            .padding([8, 12])
            .style(style::path_input);

        let path_bar = container(path_bar).height(38).width(Fill);

        let menu = button(toolbar_icon(
            include_bytes!("../../../icons/menu.svg"),
            true,
        ))
        .height(36)
        .width(36)
        .padding(6)
        .style(style::toolbar_button)
        .on_press(Message::ToggleMenu);

        let drag_region = mouse_area(
            container(space())
                .height(38)
                .width(70)
                .style(style::toolbar_drag_area),
        )
        .on_press(Message::WindowDrag)
        .on_double_click(Message::WindowToggleMaximize);

        let toolbar = row![
            back,
            forward,
            path_bar,
            drag_region,
            menu,
            space().width(8),
            button(window_icon(include_bytes!("../../../icons/min.svg")))
                .height(36)
                .width(36)
                .padding(6)
                .style(style::window_button)
                .on_press(Message::WindowMinimize),
            button(window_icon(include_bytes!("../../../icons/max.svg")))
                .height(36)
                .width(36)
                .padding(6)
                .style(style::window_button)
                .on_press(Message::WindowToggleMaximize),
            button(window_icon(include_bytes!("../../../icons/close.svg")))
                .height(36)
                .width(36)
                .padding(6)
                .style(style::close_button)
                .on_press(Message::WindowClose),
        ]
        .align_y(iced::Center)
        .spacing(10)
        .padding([8, 12])
        .height(TOOLBAR_HEIGHT);

        container(toolbar)
            .height(TOOLBAR_HEIGHT)
            .width(Fill)
            .style(style::toolbar)
            .into()
    }

    fn toolbar_menu(&self) -> Element<'_, Message> {
        let view_item = button(
            row![
                text("视图").size(14).style(style::primary_text),
                space().width(Fill),
                text("›").size(17).style(style::muted_text),
            ]
            .align_y(iced::Center)
            .width(Fill),
        )
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, self.view_submenu_open))
        .on_press(Message::ToggleViewSubmenu);

        let hidden_item = container(
            checkbox(self.show_hidden)
                .label("显示隐藏文件")
                .on_toggle(Message::ToggleHidden)
                .size(16)
                .text_size(13)
                .style(style::checkbox),
        )
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .align_y(Vertical::Center);

        let mut menu = row![
            container(column![view_item, hidden_item].spacing(4).padding(6))
                .width(178)
                .style(style::menu_panel)
        ]
        .spacing(6)
        .align_y(iced::Alignment::Start);

        if self.view_submenu_open {
            menu = menu.push(
                container(
                    column![
                        self.view_option(ViewMode::Icons),
                        self.view_option(ViewMode::List),
                    ]
                    .spacing(4)
                    .padding(6),
                )
                .width(160)
                .style(style::menu_panel),
            );
        }

        container(row![space().width(Fill), menu])
            .height(Fill)
            .width(Fill)
            .padding(
                Padding::default()
                    .top(TOOLBAR_HEIGHT)
                    .right(168.0)
                    .left(12.0),
            )
            .into()
    }

    fn view_option(&self, view_mode: ViewMode) -> Element<'_, Message> {
        let active = self.view_mode == view_mode;
        let marker = if active { "✓" } else { "" };

        button(
            row![
                container(text(marker).size(13).style(style::primary_text))
                    .width(20)
                    .align_x(Horizontal::Center),
                text(view_mode.label()).size(14).style(style::primary_text),
            ]
            .spacing(8)
            .align_y(iced::Center)
            .width(Fill),
        )
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, active))
        .on_press(Message::SetViewMode(view_mode))
        .into()
    }

    fn browser_view(&self) -> Element<'_, Message> {
        let content = match self.view_mode {
            ViewMode::Icons => self.grid(),
            ViewMode::List => self.list(),
        };

        let content = mouse_area(content)
            .on_move(Message::BrowserPointerMoved)
            .on_press(Message::BrowserPressed)
            .on_release(Message::BrowserReleased)
            .on_right_press(Message::BrowserRightPressed);

        stack([
            content.into(),
            self.selection_overlay(),
            self.rename_overlay(),
            self.context_menu_overlay(),
        ])
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn grid(&self) -> Element<'_, Message> {
        let columns = self.icon_grid_columns();
        let mut rows = column![]
            .spacing(GRID_ROW_SPACING)
            .padding([GRID_PADDING_TOP as u16, GRID_PADDING_LEFT as u16]);

        if self.entries.is_empty() {
            rows = rows.push(
                container(
                    column![
                        text("没有可显示的项目").size(18).style(style::primary_text),
                        text(&self.status).size(13).style(style::muted_text),
                    ]
                    .spacing(8)
                    .align_x(iced::Center),
                )
                .width(Fill)
                .height(220)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center),
            );
        } else {
            for chunk in self.entries.chunks(columns) {
                let mut row = row![].spacing(GRID_COLUMN_SPACING).height(TILE_HEIGHT);

                for entry in chunk {
                    row = row.push(self.tile(entry));
                }

                rows = rows.push(row);
            }
        }

        let footer = row![
            text(&self.status).size(13).style(style::muted_text),
            space().width(Fill),
            text(format!("{} 项", self.entries.len()))
                .size(13)
                .style(style::muted_text),
        ]
        .align_y(iced::Center);

        let footer = container(footer).padding([0, 36]).height(34).width(Fill);

        let view = column![
            scrollable(rows)
                .on_scroll(|viewport| Message::BrowserScrolled(viewport.absolute_offset().y))
                .height(Fill)
                .width(Fill),
            footer
        ]
        .height(Fill)
        .width(Fill);

        container(view)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into()
    }

    fn list(&self) -> Element<'_, Message> {
        let mut rows = column![self.list_header()].spacing(0).padding(
            Padding::default()
                .top(LIST_PADDING_TOP)
                .right(LIST_PADDING_LEFT)
                .left(LIST_PADDING_LEFT),
        );

        if self.entries.is_empty() {
            rows = rows.push(
                container(
                    column![
                        text("没有可显示的项目").size(18).style(style::primary_text),
                        text(&self.status).size(13).style(style::muted_text),
                    ]
                    .spacing(8)
                    .align_x(iced::Center),
                )
                .width(Fill)
                .height(220)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center),
            );
        } else {
            for entry in &self.entries {
                rows = rows.push(self.list_row(entry));
            }
        }

        let footer = row![
            text(&self.status).size(13).style(style::muted_text),
            space().width(Fill),
            text(format!("{} 项", self.entries.len()))
                .size(13)
                .style(style::muted_text),
        ]
        .align_y(iced::Center);

        let footer = container(footer).padding([0, 36]).height(34).width(Fill);

        let view = column![
            scrollable(rows)
                .on_scroll(|viewport| Message::BrowserScrolled(viewport.absolute_offset().y))
                .height(Fill)
                .width(Fill),
            footer
        ]
        .height(Fill)
        .width(Fill);

        container(view)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into()
    }

    fn list_header(&self) -> Element<'_, Message> {
        container(
            row![
                list_header_cell("名称", Length::FillPortion(5)),
                list_header_cell("大小", Length::FillPortion(2)),
                list_header_cell("所有者", Length::FillPortion(2)),
                list_header_cell("修改时间", Length::FillPortion(3)),
            ]
            .spacing(12)
            .align_y(iced::Center)
            .height(LIST_HEADER_HEIGHT),
        )
        .width(Fill)
        .padding([0, 14])
        .style(style::list_header)
        .into()
    }

    fn list_row(&self, entry: &DisplayEntry) -> Element<'_, Message> {
        let selected = self.is_selected(entry);
        let size = entry_size(&entry.file);
        let owner = entry_owner(&entry.file);
        let modified = format_modified(entry.file.modified);
        let name_cell = list_name_cell(
            entry,
            short_list_text(&self.entry_display_name(entry)),
            Length::FillPortion(5),
        );

        let content = row![
            name_cell,
            list_value_cell(size, Length::FillPortion(2), false),
            list_value_cell(owner, Length::FillPortion(2), false),
            list_value_cell(modified, Length::FillPortion(3), false),
        ]
        .spacing(12)
        .align_y(iced::Center)
        .height(LIST_ROW_HEIGHT);

        let row = container(content)
            .height(LIST_ROW_HEIGHT)
            .width(Fill)
            .padding([0, 14])
            .style(move |_| style::list_row_container(selected));

        mouse_area(row)
            .interaction(mouse::Interaction::Pointer)
            .on_press(Message::SelectEntry(entry.file.path.clone()))
            .on_double_click(Message::Open(entry.file.path.clone(), entry.file.kind))
            .into()
    }

    fn tile(&self, entry: &DisplayEntry) -> Element<'_, Message> {
        let selected = self.is_selected(entry);
        let name: Element<'_, Message> = text(short_name(&entry.file.name))
            .size(15)
            .width(TILE_WIDTH)
            .align_x(Horizontal::Center)
            .style(style::primary_text)
            .into();

        let meta = text(self.entry_subtitle(entry))
            .size(12)
            .width(TILE_WIDTH)
            .align_x(Horizontal::Center)
            .style(style::muted_text);

        let content = column![entry_icon(&entry.icon, 78.0), name, meta]
            .spacing(8)
            .align_x(iced::Center)
            .width(TILE_WIDTH);

        let tile = container(content)
            .height(TILE_HEIGHT)
            .width(TILE_WIDTH)
            .padding(8)
            .style(move |_| style::tile_container(selected));

        mouse_area(tile)
            .interaction(mouse::Interaction::Pointer)
            .on_press(Message::SelectEntry(entry.file.path.clone()))
            .on_double_click(Message::Open(entry.file.path.clone(), entry.file.kind))
            .into()
    }

    fn nav_item(
        &self,
        kind: NavKind,
        icon: &'static [u8],
        label: &'static str,
    ) -> Element<'_, Message> {
        let path = match kind {
            NavKind::Home => self.home_path(),
            NavKind::Root => PathBuf::from("/"),
        };

        self.nav_path(icon, label, path)
    }

    fn nav_path(
        &self,
        icon: &'static [u8],
        label: &'static str,
        path: PathBuf,
    ) -> Element<'_, Message> {
        let active = path == self.cwd;
        let content = row![
            sidebar_icon(icon),
            text(label).size(15).style(style::primary_text),
        ]
        .spacing(10)
        .align_y(iced::Center)
        .width(Fill);

        let content = container(content)
            .height(Fill)
            .width(Fill)
            .align_x(Horizontal::Left)
            .align_y(Vertical::Center);

        button(content)
            .height(36)
            .width(Fill)
            .padding([0, 12])
            .style(move |theme, status| style::sidebar_button(theme, status, active))
            .on_press(Message::Go(path))
            .into()
    }

    fn home_path(&self) -> PathBuf {
        self.home.clone().unwrap_or_else(|| PathBuf::from("/"))
    }

    fn open_path(&mut self, path: PathBuf, kind: EntryKind) -> Task<Message> {
        if matches!(kind, EntryKind::Directory) {
            self.visit_path(path)
        } else {
            let name = path
                .file_name()
                .and_then(|value| value.to_str())
                .unwrap_or("file");
            self.status = format!("No opener configured for {name}");
            Task::none()
        }
    }

    fn open_path_input(&mut self) -> Task<Message> {
        let input = self.path_input.trim().to_string();
        if input.is_empty() {
            self.status = "Path or search is empty".to_string();
            return Task::none();
        }

        let path = PathBuf::from(&input);
        if !path.is_absolute() {
            return self.search_current_dir(input);
        }

        self.visit_path(path)
    }

    fn visit_path(&mut self, path: PathBuf) -> Task<Message> {
        if path == self.cwd {
            return self.reload();
        }

        let previous = self.cwd.clone();
        self.load_path(path, DirectoryLoadMode::Visit, Some(previous))
    }

    fn go_back(&mut self) -> Task<Message> {
        if let Some(path) = self.back_history.last().cloned() {
            let previous = self.cwd.clone();
            self.load_path(path, DirectoryLoadMode::Back, Some(previous))
        } else {
            Task::none()
        }
    }

    fn go_forward(&mut self) -> Task<Message> {
        if let Some(path) = self.forward_history.last().cloned() {
            let previous = self.cwd.clone();
            self.load_path(path, DirectoryLoadMode::Forward, Some(previous))
        } else {
            Task::none()
        }
    }

    fn reload(&mut self) -> Task<Message> {
        self.load_path(self.cwd.clone(), DirectoryLoadMode::Replace, None)
    }

    fn search_current_dir(&mut self, query: String) -> Task<Message> {
        if query.trim().is_empty() {
            self.status = "Search is empty".to_string();
            return Task::none();
        }

        let id = self.next_request_id();
        let root = self.cwd.clone();
        let options = ScanOptions {
            show_hidden: self.show_hidden,
        };
        let request = SearchRequest { id };

        self.active_request_id = Some(id);
        self.search_query = Some(query.clone());
        self.search_root = Some(root.clone());
        self.status = format!("Searching names for /{query}/...");

        Task::perform(
            async move { load_search(root, query, options) },
            move |result| Message::SearchFinished(request.clone(), result),
        )
    }

    fn rerun_search(&mut self) -> Task<Message> {
        let Some(query) = self.search_query.clone() else {
            return Task::none();
        };

        self.search_current_dir(query)
    }

    fn load_path(
        &mut self,
        path: PathBuf,
        mode: DirectoryLoadMode,
        previous: Option<PathBuf>,
    ) -> Task<Message> {
        let id = self.next_request_id();
        let options = ScanOptions {
            show_hidden: self.show_hidden,
        };
        let request = DirectoryRequest {
            id,
            path: path.clone(),
            mode,
            previous,
        };

        self.active_request_id = Some(id);
        self.status = format!("Loading {}...", path.display());

        Task::perform(
            async move { load_directory(path, options) },
            move |result| Message::DirectoryLoaded(request.clone(), result),
        )
    }

    fn directory_loaded(
        &mut self,
        request: DirectoryRequest,
        result: Result<DisplayListing, FsError>,
    ) {
        if self.active_request_id != Some(request.id) {
            return;
        }

        self.active_request_id = None;

        match result {
            Ok(DisplayListing { path, entries }) => {
                let count = entries.len();

                match request.mode {
                    DirectoryLoadMode::Replace => {}
                    DirectoryLoadMode::Visit => {
                        if let Some(previous) = request.previous {
                            self.back_history.push(previous);
                            self.forward_history.clear();
                        }
                    }
                    DirectoryLoadMode::Back => {
                        if self.back_history.last() == Some(&request.path) {
                            self.back_history.pop();
                        }
                        if let Some(previous) = request.previous {
                            self.forward_history.push(previous);
                        }
                    }
                    DirectoryLoadMode::Forward => {
                        if self.forward_history.last() == Some(&request.path) {
                            self.forward_history.pop();
                        }
                        if let Some(previous) = request.previous {
                            self.back_history.push(previous);
                        }
                    }
                }

                self.cwd = path;
                self.entries = entries;
                self.path_input = self.cwd.display().to_string();
                self.search_query = None;
                self.search_root = None;
                self.clear_selection_state();
                self.status = format!("{count} entries");
            }
            Err(error) => {
                self.clear_selection_state();
                self.set_scan_error(error);
            }
        }
    }

    fn search_finished(
        &mut self,
        request: SearchRequest,
        result: Result<DisplaySearchResults, FsError>,
    ) {
        if self.active_request_id != Some(request.id) {
            return;
        }

        self.active_request_id = None;

        match result {
            Ok(results) => {
                let count = results.entries.len();
                self.entries = results.entries;
                self.search_query = Some(results.query);
                self.search_root = Some(results.root);
                self.clear_selection_state();
                self.status = format!("{count} name matches");
            }
            Err(error) => {
                self.clear_selection_state();
                self.set_scan_error(error);
            }
        }
    }

    fn set_scan_error(&mut self, error: FsError) {
        self.status = error.to_string();
    }

    fn next_request_id(&mut self) -> u64 {
        self.next_request_id += 1;
        self.next_request_id
    }

    fn entry_subtitle(&self, entry: &DisplayEntry) -> String {
        let Some(root) = &self.search_root else {
            return entry_meta(&entry.file);
        };

        entry
            .file
            .path
            .strip_prefix(root)
            .map(|path| path.display().to_string())
            .unwrap_or_else(|_| entry.file.path.display().to_string())
    }

    fn entry_display_name(&self, entry: &DisplayEntry) -> String {
        let Some(root) = &self.search_root else {
            return entry.file.name.clone();
        };

        entry
            .file
            .path
            .strip_prefix(root)
            .map(|path| path.display().to_string())
            .unwrap_or_else(|_| entry.file.name.clone())
    }

    fn is_selected(&self, entry: &DisplayEntry) -> bool {
        self.selected_paths.contains(&entry.file.path)
    }

    fn clear_selection_state(&mut self) {
        self.selected_paths.clear();
        self.selection_drag = None;
        self.browser_pointer = None;
        self.browser_scroll_y = 0.0;
    }

    fn icon_grid_columns(&self) -> usize {
        icon_grid_columns(self.browser_width)
    }

    fn select_entries_in_drag_rect(&mut self) {
        let Some(selection) = self.selection_content_rect() else {
            return;
        };

        self.selected_paths = self
            .entries
            .iter()
            .enumerate()
            .filter(|(index, _)| {
                self.entry_content_rect(*index)
                    .is_some_and(|rect| rect.intersects(&selection))
            })
            .map(|(_, entry)| entry.file.path.clone())
            .collect();
    }

    fn selection_content_rect(&self) -> Option<Rectangle> {
        let drag = self.selection_drag?;
        let mut rect = rect_from_points(drag.origin, drag.current);

        if rect.width < SELECTION_DRAG_THRESHOLD && rect.height < SELECTION_DRAG_THRESHOLD {
            return Some(Rectangle::new(rect.position(), Size::new(0.0, 0.0)));
        }

        rect.y += self.browser_scroll_y;
        Some(rect)
    }

    fn selection_view_rect(&self) -> Option<Rectangle> {
        let drag = self.selection_drag?;
        let rect = rect_from_points(drag.origin, drag.current);

        if rect.width < SELECTION_DRAG_THRESHOLD && rect.height < SELECTION_DRAG_THRESHOLD {
            None
        } else {
            Some(rect)
        }
    }

    fn entry_content_rect(&self, index: usize) -> Option<Rectangle> {
        match self.view_mode {
            ViewMode::Icons => {
                let columns = self.icon_grid_columns();
                let row = index / columns;
                let column = index % columns;
                Some(Rectangle::new(
                    Point::new(
                        GRID_PADDING_LEFT + column as f32 * (TILE_WIDTH + GRID_COLUMN_SPACING),
                        GRID_PADDING_TOP + row as f32 * (TILE_HEIGHT + GRID_ROW_SPACING),
                    ),
                    Size::new(TILE_WIDTH, TILE_HEIGHT),
                ))
            }
            ViewMode::List => {
                if index >= self.entries.len() {
                    return None;
                }

                Some(Rectangle::new(
                    Point::new(
                        LIST_PADDING_LEFT,
                        LIST_PADDING_TOP + LIST_HEADER_HEIGHT + index as f32 * LIST_ROW_HEIGHT,
                    ),
                    Size::new(10_000.0, LIST_ROW_HEIGHT),
                ))
            }
        }
    }

    fn selection_overlay(&self) -> Element<'_, Message> {
        let Some(rect) = self.selection_view_rect() else {
            return container(space()).height(Fill).width(Fill).into();
        };

        container(
            column![
                space().height(Length::Fixed(rect.y.max(0.0))),
                row![
                    space().width(Length::Fixed(rect.x.max(0.0))),
                    container(space())
                        .height(Length::Fixed(rect.height))
                        .width(Length::Fixed(rect.width))
                        .style(style::selection_rectangle),
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn rename_overlay(&self) -> Element<'_, Message> {
        let Some(rename) = &self.rename_state else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let Some((index, _entry)) = self
            .entries
            .iter()
            .enumerate()
            .find(|(_, entry)| entry.file.path == rename.path)
        else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let Some(rect) = self.entry_content_rect(index) else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let layout = match self.view_mode {
            ViewMode::Icons => rename_editor_layout(&rename.value, TILE_WIDTH),
            ViewMode::List => rename_editor_layout(&rename.value, LIST_RENAME_BASE_WIDTH),
        };
        let (x, y) = match self.view_mode {
            ViewMode::Icons => (
                self.clamped_overlay_x(rect.x + (TILE_WIDTH - layout.width) / 2.0, layout.width),
                rect.y + GRID_RENAME_Y_OFFSET - self.browser_scroll_y,
            ),
            ViewMode::List => (
                self.clamped_overlay_x(rect.x + LIST_RENAME_X_OFFSET, layout.width),
                rect.y + (LIST_ROW_HEIGHT - layout.height) / 2.0 - self.browser_scroll_y,
            ),
        };

        container(
            column![
                space().height(Length::Fixed(y.max(0.0))),
                row![
                    space().width(Length::Fixed(x)),
                    rename_editor(rename, layout.width, layout.height),
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn clamped_overlay_x(&self, x: f32, width: f32) -> f32 {
        let max_x = (self.browser_width - width).max(0.0);
        x.clamp(0.0, max_x)
    }

    fn context_menu_overlay(&self) -> Element<'_, Message> {
        let Some(position) = self.context_menu else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let menu = container(
            column![
                context_menu_item("新建文件", Message::ContextNewFile),
                context_menu_item("新建文件夹", Message::ContextNewFolder),
                context_menu_separator(),
                context_menu_item("粘贴", Message::ContextPaste),
                context_menu_item("全选", Message::ContextSelectAll),
                context_menu_separator(),
                context_menu_item("在终端打开", Message::ContextOpenTerminal),
                context_menu_separator(),
                context_menu_item("属性", Message::ContextProperties),
            ]
            .spacing(2)
            .padding(6),
        )
        .width(CONTEXT_MENU_WIDTH)
        .style(style::context_menu);

        container(
            column![
                space().height(Length::Fixed(position.y.max(0.0))),
                row![
                    space().width(Length::Fixed(position.x.max(0.0))),
                    menu,
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn properties_overlay(&self) -> Element<'_, Message> {
        let Some(dialog) = &self.properties_dialog else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let title = match dialog.view {
            PropertiesView::Summary => "属性",
            PropertiesView::Permissions => "设置自定义权限",
            PropertiesView::Contents => "更改内容文件的权限",
        };

        let back = if dialog.view == PropertiesView::Summary {
            button(text("").size(16))
                .height(30)
                .width(30)
                .style(style::toolbar_button)
        } else {
            button(text("‹").size(26).style(style::primary_text))
                .height(30)
                .width(30)
                .padding(0)
                .style(style::toolbar_button)
                .on_press(Message::SetPropertiesView(PropertiesView::Summary))
        };

        let header = row![
            back,
            text(title).size(16).style(style::muted_text),
            space().width(Fill),
            button(text("×").size(18).style(style::primary_text))
                .height(30)
                .width(30)
                .padding(0)
                .style(style::toolbar_button)
                .on_press(Message::CloseProperties),
        ]
        .align_y(iced::Center);

        let body = match (&dialog.state, dialog.view) {
            (PropertiesState::Loading(path), _) => self.properties_loading(path),
            (PropertiesState::Error(error), _) => self.properties_error(error),
            (PropertiesState::Loaded(properties), PropertiesView::Summary) => {
                self.properties_summary(properties)
            }
            (PropertiesState::Loaded(properties), PropertiesView::Permissions) => {
                self.properties_permissions(properties)
            }
            (PropertiesState::Loaded(properties), PropertiesView::Contents) => {
                self.properties_contents_permissions(properties)
            }
        };

        let dialog = container(column![header, body].spacing(16).padding(14))
            .width(PROPERTIES_DIALOG_WIDTH)
            .style(style::properties_dialog);

        container(dialog)
            .width(Fill)
            .height(Fill)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .style(style::modal_overlay)
            .into()
    }

    fn properties_loading(&self, path: &PathBuf) -> Element<'_, Message> {
        container(
            column![
                text(path.display().to_string())
                    .size(14)
                    .style(style::muted_text),
                text("正在统计文件夹属性...")
                    .size(16)
                    .style(style::primary_text),
            ]
            .spacing(10)
            .align_x(iced::Center),
        )
        .height(220)
        .width(Fill)
        .align_x(Horizontal::Center)
        .align_y(Vertical::Center)
        .into()
    }

    fn properties_error<'a>(&self, error: &'a str) -> Element<'a, Message> {
        container(text(error).size(14).style(style::primary_text))
            .height(220)
            .width(Fill)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .into()
    }

    fn properties_summary(&self, properties: &FolderProperties) -> Element<'_, Message> {
        let parent = properties
            .parent
            .as_ref()
            .map(|path| path.display().to_string())
            .unwrap_or_else(|| "-".to_string());
        let free = properties
            .free_space
            .map(|size| format!("{} 剩余", format_size(size)))
            .unwrap_or_else(|| "剩余空间未知".to_string());

        column![
            column![
                entry_icon(
                    &EntryIcon::Embedded(include_bytes!("../../../icons/folder.svg")),
                    82.0
                ),
                text(properties.name.clone())
                    .size(22)
                    .style(style::primary_text)
                    .align_x(Horizontal::Center),
                text(format!(
                    "{} 项，共 {}",
                    properties.item_count,
                    format_size(properties.total_size)
                ))
                .size(14)
                .style(style::primary_text),
                text(free).size(13).style(style::muted_text),
            ]
            .spacing(8)
            .align_x(iced::Center)
            .width(Fill),
            container(
                row![
                    column![
                        text("上级文件夹(F)").size(12).style(style::muted_text),
                        text(parent).size(15).style(style::primary_text),
                    ]
                    .spacing(4),
                    space().width(Fill),
                    text("...").size(18).style(style::primary_text),
                ]
                .align_y(iced::Center)
            )
            .padding(14)
            .width(Fill)
            .style(style::property_group),
            container(
                column![
                    property_row("修改时间", format_modified(properties.modified)),
                    property_row("创建时间", format_modified(properties.created)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            button(
                row![
                    text("权限(P)").size(15).style(style::primary_text),
                    space().width(Fill),
                    text(permission_summary(properties.mode))
                        .size(14)
                        .style(style::muted_text),
                    text("›").size(22).style(style::primary_text),
                ]
                .align_y(iced::Center)
            )
            .height(54)
            .width(Fill)
            .padding([0, 14])
            .style(style::property_button)
            .on_press(Message::SetPropertiesView(PropertiesView::Permissions)),
        ]
        .spacing(16)
        .into()
    }

    fn properties_permissions(&self, properties: &FolderProperties) -> Element<'_, Message> {
        let mode = properties.mode.unwrap_or(0);

        column![
            container(
                column![
                    property_row("所有者(U)", owner_label(properties.owner)),
                    property_row("访问", directory_permission(mode, 6)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            container(
                column![
                    property_row("用户组(G)", group_label(properties.group)),
                    property_row("访问", directory_permission(mode, 3)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            text("其他用户").size(15).style(style::primary_text),
            container(property_row("访问", directory_permission(mode, 0)))
                .width(Fill)
                .style(style::property_group),
            button(
                text("更改内容文件的权限(E)...")
                    .size(15)
                    .style(style::primary_text)
            )
            .height(44)
            .width(Fill)
            .padding([0, 14])
            .style(style::property_button)
            .on_press(Message::SetPropertiesView(PropertiesView::Contents)),
        ]
        .spacing(16)
        .into()
    }

    fn properties_contents_permissions(
        &self,
        properties: &FolderProperties,
    ) -> Element<'_, Message> {
        let mode = properties.mode.unwrap_or(0);

        column![
            row![
                button(text("取消(C)").size(14).style(style::primary_text))
                    .height(36)
                    .padding([0, 14])
                    .style(style::toolbar_button)
                    .on_press(Message::SetPropertiesView(PropertiesView::Permissions)),
                space().width(Fill),
                button(text("更改(H)").size(14).style(style::primary_text))
                    .height(36)
                    .padding([0, 14])
                    .style(style::disabled_button),
            ]
            .align_y(iced::Center),
            text("所有者").size(15).style(style::primary_text),
            container(
                column![
                    property_row("文件(F)", file_permission(mode, 6)),
                    property_row("文件夹(O)", directory_permission(mode, 6)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            text("组").size(15).style(style::primary_text),
            container(
                column![
                    property_row("文件(L)", file_permission(mode, 3)),
                    property_row("文件夹(D)", directory_permission(mode, 3)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            text("其它").size(15).style(style::primary_text),
            container(
                column![
                    property_row("文件(E)", file_permission(mode, 0)),
                    property_row("文件夹(R)", directory_permission(mode, 0)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
        ]
        .spacing(14)
        .into()
    }

    fn submit_rename(&mut self) -> Task<Message> {
        if let Some(rename) = &mut self.rename_state {
            rename.value = rename.content.text();
        }

        let Some(rename) = self.rename_state.clone() else {
            return Task::none();
        };

        let new_name = rename.content.text().trim().to_string();
        if let Some(message) = rename_limit_error(&rename, &new_name) {
            self.status = message;
            return self.focus_rename_input(false);
        }

        if new_name.is_empty() {
            self.status = "File name cannot be empty".to_string();
            return self.focus_rename_input(false);
        }

        let current_name = rename
            .path
            .file_name()
            .and_then(|name| name.to_str())
            .unwrap_or_default();

        if new_name == current_name {
            self.rename_state = None;
            self.status = "Name unchanged".to_string();
            return Task::none();
        }

        self.status = "Renaming...".to_string();
        Task::perform(
            async move { rename_entry(rename.path, new_name) },
            Message::RenameFinished,
        )
    }

    fn prepare_rename_retry(&mut self) -> Task<Message> {
        if let Some(rename) = &mut self.rename_state {
            rename.content.perform(text_editor::Action::SelectAll);
            rename.value = rename.content.text();
        }

        self.focus_rename_input(false)
    }

    fn focus_rename_input(&self, select_all: bool) -> Task<Message> {
        if self.rename_state.is_none() {
            return Task::none();
        }

        let _ = select_all;
        operation::focus(rename_input_id())
    }

    fn pointer_over_entry(&self, position: Point) -> bool {
        let content_position = Point::new(position.x, position.y + self.browser_scroll_y);

        self.entries.iter().enumerate().any(|(index, _)| {
            self.entry_content_rect(index)
                .is_some_and(|rect| rect_contains(rect, content_position))
        })
    }

    fn close_menu(&mut self) {
        self.menu_open = false;
        self.view_submenu_open = false;
        self.context_menu = None;
    }
}

fn toolbar_icon<'a>(bytes: &'static [u8], enabled: bool) -> Element<'a, Message> {
    let color = if enabled {
        style::MUTED
    } else {
        style::DISABLED
    };

    container(
        svg(svg::Handle::from_memory(bytes))
            .width(Fill)
            .height(Fill)
            .style(move |_, _| svg_style::Style { color: Some(color) }),
    )
    .width(Fill)
    .height(Fill)
    .align_x(Horizontal::Center)
    .align_y(Vertical::Center)
    .into()
}

fn entry_icon<'a>(icon: &EntryIcon, size: f32) -> Element<'a, Message> {
    let handle = match icon {
        EntryIcon::Embedded(bytes) => svg::Handle::from_memory(*bytes),
        EntryIcon::Theme(bytes) => svg::Handle::from_memory(bytes.clone()),
    };

    container(svg(handle).width(size).height(size))
        .width(size)
        .height(size)
        .align_x(Horizontal::Center)
        .align_y(Vertical::Center)
        .into()
}

fn sidebar_icon<'a>(bytes: &'static [u8]) -> Element<'a, Message> {
    container(
        svg(svg::Handle::from_memory(bytes))
            .width(Fill)
            .height(Fill),
    )
    .width(20)
    .height(20)
    .align_x(Horizontal::Center)
    .align_y(Vertical::Center)
    .into()
}

fn resize_layer<'a>() -> Element<'a, Message> {
    use mouse::Interaction;
    use window::Direction;

    stack([
        resize_region(
            Direction::North,
            Interaction::ResizingVertically,
            Length::Fill,
            Length::Fixed(RESIZE_HIT_SIZE),
            Horizontal::Center,
            Vertical::Top,
        ),
        resize_region(
            Direction::South,
            Interaction::ResizingVertically,
            Length::Fill,
            Length::Fixed(RESIZE_HIT_SIZE),
            Horizontal::Center,
            Vertical::Bottom,
        ),
        resize_region(
            Direction::West,
            Interaction::ResizingHorizontally,
            Length::Fixed(RESIZE_HIT_SIZE),
            Length::Fill,
            Horizontal::Left,
            Vertical::Center,
        ),
        resize_region(
            Direction::East,
            Interaction::ResizingHorizontally,
            Length::Fixed(RESIZE_HIT_SIZE),
            Length::Fill,
            Horizontal::Right,
            Vertical::Center,
        ),
        resize_region(
            Direction::NorthWest,
            Interaction::ResizingDiagonallyDown,
            Length::Fixed(RESIZE_HIT_SIZE),
            Length::Fixed(RESIZE_HIT_SIZE),
            Horizontal::Left,
            Vertical::Top,
        ),
        resize_region(
            Direction::NorthEast,
            Interaction::ResizingDiagonallyUp,
            Length::Fixed(RESIZE_HIT_SIZE),
            Length::Fixed(RESIZE_HIT_SIZE),
            Horizontal::Right,
            Vertical::Top,
        ),
        resize_region(
            Direction::SouthWest,
            Interaction::ResizingDiagonallyUp,
            Length::Fixed(RESIZE_HIT_SIZE),
            Length::Fixed(RESIZE_HIT_SIZE),
            Horizontal::Left,
            Vertical::Bottom,
        ),
        resize_region(
            Direction::SouthEast,
            Interaction::ResizingDiagonallyDown,
            Length::Fixed(RESIZE_HIT_SIZE),
            Length::Fixed(RESIZE_HIT_SIZE),
            Horizontal::Right,
            Vertical::Bottom,
        ),
    ])
    .height(Fill)
    .width(Fill)
    .into()
}

fn resize_region<'a>(
    direction: window::Direction,
    interaction: mouse::Interaction,
    width: Length,
    height: Length,
    horizontal: Horizontal,
    vertical: Vertical,
) -> Element<'a, Message> {
    let handle = mouse_area(
        container(space())
            .width(width)
            .height(height)
            .style(style::resize_hit_area),
    )
    .interaction(interaction)
    .on_press(Message::WindowResize(direction));

    container(handle)
        .width(Fill)
        .height(Fill)
        .align_x(horizontal)
        .align_y(vertical)
        .into()
}

fn window_icon<'a>(bytes: &'static [u8]) -> Element<'a, Message> {
    container(
        svg(svg::Handle::from_memory(bytes))
            .width(Fill)
            .height(Fill)
            .style(style::window_icon),
    )
    .width(Fill)
    .height(Fill)
    .align_x(Horizontal::Center)
    .align_y(Vertical::Center)
    .into()
}

fn list_header_cell<'a>(label: &'static str, width: Length) -> Element<'a, Message> {
    container(text(label).size(13).style(style::muted_text))
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .into()
}

fn list_name_cell<'a>(entry: &DisplayEntry, value: String, width: Length) -> Element<'a, Message> {
    let content = row![
        entry_icon(&entry.icon, 24.0),
        text(value).size(14).style(style::primary_text),
    ]
    .spacing(10)
    .align_y(iced::Center);

    container(content)
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .into()
}

fn rename_editor<'a>(rename: &'a RenameState, width: f32, height: f32) -> Element<'a, Message> {
    text_editor(&rename.content)
        .id(rename_input_id())
        .on_action(Message::RenameEditorAction)
        .key_binding(rename_editor_key_binding)
        .size(RENAME_TEXT_SIZE)
        .line_height(RENAME_LINE_HEIGHT)
        .padding(rename_editor_padding())
        .wrapping(iced::advanced::text::Wrapping::WordOrGlyph)
        .width(width)
        .height(Length::Fixed(height))
        .style(style::rename_editor)
        .into()
}

fn rename_input_id() -> iced::widget::Id {
    iced::widget::Id::new(RENAME_INPUT_ID)
}

fn rename_limit_error(rename: &RenameState, value: &str) -> Option<String> {
    let name = value.trim();
    if name.is_empty() {
        return None;
    }

    let name_bytes = name.as_bytes().len();
    if let Some(limit) = rename.limits.name_bytes {
        if name_bytes > limit {
            return Some(format!("File name is too long; limit is {limit} bytes"));
        }
    }

    if let (Some(limit), Some(parent)) = (rename.limits.path_bytes, rename.path.parent()) {
        let parent_bytes = parent.as_os_str().as_bytes().len();
        let separator_bytes = usize::from(!parent.as_os_str().as_bytes().ends_with(b"/"));
        let path_bytes = parent_bytes + separator_bytes + name_bytes;

        if path_bytes > limit {
            return Some(format!("File path is too long; limit is {limit} bytes"));
        }
    }

    None
}

#[derive(Debug, Clone, Copy)]
struct RenameEditorLayout {
    width: f32,
    height: f32,
}

fn rename_editor_layout(value: &str, base_width: f32) -> RenameEditorLayout {
    let character_count = value.chars().count().max(1);
    let max_width = base_width * RENAME_MAX_WIDTH_MULTIPLIER;
    let desired_width = (character_count as f32 * RENAME_AVERAGE_CHAR_WIDTH
        + RENAME_HORIZONTAL_PADDING * 2.0)
        .clamp(base_width, max_width);
    let usable_width =
        (desired_width - RENAME_HORIZONTAL_PADDING * 2.0).max(RENAME_AVERAGE_CHAR_WIDTH);
    let characters_per_line = (usable_width / RENAME_AVERAGE_CHAR_WIDTH).floor().max(1.0) as usize;

    let visual_lines = if value.is_empty() {
        1
    } else {
        value
            .split('\n')
            .map(|line| {
                let line_chars = line.chars().count().max(1);
                line_chars.div_ceil(characters_per_line)
            })
            .sum::<usize>()
            .max(1)
    };

    let line_height = RENAME_TEXT_SIZE * RENAME_LINE_HEIGHT;
    let height =
        (visual_lines as f32 * line_height + RENAME_VERTICAL_PADDING * 2.0).max(RENAME_MIN_HEIGHT);

    RenameEditorLayout {
        width: desired_width,
        height,
    }
}

fn rename_editor_padding() -> Padding {
    Padding::default()
        .top(RENAME_VERTICAL_PADDING)
        .bottom(RENAME_VERTICAL_PADDING)
        .left(RENAME_HORIZONTAL_PADDING)
        .right(RENAME_HORIZONTAL_PADDING)
}

fn rename_editor_key_binding(
    event: text_editor::KeyPress,
) -> Option<text_editor::Binding<Message>> {
    if matches!(
        event.modified_key.as_ref(),
        iced::keyboard::Key::Named(iced::keyboard::key::Named::Enter)
    ) {
        Some(text_editor::Binding::Custom(Message::RenameSubmit))
    } else {
        text_editor::Binding::from_key_press(event)
    }
}

fn context_menu_item<'a>(label: &'static str, message: Message) -> Element<'a, Message> {
    button(text(label).size(14).style(style::primary_text))
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, false))
        .on_press(message)
        .into()
}

fn context_menu_separator<'a>() -> Element<'a, Message> {
    container(space())
        .height(1)
        .width(Fill)
        .style(|_| container_style::Style::default().background(style::BORDER))
        .into()
}

fn property_row<'a>(label: &'static str, value: String) -> Element<'a, Message> {
    container(
        row![
            text(label).size(14).style(style::primary_text),
            space().width(Fill),
            text(value).size(14).style(style::primary_text),
        ]
        .align_y(iced::Center),
    )
    .height(52)
    .width(Fill)
    .padding([0, 14])
    .into()
}

fn browser_width_from_window(window_width: f32) -> f32 {
    (window_width - SIDEBAR_WIDTH).max(TILE_WIDTH + GRID_PADDING_LEFT * 2.0)
}

fn icon_grid_columns(browser_width: f32) -> usize {
    let content_width = (browser_width - GRID_PADDING_LEFT * 2.0).max(TILE_WIDTH);
    ((content_width + GRID_COLUMN_SPACING) / (TILE_WIDTH + GRID_COLUMN_SPACING))
        .floor()
        .max(1.0) as usize
}

fn rect_from_points(start: Point, end: Point) -> Rectangle {
    let x = start.x.min(end.x);
    let y = start.y.min(end.y);
    let width = (start.x - end.x).abs();
    let height = (start.y - end.y).abs();

    Rectangle::new(Point::new(x, y), Size::new(width, height))
}

fn rect_contains(rect: Rectangle, point: Point) -> bool {
    point.x >= rect.x
        && point.x <= rect.x + rect.width
        && point.y >= rect.y
        && point.y <= rect.y + rect.height
}

fn list_value_cell<'a>(value: String, width: Length, primary: bool) -> Element<'a, Message> {
    let text = if primary {
        text(value).size(14).style(style::primary_text)
    } else {
        text(value).size(13).style(style::muted_text)
    };

    container(text)
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .into()
}

fn short_name(name: &str) -> String {
    const MAX_CHARS: usize = 18;
    if name.chars().count() <= MAX_CHARS {
        return name.to_string();
    }

    let mut short = name.chars().take(MAX_CHARS - 1).collect::<String>();
    short.push('…');
    short
}

fn short_list_text(value: &str) -> String {
    const MAX_CHARS: usize = 58;
    if value.chars().count() <= MAX_CHARS {
        return value.to_string();
    }

    let mut short = value.chars().take(MAX_CHARS - 1).collect::<String>();
    short.push('…');
    short
}

fn entry_meta(entry: &FileEntry) -> String {
    match (entry.kind, entry.size) {
        (EntryKind::Directory, _) => "Folder".to_string(),
        (EntryKind::Symlink, _) => "Link".to_string(),
        (EntryKind::File, Some(size)) => format_size(size),
        (EntryKind::File, None) => "File".to_string(),
        (EntryKind::Other, _) => "Other".to_string(),
    }
}

fn entry_size(entry: &FileEntry) -> String {
    match (entry.kind, entry.size) {
        (EntryKind::File, Some(size)) => format_size(size),
        _ => "-".to_string(),
    }
}

fn entry_owner(entry: &FileEntry) -> String {
    entry
        .owner
        .map(|owner| owner.to_string())
        .unwrap_or_else(|| "-".to_string())
}

fn format_size(size: u64) -> String {
    const KB: u64 = 1024;
    const MB: u64 = KB * 1024;
    const GB: u64 = MB * 1024;

    if size >= GB {
        format!("{:.1} GB", size as f64 / GB as f64)
    } else if size >= MB {
        format!("{:.1} MB", size as f64 / MB as f64)
    } else if size >= KB {
        format!("{:.1} KB", size as f64 / KB as f64)
    } else {
        format!("{size} B")
    }
}

fn format_modified(modified: Option<SystemTime>) -> String {
    let Some(modified) = modified else {
        return "-".to_string();
    };
    let Ok(duration) = modified.duration_since(UNIX_EPOCH) else {
        return "-".to_string();
    };

    let seconds = duration.as_secs();
    let days = (seconds / 86_400) as i64;
    let seconds_of_day = seconds % 86_400;
    let (year, month, day) = civil_from_days(days);
    let hour = seconds_of_day / 3_600;
    let minute = (seconds_of_day % 3_600) / 60;

    format!("{year:04}-{month:02}-{day:02} {hour:02}:{minute:02}")
}

fn create_entry_task(parent: PathBuf, kind: NewEntryKind) -> Task<Message> {
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

fn open_terminal(cwd: PathBuf) -> Result<String, String> {
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

fn owner_label(owner: Option<u32>) -> String {
    owner
        .map(|owner| format!("UID {owner}"))
        .unwrap_or_else(|| "-".to_string())
}

fn group_label(group: Option<u32>) -> String {
    group
        .map(|group| format!("GID {group}"))
        .unwrap_or_else(|| "-".to_string())
}

fn permission_summary(mode: Option<u32>) -> String {
    mode.map(|mode| format!("{:04o}", mode & 0o7777))
        .unwrap_or_else(|| "未知".to_string())
}

fn file_permission(mode: u32, shift: u8) -> String {
    let bits = (mode >> shift) & 0o7;
    let read = bits & 0o4 != 0;
    let write = bits & 0o2 != 0;

    match (read, write) {
        (true, true) => "读取和写入".to_string(),
        (true, false) => "只能读取".to_string(),
        (false, true) => "只能写入".to_string(),
        (false, false) => "无".to_string(),
    }
}

fn directory_permission(mode: u32, shift: u8) -> String {
    let bits = (mode >> shift) & 0o7;
    let read = bits & 0o4 != 0;
    let write = bits & 0o2 != 0;
    let execute = bits & 0o1 != 0;

    if write && execute {
        "新建和删除文件".to_string()
    } else if read && execute {
        "访问文件".to_string()
    } else if execute {
        "只能访问".to_string()
    } else if read {
        "只能列出文件".to_string()
    } else {
        "无".to_string()
    }
}

fn civil_from_days(days_since_unix_epoch: i64) -> (i32, u32, u32) {
    let z = days_since_unix_epoch + 719_468;
    let era = if z >= 0 { z } else { z - 146_096 } / 146_097;
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1_460 + doe / 36_524 - doe / 146_096) / 365;
    let y = yoe + era * 400;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = doy - (153 * mp + 2) / 5 + 1;
    let month = mp + if mp < 10 { 3 } else { -9 };
    let year = y + if month <= 2 { 1 } else { 0 };

    (year as i32, month as u32, day as u32)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn icon_grid_columns_keeps_at_least_one_column() {
        assert_eq!(icon_grid_columns(1.0), 1);
        assert_eq!(icon_grid_columns(TILE_WIDTH + GRID_PADDING_LEFT * 2.0), 1);
    }

    #[test]
    fn icon_grid_columns_grows_with_available_width() {
        let one_column = TILE_WIDTH + GRID_PADDING_LEFT * 2.0;
        let two_columns = TILE_WIDTH * 2.0 + GRID_COLUMN_SPACING + GRID_PADDING_LEFT * 2.0;
        let three_columns = TILE_WIDTH * 3.0 + GRID_COLUMN_SPACING * 2.0 + GRID_PADDING_LEFT * 2.0;

        assert_eq!(icon_grid_columns(one_column), 1);
        assert_eq!(icon_grid_columns(two_columns), 2);
        assert_eq!(icon_grid_columns(three_columns), 3);
    }

    #[test]
    fn browser_width_uses_window_width_minus_sidebar() {
        assert_eq!(
            browser_width_from_window(WINDOW_MIN_WIDTH),
            WINDOW_MIN_WIDTH - SIDEBAR_WIDTH
        );
    }

    #[test]
    fn rename_limit_error_blocks_names_over_name_limit() {
        let rename = RenameState::new(
            PathBuf::from("/tmp/default"),
            "default".to_string(),
            ChildPathLimits {
                name_bytes: Some(4),
                path_bytes: None,
            },
        );

        assert!(rename_limit_error(&rename, "abcd").is_none());
        assert!(rename_limit_error(&rename, "abcde").is_some());
    }

    #[test]
    fn rename_limit_error_blocks_paths_over_path_limit() {
        let rename = RenameState::new(
            PathBuf::from("/tmp/default"),
            "default".to_string(),
            ChildPathLimits {
                name_bytes: None,
                path_bytes: Some(8),
            },
        );

        assert!(rename_limit_error(&rename, "abc").is_none());
        assert!(rename_limit_error(&rename, "abcd").is_some());
    }
}

fn latest_window_task(
    action: impl Fn(window::Id) -> Task<Message> + Send + 'static,
) -> Task<Message> {
    window::latest().then(move |id| match id {
        Some(id) => action(id),
        None => Task::none(),
    })
}

mod style {
    use super::*;
    use iced::widget::checkbox as checkbox_widget;
    use iced::widget::text_editor as text_editor_widget;
    use iced::widget::text_input as text_input_widget;

    pub const CONTENT: Color = Color::from_rgb(0.10, 0.10, 0.12);
    pub const SIDEBAR: Color = Color::from_rgb(0.15, 0.15, 0.18);
    pub const TOOLBAR: Color = Color::from_rgb(0.13, 0.13, 0.15);
    pub const SURFACE: Color = Color::from_rgb(0.18, 0.18, 0.21);
    pub const SURFACE_HOVER: Color = Color::from_rgb(0.24, 0.24, 0.27);
    pub const TEXT: Color = Color::from_rgb(0.91, 0.91, 0.93);
    pub const MUTED: Color = Color::from_rgb(0.60, 0.60, 0.64);
    pub const DISABLED: Color = Color::from_rgb(0.42, 0.42, 0.46);
    pub const BORDER: Color = Color::from_rgb(0.22, 0.22, 0.25);
    const SELECTION: Color = Color::from_rgb(0.31, 0.43, 0.62);

    pub fn app_background(_theme: &Theme) -> container_style::Style {
        container_style::Style::default().background(CONTENT)
    }

    pub fn sidebar(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(SIDEBAR)
            .color(TEXT)
            .border(Border::default().color(BORDER).width(1))
    }

    pub fn sidebar_drag_area(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(SIDEBAR)
            .color(TEXT)
    }

    pub fn toolbar(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(TOOLBAR)
            .color(TEXT)
            .border(Border::default().color(BORDER).width(1))
    }

    pub fn content_background(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(CONTENT)
            .color(TEXT)
    }

    pub fn toolbar_drag_area(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(Color::TRANSPARENT)
            .color(TEXT)
    }

    pub fn resize_hit_area(_theme: &Theme) -> container_style::Style {
        container_style::Style::default().background(Color::TRANSPARENT)
    }

    pub fn path_input(
        _theme: &Theme,
        _status: text_input_widget::Status,
    ) -> text_input_widget::Style {
        text_input_widget::Style {
            background: Background::Color(SURFACE),
            border: Border::default().rounded(6).color(BORDER).width(1),
            icon: MUTED,
            placeholder: MUTED,
            value: TEXT,
            selection: Color::from_rgb(0.30, 0.44, 0.65),
        }
    }

    pub fn rename_editor(
        _theme: &Theme,
        _status: text_editor_widget::Status,
    ) -> text_editor_widget::Style {
        text_editor_widget::Style {
            background: Background::Color(SURFACE),
            border: Border::default().rounded(6).color(BORDER).width(1),
            placeholder: MUTED,
            value: TEXT,
            selection: Color::from_rgb(0.30, 0.44, 0.65),
        }
    }

    pub fn tile_container(selected: bool) -> container_style::Style {
        let border = if selected {
            Border::default().rounded(8).color(SELECTION).width(1)
        } else {
            Border::default().rounded(8)
        };

        let style = container_style::Style::default().color(TEXT).border(border);

        if selected {
            style.background(SURFACE)
        } else {
            style
        }
    }

    pub fn list_header(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(Color::from_rgb(0.12, 0.12, 0.14))
            .border(Border::default().color(BORDER).width(1).rounded(6))
    }

    pub fn list_row_container(selected: bool) -> container_style::Style {
        let background = if selected {
            SURFACE_HOVER
        } else {
            Color::TRANSPARENT
        };
        let border = if selected {
            Border::default().rounded(4).color(SELECTION).width(1)
        } else {
            Border::default().rounded(4)
        };

        container_style::Style::default()
            .background(Background::Color(background))
            .color(TEXT)
            .border(border)
    }

    pub fn selection_rectangle(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(Color::from_rgba(0.31, 0.43, 0.62, 0.22))
            .border(Border::default().rounded(3).color(SELECTION).width(1))
    }

    pub fn menu_panel(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(SURFACE)
            .color(TEXT)
            .border(Border::default().rounded(6).color(BORDER).width(1))
            .shadow(Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.24),
                offset: iced::Vector::new(0.0, 4.0),
                blur_radius: 12.0,
            })
    }

    pub fn context_menu(theme: &Theme) -> container_style::Style {
        menu_panel(theme)
    }

    pub fn modal_overlay(_theme: &Theme) -> container_style::Style {
        container_style::Style::default().background(Color::from_rgba(0.0, 0.0, 0.0, 0.18))
    }

    pub fn properties_dialog(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(CONTENT)
            .color(TEXT)
            .border(Border::default().rounded(8).color(BORDER).width(1))
            .shadow(Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.36),
                offset: iced::Vector::new(0.0, 8.0),
                blur_radius: 18.0,
            })
    }

    pub fn property_group(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(SURFACE)
            .color(TEXT)
            .border(Border::default().rounded(8).color(BORDER).width(1))
    }

    pub fn menu_button(
        _theme: &Theme,
        status: button_style::Status,
        active: bool,
    ) -> button_style::Style {
        let background = if active {
            SURFACE_HOVER
        } else if matches!(
            status,
            button_style::Status::Hovered | button_style::Status::Pressed
        ) {
            Color::from_rgb(0.22, 0.22, 0.25)
        } else {
            Color::TRANSPARENT
        };

        button_style::Style {
            background: Some(Background::Color(background)),
            text_color: TEXT,
            border: Border::default().rounded(5),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn property_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
        let background = if matches!(
            status,
            button_style::Status::Hovered | button_style::Status::Pressed
        ) {
            SURFACE_HOVER
        } else {
            SURFACE
        };

        button_style::Style {
            background: Some(Background::Color(background)),
            text_color: TEXT,
            border: Border::default().rounded(8).color(BORDER).width(1),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn disabled_button(_theme: &Theme, _status: button_style::Status) -> button_style::Style {
        button_style::Style {
            background: Some(Background::Color(Color::from_rgb(0.56, 0.16, 0.22))),
            text_color: Color::from_rgb(0.84, 0.76, 0.78),
            border: Border::default().rounded(8),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn toolbar_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
        let color = match status {
            button_style::Status::Hovered | button_style::Status::Pressed => SURFACE_HOVER,
            button_style::Status::Disabled => Color::TRANSPARENT,
            _ => Color::TRANSPARENT,
        };

        button_style::Style {
            background: Some(Background::Color(color)),
            text_color: if matches!(status, button_style::Status::Disabled) {
                DISABLED
            } else {
                MUTED
            },
            border: Border::default().rounded(6),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn window_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
        let background = match status {
            button_style::Status::Hovered | button_style::Status::Pressed => SURFACE_HOVER,
            _ => Color::TRANSPARENT,
        };

        button_style::Style {
            background: Some(Background::Color(background)),
            text_color: MUTED,
            border: Border::default().rounded(6),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn close_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
        let background = match status {
            button_style::Status::Hovered | button_style::Status::Pressed => {
                Color::from_rgb(0.75, 0.20, 0.20)
            }
            _ => Color::TRANSPARENT,
        };

        button_style::Style {
            background: Some(Background::Color(background)),
            text_color: if matches!(
                status,
                button_style::Status::Hovered | button_style::Status::Pressed
            ) {
                TEXT
            } else {
                MUTED
            },
            border: Border::default().rounded(6),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn window_icon(_theme: &Theme, _status: svg_style::Status) -> svg_style::Style {
        svg_style::Style { color: Some(MUTED) }
    }

    pub fn sidebar_button(
        _theme: &Theme,
        status: button_style::Status,
        active: bool,
    ) -> button_style::Style {
        let background = if active {
            Color::from_rgb(0.25, 0.25, 0.28)
        } else if matches!(
            status,
            button_style::Status::Hovered | button_style::Status::Pressed
        ) {
            Color::from_rgb(0.20, 0.20, 0.23)
        } else {
            Color::TRANSPARENT
        };

        button_style::Style {
            background: Some(Background::Color(background)),
            text_color: TEXT,
            border: Border::default().rounded(8),
            shadow: Shadow::default(),
            snap: true,
        }
    }

    pub fn checkbox(_theme: &Theme, status: checkbox_widget::Status) -> checkbox_widget::Style {
        let active = matches!(status, checkbox_widget::Status::Active { is_checked: true });

        checkbox_widget::Style {
            background: Background::Color(if active { SURFACE_HOVER } else { SURFACE }),
            icon_color: TEXT,
            border: Border::default().rounded(4).color(BORDER).width(1),
            text_color: Some(MUTED),
        }
    }

    pub fn primary_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style { color: Some(TEXT) }
    }

    pub fn muted_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style { color: Some(MUTED) }
    }

    pub fn divider<'a>() -> Element<'a, Message> {
        container(space())
            .height(1)
            .width(Fill)
            .style(|_| {
                container_style::Style::default().background(Color::from_rgb(0.22, 0.22, 0.25))
            })
            .into()
    }
}
