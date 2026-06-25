use regex::Regex;
use std::cmp::Ordering;
use std::error::Error;
use std::ffi::CString;
use std::fmt;
use std::fs::{self, OpenOptions};
use std::io;
use std::os::unix::ffi::OsStrExt;
use std::os::unix::fs::MetadataExt;
use std::path::{Path, PathBuf};
use std::time::SystemTime;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum EntryKind {
    Directory,
    File,
    Symlink,
    Other,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FileEntry {
    pub name: String,
    pub path: PathBuf,
    pub kind: EntryKind,
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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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

impl Default for ScanOptions {
    fn default() -> Self {
        Self { show_hidden: false }
    }
}

#[derive(Debug, Clone)]
pub struct FsError {
    path: PathBuf,
    kind: io::ErrorKind,
    message: String,
}

impl FsError {
    pub fn path(&self) -> &Path {
        &self.path
    }

    pub fn kind(&self) -> io::ErrorKind {
        self.kind
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

pub fn paste_paths(
    sources: impl IntoIterator<Item = PathBuf>,
    destination: impl AsRef<Path>,
    action: PasteAction,
) -> Result<Vec<PathBuf>, FsError> {
    let destination = destination.as_ref();
    ensure_directory(destination)?;

    let mut pasted = Vec::new();

    for source in sources {
        let name = source.file_name().ok_or_else(|| {
            FsError::from_message(&source, io::ErrorKind::InvalidInput, "missing file name")
        })?;
        let target = unique_child_path(destination, &name.to_string_lossy())?;

        match action {
            PasteAction::Copy => copy_entry(&source, &target)?,
            PasteAction::Cut => {
                fs::rename(&source, &target).map_err(|error| FsError::from_io(&source, error))?;
            }
        }

        pasted.push(target);
    }

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
    let metadata = fs::symlink_metadata(path).map_err(|error| FsError::from_io(path, error))?;

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

impl FsError {
    fn from_io(path: &Path, error: io::Error) -> Self {
        Self {
            path: path.to_path_buf(),
            kind: error.kind(),
            message: error.to_string(),
        }
    }

    fn from_message(path: &Path, kind: io::ErrorKind, message: impl Into<String>) -> Self {
        Self {
            path: path.to_path_buf(),
            kind,
            message: message.into(),
        }
    }
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
    let size = matches!(kind, EntryKind::File).then_some(metadata.len());
    let owner = Some(metadata.uid());
    let modified = metadata.modified().ok();

    Ok(FileEntry {
        name,
        path: entry_path,
        kind,
        hidden,
        size,
        owner,
        modified,
    })
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
    let preferred_name = validated_child_name(preferred_name)?;
    let preferred_path = Path::new(preferred_name);
    let stem = preferred_path
        .file_stem()
        .and_then(|value| value.to_str())
        .filter(|value| !value.is_empty())
        .unwrap_or(preferred_name);
    let extension = preferred_path.extension().and_then(|value| value.to_str());
    let initial = parent.join(preferred_name);

    if !initial.exists() {
        return Ok(initial);
    }

    for index in 1..10_000 {
        let name = if let Some(extension) = extension {
            format!("{stem} {index}.{extension}")
        } else {
            format!("{stem} {index}")
        };
        let candidate = parent.join(name);

        if !candidate.exists() {
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

fn copy_entry(source: &Path, target: &Path) -> Result<(), FsError> {
    let metadata = fs::symlink_metadata(source).map_err(|error| FsError::from_io(source, error))?;
    let file_type = metadata.file_type();

    if file_type.is_dir() {
        if target_is_inside_source(source, target) {
            return Err(FsError::from_message(
                target,
                io::ErrorKind::InvalidInput,
                "cannot copy a directory into itself",
            ));
        }

        fs::create_dir(target).map_err(|error| FsError::from_io(target, error))?;

        for child in fs::read_dir(source).map_err(|error| FsError::from_io(source, error))? {
            let child = child.map_err(|error| FsError::from_io(source, error))?;
            let child_target = target.join(child.file_name());
            copy_entry(&child.path(), &child_target)?;
        }
    } else if file_type.is_file() {
        fs::copy(source, target).map_err(|error| FsError::from_io(source, error))?;
    } else if file_type.is_symlink() {
        let link_target = fs::read_link(source).map_err(|error| FsError::from_io(source, error))?;
        std::os::unix::fs::symlink(link_target, target)
            .map_err(|error| FsError::from_io(target, error))?;
    } else {
        return Err(FsError::from_message(
            source,
            io::ErrorKind::Unsupported,
            "unsupported file type",
        ));
    }

    Ok(())
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
    let path = CString::new(path.as_os_str().as_bytes()).map_err(|_| {
        FsError::from_message(path, io::ErrorKind::InvalidInput, "path contains NUL byte")
    })?;
    let mut stats = std::mem::MaybeUninit::<libc::statvfs>::uninit();
    let result = unsafe { libc::statvfs(path.as_ptr(), stats.as_mut_ptr()) };

    if result != 0 {
        return Err(FsError::from_io(
            Path::new(path.to_str().unwrap_or("")),
            io::Error::last_os_error(),
        ));
    }

    let stats = unsafe { stats.assume_init() };
    Ok(stats.f_bavail as u64 * stats.f_frsize as u64)
}

fn compare_entries(left: &FileEntry, right: &FileEntry) -> Ordering {
    entry_group(left.kind)
        .cmp(&entry_group(right.kind))
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

fn entry_group(kind: EntryKind) -> u8 {
    match kind {
        EntryKind::Directory => 0,
        EntryKind::Symlink => 1,
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
        assert_eq!(link.size, None);
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
