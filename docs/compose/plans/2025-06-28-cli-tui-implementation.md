# AuroraStream CLI + TUI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use compose:subagent (recommended) or compose:execute to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a powerful CLI tool with interactive TUI dashboard to AuroraStream, enabling both geek-style terminal control and real-time monitoring.

**Architecture:** Single binary `aurora` with clap subcommands. CLI mode connects to a running `aurora serve` process via HTTP API. TUI built with ratatui + crossterm for cross-platform terminal UI. Shared engine process, CLI is a client.

**Tech Stack:** clap (CLI), ratatui + crossterm (TUI), reqwest (HTTP client for CLI-to-engine communication), serde_json (JSON parsing)

---

### Task 1: CLI Framework Setup

**Covers:** [S2, S5]
**Files:**
- Modify: `crates/aurora-core/Cargo.toml`
- Modify: `crates/aurora-core/src/main.rs`
- Create: `crates/aurora-core/src/cli/mod.rs`
- Create: `crates/aurora-core/src/cli/commands.rs`

- [ ] **Step 1: Add dependencies**

Add to `crates/aurora-core/Cargo.toml`:
```toml
clap = { workspace = true }
```

- [ ] **Step 2: Create CLI module**

Create `crates/aurora-core/src/cli/mod.rs`:
```rust
pub mod commands;

use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "aurora", about = "AuroraStream - Lightweight Plugin-based Streaming Server")]
pub struct Cli {
    #[command(subcommand)]
    pub command: Option<Commands>,
}

#[derive(Subcommand)]
pub enum Commands {
    /// Start the streaming engine
    Serve {
        /// Config file path
        #[arg(short, long)]
        config: Option<String>,
        /// HTTP API port
        #[arg(long)]
        port: Option<u16>,
    },
    /// Interactive TUI console
    Cli {
        /// Engine API address
        #[arg(long, default_value = "http://127.0.0.1:8080")]
        api: String,
    },
    /// Show engine status
    Status {
        /// Output as JSON
        #[arg(long)]
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
    /// View logs
    Logs {
        /// Follow log output
        #[arg(short, long)]
        follow: bool,
        /// Filter by log level
        #[arg(long)]
        level: Option<String>,
    },
}

#[derive(Subcommand)]
pub enum PluginAction {
    /// List plugins
    List {
        /// Show only loaded plugins
        #[arg(long)]
        loaded: bool,
        /// Show only available plugins
        #[arg(long)]
        available: bool,
    },
    /// Install a plugin
    Install { name: String },
    /// Remove a plugin
    Remove { name: String },
    /// Enable a plugin
    Enable { name: String },
    /// Disable a plugin
    Disable { name: String },
}

#[derive(Subcommand)]
pub enum ConfigAction {
    /// Show current configuration
    Show,
    /// Set a configuration value
    Set { key: String, value: String },
    /// Get a configuration value
    Get { key: String },
}
```

- [ ] **Step 3: Update main.rs**

Replace `crates/aurora-core/src/main.rs`:
```rust
use clap::Parser;
use aurora_core::cli::{Cli, Commands};

#[tokio::main]
async fn main() {
    let cli = Cli::parse();

    match cli.command {
        Some(Commands::Serve { config, port }) => {
            let mut cfg = aurora_core::Config::load(config.as_deref())
                .unwrap_or_else(|e| {
                    eprintln!("failed to load config: {}, using defaults", e);
                    aurora_core::Config::default()
                });
            if let Some(p) = port {
                cfg.server.http_port = p;
            }
            let mut engine = aurora_core::Engine::new(cfg);
            if let Err(e) = engine.run().await {
                eprintln!("fatal error: {}", e);
                std::process::exit(1);
            }
        }
        Some(Commands::Cli { api }) => {
            aurora_core::cli::commands::run_tui(&api).await;
        }
        Some(Commands::Status { json }) => {
            aurora_core::cli::commands::show_status(json).await;
        }
        Some(Commands::Plugin { action }) => {
            aurora_core::cli::commands::handle_plugin(action).await;
        }
        Some(Commands::Config { action }) => {
            aurora_core::cli::commands::handle_config(action).await;
        }
        Some(Commands::Logs { follow, level }) => {
            aurora_core::cli::commands::show_logs(follow, level).await;
        }
        None => {
            // Default: start server
            let mut engine = aurora_core::Engine::new(
                aurora_core::Config::default()
            );
            if let Err(e) = engine.run().await {
                eprintln!("fatal error: {}", e);
                std::process::exit(1);
            }
        }
    }
}
```

- [ ] **Step 4: Create stub commands module**

Create `crates/aurora-core/src/cli/commands.rs`:
```rust
use super::commands;
use super::{PluginAction, ConfigAction};

pub async fn run_tui(api: &str) {
    eprintln!("TUI mode connecting to: {}", api);
    eprintln!("Not yet implemented");
}

pub async fn show_status(json: bool) {
    eprintln!("Status mode (json={})", json);
    eprintln!("Not yet implemented");
}

pub async fn handle_plugin(action: PluginAction) {
    eprintln!("Plugin management");
    eprintln!("Not yet implemented");
}

pub async fn handle_config(action: ConfigAction) {
    eprintln!("Config management");
    eprintln!("Not yet implemented");
}

pub async fn show_logs(follow: bool, level: Option<String>) {
    eprintln!("Logs mode (follow={}, level={:?})", follow, level);
    eprintln!("Not yet implemented");
}
```

- [ ] **Step 5: Add cli module to lib.rs**

Add to `crates/aurora-core/src/lib.rs`:
```rust
pub mod cli;
```

- [ ] **Step 6: Build and verify CLI**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

- [ ] **Step 7: Test CLI help**

Run: `cargo run -p aurora-core -- --help`
Expected: Shows aurora help with subcommands

---

### Task 2: HTTP Client for CLI-to-Engine Communication

**Covers:** [S6]
**Files:**
- Modify: `crates/aurora-core/Cargo.toml`
- Create: `crates/aurora-core/src/cli/client.rs`

- [ ] **Step 1: Add reqwest dependency**

Add to `crates/aurora-core/Cargo.toml`:
```toml
reqwest = { version = "0.12", features = ["json"] }
```

- [ ] **Step 2: Create HTTP client**

Create `crates/aurora-core/src/cli/client.rs`:
```rust
use serde::Deserialize;

#[derive(Debug, Deserialize)]
pub struct HealthResponse {
    pub status: String,
    pub uptime: u64,
}

#[derive(Debug, Deserialize)]
pub struct PluginInfo {
    pub name: String,
    pub routes: Vec<String>,
}

#[derive(Debug, Deserialize)]
pub struct PluginsResponse {
    pub plugins: Vec<PluginInfo>,
}

#[derive(Debug, Deserialize)]
pub struct RootResponse {
    pub name: String,
    pub version: String,
    pub status: String,
}

pub struct ApiClient {
    base_url: String,
    client: reqwest::Client,
}

impl ApiClient {
    pub fn new(base_url: &str) -> Self {
        Self {
            base_url: base_url.trim_end_matches('/').to_string(),
            client: reqwest::Client::new(),
        }
    }

    pub async fn health(&self) -> Result<HealthResponse, reqwest::Error> {
        self.client
            .get(format!("{}/api/health", self.base_url))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn root(&self) -> Result<RootResponse, reqwest::Error> {
        self.client
            .get(&self.base_url)
            .send()
            .await?
            .json()
            .await
    }

    pub async fn plugins(&self) -> Result<PluginsResponse, reqwest::Error> {
        self.client
            .get(format!("{}/api/plugins", self.base_url))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn plugin_info(&self, name: &str) -> Result<PluginInfo, reqwest::Error> {
        self.client
            .get(format!("{}/api/plugins/{}", self.base_url, name))
            .send()
            .await?
            .json()
            .await
    }
}
```

- [ ] **Step 3: Add client module to cli/mod.rs**

Add to `crates/aurora-core/src/cli/mod.rs`:
```rust
pub mod client;
```

- [ ] **Step 4: Build and verify**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

---

### Task 3: Implement Status Command

**Covers:** [S5]
**Files:**
- Modify: `crates/aurora-core/src/cli/commands.rs`

- [ ] **Step 1: Implement show_status**

Replace the `show_status` function in `crates/aurora-core/src/cli/commands.rs`:
```rust
pub async fn show_status(json: bool) {
    let client = super::client::ApiClient::new("http://127.0.0.1:8080");

    match client.health().await {
        Ok(health) => {
            if json {
                println!("{}", serde_json::json!({
                    "status": health.status,
                    "uptime": health.uptime,
                }));
            } else {
                println!("AuroraStream Status");
                println!("==================");
                println!("Status: {}", health.status);
                println!("Uptime: {} seconds", health.uptime);
            }
        }
        Err(e) => {
            if json {
                println!("{}", serde_json::json!({
                    "status": "error",
                    "error": e.to_string(),
                }));
            } else {
                eprintln!("Error: Could not connect to engine: {}", e);
                eprintln!("Make sure 'aurora serve' is running.");
            }
            std::process::exit(1);
        }
    }
}
```

- [ ] **Step 2: Build and verify**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

---

### Task 4: Implement Plugin Management Commands

**Covers:** [S5]
**Files:**
- Modify: `crates/aurora-core/src/cli/commands.rs`

- [ ] **Step 1: Implement handle_plugin**

Replace the `handle_plugin` function in `crates/aurora-core/src/cli/commands.rs`:
```rust
pub async fn handle_plugin(action: super::PluginAction) {
    let client = super::client::ApiClient::new("http://127.0.0.1:8080");

    match action {
        super::PluginAction::List { loaded, available } => {
            match client.plugins().await {
                Ok(resp) => {
                    println!("Loaded Plugins:");
                    println!("{:<20} {}", "Name", "Routes");
                    println!("{:<20} {}", "----", "------");
                    for p in &resp.plugins {
                        println!("{:<20} {}", p.name, p.routes.join(", "));
                    }
                }
                Err(e) => {
                    eprintln!("Error: {}", e);
                    std::process::exit(1);
                }
            }
        }
        super::PluginAction::Install { name } => {
            println!("Installing plugin: {}", name);
            eprintln!("Not yet implemented - use manual installation");
        }
        super::PluginAction::Remove { name } => {
            println!("Removing plugin: {}", name);
            eprintln!("Not yet implemented");
        }
        super::PluginAction::Enable { name } => {
            println!("Enabling plugin: {}", name);
            eprintln!("Not yet implemented");
        }
        super::PluginAction::Disable { name } => {
            println!("Disabling plugin: {}", name);
            eprintln!("Not yet implemented");
        }
    }
}
```

- [ ] **Step 2: Build and verify**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

---

### Task 5: Implement Config Management Commands

**Covers:** [S5]
**Files:**
- Modify: `crates/aurora-core/src/cli/commands.rs`

- [ ] **Step 1: Implement handle_config**

Replace the `handle_config` function in `crates/aurora-core/src/cli/commands.rs`:
```rust
pub async fn handle_config(action: super::ConfigAction) {
    match action {
        super::ConfigAction::Show => {
            let config = aurora_core::Config::load(None)
                .unwrap_or_default();
            println!("{}", toml::to_string_pretty(&config).unwrap());
        }
        super::ConfigAction::Set { key, value } => {
            println!("Setting {} = {}", key, value);
            eprintln!("Not yet implemented - edit config.toml directly");
        }
        super::ConfigAction::Get { key } => {
            let config = aurora_core::Config::load(None)
                .unwrap_or_default();
            match key.as_str() {
                "server.host" => println!("{}", config.server.host),
                "server.http_port" => println!("{}", config.server.http_port),
                "server.rtmp_port" => println!("{}", config.server.rtmp_port),
                "plugins.directory" => println!("{}", config.plugins.directory),
                "plugins.auto_load" => println!("{}", config.plugins.auto_load),
                "logging.level" => println!("{}", config.logging.level),
                "media.video_codec" => println!("{}", config.media.video_codec),
                "media.audio_codec" => println!("{}", config.media.audio_codec),
                _ => eprintln!("Unknown config key: {}", key),
            }
        }
    }
}
```

- [ ] **Step 2: Build and verify**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

---

### Task 6: Implement Logs Command

**Covers:** [S5]
**Files:**
- Modify: `crates/aurora-core/src/cli/commands.rs`

- [ ] **Step 1: Implement show_logs**

Replace the `show_logs` function in `crates/aurora-core/src/cli/commands.rs`:
```rust
pub async fn show_logs(follow: bool, level: Option<String>) {
    if follow {
        println!("Following logs (Ctrl+C to stop)...");
        eprintln!("Not yet implemented - logs streaming requires WebSocket");
    } else {
        println!("Recent logs:");
        eprintln!("Not yet implemented - requires log storage");
    }
}
```

- [ ] **Step 2: Build and verify**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

---

### Task 7: TUI Framework Setup with ratatui

**Covers:** [S4]
**Files:**
- Modify: `crates/aurora-core/Cargo.toml`
- Create: `crates/aurora-core/src/tui/mod.rs`
- Create: `crates/aurora-core/src/tui/app.rs`
- Create: `crates/aurora-core/src/tui/ui.rs`
- Create: `crates/aurora-core/src/tui/event.rs`

- [ ] **Step 1: Add ratatui and crossterm dependencies**

Add to `crates/aurora-core/Cargo.toml`:
```toml
ratatui = "0.29"
crossterm = "0.28"
```

- [ ] **Step 2: Create TUI module structure**

Create `crates/aurora-core/src/tui/mod.rs`:
```rust
pub mod app;
pub mod event;
pub mod ui;

use crossterm::{
    event::{self, Event, KeyCode, KeyEventKind},
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
    execute,
};
use ratatui::prelude::*;
use std::io;
use std::time::Duration;

use app::App;
use event::{AppEvent, EventHandler};

pub fn run(api_url: &str) -> Result<(), Box<dyn std::error::Error>> {
    enable_raw_mode()?;
    let mut stdout = io::stdout();
    execute!(stdout, EnterAlternateScreen)?;
    let backend = CrosstermBackend::new(stdout);
    let mut terminal = Terminal::new(backend)?;

    let client = super::cli::client::ApiClient::new(api_url);
    let mut app = App::new(client);
    let events = EventHandler::new(250);

    loop {
        terminal.draw(|f| ui::render(f, &mut app))?;

        if let Event::Key(key) = event::read()? {
            if key.kind == KeyEventKind::Press {
                match key.code {
                    KeyCode::Char('q') => break,
                    KeyCode::Tab => app.next_tab(),
                    KeyCode::BackTab => app.prev_tab(),
                    KeyCode::Up | KeyCode::Char('k') => app.previous(),
                    KeyCode::Down | KeyCode::Char('j') => app.next(),
                    _ => {}
                }
            }
        }

        if let Some(event) = events.next() {
            match event {
                AppEvent::Tick => app.tick(),
                AppEvent::Quit => break,
            }
        }
    }

    disable_raw_mode()?;
    execute!(terminal.backend_mut(), LeaveAlternateScreen)?;
    terminal.show_cursor()?;
    Ok(())
}
```

- [ ] **Step 3: Create event handler**

Create `crates/aurora-core/src/tui/event.rs`:
```rust
use crossterm::event::{self, Event, KeyCode, KeyEvent};
use std::sync::mpsc;
use std::thread;
use std::time::Duration;

pub enum AppEvent {
    Tick,
    Quit,
}

pub struct EventHandler {
    rx: mpsc::Receiver<AppEvent>,
}

impl EventHandler {
    pub fn new(tick_rate: u64) -> Self {
        let (tx, rx) = mpsc::channel();
        let tick_rate = Duration::from_millis(tick_rate);

        thread::spawn(move || loop {
            if event::poll(tick_rate).unwrap_or(false) {
                if let Event::Key(key) = event::read().unwrap_or(Event::Key(KeyEvent::new(
                    KeyCode::Char('q'),
                    crossterm::event::KeyModifiers::NONE,
                ))) {
                    if key.code == KeyCode::Char('q') {
                        let _ = tx.send(AppEvent::Quit);
                    }
                }
            }
            let _ = tx.send(AppEvent::Tick);
        });

        Self { rx }
    }

    pub fn next(&self) -> Option<AppEvent> {
        self.rx.recv().ok()
    }
}
```

- [ ] **Step 4: Create app state**

Create `crates/aurora-core/src/tui/app.rs`:
```rust
use super::super::client::ApiClient;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Tab {
    Dashboard,
    Connections,
    Plugins,
    Logs,
    Performance,
}

impl Tab {
    pub fn next(self) -> Self {
        match self {
            Self::Dashboard => Self::Connections,
            Self::Connections => Self::Plugins,
            Self::Plugins => Self::Logs,
            Self::Logs => Self::Performance,
            Self::Performance => Self::Dashboard,
        }
    }

    pub fn prev(self) -> Self {
        match self {
            Self::Dashboard => Self::Performance,
            Self::Performance => Self::Logs,
            Self::Logs => Self::Plugins,
            Self::Plugins => Self::Connections,
            Self::Connections => Self::Dashboard,
        }
    }

    pub fn titles() -> &'static [&'static str] {
        &["Dashboard", "Connections", "Plugins", "Logs", "Performance"]
    }
}

pub struct App {
    pub current_tab: Tab,
    pub should_quit: bool,
    pub client: ApiClient,
    pub plugins: Vec<String>,
    pub health_status: String,
    pub tick_count: u64,
}

impl App {
    pub fn new(client: ApiClient) -> Self {
        Self {
            current_tab: Tab::Dashboard,
            should_quit: false,
            client,
            plugins: Vec::new(),
            health_status: "Unknown".to_string(),
            tick_count: 0,
        }
    }

    pub fn next_tab(&mut self) {
        self.current_tab = self.current_tab.next();
    }

    pub fn prev_tab(&mut self) {
        self.current_tab = self.current_tab.prev();
    }

    pub fn next(&mut self) {}
    pub fn previous(&mut self) {}

    pub fn tick(&mut self) {
        self.tick_count += 1;
    }
}
```

- [ ] **Step 5: Create UI renderer**

Create `crates/aurora-core/src/tui/ui.rs`:
```rust
use ratatui::{
    prelude::*,
    widgets::{Block, Borders, Paragraph, Tabs},
};
use super::app::{App, Tab};

pub fn render(f: &mut Frame, app: &mut App) {
    let area = f.area();

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3),  // Header
            Constraint::Min(0),    // Content
            Constraint::Length(3),  // Footer
        ])
        .split(area);

    render_header(f, app, chunks[0]);
    render_content(f, app, chunks[1]);
    render_footer(f, chunks[2]);
}

fn render_header(f: &mut Frame, app: &App, area: Rect) {
    let titles = Tab::titles()
        .iter()
        .map(|t| Line::from(*t))
        .collect::<Vec<_>>();

    let tabs = Tabs::new(titles)
        .block(Block::default().borders(Borders::ALL).title("AuroraStream"))
        .select(app.current_tab as usize)
        .style(Style::default().fg(Color::White))
        .highlight_style(Style::default().fg(Color::Yellow));

    f.render_widget(tabs, area);
}

fn render_content(f: &mut Frame, app: &App, area: Rect) {
    match app.current_tab {
        Tab::Dashboard => render_dashboard(f, app, area),
        Tab::Connections => render_connections(f, area),
        Tab::Plugins => render_plugins(f, app, area),
        Tab::Logs => render_logs(f, area),
        Tab::Performance => render_performance(f, area),
    }
}

fn render_dashboard(f: &mut Frame, app: &App, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(5),
            Constraint::Min(0),
        ])
        .split(area);

    let stats = Paragraph::new(vec![
        Line::from(vec![
            Span::styled("Status: ", Style::default().fg(Color::White)),
            Span::styled(&app.health_status, Style::default().fg(Color::Green)),
        ]),
        Line::from(vec![
            Span::styled("Tick: ", Style::default().fg(Color::White)),
            Span::styled(format!("{}", app.tick_count), Style::default().fg(Color::Cyan)),
        ]),
    ])
    .block(Block::default().borders(Borders::ALL).title("Overview"));

    f.render_widget(stats, chunks[0]);

    let placeholder = Paragraph::new("Dashboard content...")
        .block(Block::default().borders(Borders::ALL).title("Details"));
    f.render_widget(placeholder, chunks[1]);
}

fn render_connections(f: &mut Frame, area: Rect) {
    let block = Block::default()
        .borders(Borders::ALL)
        .title("Connections");
    let text = Paragraph::new("No active connections")
        .block(block);
    f.render_widget(text, area);
}

fn render_plugins(f: &mut Frame, app: &App, area: Rect) {
    let items: Vec<Line> = app.plugins.iter().map(|p| Line::from(p.as_str())).collect();
    let block = Block::default().borders(Borders::ALL).title("Plugins");
    let list = Paragraph::new(items).block(block);
    f.render_widget(list, area);
}

fn render_logs(f: &mut Frame, area: Rect) {
    let block = Block::default().borders(Borders::ALL).title("Logs");
    let text = Paragraph::new("No logs yet...").block(block);
    f.render_widget(text, area);
}

fn render_performance(f: &mut Frame, area: Rect) {
    let block = Block::default().borders(Borders::ALL).title("Performance");
    let text = Paragraph::new("Performance metrics...").block(block);
    f.render_widget(text, area);
}

fn render_footer(f: &mut Frame, area: Rect) {
    let text = Paragraph::new("[↑↓] Navigate  [Tab] Switch View  [q] Quit")
        .style(Style::default().fg(Color::DarkGray));
    f.render_widget(text, area);
}
```

- [ ] **Step 6: Add tui module to lib.rs**

Add to `crates/aurora-core/src/lib.rs`:
```rust
pub mod tui;
```

- [ ] **Step 7: Update run_tui command**

Replace the `run_tui` function in `crates/aurora-core/src/cli/commands.rs`:
```rust
pub async fn run_tui(api: &str) {
    if let Err(e) = crate::tui::run(api) {
        eprintln!("TUI error: {}", e);
        std::process::exit(1);
    }
}
```

- [ ] **Step 8: Build and verify TUI**

Run: `cargo build -p aurora-core 2>&1 | tail -5`
Expected: Build succeeds

- [ ] **Step 9: Test TUI launch (manual)**

Run: `cargo run -p aurora-core -- cli --api http://localhost:8080`
Expected: TUI interface appears, navigate with Tab, quit with q

---

### Task 8: Wire Everything Together and Final Test

**Covers:** [S2, S5]
**Files:**
- Modify: `crates/aurora-core/src/main.rs` (verify)

- [ ] **Step 1: Full build**

Run: `cargo build --release 2>&1 | tail -5`
Expected: Build succeeds

- [ ] **Step 2: Test CLI help**

Run: `cargo run --release -- --help`
Expected: Shows all subcommands

- [ ] **Step 3: Test serve command**

Run: `cargo run --release -- serve --help`
Expected: Shows serve options

- [ ] **Step 4: Test status command (engine not running)**

Run: `cargo run --release -- status`
Expected: Error message "Could not connect to engine"

- [ ] **Step 5: Test config show**

Run: `cargo run --release -- config show`
Expected: Shows current config in TOML format

- [ ] **Step 6: Test plugin list (engine not running)**

Run: `cargo run --release -- plugin list`
Expected: Error message about connection

- [ ] **Step 7: Verify binary size**

Run: `ls -lh target/release/aurora`
Expected: Size under 10MB
