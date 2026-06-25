use crate::model::Message;
use iced::widget::checkbox as checkbox_widget;
use iced::widget::text_editor as text_editor_widget;
use iced::widget::text_input as text_input_widget;
use iced::widget::{
    button as button_style, container, container as container_style, space, svg as svg_style,
};
use iced::{Background, Border, Color, Element, Fill, Shadow, Theme};

pub(crate) const CONTENT: Color = Color::from_rgb(0.10, 0.10, 0.12);
pub(crate) const SIDEBAR: Color = Color::from_rgb(0.15, 0.15, 0.18);
pub(crate) const TOOLBAR: Color = Color::from_rgb(0.13, 0.13, 0.15);
pub(crate) const SURFACE: Color = Color::from_rgb(0.18, 0.18, 0.21);
pub(crate) const SURFACE_HOVER: Color = Color::from_rgb(0.24, 0.24, 0.27);
pub(crate) const TEXT: Color = Color::from_rgb(0.91, 0.91, 0.93);
pub(crate) const MUTED: Color = Color::from_rgb(0.60, 0.60, 0.64);
pub(crate) const DISABLED: Color = Color::from_rgb(0.42, 0.42, 0.46);
pub(crate) const BORDER: Color = Color::from_rgb(0.22, 0.22, 0.25);
const SELECTION: Color = Color::from_rgb(0.31, 0.43, 0.62);

pub(crate) fn app_background(_theme: &Theme) -> container_style::Style {
    container_style::Style::default().background(CONTENT)
}

pub(crate) fn sidebar(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(SIDEBAR)
        .color(TEXT)
        .border(Border::default().color(BORDER).width(1))
}

pub(crate) fn sidebar_drag_area(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(SIDEBAR)
        .color(TEXT)
}

pub(crate) fn toolbar(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(TOOLBAR)
        .color(TEXT)
        .border(Border::default().color(BORDER).width(1))
}

pub(crate) fn content_background(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(CONTENT)
        .color(TEXT)
}

pub(crate) fn toolbar_drag_area(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(Color::TRANSPARENT)
        .color(TEXT)
}

pub(crate) fn resize_hit_area(_theme: &Theme) -> container_style::Style {
    container_style::Style::default().background(Color::TRANSPARENT)
}

pub(crate) fn path_input(
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

pub(crate) fn rename_editor(
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

pub(crate) fn tile_container(selected: bool, dimmed: bool) -> container_style::Style {
    let border = if selected {
        Border::default().rounded(8).color(SELECTION).width(1)
    } else if dimmed {
        Border::default().rounded(8).color(BORDER).width(1)
    } else {
        Border::default().rounded(8)
    };

    let style = container_style::Style::default().color(TEXT).border(border);

    if selected {
        style.background(SURFACE)
    } else if dimmed {
        style.background(Color::from_rgba(0.18, 0.18, 0.21, 0.35))
    } else {
        style
    }
}

pub(crate) fn list_header(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(Color::from_rgb(0.12, 0.12, 0.14))
        .border(Border::default().color(BORDER).width(1).rounded(6))
}

pub(crate) fn list_row_container(selected: bool, dimmed: bool) -> container_style::Style {
    let background = if selected {
        SURFACE_HOVER
    } else if dimmed {
        Color::from_rgba(0.18, 0.18, 0.21, 0.35)
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

pub(crate) fn selection_rectangle(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(Color::from_rgba(0.31, 0.43, 0.62, 0.22))
        .border(Border::default().rounded(3).color(SELECTION).width(1))
}

pub(crate) fn menu_panel(_theme: &Theme) -> container_style::Style {
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

pub(crate) fn context_menu(theme: &Theme) -> container_style::Style {
    menu_panel(theme)
}

pub(crate) fn modal_overlay(_theme: &Theme) -> container_style::Style {
    container_style::Style::default().background(Color::from_rgba(0.0, 0.0, 0.0, 0.18))
}

pub(crate) fn properties_dialog(_theme: &Theme) -> container_style::Style {
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

pub(crate) fn property_group(_theme: &Theme) -> container_style::Style {
    container_style::Style::default()
        .background(SURFACE)
        .color(TEXT)
        .border(Border::default().rounded(8).color(BORDER).width(1))
}

pub(crate) fn menu_button(
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

pub(crate) fn property_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
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

pub(crate) fn disabled_button(
    _theme: &Theme,
    _status: button_style::Status,
) -> button_style::Style {
    button_style::Style {
        background: Some(Background::Color(Color::from_rgb(0.56, 0.16, 0.22))),
        text_color: Color::from_rgb(0.84, 0.76, 0.78),
        border: Border::default().rounded(8),
        shadow: Shadow::default(),
        snap: true,
    }
}

pub(crate) fn danger_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
    let background = match status {
        button_style::Status::Hovered | button_style::Status::Pressed => {
            Color::from_rgb(0.68, 0.18, 0.22)
        }
        _ => Color::from_rgb(0.56, 0.16, 0.22),
    };

    button_style::Style {
        background: Some(Background::Color(background)),
        text_color: TEXT,
        border: Border::default().rounded(8),
        shadow: Shadow::default(),
        snap: true,
    }
}

pub(crate) fn toolbar_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
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

pub(crate) fn window_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
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

pub(crate) fn close_button(_theme: &Theme, status: button_style::Status) -> button_style::Style {
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

pub(crate) fn window_icon(_theme: &Theme, _status: svg_style::Status) -> svg_style::Style {
    svg_style::Style { color: Some(MUTED) }
}

pub(crate) fn sidebar_button(
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

pub(crate) fn checkbox(_theme: &Theme, status: checkbox_widget::Status) -> checkbox_widget::Style {
    let active = matches!(status, checkbox_widget::Status::Active { is_checked: true });

    checkbox_widget::Style {
        background: Background::Color(if active { SURFACE_HOVER } else { SURFACE }),
        icon_color: TEXT,
        border: Border::default().rounded(4).color(BORDER).width(1),
        text_color: Some(MUTED),
    }
}

pub(crate) fn primary_text(_theme: &Theme) -> iced::widget::text::Style {
    iced::widget::text::Style { color: Some(TEXT) }
}

pub(crate) fn muted_text(_theme: &Theme) -> iced::widget::text::Style {
    iced::widget::text::Style { color: Some(MUTED) }
}

pub(crate) fn disabled_text(_theme: &Theme) -> iced::widget::text::Style {
    iced::widget::text::Style {
        color: Some(DISABLED),
    }
}

pub(crate) fn divider<'a>() -> Element<'a, Message> {
    container(space())
        .height(1)
        .width(Fill)
        .style(|_| container_style::Style::default().background(Color::from_rgb(0.22, 0.22, 0.25)))
        .into()
}
