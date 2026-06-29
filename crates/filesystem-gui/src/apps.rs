use crate::icons::resolve_app_icon;
use crate::model::{AppRegistry, DesktopApp};
use std::collections::{BTreeMap, BTreeSet};
use std::env;
use std::fs;
use std::os::unix::ffi::OsStrExt;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::thread;

pub(crate) fn load_app_registry() -> AppRegistry {
    let mut apps = Vec::new();
    let mut seen = BTreeSet::new();
    let mimeapps = load_mimeapps();

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

    apply_mimeapps_associations(&mut apps, &mimeapps);
    apps.sort_by(|left, right| {
        left.name
            .to_lowercase()
            .cmp(&right.name.to_lowercase())
            .then_with(|| left.id.cmp(&right.id))
    });

    AppRegistry {
        apps,
        defaults: mimeapps.defaults,
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

pub(crate) fn apps_for_mime(registry: &AppRegistry, mime: &str) -> Vec<DesktopApp> {
    let default_id = default_app_for_mime(registry, mime).map(|app| app.id);
    let mut apps: Vec<DesktopApp> = registry
        .apps
        .iter()
        .filter(|app| app_supports_mime(app, mime))
        .cloned()
        .collect();

    apps.sort_by(|left, right| compare_app_candidates(left, right, mime, default_id.as_deref()));

    apps
}

pub(crate) fn best_app_for_mime(registry: &AppRegistry, mime: &str) -> Option<DesktopApp> {
    default_app_for_mime(registry, mime)
        .or_else(|| apps_for_mime(registry, mime).into_iter().next())
}

pub(crate) fn default_app_for_mime(registry: &AppRegistry, mime: &str) -> Option<DesktopApp> {
    for candidate_mime in default_mime_candidates(&mime) {
        if let Some(defaults) = registry.defaults.get(candidate_mime) {
            for id in defaults {
                if let Some(app) = registry
                    .apps
                    .iter()
                    .find(|app| &app.id == id && app_supports_mime(app, mime))
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

    let mut child = child
        .spawn()
        .map_err(|error| format!("Failed to open with {}: {error}", app.name))?;
    thread::spawn(move || {
        let _ = child.wait();
    });

    Ok(app.name)
}

pub(crate) fn set_default_app_for_mime(mime: &str, app_id: &str) -> Result<(), String> {
    let paths = user_mimeapps_paths()?;
    set_default_app_for_mime_at(&paths.default_path, &paths.association_path, mime, app_id)
}

fn set_default_app_for_mime_at(
    default_path: &Path,
    association_path: &Path,
    mime: &str,
    app_id: &str,
) -> Result<(), String> {
    validate_mimeapps_assignment(mime, app_id)?;

    if default_path == association_path {
        return update_mimeapps_file(default_path, |contents| {
            update_mimeapps_contents(contents, mime, app_id)
        });
    }

    update_mimeapps_file(default_path, |contents| {
        update_mimeapps_default_contents(contents, mime, app_id)
    })?;
    update_mimeapps_file(association_path, |contents| {
        update_mimeapps_association_contents(contents, mime, app_id)
    })
}

fn update_mimeapps_file<F>(path: &Path, update: F) -> Result<(), String>
where
    F: FnOnce(&str) -> String,
{
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).map_err(|error| {
            format!(
                "Failed to create user MIME applications directory {}: {error}",
                parent.display()
            )
        })?;
    }

    let contents = match fs::read_to_string(path) {
        Ok(contents) => contents,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => String::new(),
        Err(error) => {
            return Err(format!(
                "Failed to read user MIME applications file {}: {error}",
                path.display()
            ));
        }
    };
    let updated = update(&contents);

    fs::write(path, updated).map_err(|error| {
        format!(
            "Failed to write user MIME applications file {}: {error}",
            path.display()
        )
    })
}

#[derive(Debug, Clone, PartialEq, Eq)]
struct UserMimeappsPaths {
    default_path: PathBuf,
    association_path: PathBuf,
}

fn user_mimeapps_paths() -> Result<UserMimeappsPaths, String> {
    let config_home = env::var_os("XDG_CONFIG_HOME").map(PathBuf::from);
    let home = env::var_os("HOME").map(PathBuf::from);
    let desktops = current_desktops();

    Ok(UserMimeappsPaths {
        default_path: user_mimeapps_path_from(config_home.clone(), home.clone(), &desktops)?,
        association_path: user_association_mimeapps_path_from(config_home, home)?,
    })
}

fn user_mimeapps_path_from(
    config_home: Option<PathBuf>,
    home: Option<PathBuf>,
    desktops: &[String],
) -> Result<PathBuf, String> {
    let config_dir = user_config_dir(config_home, home)?;

    if let Some(desktop) = desktops.first() {
        return Ok(config_dir.join(format!("{desktop}-mimeapps.list")));
    }

    Ok(config_dir.join("mimeapps.list"))
}

fn user_association_mimeapps_path_from(
    config_home: Option<PathBuf>,
    home: Option<PathBuf>,
) -> Result<PathBuf, String> {
    let config_dir = user_config_dir(config_home, home)?;
    Ok(config_dir.join("mimeapps.list"))
}

fn user_config_dir(config_home: Option<PathBuf>, home: Option<PathBuf>) -> Result<PathBuf, String> {
    config_home
        .filter(|path| !path.as_os_str().is_empty())
        .or_else(|| {
            home.filter(|path| !path.as_os_str().is_empty())
                .map(|home| home.join(".config"))
        })
        .ok_or_else(|| "Cannot determine the user MIME applications file path".to_string())
}

fn validate_mimeapps_assignment(mime: &str, app_id: &str) -> Result<(), String> {
    let invalid_mime =
        mime.trim().is_empty() || mime.chars().any(|ch| matches!(ch, '\n' | '\r' | '=' | ';'));
    if invalid_mime {
        return Err("Invalid MIME type for default application".to_string());
    }

    let invalid_app_id = app_id.trim().is_empty()
        || app_id
            .chars()
            .any(|ch| matches!(ch, '\n' | '\r' | '=' | ';'));
    if invalid_app_id {
        return Err("Invalid desktop application id for default application".to_string());
    }

    Ok(())
}

fn update_mimeapps_contents(contents: &str, mime: &str, app_id: &str) -> String {
    let updated = update_mimeapps_default_contents(contents, mime, app_id);
    update_mimeapps_association_contents(&updated, mime, app_id)
}

fn update_mimeapps_default_contents(contents: &str, mime: &str, app_id: &str) -> String {
    upsert_mimeapps_section_key(contents, "Default Applications", mime, |existing| {
        desktop_list_with_first(app_id, existing)
    })
}

fn update_mimeapps_association_contents(contents: &str, mime: &str, app_id: &str) -> String {
    upsert_mimeapps_section_key(contents, "Added Associations", mime, |existing| {
        desktop_list_with_first(app_id, existing)
    })
}

fn desktop_list_with_first(app_id: &str, existing: Option<&str>) -> String {
    let mut ids = Vec::new();
    push_unique_string(&mut ids, app_id.to_string());

    for id in existing.unwrap_or_default().split(';') {
        push_unique_string(&mut ids, id.trim().to_string());
    }

    format!("{};", ids.join(";"))
}

fn upsert_mimeapps_section_key<F>(contents: &str, section: &str, key: &str, value_for: F) -> String
where
    F: Fn(Option<&str>) -> String,
{
    let section_header = format!("[{section}]");
    let mut lines = Vec::new();
    let mut found_section = false;
    let mut in_target_section = false;
    let mut inserted_key = false;

    for raw_line in contents.lines() {
        let trimmed = raw_line.trim();
        let is_section_header = trimmed.starts_with('[') && trimmed.ends_with(']');

        if is_section_header {
            if in_target_section && !inserted_key {
                lines.push(format!("{key}={}", value_for(None)));
                inserted_key = true;
            }

            in_target_section = trimmed == section_header;
            found_section |= in_target_section;
            lines.push(raw_line.to_string());
            continue;
        }

        if in_target_section {
            if let Some((line_key, line_value)) = trimmed.split_once('=') {
                if line_key.trim() == key {
                    if !inserted_key {
                        lines.push(format!("{key}={}", value_for(Some(line_value.trim()))));
                        inserted_key = true;
                    }
                    continue;
                }
            }
        }

        lines.push(raw_line.to_string());
    }

    if in_target_section && !inserted_key {
        lines.push(format!("{key}={}", value_for(None)));
    }

    if !found_section {
        if lines.last().is_some_and(|line| !line.is_empty()) {
            lines.push(String::new());
        }
        lines.push(section_header);
        lines.push(format!("{key}={}", value_for(None)));
    }

    let mut updated = lines.join("\n");
    updated.push('\n');
    updated
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
    let mime_score = app
        .mime_types
        .iter()
        .filter_map(|candidate| mime_match_score(candidate, mime))
        .max();

    mime_score
        .or_else(|| wps_app_mime_match_score(app, mime))
        .or_else(|| (is_text_editable_mime(mime) && app.text_editor).then_some(4))
}

fn mime_match_score(candidate: &str, mime: &str) -> Option<u8> {
    if candidate == mime {
        return Some(7);
    }

    if mimes_are_equivalent(candidate, mime) {
        return Some(6);
    }

    if is_text_editable_mime(mime) && candidate == "text/plain" {
        return Some(5);
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
    for alias in mime_aliases(mime) {
        if *alias != mime && !candidates.contains(alias) {
            candidates.push(alias);
        }
    }
    if is_text_editable_mime(mime) && mime != "text/plain" {
        candidates.push("text/plain");
    }
    candidates
}

fn mimes_are_equivalent(left: &str, right: &str) -> bool {
    left == right || office_mime_family(left).is_some_and(|family| family.contains(&right))
}

fn mime_aliases(mime: &str) -> &'static [&'static str] {
    office_mime_family(mime).unwrap_or(&[])
}

fn office_mime_family(mime: &str) -> Option<&'static [&'static str]> {
    [
        WPS_WRITER_MIME_FAMILY,
        WPS_SPREADSHEET_MIME_FAMILY,
        WPS_PRESENTATION_MIME_FAMILY,
    ]
    .into_iter()
    .find(|family| family.contains(&mime))
}

const WPS_WRITER_MIME_FAMILY: &[&str] = &[
    "application/msword",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    "application/wps-office.doc",
    "application/wps-office.docx",
    "application/wps-office.dot",
    "application/wps-office.dotx",
    "application/wps-office.wps",
    "application/wps-office.wpt",
];

const WPS_SPREADSHEET_MIME_FAMILY: &[&str] = &[
    "application/vnd.ms-excel",
    "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
    "application/wps-office.xls",
    "application/wps-office.xlsx",
    "application/wps-office.xlt",
    "application/wps-office.xltx",
    "application/wps-office.et",
    "application/wps-office.ett",
];

const WPS_PRESENTATION_MIME_FAMILY: &[&str] = &[
    "application/vnd.ms-powerpoint",
    "application/vnd.openxmlformats-officedocument.presentationml.presentation",
    "application/wps-office.ppt",
    "application/wps-office.pptx",
    "application/wps-office.pot",
    "application/wps-office.potx",
    "application/wps-office.dps",
    "application/wps-office.dpt",
];

fn wps_app_mime_match_score(app: &DesktopApp, mime: &str) -> Option<u8> {
    let family = office_mime_family(mime)?;
    (wps_app_mime_family(app) == Some(family)).then_some(4)
}

fn wps_app_mime_family(app: &DesktopApp) -> Option<&'static [&'static str]> {
    let id = app.id.to_ascii_lowercase();
    let name = app.name.to_ascii_lowercase();
    let exec = app.exec.to_ascii_lowercase();
    let program = split_exec(&app.exec)
        .ok()
        .and_then(|tokens| tokens.into_iter().next())
        .and_then(|program| {
            Path::new(&program)
                .file_name()
                .and_then(|name| name.to_str())
                .map(str::to_ascii_lowercase)
        })
        .unwrap_or_default();

    let looks_like_wps = id.contains("wps") || name.contains("wps") || exec.contains("kingsoft");
    if !looks_like_wps {
        return None;
    }

    if id.contains("wps-office-wps")
        || name.contains("wps writer")
        || program == "wps"
        || program == "wps.bin"
    {
        return Some(WPS_WRITER_MIME_FAMILY);
    }

    if id.contains("wps-office-et")
        || name.contains("wps spreadsheets")
        || name.contains("wps spreadsheet")
        || program == "et"
        || program == "et.bin"
    {
        return Some(WPS_SPREADSHEET_MIME_FAMILY);
    }

    if id.contains("wps-office-wpp")
        || name.contains("wps presentation")
        || program == "wpp"
        || program == "wpp.bin"
    {
        return Some(WPS_PRESENTATION_MIME_FAMILY);
    }

    None
}

fn is_text_editable_mime(mime: &str) -> bool {
    mime.starts_with("text/")
        || mime.ends_with("+xml")
        || mime.ends_with("+json")
        || matches!(
            mime,
            "application/ecmascript"
                | "application/javascript"
                | "application/json"
                | "application/toml"
                | "application/x-desktop"
                | "application/x-gnome-app-info"
                | "application/x-javascript"
                | "application/x-perl"
                | "application/x-php"
                | "application/x-ruby"
                | "application/x-shellscript"
                | "application/x-yaml"
                | "application/xml"
                | "application/yaml"
                | "image/svg+xml"
        )
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
    let mut terminal = false;
    let mut text_editor = false;

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
                    .map(str::trim)
                    .filter(|value| !value.is_empty())
                    .map(ToString::to_string)
                    .collect();
            }
            "Categories" => {
                text_editor = value
                    .split(';')
                    .map(str::trim)
                    .any(|category| category == "TextEditor");
            }
            "Hidden" => hidden = value.eq_ignore_ascii_case("true"),
            "Terminal" => terminal = value.eq_ignore_ascii_case("true"),
            "NoDisplay" => {}
            _ => {}
        }
    }

    if hidden || terminal || (!entry_type.is_empty() && entry_type != "Application") {
        return None;
    }

    Some(DesktopApp {
        id,
        name: name?,
        exec: exec?,
        mime_types,
        text_editor,
        icon: resolve_app_icon(icon.as_deref()),
    })
}

#[derive(Default)]
struct MimeApps {
    defaults: BTreeMap<String, Vec<String>>,
    added: BTreeMap<String, Vec<String>>,
    removed: BTreeMap<String, Vec<String>>,
}

fn load_mimeapps() -> MimeApps {
    let mut mimeapps = MimeApps::default();

    for candidate in mimeapps_paths() {
        let Ok(contents) = fs::read_to_string(&candidate.path) else {
            continue;
        };
        merge_mimeapps_contents(&mut mimeapps, &contents, candidate.allow_associations);
    }

    mimeapps
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum MimeAppsSection {
    DefaultApplications,
    AddedAssociations,
    RemovedAssociations,
    Other,
}

fn merge_mimeapps_contents(mimeapps: &mut MimeApps, contents: &str, allow_associations: bool) {
    let file = parse_mimeapps_contents(contents);

    for (mime, app_ids) in &file.defaults {
        mimeapps
            .defaults
            .entry(mime.clone())
            .or_insert_with(|| app_ids.clone());
        push_mimeapps_associations_unless_removed(mimeapps, mime, app_ids);
    }

    if allow_associations {
        for (mime, app_ids) in &file.added {
            push_mimeapps_associations_unless_removed(mimeapps, mime, app_ids);
        }

        for (mime, app_ids) in file.removed {
            push_mimeapps_entries(&mut mimeapps.removed, &mime, &app_ids);
        }
    }
}

fn parse_mimeapps_contents(contents: &str) -> MimeApps {
    let mut mimeapps = MimeApps::default();
    let mut section = MimeAppsSection::Other;

    for raw_line in contents.lines() {
        let line = raw_line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }

        if line.starts_with('[') && line.ends_with(']') {
            section = match line {
                "[Default Applications]" => MimeAppsSection::DefaultApplications,
                "[Added Associations]" => MimeAppsSection::AddedAssociations,
                "[Removed Associations]" => MimeAppsSection::RemovedAssociations,
                _ => MimeAppsSection::Other,
            };
            continue;
        }

        let Some((mime, apps)) = line.split_once('=') else {
            continue;
        };
        let mime = mime.trim();
        if mime.is_empty() {
            continue;
        }
        let app_ids = parse_desktop_id_list(apps);

        match section {
            MimeAppsSection::DefaultApplications => {
                mimeapps
                    .defaults
                    .entry(mime.to_string())
                    .or_insert_with(|| app_ids.clone());
            }
            MimeAppsSection::AddedAssociations => {
                push_mimeapps_entries(&mut mimeapps.added, mime, &app_ids);
            }
            MimeAppsSection::RemovedAssociations => {
                push_mimeapps_entries(&mut mimeapps.removed, mime, &app_ids);
            }
            MimeAppsSection::Other => {}
        }
    }

    mimeapps
}

fn parse_desktop_id_list(value: &str) -> Vec<String> {
    let mut ids = Vec::new();
    for id in value.split(';') {
        push_unique_string(&mut ids, id.trim().to_string());
    }
    ids
}

fn push_mimeapps_entries(
    entries: &mut BTreeMap<String, Vec<String>>,
    mime: &str,
    app_ids: &[String],
) {
    let mime_entries = entries.entry(mime.to_string()).or_default();
    for app_id in app_ids {
        push_unique_string(mime_entries, app_id.clone());
    }
}

fn push_mimeapps_associations_unless_removed(
    mimeapps: &mut MimeApps,
    mime: &str,
    app_ids: &[String],
) {
    let filtered = app_ids
        .iter()
        .filter(|app_id| !mimeapp_is_removed(mimeapps, mime, app_id))
        .cloned()
        .collect::<Vec<_>>();
    push_mimeapps_entries(&mut mimeapps.added, mime, &filtered);
}

fn mimeapp_is_removed(mimeapps: &MimeApps, mime: &str, app_id: &str) -> bool {
    mimeapps
        .removed
        .get(mime)
        .is_some_and(|removed| removed.iter().any(|removed_id| removed_id == app_id))
}

fn apply_mimeapps_associations(apps: &mut [DesktopApp], mimeapps: &MimeApps) {
    for app in apps.iter_mut() {
        for (mime, app_ids) in &mimeapps.removed {
            if app_ids.iter().any(|app_id| app_id == &app.id) {
                app.mime_types.retain(|candidate| candidate != mime);
            }
        }

        for (mime, app_ids) in &mimeapps.added {
            if app_ids.iter().any(|app_id| app_id == &app.id) {
                push_unique_string(&mut app.mime_types, mime.clone());
            }
        }
    }
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

#[derive(Debug, Clone, PartialEq, Eq)]
struct MimeAppsPath {
    path: PathBuf,
    allow_associations: bool,
}

fn mimeapps_paths() -> Vec<MimeAppsPath> {
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
        push_unique_path(&mut dirs, directory);
    }

    dirs
}

fn push_mimeapps_candidates(paths: &mut Vec<MimeAppsPath>, directory: &Path, desktops: &[String]) {
    for desktop in desktops {
        push_unique_mimeapps_candidate(
            paths,
            directory.join(format!("{desktop}-mimeapps.list")),
            false,
        );
    }
    push_unique_mimeapps_candidate(paths, directory.join("mimeapps.list"), true);
}

fn push_unique_mimeapps_candidate(
    paths: &mut Vec<MimeAppsPath>,
    path: PathBuf,
    allow_associations: bool,
) {
    if !paths.iter().any(|existing| existing.path == path) {
        paths.push(MimeAppsPath {
            path,
            allow_associations,
        });
    }
}

fn push_unique_path(paths: &mut Vec<PathBuf>, path: PathBuf) {
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
    let uri_value = if wps_app_mime_family(app).is_some() {
        path_value.as_ref().to_string()
    } else {
        file_uri(path)
    };
    let mut saw_file_code = false;
    let mut command = Vec::new();

    for token in split_exec(&app.exec)? {
        let (value, token_saw_file_code) =
            expand_exec_token(&token, &app.name, path_value.as_ref(), &uri_value);
        saw_file_code |= token_saw_file_code;

        if let Some(value) = value {
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

fn expand_exec_token(
    token: &str,
    app_name: &str,
    path_value: &str,
    uri_value: &str,
) -> (Option<String>, bool) {
    let mut value = String::with_capacity(token.len());
    let mut saw_file_code = false;
    let mut chars = token.chars();

    while let Some(ch) = chars.next() {
        if ch != '%' {
            value.push(ch);
            continue;
        }

        let Some(code) = chars.next() else {
            continue;
        };

        match code {
            '%' => value.push('%'),
            'c' => value.push_str(app_name),
            'f' | 'F' => {
                value.push_str(path_value);
                saw_file_code = true;
            }
            'u' | 'U' => {
                value.push_str(uri_value);
                saw_file_code = true;
            }
            'i' | 'k' | 'd' | 'D' | 'n' | 'N' | 'v' | 'm' => {}
            _ => {}
        }
    }

    ((!value.is_empty()).then_some(value), saw_file_code)
}

fn file_uri(path: &Path) -> String {
    let mut value = String::from("file://");

    for byte in path.as_os_str().as_bytes() {
        match *byte {
            b'A'..=b'Z' | b'a'..=b'z' | b'0'..=b'9' | b'/' | b'-' | b'_' | b'.' | b'~' => {
                value.push(*byte as char)
            }
            byte => value.push_str(&format!("%{byte:02X}")),
        }
    }

    value
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
    fn build_exec_command_replaces_file_field_codes() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor --new-window %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            text_editor: true,
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
            text_editor: false,
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a.txt")).unwrap();

        assert_eq!(command, vec!["viewer", "/tmp/a.txt"]);
    }

    #[test]
    fn build_exec_command_replaces_uri_field_codes_with_file_uri() {
        let app = DesktopApp {
            id: "viewer.desktop".to_string(),
            name: "Viewer".to_string(),
            exec: "viewer %u".to_string(),
            mime_types: vec!["text/plain".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a file#.txt")).unwrap();

        assert_eq!(command, vec!["viewer", "file:///tmp/a%20file%23.txt"]);
    }

    #[test]
    fn build_exec_command_uses_local_paths_for_wps_uri_field_codes() {
        let app = DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "/usr/bin/wps %U".to_string(),
            mime_types: vec!["application/wps-office.wps".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a file.wps")).unwrap();

        assert_eq!(command, vec!["/usr/bin/wps", "/tmp/a file.wps"]);
    }

    #[test]
    fn build_exec_command_removes_unsupported_field_codes() {
        let app = DesktopApp {
            id: "viewer.desktop".to_string(),
            name: "Viewer".to_string(),
            exec: "viewer --token=%2 %d --name=%c %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a.txt")).unwrap();

        assert_eq!(
            command,
            vec!["viewer", "--token=", "--name=Viewer", "/tmp/a.txt"]
        );
    }

    #[test]
    fn build_exec_command_keeps_escaped_percent_literals() {
        let app = DesktopApp {
            id: "viewer.desktop".to_string(),
            name: "Viewer".to_string(),
            exec: "viewer %% %%f %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };

        let command = build_exec_command(&app, Path::new("/tmp/a.txt")).unwrap();

        assert_eq!(command, vec!["viewer", "%", "%f", "/tmp/a.txt"]);
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
            text_editor: true,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![app],
            defaults: BTreeMap::new(),
        };

        assert!(default_app_for_mime(&registry, "text/x-log").is_none());
    }

    #[test]
    fn best_app_uses_highest_mime_match_when_no_default_exists() {
        let generic = DesktopApp {
            id: "generic.desktop".to_string(),
            name: "Generic Viewer".to_string(),
            exec: "generic %f".to_string(),
            mime_types: vec!["application/*".to_string()],
            text_editor: false,
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
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let wildcard = DesktopApp {
            id: "wildcard.desktop".to_string(),
            name: "Any File".to_string(),
            exec: "any %f".to_string(),
            mime_types: vec!["*/*".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![generic, writer, wildcard],
            defaults: BTreeMap::new(),
        };

        let apps = apps_for_mime(
            &registry,
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
        );

        assert_eq!(
            apps.first().map(|app| app.id.as_str()),
            Some("writer.desktop")
        );
        assert_eq!(
            best_app_for_mime(
                &registry,
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            )
            .map(|app| app.id),
            Some("writer.desktop".to_string())
        );
    }

    #[test]
    fn wps_office_alias_mimes_match_standard_office_mimes() {
        let writer = DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "wps %f".to_string(),
            mime_types: vec!["application/wps-office.wps".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![writer],
            defaults: BTreeMap::from([(
                "application/wps-office.wps".to_string(),
                vec!["wps-office-wps.desktop".to_string()],
            )]),
        };

        let standard_docx =
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document";

        assert_eq!(
            apps_for_mime(&registry, standard_docx)
                .first()
                .map(|app| app.id.as_str()),
            Some("wps-office-wps.desktop")
        );
        assert_eq!(
            default_app_for_mime(&registry, standard_docx).map(|app| app.id),
            Some("wps-office-wps.desktop".to_string())
        );
    }

    #[test]
    fn standard_office_mimes_match_wps_office_alias_mimes() {
        let writer = DesktopApp {
            id: "writer.desktop".to_string(),
            name: "Writer".to_string(),
            exec: "writer %f".to_string(),
            mime_types: vec![
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
                    .to_string(),
            ],
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![writer],
            defaults: BTreeMap::new(),
        };

        assert_eq!(
            apps_for_mime(&registry, "application/wps-office.wps")
                .first()
                .map(|app| app.id.as_str()),
            Some("writer.desktop")
        );
    }

    #[test]
    fn exact_wps_native_mime_beats_family_alias_candidate() {
        let exact_wps = DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "Z WPS Writer".to_string(),
            exec: "wps %f".to_string(),
            mime_types: vec!["application/wps-office.wps".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let standard_writer = DesktopApp {
            id: "writer.desktop".to_string(),
            name: "A Writer".to_string(),
            exec: "writer %f".to_string(),
            mime_types: vec![
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
                    .to_string(),
            ],
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![standard_writer, exact_wps],
            defaults: BTreeMap::new(),
        };

        assert_eq!(
            apps_for_mime(&registry, "application/wps-office.wps")
                .first()
                .map(|app| app.id.as_str()),
            Some("wps-office-wps.desktop")
        );
    }

    #[test]
    fn mimeapps_added_associations_make_apps_support_mimes() {
        let mut mimeapps = MimeApps::default();
        merge_mimeapps_contents(
            &mut mimeapps,
            "[Added Associations]\napplication/wps-office.wps=wps-office-wps.desktop;\n",
            true,
        );
        let mut apps = vec![DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "wps %f".to_string(),
            mime_types: Vec::new(),
            text_editor: false,
            icon: resolve_app_icon(None),
        }];

        apply_mimeapps_associations(&mut apps, &mimeapps);
        let registry = AppRegistry {
            apps,
            defaults: mimeapps.defaults,
        };

        assert_eq!(
            apps_for_mime(
                &registry,
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            )
            .first()
            .map(|app| app.id.as_str()),
            Some("wps-office-wps.desktop")
        );
    }

    #[test]
    fn mimeapps_default_entries_also_make_apps_support_mimes() {
        let mut mimeapps = MimeApps::default();
        merge_mimeapps_contents(
            &mut mimeapps,
            "[Default Applications]\napplication/wps-office.wps=wps-office-wps.desktop;\n",
            true,
        );
        let mut apps = vec![DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "wps %f".to_string(),
            mime_types: Vec::new(),
            text_editor: false,
            icon: resolve_app_icon(None),
        }];

        apply_mimeapps_associations(&mut apps, &mimeapps);
        let registry = AppRegistry {
            apps,
            defaults: mimeapps.defaults,
        };

        assert_eq!(
            default_app_for_mime(
                &registry,
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            )
            .map(|app| app.id),
            Some("wps-office-wps.desktop".to_string())
        );
    }

    #[test]
    fn mimeapps_removed_associations_remove_desktop_mimes() {
        let mut mimeapps = MimeApps::default();
        merge_mimeapps_contents(
            &mut mimeapps,
            "[Removed Associations]\napplication/wps-office.wps=wps-office-wps.desktop;\n",
            true,
        );
        let mut apps = vec![DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "wps %f".to_string(),
            mime_types: vec!["application/wps-office.wps".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        }];

        apply_mimeapps_associations(&mut apps, &mimeapps);

        assert!(apps[0].mime_types.is_empty());
    }

    #[test]
    fn higher_priority_added_association_survives_lower_priority_removed_association() {
        let mut mimeapps = MimeApps::default();
        merge_mimeapps_contents(
            &mut mimeapps,
            "[Added Associations]\napplication/wps-office.wps=wps-office-wps.desktop;\n",
            true,
        );
        merge_mimeapps_contents(
            &mut mimeapps,
            "[Removed Associations]\napplication/wps-office.wps=wps-office-wps.desktop;\n",
            true,
        );
        let mut apps = vec![DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "wps %f".to_string(),
            mime_types: Vec::new(),
            text_editor: false,
            icon: resolve_app_icon(None),
        }];

        apply_mimeapps_associations(&mut apps, &mimeapps);

        assert_eq!(apps[0].mime_types, vec!["application/wps-office.wps"]);
    }

    #[test]
    fn wps_writer_app_name_fallback_handles_missing_mime_declarations() {
        let writer = DesktopApp {
            id: "wps-office-wps.desktop".to_string(),
            name: "WPS Writer".to_string(),
            exec: "/usr/bin/wps %f".to_string(),
            mime_types: Vec::new(),
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![writer],
            defaults: BTreeMap::new(),
        };

        assert_eq!(
            apps_for_mime(
                &registry,
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            )
            .first()
            .map(|app| app.id.as_str()),
            Some("wps-office-wps.desktop")
        );
        assert!(apps_for_mime(&registry, "application/vnd.ms-excel").is_empty());
    }

    #[test]
    fn desktop_specific_mimeapps_ignores_added_and_removed_associations() {
        let mut mimeapps = MimeApps::default();
        merge_mimeapps_contents(
            &mut mimeapps,
            "[Default Applications]\napplication/wps-office.docx=wps-office-prometheus.desktop;\n\n[Added Associations]\napplication/wps-office.docx=wps-office-wps.desktop;\n\n[Removed Associations]\napplication/wps-office.wps=wps-office-wps.desktop;\n",
            false,
        );

        assert_eq!(
            mimeapps.defaults.get("application/wps-office.docx"),
            Some(&vec!["wps-office-prometheus.desktop".to_string()])
        );
        assert!(mimeapps.added.contains_key("application/wps-office.docx"));
        assert!(!mimeapps
            .added
            .get("application/wps-office.docx")
            .is_some_and(|apps| apps.iter().any(|app| app == "wps-office-wps.desktop")));
        assert!(!mimeapps.removed.contains_key("application/wps-office.wps"));
    }

    #[test]
    fn wps_office_alias_table_covers_common_office_formats() {
        let families = [
            (
                "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
                WPS_WRITER_MIME_FAMILY,
            ),
            (
                "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
                WPS_SPREADSHEET_MIME_FAMILY,
            ),
            (
                "application/vnd.openxmlformats-officedocument.presentationml.presentation",
                WPS_PRESENTATION_MIME_FAMILY,
            ),
        ];

        for (standard, family) in families {
            let candidates = default_mime_candidates(standard);
            for mime in family {
                assert!(mimes_are_equivalent(standard, mime));
                assert!(mimes_are_equivalent(mime, standard));
                assert!(candidates.contains(mime));
            }
        }
    }

    #[test]
    fn text_plain_apps_can_open_specialized_text_files() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            text_editor: true,
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
            default_app_for_mime(&registry, "text/x-makefile").map(|app| app.id),
            Some("editor.desktop".to_string())
        );
        assert_eq!(apps_for_mime(&registry, "text/x-makefile").len(), 1);
        assert_eq!(
            default_app_for_mime(&registry, "text/markdown").map(|app| app.id),
            Some("editor.desktop".to_string())
        );
        assert_eq!(
            default_app_for_mime(&registry, "application/json").map(|app| app.id),
            Some("editor.desktop".to_string())
        );
        assert_eq!(
            default_app_for_mime(&registry, "application/x-desktop").map(|app| app.id),
            Some("editor.desktop".to_string())
        );
    }

    #[test]
    fn text_editor_category_can_open_text_like_files_without_mime_match() {
        let editor = DesktopApp {
            id: "code.desktop".to_string(),
            name: "Code".to_string(),
            exec: "code %F".to_string(),
            mime_types: vec!["application/x-code-workspace".to_string()],
            text_editor: true,
            icon: resolve_app_icon(None),
        };
        let generic = DesktopApp {
            id: "generic.desktop".to_string(),
            name: "Generic".to_string(),
            exec: "generic %f".to_string(),
            mime_types: vec!["application/*".to_string()],
            text_editor: false,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![generic, editor.clone()],
            defaults: BTreeMap::new(),
        };

        assert_eq!(
            best_app_for_mime(&registry, "application/x-desktop").map(|app| app.id),
            Some("code.desktop".to_string())
        );

        let registry = AppRegistry {
            apps: vec![editor],
            defaults: BTreeMap::new(),
        };

        assert!(best_app_for_mime(&registry, "application/octet-stream").is_none());
    }

    #[test]
    fn application_octet_stream_default_still_requires_matching_app_support() {
        let app = DesktopApp {
            id: "editor.desktop".to_string(),
            name: "Editor".to_string(),
            exec: "editor %f".to_string(),
            mime_types: vec!["text/plain".to_string()],
            text_editor: true,
            icon: resolve_app_icon(None),
        };
        let registry = AppRegistry {
            apps: vec![app],
            defaults: BTreeMap::from([(
                "application/octet-stream".to_string(),
                vec!["editor.desktop".to_string()],
            )]),
        };

        assert!(default_app_for_mime(&registry, "application/octet-stream").is_none());
        assert!(best_app_for_mime(&registry, "application/octet-stream").is_none());
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
                MimeAppsPath {
                    path: PathBuf::from("/etc/xdg/gnome-mimeapps.list"),
                    allow_associations: false
                },
                MimeAppsPath {
                    path: PathBuf::from("/etc/xdg/unity-mimeapps.list"),
                    allow_associations: false
                },
                MimeAppsPath {
                    path: PathBuf::from("/etc/xdg/mimeapps.list"),
                    allow_associations: true
                },
            ]
        );
    }

    #[test]
    fn update_mimeapps_contents_adds_default_and_association_sections() {
        let updated = update_mimeapps_contents("", "text/plain", "editor.desktop");

        assert_eq!(
            updated,
            "[Default Applications]\ntext/plain=editor.desktop;\n\n[Added Associations]\ntext/plain=editor.desktop;\n"
        );
    }

    #[test]
    fn update_mimeapps_contents_moves_selected_app_to_front() {
        let contents = "# user defaults\n[Default Applications]\ntext/plain=viewer.desktop;editor.desktop;\nimage/png=image-viewer.desktop;\n\n[Added Associations]\ntext/plain=viewer.desktop;\n";

        let updated = update_mimeapps_contents(contents, "text/plain", "editor.desktop");

        assert!(updated.contains("# user defaults\n"));
        assert!(updated.contains("text/plain=editor.desktop;viewer.desktop;\n"));
        assert!(updated.contains("image/png=image-viewer.desktop;\n"));
        assert!(
            updated.contains("[Added Associations]\ntext/plain=editor.desktop;viewer.desktop;\n")
        );
    }

    #[test]
    fn set_default_app_for_mime_at_creates_user_file() {
        let root = temp_dir("mimeapps-write");
        let path = root.join("config/mimeapps.list");

        set_default_app_for_mime_at(&path, &path, "application/pdf", "viewer.desktop").unwrap();

        let contents = fs::read_to_string(&path).unwrap();
        assert!(contents.contains("[Default Applications]\napplication/pdf=viewer.desktop;\n"));
        assert!(contents.contains("[Added Associations]\napplication/pdf=viewer.desktop;\n"));

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn set_default_app_for_mime_at_splits_desktop_default_and_generic_association() {
        let root = temp_dir("mimeapps-split-write");
        let default_path = root.join("config/ubuntu-mimeapps.list");
        let association_path = root.join("config/mimeapps.list");

        set_default_app_for_mime_at(
            &default_path,
            &association_path,
            "application/pdf",
            "viewer.desktop",
        )
        .unwrap();

        let default_contents = fs::read_to_string(&default_path).unwrap();
        assert!(
            default_contents.contains("[Default Applications]\napplication/pdf=viewer.desktop;\n")
        );
        assert!(!default_contents.contains("[Added Associations]"));

        let association_contents = fs::read_to_string(&association_path).unwrap();
        assert!(association_contents
            .contains("[Added Associations]\napplication/pdf=viewer.desktop;\n"));
        assert!(!association_contents.contains("[Default Applications]"));

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn user_mimeapps_path_prefers_xdg_config_home() {
        assert_eq!(
            user_mimeapps_path_from(
                Some(PathBuf::from("/tmp/config")),
                Some(PathBuf::from("/tmp/home")),
                &[]
            )
            .unwrap(),
            PathBuf::from("/tmp/config/mimeapps.list")
        );

        assert_eq!(
            user_mimeapps_path_from(None, Some(PathBuf::from("/tmp/home")), &[]).unwrap(),
            PathBuf::from("/tmp/home/.config/mimeapps.list")
        );
    }

    #[test]
    fn user_mimeapps_path_prefers_current_desktop_file() {
        assert_eq!(
            user_mimeapps_path_from(
                Some(PathBuf::from("/tmp/config")),
                None,
                &["gnome".to_string(), "unity".to_string()]
            )
            .unwrap(),
            PathBuf::from("/tmp/config/gnome-mimeapps.list")
        );
    }

    #[test]
    fn user_association_mimeapps_path_always_uses_generic_file() {
        assert_eq!(
            user_association_mimeapps_path_from(
                Some(PathBuf::from("/tmp/config")),
                Some(PathBuf::from("/tmp/home")),
            )
            .unwrap(),
            PathBuf::from("/tmp/config/mimeapps.list")
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
        assert!(!app.text_editor);
        assert!(matches!(app.icon, crate::model::EntryIcon::Embedded(_)));

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn desktop_app_parser_trims_mime_types_and_detects_text_editor_category() {
        let root = temp_dir("desktop-app-trim");
        fs::create_dir_all(&root).unwrap();
        let path = root.join("editor.desktop");
        fs::write(
            &path,
            "[Desktop Entry]\nType=Application\nName=Editor\nExec=editor %F\nMimeType= text/x-c++src; text/markdown;\nCategories=Utility;TextEditor;\n",
        )
        .unwrap();

        let app = parse_desktop_app("editor.desktop".to_string(), &path).unwrap();

        assert_eq!(app.mime_types, vec!["text/x-c++src", "text/markdown"]);
        assert!(app.text_editor);

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn terminal_desktop_apps_are_ignored() {
        let root = temp_dir("desktop-app-terminal");
        fs::create_dir_all(&root).unwrap();
        let path = root.join("vim.desktop");
        fs::write(
            &path,
            "[Desktop Entry]\nType=Application\nName=Vim\nExec=vim %F\nTerminal=true\nMimeType=text/plain;\nCategories=Utility;TextEditor;\n",
        )
        .unwrap();

        assert!(parse_desktop_app("vim.desktop".to_string(), &path).is_none());

        fs::remove_dir_all(root).unwrap();
    }
}
