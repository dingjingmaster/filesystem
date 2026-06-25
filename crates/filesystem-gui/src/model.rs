use filesystem_core::{
    ChildPathLimits, EntryKind, FileEntry, FolderProperties, FsError, PasteAction,
};
use iced::widget::text_editor;
use iced::{Point, Size, window};
use std::path::PathBuf;

#[derive(Debug, Clone)]
pub(crate) enum Message {
    Open(PathBuf, EntryKind),
    Go(PathBuf),
    Back,
    Forward,
    PathChanged(String),
    PathSubmit,
    SelectEntry(PathBuf),
    BrowserPointerMoved(Point),
    BrowserPressed,
    BrowserReleased,
    BrowserRightPressed,
    BrowserScrolled(f32),
    WindowResized(Size),
    ContextNewFile,
    ContextNewFolder,
    ContextPaste,
    ContextSelectAll,
    ContextOpenTerminal,
    ContextProperties,
    FolderOpen(PathBuf),
    FolderCopy(PathBuf),
    FolderCut(PathBuf),
    FolderRename(PathBuf),
    FolderDelete(PathBuf),
    FolderOpenTerminal(PathBuf),
    FolderProperties(PathBuf),
    CancelDelete,
    ConfirmDelete(PathBuf),
    DeleteFinished(Result<PathBuf, FsError>),
    CreateFinished(NewEntryKind, Result<PathBuf, FsError>),
    RenameEditorAction(text_editor::Action),
    RenameSubmit,
    RenameFinished(Result<PathBuf, FsError>),
    PasteClipboardRead(Option<String>),
    PasteFinished(Result<Vec<PathBuf>, FsError>),
    TerminalOpened(Result<String, String>),
    PropertiesLoaded(Result<FolderProperties, FsError>),
    PropertiesPermissionsSaved(Result<FolderProperties, FsError>),
    CloseProperties,
    SetPropertiesView(PropertiesView),
    PropertiesPointerMoved(Point),
    PropertiesDragStarted,
    PropertiesDragEnded,
    PropertiesEventSink,
    CyclePermission(PermissionClass),
    CancelPermissions,
    ApplyPermissions,
    ToggleHidden(bool),
    ToggleMenu,
    ToggleViewSubmenu,
    SetViewMode(ViewMode),
    DirectoryLoaded(DirectoryRequest, Result<DisplayListing, FsError>),
    SearchFinished(SearchRequest, Result<DisplaySearchResults, FsError>),
    HomeShortcutsLoaded(Vec<HomeShortcut>),
    WindowDrag,
    WindowClose,
    WindowMinimize,
    WindowToggleMaximize,
    WindowResize(window::Direction),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum NavKind {
    Home,
    Root,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum ViewMode {
    Icons,
    List,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum NewEntryKind {
    File,
    Folder,
}

#[derive(Debug, Clone)]
pub(crate) struct RenameState {
    pub(crate) path: PathBuf,
    pub(crate) fallback_name: String,
    pub(crate) value: String,
    pub(crate) content: text_editor::Content,
    pub(crate) limits: ChildPathLimits,
}

impl RenameState {
    pub(crate) fn new(path: PathBuf, value: String, limits: ChildPathLimits) -> Self {
        let mut content = text_editor::Content::with_text(&value);
        content.perform(text_editor::Action::SelectAll);

        Self {
            path,
            fallback_name: value.clone(),
            value,
            content,
            limits,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum PropertiesView {
    Summary,
    Permissions,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum PermissionClass {
    Owner,
    Group,
    Other,
}

impl PermissionClass {
    pub(crate) fn shift(self) -> u8 {
        match self {
            Self::Owner => 6,
            Self::Group => 3,
            Self::Other => 0,
        }
    }
}

#[derive(Debug, Clone)]
pub(crate) enum PropertiesState {
    Loading(PathBuf),
    Loaded(FolderProperties),
    Error(String),
}

#[derive(Debug, Clone)]
pub(crate) struct PropertiesDialog {
    pub(crate) view: PropertiesView,
    pub(crate) state: PropertiesState,
    pub(crate) position: Point,
    pub(crate) permissions_mode: Option<u32>,
    pub(crate) saving_permissions: bool,
    pub(crate) permission_error: Option<String>,
}

#[derive(Debug, Clone, Copy)]
pub(crate) struct PropertiesDrag {
    pub(crate) pointer_origin: Point,
    pub(crate) dialog_origin: Point,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct ClipboardState {
    pub(crate) action: PasteAction,
    pub(crate) paths: Vec<PathBuf>,
}

#[derive(Debug, Clone)]
pub(crate) enum ContextMenuState {
    Blank(Point),
    Folder { position: Point, path: PathBuf },
}

#[derive(Debug, Clone)]
pub(crate) struct DeleteConfirm {
    pub(crate) path: PathBuf,
    pub(crate) name: String,
}

impl ViewMode {
    pub(crate) fn label(self) -> &'static str {
        match self {
            Self::Icons => "图标视图",
            Self::List => "列表视图",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum DirectoryLoadMode {
    Replace,
    Visit,
    Back,
    Forward,
}

#[derive(Debug, Clone)]
pub(crate) struct DirectoryRequest {
    pub(crate) id: u64,
    pub(crate) path: PathBuf,
    pub(crate) mode: DirectoryLoadMode,
    pub(crate) previous: Option<PathBuf>,
}

#[derive(Debug, Clone)]
pub(crate) struct SearchRequest {
    pub(crate) id: u64,
}

#[derive(Debug, Clone)]
pub(crate) struct HomeShortcut {
    pub(crate) icon: &'static [u8],
    pub(crate) label: &'static str,
    pub(crate) path: PathBuf,
}

#[derive(Debug, Clone)]
pub(crate) struct DisplayEntry {
    pub(crate) file: FileEntry,
    pub(crate) icon: EntryIcon,
}

#[derive(Debug, Clone)]
pub(crate) enum EntryIcon {
    Embedded(&'static [u8]),
    Theme(Vec<u8>),
}

#[derive(Debug, Clone)]
pub(crate) struct DisplayListing {
    pub(crate) path: PathBuf,
    pub(crate) entries: Vec<DisplayEntry>,
}

#[derive(Debug, Clone)]
pub(crate) struct DisplaySearchResults {
    pub(crate) root: PathBuf,
    pub(crate) query: String,
    pub(crate) entries: Vec<DisplayEntry>,
}

#[derive(Debug, Clone, Copy)]
pub(crate) struct SelectionDrag {
    pub(crate) origin: Point,
    pub(crate) current: Point,
}
