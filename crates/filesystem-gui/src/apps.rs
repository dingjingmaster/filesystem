use crate::icons::resolve_app_icon;
use crate::model::{AppRegistry, DesktopApp};
use std::collections::{BTreeMap, BTreeSet};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

pub(crate) fn load_app_registry() -> AppRegistry {
    let mut apps = Vec::new();
    let mut seen = BTreeSet::new();

    for directory in application_dirs() {
        for (id, path) in desktop_files(&directory) {
            if !seen.insert(id.clone()) {
                continue;
            }

            if let Some(app) = parse_desktop_app(id, &path) {
                apps.push(app);
            }
        }
    }

    apps.sort_by(|left, right| {
        left.name
            .to_lowercase()
            .cmp(&right.name.to_lowercase())
            .then_with(|| left.id.cmp(&right.id))
    });

    AppRegistry {
        apps,
        defaults: load_mime_defaults(),
    }
}

fn desktop_files(directory: &Path) -> Vec<(String, PathBuf)> {
    let mut files = Vec::new();
    collect_desktop_files(directory, String::new(), &mut files);
    files
}

fn collect_desktop_files(directory: &Path, prefix: String, files: &mut Vec<(String, PathBuf)>) {
    let Ok(entries) = fs::read_dir(directory) else {
        return;
    };

    for entry in entries.flatten() {
        let Ok(file_type) = entry.file_type() else {
            continue;
        };
        let path = entry.path();
        let name = entry.file_name().to_string_lossy().into_owned();

        if file_type.is_dir() {
            collect_desktop_files(&path, format!("{prefix}{name}-"), files);
        } else if path.extension().and_then(|value| value.to_str()) == Some("desktop") {
            files.push((format!("{prefix}{name}"), path));
        }
    }
}

pub(crate) fn mime_for_path(path: &Path) -> String {
    let file_name = path
        .file_name()
        .and_then(|value| value.to_str())
        .unwrap_or_default();
    let lower_name = file_name.to_ascii_lowercase();

    if is_makefile_name(&lower_name) {
        return "text/x-makefile".to_string();
    }

    if is_plain_text_name(&lower_name) {
        return "text/plain".to_string();
    }

    let extension = path
        .extension()
        .and_then(|value| value.to_str())
        .map(str::to_ascii_lowercase);

    match extension.as_deref() {
        Some("txt" | "text" | "log" | "md" | "markdown" | "conf" | "ini" | "toml" | "yaml")
        | Some("yml" | "rs" | "c" | "h" | "cpp" | "hpp" | "js" | "ts" | "py" | "go" | "sh")
        | Some("csv") => "text/plain",
        Some("mk") => "text/x-makefile",
        Some("html" | "htm") => "text/html",
        Some("json") => "application/json",
        Some("xml") => "application/xml",
        Some("pdf") => "application/pdf",
        Some("png") => "image/png",
        Some("jpg" | "jpeg") => "image/jpeg",
        Some("gif") => "image/gif",
        Some("webp") => "image/webp",
        Some("svg") => "image/svg+xml",
        Some("mp3") => "audio/mpeg",
        Some("flac") => "audio/flac",
        Some("wav") => "audio/wav",
        Some("ogg") => "audio/ogg",
        Some("mp4") => "video/mp4",
        Some("mkv") => "video/x-matroska",
        Some("webm") => "video/webm",
        Some("zip") => "application/zip",
        Some("gz" | "tgz") => "application/gzip",
        Some("tar") => "application/x-tar",
        Some("xz") => "application/x-xz",
        Some("doc") => "application/msword",
        Some("docx") => "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
        Some("odt") => "application/vnd.oasis.opendocument.text",
        Some("xls") => "application/vnd.ms-excel",
        Some("xlsx") => "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
        Some("ods") => "application/vnd.oasis.opendocument.spreadsheet",
        Some("ppt") => "application/vnd.ms-powerpoint",
        Some("pptx") => "application/vnd.openxmlformats-officedocument.presentationml.presentation",
        Some("odp") => "application/vnd.oasis.opendocument.presentation",
        _ => "application/octet-stream",
    }
    .to_string()
}

fn is_makefile_name(lower_name: &str) -> bool {
    matches!(lower_name, "makefile" | "gnumakefile" | "bsdmakefile")
        || lower_name.starts_with("makefile.")
}

fn is_plain_text_name(lower_name: &str) -> bool {
    matches!(
        lower_name,
        "dockerfile"
            | "containerfile"
            | "readme"
            | "license"
            | "copying"
            | "changelog"
            | "changes"
            | "install"
            | "authors"
            | "contributors"
            | "todo"
            | "news"
            | ".gitignore"
            | ".gitattributes"
            | ".gitmodules"
            | ".dockerignore"
            | ".env"
            | ".editorconfig"
    )
}

pub(crate) fn file_type_label(path: &Path, mime: &str) -> String {
    if path
        .extension()
        .and_then(|value| value.to_str())
        .is_some_and(|extension| extension.eq_ignore_ascii_case("log"))
    {
        return "Application Log".to_string();
    }

    match mime {
        "text/plain" => "Plain Text".to_string(),
        "text/x-makefile" => "Makefile".to_string(),
        "text/html" => "HTML Document".to_string(),
        "application/json" => "JSON Document".to_string(),
        "application/xml" => "XML Document".to_string(),
        "application/pdf" => "PDF Document".to_string(),
        "application/msword"
        | "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        | "application/vnd.oasis.opendocument.text" => "Document".to_string(),
        "application/vnd.ms-excel"
        | "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
        | "application/vnd.oasis.opendocument.spreadsheet" => "Spreadsheet".to_string(),
        "application/vnd.ms-powerpoint"
        | "application/vnd.openxmlformats-officedocument.presentationml.presentation"
        | "application/vnd.oasis.opendocument.presentation" => "Presentation".to_string(),
        value if value.starts_with("image/") => "Image".to_string(),
        value if value.starts_with("audio/") => "Audio".to_string(),
        value if value.starts_with("video/") => "Video".to_string(),
        "application/zip" | "application/gzip" | "application/x-tar" | "application/x-xz" => {
            "Archive".to_string()
        }
        "application/octet-stream" => "File".to_string(),
        _ => mime.to_string(),
    }
}

pub(crate) fn apps_for_file(registry: &AppRegistry, path: &Path) -> Vec<DesktopApp> {
    let mime = mime_for_path(path);
    let default_id = default_app_for_file(registry, path).map(|app| app.id);
    let mut apps: Vec<DesktopApp> = registry
        .apps
        .iter()
        .filter(|app| app_supports_mime(app, &mime))
        .cloned()
        .collect();

    apps.sort_by(|left, right| compare_app_candidates(left, right, &mime, default_id.as_deref()));

    apps
}

pub(crate) fn best_app_for_file(registry: &AppRegistry, path: &Path) -> Option<DesktopApp> {
    default_app_for_file(registry, path)
        .or_else(|| apps_for_file(registry, path).into_iter().next())
}

pub(crate) fn default_app_for_file(registry: &AppRegistry, path: &Path) -> Option<DesktopApp> {
    let mime = mime_for_path(path);

    for candidate_mime in default_mime_candidates(&mime) {
        if let Some(defaults) = registry.defaults.get(candidate_mime) {
            for id in defaults {
                if let Some(app) = registry
                    .apps
                    .iter()
                    .find(|app| &app.id == id && app_supports_mime(app, &mime))
                {
                    return Some(app.clone());
                }
            }
        }
    }

    None
}

pub(crate) fn open_file_with_app(path: PathBuf, app: DesktopApp) -> Result<String, String> {
    let command = build_exec_command(&app, &path)?;
    let Some((program, args)) = command.split_first() else {
        return Err(format!("{} has no executable command", app.name));
    };

    let mut child = Command::new(program);
    child.args(args);
    if let Some(parent) = path.parent() {
        child.current_dir(parent);
    }

    child
        .spawn()
        .map_err(|error| format!("Failed to open with {}: {error}", app.name))?;

    Ok(app.name)
}

fn app_supports_mime(app: &DesktopApp, mime: &str) -> bool {
    app_mime_match_score(app, mime).is_some()
}

fn compare_app_candidates(
    left: &DesktopApp,
    right: &DesktopApp,
    mime: &str,
    default_id: Option<&str>,
) -> std::cmp::Ordering {
    let left_default = default_id.is_some_and(|id| id == left.id);
    let right_default = default_id.is_some_and(|id| id == right.id);

    right_default
        .cmp(&left_default)
        .then_with(|| {
            let left_score = app_mime_match_score(left, mime).unwrap_or_default();
            let right_score = app_mime_match_score(right, mime).unwrap_or_default();
            right_score.cmp(&left_score)
        })
        .then_with(|| left.name.to_lowercase().cmp(&right.name.to_lowercase()))
        .then_with(|| left.id.cmp(&right.id))
}

fn app_mime_match_score(app: &DesktopApp, mime: &str) -> Option<u8> {
    app.mime_types
        .iter()
        .filter_map(|candidate| mime_match_score(candidate, mime))
        .max()
}

fn mime_match_score(candidate: &str, mime: &str) -> Option<u8> {
    if candidate == mime {
        return Some(4);
    }

    if mime.starts_with("text/") && candidate == "text/plain" {
        return Some(3);
    }

    if candidate
        .strip_suffix("/*")
        .is_some_and(|prefix| mime.starts_with(&format!("{prefix}/")))
    {
        return Some(2);
    }

    (candidate == "*/*").then_some(1)
}

fn default_mime_candidates(mime: &str) -> Vec<&str> {
    let mut candidates = vec![mime];
    if mime.starts_with("text/") && mime != "text/plain" {
        candidates.push("text/plain");
    }
    candidates
}

fn parse_desktop_app(id: String, path: &Path) -> Option<DesktopApp> {
    let contents = fs::read_to_string(path).ok()?;
    let mut in_entry = false;
    let mut entry_type = String::new();
    let mut name = None;
    let mut exec = None;
    let mut icon = None;
    let mut mime_types = Vec::new();
    let mut hidden = false;

    for raw_line in contents.lines() {
        let line = raw_line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }

        if line.starts_with('[') && line.ends_with(']') {
            in_entry = line == "[Desktop Entry]";
            continue;
        }

        if !in_entry {
            continue;
        }

        let Some((key, value)) = line.split_once('=') else {
            continue;
        };

        match key {
            "Type" => entry_type = value.to_string(),
            "Name" => name = Some(value.to_string()),
            "Exec" => exec = Some(value.to_string()),
            "Icon" => icon = Some(value.to_string()),
            "MimeType" => {
                mime_types = value
                    .split(';')
                    .filter(|value| !value.is_empty())
                    .map(ToString::to_string)
                    .collect();
            }
            "Hidden" => hidden = value.eq_ignore_ascii_case("true"),
            "NoDisplay" => {}
            _ => {}
        }
    }

    if hidden || (!entry_type.is_empty() && entry_type != "Application") {
        return None;
    }

    Some(DesktopApp {
        id,
        name: name?,
        exec: exec?,
        mime_types,
        icon: resolve_app_icon(icon.as_deref()),
    })
    .filter(|app| !app.mime_types.is_empty())
}

fn load_mime_defaults() -> BTreeMap<String, Vec<String>> {
    let mut defaults = BTreeMap::new();

    for path in mimeapps_paths() {
        let Ok(contents) = fs::read_to_string(path) else {
            continue;
        };
        let mut in_defaults = false;

        for raw_line in contents.lines() {
            let line = raw_line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }

            if line.starts_with('[') && line.ends_with(']') {
                in_defaults = line == "[Default Applications]";
                continue;
            }

            if !in_defaults {
                continue;
            }

            let Some((mime, apps)) = line.split_once('=') else {
                continue;
            };
            defaults.entry(mime.to_string()).or_insert_with(|| {
                apps.split(';')
                    .filter(|value| !value.is_empty())
                    .map(ToString::to_string)
                    .collect()
            });
        }
    }

    defaults
}

fn application_dirs() -> Vec<PathBuf> {
    let mut dirs = Vec::new();

    if let Some(data_home) = env::var_os("XDG_DATA_HOME").map(PathBuf::from) {
        dirs.push(data_home.join("applications"));
    } else if let Some(home) = env::var_os("HOME").map(PathBuf::from) {
        dirs.push(home.join(".local/share/applications"));
    }

    let data_dirs =
        env::var_os("XDG_DATA_DIRS").unwrap_or_else(|| "/usr/local/share:/usr/share".into());
    for directory in env::split_paths(&data_dirs) {
        dirs.push(directory.join("applications"));
    }

    dirs
}

fn mimeapps_paths() -> Vec<PathBuf> {
    let mut paths = Vec::new();
    let desktops = current_desktops();

    for directory in config_dirs() {
        push_mimeapps_candidates(&mut paths, &directory, &desktops);
    }

    for directory in application_dirs() {
        push_mimeapps_candidates(&mut paths, &directory, &desktops);
    }

    paths
}

fn current_desktops() -> Vec<String> {
    env::var("XDG_CURRENT_DESKTOP")
        .map(|value| split_current_desktops(&value))
        .unwrap_or_default()
}

fn split_current_desktops(value: &str) -> Vec<String> {
    let mut desktops = Vec::new();

    for desktop in value.split(':') {
        let desktop = desktop.trim();
        if !desktop.is_empty() {
            push_unique_string(&mut desktops, desktop.to_ascii_lowercase());
        }
    }

    desktops
}

fn config_dirs() -> Vec<PathBuf> {
    let mut dirs = Vec::new();

    if let Some(config_home) = env::var_os("XDG_CONFIG_HOME").map(PathBuf::from) {
        dirs.push(config_home);
    } else if let Some(home) = env::var_os("HOME").map(PathBuf::from) {
        dirs.push(home.join(".config"));
    }

    let config_dirs = env::var_os("XDG_CONFIG_DIRS").unwrap_or_else(|| "/etc/xdg".into());
    for directory in env::split_paths(&config_dirs) {
        push_unique_candidate_path(&mut dirs, directory);
    }

    dirs
}

fn push_mimeapps_candidates(paths: &mut Vec<PathBuf>, directory: &Path, desktops: &[String]) {
    for desktop in desktops {
        push_unique_candidate_path(paths, directory.join(format!("{desktop}-mimeapps.list")));
    }
    push_unique_candidate_path(paths, directory.join("mimeapps.list"));
}

fn push_unique_candidate_path(paths: &mut Vec<PathBuf>, path: PathBuf) {
    if !paths.iter().any(|existing| existing == &path) {
        paths.push(path);
    }
}

fn push_unique_string(values: &mut Vec<String>, value: String) {
    if !value.is_empty() && !values.iter().any(|existing| existing == &value) {
        values.push(value);
    }
}

fn build_exec_command(app: &DesktopApp, path: &Path) -> Result<Vec<String>, String> {
    let path_value = path.to_string_lossy();
    let mut saw_file_code = false;
    let mut command = Vec::new();

    for token in split_exec(&app.exec)? {
        if matches!(token.as_str(), "%i" | "%k") {
            continue;
        }

        let mut value = token.replace("%%", "%");

        if value.contains("%c") {
            value = value.replace("%c", &app.name);
        }

        let before = value.clone();
        for code in ["%f", "%F", "%u", "%U"] {
            if value.contains(code) {
                value = value.replace(code, &path_value);
                saw_file_code = true;
            }
        }

        if before != value || !value.starts_with('%') {
            command.push(value);
        }
    }

    if command.is_empty() {
        return Err(format!("{} has an empty Exec command", app.name));
    }

    if !saw_file_code {
        command.push(path_value.into_owned());
    }

    Ok(command)
}

fn split_exec(value: &str) -> Result<Vec<String>, String> {
    let mut tokens = Vec::new();
    let mut current = String::new();
    let mut chars = value.chars();
    let mut quote = None;

    while let Some(ch) = chars.next() {
        if let Some(active_quote) = quote {
            if ch == active_quote {
                quote = None;
            } else if ch == '\\' {
                if let Some(next) = chars.next() {
                    current.push(next);
                }
            } else {
                current.push(ch);
            }
            continue;
        }

        match ch {
            '\'' | '"' => quote = Some(ch),
            '\\' => {
                if let Some(next) = chars.next() {
                    current.push(next);
                }
            }
            ch if ch.is_whitespace() => {
                if !current.is_empty() {
                    tokens.push(std::mem::take(&mut current));
                }
            }
            _ => current.push(ch),
        }
    }

    if quote.is_some() {
        return Err("unterminated quote in Exec command".to_string());
    }

    if !current.is_empty() {
        tokens.push(current);
    }

    Ok(tokens)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::{SystemTime, UNIX_EPOCH};

    fn temp_dir(name: &str) -> PathBuf {
        let id = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        env::temp_dir().join(format!("filesystem-gui-{name}-{}-{id}", std::process::id()))
    }

    #[test]
    fn mime_for_path_maps_common_text_files() {
        assert_eq!(mime_for_path(Path::new("a.log")), "text/plain");
        assert_eq!(
            file_type_label(Path::new("a.log"), "text/plain"),
            "Application Log"
        );
    }

    #[test]
    fn mime_for_path_maps_common_extensionless_text_files() {
        assert_eq!(mime_for_path(Path::new("Makefile")), "text/x-makefile");
        assert_eq!(mime_for_path(Path::new("GNUmakefile")), "text/x-makefile");
        assert_eq!(mime_for_path(Path::new("Makefile.in")), "text/x-makefile");
        assert_eq!(mime_for_path(Path::new("rules.mk")), "text/x-makefile");
        assert_eq!(
            file_type_label(Path::new("Makefile"), "text/x-makefile"),
            "Makefile"
        );
        assert_eq!(mime_for_path(Path::new("Dockerfile")), "text/plain");
        assert_eq!(mime_for_path(Path::new(".gitignore")), "text/plain");
    }

    #[test]
    fn mime_for_path_maps_common_office_files() {
        assert_eq!(
            mime_for_path(Path::new("a.docx")),
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
        );
        assert_eq!(
            file_type_label(
                Path::new("a.xlsx"),
                "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
            ),
            "Spreadsheet"
        );
    }

    #[test]
    fn build_exec_command_replaces_file_field_codes() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor --new-window %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a file.txt")).unwrap();

        assert_eq!(command, vec!["editor", "--new-window", "/tmp/a file.txt"]);
    }

    #[test]
    fn build_exec_command_appends_file_when_exec_has_no_field_code() {
        let app = DesktopApp {
            id: "viewer.desktop".to_string(),
            name: "Viewer".to_string(),
            exec: "viewer".to_string(),
            mime_types: vec!["text/plain".to_string()],
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a.txt")).unwrap();

        assert_eq!(command, vec!["viewer", "/tmp/a.txt"]);
    }

    #[test]
    fn split_exec_handles_quoted_arguments() {
        assert_eq!(
            split_exec("app \"two words\" 'three words'").unwrap(),
            vec!["app", "two words", "three words"]
        );
    }

    #[test]
    fn desktop_files_include_nested_applications() {
        let root = temp_dir("desktop-files");
        fs::create_dir_all(root.join("nested")).unwrap();
        fs::write(root.join("top.desktop"), "").unwrap();
        fs::write(root.join("nested/app.desktop"), "").unwrap();

        let mut files = desktop_files(&root);
        files.sort_by(|left, right| left.0.cmp(&right.0));

        assert_eq!(
            files.iter().map(|(id, _)| id.as_str()).collect::<Vec<_>>(),
            vec!["nested-app.desktop", "top.desktop"]
        );

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn default_app_requires_mimeapps_entry() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![app],
            defaults: BTreeMap::new(),
        };

        assert!(default_app_for_file(&registry, Path::new("a.log")).is_none());
    }

    #[test]
    fn best_app_uses_highest_mime_match_when_no_default_exists() {
        let generic = DesktopApp {
            id: "generic.desktop".to_string(),
            name: "Generic Viewer".to_string(),
            exec: "generic %f".to_string(),
            mime_types: vec!["application/*".to_string()],
            icon: resolve_app_icon(None),
        };
        let writer = DesktopApp {
            id: "writer.desktop".to_string(),
            name: "Writer".to_string(),
            exec: "writer %f".to_string(),
            mime_types: vec![
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
                    .to_string(),
            ],
            icon: resolve_app_icon(None),
        };
        let wildcard = DesktopApp {
            id: "wildcard.desktop".to_string(),
            name: "Any File".to_string(),
            exec: "any %f".to_string(),
            mime_types: vec!["*/*".to_string()],
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![generic, writer, wildcard],
            defaults: BTreeMap::new(),
        };

        let apps = apps_for_file(&registry, Path::new("a.docx"));

        assert_eq!(
            apps.first().map(|app| app.id.as_str()),
            Some("writer.desktop")
        );
        assert_eq!(
            best_app_for_file(&registry, Path::new("a.docx")).map(|app| app.id),
            Some("writer.desktop".to_string())
        );
    }

    #[test]
    fn text_plain_apps_can_open_specialized_text_files() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![app],
            defaults: BTreeMap::from([(
                "text/plain".to_string(),
                vec!["editor.desktop".to_string()],
            )]),
        };

        assert_eq!(
            default_app_for_file(&registry, Path::new("Makefile")).map(|app| app.id),
            Some("editor.desktop".to_string())
        );
        assert_eq!(apps_for_file(&registry, Path::new("Makefile")).len(), 1);
    }

    #[test]
    fn application_octet_stream_default_still_requires_matching_app_support() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![app],
            defaults: BTreeMap::from([(
                "application/octet-stream".to_string(),
                vec!["editor.desktop".to_string()],
            )]),
        };

        assert!(default_app_for_file(&registry, Path::new("unknown.bin")).is_none());
        assert!(best_app_for_file(&registry, Path::new("unknown.bin")).is_none());
    }

    #[test]
    fn split_current_desktops_normalizes_and_deduplicates_names() {
        assert_eq!(
            split_current_desktops("GNOME:Unity:gnome"),
            vec!["gnome", "unity"]
        );
    }

    #[test]
    fn mimeapps_candidates_prefer_desktop_specific_files() {
        let mut paths = Vec::new();
        push_mimeapps_candidates(
            &mut paths,
            Path::new("/etc/xdg"),
            &["gnome".to_string(), "unity".to_string()],
        );

        assert_eq!(
            paths,
            vec![
                PathBuf::from("/etc/xdg/gnome-mimeapps.list"),
                PathBuf::from("/etc/xdg/unity-mimeapps.list"),
                PathBuf::from("/etc/xdg/mimeapps.list"),
            ]
        );
    }

    #[test]
    fn no_display_desktop_apps_can_open_files() {
        let root = temp_dir("desktop-app");
        fs::create_dir_all(&root).unwrap();
        let path = root.join("hidden-menu.desktop");
        fs::write(
            &path,
            "[Desktop Entry]\nType=Application\nName=Hidden Menu App\nExec=app %f\nIcon=hidden-app\nMimeType=text/plain;\nNoDisplay=true\n",
        )
        .unwrap();

        let app = parse_desktop_app("hidden-menu.desktop".to_string(), &path).unwrap();

        assert_eq!(app.name, "Hidden Menu App");
        assert_eq!(app.mime_types, vec!["text/plain"]);
        assert!(matches!(app.icon, crate::model::EntryIcon::Embedded(_)));

        fs::remove_dir_all(root).unwrap();
    }
}
