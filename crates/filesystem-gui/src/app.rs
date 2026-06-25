use crate::apps::{apps_for_mime, best_app_for_mime};
use crate::components::*;
use crate::config::*;
use crate::model::*;
use crate::style;
use crate::tasks::*;
use crate::utils::*;
use filesystem_core::{
    ClipboardPaths, EntryKind, FileProperties, FolderProperties, FsError, PasteAction, ScanOptions,
    SymlinkTargetKind, child_path_limits, file_properties, folder_properties,
    parse_clipboard_paths, paste_paths, rename_entry, set_permissions,
};
use filesystem_mime::{MimeInfo, detect_name};
use iced::alignment::{Horizontal, Vertical};
use iced::widget::{
    button, checkbox, column, container, mouse_area, operation, row, scrollable, space, stack,
    text, text_editor, text_input,
};
use iced::{
    Element, Fill, Length, Padding, Point, Rectangle, Size, Subscription, Task, mouse, window,
};
use std::cmp::Ordering;
use std::collections::{BTreeMap, BTreeSet};
use std::path::PathBuf;

pub(crate) struct FileManager {
    cwd: PathBuf,
    entries: Vec<DisplayEntry>,
    show_hidden: bool,
    status: String,
    path_input: String,
    search_query: Option<String>,
    search_root: Option<PathBuf>,
    view_mode: ViewMode,
    menu_open: bool,
    view_submenu_open: bool,
    context_menu: Option<ContextMenuState>,
    clipboard: Option<ClipboardState>,
    delete_confirm: Option<DeleteConfirm>,
    alert_dialog: Option<AlertDialog>,
    rename_state: Option<RenameState>,
    properties_dialog: Option<PropertiesDialog>,
    properties_drag: Option<PropertiesDrag>,
    properties_pointer: Option<Point>,
    selected_paths: BTreeSet<PathBuf>,
    selection_drag: Option<SelectionDrag>,
    browser_pointer: Option<Point>,
    browser_scroll_y: f32,
    browser_width: f32,
    window_size: Size,
    next_request_id: u64,
    active_request_id: Option<u64>,
    back_history: Vec<PathBuf>,
    forward_history: Vec<PathBuf>,
    home: Option<PathBuf>,
    home_shortcuts: Vec<HomeShortcut>,
    app_registry: AppRegistry,
    open_with_dialog: Option<OpenWithDialog>,
}

impl FileManager {
    pub(crate) fn title(&self) -> String {
        format!("{APP_NAME_EN} - {}", self.cwd.display())
    }

    pub(crate) fn new() -> (Self, Task<Message>) {
        let cwd = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("/"));
        let home = std::env::var_os("HOME").map(PathBuf::from);
        let shortcuts_task = load_home_shortcuts(home.clone());
        let app_registry_task = load_app_registry_task();
        let mut manager = Self {
            cwd,
            entries: Vec::new(),
            show_hidden: false,
            status: String::new(),
            path_input: String::new(),
            search_query: None,
            search_root: None,
            view_mode: ViewMode::Icons,
            menu_open: false,
            view_submenu_open: false,
            context_menu: None,
            clipboard: None,
            delete_confirm: None,
            alert_dialog: None,
            rename_state: None,
            properties_dialog: None,
            properties_drag: None,
            properties_pointer: None,
            selected_paths: BTreeSet::new(),
            selection_drag: None,
            browser_pointer: None,
            browser_scroll_y: 0.0,
            browser_width: browser_width_from_window(WINDOW_INITIAL_WIDTH),
            window_size: Size::new(WINDOW_INITIAL_WIDTH, WINDOW_INITIAL_HEIGHT),
            next_request_id: 0,
            active_request_id: None,
            back_history: Vec::new(),
            forward_history: Vec::new(),
            home,
            home_shortcuts: Vec::new(),
            app_registry: AppRegistry::default(),
            open_with_dialog: None,
        };
        let task = manager.reload();
        (
            manager,
            Task::batch([task, shortcuts_task, app_registry_task]),
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
                self.selection_drag = None;
                self.selected_paths.clear();
                self.selected_paths.insert(path);
                Task::none()
            }
            Message::BrowserPointerMoved(position) => {
                self.browser_pointer = Some(position);
                if let Some(drag) = &mut self.selection_drag {
                    drag.current = position;
                    self.select_entries_in_drag_rect();
                }
                Task::none()
            }
            Message::BrowserPressed => {
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
                }
                Task::none()
            }
            Message::BrowserReleased => {
                if self.selection_drag.is_some() {
                    self.select_entries_in_drag_rect();
                    self.selection_drag = None;
                }
                Task::none()
            }
            Message::BrowserRightPressed => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.menu_open = false;
                self.view_submenu_open = false;
                self.selection_drag = None;

                self.context_menu = self.browser_pointer.and_then(|position| {
                    if let Some(path) = self.folder_menu_path_at(position) {
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

                Task::none()
            }
            Message::BrowserScrolled(offset_y) => {
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
                Task::none()
            }
            Message::ContextOpenTerminal => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.status = "Opening terminal...".to_string();
                let cwd = self.cwd.clone();
                Task::perform(async move { open_terminal(cwd) }, Message::TerminalOpened)
            }
            Message::ContextProperties => {
                if self.rename_state.is_some() {
                    return self.submit_rename();
                }
                self.close_menu();
                self.open_properties(self.cwd.clone())
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
                self.close_menu();
                self.delete_confirm = Some(DeleteConfirm {
                    name: display_name_for_path(&path),
                    kind_label: "文件夹",
                    path,
                });
                Task::none()
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
                Task::perform(async move { open_terminal(path) }, Message::TerminalOpened)
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
                self.close_menu();
                self.delete_confirm = Some(DeleteConfirm {
                    name: display_name_for_path(&path),
                    kind_label: "文件",
                    path,
                });
                Task::none()
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
            Message::ConfirmDelete(path) => {
                self.delete_confirm = None;
                self.status = format!("Deleting {}...", display_name_for_path(&path));
                delete_entry_task(path)
            }
            Message::DeleteFinished(result) => {
                match result {
                    Ok(path) => {
                        self.selected_paths.remove(&path);
                        if self.clipboard.as_ref().is_some_and(|clipboard| {
                            clipboard.paths.iter().any(|item| item == &path)
                        }) {
                            self.clipboard = None;
                        }
                        self.status = format!("Deleted {}", display_name_for_path(&path));
                        self.reload()
                    }
                    Err(error) => {
                        self.status = error.to_string();
                        Task::none()
                    }
                }
            }
            Message::ApplicationsLoaded(registry) => {
                self.app_registry = registry;
                Task::none()
            }
            Message::OpenWithSelect(app_id) => {
                if let Some(dialog) = &mut self.open_with_dialog {
                    dialog.selected_app_id = Some(app_id);
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

                self.status = format!("Opening with {}...", app.name);
                open_file_with_app_task(dialog.path, app)
            }
            Message::OpenFileFinished(result) => {
                self.status = match result {
                    Ok(app_name) => format!("Opened with {app_name}"),
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
                    self.selected_paths.insert(path);
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
                        self.selected_paths.insert(rename.path);
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
            Message::PasteFinished(result) => match result {
                Ok(paths) => {
                    if self
                        .clipboard
                        .as_ref()
                        .is_some_and(|clipboard| clipboard.action == PasteAction::Cut)
                    {
                        self.clipboard = None;
                    }
                    self.status = format!("Pasted {} item(s)", paths.len());
                    self.reload()
                }
                Err(error) => {
                    self.status = error.to_string();
                    Task::none()
                }
            },
            Message::TerminalOpened(result) => {
                self.status = match result {
                    Ok(name) => format!("Opened {name}"),
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
        window::resize_events().map(|(_id, size)| Message::WindowResized(size))
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
        let top_drag = mouse_area(
            container(text(APP_NAME_ZH).size(16).style(style::primary_text))
                .height(TOOLBAR_HEIGHT)
                .width(Fill)
                .align_x(Horizontal::Center)
                .align_y(Vertical::Center)
                .style(style::sidebar_drag_area),
        )
        .on_press(Message::WindowDrag)
        .on_double_click(Message::WindowToggleMaximize);

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

        let sidebar = column![top_drag, container(content).height(Fill).width(Fill)].spacing(0);

        container(sidebar)
            .width(SIDEBAR_WIDTH)
            .height(Fill)
            .style(style::sidebar)
            .into()
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
        let content = match self.view_mode {
            ViewMode::Icons => self.grid(),
            ViewMode::List => self.list(),
        };

        let content = mouse_area(content)
            .on_move(Message::BrowserPointerMoved)
            .on_press(Message::BrowserPressed)
            .on_release(Message::BrowserReleased)
            .on_right_press(Message::BrowserRightPressed);

        stack([
            content.into(),
            self.selection_overlay(),
            self.rename_overlay(),
            self.context_menu_overlay(),
        ])
        .height(Fill)
        .width(Fill)
        .into()
    }

    fn grid(&self) -> Element<'_, Message> {
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
                .on_scroll(|viewport| Message::BrowserScrolled(viewport.absolute_offset().y))
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
                .on_scroll(|viewport| Message::BrowserScrolled(viewport.absolute_offset().y))
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
        let name_cell = list_name_cell(
            entry,
            short_list_text(&self.entry_display_name(entry)),
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

        mouse_area(row)
            .interaction(mouse::Interaction::Pointer)
            .on_press(Message::SelectEntry(entry.file.path.clone()))
            .on_double_click(Message::Open(entry.file.path.clone(), entry.file.kind))
            .into()
    }

    fn tile(&self, entry: &DisplayEntry) -> Element<'_, Message> {
        let selected = self.is_selected(entry);
        let cut = self.is_cut_path(&entry.file.path);
        let name: Element<'_, Message> = text(short_name(&entry.file.name))
            .size(15)
            .width(TILE_WIDTH)
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

        mouse_area(tile)
            .interaction(mouse::Interaction::Pointer)
            .on_press(Message::SelectEntry(entry.file.path.clone()))
            .on_double_click(Message::Open(entry.file.path.clone(), entry.file.kind))
            .into()
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
            open_file_with_app_task(path, app)
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
            DirectoryLoadEvent::Started(path) => {
                self.directory_started(request, path);
                Task::none()
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

    fn directory_started(&mut self, request: DirectoryRequest, path: PathBuf) {
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

        self.cwd = path;
        self.entries.clear();
        self.path_input = self.cwd.display().to_string();
        self.search_query = None;
        self.search_root = None;
        self.clear_selection_state();
        self.status = "Loading entries...".to_string();
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

    fn clear_selection_state(&mut self) {
        self.selected_paths.clear();
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

    fn context_menu_position(&self, position: Point) -> Point {
        Point::new(
            self.clamped_overlay_x(position.x, CONTEXT_MENU_WIDTH),
            position.y.max(0.0),
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
            ContextMenuState::Blank(position) => (
                self.context_menu_position(*position),
                self.blank_context_menu(),
            ),
            ContextMenuState::Folder { position, path } => (
                self.context_menu_position(*position),
                self.folder_context_menu(path.clone()),
            ),
            ContextMenuState::File { position, path } => (
                self.context_menu_position(*position),
                self.file_context_menu(path.clone()),
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

    fn blank_context_menu(&self) -> Element<'_, Message> {
        container(
            column![
                context_menu_item("新建文件", Message::ContextNewFile),
                context_menu_item("新建文件夹", Message::ContextNewFolder),
                context_menu_separator(),
                context_menu_item("粘贴", Message::ContextPaste),
                context_menu_item("全选", Message::ContextSelectAll),
                context_menu_separator(),
                context_menu_item("在终端打开", Message::ContextOpenTerminal),
                context_menu_separator(),
                context_menu_item("属性", Message::ContextProperties),
            ]
            .spacing(2)
            .align_x(iced::Alignment::Start)
            .padding(6),
        )
        .width(CONTEXT_MENU_WIDTH)
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
            .spacing(2)
            .align_x(iced::Alignment::Start)
            .padding(6),
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
            .spacing(2)
            .align_x(iced::Alignment::Start)
            .padding(6),
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
                text(format!("删除{}", confirm.kind_label))
                    .size(16)
                    .style(style::primary_text),
                text(format!(
                    "确定删除 {} {}吗？",
                    confirm.name, confirm.kind_label
                ))
                .size(14)
                .style(style::primary_text),
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
                    .on_press(Message::ConfirmDelete(confirm.path.clone())),
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
        let marker = if active { "✓" } else { "" };

        button(
            container(
                row![
                    entry_icon(&app.icon, 24.0),
                    text(app.name.clone()).size(14).style(style::primary_text),
                    space().width(Fill),
                    container(text(marker).size(13).style(style::primary_text))
                        .width(24)
                        .align_x(Horizontal::Center)
                        .align_y(Vertical::Center),
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

    fn properties_loading(&self, path: &PathBuf) -> Element<'_, Message> {
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

        let current_mode = dialog.permissions_mode.or_else(|| match &dialog.state {
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
        })
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
            && !entry
                .file
                .symlink_target
                .as_ref()
                .is_some_and(|target| !target.broken)
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
        self.selected_paths.insert(path);
        self.status = "Enter a new name".to_string();
        self.focus_rename_input(true)
    }

    fn paste_paths(&mut self, paths: Vec<PathBuf>, action: PasteAction) -> Task<Message> {
        let destination = self.cwd.clone();
        self.status = "Pasting...".to_string();
        Task::perform(
            async move { paste_paths(paths, destination, action) },
            Message::PasteFinished,
        )
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
        self.context_menu = None;
    }
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
