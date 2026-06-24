use filesystem_core::{DirectoryListing, EntryKind, FileEntry, ScanOptions, scan_dir};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{
    button, checkbox, column, container, mouse_area, row, scrollable, space, stack, svg, text,
    text_input,
};
use iced::widget::{button as button_style, container as container_style, svg as svg_style};
use iced::{
    Background, Border, Color, Element, Fill, Length, Result, Shadow, Task, Theme, mouse, window,
};
use std::path::PathBuf;

const SIDEBAR_WIDTH: f32 = 240.0;
const TOOLBAR_HEIGHT: f32 = 54.0;
const RESIZE_HIT_SIZE: f32 = 6.0;
const TILE_WIDTH: f32 = 142.0;
const TILE_HEIGHT: f32 = 128.0;
const GRID_COLUMNS: usize = 6;

pub fn main() -> Result {
    iced::application(FileManager::new, FileManager::update, FileManager::view)
        .title(|manager: &FileManager| format!("Filesystem - {}", manager.cwd.display()))
        .theme(Theme::Dark)
        .window_size((1220, 760))
        .decorations(false)
        .run()
}

struct FileManager {
    cwd: PathBuf,
    entries: Vec<FileEntry>,
    show_hidden: bool,
    status: String,
    path_input: String,
    home: Option<PathBuf>,
}

#[derive(Debug, Clone)]
enum Message {
    Open(PathBuf),
    Go(PathBuf),
    Up,
    Refresh,
    PathChanged(String),
    PathSubmit,
    ToggleHidden(bool),
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

struct HomeShortcut {
    icon: &'static str,
    label: &'static str,
    path: PathBuf,
}

impl FileManager {
    fn new() -> (Self, Task<Message>) {
        let cwd = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("/"));
        let home = std::env::var_os("HOME").map(PathBuf::from);
        let mut manager = Self {
            cwd,
            entries: Vec::new(),
            show_hidden: false,
            status: String::new(),
            path_input: String::new(),
            home,
        };
        manager.reload();
        (manager, Task::none())
    }

    fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Open(path) | Message::Go(path) => {
                self.open_path(path);
                Task::none()
            }
            Message::Up => {
                if let Some(parent) = self.cwd.parent() {
                    self.cwd = parent.to_path_buf();
                    self.reload();
                }
                Task::none()
            }
            Message::Refresh => {
                self.reload();
                Task::none()
            }
            Message::PathChanged(path) => {
                self.path_input = path;
                Task::none()
            }
            Message::PathSubmit => {
                self.open_path_input();
                Task::none()
            }
            Message::ToggleHidden(show_hidden) => {
                self.show_hidden = show_hidden;
                self.reload();
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
            container(space())
                .height(TOOLBAR_HEIGHT)
                .width(Fill)
                .style(style::sidebar_drag_area),
        )
        .on_press(Message::WindowDrag)
        .on_double_click(Message::WindowToggleMaximize);

        let quick = column![
            self.nav_item(NavKind::Home, "⌂", "主文件夹"),
            self.nav_item(NavKind::Root, "⌁", "根目录"),
        ]
        .spacing(4);

        let mut content = column![quick].spacing(8).padding([14, 10]);

        let home_shortcuts = self.home_shortcuts();
        if !home_shortcuts.is_empty() {
            content = content.push(style::divider());

            for shortcut in home_shortcuts {
                content = content.push(self.nav_path(shortcut.icon, shortcut.label, shortcut.path));
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
        let can_go_up = self.cwd.parent().is_some();
        let up = button(text("‹").size(30).style(style::icon_text))
            .height(36)
            .width(36)
            .padding(0)
            .style(style::toolbar_button)
            .on_press_maybe(can_go_up.then_some(Message::Up));

        let refresh = button(text("↻").size(20).style(style::icon_text))
            .height(36)
            .width(36)
            .padding(0)
            .style(style::toolbar_button)
            .on_press(Message::Refresh);

        let path_bar = text_input("输入路径", &self.path_input)
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
            up,
            refresh,
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

        let meta = text(entry_meta(entry))
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
            .on_press(Message::Open(entry.path.clone()))
            .into()
    }

    fn nav_item(
        &self,
        kind: NavKind,
        icon: &'static str,
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
        icon: &'static str,
        label: &'static str,
        path: PathBuf,
    ) -> Element<'_, Message> {
        let active = path == self.cwd;
        let content = row![
            text(icon).size(18).style(style::sidebar_icon_text),
            text(label).size(15).style(style::primary_text),
        ]
        .spacing(10)
        .align_y(iced::Center);

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

    fn home_shortcuts(&self) -> Vec<HomeShortcut> {
        let Some(home) = &self.home else {
            return Vec::new();
        };

        let definitions = [
            ("↓", "下载", ["下载", "Downloads"]),
            ("▧", "图片", ["图片", "Pictures"]),
            ("▦", "桌面", ["桌面", "Desktop"]),
            ("▤", "文档", ["文档", "Documents"]),
            ("♫", "音乐", ["音乐", "Music"]),
            ("▶", "视频", ["视频", "Videos"]),
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

    fn open_path(&mut self, path: PathBuf) {
        if path.is_dir() {
            self.cwd = path;
            self.reload();
        } else {
            let name = path
                .file_name()
                .and_then(|value| value.to_str())
                .unwrap_or("file");
            self.status = format!("No opener configured for {name}");
        }
    }

    fn open_path_input(&mut self) {
        let Some(path) = self.resolve_path_input() else {
            self.status = "Path is empty".to_string();
            return;
        };

        if path.is_dir() {
            self.cwd = path;
            self.reload();
        } else if path.exists() {
            self.status = format!("Not a directory: {}", path.display());
        } else {
            self.status = format!("Path does not exist: {}", path.display());
        }
    }

    fn resolve_path_input(&self) -> Option<PathBuf> {
        let input = self.path_input.trim();
        if input.is_empty() {
            return None;
        }

        if input == "~" {
            return Some(self.home_path());
        }

        if let Some(rest) = input.strip_prefix("~/") {
            return Some(self.home_path().join(rest));
        }

        let path = PathBuf::from(input);
        if path.is_absolute() {
            Some(path)
        } else {
            Some(self.cwd.join(path))
        }
    }

    fn reload(&mut self) {
        match scan_dir(
            &self.cwd,
            ScanOptions {
                show_hidden: self.show_hidden,
            },
        ) {
            Ok(DirectoryListing { path, entries }) => {
                let count = entries.len();
                self.cwd = path;
                self.entries = entries;
                self.path_input = self.cwd.display().to_string();
                self.status = format!("{count} entries");
            }
            Err(error) => {
                self.entries.clear();
                self.status = error.to_string();
            }
        }
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

    pub fn icon_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style { color: Some(MUTED) }
    }

    pub fn sidebar_icon_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style {
            color: Some(Color::from_rgb(0.78, 0.78, 0.82)),
        }
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
