use iced::{window, Size};

pub(crate) const APP_NAME_EN: &str = "File";
pub(crate) const APP_NAME_ZH: &str = "文件";
pub(crate) const WINDOW_ICON_SIZE: u32 = 128;
pub(crate) const WINDOW_INITIAL_WIDTH: f32 = 1220.0;
pub(crate) const WINDOW_INITIAL_HEIGHT: f32 = 760.0;
pub(crate) const WINDOW_MIN_WIDTH: f32 = 800.0;
pub(crate) const WINDOW_MIN_HEIGHT: f32 = 600.0;
pub(crate) const SIDEBAR_WIDTH: f32 = 240.0;
pub(crate) const FILE_OPERATION_POPUP_WIDTH: f32 = 360.0;
pub(crate) const MAX_RUNNING_FILE_OPERATIONS: usize = 3;
pub(crate) const TOOLBAR_HEIGHT: f32 = 54.0;
pub(crate) const RESIZE_HIT_SIZE: f32 = 6.0;
pub(crate) const TILE_WIDTH: f32 = 142.0;
pub(crate) const TILE_HEIGHT: f32 = 128.0;
pub(crate) const LIST_ROW_HEIGHT: f32 = 40.0;
pub(crate) const GRID_PADDING_TOP: f32 = 28.0;
pub(crate) const GRID_PADDING_LEFT: f32 = 36.0;
pub(crate) const GRID_ROW_SPACING: f32 = 22.0;
pub(crate) const GRID_COLUMN_SPACING: f32 = 28.0;
pub(crate) const LIST_PADDING_TOP: f32 = 18.0;
pub(crate) const LIST_PADDING_LEFT: f32 = 28.0;
pub(crate) const LIST_HEADER_HEIGHT: f32 = 34.0;
pub(crate) const VIRTUAL_ROW_BUFFER: usize = 4;
pub(crate) const SELECTION_DRAG_THRESHOLD: f32 = 3.0;
pub(crate) const CONTEXT_MENU_WIDTH: f32 = 184.0;
pub(crate) const PROPERTIES_DIALOG_WIDTH: f32 = 460.0;
pub(crate) const PROPERTIES_DIALOG_HEIGHT: f32 = 560.0;
pub(crate) const PROPERTIES_LABEL_WIDTH: f32 = 124.0;
pub(crate) const DIRECTORY_ACCESS_PRESETS: [u32; 5] = [0o0, 0o1, 0o4, 0o5, 0o7];
pub(crate) const RENAME_INPUT_ID: &str = "file-rename-input";
pub(crate) const RENAME_TEXT_SIZE: f32 = 16.0;
pub(crate) const RENAME_LINE_HEIGHT: f32 = 1.5;
pub(crate) const RENAME_VERTICAL_PADDING: f32 = 14.0;
pub(crate) const RENAME_HORIZONTAL_PADDING: f32 = 10.0;
pub(crate) const RENAME_MIN_HEIGHT: f32 = 64.0;
pub(crate) const RENAME_AVERAGE_CHAR_WIDTH: f32 = 12.0;
pub(crate) const RENAME_MAX_WIDTH_MULTIPLIER: f32 = 3.0;
pub(crate) const LIST_RENAME_BASE_WIDTH: f32 = 340.0;
pub(crate) const GRID_RENAME_Y_OFFSET: f32 = 94.0;
pub(crate) const LIST_RENAME_X_OFFSET: f32 = 48.0;

pub(crate) fn window_settings() -> window::Settings {
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
