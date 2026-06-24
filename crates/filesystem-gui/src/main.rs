use filesystem_core::{DirectoryListing, EntryKind, FileEntry, ScanOptions, scan_dir};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{button, checkbox, column, container, row, scrollable, space, text};
use iced::widget::{button as button_style, container as container_style};
use iced::{Background, Border, Color, Element, Fill, Result, Shadow, Task, Theme};
use std::path::{Path, PathBuf};

const SIDEBAR_WIDTH: f32 = 240.0;
const TOOLBAR_HEIGHT: f32 = 54.0;
const TILE_WIDTH: f32 = 142.0;
const TILE_HEIGHT: f32 = 128.0;
const GRID_COLUMNS: usize = 6;

pub fn main() -> Result {
    iced::application(FileManager::new, FileManager::update, FileManager::view)
        .title(|manager: &FileManager| format!("Filesystem - {}", manager.cwd.display()))
        .theme(Theme::Dark)
        .window_size((1220, 760))
        .run()
}

struct FileManager {
    cwd: PathBuf,
    entries: Vec<FileEntry>,
    show_hidden: bool,
    status: String,
    home: Option<PathBuf>,
}

#[derive(Debug, Clone)]
enum Message {
    Open(PathBuf),
    Go(PathBuf),
    Up,
    Refresh,
    ToggleHidden(bool),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum NavKind {
    Home,
    Current,
    Root,
    Data,
    Tmp,
    Downloads,
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
            home,
        };
        manager.reload();
        (manager, Task::none())
    }

    fn update(&mut self, message: Message) {
        match message {
            Message::Open(path) | Message::Go(path) => self.open_path(path),
            Message::Up => {
                if let Some(parent) = self.cwd.parent() {
                    self.cwd = parent.to_path_buf();
                    self.reload();
                }
            }
            Message::Refresh => self.reload(),
            Message::ToggleHidden(show_hidden) => {
                self.show_hidden = show_hidden;
                self.reload();
            }
        }
    }

    fn view(&self) -> Element<'_, Message> {
        let shell = row![self.sidebar(), self.main_area()]
            .height(Fill)
            .width(Fill);

        container(shell)
            .height(Fill)
            .width(Fill)
            .style(style::app_background)
            .into()
    }

    fn sidebar(&self) -> Element<'_, Message> {
        let mut quick = column![
            self.nav_item(NavKind::Home, "⌂", "主文件夹"),
            self.nav_item(NavKind::Current, "◉", "当前位置"),
            self.nav_item(NavKind::Root, "⌁", "根目录"),
        ]
        .spacing(4);

        if Path::new("/data").exists() {
            quick = quick.push(self.nav_item(NavKind::Data, "▣", "data"));
        }

        quick = quick.push(self.nav_item(NavKind::Tmp, "▢", "tmp"));

        if self.downloads_path().is_some() {
            quick = quick.push(self.nav_item(NavKind::Downloads, "↓", "Downloads"));
        }

        let header = row![
            text("⌕").size(24).style(style::muted_text),
            text("文件").size(16).style(style::primary_text),
            space().width(Fill),
            text("☰").size(20).style(style::muted_text),
        ]
        .align_y(iced::Center)
        .height(46);

        let content = column![
            header,
            quick,
            style::divider(),
            self.placeholder_item("◷", "最近"),
            self.placeholder_item("★", "收藏"),
            self.placeholder_item("♲", "本地回收站"),
            style::divider(),
            self.path_bookmark("/data/source"),
            self.path_bookmark("/data/code"),
            self.path_bookmark("/tmp"),
        ]
        .spacing(8)
        .padding([10, 10]);

        container(content)
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

        let path_bar = container(self.breadcrumbs())
            .height(38)
            .width(Fill)
            .padding([0, 12])
            .style(style::path_bar);

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
            button(text("⋮").size(24).style(style::icon_text))
                .height(36)
                .width(36)
                .padding(0)
                .style(style::toolbar_button),
            show_hidden,
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

    fn breadcrumbs(&self) -> Element<'_, Message> {
        let mut parts = row![text("▣").size(15).style(style::muted_text)]
            .spacing(8)
            .align_y(iced::Center);

        for segment in breadcrumb_segments(&self.cwd) {
            parts = parts.push(text(segment).size(15).style(style::path_text));
            parts = parts.push(text("/").size(15).style(style::muted_text));
        }

        parts.into()
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
            NavKind::Home => self.home.clone().unwrap_or_else(|| PathBuf::from("/")),
            NavKind::Current => self.cwd.clone(),
            NavKind::Root => PathBuf::from("/"),
            NavKind::Data => PathBuf::from("/data"),
            NavKind::Tmp => PathBuf::from("/tmp"),
            NavKind::Downloads => self.downloads_path().unwrap_or_else(|| self.cwd.clone()),
        };

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

    fn placeholder_item(&self, icon: &'static str, label: &'static str) -> Element<'_, Message> {
        container(
            row![
                text(icon).size(17).style(style::disabled_text),
                text(label).size(15).style(style::disabled_text),
            ]
            .spacing(10)
            .align_y(iced::Center),
        )
        .height(34)
        .width(Fill)
        .padding([0, 12])
        .into()
    }

    fn path_bookmark(&self, path: &'static str) -> Element<'_, Message> {
        if Path::new(path).exists() {
            let active = self.cwd == PathBuf::from(path);
            let label = Path::new(path)
                .file_name()
                .and_then(|name| name.to_str())
                .unwrap_or(path);

            let content = row![
                text("▣").size(15).style(style::sidebar_icon_text),
                text(label).size(14).style(style::primary_text),
            ]
            .spacing(10)
            .align_y(iced::Center);

            button(content)
                .height(34)
                .width(Fill)
                .padding([0, 12])
                .style(move |theme, status| style::sidebar_button(theme, status, active))
                .on_press(Message::Go(PathBuf::from(path)))
                .into()
        } else {
            container(space()).height(0).into()
        }
    }

    fn downloads_path(&self) -> Option<PathBuf> {
        self.home
            .as_ref()
            .map(|home| home.join("Downloads"))
            .filter(|path| path.exists())
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

fn breadcrumb_segments(path: &Path) -> Vec<String> {
    let mut segments = Vec::new();

    if path.is_absolute() {
        segments.push("/".to_string());
    }

    for component in path.components() {
        if let std::path::Component::Normal(segment) = component {
            segments.push(segment.to_string_lossy().into_owned());
        }
    }

    if segments.is_empty() {
        segments.push(path.display().to_string());
    }

    segments
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

mod style {
    use super::*;
    use iced::widget::checkbox as checkbox_widget;

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

    pub fn path_bar(_theme: &Theme) -> container_style::Style {
        container_style::Style::default()
            .background(SURFACE)
            .color(TEXT)
            .border(Border::default().rounded(6).color(BORDER).width(1))
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

    pub fn disabled_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style {
            color: Some(DISABLED),
        }
    }

    pub fn icon_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style { color: Some(MUTED) }
    }

    pub fn sidebar_icon_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style {
            color: Some(Color::from_rgb(0.78, 0.78, 0.82)),
        }
    }

    pub fn path_text(_theme: &Theme) -> iced::widget::text::Style {
        iced::widget::text::Style {
            color: Some(Color::from_rgb(0.74, 0.74, 0.78)),
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
