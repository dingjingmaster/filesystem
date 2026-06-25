use std::env;
use std::fs;
use std::io::{self, Read};
use std::path::{Path, PathBuf};
use std::sync::OnceLock;

const READ_LIMIT: usize = 256 * 1024;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MimeInfo {
    pub mime: String,
    pub label: String,
    pub source: MimeSource,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MimeSource {
    BuiltInContent,
    BuiltInName,
    SystemMagic,
    SystemGlob,
    Text,
    Unknown,
}

impl MimeInfo {
    pub fn new(mime: impl Into<String>, source: MimeSource) -> Self {
        let mime = mime.into();
        let label = label_for_mime(&mime).to_string();

        Self {
            mime,
            label,
            source,
        }
    }
}

pub fn detect_path(path: impl AsRef<Path>) -> MimeInfo {
    let path = path.as_ref();

    match read_prefix(path) {
        Ok(bytes) => detect_bytes(path, &bytes),
        Err(_) => detect_name(path),
    }
}

pub fn detect_name(path: impl AsRef<Path>) -> MimeInfo {
    let path = path.as_ref();

    detect_builtin_name(path)
        .map(|mime| MimeInfo::new(mime, MimeSource::BuiltInName))
        .or_else(|| detect_system_glob(path))
        .unwrap_or_else(unknown)
}

pub fn detect_bytes(path: impl AsRef<Path>, bytes: &[u8]) -> MimeInfo {
    let path = path.as_ref();

    detect_builtin_content(bytes, path)
        .map(|mime| MimeInfo::new(mime, MimeSource::BuiltInContent))
        .or_else(|| detect_system_magic(bytes))
        .or_else(|| detect_structured_text(bytes).map(|mime| MimeInfo::new(mime, MimeSource::Text)))
        .or_else(|| {
            detect_builtin_name(path).map(|mime| MimeInfo::new(mime, MimeSource::BuiltInName))
        })
        .or_else(|| detect_system_glob(path))
        .or_else(|| detect_plain_text(bytes).map(|mime| MimeInfo::new(mime, MimeSource::Text)))
        .unwrap_or_else(unknown)
}

pub fn label_for_mime(mime: &str) -> &'static str {
    match mime {
        "inode/directory" => "Folder",
        "inode/symlink" => "Symbolic Link",
        "inode/x-empty" => "Empty File",
        "text/plain" => "Plain Text",
        "text/x-makefile" => "Makefile",
        "text/html" | "application/xhtml+xml" => "HTML Document",
        "text/markdown" => "Markdown Document",
        "text/x-log" => "Application Log",
        "text/css" => "CSS Stylesheet",
        "text/csv" => "CSV Document",
        "text/tab-separated-values" => "TSV Document",
        "application/json" => "JSON Document",
        "application/xml" | "text/xml" => "XML Document",
        "application/pdf" => "PDF Document",
        "application/rtf" => "Rich Text Document",
        "application/msword"
        | "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        | "application/vnd.oasis.opendocument.text" => "Document",
        "application/vnd.ms-excel"
        | "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
        | "application/vnd.oasis.opendocument.spreadsheet" => "Spreadsheet",
        "application/vnd.ms-powerpoint"
        | "application/vnd.openxmlformats-officedocument.presentationml.presentation"
        | "application/vnd.oasis.opendocument.presentation" => "Presentation",
        "application/epub+zip" => "EPUB Book",
        "application/java-archive" => "Java Archive",
        "application/vnd.android.package-archive" => "Android Package",
        "application/zip"
        | "application/gzip"
        | "application/x-bzip2"
        | "application/x-tar"
        | "application/x-xz"
        | "application/zstd"
        | "application/x-7z-compressed"
        | "application/vnd.rar" => "Archive",
        "application/x-executable" => "Executable",
        "application/x-sharedlib" => "Shared Library",
        "application/x-desktop" => "Desktop Entry",
        "application/x-ole-storage" => "OLE Compound Document",
        "application/octet-stream" => "File",
        value if value.starts_with("image/") => "Image",
        value if value.starts_with("audio/") => "Audio",
        value if value.starts_with("video/") => "Video",
        value if value.starts_with("font/") || value.starts_with("application/font") => "Font",
        value if value.starts_with("text/") => "Text",
        _ => "File",
    }
}

pub fn read_limit() -> usize {
    READ_LIMIT
}

fn read_prefix(path: &Path) -> io::Result<Vec<u8>> {
    let mut file = fs::File::open(path)?;
    let mut bytes = Vec::with_capacity(READ_LIMIT.min(8192));
    file.by_ref()
        .take(READ_LIMIT as u64)
        .read_to_end(&mut bytes)?;
    Ok(bytes)
}

fn detect_builtin_content(bytes: &[u8], path: &Path) -> Option<&'static str> {
    if bytes.is_empty() {
        return detect_builtin_name(path).or(Some("inode/x-empty"));
    }

    if is_pdf(bytes) {
        return Some("application/pdf");
    }

    if bytes.starts_with(b"\x89PNG\r\n\x1a\n") {
        return Some("image/png");
    }
    if bytes.starts_with(b"\xff\xd8\xff") {
        return Some("image/jpeg");
    }
    if bytes.starts_with(b"GIF87a") || bytes.starts_with(b"GIF89a") {
        return Some("image/gif");
    }
    if bytes.starts_with(b"BM") {
        return Some("image/bmp");
    }
    if bytes.starts_with(b"\0\0\x01\0") {
        return Some("image/vnd.microsoft.icon");
    }
    if bytes.starts_with(b"II*\0") || bytes.starts_with(b"MM\0*") {
        return Some("image/tiff");
    }
    if riff_type(bytes, b"WEBP") {
        return Some("image/webp");
    }
    if riff_type(bytes, b"WAVE") {
        return Some("audio/wav");
    }
    if riff_type(bytes, b"AVI ") {
        return Some("video/x-msvideo");
    }

    if bytes.starts_with(b"ID3") || looks_like_mp3_frame(bytes) {
        return Some("audio/mpeg");
    }
    if bytes.starts_with(b"fLaC") {
        return Some("audio/flac");
    }
    if bytes.starts_with(b"OggS") {
        return Some("audio/ogg");
    }
    if is_mp4(bytes) {
        return Some("video/mp4");
    }

    if bytes.starts_with(b"\x7fELF") {
        return elf_mime(bytes, path);
    }
    if bytes.starts_with(b"\x1f\x8b") {
        return Some("application/gzip");
    }
    if bytes.starts_with(b"BZh") {
        return Some("application/x-bzip2");
    }
    if bytes.starts_with(b"\xfd7zXZ\0") {
        return Some("application/x-xz");
    }
    if bytes.starts_with(b"(\xb5/\xfd") {
        return Some("application/zstd");
    }
    if bytes.starts_with(b"7z\xbc\xaf\x27\x1c") {
        return Some("application/x-7z-compressed");
    }
    if bytes.starts_with(b"Rar!\x1a\x07\0") || bytes.starts_with(b"Rar!\x1a\x07\x01\0") {
        return Some("application/vnd.rar");
    }
    if is_tar(bytes) {
        return Some("application/x-tar");
    }
    if bytes.starts_with(b"PK\x03\x04")
        || bytes.starts_with(b"PK\x05\x06")
        || bytes.starts_with(b"PK\x07\x08")
    {
        return Some(
            zip_mime(bytes)
                .or_else(|| zip_container_extension_mime(path))
                .unwrap_or("application/zip"),
        );
    }
    if bytes.starts_with(b"\xd0\xcf\x11\xe0\xa1\xb1\x1a\xe1") {
        return Some(ole_mime(bytes).unwrap_or("application/x-ole-storage"));
    }

    None
}

fn detect_structured_text(bytes: &[u8]) -> Option<&'static str> {
    let text = text_from_bytes(bytes)?;
    let text = text.trim_start_matches('\u{feff}').trim_start();

    if text.starts_with("#!") {
        return Some(script_mime(text.lines().next().unwrap_or_default()));
    }
    if starts_with_ignore_ascii_case(text, "<!doctype html")
        || starts_with_ignore_ascii_case(text, "<html")
    {
        return Some("text/html");
    }
    if text.starts_with("<?xml") {
        return Some("application/xml");
    }
    if looks_like_json(text) {
        return Some("application/json");
    }

    None
}

fn detect_plain_text(bytes: &[u8]) -> Option<&'static str> {
    text_from_bytes(bytes).map(|_| "text/plain")
}

fn text_from_bytes(bytes: &[u8]) -> Option<&str> {
    if bytes.iter().take(4096).any(|byte| *byte == 0) {
        return None;
    }

    std::str::from_utf8(bytes).ok()
}

fn detect_builtin_name(path: &Path) -> Option<&'static str> {
    let file_name = path.file_name().and_then(|value| value.to_str())?;
    let lower_name = file_name.to_ascii_lowercase();

    if is_makefile_name(&lower_name) {
        return Some("text/x-makefile");
    }

    if let Some(mime) = special_name_mime(&lower_name) {
        return Some(mime);
    }

    let extension = path
        .extension()
        .and_then(|value| value.to_str())
        .map(str::to_ascii_lowercase);

    extension.as_deref().and_then(extension_mime)
}

fn special_name_mime(lower_name: &str) -> Option<&'static str> {
    match lower_name {
        "dockerfile" | "containerfile" => Some("text/x-dockerfile"),
        "readme" => Some("text/x-readme"),
        "license" | "copying" => Some("text/x-copying"),
        "changelog" | "changes" => Some("text/x-changelog"),
        "install" => Some("text/x-install"),
        "authors" => Some("text/x-authors"),
        "contributors" | "todo" | "news" => Some("text/plain"),
        ".gitignore" | ".gitattributes" | ".gitmodules" => Some("text/plain"),
        ".dockerignore" | ".env" | ".editorconfig" => Some("text/plain"),
        _ => None,
    }
}

fn extension_mime(extension: &str) -> Option<&'static str> {
    match extension {
        "txt" | "text" | "asc" | "nfo" => Some("text/plain"),
        "log" => Some("text/x-log"),
        "md" | "markdown" => Some("text/markdown"),
        "csv" => Some("text/csv"),
        "tsv" => Some("text/tab-separated-values"),
        "conf" | "ini" | "cfg" | "toml" | "yaml" | "yml" => Some("text/plain"),
        "rs" => Some("text/rust"),
        "c" => Some("text/x-csrc"),
        "h" => Some("text/x-chdr"),
        "cpp" | "cc" | "cxx" => Some("text/x-c++src"),
        "hpp" | "hh" | "hxx" => Some("text/x-c++hdr"),
        "java" => Some("text/x-java"),
        "js" | "mjs" | "cjs" => Some("text/javascript"),
        "ts" | "tsx" => Some("text/vnd.trolltech.linguist"),
        "py" | "pyw" => Some("text/x-python"),
        "go" => Some("text/x-go"),
        "sh" | "bash" | "zsh" | "fish" => Some("text/x-shellscript"),
        "pl" | "pm" => Some("text/x-perl"),
        "rb" => Some("application/x-ruby"),
        "lua" => Some("text/x-lua"),
        "mk" => Some("text/x-makefile"),
        "html" | "htm" => Some("text/html"),
        "css" => Some("text/css"),
        "scss" | "sass" => Some("text/x-scss"),
        "json" | "map" => Some("application/json"),
        "xml" | "xsl" | "xsd" => Some("application/xml"),
        "desktop" => Some("application/x-desktop"),
        "svg" => Some("image/svg+xml"),
        "pdf" => Some("application/pdf"),
        "rtf" => Some("application/rtf"),
        "png" => Some("image/png"),
        "jpg" | "jpeg" | "jpe" | "jfif" => Some("image/jpeg"),
        "gif" => Some("image/gif"),
        "webp" => Some("image/webp"),
        "bmp" => Some("image/bmp"),
        "tif" | "tiff" => Some("image/tiff"),
        "ico" => Some("image/vnd.microsoft.icon"),
        "heic" | "heif" => Some("image/heif"),
        "avif" => Some("image/avif"),
        "mp3" => Some("audio/mpeg"),
        "flac" => Some("audio/flac"),
        "wav" => Some("audio/wav"),
        "ogg" | "oga" => Some("audio/ogg"),
        "opus" => Some("audio/ogg"),
        "m4a" | "aac" => Some("audio/aac"),
        "mp4" | "m4v" => Some("video/mp4"),
        "mkv" => Some("video/x-matroska"),
        "webm" => Some("video/webm"),
        "mov" => Some("video/quicktime"),
        "avi" => Some("video/x-msvideo"),
        "flv" => Some("video/x-flv"),
        "zip" => Some("application/zip"),
        "gz" | "tgz" => Some("application/gzip"),
        "bz2" => Some("application/x-bzip2"),
        "tar" => Some("application/x-tar"),
        "xz" | "txz" => Some("application/x-xz"),
        "zst" => Some("application/zstd"),
        "7z" => Some("application/x-7z-compressed"),
        "rar" => Some("application/vnd.rar"),
        "doc" => Some("application/msword"),
        "docx" => Some("application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
        "odt" => Some("application/vnd.oasis.opendocument.text"),
        "xls" => Some("application/vnd.ms-excel"),
        "xlsx" => Some("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
        "ods" => Some("application/vnd.oasis.opendocument.spreadsheet"),
        "ppt" => Some("application/vnd.ms-powerpoint"),
        "pptx" => Some("application/vnd.openxmlformats-officedocument.presentationml.presentation"),
        "odp" => Some("application/vnd.oasis.opendocument.presentation"),
        "epub" => Some("application/epub+zip"),
        "jar" => Some("application/java-archive"),
        "apk" => Some("application/vnd.android.package-archive"),
        "ttf" => Some("font/ttf"),
        "otf" => Some("font/otf"),
        "woff" => Some("font/woff"),
        "woff2" => Some("font/woff2"),
        _ => None,
    }
}

fn detect_system_glob(path: &Path) -> Option<MimeInfo> {
    system_mime_database()
        .match_path(path)
        .map(|mime| MimeInfo::new(mime, MimeSource::SystemGlob))
}

fn detect_system_magic(bytes: &[u8]) -> Option<MimeInfo> {
    system_mime_database()
        .match_magic(bytes)
        .map(|mime| MimeInfo::new(mime, MimeSource::SystemMagic))
}

fn system_mime_database() -> &'static SystemMimeDatabase {
    static DATABASE: OnceLock<SystemMimeDatabase> = OnceLock::new();
    DATABASE.get_or_init(SystemMimeDatabase::load)
}

#[derive(Default)]
struct SystemMimeDatabase {
    glob_rules: Vec<SystemGlobRule>,
    magic_rules: Vec<SystemMagicRule>,
}

struct SystemGlobRule {
    weight: u16,
    mime: String,
    pattern: String,
    matcher: GlobMatcher,
}

enum GlobMatcher {
    Exact(String),
    Suffix(String),
    Prefix(String),
    Contains { prefix: String, suffix: String },
}

struct SystemMagicRule {
    priority: u16,
    mime: String,
    offset: usize,
    value: Vec<u8>,
    range: usize,
}

impl SystemMimeDatabase {
    fn load() -> Self {
        let mut glob_rules = Vec::new();
        for path in system_globs2_paths() {
            if let Ok(contents) = fs::read_to_string(path) {
                for line in contents.lines() {
                    if let Some(rule) = parse_globs2_line(line) {
                        glob_rules.push(rule);
                    }
                }
            }
        }

        glob_rules.sort_by(|left, right| {
            right
                .weight
                .cmp(&left.weight)
                .then_with(|| right.pattern.len().cmp(&left.pattern.len()))
                .then_with(|| left.mime.cmp(&right.mime))
        });

        let mut magic_rules = Vec::new();
        for path in system_magic_paths() {
            if let Ok(contents) = fs::read(path) {
                magic_rules.extend(parse_magic_file(&contents));
            }
        }

        magic_rules.sort_by(|left, right| {
            right
                .priority
                .cmp(&left.priority)
                .then_with(|| right.value.len().cmp(&left.value.len()))
                .then_with(|| left.mime.cmp(&right.mime))
        });

        Self {
            glob_rules,
            magic_rules,
        }
    }

    fn match_path(&self, path: &Path) -> Option<String> {
        let name = path.file_name()?.to_str()?.to_ascii_lowercase();
        self.glob_rules
            .iter()
            .find(|rule| rule.matcher.matches(&name))
            .map(|rule| rule.mime.clone())
    }

    fn match_magic(&self, bytes: &[u8]) -> Option<String> {
        self.magic_rules
            .iter()
            .find(|rule| rule.matches(bytes))
            .map(|rule| rule.mime.clone())
    }
}

impl SystemMagicRule {
    fn matches(&self, bytes: &[u8]) -> bool {
        if self.value.is_empty() || self.offset >= bytes.len() {
            return false;
        }

        if self.range == 0 {
            return bytes
                .get(self.offset..self.offset + self.value.len())
                .is_some_and(|candidate| candidate == self.value.as_slice());
        }

        let Some(search) = bytes.get(self.offset..) else {
            return false;
        };
        let max_start = self
            .range
            .min(search.len().saturating_sub(self.value.len()));

        (0..=max_start).any(|start| {
            search
                .get(start..start + self.value.len())
                .is_some_and(|candidate| candidate == self.value.as_slice())
        })
    }
}

impl GlobMatcher {
    fn matches(&self, name: &str) -> bool {
        match self {
            Self::Exact(value) => name == value,
            Self::Suffix(suffix) => name.ends_with(suffix),
            Self::Prefix(prefix) => name.starts_with(prefix),
            Self::Contains { prefix, suffix } => name.starts_with(prefix) && name.ends_with(suffix),
        }
    }
}

fn parse_globs2_line(line: &str) -> Option<SystemGlobRule> {
    let line = line.trim();
    if line.is_empty() || line.starts_with('#') {
        return None;
    }

    let mut parts = line.splitn(3, ':');
    let weight = parts.next()?.parse().ok()?;
    let mime = parts.next()?.to_string();
    let pattern = parts.next()?.to_ascii_lowercase();
    let matcher = glob_matcher(&pattern)?;

    Some(SystemGlobRule {
        weight,
        mime,
        pattern,
        matcher,
    })
}

fn glob_matcher(pattern: &str) -> Option<GlobMatcher> {
    let wildcard_count = pattern.bytes().filter(|byte| *byte == b'*').count();
    match wildcard_count {
        0 => Some(GlobMatcher::Exact(pattern.to_string())),
        1 if pattern.starts_with("*.") => Some(GlobMatcher::Suffix(pattern[1..].to_string())),
        1 if pattern.ends_with('*') => Some(GlobMatcher::Prefix(
            pattern[..pattern.len() - 1].to_string(),
        )),
        1 => {
            let (prefix, suffix) = pattern.split_once('*')?;
            Some(GlobMatcher::Contains {
                prefix: prefix.to_string(),
                suffix: suffix.to_string(),
            })
        }
        _ => None,
    }
}

fn parse_magic_file(bytes: &[u8]) -> Vec<SystemMagicRule> {
    let mut rules = Vec::new();
    let mut position = match bytes.strip_prefix(b"MIME-Magic\0\n") {
        Some(_) => b"MIME-Magic\0\n".len(),
        None => return rules,
    };
    let mut section: Option<(u16, String)> = None;
    let mut pending: Option<(SystemMagicRule, bool)> = None;

    while position < bytes.len() {
        if bytes[position] == b'\n' {
            position += 1;
            continue;
        }

        if bytes[position] == b'[' {
            push_pending_magic_rule(&mut rules, pending.take());
            if let Some((priority, mime, next)) = parse_magic_section(bytes, position) {
                section = Some((priority, mime));
                position = next;
            } else {
                break;
            }
            continue;
        }

        let Some((indent, offset, value, range, next)) = parse_magic_line(bytes, position) else {
            position = skip_line(bytes, position);
            continue;
        };

        if indent == 0 {
            push_pending_magic_rule(&mut rules, pending.take());
            if let Some((priority, mime)) = &section {
                pending = Some((
                    SystemMagicRule {
                        priority: *priority,
                        mime: mime.clone(),
                        offset,
                        value,
                        range,
                    },
                    false,
                ));
            }
        } else if let Some((_, has_children)) = pending.as_mut() {
            *has_children = true;
        }

        position = next;
    }

    push_pending_magic_rule(&mut rules, pending);
    rules
}

fn push_pending_magic_rule(
    rules: &mut Vec<SystemMagicRule>,
    pending: Option<(SystemMagicRule, bool)>,
) {
    let Some((rule, has_children)) = pending else {
        return;
    };

    if !has_children {
        rules.push(rule);
    }
}

fn parse_magic_section(bytes: &[u8], start: usize) -> Option<(u16, String, usize)> {
    let end = bytes[start..].iter().position(|byte| *byte == b'\n')? + start;
    let section = std::str::from_utf8(bytes.get(start + 1..end.checked_sub(1)?)?).ok()?;
    let (priority, mime) = section.split_once(':')?;
    let priority = priority.parse().ok()?;
    Some((priority, mime.to_string(), end + 1))
}

fn parse_magic_line(bytes: &[u8], start: usize) -> Option<(usize, usize, Vec<u8>, usize, usize)> {
    let mut position = start;
    let indent = if bytes.get(position) == Some(&b'>') {
        0
    } else {
        let indent_start = position;
        while bytes
            .get(position)
            .is_some_and(|byte| byte.is_ascii_digit())
        {
            position += 1;
        }
        if bytes.get(position) != Some(&b'>') {
            return None;
        }
        parse_ascii_usize(bytes.get(indent_start..position)?)?
    };

    position += 1;
    let offset_start = position;
    while bytes
        .get(position)
        .is_some_and(|byte| byte.is_ascii_digit())
    {
        position += 1;
    }
    if bytes.get(position) != Some(&b'=') {
        return None;
    }

    let offset = parse_ascii_usize(bytes.get(offset_start..position)?)?;
    position += 1;

    let length = be_u16(bytes, position) as usize;
    position += 2;
    let value = bytes.get(position..position.checked_add(length)?)?.to_vec();
    position += length;

    if bytes.get(position) == Some(&b'&') {
        position = position.checked_add(1 + length)?;
        if position > bytes.len() {
            return None;
        }
    }

    let mut range = 0;
    if bytes.get(position) == Some(&b'+') {
        position += 1;
        let range_start = position;
        while bytes
            .get(position)
            .is_some_and(|byte| byte.is_ascii_digit())
        {
            position += 1;
        }
        range = parse_ascii_usize(bytes.get(range_start..position)?)?;
    }

    Some((indent, offset, value, range, skip_line(bytes, position)))
}

fn parse_ascii_usize(bytes: &[u8]) -> Option<usize> {
    std::str::from_utf8(bytes).ok()?.parse().ok()
}

fn skip_line(bytes: &[u8], start: usize) -> usize {
    if start >= bytes.len() {
        return bytes.len();
    }

    bytes[start..]
        .iter()
        .position(|byte| *byte == b'\n')
        .map(|position| start + position + 1)
        .unwrap_or(bytes.len())
}

fn system_globs2_paths() -> Vec<PathBuf> {
    let mut paths = Vec::new();

    if let Some(data_home) = env::var_os("XDG_DATA_HOME").map(PathBuf::from) {
        paths.push(data_home.join("mime/globs2"));
    } else if let Some(home) = env::var_os("HOME").map(PathBuf::from) {
        paths.push(home.join(".local/share/mime/globs2"));
    }

    let data_dirs =
        env::var_os("XDG_DATA_DIRS").unwrap_or_else(|| "/usr/local/share:/usr/share".into());
    for directory in env::split_paths(&data_dirs) {
        push_unique_path(&mut paths, directory.join("mime/globs2"));
    }

    paths
}

fn system_magic_paths() -> Vec<PathBuf> {
    let mut paths = Vec::new();

    if let Some(data_home) = env::var_os("XDG_DATA_HOME").map(PathBuf::from) {
        paths.push(data_home.join("mime/magic"));
    } else if let Some(home) = env::var_os("HOME").map(PathBuf::from) {
        paths.push(home.join(".local/share/mime/magic"));
    }

    let data_dirs =
        env::var_os("XDG_DATA_DIRS").unwrap_or_else(|| "/usr/local/share:/usr/share".into());
    for directory in env::split_paths(&data_dirs) {
        push_unique_path(&mut paths, directory.join("mime/magic"));
    }

    paths
}

fn push_unique_path(paths: &mut Vec<PathBuf>, path: PathBuf) {
    if !paths.iter().any(|existing| existing == &path) {
        paths.push(path);
    }
}

fn is_makefile_name(lower_name: &str) -> bool {
    matches!(lower_name, "makefile" | "gnumakefile" | "bsdmakefile")
        || lower_name.starts_with("makefile.")
}

fn is_pdf(bytes: &[u8]) -> bool {
    bytes.starts_with(b"%PDF-")
        || bytes.starts_with(b"\n%PDF-")
        || bytes.starts_with(b"\xef\xbb\xbf%PDF-")
        || bytes.windows(5).take(1024).any(|window| window == b"%PDF-")
}

fn riff_type(bytes: &[u8], kind: &[u8; 4]) -> bool {
    bytes.len() >= 12 && bytes.starts_with(b"RIFF") && &bytes[8..12] == kind
}

fn looks_like_mp3_frame(bytes: &[u8]) -> bool {
    bytes.len() >= 2 && bytes[0] == 0xff && (bytes[1] & 0xe0) == 0xe0
}

fn is_mp4(bytes: &[u8]) -> bool {
    bytes.len() >= 12 && &bytes[4..8] == b"ftyp"
}

fn is_tar(bytes: &[u8]) -> bool {
    bytes.len() > 265 && (&bytes[257..262] == b"ustar")
}

fn elf_mime(bytes: &[u8], path: &Path) -> Option<&'static str> {
    if path
        .extension()
        .and_then(|value| value.to_str())
        .is_some_and(|extension| extension == "so")
    {
        return Some("application/x-sharedlib");
    }

    match bytes.get(16) {
        Some(2) | Some(3) => Some("application/x-executable"),
        Some(1) => Some("application/x-object"),
        _ => Some("application/octet-stream"),
    }
}

fn zip_mime(bytes: &[u8]) -> Option<&'static str> {
    let entries = zip_entry_names(bytes);

    for name in &entries {
        if name.starts_with("word/") {
            return Some("application/vnd.openxmlformats-officedocument.wordprocessingml.document");
        }
        if name.starts_with("xl/") {
            return Some("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
        }
        if name.starts_with("ppt/") {
            return Some(
                "application/vnd.openxmlformats-officedocument.presentationml.presentation",
            );
        }
    }

    if let Some(mimetype) = zip_mimetype_entry(bytes) {
        return odf_or_epub_mime(mimetype.trim());
    }

    if entries.iter().any(|name| name == "META-INF/MANIFEST.MF") {
        return Some("application/java-archive");
    }
    if entries.iter().any(|name| name == "AndroidManifest.xml") {
        return Some("application/vnd.android.package-archive");
    }
    if entries.iter().any(|name| name == "META-INF/container.xml") {
        return Some("application/epub+zip");
    }

    None
}

fn zip_container_extension_mime(path: &Path) -> Option<&'static str> {
    let extension = path
        .extension()
        .and_then(|value| value.to_str())?
        .to_ascii_lowercase();

    match extension.as_str() {
        "docx" => Some("application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
        "xlsx" => Some("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
        "pptx" => Some("application/vnd.openxmlformats-officedocument.presentationml.presentation"),
        "odt" => Some("application/vnd.oasis.opendocument.text"),
        "ods" => Some("application/vnd.oasis.opendocument.spreadsheet"),
        "odp" => Some("application/vnd.oasis.opendocument.presentation"),
        "epub" => Some("application/epub+zip"),
        "jar" => Some("application/java-archive"),
        "apk" => Some("application/vnd.android.package-archive"),
        _ => None,
    }
}

fn zip_entry_names(bytes: &[u8]) -> Vec<String> {
    let mut names = Vec::new();
    let mut offset = 0;

    while let Some(position) = find_bytes(&bytes[offset..], b"PK\x03\x04") {
        let start = offset + position;
        if start + 30 > bytes.len() {
            break;
        }

        let name_len = le_u16(bytes, start + 26) as usize;
        let extra_len = le_u16(bytes, start + 28) as usize;
        let name_start = start + 30;
        let name_end = name_start.saturating_add(name_len);
        if name_end > bytes.len() {
            break;
        }

        if let Ok(name) = std::str::from_utf8(&bytes[name_start..name_end]) {
            names.push(name.to_string());
        }

        offset = name_end.saturating_add(extra_len);
        if offset <= start {
            offset = start + 4;
        }
    }

    names
}

fn zip_mimetype_entry(bytes: &[u8]) -> Option<&str> {
    if !bytes.starts_with(b"PK\x03\x04") || bytes.len() < 30 {
        return None;
    }

    let compressed_size = le_u32(bytes, 18) as usize;
    let name_len = le_u16(bytes, 26) as usize;
    let extra_len = le_u16(bytes, 28) as usize;
    let name_start: usize = 30;
    let name_end = name_start.checked_add(name_len)?;
    if name_end > bytes.len()
        || std::str::from_utf8(&bytes[name_start..name_end]).ok()? != "mimetype"
    {
        return None;
    }

    let content_start = name_end.checked_add(extra_len)?;
    let content_end = if compressed_size > 0 {
        content_start.checked_add(compressed_size)?
    } else {
        bytes[content_start..]
            .iter()
            .position(|byte| *byte == 0 || *byte == b'\n' || *byte == b'\r')
            .map(|position| content_start + position)
            .unwrap_or_else(|| (content_start + 128).min(bytes.len()))
    };
    if content_end > bytes.len() {
        return None;
    }

    std::str::from_utf8(&bytes[content_start..content_end]).ok()
}

fn odf_or_epub_mime(value: &str) -> Option<&'static str> {
    match value {
        "application/vnd.oasis.opendocument.text" => {
            Some("application/vnd.oasis.opendocument.text")
        }
        "application/vnd.oasis.opendocument.spreadsheet" => {
            Some("application/vnd.oasis.opendocument.spreadsheet")
        }
        "application/vnd.oasis.opendocument.presentation" => {
            Some("application/vnd.oasis.opendocument.presentation")
        }
        "application/epub+zip" => Some("application/epub+zip"),
        _ => None,
    }
}

fn ole_mime(bytes: &[u8]) -> Option<&'static str> {
    let names = [
        ("WordDocument", "application/msword"),
        ("Workbook", "application/vnd.ms-excel"),
        ("PowerPoint Document", "application/vnd.ms-powerpoint"),
    ];

    for (needle, mime) in names {
        if contains_utf16le(bytes, needle) || contains_ascii(bytes, needle.as_bytes()) {
            return Some(mime);
        }
    }

    None
}

fn contains_utf16le(bytes: &[u8], value: &str) -> bool {
    let mut needle = Vec::with_capacity(value.len() * 2);
    for unit in value.encode_utf16() {
        needle.extend_from_slice(&unit.to_le_bytes());
    }
    contains_ascii(bytes, &needle)
}

fn contains_ascii(bytes: &[u8], needle: &[u8]) -> bool {
    bytes.windows(needle.len()).any(|window| window == needle)
}

fn script_mime(line: &str) -> &'static str {
    let lower = line.to_ascii_lowercase();
    if lower.contains("python") {
        "text/x-python"
    } else if lower.contains("node") {
        "text/javascript"
    } else if lower.contains("perl") {
        "text/x-perl"
    } else if lower.contains("ruby") {
        "application/x-ruby"
    } else if lower.contains("lua") {
        "text/x-lua"
    } else {
        "text/x-shellscript"
    }
}

fn starts_with_ignore_ascii_case(value: &str, prefix: &str) -> bool {
    value
        .get(..prefix.len())
        .is_some_and(|head| head.eq_ignore_ascii_case(prefix))
}

fn looks_like_json(text: &str) -> bool {
    let trimmed = text.trim_start();
    (trimmed.starts_with('{') && trimmed.contains(':'))
        || (trimmed.starts_with('[') && trimmed.contains(']'))
}

fn find_bytes(haystack: &[u8], needle: &[u8]) -> Option<usize> {
    haystack
        .windows(needle.len())
        .position(|window| window == needle)
}

fn le_u16(bytes: &[u8], offset: usize) -> u16 {
    let Some(raw) = bytes.get(offset..offset + 2) else {
        return 0;
    };
    u16::from_le_bytes([raw[0], raw[1]])
}

fn le_u32(bytes: &[u8], offset: usize) -> u32 {
    let Some(raw) = bytes.get(offset..offset + 4) else {
        return 0;
    };
    u32::from_le_bytes([raw[0], raw[1], raw[2], raw[3]])
}

fn be_u16(bytes: &[u8], offset: usize) -> u16 {
    let Some(raw) = bytes.get(offset..offset + 2) else {
        return 0;
    };
    u16::from_be_bytes([raw[0], raw[1]])
}

fn unknown() -> MimeInfo {
    MimeInfo::new("application/octet-stream", MimeSource::Unknown)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn detects_common_extensionless_text_names() {
        assert_eq!(detect_name("Makefile").mime, "text/x-makefile");
        assert_eq!(detect_name("GNUmakefile").mime, "text/x-makefile");
        assert_eq!(detect_name("Makefile.in").mime, "text/x-makefile");
        assert_eq!(detect_name("Dockerfile").mime, "text/x-dockerfile");
        assert_eq!(detect_name(".gitignore").mime, "text/plain");
    }

    #[test]
    fn detects_common_extensions() {
        assert_eq!(
            detect_name("a.docx").mime,
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        );
        assert_eq!(detect_name("a.pdf").label, "PDF Document");
        assert_eq!(detect_name("a.webp").mime, "image/webp");
        assert_eq!(detect_name("a.desktop").mime, "application/x-desktop");
    }

    #[test]
    fn content_signature_beats_extension() {
        let info = detect_bytes("wrong.txt", b"%PDF-1.7\n");

        assert_eq!(info.mime, "application/pdf");
        assert_eq!(info.source, MimeSource::BuiltInContent);

        let info = detect_bytes("wrong.odt", b"%PDF-1.7\n");

        assert_eq!(info.mime, "application/pdf");
        assert_eq!(info.source, MimeSource::BuiltInContent);
    }

    #[test]
    fn detects_text_content() {
        assert_eq!(
            detect_bytes("unknown", b"#!/usr/bin/env python\n").mime,
            "text/x-python"
        );
        assert_eq!(
            detect_bytes("unknown", br#"{"a":1}"#).mime,
            "application/json"
        );
        assert_eq!(detect_bytes("unknown", b"plain text").mime, "text/plain");
    }

    #[test]
    fn detects_zip_office_entry_names() {
        let bytes = zip_local_file("word/document.xml", b"");

        assert_eq!(
            detect_bytes("a.zip", &bytes).mime,
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        );
    }

    #[test]
    fn detects_odf_mimetype_before_next_zip_header() {
        let mut bytes = zip_local_file("mimetype", b"application/vnd.oasis.opendocument.text");
        bytes.extend_from_slice(&zip_local_file("content.xml", b"<office:document />"));

        assert_eq!(
            detect_bytes("a.odt", &bytes).mime,
            "application/vnd.oasis.opendocument.text"
        );
    }

    #[test]
    fn zip_container_extensions_fall_back_to_document_mime() {
        let bytes = zip_local_file("plain.txt", b"plain");

        assert_eq!(
            detect_bytes("a.odt", &bytes).mime,
            "application/vnd.oasis.opendocument.text"
        );
        assert_eq!(detect_bytes("a.zip", &bytes).mime, "application/zip");
    }

    #[test]
    fn parses_simple_globs2_lines() {
        let rule = parse_globs2_line("50:application/example:*.example").unwrap();

        assert_eq!(rule.weight, 50);
        assert!(rule.matcher.matches("a.example"));
        assert!(!rule.matcher.matches("a.txt"));
    }

    #[test]
    fn parses_standalone_system_magic_rules() {
        let bytes = b"MIME-Magic\0\n[50:application/example]\n>4=\0\x03ABC\n";
        let rules = parse_magic_file(bytes);

        assert_eq!(rules.len(), 1);
        assert!(rules[0].matches(b"xxxxABC"));
        assert!(!rules[0].matches(b"xxxABC"));
    }

    #[test]
    fn skips_system_magic_rules_with_children() {
        let bytes = b"MIME-Magic\0\n[50:application/example]\n>0=\0\x01A\n1>1=\0\x01B\n";

        assert!(parse_magic_file(bytes).is_empty());
    }

    fn zip_local_file(name: &str, contents: &[u8]) -> Vec<u8> {
        let mut bytes = Vec::new();
        bytes.extend_from_slice(b"PK\x03\x04");
        bytes.extend_from_slice(&20u16.to_le_bytes());
        bytes.extend_from_slice(&0u16.to_le_bytes());
        bytes.extend_from_slice(&0u16.to_le_bytes());
        bytes.extend_from_slice(&0u16.to_le_bytes());
        bytes.extend_from_slice(&0u16.to_le_bytes());
        bytes.extend_from_slice(&0u32.to_le_bytes());
        bytes.extend_from_slice(&(contents.len() as u32).to_le_bytes());
        bytes.extend_from_slice(&(contents.len() as u32).to_le_bytes());
        bytes.extend_from_slice(&(name.len() as u16).to_le_bytes());
        bytes.extend_from_slice(&0u16.to_le_bytes());
        bytes.extend_from_slice(name.as_bytes());
        bytes.extend_from_slice(contents);
        bytes
    }
}
