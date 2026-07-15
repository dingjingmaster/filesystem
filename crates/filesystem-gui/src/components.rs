use crate::config::*;
use crate::model::{DisplayEntry, EntryIcon, IconBadge, Message, PermissionClass, RenameState};
use crate::style;
use crate::utils::directory_permission;
use iced::advanced::text::{Paragraph as _, Renderer as _};
use iced::advanced::widget::{text as advanced_widget_text, tree, Tree};
use iced::advanced::{layout, renderer, text as advanced_text, Layout, Widget};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{button, container, mouse_area, row, space, stack, svg, text, text_editor};
use iced::widget::{container as container_style, svg as svg_style};
use iced::{mouse, window, Element, Fill, Length, Padding, Rectangle, Renderer, Size, Theme};
use std::os::unix::ffi::OsStrExt;

const ICON_TITLE_TEXT_SIZE: f32 = 15.0;
const ICON_TITLE_HEIGHT: f32 = 38.0;
const ICON_TITLE_MEASURE_HEIGHT: f32 = 4096.0;
const ICON_TITLE_LINE_HEIGHT: f32 = 1.2;
const ICON_TITLE_HEIGHT_TOLERANCE: f32 = 0.5;
const LIST_NAME_ICON_SIZE: f32 = 24.0;
const LIST_NAME_ICON_SPACING: f32 = 10.0;
const LIST_NAME_TEXT_SIZE: f32 = 14.0;
const LIST_NAME_LINE_HEIGHT: f32 = 1.25;
const LIST_NAME_WIDTH_BUCKET: f32 = 8.0;
const TEXT_WIDTH_TOLERANCE: f32 = 0.5;

type UiTextFont = <Renderer as advanced_text::Renderer>::Font;
type UiParagraph = <Renderer as advanced_text::Renderer>::Paragraph;

pub(crate) fn toolbar_icon<'a>(bytes: &'static [u8], enabled: bool) -> Element<'a, Message> {
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

pub(crate) fn entry_icon(icon: &EntryIcon, size: f32) -> Element<'static, Message> {
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

pub(crate) fn badged_entry_icon(entry: &DisplayEntry, size: f32) -> Element<'static, Message> {
    let Some(badge) = entry.badge else {
        return entry_icon(&entry.icon, size);
    };

    let badge_bytes = match badge {
        IconBadge::Symlink => include_bytes!("../../../icons/symbol.svg").as_slice(),
        IconBadge::BrokenSymlink => {
            include_bytes!("../../../icons/symbol-disconnect.svg").as_slice()
        }
    };
    let badge_size = (size * 0.36).clamp(12.0, 24.0);

    stack([
        entry_icon(&entry.icon, size),
        container(
            svg(svg::Handle::from_memory(badge_bytes))
                .width(badge_size)
                .height(badge_size),
        )
        .width(size)
        .height(size)
        .align_x(Horizontal::Right)
        .align_y(Vertical::Top)
        .into(),
    ])
    .width(size)
    .height(size)
    .into()
}

pub(crate) fn icon_title<'a>(name: String, dimmed: bool) -> Element<'a, Message> {
    Element::new(IconTitle { name, dimmed })
}

struct IconTitle {
    name: String,
    dimmed: bool,
}

#[derive(Default)]
struct IconTitleState {
    source_name: String,
    available_width: f32,
    visible_name: String,
    paragraph: advanced_widget_text::State<UiParagraph>,
}

impl Widget<Message, Theme, Renderer> for IconTitle {
    fn tag(&self) -> tree::Tag {
        tree::Tag::of::<IconTitleState>()
    }

    fn state(&self) -> tree::State {
        tree::State::new(IconTitleState::default())
    }

    fn size(&self) -> Size<Length> {
        Size::new(Length::Fill, Length::Fixed(ICON_TITLE_HEIGHT))
    }

    fn layout(
        &mut self,
        tree: &mut Tree,
        renderer: &Renderer,
        limits: &layout::Limits,
    ) -> layout::Node {
        let state = tree.state.downcast_mut::<IconTitleState>();
        let available_width = limits.max().width.max(0.0);
        let width_changed = (state.available_width - available_width).abs() > TEXT_WIDTH_TOLERANCE;

        if state.source_name != self.name || width_changed {
            state.visible_name = fit_icon_title(&self.name, renderer, available_width);
            self.name.clone_into(&mut state.source_name);
            state.available_width = available_width;
        }

        advanced_widget_text::layout(
            &mut state.paragraph,
            renderer,
            limits,
            &state.visible_name,
            icon_title_format(),
        )
    }

    fn draw(
        &self,
        tree: &Tree,
        renderer: &mut Renderer,
        _theme: &Theme,
        defaults: &renderer::Style,
        layout: Layout<'_>,
        _cursor_position: mouse::Cursor,
        viewport: &Rectangle,
    ) {
        let bounds = layout.bounds();
        let Some(clip_bounds) = bounds.intersection(viewport) else {
            return;
        };
        let state = tree.state.downcast_ref::<IconTitleState>();
        let color = if self.dimmed {
            style::DISABLED
        } else {
            style::TEXT
        };

        advanced_widget_text::draw(
            renderer,
            defaults,
            bounds,
            state.paragraph.raw(),
            advanced_widget_text::Style { color: Some(color) },
            &clip_bounds,
        );
    }
}

fn icon_title_format() -> advanced_widget_text::Format<UiTextFont> {
    advanced_widget_text::Format {
        width: Length::Fill,
        height: Length::Fixed(ICON_TITLE_HEIGHT),
        size: Some(ICON_TITLE_TEXT_SIZE.into()),
        font: None,
        line_height: advanced_text::LineHeight::Relative(ICON_TITLE_LINE_HEIGHT),
        align_x: advanced_text::Alignment::Center,
        align_y: Vertical::Top,
        shaping: advanced_text::Shaping::default(),
        wrapping: advanced_text::Wrapping::WordOrGlyph,
    }
}

fn fit_icon_title(name: &str, renderer: &Renderer, available_width: f32) -> String {
    if available_width <= TEXT_WIDTH_TOLERANCE {
        return String::new();
    }

    if icon_title_fits(name, renderer, available_width) {
        return name.to_string();
    }

    let char_count = name.chars().count();
    let mut low = 0;
    let mut high = char_count.saturating_sub(1);

    while low < high {
        let middle = (low + high + 1) / 2;
        let candidate = middle_ellipsis_by_kept_chars(name, middle);

        if icon_title_fits(&candidate, renderer, available_width) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }

    middle_ellipsis_by_kept_chars(name, low)
}

fn icon_title_fits(name: &str, renderer: &Renderer, available_width: f32) -> bool {
    let paragraph = UiParagraph::with_text(icon_title_text(
        name,
        renderer,
        Size::new(available_width, ICON_TITLE_MEASURE_HEIGHT),
    ));

    paragraph.min_height() <= ICON_TITLE_HEIGHT + ICON_TITLE_HEIGHT_TOLERANCE
}

fn icon_title_text<'a>(
    name: &'a str,
    renderer: &Renderer,
    bounds: Size,
) -> advanced_text::Text<&'a str, UiTextFont> {
    advanced_text::Text {
        content: name,
        bounds,
        size: ICON_TITLE_TEXT_SIZE.into(),
        line_height: advanced_text::LineHeight::Relative(ICON_TITLE_LINE_HEIGHT),
        font: renderer.default_font(),
        align_x: advanced_text::Alignment::Center,
        align_y: Vertical::Top,
        shaping: advanced_text::Shaping::default(),
        wrapping: advanced_text::Wrapping::WordOrGlyph,
    }
}

fn middle_ellipsis_by_kept_chars(value: &str, kept_chars: usize) -> String {
    let char_count = value.chars().count();
    if char_count <= kept_chars {
        return value.to_string();
    }

    if kept_chars == 0 {
        return "…".to_string();
    }

    let balanced_tail = kept_chars - kept_chars / 2;
    let extension_tail = trailing_extension_chars(value)
        .unwrap_or(0)
        .min(kept_chars.saturating_sub(1));
    let tail = balanced_tail.max(extension_tail);
    let head = kept_chars - tail;
    let prefix: String = value.chars().take(head).collect();
    let suffix: String = value
        .chars()
        .rev()
        .take(tail)
        .collect::<Vec<_>>()
        .into_iter()
        .rev()
        .collect();

    format!("{prefix}…{suffix}")
}

fn trailing_extension_chars(value: &str) -> Option<usize> {
    let char_count = value.chars().count();
    let dot_byte = value
        .char_indices()
        .rev()
        .find_map(|(index, ch)| (ch == '.').then_some(index))?;

    if dot_byte == 0 {
        return None;
    }

    let extension_chars = value[dot_byte..].chars().count();
    (extension_chars < char_count).then_some(extension_chars)
}

#[cfg(test)]
mod tests {
    use super::middle_ellipsis_by_kept_chars;

    #[test]
    fn icon_title_ellipsis_keeps_name_start_and_end() {
        let shortened = middle_ellipsis_by_kept_chars("安得电子文档安全专项测试报告.docx", 14);

        assert_eq!(shortened, "安得电子文档安…报告.docx");
    }

    #[test]
    fn icon_title_ellipsis_handles_zero_kept_chars() {
        assert_eq!(middle_ellipsis_by_kept_chars("abcdef", 0), "…");
    }

    #[test]
    fn icon_title_ellipsis_preserves_extension_with_small_budget() {
        let shortened = middle_ellipsis_by_kept_chars("安得电子文档安全专项测试报告.docx", 8);

        assert_eq!(shortened, "安得电….docx");
    }

    #[test]
    fn list_name_measure_width_uses_floor_bucket() {
        assert_eq!(super::list_name_measure_width(0.0), 0.0);
        assert_eq!(super::list_name_measure_width(7.0), 7.0);
        assert_eq!(super::list_name_measure_width(15.9), 8.0);
        assert_eq!(super::list_name_measure_width(24.0), 24.0);
    }
}

pub(crate) fn sidebar_icon<'a>(bytes: &'static [u8]) -> Element<'a, Message> {
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

pub(crate) fn operation_progress_circle(
    progress: f32,
    complete: bool,
) -> Element<'static, Message> {
    let progress = progress.clamp(0.0, 1.0);
    let marker = if complete { "✓" } else { "" };

    stack([
        container(
            svg(svg::Handle::from_memory(progress_ring_svg(progress)))
                .width(30)
                .height(30),
        )
        .width(30)
        .height(30)
        .align_x(Horizontal::Center)
        .align_y(Vertical::Center)
        .into(),
        container(text(marker).size(15).style(style::primary_text))
            .width(30)
            .height(30)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .into(),
    ])
    .width(30)
    .height(30)
    .into()
}

fn progress_ring_svg(progress: f32) -> Vec<u8> {
    let radius = 13.0_f32;
    let circumference = 2.0 * std::f32::consts::PI * radius;
    let filled = circumference * progress;
    let empty = circumference - filled;

    format!(
        r##"<svg xmlns="http://www.w3.org/2000/svg" width="30" height="30" viewBox="0 0 30 30">
<circle cx="15" cy="15" r="{radius}" fill="#2e2e36" stroke="#383840" stroke-width="3"/>
<circle cx="15" cy="15" r="{radius}" fill="none" stroke="#6f9ee8" stroke-width="3" stroke-linecap="round" transform="rotate(-90 15 15)" stroke-dasharray="{filled:.3} {empty:.3}"/>
</svg>"##
    )
    .into_bytes()
}

pub(crate) fn resize_layer<'a>() -> Element<'a, Message> {
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

pub(crate) fn window_icon<'a>(bytes: &'static [u8]) -> Element<'a, Message> {
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

pub(crate) fn list_header_cell<'a>(label: &'static str, width: Length) -> Element<'a, Message> {
    container(text(label).size(13).style(style::muted_text))
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .into()
}

pub(crate) fn list_name_cell<'a>(
    entry: &DisplayEntry,
    value: String,
    width: Length,
    dimmed: bool,
) -> Element<'a, Message> {
    let content = row![
        badged_entry_icon(entry, LIST_NAME_ICON_SIZE),
        list_name_label(value, dimmed),
    ]
    .spacing(LIST_NAME_ICON_SPACING)
    .align_y(iced::Center)
    .width(Fill)
    .height(Fill);

    container(content)
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .into()
}

fn list_name_label<'a>(name: String, dimmed: bool) -> Element<'a, Message> {
    Element::new(ListNameLabel { name, dimmed })
}

struct ListNameLabel {
    name: String,
    dimmed: bool,
}

#[derive(Default)]
struct ListNameLabelState {
    source_name: String,
    measured_width: f32,
    visible_name: String,
    paragraph: advanced_widget_text::State<UiParagraph>,
}

impl Widget<Message, Theme, Renderer> for ListNameLabel {
    fn tag(&self) -> tree::Tag {
        tree::Tag::of::<ListNameLabelState>()
    }

    fn state(&self) -> tree::State {
        tree::State::new(ListNameLabelState::default())
    }

    fn size(&self) -> Size<Length> {
        Size::new(Length::Fill, Length::Fill)
    }

    fn layout(
        &mut self,
        tree: &mut Tree,
        renderer: &Renderer,
        limits: &layout::Limits,
    ) -> layout::Node {
        let state = tree.state.downcast_mut::<ListNameLabelState>();
        let available_width = limits.max().width.max(0.0);
        let measured_width = list_name_measure_width(available_width);
        let width_changed = (state.measured_width - measured_width).abs() > TEXT_WIDTH_TOLERANCE;

        if state.source_name != self.name || width_changed {
            state.visible_name = fit_list_name(&self.name, renderer, measured_width);
            self.name.clone_into(&mut state.source_name);
            state.measured_width = measured_width;
        }

        advanced_widget_text::layout(
            &mut state.paragraph,
            renderer,
            limits,
            &state.visible_name,
            list_name_format(),
        )
    }

    fn draw(
        &self,
        tree: &Tree,
        renderer: &mut Renderer,
        _theme: &Theme,
        defaults: &renderer::Style,
        layout: Layout<'_>,
        _cursor_position: mouse::Cursor,
        viewport: &Rectangle,
    ) {
        let bounds = layout.bounds();
        let Some(clip_bounds) = bounds.intersection(viewport) else {
            return;
        };
        let state = tree.state.downcast_ref::<ListNameLabelState>();
        let color = if self.dimmed {
            style::DISABLED
        } else {
            style::TEXT
        };

        advanced_widget_text::draw(
            renderer,
            defaults,
            bounds,
            state.paragraph.raw(),
            advanced_widget_text::Style { color: Some(color) },
            &clip_bounds,
        );
    }
}

fn list_name_format() -> advanced_widget_text::Format<UiTextFont> {
    advanced_widget_text::Format {
        width: Length::Fill,
        height: Length::Fill,
        size: Some(LIST_NAME_TEXT_SIZE.into()),
        font: None,
        line_height: advanced_text::LineHeight::Relative(LIST_NAME_LINE_HEIGHT),
        align_x: advanced_text::Alignment::Default,
        align_y: Vertical::Center,
        shaping: advanced_text::Shaping::default(),
        wrapping: advanced_text::Wrapping::None,
    }
}

fn fit_list_name(name: &str, renderer: &Renderer, available_width: f32) -> String {
    if available_width <= TEXT_WIDTH_TOLERANCE {
        return String::new();
    }

    if list_name_fits(name, renderer, available_width) {
        return name.to_string();
    }

    let char_count = name.chars().count();
    let mut low = 0;
    let mut high = char_count.saturating_sub(1);

    while low < high {
        let middle = (low + high + 1) / 2;
        let candidate = middle_ellipsis_by_kept_chars(name, middle);

        if list_name_fits(&candidate, renderer, available_width) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }

    middle_ellipsis_by_kept_chars(name, low)
}

fn list_name_measure_width(available_width: f32) -> f32 {
    let available_width = available_width.max(0.0);
    if available_width < LIST_NAME_WIDTH_BUCKET {
        available_width
    } else {
        (available_width / LIST_NAME_WIDTH_BUCKET).floor() * LIST_NAME_WIDTH_BUCKET
    }
}

fn list_name_fits(name: &str, renderer: &Renderer, available_width: f32) -> bool {
    let paragraph = UiParagraph::with_text(list_name_text(
        name,
        renderer,
        Size::new(available_width, LIST_ROW_HEIGHT),
    ));

    paragraph.min_width() <= available_width + TEXT_WIDTH_TOLERANCE
}

fn list_name_text<'a>(
    name: &'a str,
    renderer: &Renderer,
    bounds: Size,
) -> advanced_text::Text<&'a str, UiTextFont> {
    advanced_text::Text {
        content: name,
        bounds,
        size: LIST_NAME_TEXT_SIZE.into(),
        line_height: advanced_text::LineHeight::Relative(LIST_NAME_LINE_HEIGHT),
        font: renderer.default_font(),
        align_x: advanced_text::Alignment::Default,
        align_y: Vertical::Center,
        shaping: advanced_text::Shaping::default(),
        wrapping: advanced_text::Wrapping::None,
    }
}

pub(crate) fn rename_editor<'a>(
    rename: &'a RenameState,
    width: f32,
    height: f32,
) -> Element<'a, Message> {
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

pub(crate) fn rename_input_id() -> iced::widget::Id {
    iced::widget::Id::new(RENAME_INPUT_ID)
}

pub(crate) fn rename_limit_error(rename: &RenameState, value: &str) -> Option<String> {
    let name = value.trim();
    if name.is_empty() {
        return None;
    }

    let name_bytes = name.len();
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
pub(crate) struct RenameEditorLayout {
    pub(crate) width: f32,
    pub(crate) height: f32,
}

pub(crate) fn rename_editor_layout(value: &str, base_width: f32) -> RenameEditorLayout {
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

pub(crate) fn context_menu_item<'a>(label: &'static str, message: Message) -> Element<'a, Message> {
    context_menu_item_owned(label.to_string(), message)
}

pub(crate) fn context_menu_item_owned<'a>(label: String, message: Message) -> Element<'a, Message> {
    let content = container(text(label).size(14).style(style::primary_text))
        .height(Fill)
        .width(Fill)
        .align_x(Horizontal::Left)
        .align_y(Vertical::Center);

    button(content)
        .height(CONTEXT_MENU_ITEM_HEIGHT)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, false))
        .on_press(message)
        .into()
}

pub(crate) fn context_menu_separator<'a>() -> Element<'a, Message> {
    container(space())
        .height(CONTEXT_MENU_SEPARATOR_HEIGHT)
        .width(Fill)
        .style(|_| container_style::Style::default().background(style::BORDER))
        .into()
}

pub(crate) fn permission_access_row<'a>(
    label: &'static str,
    mode: u32,
    class: PermissionClass,
    disabled: bool,
) -> Element<'a, Message> {
    let content = container(
        row![
            text(label)
                .size(14)
                .style(style::primary_text)
                .width(Length::Fixed(PROPERTIES_LABEL_WIDTH)),
            text(directory_permission(mode, class.shift()))
                .size(14)
                .style(style::primary_text)
                .width(Fill),
            text("›").size(20).style(style::muted_text),
        ]
        .align_y(iced::Center),
    )
    .height(Fill)
    .width(Fill)
    .align_y(Vertical::Center);

    button(content)
        .height(52)
        .width(Fill)
        .padding([0, 14])
        .style(move |theme, status| style::menu_button(theme, status, false))
        .on_press_maybe((!disabled).then_some(Message::CyclePermission(class)))
        .into()
}

pub(crate) fn property_label_row<'a>(label: &'static str) -> Element<'a, Message> {
    container(text(label).size(15).style(style::primary_text))
        .height(52)
        .width(Fill)
        .padding([0, 14])
        .align_x(Horizontal::Left)
        .align_y(Vertical::Center)
        .into()
}

pub(crate) fn property_row<'a>(label: &'static str, value: String) -> Element<'a, Message> {
    container(
        row![
            text(label)
                .size(14)
                .style(style::primary_text)
                .width(Length::Fixed(PROPERTIES_LABEL_WIDTH)),
            text(value).size(14).style(style::primary_text).width(Fill),
        ]
        .align_y(iced::Center),
    )
    .height(52)
    .width(Fill)
    .padding([0, 14])
    .align_y(Vertical::Center)
    .into()
}

pub(crate) fn list_value_cell<'a>(
    value: String,
    width: Length,
    primary: bool,
    dimmed: bool,
) -> Element<'a, Message> {
    let label = if dimmed {
        text(value).size(13).style(style::disabled_text)
    } else if primary {
        text(value).size(14).style(style::primary_text)
    } else {
        text(value).size(13).style(style::muted_text)
    }
    .width(Fill)
    .wrapping(advanced_text::Wrapping::None);

    container(label)
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .clip(true)
        .into()
}
