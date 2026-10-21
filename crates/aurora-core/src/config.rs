use std::path::Path;

use serde::{Deserialize, Serialize};

use crate::error::{AuroraError, Result};

/// Configuration file that is loaded when no explicit path is given.
pub const DEFAULT_CONFIG_FILE: &str = "config.toml";

fn read_config_file(path: &Path) -> Result<Config> {
    let content = std::fs::read_to_string(path).map_err(|e| {
        AuroraError::Config(format!("failed to read {}: {}", path.display(), e))
    })?;
    toml::from_str(&content)
        .map_err(|e| AuroraError::Config(format!("failed to parse {}: {}", path.display(), e)))
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Config {
    #[serde(default = "default_server")]
    pub server: ServerConfig,
    #[serde(default = "default_plugins")]
    pub plugins: PluginsConfig,
    #[serde(default = "default_logging")]
    pub logging: LoggingConfig,
    #[serde(default = "default_media")]
    pub media: MediaConfig,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ServerConfig {
    #[serde(default = "default_host")]
    pub host: String,
    #[serde(default = "default_http_port")]
    pub http_port: u16,
    #[serde(default = "default_rtmp_port")]
    pub rtmp_port: u16,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct PluginsConfig {
    #[serde(default = "default_plugin_dir")]
    pub directory: String,
    #[serde(default = "default_true")]
    pub auto_load: bool,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct LoggingConfig {
    #[serde(default = "default_log_level")]
    pub level: String,
    #[serde(default = "default_log_format")]
    pub format: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct MediaConfig {
    #[serde(default = "default_video_codec")]
    pub video_codec: String,
    #[serde(default = "default_audio_codec")]
    pub audio_codec: String,
    #[serde(default = "default_max_bitrate")]
    pub max_bitrate: u32,
    #[serde(default = "default_gop_size")]
    pub gop_size: u32,
}

fn default_server() -> ServerConfig {
    ServerConfig {
        host: default_host(),
        http_port: default_http_port(),
        rtmp_port: default_rtmp_port(),
    }
}
fn default_host() -> String {
    "0.0.0.0".to_string()
}
fn default_http_port() -> u16 {
    8080
}
fn default_rtmp_port() -> u16 {
    1935
}

fn default_plugins() -> PluginsConfig {
    PluginsConfig {
        directory: default_plugin_dir(),
        auto_load: default_true(),
    }
}
fn default_plugin_dir() -> String {
    "./plugins".to_string()
}
fn default_true() -> bool {
    true
}

fn default_logging() -> LoggingConfig {
    LoggingConfig {
        level: default_log_level(),
        format: default_log_format(),
    }
}
fn default_log_level() -> String {
    "info".to_string()
}
fn default_log_format() -> String {
    "pretty".to_string()
}

fn default_media() -> MediaConfig {
    MediaConfig {
        video_codec: default_video_codec(),
        audio_codec: default_audio_codec(),
        max_bitrate: default_max_bitrate(),
        gop_size: default_gop_size(),
    }
}
fn default_video_codec() -> String {
    "h264".to_string()
}
fn default_audio_codec() -> String {
    "aac".to_string()
}
fn default_max_bitrate() -> u32 {
    4_000_000
}
fn default_gop_size() -> u32 {
    30
}

impl Config {
    pub fn load(path: Option<&str>) -> Result<Self> {
        let mut config: Config = if let Some(p) = path {
            read_config_file(Path::new(p))?
        } else if Path::new(DEFAULT_CONFIG_FILE).exists() {
            // Pick up the configuration file that ships with the server so editing it
            // takes effect without having to pass `--config`.
            read_config_file(Path::new(DEFAULT_CONFIG_FILE))?
        } else {
            Self::default_config()
        };

        config.apply_env_overrides();
        config.apply_cli_args();
        Ok(config)
    }

    fn apply_env_overrides(&mut self) {
        if let Ok(val) = std::env::var("AURORA_HOST") {
            self.server.host = val;
        }
        if let Ok(val) = std::env::var("AURORA_HTTP_PORT") {
            if let Ok(port) = val.parse() {
                self.server.http_port = port;
            }
        }
        if let Ok(val) = std::env::var("AURORA_RTMP_PORT") {
            if let Ok(port) = val.parse() {
                self.server.rtmp_port = port;
            }
        }
        if let Ok(val) = std::env::var("AURORA_LOG_LEVEL") {
            self.logging.level = val;
        }
        if let Ok(val) = std::env::var("AURORA_PLUGIN_DIR") {
            self.plugins.directory = val;
        }
    }

    fn apply_cli_args(&mut self) {
        let args: Vec<String> = std::env::args().collect();
        let mut i = 1;
        while i < args.len() {
            match args[i].as_str() {
                "--host" => {
                    if let Some(val) = args.get(i + 1) {
                        self.server.host = val.clone();
                        i += 2;
                    } else {
                        i += 1;
                    }
                }
                "--http-port" => {
                    if let Some(val) = args.get(i + 1) {
                        if let Ok(port) = val.parse() {
                            self.server.http_port = port;
                        }
                        i += 2;
                    } else {
                        i += 1;
                    }
                }
                "--rtmp-port" => {
                    if let Some(val) = args.get(i + 1) {
                        if let Ok(port) = val.parse() {
                            self.server.rtmp_port = port;
                        }
                        i += 2;
                    } else {
                        i += 1;
                    }
                }
                "--plugin-dir" => {
                    if let Some(val) = args.get(i + 1) {
                        self.plugins.directory = val.clone();
                        i += 2;
                    } else {
                        i += 1;
                    }
                }
                _ => {
                    i += 1;
                }
            }
        }
    }

    fn default_config() -> Self {
        Config {
            server: default_server(),
            plugins: default_plugins(),
            logging: default_logging(),
            media: default_media(),
        }
    }
}

impl Default for Config {
    fn default() -> Self {
        Self::default_config()
    }
}
