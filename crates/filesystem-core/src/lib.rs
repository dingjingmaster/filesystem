use regex::Regex;
use std::cmp::Ordering;
use std::error::Error;
use std::fmt;
use std::fs;
use std::io;
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
    let modified = metadata.modified().ok();

    Ok(FileEntry {
        name,
        path: entry_path,
        kind,
        hidden,
        size,
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
