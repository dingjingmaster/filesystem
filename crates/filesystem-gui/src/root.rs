use std::ffi::OsString;
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct RootScope {
    root: PathBuf,
}

impl RootScope {
    pub(crate) fn from_env_args() -> Result<Option<Self>, String> {
        Self::from_args(std::env::args_os())
    }

    pub(crate) fn from_args<I>(args: I) -> Result<Option<Self>, String>
    where
        I: IntoIterator<Item = OsString>,
    {
        let mut args = args.into_iter();
        let _ = args.next();

        while let Some(arg) = args.next() {
            if arg == "--root" {
                let Some(root) = args.next() else {
                    return Err("--root requires a directory".to_string());
                };
                return Self::new(PathBuf::from(root)).map(Some);
            }
        }

        Ok(None)
    }

    pub(crate) fn new(path: PathBuf) -> Result<Self, String> {
        let root = path
            .canonicalize()
            .map_err(|error| format!("Invalid root: {error}"))?;
        if !root.is_dir() {
            return Err("Root is not a directory".to_string());
        }
        Ok(Self { root })
    }

    #[cfg(test)]
    pub(crate) fn new_for_test(root: PathBuf) -> Self {
        Self { root }
    }

    pub(crate) fn root(&self) -> &Path {
        &self.root
    }

    pub(crate) fn contains_existing(&self, path: &Path) -> bool {
        path.canonicalize()
            .map(|path| path.starts_with(&self.root))
            .unwrap_or(false)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn args(values: &[&str]) -> Vec<OsString> {
        values.iter().map(OsString::from).collect()
    }

    #[test]
    fn parses_root_argument() {
        let root = std::env::temp_dir().canonicalize().unwrap();
        let scope =
            RootScope::from_args(args(&["filesystem-gui", "--root", root.to_str().unwrap()]))
                .unwrap()
                .unwrap();

        assert_eq!(scope.root(), root.as_path());
    }

    #[test]
    fn rejects_missing_root_value() {
        let error = RootScope::from_args(args(&["filesystem-gui", "--root"])).unwrap_err();

        assert_eq!(error, "--root requires a directory");
    }

    #[test]
    fn checks_existing_paths_under_root() {
        let root = std::env::temp_dir().canonicalize().unwrap();
        let scope = RootScope::new_for_test(root.clone());

        assert!(scope.contains_existing(&root));
        assert!(!scope.contains_existing(Path::new("/")));
    }
}
