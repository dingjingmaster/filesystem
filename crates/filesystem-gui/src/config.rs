use iced::{window, Size};
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

pub(crate) const APP_NAME_EN: &str = "File";
pub(crate) const APP_NAME_ZH: &str = "文件";
pub(crate) const RUNTIME_CONFIG_FILE: &str = "filesystem.ini";
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
pub(crate) const TILE_HEIGHT: f32 = 164.0;
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
pub(crate) const CONTEXT_MENU_ITEM_HEIGHT: u32 = 32;
pub(crate) const CONTEXT_MENU_SEPARATOR_HEIGHT: u32 = 1;
pub(crate) const CONTEXT_MENU_PANEL_PADDING: u16 = 6;
pub(crate) const CONTEXT_MENU_ITEM_SPACING: u32 = 2;
pub(crate) const CONTEXT_MENU_SUBMENU_GAP: f32 = 6.0;
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

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct RuntimeConfig {
    pub(crate) name: String,
    pub(crate) terminal: Option<PathBuf>,
    pub(crate) blank_menu_commands: Vec<BlankMenuCommand>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct BlankMenuCommand {
    pub(crate) label: String,
    pub(crate) command: PathBuf,
    pub(crate) args: Vec<String>,
}

impl Default for RuntimeConfig {
    fn default() -> Self {
        Self {
            name: APP_NAME_ZH.to_string(),
            terminal: None,
            blank_menu_commands: Vec::new(),
        }
    }
}

pub(crate) fn load_runtime_config() -> RuntimeConfig {
    runtime_config_path()
        .and_then(|path| load_runtime_config_from_path(&path).ok())
        .unwrap_or_default()
}

fn runtime_config_path() -> Option<PathBuf> {
    let executable = std::env::current_exe().ok()?;
    executable
        .parent()
        .map(|directory| directory.join(RUNTIME_CONFIG_FILE))
}

fn load_runtime_config_from_path(path: &Path) -> io::Result<RuntimeConfig> {
    if !path.is_file() {
        return Ok(RuntimeConfig::default());
    }

    fs::read_to_string(path).map(|contents| parse_runtime_config(&contents))
}

fn parse_runtime_config(contents: &str) -> RuntimeConfig {
    let mut config = RuntimeConfig::default();
    let mut section = RuntimeConfigSection::Root;
    let mut blank_menu_command: Option<BlankMenuCommandBuilder> = None;

    for line in contents.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') || line.starts_with(';') {
            continue;
        }

        if line.starts_with('[') {
            push_blank_menu_command(&mut config, blank_menu_command.take());

            let section_name = line
                .strip_prefix('[')
                .and_then(|line| line.strip_suffix(']'))
                .map(str::trim)
                .unwrap_or_default();

            if section_name.to_ascii_lowercase().starts_with("blank-menu.") {
                section = RuntimeConfigSection::BlankMenu;
                blank_menu_command = Some(BlankMenuCommandBuilder::default());
            } else if section_name.eq_ignore_ascii_case("window") {
                section = RuntimeConfigSection::Root;
            } else {
                section = RuntimeConfigSection::Other;
            }
            continue;
        }

        let Some((key, value)) = line.split_once('=') else {
            continue;
        };
        let key = key.trim().to_ascii_lowercase();
        let value = normalized_ini_value(value);

        match section {
            RuntimeConfigSection::Root => {
                if value.is_empty() {
                    continue;
                }

                match key.as_str() {
                    "name" => config.name = value.to_string(),
                    "terminal" => config.terminal = Some(PathBuf::from(value)),
                    _ => {}
                }
            }
            RuntimeConfigSection::BlankMenu => {
                let Some(command) = blank_menu_command.as_mut() else {
                    continue;
                };

                match key.as_str() {
                    "label" if !value.is_empty() => command.label = Some(value.to_string()),
                    "command" if !value.is_empty() => command.command = Some(PathBuf::from(value)),
                    "arg" => command.args.push(value.to_string()),
                    _ => {}
                }
            }
            RuntimeConfigSection::Other => {}
        }
    }

    push_blank_menu_command(&mut config, blank_menu_command);

    config
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum RuntimeConfigSection {
    Root,
    BlankMenu,
    Other,
}

#[derive(Debug, Default)]
struct BlankMenuCommandBuilder {
    label: Option<String>,
    command: Option<PathBuf>,
    args: Vec<String>,
}

impl BlankMenuCommandBuilder {
    fn build(self) -> Option<BlankMenuCommand> {
        Some(BlankMenuCommand {
            label: self.label?,
            command: self.command?,
            args: self.args,
        })
    }
}

fn push_blank_menu_command(config: &mut RuntimeConfig, command: Option<BlankMenuCommandBuilder>) {
    if let Some(command) = command.and_then(BlankMenuCommandBuilder::build) {
        config.blank_menu_commands.push(command);
    }
}

fn normalized_ini_value(value: &str) -> &str {
    let value = value.trim();

    if let Some(value) = value
        .strip_prefix('"')
        .and_then(|value| value.strip_suffix('"'))
    {
        return value.trim();
    }

    if let Some(value) = value
        .strip_prefix('\'')
        .and_then(|value| value.strip_suffix('\''))
    {
        return value.trim();
    }

    value
}

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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn runtime_config_defaults_without_supported_values() {
        let config = parse_runtime_config(
            r#"
            # comment
            ; another comment
            [window]
            ignored=true
            name=
            terminal=
            "#,
        );

        assert_eq!(config, RuntimeConfig::default());
    }

    #[test]
    fn runtime_config_reads_name_and_terminal() {
        let config = parse_runtime_config(
            r#"
            name = Work Files
            terminal = /usr/bin/gnome-terminal
            "#,
        );

        assert_eq!(config.name, "Work Files");
        assert_eq!(
            config.terminal,
            Some(PathBuf::from("/usr/bin/gnome-terminal"))
        );
    }

    #[test]
    fn runtime_config_reads_window_section_name_and_terminal() {
        let config = parse_runtime_config(
            r#"
            [window]
            name = 沙盒
            terminal = /usr/bin/gnome-terminal
            "#,
        );

        assert_eq!(config.name, "沙盒");
        assert_eq!(
            config.terminal,
            Some(PathBuf::from("/usr/bin/gnome-terminal"))
        );
    }

    #[test]
    fn runtime_config_reads_blank_menu_commands_in_section_order() {
        let config = parse_runtime_config(
            r#"
            [blank-menu.open-terminal]
            label = 用 WezTerm 打开
            command = /usr/bin/wezterm
            arg = start
            arg = --cwd
            arg = {cwd}

            [blank-menu.custom-script]
            label = 执行整理脚本
            command = /home/user/bin/organize-folder
            arg = {cwd}
            "#,
        );

        assert_eq!(config.blank_menu_commands.len(), 2);
        assert_eq!(config.blank_menu_commands[0].label, "用 WezTerm 打开");
        assert_eq!(
            config.blank_menu_commands[0].command,
            PathBuf::from("/usr/bin/wezterm")
        );
        assert_eq!(
            config.blank_menu_commands[0].args,
            vec!["start", "--cwd", "{cwd}"]
        );
        assert_eq!(config.blank_menu_commands[1].label, "执行整理脚本");
        assert_eq!(
            config.blank_menu_commands[1].command,
            PathBuf::from("/home/user/bin/organize-folder")
        );
        assert_eq!(config.blank_menu_commands[1].args, vec!["{cwd}"]);
    }

    #[test]
    fn runtime_config_ignores_incomplete_blank_menu_commands() {
        let config = parse_runtime_config(
            r#"
            [blank-menu.no-label]
            command = /usr/bin/tool

            [blank-menu.no-command]
            label = Broken

            [other]
            label = Ignored
            command = /usr/bin/ignored
            "#,
        );

        assert!(config.blank_menu_commands.is_empty());
    }

    #[test]
    fn runtime_config_trims_quotes_and_uses_last_supported_value() {
        let config = parse_runtime_config(
            r#"
            NAME = "First"
            name = 'Second'
            TERMINAL = "/opt/Terminal/bin/terminal"
            "#,
        );

        assert_eq!(config.name, "Second");
        assert_eq!(
            config.terminal,
            Some(PathBuf::from("/opt/Terminal/bin/terminal"))
        );
    }
}
