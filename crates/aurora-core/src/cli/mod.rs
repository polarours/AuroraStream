pub mod client;
pub mod commands;

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(
    name = "aurora",
    version,
    about = "AuroraStream - Real-time streaming engine"
)]
pub struct Cli {
    /// Engine API endpoint the client commands talk to
    #[arg(short, long, global = true, default_value = "http://127.0.0.1:8080")]
    pub api: String,

    #[command(subcommand)]
    pub command: Commands,
}

#[derive(Subcommand)]
pub enum Commands {
    /// Start the streaming engine daemon
    Serve {
        /// Path to configuration file
        #[arg(short, long)]
        config: Option<String>,
        /// Port for the HTTP API
        #[arg(short, long)]
        port: Option<u16>,
    },
    /// Connect to a running engine instance (interactive TUI)
    Cli,
    /// Query engine status (suitable for scripting)
    Status {
        /// Output in JSON format
        #[arg(short, long)]
        json: bool,
    },
    /// Plugin management
    Plugin {
        #[command(subcommand)]
        action: PluginAction,
    },
    /// Configuration management
    Config {
        #[command(subcommand)]
        action: ConfigAction,
    },
    /// View engine logs
    Logs {
        /// Follow log output
        #[arg(short, long)]
        follow: bool,
        /// Minimum log level
        #[arg(short, long)]
        level: Option<String>,
    },
}

#[derive(Subcommand)]
pub enum PluginAction {
    /// List installed plugins
    List {
        /// Show only loaded plugins
        #[arg(long)]
        loaded: bool,
        /// Show only available plugins
        #[arg(long)]
        available: bool,
    },
    /// Install a plugin
    Install {
        /// Plugin identifier
        name: String,
    },
    /// Remove a plugin
    Remove {
        /// Plugin identifier
        name: String,
    },
    /// Enable a plugin
    Enable {
        /// Plugin identifier
        name: String,
    },
    /// Disable a plugin
    Disable {
        /// Plugin identifier
        name: String,
    },
}

#[derive(Subcommand)]
pub enum ConfigAction {
    /// Show current configuration
    Show,
    /// Set a configuration value
    Set {
        /// Configuration key
        key: String,
        /// Configuration value
        value: String,
    },
    /// Get a configuration value
    Get {
        /// Configuration key
        key: String,
    },
}
