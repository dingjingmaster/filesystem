use crate::model::{DisplayEntry, EntryIcon};
use filesystem_core::{EntryKind, FileEntry};
use filesystem_mime::{MimeInfo, detect_path};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

pub(crate) fn decorate_entries(entries: Vec<FileEntry>) -> Vec<DisplayEntry> {
    let resolver = IconResolver::new();

    entries
        .into_iter()
        .map(|file| {
            let mime = entry_mime(&file);
            let icon = resolver.entry_icon(&file, &mime);
            DisplayEntry { file, mime, icon }
        })
        .collect()
}

pub(crate) fn resolve_app_icon(icon_name: Option<&str>) -> EntryIcon {
    IconResolver::new().app_icon(icon_name)
}

struct IconResolver {
    roots: Vec<PathBuf>,
    themes: Vec<String>,
}

impl IconResolver {
    fn new() -> Self {
        let roots = icon_roots();
        let themes = icon_theme_names(&roots);

        Self { roots, themes }
    }

    fn entry_icon(&self, entry: &FileEntry, mime: &MimeInfo) -> EntryIcon {
        match entry.kind {
            EntryKind::Directory => {
                EntryIcon::Embedded(include_bytes!("../../../icons/folder.svg"))
            }
            EntryKind::File => self
                .file_icon(mime)
                .map(EntryIcon::Theme)
                .unwrap_or_else(fallback_file_icon),
            EntryKind::Symlink | EntryKind::Other => fallback_file_icon(),
        }
    }

    fn app_icon(&self, icon_name: Option<&str>) -> EntryIcon {
        icon_name
            .and_then(|icon_name| self.find_app_icon(icon_name))
            .map(EntryIcon::Theme)
            .unwrap_or_else(fallback_app_icon)
    }

    fn file_icon(&self, mime: &MimeInfo) -> Option<Vec<u8>> {
        let names = icon_names_for_mime(&mime.mime);

        if names.is_empty() {
            return None;
        }

        self.find_icon(names)
    }

    fn find_icon(&self, names: &[&str]) -> Option<Vec<u8>> {
        for theme in &self.themes {
            for root in &self.roots {
                let theme_dir = root.join(theme);

                for directory in ICON_THEME_SUBDIRS {
                    let directory = theme_dir.join(directory);

                    for name in names {
                        for filename in icon_filenames(name) {
                            let path = directory.join(filename);
                            if path.is_file() {
                                if let Ok(bytes) = fs::read(path) {
                                    return Some(bytes);
                                }
                            }
                        }
                    }
                }
            }
        }

        None
    }

    fn find_app_icon(&self, icon_name: &str) -> Option<Vec<u8>> {
        let icon_name = icon_name.trim();
        if icon_name.is_empty() {
            return None;
        }

        let path = PathBuf::from(icon_name);
        if path.is_absolute() {
            return read_svg_icon(&path);
        }

        if icon_name.contains('/') {
            return None;
        }

        let icon_path = Path::new(icon_name);
        let name = if is_image_extension(icon_path) {
            icon_path
                .file_stem()
                .and_then(|value| value.to_str())
                .unwrap_or(icon_name)
        } else {
            icon_name
        };

        self.find_icon(&[name])
    }
}

fn entry_mime(entry: &FileEntry) -> MimeInfo {
    match entry.kind {
        EntryKind::Directory => {
            MimeInfo::new("inode/directory", filesystem_mime::MimeSource::BuiltInName)
        }
        EntryKind::File => detect_path(&entry.path),
        EntryKind::Symlink => {
            MimeInfo::new("inode/symlink", filesystem_mime::MimeSource::BuiltInName)
        }
        EntryKind::Other => MimeInfo::new(
            "application/octet-stream",
            filesystem_mime::MimeSource::Unknown,
        ),
    }
}

fn fallback_file_icon() -> EntryIcon {
    EntryIcon::Embedded(include_bytes!("../../../icons/file.svg"))
}

fn fallback_app_icon() -> EntryIcon {
    EntryIcon::Embedded(include_bytes!("../../../icons/app.svg"))
}

fn read_svg_icon(path: &Path) -> Option<Vec<u8>> {
    (path.extension().and_then(|value| value.to_str()) == Some("svg"))
        .then(|| fs::read(path).ok())
        .flatten()
}

fn is_image_extension(path: &Path) -> bool {
    path.extension()
        .and_then(|value| value.to_str())
        .is_some_and(|extension| {
            matches!(
                extension.to_ascii_lowercase().as_str(),
                "svg" | "png" | "xpm"
            )
        })
}

const ICON_THEME_SUBDIRS: &[&str] = &[
    "scalable/mimetypes",
    "symbolic/mimetypes",
    "mimetypes/scalable",
    "mimes/scalable",
    "mimes/scalable-light",
    "mimes/scalable-night",
    "scalable/apps",
    "apps/scalable",
    "scalable/devices",
    "symbolic/devices",
    "devices/scalable",
    "scalable/actions",
    "symbolic/actions",
    "actions/scalable",
    "actions/scalable-light",
    "actions/scalable-night",
];

fn icon_roots() -> Vec<PathBuf> {
    let mut roots = Vec::new();

    if let Some(home) = env::var_os("HOME").map(PathBuf::from) {
        push_unique_path(&mut roots, home.join(".local/share/icons"));
        push_unique_path(&mut roots, home.join(".icons"));
    }

    let data_dirs =
        env::var_os("XDG_DATA_DIRS").unwrap_or_else(|| "/usr/local/share:/usr/share".into());
    for directory in env::split_paths(&data_dirs) {
        push_unique_path(&mut roots, directory.join("icons"));
    }

    roots
}

fn icon_theme_names(roots: &[PathBuf]) -> Vec<String> {
    let mut themes = Vec::new();

    for variable in [
        "FILE_ICON_THEME",
        "XDG_ICON_THEME",
        "GTK_ICON_THEME",
        "ICON_THEME",
    ] {
        if let Ok(theme) = env::var(variable) {
            push_unique_string(&mut themes, theme);
        }
    }

    for theme in ["Adwaita", "hicolor", "HighContrast", "ContrastHigh"] {
        push_unique_string(&mut themes, theme.to_string());
    }

    let mut discovered = Vec::new();
    for root in roots {
        let Ok(entries) = fs::read_dir(root) else {
            continue;
        };

        for entry in entries.flatten() {
            if entry.path().is_dir() {
                if let Some(name) = entry.file_name().to_str() {
                    discovered.push(name.to_string());
                }
            }
        }
    }

    discovered.sort();
    for theme in discovered {
        push_unique_string(&mut themes, theme);
    }

    themes
}

fn icon_filenames(name: &str) -> [String; 2] {
    [format!("{name}.svg"), format!("{name}-symbolic.svg")]
}

fn icon_names_for_mime(mime: &str) -> &'static [&'static str] {
    match mime {
        "application/pdf" => &["application-pdf", "gnome-mime-application-pdf"],
        "application/zip" => &["application-zip", "package-x-generic", "media-zip"],
        "application/gzip"
        | "application/x-tar"
        | "application/x-xz"
        | "application/x-bzip2"
        | "application/zstd"
        | "application/x-7z-compressed"
        | "application/vnd.rar" => &[
            "package-x-generic",
            "application-zip",
            "application-x-gzip",
            "media-zip",
        ],
        "text/csv"
        | "application/vnd.ms-excel"
        | "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
        | "application/vnd.oasis.opendocument.spreadsheet" => &["x-office-spreadsheet"],
        "application/vnd.ms-powerpoint"
        | "application/vnd.openxmlformats-officedocument.presentationml.presentation"
        | "application/vnd.oasis.opendocument.presentation" => &["x-office-presentation"],
        "application/msword"
        | "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        | "application/vnd.oasis.opendocument.text"
        | "application/rtf" => &["x-office-document", "text-x-generic"],
        value
            if value.starts_with("text/")
                || value == "application/json"
                || value == "application/xml" =>
        {
            &["text-x-generic"]
        }
        value if value.starts_with("image/") => &["image-x-generic"],
        value if value.starts_with("audio/") => &["audio-x-generic"],
        value if value.starts_with("video/") => &["video-x-generic"],
        _ => &[],
    }
}

fn push_unique_path(paths: &mut Vec<PathBuf>, path: PathBuf) {
    if path.is_dir() && !paths.iter().any(|existing| existing == &path) {
        paths.push(path);
    }
}

fn push_unique_string(values: &mut Vec<String>, value: String) {
    if !value.is_empty() && !values.iter().any(|existing| existing == &value) {
        values.push(value);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn resolve_app_icon_falls_back_to_embedded_icon() {
        let icon = resolve_app_icon(None);

        match icon {
            EntryIcon::Embedded(bytes) => assert!(!bytes.is_empty()),
            EntryIcon::Theme(_) => panic!("expected fallback icon"),
        }
    }

    #[test]
    fn dotted_app_icon_names_are_not_treated_as_extensions() {
        assert!(!is_image_extension(Path::new("org.example.App")));
        assert!(is_image_extension(Path::new("example.svg")));
    }
}
