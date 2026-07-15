use crate::apps::{apps_for_mime, best_app_for_mime};
use crate::components::*;
use crate::config::*;
use crate::model::*;
use crate::style;
use crate::tasks::*;
use crate::utils::*;
use filesystem_core::{
    child_path_limits, file_properties, folder_properties, parse_clipboard_paths, rename_entry,
    set_permissions, ClipboardPaths, EntryKind, FileEntry, FileOperationCancelToken,
    FileProperties, FolderProperties, FsError, PasteAction, PasteProgress, PasteProgressEvent,
    PasteProgressPhase, ScanOptions, SymlinkTargetKind,
};
use filesystem_mime::{detect_name, MimeInfo};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{
    button, checkbox, column, container, mouse_area, operation, progress_bar, row, scrollable,
    space, stack, svg, text, text_editor, text_input, tooltip,
};
use iced::{
    event, keyboard, mouse, window, Element, Fill, Length, Padding, Point, Rectangle, Size,
    Subscription, Task,
};
use std::cmp::Ordering;
use std::collections::{BTreeMap, BTreeSet};
use std::io;
use std::path::{Path, PathBuf};
use std::time::{Duration, Instant};

const TEMPLATE_SUBMENU_WIDTH: f32 = 220.0;
const CHECK_MARK_SVG: &[u8] = br##"<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 16 16">
<path d="M3.2 8.2 6.4 11.4 12.8 4.6" fill="none" stroke="#f3f6ff" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>"##;

pub(crate) struct FileManager {
    cwd: PathBuf,
    runtime_config: RuntimeConfig,
    entries: Vec<DisplayEntry>,
    show_hidden: bool,
    status: String,
    path_input: String,
    search_query: Option<String>,
    search_root: Option<PathBuf>,
    view_mode: ViewMode,
    menu_open: bool,
    view_submenu_open: bool,
    template_submenu_open: bool,
    context_menu: Option<ContextMenuState>,
    clipboard: Option<ClipboardState>,
    file_operations: BTreeMap<u64, FileOperationState>,
    active_file_operation_ids: BTreeSet<u64>,
    dismissed_file_operations: BTreeSet<u64>,
    file_operations_expanded: bool,
    next_file_operation_id: u64,
    delete_confirm: Option<DeleteConfirm>,
    alert_dialog: Option<AlertDialog>,
    rename_state: Option<RenameState>,
    properties_dialog: Option<PropertiesDialog>,
    properties_drag: Option<PropertiesDrag>,
    properties_pointer: Option<Point>,
    selected_paths: BTreeSet<PathBuf>,
    selection_anchor: Option<PathBuf>,
    selection_modifiers: SelectionModifiers,
    selection_drag: Option<SelectionDrag>,
    browser_generation: u64,
    browser_pointer: Option<Point>,
    browser_scroll_y: f32,
    browser_scroll_id: iced::widget::Id,
    browser_content_id: iced::widget::Id,
    browser_width: f32,
    window_size: Size,
    next_request_id: u64,
    active_request_id: Option<u64>,
    auto_refresh_snapshot_id: Option<u64>,
    auto_refresh_snapshot_dirty: bool,
    back_history: Vec<PathBuf>,
    forward_history: Vec<PathBuf>,
    home: Option<PathBuf>,
    home_shortcuts: Vec<HomeShortcut>,
    template_files: Vec<TemplateFile>,
    app_registry: AppRegistry,
    open_with_dialog: Option<OpenWithDialog>,
}

impl FileManager {
    pub(crate) fn title(&self) -> String {
        format!("{APP_NAME_EN} - {}", self.cwd.display())
    }

    pub(crate) fn new() -> (Self, Task<Message>) {
        let runtime_config = load_runtime_config();
        let cwd = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("/"));
        let home = std::env::var_os("HOME").map(PathBuf::from);
        let shortcuts_task = load_home_shortcuts(home.clone());
        let template_files_task = load_template_files_task(home.clone());
        let app_registry_task = load_app_registry_task();
        let mut manager = Self {
            cwd,
            runtime_config,
            entries: Vec::new(),
            show_hidden: false,
            status: String::new(),
            path_input: String::new(),
            search_query: None,
            search_root: None,
            view_mode: ViewMode::Icons,
            menu_open: false,
            view_submenu_open: false,
            template_submenu_open: false,
            context_menu: None,
            clipboard: None,
            file_operations: BTreeMap::new(),
            active_file_operation_ids: BTreeSet::new(),
            dismissed_file_operations: BTreeSet::new(),
            file_operations_expanded: false,
            next_file_operation_id: 0,
            delete_confirm: None,
            alert_dialog: None,
            rename_state: None,
            properties_dialog: None,
            properties_drag: None,
            properties_pointer: None,
            selected_paths: BTreeSet::new(),
            selection_anchor: None,
            selection_modifiers: SelectionModifiers::default(),
            selection_drag: None,
            browser_generation: 0,
            browser_pointer: None,
            browser_scroll_y: 0.0,
            browser_scroll_id: iced::widget::Id::new("browser-scroll"),
            browser_content_id: iced::widget::Id::new("browser-content"),
            browser_width: browser_width_from_window(WINDOW_INITIAL_WIDTH),
            window_size: Size::new(WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT),
            next_request_id: 0,
            active_request_id: None,
            auto_refresh_snapshot_id: None,
            auto_refresh_snapshot_dirty: false,
            back_history: Vec::new(),
            forward_history: Vec::new(),
            home,
            home_shortcuts: Vec::new(),
            template_files: Vec::new(),
            app_registry: AppRegistry::default(),
            open_with_dialog: None,
        };
        let task = manager.reload();
        (
            manager,
            Task::batch([task, shortcuts_task, template_files_task, app_registry_task]),
        )
    }

    pub(crate) fn update(&mut self, message: Message) -> Task<Message> {
        match message {
            Message::Open(path, kind) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_path(path, kind)
            }
            Message::Go(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.visit_path(path)
            }
            Message::Back => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.go_back()
            }
            Message::Forward => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.go_forward()
            }
            Message::PathChanged(path) => {
                self.path_input = path;
                Task::none()
            }
            Message::PathSubmit => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_path_input()
            }
            Message::SelectEntry(path) => {
                if self
                    .rename_state
                    .as_ref()
                    .is_some_and(|rename| rename.path != path)
                {
                    return self.submit_rename();
                }
                self.close_menu();
                self.select_entry(path);
                Task::none()
            }
            Message::SelectionModifiersChanged(modifiers) => {
                self.selection_modifiers = modifiers;
                Task::none()
            }
            Message::BrowserPointerMoved(generation, position) => {
                if generation != self.browser_generation {
                    return Task::none();
                }

                self.browser_pointer = Some(position);
                if let Some(drag) = &mut self.selection_drag {
                    drag.current = position;
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::BrowserPressed(generation) => {
                if generation != self.browser_generation {
                    return Task::none();
                }

                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                if let Some(position) = self.browser_pointer {
                    self.selection_drag = Some(SelectionDrag {
                        origin: position,
                        current: position,
                    });
                    self.selected_paths.clear();
                    self.selection_anchor = None;
                }
                Task::none()
            }
            Message::BrowserReleased(generation) => {
                if generation != self.browser_generation {
                    return Task::none();
                }

                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                    self.selection_drag = None;
                }
                Task::none()
            }
            Message::BrowserRightPressed(generation) => {
                if generation != self.browser_generation {
                    return Task::none();
                }

                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.menu_open = false;
                self.view_submenu_open = false;
                self.template_submenu_open = false;
                self.selection_drag = None;

                self.context_menu = self.browser_pointer.and_then(|position| {
                    if self.selected_paths.len() > 1 {
                        Some(ContextMenuState::Selection { position })
                    } else if let Some(path) = self.folder_menu_path_at(position) {
                        Some(ContextMenuState::Folder { position, path })
                    } else if let Some(path) = self.file_menu_path_at(position) {
                        Some(ContextMenuState::File { position, path })
                    } else {
                        should_show_blank_context_menu(
                            !self.selected_paths.is_empty(),
                            self.pointer_over_entry(position),
                        )
                        .then_some(ContextMenuState::Blank(position))
                    }
                });

                if matches!(self.context_menu, Some(ContextMenuState::Blank(_))) {
                    load_template_files_task(self.home.clone())
                } else {
                    Task::none()
                }
            }
            Message::BrowserScrolled(generation, offset_y) => {
                if generation != self.browser_generation {
                    return Task::none();
                }

                self.browser_scroll_y = offset_y;
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::WindowResized(size) => {
                self.window_size = size;
                self.browser_width = browser_width_from_window(size.width);
                self.clamp_properties_position();
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::ContextNewFile => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Creating file...".to_string();
                create_entry_task(self.cwd.clone(), NewEntryKind::File)
            }
            Message::ContextNewFolder => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Creating folder...".to_string();
                create_entry_task(self.cwd.clone(), NewEntryKind::Folder)
            }
            Message::ContextToggleTemplateSubmenu => {
                self.template_submenu_open = !self.template_submenu_open;
                Task::none()
            }
            Message::ContextNewTemplateFile(index) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                let Some(template) = self.template_files.get(index).cloned() else {
                    self.status = "Template file is unavailable".to_string();
                    return Task::none();
                };
                self.close_menu();
                self.status = format!("Creating {}...", template.label);
                create_template_file_task(self.cwd.clone(), template.path)
            }
            Message::ContextPaste => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                if let Some(clipboard) = self.clipboard.clone() {
                    return self.paste_paths(clipboard.paths, clipboard.action);
                }
                self.status = "Reading clipboard...".to_string();
                iced::clipboard::read().map(Message::PasteClipboardRead)
            }
            Message::ContextSelectAll => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.selected_paths = self
                    .entries
                    .iter()
                    .map(|entry| entry.file.path.clone())
                    .collect();
                self.selection_anchor = self.entries.first().map(|entry| entry.file.path.clone());
                Task::none()
            }
            Message::ContextOpenTerminal => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Opening terminal...".to_string();
                let cwd = self.cwd.clone();
                let terminal = self.runtime_config.terminal.clone();
                Task::perform(
                    async move { open_terminal(cwd, terminal) },
                    Message::TerminalOpened,
                )
            }
            Message::ContextCustomCommand(index) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                let Some(command) = self.runtime_config.blank_menu_commands.get(index).cloned()
                else {
                    self.status = "Custom command is unavailable".to_string();
                    return Task::none();
                };
                self.status = format!("Running {}...", command.label);
                let cwd = self.cwd.clone();
                Task::perform(
                    async move { run_blank_menu_command(cwd, command) },
                    Message::CustomCommandStarted,
                )
            }
            Message::ContextProperties => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_properties(self.cwd.clone())
            }
            Message::SelectionCopy => {
                if self.shortcuts_blocked() {
                    return Task::none();
                }
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.set_selection_clipboard(PasteAction::Copy)
            }
            Message::SelectionCut => {
                if self.shortcuts_blocked() {
                    return Task::none();
                }
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.set_selection_clipboard(PasteAction::Cut)
            }
            Message::SelectionDelete => {
                if self.shortcuts_blocked() {
                    return Task::none();
                }
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                let paths = self.selected_paths_vec();
                self.show_delete_confirmation(paths, None)
            }
            Message::ShortcutPaste => {
                if self.shortcuts_blocked() || !self.selected_paths.is_empty() {
                    return Task::none();
                }
                self.close_menu();
                if let Some(clipboard) = self.clipboard.clone() {
                    return self.paste_paths(clipboard.paths, clipboard.action);
                }
                self.status = "Reading clipboard...".to_string();
                iced::clipboard::read().map(Message::PasteClipboardRead)
            }
            Message::FolderOpen(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_path(path, EntryKind::Directory)
            }
            Message::FolderCopy(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.clipboard = Some(ClipboardState {
                    action: PasteAction::Copy,
                    paths: vec![path.clone()],
                });
                self.status = format!("Copied {}", display_name_for_path(&path));
                Task::none()
            }
            Message::FolderCut(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.clipboard = Some(ClipboardState {
                    action: PasteAction::Cut,
                    paths: vec![path.clone()],
                });
                self.status = format!("Cut {}", display_name_for_path(&path));
                Task::none()
            }
            Message::FolderRename(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.start_rename(path)
            }
            Message::FolderDelete(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.show_delete_confirmation(vec![path], Some("文件夹"))
            }
            Message::FolderOpenTerminal(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                if self.is_broken_symlink_path(&path) {
                    return self.show_broken_symlink_alert(&path);
                }
                let path = self
                    .entry_for_path(&path)
                    .map(Self::entry_open_path)
                    .unwrap_or(path);
                self.status = "Opening terminal...".to_string();
                let terminal = self.runtime_config.terminal.clone();
                Task::perform(
                    async move { open_terminal(path, terminal) },
                    Message::TerminalOpened,
                )
            }
            Message::FolderProperties(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_properties(path)
            }
            Message::FileOpenDefault(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_path(path, EntryKind::File)
            }
            Message::FileOpenWith(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                if let Some(dialog) = self.open_with_dialog_for(path.clone()) {
                    self.open_with_dialog = Some(dialog);
                    Task::none()
                } else {
                    self.show_broken_symlink_alert(&path)
                }
            }
            Message::FileCopy(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.clipboard = Some(ClipboardState {
                    action: PasteAction::Copy,
                    paths: vec![path.clone()],
                });
                self.status = format!("Copied {}", display_name_for_path(&path));
                Task::none()
            }
            Message::FileCut(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.clipboard = Some(ClipboardState {
                    action: PasteAction::Cut,
                    paths: vec![path.clone()],
                });
                self.status = format!("Cut {}", display_name_for_path(&path));
                Task::none()
            }
            Message::FileRename(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.start_rename(path)
            }
            Message::FileDelete(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.show_delete_confirmation(vec![path], Some("文件"))
            }
            Message::FileProperties(path) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_file_properties(path)
            }
            Message::CancelDelete => {
                self.delete_confirm = None;
                Task::none()
            }
            Message::ConfirmDelete(paths) => {
                self.delete_confirm = None;
                if paths.is_empty() {
                    return Task::none();
                }

                self.status = if paths.len() == 1 {
                    format!("Deleting {}...", display_name_for_path(&paths[0]))
                } else {
                    format!("Deleting {} item(s)...", paths.len())
                };
                delete_entries_task(paths)
            }
            Message::DeleteFinished(outcome) => {
                for path in &outcome.paths {
                    self.selected_paths.remove(path);
                }

                if self.clipboard.as_ref().is_some_and(|clipboard| {
                    clipboard
                        .paths
                        .iter()
                        .any(|item| outcome.paths.iter().any(|path| path == item))
                }) {
                    self.clipboard = None;
                }

                match outcome.error {
                    Some(error) if outcome.paths.is_empty() => {
                        self.status = error.to_string();
                        Task::none()
                    }
                    Some(error) => {
                        self.status =
                            format!("Deleted {} item(s); failed: {error}", outcome.paths.len());
                        self.reload()
                    }
                    None => {
                        self.status = if outcome.paths.len() == 1 {
                            format!("Deleted {}", display_name_for_path(&outcome.paths[0]))
                        } else {
                            format!("Deleted {} item(s)", outcome.paths.len())
                        };
                        self.reload()
                    }
                }
            }
            Message::ApplicationsLoaded(registry) => {
                self.app_registry = registry;
                Task::none()
            }
            Message::TemplateFilesLoaded(files) => {
                self.template_files = files;
                if self.template_files.is_empty() {
                    self.template_submenu_open = false;
                }
                Task::none()
            }
            Message::OpenWithSelect(app_id) => {
                if let Some(dialog) = &mut self.open_with_dialog {
                    dialog.selected_app_id = Some(app_id);
                }
                Task::none()
            }
            Message::OpenWithSetDefault(set_as_default) => {
                if let Some(dialog) = &mut self.open_with_dialog {
                    dialog.set_as_default = set_as_default;
                }
                Task::none()
            }
            Message::OpenWithCancel => {
                self.open_with_dialog = None;
                Task::none()
            }
            Message::OpenWithOpen => {
                let Some(dialog) = self.open_with_dialog.take() else {
                    return Task::none();
                };
                let Some(app_id) = dialog.selected_app_id else {
                    self.status = "No application selected".to_string();
                    return Task::none();
                };
                let Some(app) = dialog.apps.into_iter().find(|app| app.id == app_id) else {
                    self.status = "Selected application is not available".to_string();
                    return Task::none();
                };

                let default_mime = dialog.set_as_default.then_some(dialog.mime);
                self.status = if default_mime.is_some() {
                    format!("Setting {} as default and opening...", app.name)
                } else {
                    format!("Opening with {}...", app.name)
                };
                open_file_with_app_task(dialog.path, app, default_mime)
            }
            Message::OpenFileFinished(result) => {
                self.status = match result {
                    Ok(outcome) => {
                        if let Some(assignment) = outcome.default_assignment {
                            self.set_default_app_in_registry(assignment);
                            format!("Opened with {}; set as default", outcome.app_name)
                        } else if let Some(error) = outcome.default_error {
                            format!(
                                "Opened with {}; failed to set default: {error}",
                                outcome.app_name
                            )
                        } else {
                            format!("Opened with {}", outcome.app_name)
                        }
                    }
                    Err(error) => error,
                };
                Task::none()
            }
            Message::CreateFinished(kind, result) => match result {
                Ok(path) => {
                    let value = path
                        .file_name()
                        .map(|name| name.to_string_lossy().into_owned())
                        .unwrap_or_default();
                    let limits = path
                        .parent()
                        .and_then(|parent| child_path_limits(parent).ok())
                        .unwrap_or_default();
                    self.rename_state = Some(RenameState::new(path.clone(), value, limits));
                    self.status = match kind {
                        NewEntryKind::File => "File created; enter a new name".to_string(),
                        NewEntryKind::Folder => "Folder created; enter a new name".to_string(),
                    };
                    self.reload()
                }
                Err(error) => {
                    self.status = error.to_string();
                    Task::none()
                }
            },
            Message::TemplateCreateFinished(result) => match result {
                Ok(path) => {
                    let value = path
                        .file_name()
                        .map(|name| name.to_string_lossy().into_owned())
                        .unwrap_or_default();
                    let limits = path
                        .parent()
                        .and_then(|parent| child_path_limits(parent).ok())
                        .unwrap_or_default();
                    self.rename_state = Some(RenameState::new(path.clone(), value, limits));
                    self.status = "Template file created; enter a new name".to_string();
                    self.reload()
                }
                Err(error) => {
                    self.status = error.to_string();
                    Task::none()
                }
            },
            Message::RenameEditorAction(action) => {
                if matches!(action, text_editor::Action::Edit(text_editor::Edit::Enter)) {
                    return self.submit_rename();
                }

                if let Some(rename) = &mut self.rename_state {
                    let previous_content = rename.content.clone();
                    let previous_value = rename.value.clone();
                    rename.content.perform(action);
                    let value = rename.content.text();

                    if let Some(message) = rename_limit_error(rename, &value) {
                        rename.content = previous_content;
                        rename.value = previous_value;
                        self.status = message;
                        return self.focus_rename_input(false);
                    }

                    rename.value = value;
                }
                Task::none()
            }
            Message::RenameSubmit => self.submit_rename(),
            Message::RenameFinished(result) => match result {
                Ok(path) => {
                    if let Some(rename) = &self.rename_state {
                        if self.clipboard.as_ref().is_some_and(|clipboard| {
                            clipboard.paths.iter().any(|item| item == &rename.path)
                        }) {
                            self.clipboard = None;
                        }
                    }
                    self.rename_state = None;
                    self.selected_paths.clear();
                    self.selected_paths.insert(path.clone());
                    self.selection_anchor = Some(path);
                    self.status = "Renamed".to_string();
                    self.reload()
                }
                Err(error) => {
                    if error.kind() == std::io::ErrorKind::AlreadyExists {
                        self.status = "Name already exists; enter another name".to_string();
                    } else if error.is_name_too_long()
                        && self.rename_state.as_ref().is_some_and(|rename| {
                            rename.limits.name_bytes.is_none() || rename.limits.path_bytes.is_none()
                        })
                    {
                        let Some(rename) = self.rename_state.take() else {
                            return Task::none();
                        };
                        self.selected_paths.clear();
                        self.selected_paths.insert(rename.path.clone());
                        self.selection_anchor = Some(rename.path);
                        self.status = format!(
                            "Name is too long; kept default name {}",
                            rename.fallback_name
                        );
                        return Task::none();
                    } else {
                        self.status = error.to_string();
                    }
                    self.prepare_rename_retry()
                }
            },
            Message::PasteClipboardRead(contents) => {
                let Some(contents) = contents else {
                    self.status = "Clipboard does not contain local paths".to_string();
                    return Task::none();
                };

                let ClipboardPaths { action, paths } = parse_clipboard_paths(&contents);

                if paths.is_empty() {
                    self.status = "Clipboard does not contain local paths".to_string();
                    return Task::none();
                }

                self.paste_paths(paths, action)
            }
            Message::FileOperationEvent(operation_id, event) => {
                self.file_operation_event(operation_id, event)
            }
            Message::ToggleFileOperations => {
                self.file_operations_expanded = !self.file_operations_expanded;
                Task::none()
            }
            Message::HideFileOperations => {
                self.file_operations_expanded = false;
                Task::none()
            }
            Message::FileOperationsEventSink => Task::none(),
            Message::CloseFileOperation(operation_id) => self.close_file_operation(operation_id),
            Message::RemoveCompletedFileOperation(operation_id) => {
                if self
                    .file_operations
                    .get(&operation_id)
                    .is_some_and(|operation| operation.status == FileOperationStatus::Completed)
                {
                    self.file_operations.remove(&operation_id);
                }
                if self.file_operations.is_empty() {
                    self.file_operations_expanded = false;
                }
                Task::none()
            }
            Message::TerminalOpened(result) => {
                self.status = match result {
                    Ok(name) => format!("Opened {name}"),
                    Err(error) => error,
                };
                Task::none()
            }
            Message::CustomCommandStarted(result) => {
                self.status = match result {
                    Ok(label) => format!("Started {label}"),
                    Err(error) => error,
                };
                Task::none()
            }
            Message::PropertiesLoaded(result) => {
                if let Some(dialog) = &mut self.properties_dialog {
                    dialog.state = match result {
                        Ok(properties) => {
                            dialog.permissions_mode = properties.mode;
                            dialog.permission_error = None;
                            PropertiesState::Loaded(properties)
                        }
                        Err(error) => PropertiesState::Error(error.to_string()),
                    };
                }
                Task::none()
            }
            Message::FilePropertiesLoaded(result) => {
                if let Some(dialog) = &mut self.properties_dialog {
                    dialog.state = match result {
                        Ok(properties) => {
                            dialog.permissions_mode = properties.mode;
                            dialog.permission_error = None;
                            PropertiesState::FileLoaded(properties)
                        }
                        Err(error) => PropertiesState::Error(error.to_string()),
                    };
                }
                Task::none()
            }
            Message::PropertiesPermissionsSaved(result) => {
                if let Some(dialog) = &mut self.properties_dialog {
                    dialog.saving_permissions = false;
                    match result {
                        Ok(properties) => {
                            dialog.permissions_mode = properties.mode;
                            dialog.permission_error = None;
                            dialog.state = PropertiesState::Loaded(properties);
                            self.status = "Permissions changed".to_string();
                        }
                        Err(error) => {
                            dialog.permission_error = Some(error.to_string());
                            self.status = "Failed to change permissions".to_string();
                        }
                    }
                }
                Task::none()
            }
            Message::CloseProperties => {
                self.properties_dialog = None;
                self.properties_drag = None;
                Task::none()
            }
            Message::SetPropertiesView(view) => {
                if let Some(dialog) = &mut self.properties_dialog {
                    dialog.view = view;
                    if view == PropertiesView::Permissions {
                        if let PropertiesState::Loaded(properties) = &dialog.state {
                            dialog.permissions_mode = properties.mode;
                        }
                    }
                }
                Task::none()
            }
            Message::PropertiesPointerMoved(position) => {
                self.properties_pointer = Some(position);
                if let Some(drag) = self.properties_drag {
                    let x = drag.dialog_origin.x + position.x - drag.pointer_origin.x;
                    let y = drag.dialog_origin.y + position.y - drag.pointer_origin.y;
                    self.set_properties_position(Point::new(x, y));
                }
                Task::none()
            }
            Message::PropertiesDragStarted => {
                if let (Some(pointer), Some(dialog)) =
                    (self.properties_pointer, self.properties_dialog.as_ref())
                {
                    self.properties_drag = Some(PropertiesDrag {
                        pointer_origin: pointer,
                        dialog_origin: dialog.position,
                    });
                }
                Task::none()
            }
            Message::PropertiesDragEnded => {
                self.properties_drag = None;
                Task::none()
            }
            Message::PropertiesEventSink => Task::none(),
            Message::CloseAlert => {
                self.alert_dialog = None;
                Task::none()
            }
            Message::CyclePermission(class) => {
                self.cycle_permission(class);
                Task::none()
            }
            Message::CancelPermissions => {
                self.cancel_permissions();
                Task::none()
            }
            Message::ApplyPermissions => self.apply_permissions(),
            Message::ToggleHidden(show_hidden) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.show_hidden = show_hidden;
                if self.search_query.is_some() {
                    self.rerun_search()
                } else {
                    self.reload()
                }
            }
            Message::ToggleMenu => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.context_menu = None;
                self.menu_open = !self.menu_open;
                if !self.menu_open {
                    self.view_submenu_open = false;
                }
                Task::none()
            }
            Message::ToggleViewSubmenu => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.view_submenu_open = !self.view_submenu_open;
                Task::none()
            }
            Message::SetViewMode(view_mode) => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.view_mode = view_mode;
                self.close_menu();
                Task::none()
            }
            Message::DirectoryEvent(request, event) => self.directory_event(request, event),
            Message::EntriesDecorated(request_id, decorations) => {
                self.entries_decorated(request_id, decorations);
                Task::none()
            }
            Message::SearchFinished(request, result) => self.search_finished(request, result),
            Message::CurrentFolderChanged(change) => self.current_folder_changed(change),
            Message::AutoRefreshEntryLoaded(request_id, path, result) => {
                self.auto_refresh_entry_loaded(request_id, path, result)
            }
            Message::AutoRefreshSnapshotLoaded(request, result) => {
                self.auto_refresh_snapshot_loaded(request, result)
            }
            Message::AutoRefreshEntriesDecorated(request_id, decorations) => {
                self.auto_refresh_entries_decorated(request_id, decorations);
                Task::none()
            }
            Message::CurrentFolderWatchFailed(path, error) => {
                if path == self.cwd {
                    self.status = format!("Auto-refresh unavailable: {error}");
                }
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

    pub(crate) fn subscription(&self) -> Subscription<Message> {
        Subscription::batch([
            window::resize_events().map(|(_id, size)| Message::WindowResized(size)),
            event::listen_with(keyboard_shortcut_event),
            event::listen_with(selection_modifier_event),
            watch_current_folder(self.cwd.clone()),
        ])
    }

    pub(crate) fn view(&self) -> Element<'_, Message> {
        let shell = row![self.sidebar(), self.main_area()]
            .height(Fill)
            .width(Fill);

        let shell = container(shell)
            .height(Fill)
            .width(Fill)
            .style(style::app_background);

        stack([
            shell.into(),
            resize_layer(),
            self.file_operations_overlay(),
            self.properties_overlay(),
            self.delete_confirm_overlay(),
            self.open_with_overlay(),
            self.alert_overlay(),
        ])
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn sidebar(&self) -> Element<'_, Message> {
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

        let sidebar = column![
            self.sidebar_header(),
            container(content).height(Fill).width(Fill),
        ]
        .spacing(0)
        .height(Fill)
        .width(Fill);

        container(sidebar)
            .width(SIDEBAR_WIDTH)
            .height(Fill)
            .style(style::sidebar)
            .into()
    }

    fn sidebar_header(&self) -> Element<'_, Message> {
        let title = || {
            mouse_area(
                container(
                    text(&self.runtime_config.name)
                        .size(16)
                        .style(style::primary_text),
                )
                .height(Fill)
                .width(Fill)
                .padding([0, 14])
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center),
            )
            .on_press(Message::WindowDrag)
            .on_double_click(Message::WindowToggleMaximize)
        };

        let header = if let Some((progress, complete)) = self.overall_file_operation_progress() {
            row![
                space().width(46),
                title(),
                button(operation_progress_circle(progress, complete))
                    .height(38)
                    .width(38)
                    .padding(4)
                    .style(style::progress_circle_button)
                    .on_press(Message::ToggleFileOperations),
                space().width(8),
            ]
            .height(TOOLBAR_HEIGHT)
            .align_y(iced::Center)
        } else {
            row![title()].height(TOOLBAR_HEIGHT).align_y(iced::Center)
        };

        container(header)
            .height(TOOLBAR_HEIGHT)
            .width(Fill)
            .padding(Padding::default().right(8.0))
            .style(style::sidebar_drag_area)
            .into()
    }

    fn file_operations_overlay(&self) -> Element<'_, Message> {
        if !self.file_operations_expanded || self.file_operations.is_empty() {
            return container(space()).height(Fill).width(Fill).into();
        }

        let dismiss_layer = mouse_area(container(space()).height(Fill).width(Fill))
            .on_press(Message::HideFileOperations);

        let panel = mouse_area(self.file_operations_panel())
            .on_press(Message::FileOperationsEventSink)
            .on_scroll(|_| Message::FileOperationsEventSink);

        let panel_layer = container(
            column![
                space().height(Length::Fixed(TOOLBAR_HEIGHT)),
                row![panel, space().width(Fill)].spacing(0),
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill);

        stack([dismiss_layer.into(), panel_layer.into()])
            .height(Fill)
            .width(Fill)
            .into()
    }

    fn file_operations_panel(&self) -> Element<'_, Message> {
        let operation_count = self.file_operations.len();
        let mut rows = column![].spacing(8).padding([8, 10]);

        for operation in self.file_operations.values() {
            rows = rows.push(self.file_operation_row(operation));
        }

        let visible_rows = operation_count.min(5);
        let height = (visible_rows as f32 * 106.0 + 16.0).max(1.0);
        let width = Length::Fixed(FILE_OPERATION_POPUP_WIDTH);

        let content: Element<'_, Message> = if operation_count > 5 {
            scrollable(rows)
                .height(Length::Fixed(height))
                .width(width)
                .into()
        } else {
            rows.width(width).into()
        };

        container(content)
            .height(Length::Fixed(height))
            .width(width)
            .style(style::file_operation_panel)
            .into()
    }

    fn file_operation_row(&self, operation: &FileOperationState) -> Element<'_, Message> {
        let fraction = file_operation_fraction(operation);
        let title = format!(
            "{} {} 项",
            paste_action_label(operation.action),
            operation.sources.len()
        );
        let destination = format!(
            "到 {}",
            short_list_text(&operation.destination.display().to_string())
        );
        let current = file_operation_current_label(operation);
        let speed = file_operation_speed_label(operation);
        let percent = format!("{:.0}%", fraction * 100.0);

        let close = button(
            container(text("x").size(13).style(style::muted_text))
                .height(Fill)
                .width(Fill)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center),
        )
        .height(24)
        .width(24)
        .padding(0)
        .style(style::toolbar_button)
        .on_press(Message::CloseFileOperation(operation.id));

        container(
            column![
                row![
                    column![
                        text(title).size(13).style(style::primary_text),
                        text(destination).size(11).style(style::muted_text),
                    ]
                    .spacing(2)
                    .width(Fill),
                    close,
                ]
                .align_y(iced::Center),
                text(current).size(12).style(style::muted_text),
                progress_bar(0.0..=1.0, fraction)
                    .girth(Length::Fixed(6.0))
                    .style(style::file_operation_progress),
                row![
                    text(speed).size(11).style(style::muted_text),
                    space().width(Fill),
                    text(percent).size(11).style(style::muted_text),
                ]
                .align_y(iced::Center),
            ]
            .spacing(7)
            .padding(8),
        )
        .width(Fill)
        .style(style::file_operation_row)
        .into()
    }

    fn overall_file_operation_progress(&self) -> Option<(f32, bool)> {
        if self.file_operations.is_empty() {
            return None;
        }

        let mut total_bytes = 0;
        let mut completed_bytes = 0;
        let mut total_items = 0;
        let mut completed_items = 0;
        let mut all_completed = true;

        for operation in self.file_operations.values() {
            total_bytes += operation.progress.total_bytes;
            completed_bytes += operation.progress.completed_bytes;
            total_items += operation.progress.total_items;
            completed_items += operation.progress.completed_items;
            all_completed &= operation.status == FileOperationStatus::Completed;
        }

        let progress = if total_bytes > 0 {
            completed_bytes as f32 / total_bytes as f32
        } else if total_items > 0 {
            completed_items as f32 / total_items as f32
        } else if all_completed {
            1.0
        } else {
            0.0
        };

        Some((progress.clamp(0.0, 1.0), all_completed))
    }

    fn main_area(&self) -> Element<'_, Message> {
        let content = column![self.toolbar(), self.browser_view()]
            .height(Fill)
            .width(Fill)
            .spacing(0);

        let content = container(content)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into();

        if self.menu_open {
            stack([content, self.toolbar_menu()])
                .height(Fill)
                .width(Fill)
                .into()
        } else {
            content
        }
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

        let menu = button(toolbar_icon(
            include_bytes!("../../../icons/menu.svg"),
            true,
        ))
        .height(36)
        .width(36)
        .padding(6)
        .style(style::toolbar_button)
        .on_press(Message::ToggleMenu);

        let drag_region = mouse_area(
            container(space())
                .height(38)
                .width(70)
                .style(style::toolbar_drag_area),
        )
        .on_press(Message::WindowDrag)
        .on_double_click(Message::WindowToggleMaximize);

        let toolbar = row![
            back,
            forward,
            path_bar,
            drag_region,
            menu,
            space().width(8),
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

    fn toolbar_menu(&self) -> Element<'_, Message> {
        let view_item = button(
            row![
                text("视图").size(14).style(style::primary_text),
                space().width(Fill),
                text("›").size(17).style(style::muted_text),
            ]
            .align_y(iced::Center)
            .width(Fill),
        )
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, self.view_submenu_open))
        .on_press(Message::ToggleViewSubmenu);

        let hidden_item = container(
            checkbox(self.show_hidden)
                .label("显示隐藏文件")
                .on_toggle(Message::ToggleHidden)
                .size(16)
                .text_size(13)
                .style(style::checkbox),
        )
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .align_y(Vertical::Center);

        let mut menu = row![
            container(column![view_item, hidden_item].spacing(4).padding(6))
                .width(178)
                .style(style::menu_panel)
        ]
        .spacing(6)
        .align_y(iced::Alignment::Start);

        if self.view_submenu_open {
            menu = menu.push(
                container(
                    column![
                        self.view_option(ViewMode::Icons),
                        self.view_option(ViewMode::List),
                    ]
                    .spacing(4)
                    .padding(6),
                )
                .width(160)
                .style(style::menu_panel),
            );
        }

        container(row![space().width(Fill), menu])
            .height(Fill)
            .width(Fill)
            .padding(
                Padding::default()
                    .top(TOOLBAR_HEIGHT)
                    .right(168.0)
                    .left(12.0),
            )
            .into()
    }

    fn view_option(&self, view_mode: ViewMode) -> Element<'_, Message> {
        let active = self.view_mode == view_mode;
        let marker = if active { "✓" } else { "" };

        button(
            row![
                container(text(marker).size(13).style(style::primary_text))
                    .width(20)
                    .align_x(Horizontal::Center),
                text(view_mode.label()).size(14).style(style::primary_text),
            ]
            .spacing(8)
            .align_y(iced::Center)
            .width(Fill),
        )
        .height(32)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, active))
        .on_press(Message::SetViewMode(view_mode))
        .into()
    }

    fn browser_view(&self) -> Element<'_, Message> {
        let generation = self.browser_generation;
        let content = match self.view_mode {
            ViewMode::Icons => self.grid(),
            ViewMode::List => self.list(),
        };

        let content = mouse_area(content)
            .on_move(move |position| Message::BrowserPointerMoved(generation, position))
            .on_press(Message::BrowserPressed(generation))
            .on_release(Message::BrowserReleased(generation))
            .on_right_press(Message::BrowserRightPressed(generation));

        container(
            stack([
                content.into(),
                self.selection_overlay(),
                self.rename_overlay(),
                self.context_menu_overlay(),
            ])
            .height(Fill)
            .width(Fill),
        )
        .id(self.browser_content_id())
        .into()
    }

    fn grid(&self) -> Element<'_, Message> {
        let generation = self.browser_generation;
        let columns = self.icon_grid_columns();
        let mut rows = column![]
            .spacing(0)
            .padding([GRID_PADDING_TOP as u16, GRID_PADDING_LEFT as u16]);

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
            let total_rows = self.entries.len().div_ceil(columns);
            let (start_row, end_row) = self.visible_grid_rows(total_rows);
            let row_stride = TILE_HEIGHT + GRID_ROW_SPACING;
            let top_spacer = start_row as f32 * row_stride;
            let bottom_spacer = total_rows.saturating_sub(end_row) as f32 * row_stride;

            if top_spacer > 0.0 {
                rows = rows.push(space().height(Length::Fixed(top_spacer)));
            }

            for row_index in start_row..end_row {
                let start = row_index * columns;
                let end = ((row_index + 1) * columns).min(self.entries.len());
                let mut row = row![].spacing(GRID_COLUMN_SPACING).height(TILE_HEIGHT);

                for entry in &self.entries[start..end] {
                    row = row.push(self.tile(entry));
                }

                rows = rows.push(row);

                if row_index + 1 < end_row {
                    rows = rows.push(space().height(Length::Fixed(GRID_ROW_SPACING)));
                }
            }

            if bottom_spacer > 0.0 {
                rows = rows.push(space().height(Length::Fixed(bottom_spacer)));
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

        let view = column![
            scrollable(rows)
                .id(self.browser_scroll_id())
                .on_scroll(move |viewport| {
                    Message::BrowserScrolled(generation, viewport.absolute_offset().y)
                })
                .height(Fill)
                .width(Fill),
            footer
        ]
        .height(Fill)
        .width(Fill);

        container(view)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into()
    }

    fn list(&self) -> Element<'_, Message> {
        let generation = self.browser_generation;
        let mut rows = column![self.list_header()].spacing(0).padding(
            Padding::default()
                .top(LIST_PADDING_TOP)
                .right(LIST_PADDING_LEFT)
                .left(LIST_PADDING_LEFT),
        );

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
            let (start, end) = self.visible_list_range(self.entries.len());
            let top_spacer = start as f32 * LIST_ROW_HEIGHT;
            let bottom_spacer = self.entries.len().saturating_sub(end) as f32 * LIST_ROW_HEIGHT;

            if top_spacer > 0.0 {
                rows = rows.push(space().height(Length::Fixed(top_spacer)));
            }

            for entry in &self.entries[start..end] {
                rows = rows.push(self.list_row(entry));
            }

            if bottom_spacer > 0.0 {
                rows = rows.push(space().height(Length::Fixed(bottom_spacer)));
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

        let view = column![
            scrollable(rows)
                .id(self.browser_scroll_id())
                .on_scroll(move |viewport| {
                    Message::BrowserScrolled(generation, viewport.absolute_offset().y)
                })
                .height(Fill)
                .width(Fill),
            footer
        ]
        .height(Fill)
        .width(Fill);

        container(view)
            .height(Fill)
            .width(Fill)
            .style(style::content_background)
            .into()
    }

    fn list_header(&self) -> Element<'_, Message> {
        container(
            row![
                list_header_cell("名称", Length::FillPortion(5)),
                list_header_cell("大小", Length::FillPortion(2)),
                list_header_cell("所有者", Length::FillPortion(2)),
                list_header_cell("修改时间", Length::FillPortion(3)),
            ]
            .spacing(12)
            .align_y(iced::Center)
            .height(LIST_HEADER_HEIGHT),
        )
        .width(Fill)
        .padding([0, 14])
        .style(style::list_header)
        .into()
    }

    fn list_row(&self, entry: &DisplayEntry) -> Element<'_, Message> {
        let selected = self.is_selected(entry);
        let cut = self.is_cut_path(&entry.file.path);
        let size = entry_size(&entry.file);
        let owner = entry_owner(&entry.file);
        let modified = format_modified(entry.file.modified);
        let display_name = self.entry_display_name(entry);
        let short_display_name = short_list_text(&display_name);
        let name_cell = list_name_cell(
            entry,
            short_display_name.clone(),
            Length::FillPortion(5),
            cut,
        );

        let content = row![
            name_cell,
            list_value_cell(size, Length::FillPortion(2), false, cut),
            list_value_cell(owner, Length::FillPortion(2), false, cut),
            list_value_cell(modified, Length::FillPortion(3), false, cut),
        ]
        .spacing(12)
        .align_y(iced::Center)
        .height(LIST_ROW_HEIGHT);

        let row = container(content)
            .height(LIST_ROW_HEIGHT)
            .width(Fill)
            .padding([0, 14])
            .style(move |_| style::list_row_container(selected, cut));

        let item = mouse_area(row)
            .interaction(mouse::Interaction::Pointer)
            .on_press(Message::SelectEntry(entry.file.path.clone()))
            .on_double_click(Message::Open(entry.file.path.clone(), entry.file.kind));

        entry_name_tooltip(item, display_name, short_display_name)
    }

    fn tile(&self, entry: &DisplayEntry) -> Element<'_, Message> {
        let selected = self.is_selected(entry);
        let cut = self.is_cut_path(&entry.file.path);
        let display_name = entry.file.name.clone();
        let short_display_name = short_name(&display_name);
        let name: Element<'_, Message> = text(short_display_name.clone())
            .size(15)
            .width(TILE_WIDTH)
            .height(Length::Fixed(38.0))
            .align_x(Horizontal::Center)
            .style(if cut {
                style::disabled_text
            } else {
                style::primary_text
            })
            .into();

        let meta = text(self.entry_subtitle(entry))
            .size(12)
            .width(TILE_WIDTH)
            .align_x(Horizontal::Center)
            .style(if cut {
                style::disabled_text
            } else {
                style::muted_text
            });

        let content = column![badged_entry_icon(entry, 78.0), name, meta]
            .spacing(8)
            .align_x(iced::Center)
            .width(TILE_WIDTH);

        let tile = container(content)
            .height(TILE_HEIGHT)
            .width(TILE_WIDTH)
            .padding(8)
            .style(move |_| style::tile_container(selected, cut));

        let item = mouse_area(tile)
            .interaction(mouse::Interaction::Pointer)
            .on_press(Message::SelectEntry(entry.file.path.clone()))
            .on_double_click(Message::Open(entry.file.path.clone(), entry.file.kind));

        entry_name_tooltip(item, display_name, short_display_name)
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
        if let Some(entry) = self.entry_for_path(&path) {
            if Self::entry_is_broken_symlink(entry) {
                return self.show_broken_symlink_alert(&path);
            }

            let target_path = Self::entry_open_path(entry);
            let effective_kind = Self::entry_effective_kind(entry);
            let mime = entry.mime.clone();

            return match effective_kind {
                EntryKind::Directory => self.visit_path(target_path),
                _ => self.open_file_with_default_for_mime(target_path, mime),
            };
        }

        if matches!(kind, EntryKind::Directory) {
            self.visit_path(path)
        } else {
            self.open_file_with_default(path)
        }
    }

    fn open_file_with_default(&mut self, path: PathBuf) -> Task<Message> {
        if self.is_broken_symlink_path(&path) {
            return self.show_broken_symlink_alert(&path);
        }

        let mime = self.mime_info_for_path(&path);
        self.open_file_with_default_for_mime(path, mime)
    }

    fn open_file_with_default_for_mime(&mut self, path: PathBuf, mime: MimeInfo) -> Task<Message> {
        let mime = mime.mime;
        if let Some(app) = best_app_for_mime(&self.app_registry, &mime) {
            self.status = format!("Opening with {}...", app.name);
            open_file_with_app_task(path, app, None)
        } else {
            self.status = "No application found for this file".to_string();
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
            async move { load_search(root, query, options) },
            move |result| Message::SearchFinished(request.clone(), result),
        )
    }

    fn rerun_search(&mut self) -> Task<Message> {
        let Some(query) = self.search_query.clone() else {
            return Task::none();
        };

        self.search_current_dir(query)
    }

    fn current_folder_changed(&mut self, change: CurrentFolderChange) -> Task<Message> {
        if change.path != self.cwd {
            return Task::none();
        }

        if self.search_query.is_some() {
            return self.rerun_search();
        }

        match change.kind {
            CurrentFolderChangeKind::Rescan | CurrentFolderChangeKind::Structure => {
                self.start_auto_refresh_snapshot()
            }
            CurrentFolderChangeKind::Entry => {
                let cwd = self.cwd.clone();
                let mut tasks = Vec::new();
                for path in change.paths {
                    if path.parent() == Some(cwd.as_path()) {
                        let request_id = self.next_request_id();
                        tasks.push(refresh_entry_task(request_id, path));
                    }
                }

                Task::batch(tasks)
            }
        }
    }

    fn auto_refresh_entry_loaded(
        &mut self,
        _request_id: u64,
        path: PathBuf,
        result: Result<DisplayEntry, FsError>,
    ) -> Task<Message> {
        if path.parent() != Some(self.cwd.as_path()) {
            return Task::none();
        }

        match result {
            Ok(entry) => {
                self.upsert_auto_refresh_entry(entry);
                Task::none()
            }
            Err(error) if error.kind() == io::ErrorKind::NotFound => {
                self.start_auto_refresh_snapshot()
            }
            Err(_) => Task::none(),
        }
    }

    fn start_auto_refresh_snapshot(&mut self) -> Task<Message> {
        if self.auto_refresh_snapshot_id.is_some() {
            self.auto_refresh_snapshot_dirty = true;
            return Task::none();
        }

        let id = self.next_request_id();
        self.auto_refresh_snapshot_id = Some(id);
        let request = AutoRefreshRequest {
            id,
            path: self.cwd.clone(),
        };
        let options = ScanOptions {
            show_hidden: self.show_hidden,
        };

        load_auto_refresh_snapshot_task(request, options)
    }

    fn auto_refresh_snapshot_loaded(
        &mut self,
        request: AutoRefreshRequest,
        result: Result<Vec<DisplayEntry>, FsError>,
    ) -> Task<Message> {
        if self.auto_refresh_snapshot_id != Some(request.id) || request.path != self.cwd {
            return Task::none();
        }

        self.auto_refresh_snapshot_id = None;

        let mut tasks = Vec::new();
        match result {
            Ok(entries) => {
                let decorate_entries = self.apply_auto_refresh_snapshot(entries);
                if !decorate_entries.is_empty() {
                    tasks.push(decorate_auto_refresh_entries_task(
                        request.id,
                        decorate_entries,
                    ));
                }
            }
            Err(error) => {
                self.set_scan_error(error);
            }
        }

        if self.auto_refresh_snapshot_dirty {
            self.auto_refresh_snapshot_dirty = false;
            tasks.push(self.start_auto_refresh_snapshot());
        }

        Task::batch(tasks)
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

        self.clear_for_new_directory(&request);
        self.active_request_id = Some(id);
        self.status = format!("Loading {}...", path.display());

        load_directory_stream(request, path, options)
    }

    fn directory_event(
        &mut self,
        request: DirectoryRequest,
        event: DirectoryLoadEvent,
    ) -> Task<Message> {
        if self.active_request_id != Some(request.id) {
            return Task::none();
        }

        match event {
            DirectoryLoadEvent::Started => {
                self.directory_started(request);
                self.reset_browser_scroll()
            }
            DirectoryLoadEvent::Batch(entries) => {
                let files = entries
                    .iter()
                    .map(|entry| entry.file.clone())
                    .collect::<Vec<_>>();

                self.entries.extend(entries);
                self.sort_entries();
                self.status = format!("Loading... {} entries", self.entries.len());

                Task::batch([
                    decorate_entries_task(request.id, files),
                    self.focus_rename_input(true),
                ])
            }
            DirectoryLoadEvent::Finished { total } => {
                self.sort_entries();
                self.status = format!("{total} entries");
                self.focus_rename_input(true)
            }
            DirectoryLoadEvent::Failed(error) => {
                self.active_request_id = None;
                self.clear_selection_state();
                if self.cwd == request.path {
                    self.entries.clear();
                }
                self.set_scan_error(error);
                Task::none()
            }
        }
    }

    fn directory_started(&mut self, request: DirectoryRequest) {
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
    }

    fn clear_for_new_directory(&mut self, request: &DirectoryRequest) {
        self.cwd = request.path.clone();
        self.entries.clear();
        self.path_input = self.cwd.display().to_string();
        self.menu_open = false;
        self.view_submenu_open = false;
        self.template_submenu_open = false;
        self.context_menu = None;
        self.open_with_dialog = None;
        self.properties_dialog = None;
        self.properties_drag = None;
        self.properties_pointer = None;
        self.delete_confirm = None;
        self.alert_dialog = None;
        self.search_query = None;
        self.search_root = None;
        self.auto_refresh_snapshot_id = None;
        self.auto_refresh_snapshot_dirty = false;
        self.clear_selection_state();
        self.browser_scroll_y = 0.0;
        self.browser_pointer = None;
        self.browser_generation = self.browser_generation.wrapping_add(1);
        self.browser_scroll_id = iced::widget::Id::unique();
        self.browser_content_id = iced::widget::Id::unique();
        self.status = match request.mode {
            DirectoryLoadMode::Replace => "Reloading entries...".to_string(),
            DirectoryLoadMode::Visit => "Loading entries...".to_string(),
            DirectoryLoadMode::Back => "Loading previous path...".to_string(),
            DirectoryLoadMode::Forward => "Loading next path...".to_string(),
        };
    }

    fn search_finished(
        &mut self,
        request: SearchRequest,
        result: Result<DisplaySearchResults, FsError>,
    ) -> Task<Message> {
        if self.active_request_id != Some(request.id) {
            return Task::none();
        }

        match result {
            Ok(results) => {
                let count = results.entries.len();
                let files = results
                    .entries
                    .iter()
                    .map(|entry| entry.file.clone())
                    .collect::<Vec<_>>();
                self.entries = results.entries;
                self.search_query = Some(results.query);
                self.search_root = Some(results.root);
                self.clear_selection_state();
                self.status = format!("{count} name matches");
                decorate_entries_task(request.id, files)
            }
            Err(error) => {
                self.active_request_id = None;
                self.clear_selection_state();
                self.set_scan_error(error);
                Task::none()
            }
        }
    }

    fn entries_decorated(&mut self, request_id: u64, decorations: Vec<EntryDecoration>) {
        if self.active_request_id != Some(request_id) {
            return;
        }

        let mut decorations = decorations
            .into_iter()
            .map(|decoration| (decoration.path.clone(), decoration))
            .collect::<BTreeMap<_, _>>();

        for entry in &mut self.entries {
            if let Some(decoration) = decorations.remove(&entry.file.path) {
                entry.mime = decoration.mime;
                entry.icon = decoration.icon;
                entry.badge = decoration.badge;
            }
        }
    }

    fn auto_refresh_entries_decorated(
        &mut self,
        _request_id: u64,
        decorations: Vec<EntryDecoration>,
    ) {
        let mut decorations = decorations
            .into_iter()
            .map(|decoration| (decoration.path.clone(), decoration))
            .collect::<BTreeMap<_, _>>();

        for entry in &mut self.entries {
            if let Some(decoration) = decorations.remove(&entry.file.path) {
                entry.mime = decoration.mime;
                entry.icon = decoration.icon;
                entry.badge = decoration.badge;
            }
        }
    }

    fn upsert_auto_refresh_entry(&mut self, entry: DisplayEntry) {
        if entry.file.hidden && !self.show_hidden {
            self.remove_entry_path(&entry.file.path);
            return;
        }

        match self.entry_index_for_path(&entry.file.path) {
            Some(index) => {
                self.entries[index] = entry;
            }
            None => {
                self.entries.push(entry);
            }
        }

        self.sort_entries();
    }

    fn remove_entry_path(&mut self, path: &PathBuf) {
        self.entries.retain(|entry| entry.file.path != *path);
        self.selected_paths.remove(path);
        if self.selection_anchor.as_ref() == Some(path) {
            self.selection_anchor = None;
        }
    }

    fn apply_auto_refresh_snapshot(&mut self, entries: Vec<DisplayEntry>) -> Vec<FileEntry> {
        let previous_visible_paths = self.visible_entry_paths();
        let old_entries = self
            .entries
            .iter()
            .map(|entry| (entry.file.path.clone(), entry.clone()))
            .collect::<BTreeMap<_, _>>();
        let mut needs_decoration = BTreeSet::new();

        self.entries = entries
            .into_iter()
            .map(|mut entry| {
                if let Some(old_entry) = old_entries.get(&entry.file.path) {
                    if !same_entry_decoration_key(&old_entry.file, &entry.file) {
                        needs_decoration.insert(entry.file.path.clone());
                    }
                    entry.mime = old_entry.mime.clone();
                    entry.icon = old_entry.icon.clone();
                    entry.badge = old_entry.badge;
                } else {
                    needs_decoration.insert(entry.file.path.clone());
                }
                entry
            })
            .collect();
        self.sort_entries();
        self.retain_existing_selection();
        self.status = format!("{} entries", self.entries.len());

        if self.visible_entry_paths() != previous_visible_paths {
            self.visible_files()
        } else {
            self.visible_files_needing_decoration(&needs_decoration)
        }
    }

    fn retain_existing_selection(&mut self) {
        let paths = self
            .entries
            .iter()
            .map(|entry| entry.file.path.clone())
            .collect::<BTreeSet<_>>();
        self.selected_paths.retain(|path| paths.contains(path));
        if self
            .selection_anchor
            .as_ref()
            .is_some_and(|path| !paths.contains(path))
        {
            self.selection_anchor = None;
        }
    }

    fn visible_files_needing_decoration(&self, paths: &BTreeSet<PathBuf>) -> Vec<FileEntry> {
        let (start, end) = self.visible_entry_range();

        self.entries
            .iter()
            .enumerate()
            .filter(|(index, entry)| {
                *index >= start && *index < end && paths.contains(&entry.file.path)
            })
            .map(|(_, entry)| entry.file.clone())
            .collect()
    }

    fn visible_files(&self) -> Vec<FileEntry> {
        let (start, end) = self.visible_entry_range();

        self.entries[start..end]
            .iter()
            .map(|entry| entry.file.clone())
            .collect()
    }

    fn visible_entry_paths(&self) -> Vec<PathBuf> {
        let (start, end) = self.visible_entry_range();

        self.entries[start..end]
            .iter()
            .map(|entry| entry.file.path.clone())
            .collect()
    }

    fn visible_entry_range(&self) -> (usize, usize) {
        match self.view_mode {
            ViewMode::Icons => {
                let columns = self.icon_grid_columns();
                let total_rows = self.entries.len().div_ceil(columns);
                let (start_row, end_row) = self.visible_grid_rows(total_rows);
                (
                    (start_row * columns).min(self.entries.len()),
                    (end_row * columns).min(self.entries.len()),
                )
            }
            ViewMode::List => self.visible_list_range(self.entries.len()),
        }
    }

    fn sort_entries(&mut self) {
        self.entries.sort_by(compare_display_entries);
    }

    fn set_scan_error(&mut self, error: FsError) {
        self.status = error.to_string();
    }

    fn next_request_id(&mut self) -> u64 {
        self.next_request_id += 1;
        self.next_request_id
    }

    fn entry_subtitle(&self, entry: &DisplayEntry) -> String {
        let Some(root) = &self.search_root else {
            return entry_meta(&entry.file);
        };

        entry
            .file
            .path
            .strip_prefix(root)
            .map(|path| path.display().to_string())
            .unwrap_or_else(|_| entry.file.path.display().to_string())
    }

    fn entry_display_name(&self, entry: &DisplayEntry) -> String {
        let Some(root) = &self.search_root else {
            return entry.file.name.clone();
        };

        entry
            .file
            .path
            .strip_prefix(root)
            .map(|path| path.display().to_string())
            .unwrap_or_else(|_| entry.file.name.clone())
    }

    fn is_selected(&self, entry: &DisplayEntry) -> bool {
        self.selected_paths.contains(&entry.file.path)
    }

    fn select_entry(&mut self, path: PathBuf) {
        self.selection_drag = None;

        if self.selection_modifiers.shift
            && self.select_entry_range(path.clone(), self.selection_modifiers.control)
        {
            return;
        }

        if self.selection_modifiers.control {
            if !self.selected_paths.remove(&path) {
                self.selected_paths.insert(path.clone());
            }
            self.selection_anchor = Some(path);
            return;
        }

        self.selected_paths.clear();
        self.selected_paths.insert(path.clone());
        self.selection_anchor = Some(path);
    }

    fn select_entry_range(&mut self, path: PathBuf, add_to_existing: bool) -> bool {
        let Some(anchor) = self.selection_anchor.as_ref() else {
            return false;
        };

        let Some(anchor_index) = self.entry_index_for_path(anchor) else {
            return false;
        };
        let Some(target_index) = self.entry_index_for_path(&path) else {
            return false;
        };

        if !add_to_existing {
            self.selected_paths.clear();
        }

        let (start, end) = if anchor_index <= target_index {
            (anchor_index, target_index)
        } else {
            (target_index, anchor_index)
        };

        for entry in &self.entries[start..=end] {
            self.selected_paths.insert(entry.file.path.clone());
        }

        true
    }

    fn entry_index_for_path(&self, path: &PathBuf) -> Option<usize> {
        self.entries
            .iter()
            .position(|entry| entry.file.path == *path)
    }

    fn clear_selection_state(&mut self) {
        self.selected_paths.clear();
        self.selection_anchor = None;
        self.selection_drag = None;
        self.browser_pointer = None;
        self.browser_scroll_y = 0.0;
    }

    fn icon_grid_columns(&self) -> usize {
        icon_grid_columns(self.browser_width)
    }

    fn browser_viewport_height(&self) -> f32 {
        (self.window_size.height - TOOLBAR_HEIGHT - 34.0).max(0.0)
    }

    fn visible_grid_rows(&self, total_rows: usize) -> (usize, usize) {
        if total_rows == 0 {
            return (0, 0);
        }

        let row_stride = TILE_HEIGHT + GRID_ROW_SPACING;
        let viewport_top = (self.browser_scroll_y - GRID_PADDING_TOP).max(0.0);
        let viewport_bottom =
            (self.browser_scroll_y + self.browser_viewport_height() - GRID_PADDING_TOP).max(0.0);
        let first = (viewport_top / row_stride).floor() as usize;
        let last = (viewport_bottom / row_stride).ceil() as usize + 1;
        let start = first
            .min(total_rows.saturating_sub(1))
            .saturating_sub(VIRTUAL_ROW_BUFFER);
        let end = (last + VIRTUAL_ROW_BUFFER).min(total_rows).max(start + 1);

        (start, end)
    }

    fn visible_list_range(&self, total_entries: usize) -> (usize, usize) {
        if total_entries == 0 {
            return (0, 0);
        }

        let entries_top = LIST_PADDING_TOP + LIST_HEADER_HEIGHT;
        let viewport_top = (self.browser_scroll_y - entries_top).max(0.0);
        let viewport_bottom =
            (self.browser_scroll_y + self.browser_viewport_height() - entries_top).max(0.0);
        let first = (viewport_top / LIST_ROW_HEIGHT).floor() as usize;
        let last = (viewport_bottom / LIST_ROW_HEIGHT).ceil() as usize + 1;
        let start = first
            .min(total_entries.saturating_sub(1))
            .saturating_sub(VIRTUAL_ROW_BUFFER);
        let end = (last + VIRTUAL_ROW_BUFFER)
            .min(total_entries)
            .max(start + 1);

        (start, end)
    }

    fn select_entries_in_drag_rect(&mut self) {
        let Some(selection) = self.selection_content_rect() else {
            return;
        };

        self.selected_paths = self
            .entries
            .iter()
            .enumerate()
            .filter(|(index, _)| {
                self.entry_content_rect(*index)
                    .is_some_and(|rect| rect.intersects(&selection))
            })
            .map(|(_, entry)| entry.file.path.clone())
            .collect();
        self.selection_anchor = self.entries.iter().find_map(|entry| {
            self.selected_paths
                .contains(&entry.file.path)
                .then(|| entry.file.path.clone())
        });
    }

    fn selection_content_rect(&self) -> Option<Rectangle> {
        let drag = self.selection_drag?;
        let mut rect = rect_from_points(drag.origin, drag.current);

        if rect.width < SELECTION_DRAG_THRESHOLD && rect.height < SELECTION_DRAG_THRESHOLD {
            return Some(Rectangle::new(rect.position(), Size::new(0.0, 0.0)));
        }

        rect.y += self.browser_scroll_y;
        Some(rect)
    }

    fn selection_view_rect(&self) -> Option<Rectangle> {
        let drag = self.selection_drag?;
        let rect = rect_from_points(drag.origin, drag.current);

        if rect.width < SELECTION_DRAG_THRESHOLD && rect.height < SELECTION_DRAG_THRESHOLD {
            None
        } else {
            Some(rect)
        }
    }

    fn entry_content_rect(&self, index: usize) -> Option<Rectangle> {
        match self.view_mode {
            ViewMode::Icons => {
                let columns = self.icon_grid_columns();
                let row = index / columns;
                let column = index % columns;
                Some(Rectangle::new(
                    Point::new(
                        GRID_PADDING_LEFT + column as f32 * (TILE_WIDTH + GRID_COLUMN_SPACING),
                        GRID_PADDING_TOP + row as f32 * (TILE_HEIGHT + GRID_ROW_SPACING),
                    ),
                    Size::new(TILE_WIDTH, TILE_HEIGHT),
                ))
            }
            ViewMode::List => {
                if index >= self.entries.len() {
                    return None;
                }

                Some(Rectangle::new(
                    Point::new(
                        LIST_PADDING_LEFT,
                        LIST_PADDING_TOP + LIST_HEADER_HEIGHT + index as f32 * LIST_ROW_HEIGHT,
                    ),
                    Size::new(10_000.0, LIST_ROW_HEIGHT),
                ))
            }
        }
    }

    fn selection_overlay(&self) -> Element<'_, Message> {
        let Some(rect) = self.selection_view_rect() else {
            return container(space()).height(Fill).width(Fill).into();
        };

        container(
            column![
                space().height(Length::Fixed(rect.y.max(0.0))),
                row![
                    space().width(Length::Fixed(rect.x.max(0.0))),
                    container(space())
                        .height(Length::Fixed(rect.height))
                        .width(Length::Fixed(rect.width))
                        .style(style::selection_rectangle),
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn rename_overlay(&self) -> Element<'_, Message> {
        let Some(rename) = &self.rename_state else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let Some((index, _entry)) = self
            .entries
            .iter()
            .enumerate()
            .find(|(_, entry)| entry.file.path == rename.path)
        else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let Some(rect) = self.entry_content_rect(index) else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let layout = match self.view_mode {
            ViewMode::Icons => rename_editor_layout(&rename.value, TILE_WIDTH),
            ViewMode::List => rename_editor_layout(&rename.value, LIST_RENAME_BASE_WIDTH),
        };
        let (x, y) = match self.view_mode {
            ViewMode::Icons => (
                self.clamped_overlay_x(rect.x + (TILE_WIDTH - layout.width) / 2.0, layout.width),
                rect.y + GRID_RENAME_Y_OFFSET - self.browser_scroll_y,
            ),
            ViewMode::List => (
                self.clamped_overlay_x(rect.x + LIST_RENAME_X_OFFSET, layout.width),
                rect.y + (LIST_ROW_HEIGHT - layout.height) / 2.0 - self.browser_scroll_y,
            ),
        };

        container(
            column![
                space().height(Length::Fixed(y.max(0.0))),
                row![
                    space().width(Length::Fixed(x)),
                    rename_editor(rename, layout.width, layout.height),
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn clamped_overlay_x(&self, x: f32, width: f32) -> f32 {
        clamped_overlay_x_for_width(self.browser_width, x, width)
    }

    fn browser_height(&self) -> f32 {
        (self.window_size.height - TOOLBAR_HEIGHT).max(0.0)
    }

    fn clamped_overlay_y(&self, y: f32, height: f32) -> f32 {
        clamped_overlay_y_for_height(self.browser_height(), y, height)
    }

    fn context_menu_position(&self, position: Point, height: f32) -> Point {
        self.context_menu_position_for_size(position, CONTEXT_MENU_WIDTH, height)
    }

    fn context_menu_position_for_size(&self, position: Point, width: f32, height: f32) -> Point {
        Point::new(
            self.clamped_overlay_x(position.x, width),
            self.clamped_overlay_y(position.y, height),
        )
    }

    fn default_properties_position(&self) -> Point {
        clamp_properties_position(
            self.window_size,
            Point::new(
                (self.window_size.width - PROPERTIES_DIALOG_WIDTH) / 2.0,
                (self.window_size.height - PROPERTIES_DIALOG_HEIGHT) / 2.0,
            ),
        )
    }

    fn clamp_properties_position(&mut self) {
        let window_size = self.window_size;
        if let Some(dialog) = &mut self.properties_dialog {
            dialog.position = clamp_properties_position(window_size, dialog.position);
        }
    }

    fn set_properties_position(&mut self, position: Point) {
        let window_size = self.window_size;
        if let Some(dialog) = &mut self.properties_dialog {
            dialog.position = clamp_properties_position(window_size, position);
        }
    }

    fn context_menu_overlay(&self) -> Element<'_, Message> {
        let Some(menu_state) = &self.context_menu else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let (position, menu): (Point, Element<'_, Message>) = match menu_state {
            ContextMenuState::Blank(position) => {
                let width = self.blank_context_menu_width();
                let height = self.blank_context_menu_height();
                (
                    self.context_menu_position_for_size(*position, width, height),
                    self.blank_context_menu(),
                )
            }
            ContextMenuState::Folder { position, path } => (
                self.context_menu_position(*position, self.folder_context_menu_height()),
                self.folder_context_menu(path.clone()),
            ),
            ContextMenuState::File { position, path } => (
                self.context_menu_position(*position, self.file_context_menu_height()),
                self.file_context_menu(path.clone()),
            ),
            ContextMenuState::Selection { position } => (
                self.context_menu_position(*position, self.selection_context_menu_height()),
                self.selection_context_menu(),
            ),
        };

        container(
            column![
                space().height(Length::Fixed(position.y.max(0.0))),
                row![
                    space().width(Length::Fixed(position.x.max(0.0))),
                    menu,
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn blank_context_menu_width(&self) -> f32 {
        if self.template_submenu_open && !self.template_files.is_empty() {
            CONTEXT_MENU_WIDTH + CONTEXT_MENU_SUBMENU_GAP + TEMPLATE_SUBMENU_WIDTH
        } else {
            CONTEXT_MENU_WIDTH
        }
    }

    fn blank_context_menu_height(&self) -> f32 {
        let has_templates = !self.template_files.is_empty();
        let custom_commands = self.runtime_config.blank_menu_commands.len();
        let items = 6 + usize::from(has_templates) + custom_commands;
        let separators = 3 + usize::from(custom_commands > 0);
        let panel_height = context_menu_panel_height(items, separators);

        if self.template_submenu_open && has_templates {
            panel_height.max(self.template_context_submenu_height())
        } else {
            panel_height
        }
    }

    fn template_context_submenu_height(&self) -> f32 {
        context_menu_panel_height(self.template_files.len(), 0)
    }

    fn folder_context_menu_height(&self) -> f32 {
        context_menu_panel_height(7, 4)
    }

    fn file_context_menu_height(&self) -> f32 {
        context_menu_panel_height(7, 3)
    }

    fn selection_context_menu_height(&self) -> f32 {
        context_menu_panel_height(3, 0)
    }

    fn blank_context_menu(&self) -> Element<'_, Message> {
        let mut panels = row![self.blank_context_menu_panel()]
            .spacing(CONTEXT_MENU_SUBMENU_GAP)
            .align_y(iced::Alignment::Start);

        if self.template_submenu_open && !self.template_files.is_empty() {
            panels = panels.push(self.template_context_submenu());
        }

        panels.into()
    }

    fn blank_context_menu_panel(&self) -> Element<'_, Message> {
        let mut menu = column![
            context_menu_item("新建文件", Message::ContextNewFile),
            context_menu_item("新建文件夹", Message::ContextNewFolder),
        ]
        .spacing(CONTEXT_MENU_ITEM_SPACING)
        .align_x(iced::Alignment::Start)
        .padding(CONTEXT_MENU_PANEL_PADDING);

        if !self.template_files.is_empty() {
            menu = menu.push(self.template_menu_item());
        }

        menu = menu
            .push(context_menu_separator())
            .push(context_menu_item("粘贴", Message::ContextPaste))
            .push(context_menu_item("全选", Message::ContextSelectAll))
            .push(context_menu_separator())
            .push(context_menu_item(
                "在终端打开",
                Message::ContextOpenTerminal,
            ))
            .push(context_menu_separator());

        for (index, command) in self.runtime_config.blank_menu_commands.iter().enumerate() {
            menu = menu.push(context_menu_item_owned(
                command.label.clone(),
                Message::ContextCustomCommand(index),
            ));
        }

        if !self.runtime_config.blank_menu_commands.is_empty() {
            menu = menu.push(context_menu_separator());
        }

        menu = menu.push(context_menu_item("属性", Message::ContextProperties));

        container(menu)
            .width(CONTEXT_MENU_WIDTH)
            .style(style::context_menu)
            .into()
    }

    fn template_menu_item(&self) -> Element<'_, Message> {
        let content = container(
            row![
                text("新建文档").size(14).style(style::primary_text),
                space().width(Fill),
                text("›").size(17).style(style::muted_text),
            ]
            .align_y(iced::Center)
            .width(Fill),
        )
        .height(Fill)
        .width(Fill)
        .align_y(Vertical::Center);

        button(content)
            .height(CONTEXT_MENU_ITEM_HEIGHT)
            .width(Fill)
            .padding([0, 10])
            .style(move |theme, status| {
                style::menu_button(theme, status, self.template_submenu_open)
            })
            .on_press(Message::ContextToggleTemplateSubmenu)
            .into()
    }

    fn template_context_submenu(&self) -> Element<'_, Message> {
        let mut menu = column![]
            .spacing(CONTEXT_MENU_ITEM_SPACING)
            .padding(CONTEXT_MENU_PANEL_PADDING);

        for (index, template) in self.template_files.iter().enumerate() {
            menu = menu.push(context_menu_item_owned(
                template.label.clone(),
                Message::ContextNewTemplateFile(index),
            ));
        }

        container(menu)
            .width(TEMPLATE_SUBMENU_WIDTH)
            .style(style::context_menu)
            .into()
    }

    fn folder_context_menu(&self, path: PathBuf) -> Element<'_, Message> {
        container(
            column![
                context_menu_item("打开", Message::FolderOpen(path.clone())),
                context_menu_separator(),
                context_menu_item("复制", Message::FolderCopy(path.clone())),
                context_menu_item("剪切", Message::FolderCut(path.clone())),
                context_menu_separator(),
                context_menu_item("重命名", Message::FolderRename(path.clone())),
                context_menu_item("删除", Message::FolderDelete(path.clone())),
                context_menu_separator(),
                context_menu_item("在终端打开", Message::FolderOpenTerminal(path.clone())),
                context_menu_separator(),
                context_menu_item("属性", Message::FolderProperties(path)),
            ]
            .spacing(CONTEXT_MENU_ITEM_SPACING)
            .align_x(iced::Alignment::Start)
            .padding(CONTEXT_MENU_PANEL_PADDING),
        )
        .width(CONTEXT_MENU_WIDTH)
        .style(style::context_menu)
        .into()
    }

    fn file_context_menu(&self, path: PathBuf) -> Element<'_, Message> {
        let default_label = self
            .default_app_for_path(&path)
            .map(|app| format!("用 {} 打开", app.name))
            .unwrap_or_else(|| "用默认应用打开".to_string());

        container(
            column![
                context_menu_item_owned(default_label, Message::FileOpenDefault(path.clone())),
                context_menu_item("打开方式", Message::FileOpenWith(path.clone())),
                context_menu_separator(),
                context_menu_item("复制", Message::FileCopy(path.clone())),
                context_menu_item("剪切", Message::FileCut(path.clone())),
                context_menu_separator(),
                context_menu_item("重命名", Message::FileRename(path.clone())),
                context_menu_item("删除", Message::FileDelete(path.clone())),
                context_menu_separator(),
                context_menu_item("属性", Message::FileProperties(path)),
            ]
            .spacing(CONTEXT_MENU_ITEM_SPACING)
            .align_x(iced::Alignment::Start)
            .padding(CONTEXT_MENU_PANEL_PADDING),
        )
        .width(CONTEXT_MENU_WIDTH)
        .style(style::context_menu)
        .into()
    }

    fn selection_context_menu(&self) -> Element<'_, Message> {
        container(
            column![
                context_menu_item("复制", Message::SelectionCopy),
                context_menu_item("剪切", Message::SelectionCut),
                context_menu_item("删除", Message::SelectionDelete),
            ]
            .spacing(CONTEXT_MENU_ITEM_SPACING)
            .align_x(iced::Alignment::Start)
            .padding(CONTEXT_MENU_PANEL_PADDING),
        )
        .width(CONTEXT_MENU_WIDTH)
        .style(style::context_menu)
        .into()
    }

    fn alert_overlay(&self) -> Element<'_, Message> {
        let Some(alert) = &self.alert_dialog else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let dialog = container(
            column![
                text(alert.title.clone())
                    .size(16)
                    .style(style::primary_text),
                text(alert.message.clone())
                    .size(14)
                    .style(style::primary_text),
                row![
                    space().width(Fill),
                    button(
                        container(text("确定").size(14).style(style::primary_text))
                            .height(Fill)
                            .align_x(Horizontal::Center)
                            .align_y(Vertical::Center),
                    )
                    .height(36)
                    .padding([0, 16])
                    .style(style::toolbar_button)
                    .on_press(Message::CloseAlert),
                ]
                .align_y(iced::Center),
            ]
            .spacing(18)
            .padding(18),
        )
        .width(360)
        .style(style::properties_dialog);

        let overlay = container(dialog)
            .width(Fill)
            .height(Fill)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .style(style::modal_overlay);

        mouse_area(overlay)
            .on_press(Message::PropertiesEventSink)
            .on_release(Message::PropertiesEventSink)
            .on_right_press(Message::PropertiesEventSink)
            .on_middle_press(Message::PropertiesEventSink)
            .on_scroll(|_| Message::PropertiesEventSink)
            .into()
    }

    fn delete_confirm_overlay(&self) -> Element<'_, Message> {
        let Some(confirm) = &self.delete_confirm else {
            return container(space()).height(Fill).width(Fill).into();
        };

        let dialog = container(
            column![
                text(&confirm.title).size(16).style(style::primary_text),
                text(&confirm.message).size(14).style(style::primary_text),
                row![
                    button(
                        container(text("取消").size(14).style(style::primary_text))
                            .height(Fill)
                            .align_x(Horizontal::Center)
                            .align_y(Vertical::Center),
                    )
                    .height(36)
                    .padding([0, 16])
                    .style(style::toolbar_button)
                    .on_press(Message::CancelDelete),
                    space().width(Fill),
                    button(
                        container(text("确定").size(14).style(style::primary_text))
                            .height(Fill)
                            .align_x(Horizontal::Center)
                            .align_y(Vertical::Center),
                    )
                    .height(36)
                    .padding([0, 16])
                    .style(style::danger_button)
                    .on_press(Message::ConfirmDelete(confirm.paths.clone())),
                ]
                .align_y(iced::Center),
            ]
            .spacing(18)
            .padding(18),
        )
        .width(360)
        .style(style::properties_dialog);

        let overlay = container(dialog)
            .width(Fill)
            .height(Fill)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .style(style::modal_overlay);

        mouse_area(overlay)
            .on_press(Message::PropertiesEventSink)
            .on_release(Message::PropertiesEventSink)
            .on_right_press(Message::PropertiesEventSink)
            .on_middle_press(Message::PropertiesEventSink)
            .on_scroll(|_| Message::PropertiesEventSink)
            .into()
    }

    fn open_with_overlay(&self) -> Element<'_, Message> {
        let Some(dialog) = &self.open_with_dialog else {
            return container(space()).height(Fill).width(Fill).into();
        };
        let set_as_default = dialog.set_as_default;

        let apps: Element<'_, Message> = if dialog.apps.is_empty() {
            container(text("没有可用应用").size(14).style(style::muted_text))
                .height(220)
                .width(Fill)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center)
                .into()
        } else {
            let mut rows = column![].spacing(4).padding(6);
            for app in &dialog.apps {
                rows = rows.push(self.open_with_app_row(app));
            }

            scrollable(rows).height(260).width(Fill).into()
        };

        let dialog_content = container(
            column![
                text(format!(
                    "选择应用以打开 {}",
                    display_name_for_path(&dialog.path)
                ))
                .size(16)
                .style(style::primary_text),
                apps,
                container(
                    column![
                        text("文件类型").size(13).style(style::muted_text),
                        text(dialog.mime.clone()).size(13).style(style::muted_text),
                    ]
                    .spacing(4)
                    .align_x(iced::Alignment::Start),
                )
                .height(54)
                .width(Fill)
                .align_y(Vertical::Center),
                button(
                    container(
                        row![
                            Self::open_with_check_indicator(set_as_default, 18.0),
                            text("设为默认打开方式").size(13).style(style::muted_text),
                        ]
                        .spacing(8)
                        .align_y(iced::Center),
                    )
                    .height(Fill)
                    .width(Fill)
                    .align_y(Vertical::Center),
                )
                .height(32)
                .width(Fill)
                .padding([0, 2])
                .style(move |theme, status| style::menu_button(theme, status, set_as_default))
                .on_press(Message::OpenWithSetDefault(!set_as_default)),
                row![
                    button(
                        container(text("取消(C)").size(14).style(style::primary_text))
                            .height(Fill)
                            .align_x(Horizontal::Center)
                            .align_y(Vertical::Center),
                    )
                    .height(36)
                    .padding([0, 16])
                    .style(style::toolbar_button)
                    .on_press(Message::OpenWithCancel),
                    space().width(Fill),
                    button(
                        container(text("打开(O)").size(14).style(style::primary_text))
                            .height(Fill)
                            .align_x(Horizontal::Center)
                            .align_y(Vertical::Center),
                    )
                    .height(36)
                    .padding([0, 16])
                    .style(style::danger_button)
                    .on_press(Message::OpenWithOpen),
                ]
                .align_y(iced::Center),
            ]
            .spacing(14)
            .padding(18),
        )
        .width(430)
        .style(style::properties_dialog);

        let overlay = container(dialog_content)
            .width(Fill)
            .height(Fill)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .style(style::modal_overlay);

        mouse_area(overlay)
            .on_press(Message::PropertiesEventSink)
            .on_release(Message::PropertiesEventSink)
            .on_right_press(Message::PropertiesEventSink)
            .on_middle_press(Message::PropertiesEventSink)
            .on_scroll(|_| Message::PropertiesEventSink)
            .into()
    }

    fn open_with_app_row(&self, app: &DesktopApp) -> Element<'_, Message> {
        let active = self
            .open_with_dialog
            .as_ref()
            .and_then(|dialog| dialog.selected_app_id.as_ref())
            == Some(&app.id);

        button(
            container(
                row![
                    entry_icon(&app.icon, 24.0),
                    text(app.name.clone()).size(14).style(style::primary_text),
                    space().width(Fill),
                    Self::open_with_check_indicator(active, 18.0),
                ]
                .spacing(10)
                .align_y(iced::Center)
                .width(Fill),
            )
            .height(Fill)
            .width(Fill)
            .align_x(Horizontal::Left)
            .align_y(Vertical::Center),
        )
        .height(38)
        .width(Fill)
        .padding([0, 10])
        .style(move |theme, status| style::menu_button(theme, status, active))
        .on_press(Message::OpenWithSelect(app.id.clone()))
        .into()
    }

    fn open_with_check_indicator(checked: bool, size: f32) -> Element<'static, Message> {
        let mark: Element<'static, Message> = if checked {
            svg(svg::Handle::from_memory(CHECK_MARK_SVG))
                .width(size - 4.0)
                .height(size - 4.0)
                .into()
        } else {
            space().width(size - 4.0).height(size - 4.0).into()
        };

        container(mark)
            .width(size)
            .height(size)
            .align_x(Horizontal::Center)
            .align_y(Vertical::Center)
            .style(move |_| style::check_indicator(checked))
            .into()
    }

    fn properties_overlay(&self) -> Element<'_, Message> {
        let Some(dialog) = &self.properties_dialog else {
            return container(space()).height(Fill).width(Fill).into();
        };
        let position = dialog.position;

        let title = match dialog.view {
            PropertiesView::Summary => "属性",
            PropertiesView::Permissions => "设置自定义权限",
        };

        let back = if dialog.view == PropertiesView::Summary {
            button(
                container(text("").size(16))
                    .height(Fill)
                    .width(Fill)
                    .align_x(Horizontal::Center)
                    .align_y(Vertical::Center),
            )
            .height(36)
            .width(36)
            .style(style::toolbar_button)
        } else {
            button(toolbar_icon(
                include_bytes!("../../../icons/left.svg"),
                true,
            ))
            .height(36)
            .width(36)
            .padding(6)
            .style(style::toolbar_button)
            .on_press(Message::SetPropertiesView(PropertiesView::Summary))
        };

        let draggable_title = mouse_area(
            container(text(title).size(16).style(style::muted_text))
                .height(36)
                .width(Fill)
                .align_x(Horizontal::Left)
                .align_y(Vertical::Center),
        )
        .on_press(Message::PropertiesDragStarted);

        let header = row![
            back,
            draggable_title,
            button(window_icon(include_bytes!("../../../icons/close.svg")))
                .height(36)
                .width(36)
                .padding(6)
                .style(style::close_button)
                .on_press(Message::CloseProperties),
        ]
        .align_y(iced::Center);

        let body = match (&dialog.state, dialog.view) {
            (PropertiesState::Loading(path), _) => self.properties_loading(path),
            (PropertiesState::Error(error), _) => self.properties_error(error),
            (PropertiesState::Loaded(properties), PropertiesView::Summary) => {
                self.properties_summary(properties)
            }
            (PropertiesState::Loaded(properties), PropertiesView::Permissions) => {
                self.properties_permissions(properties)
            }
            (PropertiesState::FileLoaded(properties), _) => {
                self.file_properties_summary(properties)
            }
        };

        let dialog = container(column![header, body].spacing(16).padding(14))
            .width(PROPERTIES_DIALOG_WIDTH)
            .style(style::properties_dialog);

        let overlay = container(
            column![
                space().height(Length::Fixed(position.y.max(0.0))),
                row![
                    space().width(Length::Fixed(position.x.max(0.0))),
                    dialog,
                    space().width(Fill),
                ],
                space().height(Fill),
            ]
            .spacing(0),
        )
        .width(Fill)
        .height(Fill)
        .style(style::modal_overlay);

        mouse_area(overlay)
            .on_move(Message::PropertiesPointerMoved)
            .on_press(Message::PropertiesEventSink)
            .on_release(Message::PropertiesDragEnded)
            .on_right_press(Message::PropertiesEventSink)
            .on_middle_press(Message::PropertiesEventSink)
            .on_scroll(|_| Message::PropertiesEventSink)
            .into()
    }

    fn properties_loading(&self, path: &Path) -> Element<'_, Message> {
        container(
            column![
                text(path.display().to_string())
                    .size(14)
                    .style(style::muted_text),
                text("正在统计文件夹属性...")
                    .size(16)
                    .style(style::primary_text),
            ]
            .spacing(10)
            .align_x(iced::Alignment::Start),
        )
        .height(220)
        .width(Fill)
        .padding(14)
        .align_x(Horizontal::Left)
        .align_y(Vertical::Center)
        .into()
    }

    fn properties_error<'a>(&self, error: &'a str) -> Element<'a, Message> {
        container(text(error).size(14).style(style::primary_text))
            .height(220)
            .width(Fill)
            .padding(14)
            .align_x(Horizontal::Left)
            .align_y(Vertical::Center)
            .into()
    }

    fn properties_summary(&self, properties: &FolderProperties) -> Element<'_, Message> {
        let parent = properties
            .parent
            .as_ref()
            .map(|path| path.display().to_string())
            .unwrap_or_else(|| "-".to_string());
        let free = properties
            .free_space
            .map(|size| format!("{} 剩余", format_size(size)))
            .unwrap_or_else(|| "剩余空间未知".to_string());

        column![
            container(
                row![
                    entry_icon(
                        &EntryIcon::Embedded(include_bytes!("../../../icons/folder.svg")),
                        72.0
                    ),
                    column![
                        text(properties.name.clone())
                            .size(22)
                            .style(style::primary_text),
                        text(format!(
                            "{} 项，共 {}",
                            properties.item_count,
                            format_size(properties.total_size)
                        ))
                        .size(14)
                        .style(style::primary_text),
                        text(free).size(13).style(style::muted_text),
                    ]
                    .spacing(6)
                    .align_x(iced::Alignment::Start)
                    .width(Fill),
                ]
                .spacing(14)
                .align_y(iced::Center)
                .width(Fill),
            )
            .height(112)
            .width(Fill)
            .align_y(Vertical::Center),
            container(property_row("上级文件夹(F)", parent))
                .width(Fill)
                .style(style::property_group),
            container(
                column![
                    property_row("修改时间", format_modified(properties.modified)),
                    property_row("创建时间", format_modified(properties.created)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            button(
                container(
                    row![
                        text("权限(P)")
                            .size(15)
                            .style(style::primary_text)
                            .width(Length::Fixed(PROPERTIES_LABEL_WIDTH)),
                        text(permission_summary(properties.mode))
                            .size(14)
                            .style(style::muted_text)
                            .width(Fill),
                        text("›").size(22).style(style::primary_text),
                    ]
                    .align_y(iced::Center)
                )
                .height(Fill)
                .width(Fill)
                .align_y(Vertical::Center)
            )
            .height(54)
            .width(Fill)
            .padding([0, 14])
            .style(style::property_button)
            .on_press(Message::SetPropertiesView(PropertiesView::Permissions)),
        ]
        .spacing(16)
        .into()
    }

    fn file_properties_summary(&self, properties: &FileProperties) -> Element<'_, Message> {
        let parent = properties
            .parent
            .as_ref()
            .map(|path| path.display().to_string())
            .unwrap_or_else(|| "-".to_string());
        let mime = self.mime_info_for_path(&properties.path);
        let file_type = mime.label;
        let executable = properties
            .mode
            .map(|mode| mode & 0o111 != 0)
            .unwrap_or(false);

        column![
            container(
                row![
                    entry_icon(
                        &EntryIcon::Embedded(include_bytes!("../../../icons/file.svg")),
                        72.0
                    ),
                    column![
                        text(properties.name.clone())
                            .size(22)
                            .style(style::primary_text),
                        text(file_type).size(14).style(style::primary_text),
                        text(format_size(properties.size))
                            .size(13)
                            .style(style::muted_text),
                    ]
                    .spacing(6)
                    .align_x(iced::Alignment::Start)
                    .width(Fill),
                ]
                .spacing(14)
                .align_y(iced::Center)
                .width(Fill),
            )
            .height(132)
            .width(Fill)
            .align_y(Vertical::Center),
            container(property_row("上级文件夹(F)", parent))
                .width(Fill)
                .style(style::property_group),
            container(
                column![
                    property_row("访问时间", format_modified(properties.accessed)),
                    property_row("修改时间", format_modified(properties.modified)),
                    property_row("创建时间", format_modified(properties.created)),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            container(
                column![
                    property_row("权限(P)", permission_summary(properties.mode)),
                    property_row(
                        "作为程序执行(E)",
                        if executable { "是" } else { "否" }.to_string()
                    ),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
        ]
        .spacing(16)
        .into()
    }

    fn properties_permissions(&self, properties: &FolderProperties) -> Element<'_, Message> {
        let dialog = self.properties_dialog.as_ref();
        let pending_mode = dialog
            .and_then(|dialog| dialog.permissions_mode)
            .or(properties.mode)
            .unwrap_or(0);
        let saving = dialog.is_some_and(|dialog| dialog.saving_permissions);
        let changed = properties.mode != Some(pending_mode);
        let can_apply = changed && !saving;
        let permission_error = dialog.and_then(|dialog| dialog.permission_error.as_ref());

        let apply = if can_apply {
            button(
                container(text("更改(H)").size(14).style(style::primary_text))
                    .height(Fill)
                    .align_x(Horizontal::Center)
                    .align_y(Vertical::Center),
            )
            .height(36)
            .padding([0, 16])
            .style(style::disabled_button)
            .on_press(Message::ApplyPermissions)
        } else {
            button(
                container(
                    text(if saving { "更改中..." } else { "更改(H)" })
                        .size(14)
                        .style(style::muted_text),
                )
                .height(Fill)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center),
            )
            .height(36)
            .padding([0, 16])
            .style(style::toolbar_button)
        };

        let error: Element<'_, Message> = if let Some(error) = permission_error {
            container(text(error).size(13).style(style::muted_text))
                .height(32)
                .width(Fill)
                .align_x(Horizontal::Left)
                .align_y(Vertical::Center)
                .into()
        } else {
            container(space()).height(0).width(Fill).into()
        };

        column![
            container(
                column![
                    property_row("所有者(U)", owner_label(properties.owner)),
                    permission_access_row("访问", pending_mode, PermissionClass::Owner, saving),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            container(
                column![
                    property_row("用户组(G)", group_label(properties.group)),
                    permission_access_row("访问", pending_mode, PermissionClass::Group, saving),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            container(
                column![
                    property_label_row("其它用户"),
                    permission_access_row("访问", pending_mode, PermissionClass::Other, saving),
                ]
                .spacing(0)
            )
            .width(Fill)
            .style(style::property_group),
            error,
            row![
                button(
                    container(text("取消(C)").size(14).style(style::primary_text))
                        .height(Fill)
                        .align_x(Horizontal::Center)
                        .align_y(Vertical::Center),
                )
                .height(36)
                .padding([0, 16])
                .style(style::toolbar_button)
                .on_press(Message::CancelPermissions),
                space().width(Fill),
                apply,
            ]
            .align_y(iced::Center),
        ]
        .spacing(16)
        .into()
    }

    fn cycle_permission(&mut self, class: PermissionClass) {
        let Some(dialog) = &mut self.properties_dialog else {
            return;
        };

        if dialog.saving_permissions {
            return;
        }

        let current_mode = dialog.permissions_mode.or(match &dialog.state {
            PropertiesState::Loaded(properties) => properties.mode,
            _ => None,
        });

        if let Some(mode) = current_mode {
            dialog.permissions_mode = Some(cycle_directory_access(mode, class));
            dialog.permission_error = None;
            self.status = "Permissions changed locally".to_string();
        }
    }

    fn cancel_permissions(&mut self) {
        let Some(dialog) = &mut self.properties_dialog else {
            return;
        };

        if dialog.saving_permissions {
            return;
        }

        if let PropertiesState::Loaded(properties) = &dialog.state {
            dialog.permissions_mode = properties.mode;
            dialog.permission_error = None;
            self.status = "Permissions unchanged".to_string();
        }
    }

    fn apply_permissions(&mut self) -> Task<Message> {
        let Some(dialog) = &mut self.properties_dialog else {
            return Task::none();
        };

        if dialog.saving_permissions {
            return Task::none();
        }

        let PropertiesState::Loaded(properties) = &dialog.state else {
            return Task::none();
        };

        let Some(mode) = dialog.permissions_mode else {
            return Task::none();
        };

        if properties.mode == Some(mode) {
            self.status = "Permissions unchanged".to_string();
            return Task::none();
        }

        let path = properties.path.clone();
        dialog.saving_permissions = true;
        dialog.permission_error = None;
        self.status = "Changing permissions...".to_string();

        Task::perform(
            async move {
                set_permissions(&path, mode)?;
                folder_properties(path)
            },
            Message::PropertiesPermissionsSaved,
        )
    }

    fn open_properties(&mut self, path: PathBuf) -> Task<Message> {
        self.properties_dialog = Some(PropertiesDialog {
            view: PropertiesView::Summary,
            state: PropertiesState::Loading(path.clone()),
            position: self.default_properties_position(),
            permissions_mode: None,
            saving_permissions: false,
            permission_error: None,
        });

        Task::perform(
            async move { folder_properties(path) },
            Message::PropertiesLoaded,
        )
    }

    fn open_file_properties(&mut self, path: PathBuf) -> Task<Message> {
        self.properties_dialog = Some(PropertiesDialog {
            view: PropertiesView::Summary,
            state: PropertiesState::Loading(path.clone()),
            position: self.default_properties_position(),
            permissions_mode: None,
            saving_permissions: false,
            permission_error: None,
        });

        Task::perform(
            async move { file_properties(path) },
            Message::FilePropertiesLoaded,
        )
    }

    fn open_with_dialog_for(&self, path: PathBuf) -> Option<OpenWithDialog> {
        let (path, mime_info) = self.open_target_and_mime_for_path(&path)?;
        let mime = mime_info.mime;
        let apps = apps_for_mime(&self.app_registry, &mime);
        let selected_app_id = best_app_for_mime(&self.app_registry, &mime)
            .and_then(|best| apps.iter().find(|app| app.id == best.id).cloned())
            .or_else(|| apps.first().cloned())
            .map(|app| app.id);

        Some(OpenWithDialog {
            path,
            mime,
            apps,
            selected_app_id,
            set_as_default: false,
        })
    }

    fn set_default_app_in_registry(&mut self, assignment: DefaultAppAssignment) {
        let defaults = self
            .app_registry
            .defaults
            .entry(assignment.mime)
            .or_default();
        defaults.retain(|app_id| app_id != &assignment.app_id);
        defaults.insert(0, assignment.app_id);
    }

    fn default_app_for_path(&self, path: &PathBuf) -> Option<DesktopApp> {
        let mime = self
            .open_target_and_mime_for_path(path)
            .map(|(_, mime)| mime)
            .unwrap_or_else(|| self.mime_info_for_path(path))
            .mime;
        best_app_for_mime(&self.app_registry, &mime)
    }

    fn mime_info_for_path(&self, path: &PathBuf) -> MimeInfo {
        self.entries
            .iter()
            .find(|entry| entry.file.path == *path)
            .map(|entry| entry.mime.clone())
            .unwrap_or_else(|| detect_name(path))
    }

    fn open_target_and_mime_for_path(&self, path: &PathBuf) -> Option<(PathBuf, MimeInfo)> {
        if let Some(entry) = self.entry_for_path(path) {
            if Self::entry_is_broken_symlink(entry) {
                return None;
            }

            return Some((Self::entry_open_path(entry), entry.mime.clone()));
        }

        Some((path.clone(), detect_name(path)))
    }

    fn entry_for_path(&self, path: &PathBuf) -> Option<&DisplayEntry> {
        self.entries.iter().find(|entry| entry.file.path == *path)
    }

    fn entry_effective_kind(entry: &DisplayEntry) -> EntryKind {
        if !matches!(entry.file.kind, EntryKind::Symlink) {
            return entry.file.kind;
        }

        match entry.file.symlink_target.as_ref() {
            Some(target) if !target.broken && target.kind == SymlinkTargetKind::Directory => {
                EntryKind::Directory
            }
            Some(target) if !target.broken && target.kind == SymlinkTargetKind::File => {
                EntryKind::File
            }
            _ => EntryKind::File,
        }
    }

    fn entry_is_broken_symlink(entry: &DisplayEntry) -> bool {
        matches!(entry.file.kind, EntryKind::Symlink)
            && entry
                .file
                .symlink_target
                .as_ref()
                .is_none_or(|target| target.broken)
    }

    fn entry_open_path(entry: &DisplayEntry) -> PathBuf {
        entry
            .file
            .symlink_target
            .as_ref()
            .and_then(|target| target.path.clone())
            .unwrap_or_else(|| entry.file.path.clone())
    }

    fn is_broken_symlink_path(&self, path: &PathBuf) -> bool {
        self.entry_for_path(path)
            .is_some_and(Self::entry_is_broken_symlink)
    }

    fn show_broken_symlink_alert(&mut self, path: &PathBuf) -> Task<Message> {
        let name = self
            .entry_for_path(path)
            .map(|entry| entry.file.name.clone())
            .unwrap_or_else(|| display_name_for_path(path));
        let message = format!("软连接 {name} 损坏");

        self.status = message.clone();
        self.alert_dialog = Some(AlertDialog {
            title: "软连接损坏".to_string(),
            message,
        });

        Task::none()
    }

    fn start_rename(&mut self, path: PathBuf) -> Task<Message> {
        let Some(value) = path
            .file_name()
            .map(|name| name.to_string_lossy().into_owned())
        else {
            self.status = "Cannot rename this folder".to_string();
            return Task::none();
        };

        let limits = path
            .parent()
            .and_then(|parent| child_path_limits(parent).ok())
            .unwrap_or_default();

        self.rename_state = Some(RenameState::new(path.clone(), value, limits));
        self.selected_paths.clear();
        self.selected_paths.insert(path.clone());
        self.selection_anchor = Some(path);
        self.status = "Enter a new name".to_string();
        self.focus_rename_input(true)
    }

    fn shortcuts_blocked(&self) -> bool {
        self.delete_confirm.is_some()
            || self.alert_dialog.is_some()
            || self.open_with_dialog.is_some()
            || self.properties_dialog.is_some()
    }

    fn selected_paths_vec(&self) -> Vec<PathBuf> {
        let mut seen = BTreeSet::new();
        let mut paths = Vec::new();

        for entry in &self.entries {
            if self.selected_paths.contains(&entry.file.path)
                && seen.insert(entry.file.path.clone())
            {
                paths.push(entry.file.path.clone());
            }
        }

        for path in &self.selected_paths {
            if seen.insert(path.clone()) {
                paths.push(path.clone());
            }
        }

        paths
    }

    fn set_selection_clipboard(&mut self, action: PasteAction) -> Task<Message> {
        let paths = self.selected_paths_vec();
        if paths.is_empty() {
            return Task::none();
        }

        self.close_menu();
        self.clipboard = Some(ClipboardState {
            action,
            paths: paths.clone(),
        });

        let target = selected_paths_label(&paths);
        self.status = match action {
            PasteAction::Copy => format!("Copied {target}"),
            PasteAction::Cut => format!("Cut {target}"),
        };
        Task::none()
    }

    fn show_delete_confirmation(
        &mut self,
        paths: Vec<PathBuf>,
        single_kind_label: Option<&'static str>,
    ) -> Task<Message> {
        if paths.is_empty() {
            return Task::none();
        }

        self.close_menu();
        let (title, message) = if paths.len() == 1 {
            let path = &paths[0];
            let kind_label = single_kind_label.unwrap_or_else(|| self.delete_kind_label(path));
            (
                format!("删除{kind_label}"),
                format!(
                    "确定删除 {} {}吗？",
                    display_name_for_path(path),
                    kind_label
                ),
            )
        } else {
            let count = paths.len();
            (
                format!("删除 {count} 项"),
                format!("确定删除选中的 {count} 项吗？"),
            )
        };

        self.delete_confirm = Some(DeleteConfirm {
            paths,
            title,
            message,
        });
        Task::none()
    }

    fn delete_kind_label(&self, path: &PathBuf) -> &'static str {
        self.entry_for_path(path)
            .filter(|entry| matches!(Self::entry_effective_kind(entry), EntryKind::Directory))
            .map(|_| "文件夹")
            .unwrap_or("文件")
    }

    fn paste_paths(&mut self, paths: Vec<PathBuf>, action: PasteAction) -> Task<Message> {
        if paths.is_empty() {
            self.status = "Clipboard does not contain local paths".to_string();
            return Task::none();
        }

        let operation_id = self.next_file_operation_id();
        let destination = self.cwd.clone();
        let cancel_token = FileOperationCancelToken::new();
        let now = Instant::now();
        let mut progress = initial_paste_progress(action);
        progress.total_items = paths.len() as u64;
        let operation = FileOperationState {
            id: operation_id,
            action,
            sources: paths.clone(),
            destination: destination.clone(),
            cancel_token: cancel_token.clone(),
            progress,
            status: FileOperationStatus::Queued,
            error: None,
            started_at: now,
            last_sample_at: now,
            last_sample_bytes: 0,
            speed_bytes_per_second: None,
            targets: Vec::new(),
        };

        self.file_operations.insert(operation_id, operation);
        self.status = match action {
            PasteAction::Copy => format!("Queued copy of {} item(s)", paths.len()),
            PasteAction::Cut => format!("Queued move of {} item(s)", paths.len()),
        };

        self.start_queued_file_operations()
    }

    fn next_file_operation_id(&mut self) -> u64 {
        self.next_file_operation_id += 1;
        self.next_file_operation_id
    }

    fn start_queued_file_operations(&mut self) -> Task<Message> {
        let available_slots =
            MAX_RUNNING_FILE_OPERATIONS.saturating_sub(self.active_file_operation_ids.len());
        if available_slots == 0 {
            return Task::none();
        }

        let queued_ids = self
            .file_operations
            .iter()
            .filter_map(|(id, operation)| {
                (operation.status == FileOperationStatus::Queued).then_some(*id)
            })
            .take(available_slots)
            .collect::<Vec<_>>();

        let mut tasks = Vec::new();

        for operation_id in queued_ids {
            let Some(operation) = self.file_operations.get_mut(&operation_id) else {
                continue;
            };

            operation.status = FileOperationStatus::Running;
            operation.started_at = Instant::now();
            operation.last_sample_at = operation.started_at;
            operation.last_sample_bytes = 0;
            operation.speed_bytes_per_second = None;
            operation.error = None;

            let sources = operation.sources.clone();
            let destination = operation.destination.clone();
            let action = operation.action;
            let cancel_token = operation.cancel_token.clone();
            self.active_file_operation_ids.insert(operation_id);
            self.status = match action {
                PasteAction::Copy => format!("Copying {} item(s)...", sources.len()),
                PasteAction::Cut => format!("Moving {} item(s)...", sources.len()),
            };
            tasks.push(paste_paths_stream(
                operation_id,
                sources,
                destination,
                action,
                cancel_token,
            ));
        }

        Task::batch(tasks)
    }

    fn file_operation_event(
        &mut self,
        operation_id: u64,
        event: FileOperationEvent,
    ) -> Task<Message> {
        match event {
            FileOperationEvent::Progress(event) => {
                self.apply_file_operation_progress(operation_id, event);
                Task::none()
            }
            FileOperationEvent::Finished(result) => {
                self.finish_file_operation(operation_id, result)
            }
        }
    }

    fn apply_file_operation_progress(&mut self, operation_id: u64, event: PasteProgressEvent) {
        let Some(operation) = self.file_operations.get_mut(&operation_id) else {
            return;
        };

        match event {
            PasteProgressEvent::Started(progress) | PasteProgressEvent::Progress(progress) => {
                update_file_operation_progress(operation, progress);
            }
            PasteProgressEvent::Finished { targets, progress } => {
                update_file_operation_progress(operation, progress);
                operation.targets = targets;
                operation.status = FileOperationStatus::Completed;
                operation.error = None;
            }
            PasteProgressEvent::Cancelled { targets, progress } => {
                update_file_operation_progress(operation, progress);
                operation.targets = targets;
                operation.status = FileOperationStatus::Cancelled;
                operation.error = Some("已取消".to_string());
            }
        }
    }

    fn finish_file_operation(
        &mut self,
        operation_id: u64,
        result: Result<Vec<PathBuf>, FsError>,
    ) -> Task<Message> {
        self.active_file_operation_ids.remove(&operation_id);

        if !self.file_operations.contains_key(&operation_id)
            && self.dismissed_file_operations.remove(&operation_id)
        {
            return Task::batch(vec![self.reload(), self.start_queued_file_operations()]);
        }

        let mut clear_clipboard = None;
        let mut refresh = false;
        let mut remove_completed = false;

        if let Some(operation) = self.file_operations.get_mut(&operation_id) {
            clear_clipboard = Some((operation.action, operation.sources.clone()));
            match result {
                Ok(paths) => {
                    operation.targets = paths.clone();
                    operation.status = FileOperationStatus::Completed;
                    operation.error = None;
                    operation.progress.phase = PasteProgressPhase::Finished;
                    operation.progress.completed_bytes = operation.progress.total_bytes;
                    operation.progress.completed_items = operation.progress.total_items;
                    operation.progress.current_source = None;
                    operation.progress.current_target = None;
                    operation.progress.current_bytes = 0;
                    operation.progress.current_total_bytes = 0;
                    self.status = format!("Pasted {} item(s)", paths.len());
                    refresh = true;
                    remove_completed = true;
                }
                Err(error) if error.is_cancelled() => {
                    operation.status = FileOperationStatus::Cancelled;
                    operation.error = Some("已取消".to_string());
                    operation.progress.phase = PasteProgressPhase::Cancelled;
                    self.status = "File operation cancelled".to_string();
                    refresh = true;
                }
                Err(error) => {
                    operation.status = FileOperationStatus::Failed;
                    operation.error = Some(error.to_string());
                    self.status = "File operation failed".to_string();
                    refresh = true;
                }
            }
        }

        if let Some((action, sources)) = clear_clipboard {
            self.clear_matching_clipboard(action, &sources);
        }

        let mut tasks = Vec::new();
        if refresh {
            tasks.push(self.reload());
        }
        if remove_completed {
            tasks.push(remove_completed_file_operation_task(operation_id));
        }
        tasks.push(self.start_queued_file_operations());

        Task::batch(tasks)
    }

    fn close_file_operation(&mut self, operation_id: u64) -> Task<Message> {
        let Some(operation) = self.file_operations.get(&operation_id) else {
            return Task::none();
        };

        if operation.status == FileOperationStatus::Running {
            let action = operation.action;
            let sources = operation.sources.clone();
            let was_active = self.active_file_operation_ids.contains(&operation_id);
            operation.cancel_token.cancel();
            self.clear_matching_clipboard(action, &sources);
            self.file_operations.remove(&operation_id);
            if was_active {
                self.dismissed_file_operations.insert(operation_id);
            }
            if self.file_operations.is_empty() {
                self.file_operations_expanded = false;
            }
            if !was_active {
                return self.start_queued_file_operations();
            }
        } else if operation.status == FileOperationStatus::Queued {
            let action = operation.action;
            let sources = operation.sources.clone();
            operation.cancel_token.cancel();
            self.clear_matching_clipboard(action, &sources);
            self.file_operations.remove(&operation_id);
            if self.file_operations.is_empty() {
                self.file_operations_expanded = false;
            }
        } else {
            self.file_operations.remove(&operation_id);
            if self.file_operations.is_empty() {
                self.file_operations_expanded = false;
            }
        }

        Task::none()
    }

    fn clear_matching_clipboard(&mut self, action: PasteAction, sources: &[PathBuf]) {
        if self
            .clipboard
            .as_ref()
            .is_some_and(|clipboard| clipboard.action == action && clipboard.paths == sources)
        {
            self.clipboard = None;
        }
    }

    fn submit_rename(&mut self) -> Task<Message> {
        if let Some(rename) = &mut self.rename_state {
            rename.value = rename.content.text();
        }

        let Some(rename) = self.rename_state.clone() else {
            return Task::none();
        };

        let new_name = rename.content.text().trim().to_string();
        if let Some(message) = rename_limit_error(&rename, &new_name) {
            self.status = message;
            return self.focus_rename_input(false);
        }

        if new_name.is_empty() {
            self.status = "File name cannot be empty".to_string();
            return self.focus_rename_input(false);
        }

        let current_name = rename
            .path
            .file_name()
            .and_then(|name| name.to_str())
            .unwrap_or_default();

        if new_name == current_name {
            self.rename_state = None;
            self.status = "Name unchanged".to_string();
            return Task::none();
        }

        self.status = "Renaming...".to_string();
        Task::perform(
            async move { rename_entry(rename.path, new_name) },
            Message::RenameFinished,
        )
    }

    fn prepare_rename_retry(&mut self) -> Task<Message> {
        if let Some(rename) = &mut self.rename_state {
            rename.content.perform(text_editor::Action::SelectAll);
            rename.value = rename.content.text();
        }

        self.focus_rename_input(false)
    }

    fn focus_rename_input(&self, select_all: bool) -> Task<Message> {
        if self.rename_state.is_none() {
            return Task::none();
        }

        let _ = select_all;
        operation::focus(rename_input_id())
    }

    fn reset_browser_scroll(&mut self) -> Task<Message> {
        self.browser_scroll_y = 0.0;
        operation::snap_to(self.browser_scroll_id(), operation::RelativeOffset::START)
    }

    fn browser_scroll_id(&self) -> iced::widget::Id {
        self.browser_scroll_id.clone()
    }

    fn browser_content_id(&self) -> iced::widget::Id {
        self.browser_content_id.clone()
    }

    fn pointer_over_entry(&self, position: Point) -> bool {
        self.entry_at_position(position).is_some()
    }

    fn folder_menu_path_at(&self, position: Point) -> Option<PathBuf> {
        let entry = self.entry_at_position(position)?;

        if self.selected_paths.len() == 1
            && self.selected_paths.contains(&entry.file.path)
            && matches!(Self::entry_effective_kind(entry), EntryKind::Directory)
        {
            Some(entry.file.path.clone())
        } else {
            None
        }
    }

    fn file_menu_path_at(&self, position: Point) -> Option<PathBuf> {
        let entry = self.entry_at_position(position)?;

        if self.selected_paths.len() == 1
            && self.selected_paths.contains(&entry.file.path)
            && !matches!(Self::entry_effective_kind(entry), EntryKind::Directory)
        {
            Some(entry.file.path.clone())
        } else {
            None
        }
    }

    fn entry_at_position(&self, position: Point) -> Option<&DisplayEntry> {
        let content_position = Point::new(position.x, position.y + self.browser_scroll_y);

        self.entries.iter().enumerate().find_map(|(index, entry)| {
            self.entry_content_rect(index)
                .is_some_and(|rect| rect_contains(rect, content_position))
                .then_some(entry)
        })
    }

    fn is_cut_path(&self, path: &PathBuf) -> bool {
        self.clipboard.as_ref().is_some_and(|clipboard| {
            clipboard.action == PasteAction::Cut && clipboard.paths.iter().any(|item| item == path)
        })
    }

    fn close_menu(&mut self) {
        self.menu_open = false;
        self.view_submenu_open = false;
        self.template_submenu_open = false;
        self.context_menu = None;
    }
}

fn context_menu_panel_height(items: usize, separators: usize) -> f32 {
    let children = items + separators;
    let content_height = items as f32 * CONTEXT_MENU_ITEM_HEIGHT as f32
        + separators as f32 * CONTEXT_MENU_SEPARATOR_HEIGHT as f32;
    let spacing = children.saturating_sub(1) as f32 * CONTEXT_MENU_ITEM_SPACING as f32;

    content_height + spacing + CONTEXT_MENU_PANEL_PADDING as f32 * 2.0
}

fn keyboard_shortcut_event(
    event: iced::Event,
    status: event::Status,
    _window: window::Id,
) -> Option<Message> {
    if status != event::Status::Ignored {
        return None;
    }

    let iced::Event::Keyboard(keyboard::Event::KeyPressed {
        key,
        physical_key,
        modifiers,
        repeat,
        ..
    }) = event
    else {
        return None;
    };

    if repeat {
        return None;
    }

    if modifiers.control() && !modifiers.shift() && !modifiers.alt() && !modifiers.logo() {
        return match key
            .to_latin(physical_key)
            .map(|key| key.to_ascii_lowercase())
        {
            Some('c') => Some(Message::SelectionCopy),
            Some('x') => Some(Message::SelectionCut),
            Some('v') => Some(Message::ShortcutPaste),
            _ => None,
        };
    }

    if !modifiers.control()
        && !modifiers.shift()
        && !modifiers.alt()
        && !modifiers.logo()
        && matches!(
            key.as_ref(),
            keyboard::Key::Named(keyboard::key::Named::Delete)
        )
    {
        return Some(Message::SelectionDelete);
    }

    None
}

fn selection_modifier_event(
    event: iced::Event,
    _status: event::Status,
    _window: window::Id,
) -> Option<Message> {
    match event {
        iced::Event::Keyboard(keyboard::Event::KeyPressed { modifiers, .. })
        | iced::Event::Keyboard(keyboard::Event::KeyReleased { modifiers, .. })
        | iced::Event::Keyboard(keyboard::Event::ModifiersChanged(modifiers)) => Some(
            Message::SelectionModifiersChanged(selection_modifiers(modifiers)),
        ),
        _ => None,
    }
}

fn selection_modifiers(modifiers: keyboard::Modifiers) -> SelectionModifiers {
    SelectionModifiers {
        control: modifiers.control(),
        shift: modifiers.shift(),
    }
}

fn initial_paste_progress(action: PasteAction) -> PasteProgress {
    PasteProgress {
        action,
        phase: PasteProgressPhase::Preparing,
        total_bytes: 0,
        completed_bytes: 0,
        total_items: 0,
        completed_items: 0,
        current_source: None,
        current_target: None,
        current_bytes: 0,
        current_total_bytes: 0,
    }
}

fn update_file_operation_progress(operation: &mut FileOperationState, progress: PasteProgress) {
    let now = Instant::now();
    let elapsed = now.duration_since(operation.last_sample_at).as_secs_f64();

    if elapsed >= 0.2 && progress.completed_bytes >= operation.last_sample_bytes {
        let delta = progress.completed_bytes - operation.last_sample_bytes;
        operation.speed_bytes_per_second = (delta > 0).then_some((delta as f64 / elapsed) as u64);
        operation.last_sample_at = now;
        operation.last_sample_bytes = progress.completed_bytes;
    }

    operation.progress = progress;
}

fn file_operation_fraction(operation: &FileOperationState) -> f32 {
    if operation.status == FileOperationStatus::Completed {
        return 1.0;
    }
    if operation.status == FileOperationStatus::Queued {
        return 0.0;
    }

    let progress = &operation.progress;
    let fraction = if progress.total_bytes > 0 {
        progress.completed_bytes as f32 / progress.total_bytes as f32
    } else if progress.total_items > 0 {
        progress.completed_items as f32 / progress.total_items as f32
    } else {
        0.0
    };

    fraction.clamp(0.0, 1.0)
}

fn selected_paths_label(paths: &[PathBuf]) -> String {
    if paths.len() == 1 {
        display_name_for_path(&paths[0])
    } else {
        format!("{} item(s)", paths.len())
    }
}

fn entry_name_tooltip<'a>(
    item: impl Into<Element<'a, Message>>,
    full_name: String,
    visible_name: String,
) -> Element<'a, Message> {
    if full_name == visible_name {
        return item.into();
    }

    let tip = container(
        text(full_name)
            .size(13)
            .width(Length::Fixed(360.0))
            .wrapping(iced::advanced::text::Wrapping::WordOrGlyph)
            .style(style::primary_text),
    )
    .padding(8);

    tooltip(item, tip, tooltip::Position::Bottom)
        .gap(8)
        .delay(Duration::from_millis(350))
        .style(style::entry_tooltip)
        .into()
}

fn paste_action_label(action: PasteAction) -> &'static str {
    match action {
        PasteAction::Copy => "复制",
        PasteAction::Cut => "剪切",
    }
}

fn file_operation_current_label(operation: &FileOperationState) -> String {
    match operation.status {
        FileOperationStatus::Queued => return "等待开始".to_string(),
        FileOperationStatus::Completed => return "已完成".to_string(),
        FileOperationStatus::Cancelled => return "已取消".to_string(),
        FileOperationStatus::Failed => {
            return operation
                .error
                .clone()
                .unwrap_or_else(|| "操作失败".to_string());
        }
        FileOperationStatus::Running => {}
    }

    let name = operation
        .progress
        .current_source
        .as_ref()
        .map(|path| display_name_for_path(path))
        .unwrap_or_else(|| "文件".to_string());

    match operation.progress.phase {
        PasteProgressPhase::Preparing => "正在准备...".to_string(),
        PasteProgressPhase::Renaming => format!("正在移动 {name}"),
        PasteProgressPhase::Copying => format!("正在复制 {name}"),
        PasteProgressPhase::DeletingSource => format!("正在删除源文件 {name}"),
        PasteProgressPhase::Finished => "已完成".to_string(),
        PasteProgressPhase::Cancelled => "已取消".to_string(),
    }
}

fn file_operation_speed_label(operation: &FileOperationState) -> String {
    match operation.status {
        FileOperationStatus::Queued => return "排队中".to_string(),
        FileOperationStatus::Completed => return "完成".to_string(),
        FileOperationStatus::Cancelled => return "已取消".to_string(),
        FileOperationStatus::Failed => return "失败".to_string(),
        FileOperationStatus::Running => {}
    }

    if matches!(
        operation.progress.phase,
        PasteProgressPhase::Renaming | PasteProgressPhase::DeletingSource
    ) {
        return "移动中".to_string();
    }

    let sampled_speed = operation.speed_bytes_per_second.or_else(|| {
        let elapsed = Instant::now()
            .duration_since(operation.started_at)
            .as_secs_f64();
        (elapsed > 0.2 && operation.progress.completed_bytes > 0)
            .then_some((operation.progress.completed_bytes as f64 / elapsed) as u64)
    });

    sampled_speed
        .map(|speed| format!("{}/s", format_size(speed)))
        .unwrap_or_else(|| "计算速度...".to_string())
}

fn compare_display_entries(left: &DisplayEntry, right: &DisplayEntry) -> Ordering {
    display_entry_sort_group(left)
        .cmp(&display_entry_sort_group(right))
        .then_with(|| {
            left.file
                .name
                .to_ascii_lowercase()
                .cmp(&right.file.name.to_ascii_lowercase())
        })
        .then_with(|| left.file.name.cmp(&right.file.name))
        .then_with(|| left.file.path.cmp(&right.file.path))
}

fn display_entry_sort_group(entry: &DisplayEntry) -> u8 {
    match entry.file.kind {
        EntryKind::Directory => 0,
        EntryKind::Symlink
            if entry.file.symlink_target.as_ref().is_some_and(|target| {
                !target.broken && target.kind == SymlinkTargetKind::Directory
            }) =>
        {
            0
        }
        _ => 1,
    }
}

fn same_entry_decoration_key(left: &FileEntry, right: &FileEntry) -> bool {
    left.name == right.name
        && left.kind == right.kind
        && left.symlink_target == right.symlink_target
}

#[cfg(test)]
mod tests {
    use super::*;

    fn key_press(
        key: keyboard::Key,
        physical_key: keyboard::key::Physical,
        modifiers: keyboard::Modifiers,
    ) -> iced::Event {
        iced::Event::Keyboard(keyboard::Event::KeyPressed {
            key: key.clone(),
            modified_key: key,
            physical_key,
            location: keyboard::Location::Standard,
            modifiers,
            text: None,
            repeat: false,
        })
    }

    fn operation_with_progress(
        status: FileOperationStatus,
        completed_bytes: u64,
        total_bytes: u64,
    ) -> FileOperationState {
        let now = Instant::now();

        FileOperationState {
            id: 1,
            action: PasteAction::Copy,
            sources: vec![PathBuf::from("/tmp/source.txt")],
            destination: PathBuf::from("/tmp/destination"),
            cancel_token: FileOperationCancelToken::new(),
            progress: PasteProgress {
                action: PasteAction::Copy,
                phase: PasteProgressPhase::Copying,
                total_bytes,
                completed_bytes,
                total_items: 1,
                completed_items: 0,
                current_source: Some(PathBuf::from("/tmp/source.txt")),
                current_target: Some(PathBuf::from("/tmp/destination/source.txt")),
                current_bytes: completed_bytes,
                current_total_bytes: total_bytes,
            },
            status,
            error: None,
            started_at: now,
            last_sample_at: now,
            last_sample_bytes: completed_bytes,
            speed_bytes_per_second: Some(2048),
            targets: Vec::new(),
        }
    }

    fn file_entry(name: &str) -> DisplayEntry {
        let path = PathBuf::from(format!("/tmp/{name}"));

        DisplayEntry {
            file: filesystem_core::FileEntry {
                name: name.to_string(),
                path,
                kind: EntryKind::File,
                symlink_target: None,
                hidden: false,
                size: None,
                owner: None,
                modified: None,
            },
            mime: MimeInfo::new("text/plain", filesystem_mime::MimeSource::BuiltInName),
            icon: EntryIcon::Embedded(include_bytes!("../../../icons/file.svg")),
            badge: None,
        }
    }

    fn manager_with_entries(names: &[&str]) -> FileManager {
        let (mut manager, _) = FileManager::new();
        manager.entries = names.iter().map(|name| file_entry(name)).collect();
        manager
    }

    fn desktop_app(id: &str, name: &str, exec: &str, mime_types: &[&str]) -> DesktopApp {
        DesktopApp {
            id: id.to_string(),
            name: name.to_string(),
            exec: exec.to_string(),
            mime_types: mime_types.iter().map(|mime| mime.to_string()).collect(),
            text_editor: false,
            icon: EntryIcon::Embedded(include_bytes!("../../../icons/app.svg")),
        }
    }

    #[test]
    fn keyboard_shortcuts_map_requested_keys() {
        let ctrl = keyboard::Modifiers::CTRL;
        let window = window::Id::unique();

        assert!(matches!(
            keyboard_shortcut_event(
                key_press(
                    keyboard::Key::Character("c".into()),
                    keyboard::key::Physical::Code(keyboard::key::Code::KeyC),
                    ctrl,
                ),
                event::Status::Ignored,
                window,
            ),
            Some(Message::SelectionCopy)
        ));
        assert!(matches!(
            keyboard_shortcut_event(
                key_press(
                    keyboard::Key::Character("x".into()),
                    keyboard::key::Physical::Code(keyboard::key::Code::KeyX),
                    ctrl,
                ),
                event::Status::Ignored,
                window,
            ),
            Some(Message::SelectionCut)
        ));
        assert!(matches!(
            keyboard_shortcut_event(
                key_press(
                    keyboard::Key::Character("v".into()),
                    keyboard::key::Physical::Code(keyboard::key::Code::KeyV),
                    ctrl,
                ),
                event::Status::Ignored,
                window,
            ),
            Some(Message::ShortcutPaste)
        ));
        assert!(matches!(
            keyboard_shortcut_event(
                key_press(
                    keyboard::Key::Named(keyboard::key::Named::Delete),
                    keyboard::key::Physical::Code(keyboard::key::Code::Delete),
                    keyboard::Modifiers::NONE,
                ),
                event::Status::Ignored,
                window,
            ),
            Some(Message::SelectionDelete)
        ));
    }

    #[test]
    fn keyboard_shortcuts_ignore_captured_or_repeated_events() {
        let captured_event = key_press(
            keyboard::Key::Character("c".into()),
            keyboard::key::Physical::Code(keyboard::key::Code::KeyC),
            keyboard::Modifiers::CTRL,
        );

        assert!(keyboard_shortcut_event(
            captured_event,
            event::Status::Captured,
            window::Id::unique()
        )
        .is_none());

        let repeated = iced::Event::Keyboard(keyboard::Event::KeyPressed {
            key: keyboard::Key::Character("c".into()),
            modified_key: keyboard::Key::Character("c".into()),
            physical_key: keyboard::key::Physical::Code(keyboard::key::Code::KeyC),
            location: keyboard::Location::Standard,
            modifiers: keyboard::Modifiers::CTRL,
            text: None,
            repeat: true,
        });

        assert!(
            keyboard_shortcut_event(repeated, event::Status::Ignored, window::Id::unique())
                .is_none()
        );
    }

    #[test]
    fn selection_modifier_events_track_ctrl_and_shift() {
        let event = key_press(
            keyboard::Key::Character("a".into()),
            keyboard::key::Physical::Code(keyboard::key::Code::KeyA),
            keyboard::Modifiers::CTRL | keyboard::Modifiers::SHIFT,
        );

        let Some(Message::SelectionModifiersChanged(modifiers)) =
            selection_modifier_event(event, iced::event::Status::Captured, window::Id::unique())
        else {
            panic!("expected selection modifier message");
        };

        assert_eq!(
            modifiers,
            SelectionModifiers {
                control: true,
                shift: true,
            }
        );
    }

    #[test]
    fn ctrl_click_toggles_entry_selection() {
        let mut manager = manager_with_entries(&["alpha", "bravo", "charlie"]);
        let first = manager.entries[0].file.path.clone();
        let second = manager.entries[1].file.path.clone();

        let _ = manager.update(Message::SelectEntry(first.clone()));
        let _ = manager.update(Message::SelectionModifiersChanged(SelectionModifiers {
            control: true,
            shift: false,
        }));
        let _ = manager.update(Message::SelectEntry(second.clone()));

        assert!(manager.selected_paths.contains(&first));
        assert!(manager.selected_paths.contains(&second));

        let _ = manager.update(Message::SelectEntry(first.clone()));

        assert!(!manager.selected_paths.contains(&first));
        assert!(manager.selected_paths.contains(&second));
    }

    #[test]
    fn shift_click_selects_display_order_range_from_anchor() {
        let mut manager = manager_with_entries(&["delta", "alpha", "charlie", "bravo"]);
        let paths = manager
            .entries
            .iter()
            .map(|entry| entry.file.path.clone())
            .collect::<Vec<_>>();

        let _ = manager.update(Message::SelectEntry(paths[0].clone()));
        let _ = manager.update(Message::SelectionModifiersChanged(SelectionModifiers {
            control: false,
            shift: true,
        }));
        let _ = manager.update(Message::SelectEntry(paths[3].clone()));

        assert_eq!(manager.selected_paths_vec(), paths);
    }

    #[test]
    fn ctrl_shift_click_adds_display_order_range_to_existing_selection() {
        let mut manager = manager_with_entries(&["alpha", "bravo", "charlie", "delta", "echo"]);
        let paths = manager
            .entries
            .iter()
            .map(|entry| entry.file.path.clone())
            .collect::<Vec<_>>();

        let _ = manager.update(Message::SelectEntry(paths[0].clone()));
        let _ = manager.update(Message::SelectionModifiersChanged(SelectionModifiers {
            control: true,
            shift: false,
        }));
        let _ = manager.update(Message::SelectEntry(paths[4].clone()));
        let _ = manager.update(Message::SelectionModifiersChanged(SelectionModifiers {
            control: true,
            shift: true,
        }));
        let _ = manager.update(Message::SelectEntry(paths[2].clone()));

        assert_eq!(
            manager.selected_paths_vec(),
            vec![
                paths[0].clone(),
                paths[2].clone(),
                paths[3].clone(),
                paths[4].clone()
            ]
        );
    }

    #[test]
    fn selection_copy_and_cut_store_all_selected_paths() {
        let (mut manager, _) = FileManager::new();
        let first = PathBuf::from("/tmp/alpha.txt");
        let second = PathBuf::from("/tmp/bravo");

        manager.selected_paths.insert(second.clone());
        manager.selected_paths.insert(first.clone());
        let _ = manager.update(Message::SelectionCopy);

        let clipboard = manager.clipboard.as_ref().unwrap();
        assert_eq!(clipboard.action, PasteAction::Copy);
        assert_eq!(clipboard.paths, vec![first.clone(), second.clone()]);
        assert_eq!(manager.status, "Copied 2 item(s)");

        let _ = manager.update(Message::SelectionCut);

        let clipboard = manager.clipboard.as_ref().unwrap();
        assert_eq!(clipboard.action, PasteAction::Cut);
        assert_eq!(clipboard.paths, vec![first, second]);
        assert_eq!(manager.status, "Cut 2 item(s)");
    }

    #[test]
    fn shortcut_paste_only_runs_without_selection() {
        let (mut manager, _) = FileManager::new();
        manager.clipboard = Some(ClipboardState {
            action: PasteAction::Copy,
            paths: vec![PathBuf::from("/tmp/source.txt")],
        });
        manager
            .selected_paths
            .insert(PathBuf::from("/tmp/source.txt"));

        let _ = manager.update(Message::ShortcutPaste);

        assert!(manager.file_operations.is_empty());

        manager.selected_paths.clear();
        let _ = manager.update(Message::ShortcutPaste);

        assert_eq!(manager.file_operations.len(), 1);
    }

    #[test]
    fn selection_delete_creates_multi_path_confirmation() {
        let (mut manager, _) = FileManager::new();
        let first = PathBuf::from("/tmp/alpha.txt");
        let second = PathBuf::from("/tmp/bravo");

        manager.selected_paths.insert(first.clone());
        manager.selected_paths.insert(second.clone());
        let _ = manager.update(Message::SelectionDelete);

        let confirm = manager.delete_confirm.as_ref().unwrap();
        assert_eq!(confirm.paths, vec![first, second]);
        assert_eq!(confirm.title, "删除 2 项");
        assert_eq!(confirm.message, "确定删除选中的 2 项吗？");
    }

    #[test]
    fn directory_started_updates_navigation() {
        let mut manager = manager_with_entries(&["old-1.txt", "old-2.txt"]);
        manager.cwd = PathBuf::from("/tmp/many");
        manager.back_history = vec![PathBuf::from("/tmp/few")];
        manager.browser_scroll_y = 2400.0;
        manager.active_request_id = Some(42);

        let request = DirectoryRequest {
            id: 42,
            path: PathBuf::from("/tmp/few"),
            mode: DirectoryLoadMode::Back,
            previous: Some(PathBuf::from("/tmp/many")),
        };

        let _ = manager.directory_event(request, DirectoryLoadEvent::Started);

        assert_eq!(manager.cwd, PathBuf::from("/tmp/many"));
        assert!(manager.back_history.is_empty());
        assert_eq!(manager.forward_history.len(), 1);
        assert_eq!(manager.forward_history[0], PathBuf::from("/tmp/many"));
    }

    #[test]
    fn load_path_clears_old_directory_state() {
        let mut manager = manager_with_entries(&["old-1.txt", "old-2.txt"]);
        manager.cwd = PathBuf::from("/tmp/many");
        manager.browser_generation = 3;
        let previous_generation = manager.browser_generation;
        manager.search_query = Some("old".to_string());
        manager.search_root = Some(PathBuf::from("/tmp"));
        manager.auto_refresh_snapshot_id = Some(7);
        manager.auto_refresh_snapshot_dirty = true;
        manager
            .selected_paths
            .insert(PathBuf::from("/tmp/old-1.txt"));
        manager.browser_scroll_y = 2400.0;
        manager.selection_anchor = Some(PathBuf::from("/tmp/old-2.txt"));
        manager.selection_drag = Some(SelectionDrag {
            origin: Point::new(12.0, 34.0),
            current: Point::new(1.0, 2.0),
        });

        let _ = manager.load_path(
            PathBuf::from("/tmp/few"),
            DirectoryLoadMode::Back,
            Some(PathBuf::from("/tmp/many")),
        );

        assert!(manager.entries.is_empty());
        assert_eq!(manager.cwd, PathBuf::from("/tmp/few"));
        assert_eq!(manager.browser_generation, previous_generation + 1);
        assert!(manager.search_query.is_none());
        assert!(manager.search_root.is_none());
        assert!(manager.auto_refresh_snapshot_id.is_none());
        assert!(!manager.auto_refresh_snapshot_dirty);
        assert!(manager.selected_paths.is_empty());
        assert!(manager.selection_anchor.is_none());
        assert!(manager.selection_drag.is_none());
        assert_eq!(manager.browser_scroll_y, 0.0);
    }

    #[test]
    fn stale_browser_scroll_event_is_ignored() {
        let (mut manager, _) = FileManager::new();
        manager.browser_generation = 5;
        let stale_generation = manager.browser_generation + 1;

        let _ = manager.update(Message::BrowserScrolled(stale_generation, 2400.0));

        assert_eq!(manager.browser_scroll_y, 0.0);

        let _ = manager.update(Message::BrowserScrolled(5, 120.0));
        assert_eq!(manager.browser_scroll_y, 120.0);
    }

    #[test]
    fn template_files_loaded_closes_empty_template_submenu() {
        let (mut manager, _) = FileManager::new();
        manager.template_submenu_open = true;

        let _ = manager.update(Message::TemplateFilesLoaded(Vec::new()));

        assert!(manager.template_files.is_empty());
        assert!(!manager.template_submenu_open);
    }

    #[test]
    fn blank_context_menu_width_includes_template_submenu() {
        let (mut manager, _) = FileManager::new();
        manager.template_files = vec![TemplateFile {
            label: "Report".to_string(),
            path: PathBuf::from("/home/user/Templates/Report.txt"),
        }];

        assert_eq!(manager.blank_context_menu_width(), CONTEXT_MENU_WIDTH);

        manager.template_submenu_open = true;

        assert_eq!(
            manager.blank_context_menu_width(),
            CONTEXT_MENU_WIDTH + CONTEXT_MENU_SUBMENU_GAP + TEMPLATE_SUBMENU_WIDTH
        );
    }

    #[test]
    fn context_menu_position_clamps_to_browser_bottom() {
        let (mut manager, _) = FileManager::new();
        manager.window_size = Size::new(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);
        let height = manager.selection_context_menu_height();
        let position = manager.context_menu_position(Point::new(40.0, 520.0), height);

        assert_eq!(height, 112.0);
        assert_eq!(position.x, 40.0);
        assert_eq!(position.y, WINDOW_MIN_HEIGHT - TOOLBAR_HEIGHT - height);
    }

    #[test]
    fn blank_context_menu_height_uses_taller_template_submenu() {
        let (mut manager, _) = FileManager::new();
        manager.template_files = (0..12)
            .map(|index| TemplateFile {
                label: format!("Template {index}"),
                path: PathBuf::from(format!("/home/user/Templates/{index}.txt")),
            })
            .collect();
        manager.template_submenu_open = true;

        assert_eq!(
            manager.blank_context_menu_height(),
            manager.template_context_submenu_height()
        );
    }

    #[test]
    fn template_create_finished_starts_rename() {
        let (mut manager, _) = FileManager::new();
        let path = PathBuf::from("/tmp/Report.txt");

        let _ = manager.update(Message::TemplateCreateFinished(Ok(path.clone())));

        let rename = manager.rename_state.as_ref().unwrap();
        assert_eq!(rename.path, path);
        assert_eq!(rename.value, "Report.txt");
    }

    #[test]
    fn delete_finished_clears_deleted_selection_and_clipboard() {
        let (mut manager, _) = FileManager::new();
        let first = PathBuf::from("/tmp/alpha.txt");
        let second = PathBuf::from("/tmp/bravo");

        manager.selected_paths.insert(first.clone());
        manager.selected_paths.insert(second.clone());
        manager.clipboard = Some(ClipboardState {
            action: PasteAction::Cut,
            paths: vec![first.clone(), second.clone()],
        });

        let _ = manager.update(Message::DeleteFinished(DeleteEntriesOutcome {
            paths: vec![first.clone()],
            error: None,
        }));

        assert!(!manager.selected_paths.contains(&first));
        assert!(!manager.selected_paths.contains(&second));
        assert!(manager.clipboard.is_none());
    }

    #[test]
    fn open_with_set_default_toggles_dialog_state() {
        let (mut manager, _) = FileManager::new();
        manager.open_with_dialog = Some(OpenWithDialog {
            path: PathBuf::from("/tmp/readme.txt"),
            mime: "text/plain".to_string(),
            apps: Vec::new(),
            selected_app_id: None,
            set_as_default: false,
        });

        let _ = manager.update(Message::OpenWithSetDefault(true));

        assert!(manager
            .open_with_dialog
            .as_ref()
            .is_some_and(|dialog| dialog.set_as_default));

        let _ = manager.update(Message::OpenWithSetDefault(false));

        assert!(manager
            .open_with_dialog
            .as_ref()
            .is_some_and(|dialog| !dialog.set_as_default));
    }

    #[test]
    fn open_file_finished_updates_in_memory_default_app() {
        let (mut manager, _) = FileManager::new();
        manager.app_registry.defaults.insert(
            "text/plain".to_string(),
            vec!["old-editor.desktop".to_string()],
        );

        let _ = manager.update(Message::OpenFileFinished(Ok(OpenFileOutcome {
            app_name: "Editor".to_string(),
            default_assignment: Some(DefaultAppAssignment {
                mime: "text/plain".to_string(),
                app_id: "editor.desktop".to_string(),
            }),
            default_error: None,
        })));

        assert_eq!(
            manager.app_registry.defaults.get("text/plain"),
            Some(&vec![
                "editor.desktop".to_string(),
                "old-editor.desktop".to_string()
            ])
        );
        assert_eq!(manager.status, "Opened with Editor; set as default");
    }

    #[test]
    fn open_file_finished_reports_default_error_without_registry_update() {
        let (mut manager, _) = FileManager::new();

        let _ = manager.update(Message::OpenFileFinished(Ok(OpenFileOutcome {
            app_name: "Editor".to_string(),
            default_assignment: None,
            default_error: Some("permission denied".to_string()),
        })));

        assert!(manager.app_registry.defaults.is_empty());
        assert_eq!(
            manager.status,
            "Opened with Editor; failed to set default: permission denied"
        );
    }

    #[test]
    fn double_click_wps_family_files_uses_wps_app() {
        let mut manager = manager_with_entries(&["sample.wps", "sample.docx"]);
        manager.app_registry = AppRegistry {
            apps: vec![desktop_app(
                "wps-office-wps.desktop",
                "WPS Writer",
                "/usr/bin/wps %F",
                &["application/wps-office.wps"],
            )],
            defaults: BTreeMap::from([(
                "application/wps-office.wps".to_string(),
                vec!["wps-office-wps.desktop".to_string()],
            )]),
        };

        manager.entries[0].mime = MimeInfo::new(
            "application/wps-office.wps",
            filesystem_mime::MimeSource::BuiltInName,
        );
        manager.entries[1].mime = MimeInfo::new(
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            filesystem_mime::MimeSource::BuiltInName,
        );

        let wps_path = manager.entries[0].file.path.clone();
        let docx_path = manager.entries[1].file.path.clone();

        let _ = manager.update(Message::Open(wps_path, EntryKind::File));
        assert_eq!(manager.status, "Opening with WPS Writer...");

        let _ = manager.update(Message::Open(docx_path, EntryKind::File));
        assert_eq!(manager.status, "Opening with WPS Writer...");
    }

    #[test]
    fn current_folder_change_ignores_non_current_path() {
        let mut manager = manager_with_entries(&["alpha.txt"]);
        manager.cwd = PathBuf::from("/tmp");
        let change = CurrentFolderChange {
            path: PathBuf::from("/var"),
            kind: CurrentFolderChangeKind::Structure,
            paths: vec![PathBuf::from("/var/alpha.txt")],
        };

        let _ = manager.update(Message::CurrentFolderChanged(change));

        assert!(manager.auto_refresh_snapshot_id.is_none());
        assert_eq!(manager.entries.len(), 1);
    }

    #[test]
    fn structure_change_starts_snapshot_without_clearing_entries() {
        let mut manager = manager_with_entries(&["alpha.txt", "bravo.txt"]);
        manager.cwd = PathBuf::from("/tmp");
        let previous_generation = manager.browser_generation;
        let previous_entries = manager
            .entries
            .iter()
            .map(|entry| entry.file.name.clone())
            .collect::<Vec<_>>();
        let change = CurrentFolderChange {
            path: manager.cwd.clone(),
            kind: CurrentFolderChangeKind::Structure,
            paths: vec![manager.cwd.join("charlie.txt")],
        };

        let _ = manager.update(Message::CurrentFolderChanged(change));

        assert!(manager.auto_refresh_snapshot_id.is_some());
        assert_eq!(manager.browser_generation, previous_generation);
        assert_eq!(
            manager
                .entries
                .iter()
                .map(|entry| entry.file.name.clone())
                .collect::<Vec<_>>(),
            previous_entries
        );
    }

    #[test]
    fn structure_change_during_snapshot_marks_dirty() {
        let mut manager = manager_with_entries(&["alpha.txt"]);
        manager.cwd = PathBuf::from("/tmp");
        manager.auto_refresh_snapshot_id = Some(42);
        let change = CurrentFolderChange {
            path: manager.cwd.clone(),
            kind: CurrentFolderChangeKind::Structure,
            paths: vec![manager.cwd.join("bravo.txt")],
        };

        let _ = manager.update(Message::CurrentFolderChanged(change));

        assert_eq!(manager.auto_refresh_snapshot_id, Some(42));
        assert!(manager.auto_refresh_snapshot_dirty);
    }

    #[test]
    fn entry_auto_refresh_updates_single_entry_without_clearing() {
        let mut manager = manager_with_entries(&["alpha.txt", "bravo.txt"]);
        manager.cwd = PathBuf::from("/tmp");
        manager.browser_generation = 9;
        manager.browser_scroll_y = 120.0;
        let mut updated = file_entry("alpha.txt");
        updated.file.size = Some(42);

        let _ = manager.update(Message::AutoRefreshEntryLoaded(
            1,
            PathBuf::from("/tmp/alpha.txt"),
            Ok(updated),
        ));

        assert_eq!(manager.entries.len(), 2);
        assert_eq!(manager.entries[0].file.name, "alpha.txt");
        assert_eq!(manager.entries[0].file.size, Some(42));
        assert_eq!(manager.browser_generation, 9);
        assert_eq!(manager.browser_scroll_y, 120.0);
    }

    #[test]
    fn snapshot_auto_refresh_merges_entries_and_retains_visible_state() {
        let mut manager = manager_with_entries(&["alpha.txt", "bravo.txt"]);
        manager.cwd = PathBuf::from("/tmp");
        manager.browser_scroll_y = 120.0;
        manager
            .selected_paths
            .insert(PathBuf::from("/tmp/alpha.txt"));
        manager
            .selected_paths
            .insert(PathBuf::from("/tmp/bravo.txt"));
        manager.selection_anchor = Some(PathBuf::from("/tmp/bravo.txt"));

        let entries = vec![file_entry("alpha.txt"), file_entry("charlie.txt")];
        let decorate_entries = manager.apply_auto_refresh_snapshot(entries);

        assert_eq!(
            manager
                .entries
                .iter()
                .map(|entry| entry.file.name.as_str())
                .collect::<Vec<_>>(),
            vec!["alpha.txt", "charlie.txt"]
        );
        assert!(manager
            .selected_paths
            .contains(&PathBuf::from("/tmp/alpha.txt")));
        assert!(!manager
            .selected_paths
            .contains(&PathBuf::from("/tmp/bravo.txt")));
        assert!(manager.selection_anchor.is_none());
        assert_eq!(manager.browser_scroll_y, 120.0);
        assert_eq!(
            decorate_entries
                .iter()
                .map(|entry| entry.name.as_str())
                .collect::<Vec<_>>(),
            vec!["alpha.txt", "charlie.txt"]
        );
    }

    #[test]
    fn file_operation_fraction_uses_byte_progress() {
        let operation = operation_with_progress(FileOperationStatus::Running, 25, 100);

        assert_eq!(file_operation_fraction(&operation), 0.25);
    }

    #[test]
    fn completed_file_operation_fraction_is_full() {
        let operation = operation_with_progress(FileOperationStatus::Completed, 25, 100);

        assert_eq!(file_operation_fraction(&operation), 1.0);
        assert_eq!(file_operation_current_label(&operation), "已完成");
        assert_eq!(file_operation_speed_label(&operation), "完成");
    }

    #[test]
    fn running_file_operation_labels_current_file_and_speed() {
        let operation = operation_with_progress(FileOperationStatus::Running, 25, 100);

        assert_eq!(
            file_operation_current_label(&operation),
            "正在复制 source.txt"
        );
        assert_eq!(file_operation_speed_label(&operation), "2.0 KB/s");
    }

    #[test]
    fn queued_file_operation_labels_waiting() {
        let operation = operation_with_progress(FileOperationStatus::Queued, 25, 100);

        assert_eq!(file_operation_fraction(&operation), 0.0);
        assert_eq!(file_operation_current_label(&operation), "等待开始");
        assert_eq!(file_operation_speed_label(&operation), "排队中");
    }

    #[test]
    fn file_operations_start_at_most_three_workers() {
        let (mut manager, _) = FileManager::new();

        for index in 0..4 {
            let _ = manager.paste_paths(
                vec![PathBuf::from(format!("/tmp/source-{index}.txt"))],
                PasteAction::Copy,
            );
        }

        let running = manager
            .file_operations
            .values()
            .filter(|operation| operation.status == FileOperationStatus::Running)
            .count();
        let queued = manager
            .file_operations
            .values()
            .filter(|operation| operation.status == FileOperationStatus::Queued)
            .count();

        assert_eq!(manager.active_file_operation_ids.len(), 3);
        assert_eq!(running, 3);
        assert_eq!(queued, 1);
    }

    #[test]
    fn finishing_file_operation_starts_next_queued_operation() {
        let (mut manager, _) = FileManager::new();

        for index in 0..4 {
            let _ = manager.paste_paths(
                vec![PathBuf::from(format!("/tmp/source-{index}.txt"))],
                PasteAction::Copy,
            );
        }

        let finished_id = *manager.active_file_operation_ids.iter().next().unwrap();
        let queued_id = manager
            .file_operations
            .iter()
            .find_map(|(id, operation)| {
                (operation.status == FileOperationStatus::Queued).then_some(*id)
            })
            .unwrap();

        let _ = manager.finish_file_operation(finished_id, Ok(Vec::new()));

        assert_eq!(
            manager.file_operations.get(&queued_id).unwrap().status,
            FileOperationStatus::Running
        );
        assert_eq!(manager.active_file_operation_ids.len(), 3);
    }

    #[test]
    fn closing_running_file_operation_removes_row_and_cancels_token() {
        let (mut manager, _) = FileManager::new();
        let mut operation = operation_with_progress(FileOperationStatus::Running, 25, 100);
        let cancel_token = operation.cancel_token.clone();
        operation.id = 42;

        manager.file_operations.insert(operation.id, operation);
        manager.active_file_operation_ids.insert(42);
        manager.file_operations_expanded = true;

        let _ = manager.close_file_operation(42);

        assert!(cancel_token.is_cancelled());
        assert!(!manager.file_operations.contains_key(&42));
        assert!(manager.active_file_operation_ids.contains(&42));
        assert!(manager.dismissed_file_operations.contains(&42));
        assert!(!manager.file_operations_expanded);
    }
}
