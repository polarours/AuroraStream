use std::path::{Path, PathBuf};

use libloading::Library;

use crate::error::{AuroraError, Result};
use crate::plugin::Plugin;

pub struct PluginLoader {
    libraries: Vec<Library>,
}

impl PluginLoader {
    pub fn new() -> Self {
        Self {
            libraries: Vec::new(),
        }
    }

    pub fn load_from_dir(&mut self, dir: &str) -> Result<Vec<Box<dyn Plugin>>> {
        let plugin_dir = Path::new(dir);
        if !plugin_dir.exists() {
            tracing::warn!("plugin directory does not exist: {}", dir);
            return Ok(vec![]);
        }

        let mut plugins = Vec::new();

        let entries: Vec<PathBuf> = std::fs::read_dir(plugin_dir)
            .map_err(|e| {
                AuroraError::PluginLoadFailed(format!("failed to read plugin dir: {}", e))
            })?
            .filter_map(|e| e.ok())
            .filter(|e| {
                let path = e.path();
                let is_lib = path
                    .extension()
                    .map(|ext| matches!(ext.to_str(), Some("so" | "dylib" | "dll")))
                    .unwrap_or(false);
                is_lib
            })
            .map(|e| e.path())
            .collect();

        for path in entries {
            match self.load_single(&path) {
                Ok(plugin) => {
                    tracing::info!("loaded plugin: {} v{}", plugin.name(), plugin.version());
                    plugins.push(plugin);
                }
                Err(e) => {
                    tracing::error!("failed to load plugin {}: {}", path.display(), e);
                }
            }
        }

        Ok(plugins)
    }

    fn load_single(&mut self, path: &Path) -> Result<Box<dyn Plugin>> {
        unsafe {
            let library = Library::new(path)
                .map_err(|e| AuroraError::PluginLoadFailed(format!("{}: {}", path.display(), e)))?;

            let create_fn: libloading::Symbol<unsafe extern "C" fn() -> *mut dyn Plugin> =
                library.get(b"aurora_plugin_create").map_err(|e| {
                    AuroraError::PluginLoadFailed(format!(
                        "{}: missing aurora_plugin_create: {}",
                        path.display(),
                        e
                    ))
                })?;

            let plugin_ptr = create_fn();
            let plugin = Box::from_raw(plugin_ptr);

            self.libraries.push(library);

            Ok(plugin)
        }
    }
}

impl Drop for PluginLoader {
    fn drop(&mut self) {
        // A plugin hands the host callbacks (HTTP handlers, vtable backed trait
        // objects) whose vtables point into the shared object. Unloading the library
        // while the host still holds such a callback leaves dangling vtable pointers,
        // and dropping them afterwards crashes. The libraries therefore stay mapped
        // for the lifetime of the process.
        for library in self.libraries.drain(..) {
            std::mem::forget(library);
        }
    }
}

impl Default for PluginLoader {
    fn default() -> Self {
        Self::new()
    }
}
