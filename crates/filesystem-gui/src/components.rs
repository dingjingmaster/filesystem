use crate::config::*;
use crate::model::{DisplayEntry, EntryIcon, Message, PermissionClass, RenameState};
use crate::style;
use crate::utils::directory_permission;
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{button, container, mouse_area, row, space, stack, svg, text, text_editor};
use iced::widget::{container as container_style, svg as svg_style};
use iced::{Element, Fill, Length, Padding, mouse, window};
use std::os::unix::ffi::OsStrExt;

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

pub(crate) fn entry_icon<'a>(icon: &EntryIcon, size: f32) -> Element<'a, Message> {
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
        entry_icon(&entry.icon, 24.0),
        text(value).size(14).style(if dimmed {
            style::disabled_text
        } else {
            style::primary_text
        }),
    ]
    .spacing(10)
    .align_y(iced::Center);

    container(content)
        .width(width)
        .height(Fill)
        .align_y(Vertical::Center)
        .into()
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
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, false))
        .on_press(message)
        .into()
}

pub(crate) fn context_menu_separator<'a>() -> Element<'a, Message> {
    container(space())
        .height(1)
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
    let text = if dimmed {
        text(value).size(13).style(style::disabled_text)
    } else if primary {
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
