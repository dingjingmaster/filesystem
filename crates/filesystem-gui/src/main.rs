use filesystem_core::{
    DirectoryListing, EntryKind, FileEntry, FsError, ScanOptions, scan_dir, search_file_names,
};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{
    button, checkbox, column, container, mouse_area, row, scrollable, space, stack, svg, text,
    text_input,
};
use iced::widget::{button as button_style, container as container_style, svg as svg_style};
use iced::{
    Background, Border, Color, Element, Fill, Length, Padding, Point, Rectangle,
    Result as IcedResult, Shadow, Size, Task, Theme, mouse, window,
};
use std::collections::BTreeSet;
use std::env;
use std::fs;
use std::path::PathBuf;
use std::time::{SystemTime, UNIX_EPOCH};

const APP_NAME_EN: &str = "File";
const APP_NAME_ZH: &str = "文件";
const WINDOW_ICON_SIZE: u32 = 128;
const SIDEBAR_WIDTH: f32 = 240.0;
const TOOLBAR_HEIGHT: f32 = 54.0;
const RESIZE_HIT_SIZE: f32 = 6.0;
const TILE_WIDTH: f32 = 142.0;
const TILE_HEIGHT: f32 = 128.0;
const GRID_COLUMNS: usize = 6;
const LIST_ROW_HEIGHT: f32 = 40.0;
const GRID_PADDING_TOP: f32 = 28.0;
const GRID_PADDING_LEFT: f32 = 36.0;
const GRID_ROW_SPACING: f32 = 22.0;
const GRID_COLUMN_SPACING: f32 = 28.0;
const LIST_PADDING_TOP: f32 = 18.0;
const LIST_PADDING_LEFT: f32 = 28.0;
const LIST_HEADER_HEIGHT: f32 = 34.0;
const SELECTION_DRAG_THRESHOLD: f32 = 3.0;

pub fn main() -> IcedResult {
    iced::application(FileManager::new, FileManager::update, FileManager::view)
        .title(|manager: &FileManager| format!("{APP_NAME_EN} - {}", manager.cwd.display()))
        .theme(Theme::Dark)
        .window(window_settings())
        .run()
}

fn window_settings() -> window::Settings {
    let mut settings = window::Settings {
        size: Size::new(1220.0, 760.0),
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
    selected_paths: BTreeSet<PathBuf>,
    selection_drag: Option<SelectionDrag>,
    browser_pointer: Option<Point>,
    browser_scroll_y: f32,
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
    BrowserScrolled(f32),
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
            selected_paths: BTreeSet::new(),
            selection_drag: None,
            browser_pointer: None,
            browser_scroll_y: 0.0,
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
                self.close_menu();
                self.open_path(path, kind)
            }
            Message::Go(path) => {
                self.close_menu();
                self.visit_path(path)
            }
            Message::Back => {
                self.close_menu();
                self.go_back()
            }
            Message::Forward => {
                self.close_menu();
                self.go_forward()
            }
            Message::PathChanged(path) => {
                self.path_input = path;
                Task::none()
            }
            Message::PathSubmit => {
                self.close_menu();
                self.open_path_input()
            }
            Message::SelectEntry(path) => {
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
            Message::BrowserScrolled(offset_y) => {
                self.browser_scroll_y = offset_y;
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::ToggleHidden(show_hidden) => {
                self.close_menu();
                self.show_hidden = show_hidden;
                if self.search_query.is_some() {
                    self.rerun_search()
                } else {
                    self.reload()
                }
            }
            Message::ToggleMenu => {
                self.menu_open = !self.menu_open;
                if !self.menu_open {
                    self.view_submenu_open = false;
                }
                Task::none()
            }
            Message::ToggleViewSubmenu => {
                self.view_submenu_open = !self.view_submenu_open;
                Task::none()
            }
            Message::SetViewMode(view_mode) => {
                self.view_mode = view_mode;
                self.close_menu();
                Task::none()
            }
            Message::DirectoryLoaded(request, result) => {
                self.directory_loaded(request, result);
                Task::none()
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

    fn view(&self) -> Element<'_, Message> {
        let shell = row![self.sidebar(), self.main_area()]
            .height(Fill)
            .width(Fill);

        let shell = container(shell)
            .height(Fill)
            .width(Fill)
            .style(style::app_background);

        stack([shell.into(), resize_layer()])
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
            .on_release(Message::BrowserReleased);

        stack([content.into(), self.selection_overlay()])
            .height(Fill)
            .width(Fill)
            .into()
    }

    fn grid(&self) -> Element<'_, Message> {
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
            for chunk in self.entries.chunks(GRID_COLUMNS) {
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
        let name = short_list_text(&self.entry_display_name(entry));
        let size = entry_size(&entry.file);
        let owner = entry_owner(&entry.file);
        let modified = format_modified(entry.file.modified);

        let content = row![
            list_name_cell(entry, name, Length::FillPortion(5)),
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
        let name = text(short_name(&entry.file.name))
            .size(15)
            .width(TILE_WIDTH)
            .align_x(Horizontal::Center)
            .style(style::primary_text);

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
                let row = index / GRID_COLUMNS;
                let column = index % GRID_COLUMNS;
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

    fn close_menu(&mut self) {
        self.menu_open = false;
        self.view_submenu_open = false;
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

fn rect_from_points(start: Point, end: Point) -> Rectangle {
    let x = start.x.min(end.x);
    let y = start.y.min(end.y);
    let width = (start.x - end.x).abs();
    let height = (start.y - end.y).abs();

    Rectangle::new(Point::new(x, y), Size::new(width, height))
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
