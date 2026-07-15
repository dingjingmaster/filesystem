use crate::config::*;
use crate::model::PermissionClass;
use filesystem_core::{EntryKind, FileEntry, SymlinkTargetKind};
use iced::{Point, Rectangle, Size};
use std::collections::BTreeMap;
use std::fs;
use std::mem::MaybeUninit;
use std::sync::OnceLock;
use std::time::{SystemTime, UNIX_EPOCH};

const PASSWD_PATH: &str = "/etc/passwd";

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

pub(crate) fn clamped_overlay_y_for_height(browser_height: f32, y: f32, height: f32) -> f32 {
    let max_y = (browser_height - height).max(0.0);
    y.clamp(0.0, max_y)
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

pub(crate) fn display_name_for_path(path: &std::path::Path) -> String {
    path.file_name()
        .map(|name| name.to_string_lossy().into_owned())
        .unwrap_or_else(|| path.display().to_string())
}

pub(crate) fn short_name(name: &str) -> String {
    const MAX_CHARS: usize = 18;
    middle_ellipsis(name, MAX_CHARS)
}

pub(crate) fn short_list_text(value: &str) -> String {
    const MAX_CHARS: usize = 58;
    middle_ellipsis(value, MAX_CHARS)
}

pub(crate) fn middle_ellipsis(value: &str, max_chars: usize) -> String {
    let char_count = value.chars().count();
    if char_count <= max_chars {
        return value.to_string();
    }

    if max_chars == 0 {
        return String::new();
    }

    if max_chars == 1 {
        return "…".to_string();
    }

    let keep = max_chars - 1;
    let head = keep / 2;
    let tail = keep - head;
    let prefix = value.chars().take(head).collect::<String>();
    let suffix = value
        .chars()
        .rev()
        .take(tail)
        .collect::<Vec<_>>()
        .into_iter()
        .rev()
        .collect::<String>();

    format!("{prefix}…{suffix}")
}

pub(crate) fn entry_meta(entry: &FileEntry) -> String {
    match entry.kind {
        EntryKind::Directory => "Folder".to_string(),
        EntryKind::Symlink => match entry.symlink_target.as_ref() {
            Some(target) if target.broken => "Broken Link".to_string(),
            Some(target) if target.kind == SymlinkTargetKind::Directory => {
                "Folder Link".to_string()
            }
            Some(target) if target.kind == SymlinkTargetKind::File => entry
                .size
                .map(format_size)
                .unwrap_or_else(|| "File Link".to_string()),
            _ => "Link".to_string(),
        },
        EntryKind::File => entry
            .size
            .map(format_size)
            .unwrap_or_else(|| "File".to_string()),
        EntryKind::Other => "Other".to_string(),
    }
}

pub(crate) fn entry_size(entry: &FileEntry) -> String {
    match entry.kind {
        EntryKind::File | EntryKind::Symlink => {
            entry.size.map(format_size).unwrap_or("-".to_string())
        }
        _ => "-".to_string(),
    }
}

pub(crate) fn entry_owner(entry: &FileEntry) -> String {
    entry
        .owner
        .map(|owner| owner_display_name(owner, user_names_by_uid()))
        .unwrap_or_else(|| "-".to_string())
}

fn owner_display_name(owner: u32, users: &BTreeMap<u32, String>) -> String {
    users
        .get(&owner)
        .cloned()
        .unwrap_or_else(|| owner.to_string())
}

fn user_names_by_uid() -> &'static BTreeMap<u32, String> {
    static USERS: OnceLock<BTreeMap<u32, String>> = OnceLock::new();

    USERS.get_or_init(|| {
        fs::read_to_string(PASSWD_PATH)
            .map(|contents| parse_passwd_users(&contents))
            .unwrap_or_default()
    })
}

fn parse_passwd_users(contents: &str) -> BTreeMap<u32, String> {
    let mut users = BTreeMap::new();

    for line in contents.lines() {
        let mut fields = line.split(':');
        let Some(name) = fields.next() else {
            continue;
        };
        if name.is_empty() {
            continue;
        }

        let _password = fields.next();
        let Some(uid) = fields.next().and_then(|uid| uid.parse::<u32>().ok()) else {
            continue;
        };

        users.entry(uid).or_insert_with(|| name.to_string());
    }

    users
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

    let Some(seconds) = system_time_to_unix_seconds(modified) else {
        return "-".to_string();
    };

    format_unix_seconds(seconds)
}

fn system_time_to_unix_seconds(time: SystemTime) -> Option<i64> {
    match time.duration_since(UNIX_EPOCH) {
        Ok(duration) => i64::try_from(duration.as_secs()).ok(),
        Err(error) => {
            let duration = error.duration();
            let seconds = i64::try_from(duration.as_secs()).ok()?;
            if duration.subsec_nanos() == 0 {
                seconds.checked_neg()
            } else {
                seconds.checked_add(1)?.checked_neg()
            }
        }
    }
}

fn format_unix_seconds(seconds: i64) -> String {
    format_unix_seconds_with(seconds, local_datetime_parts)
}

fn format_unix_seconds_with(
    seconds: i64,
    local_time: impl FnOnce(i64) -> Option<LocalDateTimeParts>,
) -> String {
    local_time(seconds)
        .map(format_local_datetime)
        .unwrap_or_else(|| "-".to_string())
}

fn local_datetime_parts(seconds: i64) -> Option<LocalDateTimeParts> {
    let timestamp = libc::time_t::try_from(seconds).ok()?;
    let mut local = MaybeUninit::<libc::tm>::uninit();

    // SAFETY: `timestamp` is a valid time_t value and `local` points to
    // writable uninitialized storage for `localtime_r` to fill.
    let result = unsafe { libc::localtime_r(&timestamp, local.as_mut_ptr()) };
    if result.is_null() {
        return None;
    }

    // SAFETY: `localtime_r` returned non-null, so it initialized `local`.
    let local = unsafe { local.assume_init() };

    Some(LocalDateTimeParts {
        year: local.tm_year + 1900,
        month: local.tm_mon + 1,
        day: local.tm_mday,
        hour: local.tm_hour,
        minute: local.tm_min,
    })
}

fn format_local_datetime(parts: LocalDateTimeParts) -> String {
    let LocalDateTimeParts {
        year,
        month,
        day,
        hour,
        minute,
    } = parts;

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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
struct LocalDateTimeParts {
    year: i32,
    month: i32,
    day: i32,
    hour: i32,
    minute: i32,
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
    fn context_menu_y_clamps_to_fixed_height_inside_browser() {
        assert_eq!(clamped_overlay_y_for_height(546.0, 520.0, 112.0), 434.0);
        assert_eq!(clamped_overlay_y_for_height(546.0, -12.0, 112.0), 0.0);
    }

    #[test]
    fn context_menu_y_clamps_to_zero_when_menu_is_taller_than_browser() {
        assert_eq!(clamped_overlay_y_for_height(80.0, 40.0, 112.0), 0.0);
    }

    #[test]
    fn blank_context_menu_opens_on_entries_when_nothing_is_selected() {
        assert!(should_show_blank_context_menu(false, true));
        assert!(should_show_blank_context_menu(false, false));
        assert!(should_show_blank_context_menu(true, false));
        assert!(!should_show_blank_context_menu(true, true));
    }

    #[test]
    fn middle_ellipsis_keeps_name_start_and_end() {
        assert_eq!(middle_ellipsis("short.txt", 18), "short.txt");
        assert_eq!(
            middle_ellipsis("very-long-document-name.docx", 18),
            "very-lon…name.docx"
        );
        assert_eq!(middle_ellipsis("abcdef", 1), "…");
        assert_eq!(middle_ellipsis("abcdef", 0), "");
    }

    #[test]
    fn passwd_users_parse_names_by_uid() {
        let users = parse_passwd_users(
            "root:x:0:0:root:/root:/bin/sh\n\
             alice:x:1000:1000:Alice:/home/alice:/bin/bash\n\
             duplicate:x:1000:1000:Duplicate:/home/duplicate:/bin/bash\n\
             broken:x:not-a-uid:1000:Broken:/home/broken:/bin/bash\n\
             :x:1001:1001:No Name:/home/noname:/bin/bash\n",
        );

        assert_eq!(users.get(&0).map(String::as_str), Some("root"));
        assert_eq!(users.get(&1000).map(String::as_str), Some("alice"));
        assert!(!users.contains_key(&1001));
    }

    #[test]
    fn owner_display_name_falls_back_to_uid_when_unknown() {
        let users = BTreeMap::from([(1000, "alice".to_string())]);

        assert_eq!(owner_display_name(1000, &users), "alice");
        assert_eq!(owner_display_name(42, &users), "42");
    }

    #[test]
    fn system_time_to_unix_seconds_floors_before_epoch() {
        assert_eq!(system_time_to_unix_seconds(UNIX_EPOCH), Some(0));
        assert_eq!(
            system_time_to_unix_seconds(UNIX_EPOCH - std::time::Duration::from_nanos(1)),
            Some(-1)
        );
    }

    #[test]
    fn format_unix_seconds_uses_local_time_parts() {
        let formatted = format_unix_seconds_with(0, |_| {
            Some(LocalDateTimeParts {
                year: 1970,
                month: 1,
                day: 1,
                hour: 8,
                minute: 5,
            })
        });

        assert_eq!(formatted, "1970-01-01 08:05");
        assert_eq!(format_unix_seconds_with(0, |_| None), "-");
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
