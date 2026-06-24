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

impl Default for ScanOptions {
    fn default() -> Self {
        Self { show_hidden: false }
    }
}

#[derive(Debug)]
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
        let entry_path = item.path();
        let name = item.file_name().to_string_lossy().into_owned();
        let hidden = name.starts_with('.');

        if hidden && !options.show_hidden {
            continue;
        }

        let metadata = fs::symlink_metadata(&entry_path)
            .map_err(|error| FsError::from_io(&entry_path, error))?;
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

        entries.push(FileEntry {
            name,
            path: entry_path,
            kind,
            hidden,
            size,
            modified,
        });
    }

    entries.sort_by(compare_entries);

    Ok(DirectoryListing {
        path: path.to_path_buf(),
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
}

fn compare_entries(left: &FileEntry, right: &FileEntry) -> Ordering {
    entry_group(left.kind)
        .cmp(&entry_group(right.kind))
        .then_with(|| left.name.to_lowercase().cmp(&right.name.to_lowercase()))
        .then_with(|| left.name.cmp(&right.name))
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

    fn names(listing: &DirectoryListing) -> Vec<&str> {
        listing
            .entries
            .iter()
            .map(|entry| entry.name.as_str())
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
