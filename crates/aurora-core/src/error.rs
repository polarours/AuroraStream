use thiserror::Error;

#[derive(Error, Debug)]
pub enum AuroraError {
    #[error("plugin error: {0}")]
    Plugin(String),

    #[error("plugin not found: {0}")]
    PluginNotFound(String),

    #[error("plugin load failed: {0}")]
    PluginLoadFailed(String),

    #[error("event bus error: {0}")]
    EventBus(String),

    #[error("media error: {0}")]
    Media(String),

    #[error("config error: {0}")]
    Config(String),

    #[error("network error: {0}")]
    Network(String),

    #[error("io error: {0}")]
    Io(#[from] std::io::Error),

    #[error("ffi error: {0}")]
    Ffi(String),

    #[error("timeout")]
    Timeout,

    #[error("insufficient resources: {0}")]
    InsufficientResources(String),

    #[error("unsupported: {0}")]
    Unsupported(String),

    #[error("{0}")]
    Other(String),
}

pub type Result<T> = std::result::Result<T, AuroraError>;
