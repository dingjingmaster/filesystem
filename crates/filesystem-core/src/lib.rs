use regex::Regex;
use std::cmp::Ordering;
use std::collections::BTreeSet;
use std::error::Error;
use std::ffi::CString;
use std::fmt;
use std::fs::{self, File, OpenOptions};
use std::io::{self, Read, Write};
use std::os::unix::ffi::OsStrExt;
use std::os::unix::fs::{MetadataExt, PermissionsExt};
use std::path::{Path, PathBuf};
use std::sync::{
    atomic::{AtomicBool, Ordering as AtomicOrdering},
    Arc,
};
use std::time::SystemTime;

const COPY_BUFFER_SIZE: usize = 1024 * 1024;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EntryKind {
    Directory,
    File,
    Symlink,
    Other,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SymlinkTargetKind {
    Directory,
    File,
    Other,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SymlinkTarget {
    pub kind: SymlinkTargetKind,
    pub broken: bool,
    pub path: Option<PathBuf>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FileEntry {
    pub name: String,
    pub path: PathBuf,
    pub kind: EntryKind,
    pub symlink_target: Option<SymlinkTarget>,
    pub hidden: bool,
    pub size: Option<u64>,
    pub owner: Option<u32>,
    pub modified: Option<SystemTime>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DirectoryListing {
    pub path: PathBuf,
    pub entries: Vec<FileEntry>,
}

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct ScanOptions {
    pub show_hidden: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SearchResults {
    pub root: PathBuf,
    pub query: String,
    pub entries: Vec<FileEntry>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PasteAction {
    Copy,
    Cut,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ClipboardPaths {
    pub action: PasteAction,
    pub paths: Vec<PathBuf>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PasteProgressPhase {
    Preparing,
    Renaming,
    Copying,
    DeletingSource,
    Finished,
    Cancelled,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PasteProgress {
    pub action: PasteAction,
    pub phase: PasteProgressPhase,
    pub total_bytes: u64,
    pub completed_bytes: u64,
    pub total_items: u64,
    pub completed_items: u64,
    pub current_source: Option<PathBuf>,
    pub current_target: Option<PathBuf>,
    pub current_bytes: u64,
    pub current_total_bytes: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PasteProgressEvent {
    Started(PasteProgress),
    Progress(PasteProgress),
    Finished {
        targets: Vec<PathBuf>,
        progress: PasteProgress,
    },
    Cancelled {
        targets: Vec<PathBuf>,
        progress: PasteProgress,
    },
}

#[derive(Debug, Clone, Default)]
pub struct FileOperationCancelToken {
    cancelled: Arc<AtomicBool>,
}

impl FileOperationCancelToken {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn cancel(&self) {
        self.cancelled.store(true, AtomicOrdering::SeqCst);
    }

    pub fn is_cancelled(&self) -> bool {
        self.cancelled.load(AtomicOrdering::SeqCst)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FolderProperties {
    pub path: PathBuf,
    pub name: String,
    pub parent: Option<PathBuf>,
    pub item_count: u64,
    pub total_size: u64,
    pub free_space: Option<u64>,
    pub owner: Option<u32>,
    pub group: Option<u32>,
    pub mode: Option<u32>,
    pub modified: Option<SystemTime>,
    pub created: Option<SystemTime>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FileProperties {
    pub path: PathBuf,
    pub name: String,
    pub parent: Option<PathBuf>,
    pub size: u64,
    pub owner: Option<u32>,
    pub group: Option<u32>,
    pub mode: Option<u32>,
    pub accessed: Option<SystemTime>,
    pub modified: Option<SystemTime>,
    pub created: Option<SystemTime>,
}

#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct ChildPathLimits {
    pub name_bytes: Option<usize>,
    pub path_bytes: Option<usize>,
}

#[derive(Debug, Clone)]
pub struct FsError {
    path: PathBuf,
    kind: io::ErrorKind,
    message: String,
    raw_os_error: Option<i32>,
}

impl FsError {
    pub fn path(&self) -> &Path {
        &self.path
    }

    pub fn kind(&self) -> io::ErrorKind {
        self.kind
    }

    pub fn raw_os_error(&self) -> Option<i32> {
        self.raw_os_error
    }

    pub fn is_name_too_long(&self) -> bool {
        self.raw_os_error == Some(libc::ENAMETOOLONG)
    }

    pub fn is_cancelled(&self) -> bool {
        self.kind == io::ErrorKind::Interrupted
    }
}

impl fmt::Display for FsError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(formatter, "{}: {}", self.path.display(), self.message)
    }
}

impl Error for FsError {}

pub fn scan_dir(path: impl AsRef<Path>, options: ScanOptions) -> Result<DirectoryListing, FsError> {
    let path = path.as_ref();
    let mut entries = Vec::new();

    let reader = fs::read_dir(path).map_err(|error| FsError::from_io(path, error))?;

    for item in reader {
        let item = item.map_err(|error| FsError::from_io(path, error))?;
        let entry = file_entry(item)?;

        if entry.hidden && !options.show_hidden {
            continue;
        }

        entries.push(entry);
    }

    entries.sort_by(compare_entries);

    Ok(DirectoryListing {
        path: path.to_path_buf(),
        entries,
    })
}

pub struct DirectoryScanner {
    path: PathBuf,
    reader: fs::ReadDir,
    options: ScanOptions,
    batch_size: usize,
    count: usize,
}

impl DirectoryScanner {
    pub fn new(
        path: impl AsRef<Path>,
        options: ScanOptions,
        batch_size: usize,
    ) -> Result<Self, FsError> {
        let path = path.as_ref();
        let reader = fs::read_dir(path).map_err(|error| FsError::from_io(path, error))?;

        Ok(Self {
            path: path.to_path_buf(),
            reader,
            options,
            batch_size: batch_size.max(1),
            count: 0,
        })
    }

    pub fn path(&self) -> &Path {
        &self.path
    }

    pub fn count(&self) -> usize {
        self.count
    }

    pub fn next_batch(&mut self) -> Result<Option<Vec<FileEntry>>, FsError> {
        let mut batch = Vec::with_capacity(self.batch_size);

        while batch.len() < self.batch_size {
            let Some(item) = self.reader.next() else {
                break;
            };
            let item = item.map_err(|error| FsError::from_io(&self.path, error))?;
            let entry = file_entry(item)?;

            if entry.hidden && !self.options.show_hidden {
                continue;
            }

            batch.push(entry);
            self.count += 1;
        }

        if batch.is_empty() {
            Ok(None)
        } else {
            Ok(Some(batch))
        }
    }
}

pub fn scan_dir_incremental(
    path: impl AsRef<Path>,
    options: ScanOptions,
    batch_size: usize,
    mut on_start: impl FnMut(PathBuf),
    mut on_batch: impl FnMut(Vec<FileEntry>),
) -> Result<usize, FsError> {
    let mut scanner = DirectoryScanner::new(path, options, batch_size)?;

    on_start(scanner.path().to_path_buf());

    while let Some(batch) = scanner.next_batch()? {
        on_batch(batch);
    }

    Ok(scanner.count())
}

pub fn search_file_names(
    root: impl AsRef<Path>,
    query: impl AsRef<str>,
    options: ScanOptions,
) -> Result<SearchResults, FsError> {
    let root = root.as_ref();
    let query = query.as_ref().trim();
    if query.is_empty() {
        return Ok(SearchResults {
            root: root.to_path_buf(),
            query: String::new(),
            entries: Vec::new(),
        });
    }

    let pattern = Regex::new(query).map_err(|error| {
        FsError::from_message(root, io::ErrorKind::InvalidInput, error.to_string())
    })?;
    let mut entries = Vec::new();

    search_file_names_inner(root, &pattern, options, &mut entries)?;
    entries.sort_by(compare_entry_paths);

    Ok(SearchResults {
        root: root.to_path_buf(),
        query: query.to_string(),
        entries,
    })
}

pub fn create_file(
    parent: impl AsRef<Path>,
    preferred_name: impl AsRef<str>,
) -> Result<PathBuf, FsError> {
    let parent = parent.as_ref();
    ensure_directory(parent)?;
    let path = unique_child_path(parent, preferred_name.as_ref())?;

    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&path)
        .map_err(|error| FsError::from_io(&path, error))?;

    Ok(path)
}

pub fn create_folder(
    parent: impl AsRef<Path>,
    preferred_name: impl AsRef<str>,
) -> Result<PathBuf, FsError> {
    let parent = parent.as_ref();
    ensure_directory(parent)?;
    let path = unique_child_path(parent, preferred_name.as_ref())?;

    fs::create_dir(&path).map_err(|error| FsError::from_io(&path, error))?;

    Ok(path)
}

pub fn create_file_from_template(
    parent: impl AsRef<Path>,
    template: impl AsRef<Path>,
) -> Result<PathBuf, FsError> {
    let parent = parent.as_ref();
    let template = template.as_ref();
    ensure_directory(parent)?;

    let metadata = fs::metadata(template).map_err(|error| FsError::from_io(template, error))?;
    if !metadata.is_file() {
        return Err(FsError::from_message(
            template,
            io::ErrorKind::InvalidInput,
            "template is not a file",
        ));
    }

    let name = template
        .file_name()
        .ok_or_else(|| {
            FsError::from_message(
                template,
                io::ErrorKind::InvalidInput,
                "missing template name",
            )
        })?
        .to_string_lossy();
    let path = unique_child_path(parent, &name)?;
    let mut reader = File::open(template).map_err(|error| FsError::from_io(template, error))?;
    let mut writer = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&path)
        .map_err(|error| FsError::from_io(&path, error))?;

    if let Err(error) = io::copy(&mut reader, &mut writer) {
        drop(writer);
        cleanup_partial_target(&path);
        return Err(FsError::from_io(&path, error));
    }
    drop(writer);

    if let Err(error) = fs::set_permissions(&path, metadata.permissions()) {
        cleanup_partial_target(&path);
        return Err(FsError::from_io(&path, error));
    }

    Ok(path)
}

pub fn rename_entry(path: impl AsRef<Path>, new_name: impl AsRef<str>) -> Result<PathBuf, FsError> {
    let path = path.as_ref();
    let new_name = validated_child_name(new_name.as_ref())?;
    let parent = path.parent().ok_or_else(|| {
        FsError::from_message(path, io::ErrorKind::InvalidInput, "missing parent")
    })?;
    let target = parent.join(new_name);

    if target == path {
        return Ok(target);
    }

    if target.exists() {
        return Err(FsError::from_message(
            &target,
            io::ErrorKind::AlreadyExists,
            "target already exists",
        ));
    }

    fs::rename(path, &target).map_err(|error| FsError::from_io(path, error))?;

    Ok(target)
}

pub fn child_path_limits(parent: impl AsRef<Path>) -> Result<ChildPathLimits, FsError> {
    let parent = parent.as_ref();
    let c_path = path_to_cstring(parent)?;

    Ok(ChildPathLimits {
        name_bytes: pathconf_limit(&c_path, libc::_PC_NAME_MAX),
        path_bytes: pathconf_limit(&c_path, libc::_PC_PATH_MAX),
    })
}

pub fn child_name_limit(parent: impl AsRef<Path>) -> Result<Option<usize>, FsError> {
    child_path_limits(parent).map(|limits| limits.name_bytes)
}

pub fn paste_paths(
    sources: impl IntoIterator<Item = PathBuf>,
    destination: impl AsRef<Path>,
    action: PasteAction,
) -> Result<Vec<PathBuf>, FsError> {
    paste_paths_with_progress(
        sources,
        destination,
        action,
        FileOperationCancelToken::new(),
        |_| {},
    )
}

pub fn paste_paths_with_progress(
    sources: impl IntoIterator<Item = PathBuf>,
    destination: impl AsRef<Path>,
    action: PasteAction,
    cancel_token: FileOperationCancelToken,
    mut on_event: impl FnMut(PasteProgressEvent),
) -> Result<Vec<PathBuf>, FsError> {
    let destination = destination.as_ref();
    ensure_directory(destination)?;

    let sources = sources.into_iter().collect::<Vec<_>>();
    let mut progress = PasteProgress::new(action);
    on_event(PasteProgressEvent::Started(progress.clone()));

    let plans = match plan_paste(&sources, destination, &cancel_token) {
        Ok(plans) => plans,
        Err(error) => {
            if error.is_cancelled() {
                progress.phase = PasteProgressPhase::Cancelled;
                on_event(PasteProgressEvent::Cancelled {
                    targets: Vec::new(),
                    progress,
                });
            }
            return Err(error);
        }
    };
    for plan in &plans {
        progress.total_bytes += plan.stats.bytes;
        progress.total_items += plan.stats.items;
    }
    progress.phase = PasteProgressPhase::Preparing;
    on_event(PasteProgressEvent::Progress(progress.clone()));

    let mut pasted = Vec::new();

    for plan in plans {
        match action {
            PasteAction::Copy => {
                copy_planned_entry(&plan, &mut progress, &cancel_token, &mut on_event)?;
            }
            PasteAction::Cut => {
                move_planned_entry(&plan, &mut progress, &cancel_token, &mut on_event)?;
            }
        }

        pasted.push(plan.target);
    }

    progress.phase = PasteProgressPhase::Finished;
    progress.current_source = None;
    progress.current_target = None;
    progress.current_bytes = 0;
    progress.current_total_bytes = 0;
    on_event(PasteProgressEvent::Finished {
        targets: pasted.clone(),
        progress,
    });

    Ok(pasted)
}

pub fn parse_clipboard_paths(contents: &str) -> ClipboardPaths {
    let mut action = PasteAction::Copy;
    let mut paths = Vec::new();

    for raw_line in contents.lines() {
        let line = raw_line.trim();

        if line.is_empty() || line.starts_with('#') {
            continue;
        }

        match line.to_ascii_lowercase().as_str() {
            "copy" => {
                action = PasteAction::Copy;
                continue;
            }
            "cut" | "move" => {
                action = PasteAction::Cut;
                continue;
            }
            _ => {}
        }

        if let Some(path) = parse_clipboard_path(line) {
            paths.push(path);
        }
    }

    ClipboardPaths { action, paths }
}

pub fn folder_properties(path: impl AsRef<Path>) -> Result<FolderProperties, FsError> {
    let path = path.as_ref();
    let metadata = fs::metadata(path).map_err(|error| FsError::from_io(path, error))?;

    if !metadata.is_dir() {
        return Err(FsError::from_message(
            path,
            io::ErrorKind::InvalidInput,
            "path is not a directory",
        ));
    }

    let (item_count, total_size) = directory_stats(path)?;

    Ok(FolderProperties {
        path: path.to_path_buf(),
        name: path
            .file_name()
            .map(|name| name.to_string_lossy().into_owned())
            .unwrap_or_else(|| path.display().to_string()),
        parent: path.parent().map(Path::to_path_buf),
        item_count,
        total_size,
        free_space: free_space(path).ok(),
        owner: Some(metadata.uid()),
        group: Some(metadata.gid()),
        mode: Some(metadata.mode() & 0o7777),
        modified: metadata.modified().ok(),
        created: metadata.created().ok(),
    })
}

pub fn file_properties(path: impl AsRef<Path>) -> Result<FileProperties, FsError> {
    let path = path.as_ref();
    let metadata = fs::metadata(path).map_err(|error| FsError::from_io(path, error))?;

    if metadata.is_dir() {
        return Err(FsError::from_message(
            path,
            io::ErrorKind::InvalidInput,
            "path is a directory",
        ));
    }

    Ok(FileProperties {
        path: path.to_path_buf(),
        name: path
            .file_name()
            .map(|name| name.to_string_lossy().into_owned())
            .unwrap_or_else(|| path.display().to_string()),
        parent: path.parent().map(Path::to_path_buf),
        size: metadata.len(),
        owner: Some(metadata.uid()),
        group: Some(metadata.gid()),
        mode: Some(metadata.mode() & 0o7777),
        accessed: metadata.accessed().ok(),
        modified: metadata.modified().ok(),
        created: metadata.created().ok(),
    })
}

pub fn set_permissions(path: impl AsRef<Path>, mode: u32) -> Result<(), FsError> {
    let path = path.as_ref();

    if mode > 0o7777 {
        return Err(FsError::from_message(
            path,
            io::ErrorKind::InvalidInput,
            "invalid permission mode",
        ));
    }

    fs::set_permissions(path, fs::Permissions::from_mode(mode))
        .map_err(|error| FsError::from_io(path, error))
}

pub fn delete_entry(path: impl AsRef<Path>) -> Result<(), FsError> {
    let path = path.as_ref();
    let metadata = fs::symlink_metadata(path).map_err(|error| FsError::from_io(path, error))?;

    if metadata.file_type().is_dir() {
        fs::remove_dir_all(path).map_err(|error| FsError::from_io(path, error))
    } else {
        fs::remove_file(path).map_err(|error| FsError::from_io(path, error))
    }
}

impl FsError {
    fn from_io(path: &Path, error: io::Error) -> Self {
        Self {
            path: path.to_path_buf(),
            kind: error.kind(),
            raw_os_error: error.raw_os_error(),
            message: error.to_string(),
        }
    }

    fn from_message(path: &Path, kind: io::ErrorKind, message: impl Into<String>) -> Self {
        Self {
            path: path.to_path_buf(),
            kind,
            raw_os_error: None,
            message: message.into(),
        }
    }
}

impl PasteProgress {
    fn new(action: PasteAction) -> Self {
        Self {
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

    fn begin_item(&mut self, phase: PasteProgressPhase, source: &Path, target: &Path, bytes: u64) {
        self.phase = phase;
        self.current_source = Some(source.to_path_buf());
        self.current_target = Some(target.to_path_buf());
        self.current_bytes = 0;
        self.current_total_bytes = bytes;
    }

    fn finish_item(&mut self) {
        self.completed_items += 1;
        self.current_bytes = self.current_total_bytes;
    }
}

#[derive(Debug, Clone, Copy, Default)]
struct EntryStats {
    bytes: u64,
    items: u64,
}

#[derive(Debug, Clone)]
struct PastePlan {
    source: PathBuf,
    target: PathBuf,
    stats: EntryStats,
}

fn plan_paste(
    sources: &[PathBuf],
    destination: &Path,
    cancel_token: &FileOperationCancelToken,
) -> Result<Vec<PastePlan>, FsError> {
    let mut reserved = BTreeSet::new();
    let mut plans = Vec::with_capacity(sources.len());

    for source in sources {
        check_cancelled(cancel_token, source)?;
        let name = source.file_name().ok_or_else(|| {
            FsError::from_message(source, io::ErrorKind::InvalidInput, "missing file name")
        })?;
        let target = unique_child_path_avoiding(destination, &name.to_string_lossy(), &reserved)?;

        if target_is_inside_source(source, &target) {
            return Err(FsError::from_message(
                &target,
                io::ErrorKind::InvalidInput,
                "cannot copy or move a directory into itself",
            ));
        }

        let stats = entry_stats(source, cancel_token)?;
        reserved.insert(target.clone());
        plans.push(PastePlan {
            source: source.clone(),
            target,
            stats,
        });
    }

    Ok(plans)
}

fn copy_planned_entry(
    plan: &PastePlan,
    progress: &mut PasteProgress,
    cancel_token: &FileOperationCancelToken,
    on_event: &mut impl FnMut(PasteProgressEvent),
) -> Result<(), FsError> {
    match copy_entry_with_progress(&plan.source, &plan.target, progress, cancel_token, on_event) {
        Ok(()) => Ok(()),
        Err(error) => {
            cleanup_partial_target(&plan.target);
            if error.is_cancelled() {
                progress.phase = PasteProgressPhase::Cancelled;
                on_event(PasteProgressEvent::Cancelled {
                    targets: Vec::new(),
                    progress: progress.clone(),
                });
            }
            Err(error)
        }
    }
}

fn move_planned_entry(
    plan: &PastePlan,
    progress: &mut PasteProgress,
    cancel_token: &FileOperationCancelToken,
    on_event: &mut impl FnMut(PasteProgressEvent),
) -> Result<(), FsError> {
    check_cancelled(cancel_token, &plan.source)?;

    progress.begin_item(
        PasteProgressPhase::Renaming,
        &plan.source,
        &plan.target,
        plan.stats.bytes,
    );
    on_event(PasteProgressEvent::Progress(progress.clone()));

    match fs::rename(&plan.source, &plan.target) {
        Ok(()) => {
            progress.completed_bytes += plan.stats.bytes;
            progress.completed_items += plan.stats.items;
            progress.current_bytes = progress.current_total_bytes;
            on_event(PasteProgressEvent::Progress(progress.clone()));
            Ok(())
        }
        Err(error) if is_cross_device_error(&error) => {
            copy_planned_entry(plan, progress, cancel_token, on_event)?;

            check_cancelled(cancel_token, &plan.source)?;
            progress.begin_item(
                PasteProgressPhase::DeletingSource,
                &plan.source,
                &plan.target,
                0,
            );
            on_event(PasteProgressEvent::Progress(progress.clone()));
            delete_entry(&plan.source)?;
            on_event(PasteProgressEvent::Progress(progress.clone()));
            Ok(())
        }
        Err(error) => Err(FsError::from_io(&plan.source, error)),
    }
}

fn entry_stats(
    path: &Path,
    cancel_token: &FileOperationCancelToken,
) -> Result<EntryStats, FsError> {
    check_cancelled(cancel_token, path)?;
    let metadata = fs::symlink_metadata(path).map_err(|error| FsError::from_io(path, error))?;
    let file_type = metadata.file_type();
    let mut stats = EntryStats { bytes: 0, items: 1 };

    if file_type.is_dir() {
        for child in fs::read_dir(path).map_err(|error| FsError::from_io(path, error))? {
            check_cancelled(cancel_token, path)?;
            let child = child.map_err(|error| FsError::from_io(path, error))?;
            let child_stats = entry_stats(&child.path(), cancel_token)?;
            stats.bytes += child_stats.bytes;
            stats.items += child_stats.items;
        }
    } else if file_type.is_file() {
        stats.bytes = metadata.len();
    } else if !file_type.is_symlink() {
        return Err(FsError::from_message(
            path,
            io::ErrorKind::Unsupported,
            "unsupported file type",
        ));
    }

    Ok(stats)
}

fn copy_entry_with_progress(
    source: &Path,
    target: &Path,
    progress: &mut PasteProgress,
    cancel_token: &FileOperationCancelToken,
    on_event: &mut impl FnMut(PasteProgressEvent),
) -> Result<(), FsError> {
    check_cancelled(cancel_token, source)?;

    let metadata = fs::symlink_metadata(source).map_err(|error| FsError::from_io(source, error))?;
    let file_type = metadata.file_type();
    let bytes = if file_type.is_file() {
        metadata.len()
    } else {
        0
    };

    progress.begin_item(PasteProgressPhase::Copying, source, target, bytes);
    on_event(PasteProgressEvent::Progress(progress.clone()));

    if file_type.is_dir() {
        fs::create_dir(target).map_err(|error| FsError::from_io(target, error))?;
        progress.finish_item();
        on_event(PasteProgressEvent::Progress(progress.clone()));

        for child in fs::read_dir(source).map_err(|error| FsError::from_io(source, error))? {
            let child = child.map_err(|error| FsError::from_io(source, error))?;
            copy_entry_with_progress(
                &child.path(),
                &target.join(child.file_name()),
                progress,
                cancel_token,
                on_event,
            )?;
        }
    } else if file_type.is_file() {
        copy_file_with_progress(source, target, &metadata, progress, cancel_token, on_event)?;
    } else if file_type.is_symlink() {
        let link_target = fs::read_link(source).map_err(|error| FsError::from_io(source, error))?;
        std::os::unix::fs::symlink(link_target, target)
            .map_err(|error| FsError::from_io(target, error))?;
        progress.finish_item();
        on_event(PasteProgressEvent::Progress(progress.clone()));
    } else {
        return Err(FsError::from_message(
            source,
            io::ErrorKind::Unsupported,
            "unsupported file type",
        ));
    }

    Ok(())
}

fn copy_file_with_progress(
    source: &Path,
    target: &Path,
    metadata: &fs::Metadata,
    progress: &mut PasteProgress,
    cancel_token: &FileOperationCancelToken,
    on_event: &mut impl FnMut(PasteProgressEvent),
) -> Result<(), FsError> {
    let mut reader = File::open(source).map_err(|error| FsError::from_io(source, error))?;
    let mut writer = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(target)
        .map_err(|error| FsError::from_io(target, error))?;
    let mut buffer = vec![0; COPY_BUFFER_SIZE];

    loop {
        check_cancelled(cancel_token, source)?;
        let read = reader
            .read(&mut buffer)
            .map_err(|error| FsError::from_io(source, error))?;

        if read == 0 {
            break;
        }

        writer
            .write_all(&buffer[..read])
            .map_err(|error| FsError::from_io(target, error))?;
        progress.completed_bytes += read as u64;
        progress.current_bytes += read as u64;
        on_event(PasteProgressEvent::Progress(progress.clone()));
    }

    fs::set_permissions(target, metadata.permissions())
        .map_err(|error| FsError::from_io(target, error))?;
    progress.finish_item();
    on_event(PasteProgressEvent::Progress(progress.clone()));

    Ok(())
}

fn check_cancelled(cancel_token: &FileOperationCancelToken, path: &Path) -> Result<(), FsError> {
    if cancel_token.is_cancelled() {
        Err(FsError::from_message(
            path,
            io::ErrorKind::Interrupted,
            "file operation cancelled",
        ))
    } else {
        Ok(())
    }
}

fn cleanup_partial_target(target: &Path) {
    let Ok(metadata) = fs::symlink_metadata(target) else {
        return;
    };

    let _ = if metadata.file_type().is_dir() {
        fs::remove_dir_all(target)
    } else {
        fs::remove_file(target)
    };
}

fn is_cross_device_error(error: &io::Error) -> bool {
    error.raw_os_error() == Some(libc::EXDEV)
}

fn file_entry(item: fs::DirEntry) -> Result<FileEntry, FsError> {
    let entry_path = item.path();
    let name = item.file_name().to_string_lossy().into_owned();
    let hidden = name.starts_with('.');

    let metadata =
        fs::symlink_metadata(&entry_path).map_err(|error| FsError::from_io(&entry_path, error))?;
    let file_type = metadata.file_type();
    let kind = if file_type.is_dir() {
        EntryKind::Directory
    } else if file_type.is_file() {
        EntryKind::File
    } else if file_type.is_symlink() {
        EntryKind::Symlink
    } else {
        EntryKind::Other
    };
    let symlink_target = matches!(kind, EntryKind::Symlink)
        .then(|| symlink_target(&entry_path))
        .flatten();
    let size = if matches!(kind, EntryKind::File)
        || symlink_target
            .as_ref()
            .is_some_and(|target| target.kind == SymlinkTargetKind::File)
    {
        Some(
            fs::metadata(&entry_path)
                .map(|metadata| metadata.len())
                .unwrap_or_else(|_| metadata.len()),
        )
    } else {
        None
    };
    let owner = Some(metadata.uid());
    let modified = metadata.modified().ok();

    Ok(FileEntry {
        name,
        path: entry_path,
        kind,
        symlink_target,
        hidden,
        size,
        owner,
        modified,
    })
}

fn symlink_target(path: &Path) -> Option<SymlinkTarget> {
    match fs::metadata(path) {
        Ok(metadata) => {
            let file_type = metadata.file_type();
            let kind = if file_type.is_dir() {
                SymlinkTargetKind::Directory
            } else if file_type.is_file() {
                SymlinkTargetKind::File
            } else {
                SymlinkTargetKind::Other
            };

            Some(SymlinkTarget {
                kind,
                broken: false,
                path: fs::canonicalize(path).ok(),
            })
        }
        Err(_) => Some(SymlinkTarget {
            kind: SymlinkTargetKind::Other,
            broken: true,
            path: None,
        }),
    }
}

fn search_file_names_inner(
    dir: &Path,
    pattern: &Regex,
    options: ScanOptions,
    entries: &mut Vec<FileEntry>,
) -> Result<(), FsError> {
    let reader = fs::read_dir(dir).map_err(|error| FsError::from_io(dir, error))?;

    for item in reader {
        let item = item.map_err(|error| FsError::from_io(dir, error))?;
        let entry = file_entry(item)?;

        if entry.hidden && !options.show_hidden {
            continue;
        }

        let should_descend = matches!(entry.kind, EntryKind::Directory);
        if pattern.is_match(&entry.name) {
            entries.push(entry.clone());
        }

        if should_descend {
            search_file_names_inner(&entry.path, pattern, options, entries)?;
        }
    }

    Ok(())
}

fn ensure_directory(path: &Path) -> Result<(), FsError> {
    let metadata = fs::metadata(path).map_err(|error| FsError::from_io(path, error))?;

    if metadata.is_dir() {
        Ok(())
    } else {
        Err(FsError::from_message(
            path,
            io::ErrorKind::InvalidInput,
            "path is not a directory",
        ))
    }
}

fn unique_child_path(parent: &Path, preferred_name: &str) -> Result<PathBuf, FsError> {
    unique_child_path_avoiding(parent, preferred_name, &BTreeSet::new())
}

fn unique_child_path_avoiding(
    parent: &Path,
    preferred_name: &str,
    reserved: &BTreeSet<PathBuf>,
) -> Result<PathBuf, FsError> {
    let preferred_name = validated_child_name(preferred_name)?;
    let preferred_path = Path::new(preferred_name);
    let stem = preferred_path
        .file_stem()
        .and_then(|value| value.to_str())
        .filter(|value| !value.is_empty())
        .unwrap_or(preferred_name);
    let extension = preferred_path.extension().and_then(|value| value.to_str());
    let initial = parent.join(preferred_name);

    if !initial.exists() && !reserved.contains(&initial) {
        return Ok(initial);
    }

    for index in 1..10_000 {
        let name = if let Some(extension) = extension {
            format!("{stem} {index}.{extension}")
        } else {
            format!("{stem} {index}")
        };
        let candidate = parent.join(name);

        if !candidate.exists() && !reserved.contains(&candidate) {
            return Ok(candidate);
        }
    }

    Err(FsError::from_message(
        parent,
        io::ErrorKind::AlreadyExists,
        "unable to find available name",
    ))
}

fn validated_child_name(name: &str) -> Result<&str, FsError> {
    let name = name.trim();

    if name.is_empty() || name == "." || name == ".." || name.contains('/') || name.contains('\0') {
        Err(FsError::from_message(
            Path::new(name),
            io::ErrorKind::InvalidInput,
            "invalid file name",
        ))
    } else {
        Ok(name)
    }
}

fn target_is_inside_source(source: &Path, target: &Path) -> bool {
    let Ok(source) = fs::canonicalize(source) else {
        return false;
    };
    let Some(parent) = target.parent() else {
        return false;
    };
    let Ok(parent) = fs::canonicalize(parent) else {
        return false;
    };

    parent.starts_with(source)
}

fn parse_clipboard_path(line: &str) -> Option<PathBuf> {
    if let Some(uri) = line.strip_prefix("file://") {
        let uri = uri.strip_prefix("localhost").unwrap_or(uri);
        if !uri.starts_with('/') {
            return None;
        }

        return percent_decode(uri).map(PathBuf::from);
    }

    let path = Path::new(line);
    path.is_absolute().then(|| path.to_path_buf())
}

fn percent_decode(value: &str) -> Option<String> {
    let bytes = value.as_bytes();
    let mut decoded = Vec::with_capacity(bytes.len());
    let mut index = 0;

    while index < bytes.len() {
        if bytes[index] == b'%' {
            let high = bytes.get(index + 1).copied().and_then(hex_value)?;
            let low = bytes.get(index + 2).copied().and_then(hex_value)?;
            decoded.push(high << 4 | low);
            index += 3;
        } else {
            decoded.push(bytes[index]);
            index += 1;
        }
    }

    String::from_utf8(decoded).ok()
}

fn hex_value(value: u8) -> Option<u8> {
    match value {
        b'0'..=b'9' => Some(value - b'0'),
        b'a'..=b'f' => Some(value - b'a' + 10),
        b'A'..=b'F' => Some(value - b'A' + 10),
        _ => None,
    }
}

fn directory_stats(path: &Path) -> Result<(u64, u64), FsError> {
    let mut count = 0;
    let mut size = 0;

    for child in fs::read_dir(path).map_err(|error| FsError::from_io(path, error))? {
        let child = child.map_err(|error| FsError::from_io(path, error))?;
        let child_path = child.path();
        let metadata = fs::symlink_metadata(&child_path)
            .map_err(|error| FsError::from_io(&child_path, error))?;
        let file_type = metadata.file_type();

        count += 1;

        if file_type.is_file() {
            size += metadata.len();
        } else if file_type.is_dir() {
            let (child_count, child_size) = directory_stats(&child_path)?;
            count += child_count;
            size += child_size;
        }
    }

    Ok((count, size))
}

fn free_space(path: &Path) -> Result<u64, FsError> {
    let path = path_to_cstring(path)?;
    let mut stats = std::mem::MaybeUninit::<libc::statvfs>::uninit();
    let result = unsafe { libc::statvfs(path.as_ptr(), stats.as_mut_ptr()) };

    if result != 0 {
        return Err(FsError::from_io(
            Path::new(path.to_str().unwrap_or("")),
            io::Error::last_os_error(),
        ));
    }

    let stats = unsafe { stats.assume_init() };
    Ok(stats.f_bavail * stats.f_frsize)
}

fn pathconf_limit(path: &CString, name: libc::c_int) -> Option<usize> {
    let limit = unsafe { libc::pathconf(path.as_ptr(), name) };

    (limit >= 0).then_some(limit as usize)
}

fn path_to_cstring(path: &Path) -> Result<CString, FsError> {
    CString::new(path.as_os_str().as_bytes()).map_err(|_| {
        FsError::from_message(path, io::ErrorKind::InvalidInput, "path contains NUL byte")
    })
}

fn compare_entries(left: &FileEntry, right: &FileEntry) -> Ordering {
    entry_group(left)
        .cmp(&entry_group(right))
        .then_with(|| left.name.to_lowercase().cmp(&right.name.to_lowercase()))
        .then_with(|| left.name.cmp(&right.name))
}

fn compare_entry_paths(left: &FileEntry, right: &FileEntry) -> Ordering {
    left.path
        .to_string_lossy()
        .to_lowercase()
        .cmp(&right.path.to_string_lossy().to_lowercase())
        .then_with(|| left.path.cmp(&right.path))
}

fn entry_group(entry: &FileEntry) -> u8 {
    match entry.kind {
        EntryKind::Directory => 0,
        EntryKind::Symlink => match entry.symlink_target.as_ref() {
            Some(SymlinkTarget {
                kind: SymlinkTargetKind::Directory,
                broken: false,
                ..
            }) => 0,
            Some(SymlinkTarget {
                kind: SymlinkTargetKind::File,
                broken: false,
                ..
            }) => 2,
            _ => 3,
        },
        EntryKind::File => 2,
        EntryKind::Other => 3,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::OsStr;
    use std::os::unix::fs::symlink;

    #[test]
    fn scan_dir_lists_visible_entries_with_directories_first() {
        let fixture = TempDir::new("visible");
        fs::write(fixture.path().join("b.txt"), b"content").unwrap();
        fs::create_dir(fixture.path().join("a-dir")).unwrap();
        fs::write(fixture.path().join(".hidden"), b"hidden").unwrap();

        let listing = scan_dir(fixture.path(), ScanOptions::default()).unwrap();
        let names = names(&listing);

        assert_eq!(names, vec!["a-dir", "b.txt"]);
        assert!(listing.entries.iter().all(|entry| !entry.hidden));
        assert_eq!(listing.entries[0].kind, EntryKind::Directory);
        assert_eq!(listing.entries[1].size, Some(7));
        assert!(listing.entries[1].owner.is_some());
    }

    #[test]
    fn scan_dir_can_include_hidden_entries() {
        let fixture = TempDir::new("hidden");
        fs::write(fixture.path().join(".a"), b"hidden").unwrap();
        fs::write(fixture.path().join("b"), b"visible").unwrap();

        let listing = scan_dir(fixture.path(), ScanOptions { show_hidden: true }).unwrap();
        let names = names(&listing);

        assert_eq!(names, vec![".a", "b"]);
        assert!(listing.entries[0].hidden);
    }

    #[test]
    fn directory_scanner_reads_visible_entries_in_batches() {
        let fixture = TempDir::new("scanner-batches");
        fs::write(fixture.path().join("a.txt"), b"a").unwrap();
        fs::write(fixture.path().join("b.txt"), b"b").unwrap();
        fs::create_dir(fixture.path().join("dir")).unwrap();
        fs::write(fixture.path().join(".hidden"), b"hidden").unwrap();

        let mut scanner = DirectoryScanner::new(fixture.path(), ScanOptions::default(), 2).unwrap();
        let mut names = Vec::new();
        let mut batch_lengths = Vec::new();

        while let Some(batch) = scanner.next_batch().unwrap() {
            batch_lengths.push(batch.len());
            names.extend(batch.into_iter().map(|entry| entry.name));
        }

        names.sort();

        assert_eq!(scanner.path(), fixture.path());
        assert_eq!(scanner.count(), 3);
        assert_eq!(names, vec!["a.txt", "b.txt", "dir"]);
        assert!(batch_lengths.iter().all(|length| *length <= 2));
        assert_eq!(scanner.next_batch().unwrap(), None);
    }

    #[test]
    fn scan_dir_marks_symlinks_without_following_them() {
        let fixture = TempDir::new("symlink");
        fs::write(fixture.path().join("target.txt"), b"target").unwrap();
        symlink("target.txt", fixture.path().join("link.txt")).unwrap();

        let listing = scan_dir(fixture.path(), ScanOptions::default()).unwrap();
        let link = listing
            .entries
            .iter()
            .find(|entry| entry.name == "link.txt")
            .unwrap();

        assert_eq!(link.kind, EntryKind::Symlink);
        assert_eq!(
            link.symlink_target,
            Some(SymlinkTarget {
                kind: SymlinkTargetKind::File,
                broken: false,
                path: fs::canonicalize(fixture.path().join("target.txt")).ok(),
            })
        );
        assert_eq!(link.size, Some(6));
    }

    #[test]
    fn scan_dir_marks_directory_and_broken_symlink_targets() {
        let fixture = TempDir::new("symlink-targets");
        fs::create_dir(fixture.path().join("target-dir")).unwrap();
        symlink("target-dir", fixture.path().join("dir-link")).unwrap();
        symlink("missing", fixture.path().join("broken-link")).unwrap();

        let listing = scan_dir(fixture.path(), ScanOptions::default()).unwrap();
        let dir_link = listing
            .entries
            .iter()
            .find(|entry| entry.name == "dir-link")
            .unwrap();
        let broken_link = listing
            .entries
            .iter()
            .find(|entry| entry.name == "broken-link")
            .unwrap();

        assert_eq!(
            dir_link.symlink_target,
            Some(SymlinkTarget {
                kind: SymlinkTargetKind::Directory,
                broken: false,
                path: fs::canonicalize(fixture.path().join("target-dir")).ok(),
            })
        );
        assert_eq!(
            broken_link.symlink_target,
            Some(SymlinkTarget {
                kind: SymlinkTargetKind::Other,
                broken: true,
                path: None,
            })
        );
        assert_eq!(
            names(&listing),
            vec!["dir-link", "target-dir", "broken-link"]
        );
    }

    #[test]
    fn scan_dir_reports_missing_path() {
        let fixture = TempDir::new("missing");
        let missing = fixture.path().join("missing");

        let error = scan_dir(&missing, ScanOptions::default()).unwrap_err();

        assert_eq!(error.path(), missing.as_path());
        assert_eq!(error.kind(), io::ErrorKind::NotFound);
    }

    #[test]
    fn search_file_names_finds_regex_matches_recursively() {
        let fixture = TempDir::new("search");
        fs::create_dir_all(fixture.path().join("src/nested")).unwrap();
        fs::write(fixture.path().join("src/Alpha.txt"), b"no match").unwrap();
        fs::write(fixture.path().join("src/nested/beta-alpha.md"), b"no match").unwrap();
        fs::write(fixture.path().join("src/nested/gamma.txt"), b"gamma").unwrap();
        fs::write(fixture.path().join("content-only.txt"), b"alpha").unwrap();

        let results = search_file_names(
            fixture.path(),
            "(?i)alpha.*\\.txt$|beta-alpha\\.md$",
            ScanOptions::default(),
        )
        .unwrap();
        let names = result_names(&results);

        assert_eq!(names, vec!["Alpha.txt", "beta-alpha.md"]);
        assert_eq!(results.root, fixture.path());
        assert_eq!(results.query, "(?i)alpha.*\\.txt$|beta-alpha\\.md$");
    }

    #[test]
    fn search_file_names_respects_hidden_option() {
        let fixture = TempDir::new("search-hidden");
        fs::create_dir(fixture.path().join(".hidden-dir")).unwrap();
        fs::write(fixture.path().join(".hidden-dir/needle.txt"), b"hidden").unwrap();
        fs::write(fixture.path().join(".needle"), b"hidden").unwrap();
        fs::write(fixture.path().join("needle.txt"), b"visible").unwrap();

        let visible = search_file_names(fixture.path(), "needle", ScanOptions::default()).unwrap();
        assert_eq!(result_names(&visible), vec!["needle.txt"]);

        let all =
            search_file_names(fixture.path(), "needle", ScanOptions { show_hidden: true }).unwrap();
        assert_eq!(
            result_relative_paths(&all, fixture.path()),
            vec![".hidden-dir/needle.txt", ".needle", "needle.txt"]
        );
    }

    #[test]
    fn search_file_names_ignores_empty_query() {
        let fixture = TempDir::new("search-empty");
        fs::write(fixture.path().join("visible.txt"), b"visible").unwrap();

        let results = search_file_names(fixture.path(), "   ", ScanOptions::default()).unwrap();

        assert!(results.entries.is_empty());
        assert_eq!(results.query, "");
    }

    #[test]
    fn search_file_names_reports_invalid_regex() {
        let fixture = TempDir::new("search-invalid-regex");

        let error = search_file_names(fixture.path(), "[", ScanOptions::default()).unwrap_err();

        assert_eq!(error.kind(), io::ErrorKind::InvalidInput);
    }

    #[test]
    fn create_file_and_folder_use_unique_names() {
        let fixture = TempDir::new("create-unique");
        fs::write(fixture.path().join("新建文件"), b"existing").unwrap();
        fs::create_dir(fixture.path().join("新建文件夹")).unwrap();

        let file = create_file(fixture.path(), "新建文件").unwrap();
        let folder = create_folder(fixture.path(), "新建文件夹").unwrap();

        assert_eq!(file.file_name().and_then(OsStr::to_str), Some("新建文件 1"));
        assert!(file.is_file());
        assert_eq!(
            folder.file_name().and_then(OsStr::to_str),
            Some("新建文件夹 1")
        );
        assert!(folder.is_dir());
    }

    #[test]
    fn create_file_from_template_copies_content_and_uses_unique_name() {
        let fixture = TempDir::new("create-template");
        let templates = fixture.path().join("Templates");
        let destination = fixture.path().join("destination");
        fs::create_dir(&templates).unwrap();
        fs::create_dir(&destination).unwrap();
        let template = templates.join("Report.txt");
        fs::write(&template, b"template content").unwrap();
        fs::write(destination.join("Report.txt"), b"existing").unwrap();

        let created = create_file_from_template(&destination, &template).unwrap();

        assert_eq!(
            created.file_name().and_then(OsStr::to_str),
            Some("Report 1.txt")
        );
        assert_eq!(fs::read(created).unwrap(), b"template content");
    }

    #[test]
    fn create_file_from_template_rejects_directories() {
        let fixture = TempDir::new("create-template-directory");
        let template = fixture.path().join("Folder Template");
        fs::create_dir(&template).unwrap();

        let error = create_file_from_template(fixture.path(), &template).unwrap_err();

        assert_eq!(error.kind(), io::ErrorKind::InvalidInput);
    }

    #[test]
    fn rename_entry_refuses_to_overwrite_existing_path() {
        let fixture = TempDir::new("rename-existing");
        let source = fixture.path().join("source.txt");
        fs::write(&source, b"source").unwrap();
        fs::write(fixture.path().join("target.txt"), b"target").unwrap();

        let error = rename_entry(&source, "target.txt").unwrap_err();

        assert_eq!(error.kind(), io::ErrorKind::AlreadyExists);
        assert_eq!(fs::read_to_string(source).unwrap(), "source");
    }

    #[test]
    fn rename_entry_changes_name_without_changing_parent() {
        let fixture = TempDir::new("rename-ok");
        let source = fixture.path().join("source.txt");
        fs::write(&source, b"source").unwrap();

        let target = rename_entry(&source, "renamed.txt").unwrap();

        assert_eq!(target, fixture.path().join("renamed.txt"));
        assert!(!source.exists());
        assert_eq!(fs::read_to_string(target).unwrap(), "source");
    }

    #[test]
    fn child_name_limit_reports_directory_name_limit() {
        let fixture = TempDir::new("name-limit");

        let limits = child_path_limits(fixture.path()).unwrap();
        let limit = child_name_limit(fixture.path()).unwrap();

        assert_eq!(limits.name_bytes, limit);
        assert!(limits.name_bytes.is_none_or(|limit| limit > 0));
        assert!(limits.path_bytes.is_none_or(|limit| limit > 0));
    }

    #[test]
    fn rename_entry_marks_name_too_long_errors() {
        let fixture = TempDir::new("rename-too-long");
        let source = fixture.path().join("source.txt");
        fs::write(&source, b"source").unwrap();
        let long_name = "a".repeat(4096);

        let error = rename_entry(&source, long_name).unwrap_err();

        assert!(error.is_name_too_long());
        assert!(source.exists());
    }

    #[test]
    fn paste_paths_copies_files_and_directories_recursively() {
        let fixture = TempDir::new("paste-copy");
        let source = fixture.path().join("source");
        let destination = fixture.path().join("destination");
        fs::create_dir(&source).unwrap();
        fs::create_dir(&destination).unwrap();
        fs::write(source.join("a.txt"), b"a").unwrap();
        fs::create_dir(source.join("nested")).unwrap();
        fs::write(source.join("nested/b.txt"), b"b").unwrap();

        let pasted = paste_paths(vec![source.clone()], &destination, PasteAction::Copy).unwrap();

        assert_eq!(pasted.len(), 1);
        assert_eq!(
            fs::read_to_string(destination.join("source/a.txt")).unwrap(),
            "a"
        );
        assert_eq!(
            fs::read_to_string(destination.join("source/nested/b.txt")).unwrap(),
            "b"
        );
        assert!(source.exists());
    }

    #[test]
    fn paste_paths_moves_with_rename() {
        let fixture = TempDir::new("paste-cut");
        let source_dir = fixture.path().join("source");
        let destination = fixture.path().join("destination");
        fs::create_dir(&source_dir).unwrap();
        fs::create_dir(&destination).unwrap();
        let source = source_dir.join("a.txt");
        fs::write(&source, b"a").unwrap();

        let pasted = paste_paths(vec![source.clone()], &destination, PasteAction::Cut).unwrap();

        assert_eq!(pasted, vec![destination.join("a.txt")]);
        assert!(!source.exists());
        assert_eq!(fs::read_to_string(destination.join("a.txt")).unwrap(), "a");
    }

    #[test]
    fn paste_paths_reports_copy_progress() {
        let fixture = TempDir::new("paste-progress");
        let source = fixture.path().join("source.txt");
        let destination = fixture.path().join("destination");
        fs::create_dir(&destination).unwrap();
        fs::write(&source, b"abc").unwrap();
        let mut events = Vec::new();

        let pasted = paste_paths_with_progress(
            vec![source.clone()],
            &destination,
            PasteAction::Copy,
            FileOperationCancelToken::new(),
            |event| events.push(event),
        )
        .unwrap();

        assert_eq!(pasted, vec![destination.join("source.txt")]);
        assert_eq!(fs::read_to_string(&pasted[0]).unwrap(), "abc");
        assert!(matches!(
            events.first(),
            Some(PasteProgressEvent::Started(_))
        ));
        assert!(matches!(
            events.last(),
            Some(PasteProgressEvent::Finished { progress, .. })
                if progress.completed_bytes == 3 && progress.total_bytes == 3
        ));
        assert!(events.iter().any(|event| matches!(
            event,
            PasteProgressEvent::Progress(progress)
                if progress.current_source.as_ref() == Some(&source)
                    && progress.phase == PasteProgressPhase::Copying
        )));
    }

    #[test]
    fn paste_paths_cancel_removes_partial_current_target() {
        let fixture = TempDir::new("paste-cancel");
        let source = fixture.path().join("big.bin");
        let destination = fixture.path().join("destination");
        fs::create_dir(&destination).unwrap();
        fs::write(&source, vec![7; COPY_BUFFER_SIZE * 2 + 17]).unwrap();
        let cancel = FileOperationCancelToken::new();
        let cancel_from_callback = cancel.clone();

        let error = paste_paths_with_progress(
            vec![source.clone()],
            &destination,
            PasteAction::Copy,
            cancel,
            |event| {
                if let PasteProgressEvent::Progress(progress) = event {
                    if progress.completed_bytes > 0 {
                        cancel_from_callback.cancel();
                    }
                }
            },
        )
        .unwrap_err();

        assert!(error.is_cancelled());
        assert!(source.exists());
        assert!(!destination.join("big.bin").exists());
    }

    #[test]
    fn paste_paths_reserves_unique_targets_before_copying() {
        let fixture = TempDir::new("paste-reserved-targets");
        let left = fixture.path().join("left");
        let right = fixture.path().join("right");
        let destination = fixture.path().join("destination");
        fs::create_dir(&left).unwrap();
        fs::create_dir(&right).unwrap();
        fs::create_dir(&destination).unwrap();
        let left_file = left.join("same.txt");
        let right_file = right.join("same.txt");
        fs::write(&left_file, b"left").unwrap();
        fs::write(&right_file, b"right").unwrap();

        let pasted =
            paste_paths(vec![left_file, right_file], &destination, PasteAction::Copy).unwrap();

        assert_eq!(
            pasted,
            vec![destination.join("same.txt"), destination.join("same 1.txt")]
        );
        assert_eq!(
            fs::read_to_string(destination.join("same.txt")).unwrap(),
            "left"
        );
        assert_eq!(
            fs::read_to_string(destination.join("same 1.txt")).unwrap(),
            "right"
        );
    }

    #[test]
    fn recognizes_cross_device_rename_errors() {
        let error = io::Error::from_raw_os_error(libc::EXDEV);

        assert!(is_cross_device_error(&error));
    }

    #[test]
    fn parse_clipboard_paths_reads_action_and_file_uris() {
        let parsed =
            parse_clipboard_paths("cut\nfile:///tmp/a%20file.txt\n# comment\n/tmp/raw-path\n");

        assert_eq!(parsed.action, PasteAction::Cut);
        assert_eq!(
            parsed.paths,
            vec![
                PathBuf::from("/tmp/a file.txt"),
                PathBuf::from("/tmp/raw-path")
            ]
        );
    }

    #[test]
    fn folder_properties_counts_entries_and_sizes() {
        let fixture = TempDir::new("properties");
        fs::write(fixture.path().join("a.txt"), b"1234").unwrap();
        fs::create_dir(fixture.path().join("nested")).unwrap();
        fs::write(fixture.path().join("nested/b.txt"), b"12").unwrap();

        let properties = folder_properties(fixture.path()).unwrap();

        assert_eq!(properties.item_count, 3);
        assert_eq!(properties.total_size, 6);
        assert_eq!(
            properties.owner,
            Some(fs::metadata(fixture.path()).unwrap().uid())
        );
        assert!(properties.free_space.is_some());
    }

    #[test]
    fn folder_properties_follows_directory_symlinks() {
        let fixture = TempDir::new("properties-directory-symlink");
        fs::create_dir(fixture.path().join("target")).unwrap();
        fs::write(fixture.path().join("target/a.txt"), b"123").unwrap();
        symlink("target", fixture.path().join("link")).unwrap();

        let properties = folder_properties(fixture.path().join("link")).unwrap();

        assert_eq!(properties.name, "link");
        assert_eq!(properties.item_count, 1);
        assert_eq!(properties.total_size, 3);
    }

    #[test]
    fn file_properties_reports_file_metadata() {
        let fixture = TempDir::new("file-properties");
        let target = fixture.path().join("a.log");
        fs::write(&target, b"123456").unwrap();

        let properties = file_properties(&target).unwrap();

        assert_eq!(properties.path, target);
        assert_eq!(properties.name, "a.log");
        assert_eq!(properties.parent, Some(fixture.path().to_path_buf()));
        assert_eq!(properties.size, 6);
        assert_eq!(properties.owner, Some(fs::metadata(&target).unwrap().uid()));
        assert!(properties.mode.is_some());
        assert!(properties.modified.is_some());
    }

    #[test]
    fn file_properties_follows_file_symlinks() {
        let fixture = TempDir::new("file-properties-symlink");
        fs::write(fixture.path().join("target.txt"), b"1234567").unwrap();
        symlink("target.txt", fixture.path().join("link.txt")).unwrap();

        let properties = file_properties(fixture.path().join("link.txt")).unwrap();

        assert_eq!(properties.name, "link.txt");
        assert_eq!(properties.size, 7);
    }

    #[test]
    fn file_properties_rejects_directories() {
        let fixture = TempDir::new("file-properties-directory");

        let error = file_properties(fixture.path()).unwrap_err();

        assert_eq!(error.kind(), io::ErrorKind::InvalidInput);
    }

    #[test]
    fn set_permissions_changes_mode() {
        let fixture = TempDir::new("set-permissions");
        let target = fixture.path().join("target");
        fs::create_dir(&target).unwrap();

        set_permissions(&target, 0o750).unwrap();

        assert_eq!(
            fs::metadata(&target).unwrap().permissions().mode() & 0o7777,
            0o750
        );
    }

    #[test]
    fn set_permissions_rejects_invalid_modes() {
        let fixture = TempDir::new("set-permissions-invalid");
        let target = fixture.path().join("target");
        fs::create_dir(&target).unwrap();

        let error = set_permissions(&target, 0o10000).unwrap_err();

        assert_eq!(error.kind(), io::ErrorKind::InvalidInput);
    }

    #[test]
    fn delete_entry_removes_directories_recursively() {
        let fixture = TempDir::new("delete-directory");
        let target = fixture.path().join("target");
        fs::create_dir(&target).unwrap();
        fs::write(target.join("a.txt"), b"a").unwrap();
        fs::create_dir(target.join("nested")).unwrap();
        fs::write(target.join("nested/b.txt"), b"b").unwrap();

        delete_entry(&target).unwrap();

        assert!(!target.exists());
        assert!(fixture.path().exists());
    }

    #[test]
    fn delete_entry_removes_symlink_without_deleting_target() {
        let fixture = TempDir::new("delete-symlink");
        let target = fixture.path().join("target");
        let link = fixture.path().join("link");
        fs::create_dir(&target).unwrap();
        fs::write(target.join("kept.txt"), b"kept").unwrap();
        symlink(&target, &link).unwrap();

        delete_entry(&link).unwrap();

        assert!(!link.exists());
        assert_eq!(fs::read_to_string(target.join("kept.txt")).unwrap(), "kept");
    }

    fn names(listing: &DirectoryListing) -> Vec<&str> {
        listing
            .entries
            .iter()
            .map(|entry| entry.name.as_str())
            .collect()
    }

    fn result_names(results: &SearchResults) -> Vec<&str> {
        results
            .entries
            .iter()
            .map(|entry| entry.name.as_str())
            .collect()
    }

    fn result_relative_paths(results: &SearchResults, root: &Path) -> Vec<String> {
        results
            .entries
            .iter()
            .map(|entry| {
                entry
                    .path
                    .strip_prefix(root)
                    .unwrap()
                    .to_string_lossy()
                    .into_owned()
            })
            .collect()
    }

    struct TempDir {
        path: PathBuf,
    }

    impl TempDir {
        fn new(label: &str) -> Self {
            let unique = format!(
                "filesystem-core-test-{label}-{}-{}",
                std::process::id(),
                SystemTime::now()
                    .duration_since(SystemTime::UNIX_EPOCH)
                    .unwrap()
                    .as_nanos()
            );
            let path = std::env::temp_dir().join(unique);
            fs::create_dir(&path).unwrap();
            Self { path }
        }

        fn path(&self) -> &Path {
            &self.path
        }
    }

    impl Drop for TempDir {
        fn drop(&mut self) {
            if self
                .path
                .file_name()
                .and_then(OsStr::to_str)
                .is_some_and(|name| name.starts_with("filesystem-core-test-"))
            {
                let _ = fs::remove_dir_all(&self.path);
            }
        }
    }
}
