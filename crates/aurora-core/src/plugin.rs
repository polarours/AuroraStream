use std::fmt;
use std::sync::Arc;

use crate::error::Result;
use crate::event_bus::Event;

/// Severity of a log message emitted by a plugin.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogLevel {
    Error,
    Warn,
    Info,
    Debug,
    Trace,
}

/// Sink the host hands to its plugins so their log output reaches the host subscriber.
///
/// A plugin is a separate shared object and therefore carries its own copy of the
/// `tracing` crate. Its global subscriber is not the host's, so logging from a plugin
/// through `tracing` directly would be invisible in the host log. Routing every plugin
/// message through this sink keeps the whole server on one log stream.
pub type LogSink = Arc<dyn Fn(LogLevel, &str) + Send + Sync>;

pub struct PluginContext {
    pub event_bus: Arc<crate::event_bus::EventBus>,
    pub memory_pool: Arc<crate::memory_pool::MemoryPool>,
    pub config: crate::config::Config,
    /// Host provided logging sink.
    pub log: LogSink,
}

impl fmt::Debug for PluginContext {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("PluginContext")
            .field("event_bus", &"EventBus")
            .field("memory_pool", &"MemoryPool")
            .field("config", &self.config)
            .finish_non_exhaustive()
    }
}

impl Clone for PluginContext {
    fn clone(&self) -> Self {
        Self {
            event_bus: Arc::clone(&self.event_bus),
            memory_pool: Arc::clone(&self.memory_pool),
            config: self.config.clone(),
            log: Arc::clone(&self.log),
        }
    }
}

impl PluginContext {
    pub fn log(&self, level: LogLevel, message: &str) {
        (self.log)(level, message);
    }

    pub fn error(&self, message: impl AsRef<str>) {
        self.log(LogLevel::Error, message.as_ref());
    }

    pub fn warn(&self, message: impl AsRef<str>) {
        self.log(LogLevel::Warn, message.as_ref());
    }

    pub fn info(&self, message: impl AsRef<str>) {
        self.log(LogLevel::Info, message.as_ref());
    }

    pub fn debug(&self, message: impl AsRef<str>) {
        self.log(LogLevel::Debug, message.as_ref());
    }
}

pub struct HttpRoute {
    pub method: String,
    pub path: String,
    pub handler: HttpHandler,
}

pub type HttpHandler = Arc<
    dyn Fn(http::Request<axum::body::Body>) -> http::Response<axum::body::Body>
        + Send
        + Sync
        + 'static,
>;

pub trait Plugin: Send + Sync + fmt::Debug {
    fn name(&self) -> &str;
    fn version(&self) -> &str;

    fn on_load(&mut self, ctx: &PluginContext) -> Result<()>;
    fn on_start(&mut self) -> Result<()>;
    fn on_stop(&mut self) -> Result<()>;
    fn on_unload(&mut self) -> Result<()>;

    fn on_event(&mut self, event: &Event) -> Result<()>;

    fn routes(&self) -> Vec<HttpRoute> {
        vec![]
    }
}

pub type PluginFactory = Box<dyn Fn() -> Box<dyn Plugin> + Send + Sync>;
