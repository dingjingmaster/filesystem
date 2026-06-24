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
    Background, Border, Color, Element, Fill, Length, Result as IcedResult, Shadow, Size, Task,
    Theme, mouse, window,
};
use std::path::PathBuf;

const APP_NAME_EN: &str = "File";
const APP_NAME_ZH: &str = "文件";
const WINDOW_ICON_SIZE: u32 = 128;
const SIDEBAR_WIDTH: f32 = 240.0;
const TOOLBAR_HEIGHT: f32 = 54.0;
const RESIZE_HIT_SIZE: f32 = 6.0;
const TILE_WIDTH: f32 = 142.0;
const TILE_HEIGHT: f32 = 128.0;
const GRID_COLUMNS: usize = 6;

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
    entries: Vec<FileEntry>,
    show_hidden: bool,
    status: String,
    path_input: String,
    search_query: Option<String>,
    search_root: Option<PathBuf>,
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
    ToggleHidden(bool),
    DirectoryLoaded(DirectoryRequest, Result<DirectoryListing, FsError>),
    SearchFinished(
        SearchRequest,
        Result<filesystem_core::SearchResults, FsError>,
    ),
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
            Message::Open(path, kind) => self.open_path(path, kind),
            Message::Go(path) => self.visit_path(path),
            Message::Back => self.go_back(),
            Message::Forward => self.go_forward(),
            Message::PathChanged(path) => {
                self.path_input = path;
                Task::none()
            }
            Message::PathSubmit => self.open_path_input(),
            Message::ToggleHidden(show_hidden) => {
                self.show_hidden = show_hidden;
                if self.search_query.is_some() {
                    self.rerun_search()
                } else {
                    self.reload()
                }
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
        let content = column![self.toolbar(), self.grid()]
            .height(Fill)
            .width(Fill)
            .spacing(0);

        container(content)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into()
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

        let drag_region = mouse_area(
            container(space())
                .height(38)
                .width(90)
                .style(style::toolbar_drag_area),
        )
        .on_press(Message::WindowDrag)
        .on_double_click(Message::WindowToggleMaximize);

        let show_hidden = checkbox(self.show_hidden)
            .label("隐藏文件")
            .on_toggle(Message::ToggleHidden)
            .size(16)
            .text_size(13)
            .style(style::checkbox);

        let toolbar = row![
            back,
            forward,
            path_bar,
            drag_region,
            show_hidden,
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

    fn grid(&self) -> Element<'_, Message> {
        let mut rows = column![].spacing(22).padding([28, 36]);

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
                let mut row = row![].spacing(28).height(TILE_HEIGHT);

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

        let view = column![scrollable(rows).height(Fill).width(Fill), footer]
            .height(Fill)
            .width(Fill);

        container(view)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into()
    }

    fn tile(&self, entry: &FileEntry) -> Element<'_, Message> {
        let icon = match entry.kind {
            EntryKind::Directory => folder_icon(),
            EntryKind::File => file_icon(entry),
            EntryKind::Symlink => link_icon(),
            EntryKind::Other => other_icon(),
        };

        let name = text(short_name(&entry.name))
            .size(15)
            .width(TILE_WIDTH)
            .align_x(Horizontal::Center)
            .style(style::primary_text);

        let meta = text(self.entry_subtitle(entry))
            .size(12)
            .width(TILE_WIDTH)
            .align_x(Horizontal::Center)
            .style(style::muted_text);

        let content = column![icon, name, meta]
            .spacing(8)
            .align_x(iced::Center)
            .width(TILE_WIDTH);

        button(content)
            .height(TILE_HEIGHT)
            .width(TILE_WIDTH)
            .padding(8)
            .style(style::tile_button)
            .on_press(Message::Open(entry.path.clone(), entry.kind))
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
            async move { search_file_names(root, query, options) },
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

        Task::perform(async move { scan_dir(path, options) }, move |result| {
            Message::DirectoryLoaded(request.clone(), result)
        })
    }

    fn directory_loaded(
        &mut self,
        request: DirectoryRequest,
        result: Result<DirectoryListing, FsError>,
    ) {
        if self.active_request_id != Some(request.id) {
            return;
        }

        self.active_request_id = None;

        match result {
            Ok(DirectoryListing { path, entries }) => {
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
                self.status = format!("{count} entries");
            }
            Err(error) => {
                self.set_scan_error(error);
            }
        }
    }

    fn search_finished(
        &mut self,
        request: SearchRequest,
        result: Result<filesystem_core::SearchResults, FsError>,
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
                self.status = format!("{count} name matches");
            }
            Err(error) => self.set_scan_error(error),
        }
    }

    fn set_scan_error(&mut self, error: FsError) {
        self.status = error.to_string();
    }

    fn next_request_id(&mut self) -> u64 {
        self.next_request_id += 1;
        self.next_request_id
    }

    fn entry_subtitle(&self, entry: &FileEntry) -> String {
        let Some(root) = &self.search_root else {
            return entry_meta(entry);
        };

        entry
            .path
            .strip_prefix(root)
            .map(|path| path.display().to_string())
            .unwrap_or_else(|_| entry.path.display().to_string())
    }
}

fn folder_icon<'a>() -> Element<'a, Message> {
    let tab = row![
        container(space())
            .width(34)
            .height(12)
            .style(style::folder_tab),
        space().width(Fill),
    ]
    .height(12);

    let body = container(space())
        .width(78)
        .height(54)
        .style(style::folder_body);

    column![tab, body].spacing(0).width(78).into()
}

fn file_icon<'a>(entry: &FileEntry) -> Element<'a, Message> {
    let accent = if entry.name.ends_with(".md") || entry.name.ends_with(".txt") {
        style::ACCENT_GREEN
    } else {
        style::ACCENT_BLUE
    };

    let body = column![
        space().height(8),
        container(space())
            .height(8)
            .width(30)
            .style(move |_| style::plain_block(accent)),
        container(space())
            .height(7)
            .width(38)
            .style(style::file_line),
        container(space())
            .height(7)
            .width(28)
            .style(style::file_line),
        space().height(6),
        container(space())
            .height(22)
            .width(30)
            .style(move |_| style::plain_block(accent)),
    ]
    .align_x(iced::Center)
    .spacing(5);

    container(body)
        .width(58)
        .height(74)
        .padding(6)
        .style(style::file_page)
        .into()
}

fn link_icon<'a>() -> Element<'a, Message> {
    let content = column![
        text("↗").size(24).style(style::primary_text),
        container(space())
            .width(54)
            .height(44)
            .style(style::link_body),
    ]
    .align_x(iced::Center)
    .spacing(0);

    container(content).width(78).height(74).into()
}

fn other_icon<'a>() -> Element<'a, Message> {
    container(text("?").size(28).style(style::primary_text))
        .width(58)
        .height(58)
        .align_x(Horizontal::Center)
        .align_y(Vertical::Center)
        .style(style::other_body)
        .into()
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

fn short_name(name: &str) -> String {
    const MAX_CHARS: usize = 18;
    if name.chars().count() <= MAX_CHARS {
        return name.to_string();
    }

    let mut short = name.chars().take(MAX_CHARS - 1).collect::<String>();
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
    pub const ACCENT_BLUE: Color = Color::from_rgb(0.34, 0.61, 0.92);
    pub const ACCENT_GREEN: Color = Color::from_rgb(0.43, 0.78, 0.68);
    const FOLDER_BODY: Color = Color::from_rgb(0.58, 0.76, 0.93);
    const FOLDER_TAB: Color = Color::from_rgb(0.25, 0.54, 0.88);

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

    pub fn folder_tab(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(FOLDER_TAB)
            .border(Border::default().rounded(6))
    }

    pub fn folder_body(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(FOLDER_BODY)
            .border(Border::default().rounded(8))
            .shadow(Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.20),
                offset: iced::Vector::new(0.0, 2.0),
                blur_radius: 6.0,
            })
    }

    pub fn file_page(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(Color::from_rgb(0.92, 0.92, 0.90))
            .border(Border::default().rounded(5))
            .shadow(Shadow {
                color: Color::from_rgba(0.0, 0.0, 0.0, 0.22),
                offset: iced::Vector::new(0.0, 2.0),
                blur_radius: 6.0,
            })
    }

    pub fn file_line(_theme: &Theme) -> container_style::Style {
        plain_block(Color::from_rgb(0.72, 0.74, 0.72))
    }

    pub fn link_body(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(Color::from_rgb(0.70, 0.72, 0.76))
            .border(Border::default().rounded(8))
    }

    pub fn other_body(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(SURFACE)
            .border(Border::default().rounded(8).color(BORDER).width(1))
    }

    pub fn plain_block(color: Color) -> container_style::Style {
        container_style::Style::default()
            .background(color)
            .border(Border::default().rounded(3))
    }

    pub fn tile_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
        let background = match status {
            button_style::Status::Hovered | button_style::Status::Pressed => Some(SURFACE),
            _ => None,
        };

        button_style::Style {
            background: background.map(Background::Color),
            text_color: TEXT,
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
