use crate::config::*;
use crate::model::PermissionClass;
use filesystem_core::{EntryKind, FileEntry};
use iced::{Point, Rectangle, Size};
use std::time::{SystemTime, UNIX_EPOCH};

pub(crate) fn browser_width_from_window(window_width: f32) -> f32 {
    (window_width - SIDEBAR_WIDTH).max(TILE_WIDTH + GRID_PADDING_LEFT * 2.0)
}

pub(crate) fn icon_grid_columns(browser_width: f32) -> usize {
    let content_width = (browser_width - GRID_PADDING_LEFT * 2.0).max(TILE_WIDTH);
    ((content_width + GRID_COLUMN_SPACING) / (TILE_WIDTH + GRID_COLUMN_SPACING))
        .floor()
        .max(1.0) as usize
}

pub(crate) fn clamped_overlay_x_for_width(browser_width: f32, x: f32, width: f32) -> f32 {
    let max_x = (browser_width - width).max(0.0);
    x.clamp(0.0, max_x)
}

pub(crate) fn clamp_properties_position(window_size: Size, position: Point) -> Point {
    Point::new(
        position
            .x
            .clamp(0.0, (window_size.width - PROPERTIES_DIALOG_WIDTH).max(0.0)),
        position.y.clamp(
            0.0,
            (window_size.height - PROPERTIES_DIALOG_HEIGHT).max(0.0),
        ),
    )
}

pub(crate) fn should_show_blank_context_menu(
    has_selection: bool,
    pointer_over_entry: bool,
) -> bool {
    !has_selection || !pointer_over_entry
}

pub(crate) fn cycle_directory_access(mode: u32, class: PermissionClass) -> u32 {
    let shift = class.shift();
    let bits = (mode >> shift) & 0o7;
    let next = next_directory_access(bits);

    (mode & !(0o7 << shift)) | (next << shift)
}

pub(crate) fn next_directory_access(bits: u32) -> u32 {
    let index = DIRECTORY_ACCESS_PRESETS
        .iter()
        .position(|candidate| *candidate == bits)
        .unwrap_or(0);
    DIRECTORY_ACCESS_PRESETS[(index + 1) % DIRECTORY_ACCESS_PRESETS.len()]
}

pub(crate) fn rect_from_points(start: Point, end: Point) -> Rectangle {
    let x = start.x.min(end.x);
    let y = start.y.min(end.y);
    let width = (start.x - end.x).abs();
    let height = (start.y - end.y).abs();

    Rectangle::new(Point::new(x, y), Size::new(width, height))
}

pub(crate) fn rect_contains(rect: Rectangle, point: Point) -> bool {
    point.x >= rect.x
        && point.x <= rect.x + rect.width
        && point.y >= rect.y
        && point.y <= rect.y + rect.height
}

pub(crate) fn short_name(name: &str) -> String {
    const MAX_CHARS: usize = 18;
    if name.chars().count() <= MAX_CHARS {
        return name.to_string();
    }

    let mut short = name.chars().take(MAX_CHARS - 1).collect::<String>();
    short.push('…');
    short
}

pub(crate) fn short_list_text(value: &str) -> String {
    const MAX_CHARS: usize = 58;
    if value.chars().count() <= MAX_CHARS {
        return value.to_string();
    }

    let mut short = value.chars().take(MAX_CHARS - 1).collect::<String>();
    short.push('…');
    short
}

pub(crate) fn entry_meta(entry: &FileEntry) -> String {
    match (entry.kind, entry.size) {
        (EntryKind::Directory, _) => "Folder".to_string(),
        (EntryKind::Symlink, _) => "Link".to_string(),
        (EntryKind::File, Some(size)) => format_size(size),
        (EntryKind::File, None) => "File".to_string(),
        (EntryKind::Other, _) => "Other".to_string(),
    }
}

pub(crate) fn entry_size(entry: &FileEntry) -> String {
    match (entry.kind, entry.size) {
        (EntryKind::File, Some(size)) => format_size(size),
        _ => "-".to_string(),
    }
}

pub(crate) fn entry_owner(entry: &FileEntry) -> String {
    entry
        .owner
        .map(|owner| owner.to_string())
        .unwrap_or_else(|| "-".to_string())
}

pub(crate) fn format_size(size: u64) -> String {
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

pub(crate) fn format_modified(modified: Option<SystemTime>) -> String {
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

pub(crate) fn owner_label(owner: Option<u32>) -> String {
    owner
        .map(|owner| format!("UID {owner}"))
        .unwrap_or_else(|| "-".to_string())
}

pub(crate) fn group_label(group: Option<u32>) -> String {
    group
        .map(|group| format!("GID {group}"))
        .unwrap_or_else(|| "-".to_string())
}

pub(crate) fn permission_summary(mode: Option<u32>) -> String {
    mode.map(|mode| format!("{:04o}", mode & 0o7777))
        .unwrap_or_else(|| "未知".to_string())
}

pub(crate) fn directory_permission(mode: u32, shift: u8) -> String {
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
    use crate::components::rename_limit_error;
    use crate::model::{PermissionClass, RenameState};
    use filesystem_core::ChildPathLimits;
    use std::path::PathBuf;

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
    fn context_menu_x_clamps_to_fixed_width_inside_browser() {
        assert_eq!(clamped_overlay_x_for_width(500.0, 480.0, 184.0), 316.0);
        assert_eq!(clamped_overlay_x_for_width(500.0, -12.0, 184.0), 0.0);
    }

    #[test]
    fn context_menu_x_clamps_to_zero_when_menu_is_wider_than_browser() {
        assert_eq!(clamped_overlay_x_for_width(120.0, 80.0, 184.0), 0.0);
    }

    #[test]
    fn blank_context_menu_opens_on_entries_when_nothing_is_selected() {
        assert!(should_show_blank_context_menu(false, true));
        assert!(should_show_blank_context_menu(false, false));
        assert!(should_show_blank_context_menu(true, false));
        assert!(!should_show_blank_context_menu(true, true));
    }

    #[test]
    fn properties_position_clamps_inside_window() {
        let position = clamp_properties_position(Size::new(800.0, 600.0), Point::new(780.0, -8.0));

        assert_eq!(position.x, 340.0);
        assert_eq!(position.y, 0.0);
    }

    #[test]
    fn cycle_directory_access_changes_only_requested_class() {
        let mode = cycle_directory_access(0o754, PermissionClass::Group);

        assert_eq!(mode, 0o774);
    }

    #[test]
    fn next_directory_access_wraps_known_presets() {
        assert_eq!(next_directory_access(0o0), 0o1);
        assert_eq!(next_directory_access(0o7), 0o0);
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
