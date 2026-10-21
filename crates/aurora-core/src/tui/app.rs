use crate::cli::client::{ApiClient, StreamInfo};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Tab {
    Dashboard,
    Connections,
    Plugins,
    Logs,
    Performance,
}

impl Tab {
    pub fn all() -> &'static [Tab] {
        &[
            Tab::Dashboard,
            Tab::Connections,
            Tab::Plugins,
            Tab::Logs,
            Tab::Performance,
        ]
    }

    pub fn title(&self) -> &str {
        match self {
            Tab::Dashboard => "Dashboard",
            Tab::Connections => "Connections",
            Tab::Plugins => "Plugins",
            Tab::Logs => "Logs",
            Tab::Performance => "Performance",
        }
    }

    pub fn next(&self) -> Tab {
        let tabs = Self::all();
        let idx = tabs.iter().position(|t| t == self).unwrap_or(0);
        tabs[(idx + 1) % tabs.len()]
    }

    pub fn prev(&self) -> Tab {
        let tabs = Self::all();
        let idx = tabs.iter().position(|t| t == self).unwrap_or(0);
        tabs[(idx + tabs.len() - 1) % tabs.len()]
    }
}

#[derive(Debug, Clone)]
pub struct PluginInfo {
    pub name: String,
    pub routes: Vec<String>,
}

pub struct App {
    pub current_tab: Tab,
    pub client: ApiClient,
    pub tick_count: u64,
    pub server_status: String,
    pub server_uptime: u64,
    pub plugins: Vec<PluginInfo>,
    pub connection_count: u32,
    pub stream_count: u32,
    pub streams: Vec<StreamInfo>,
    pub last_error: Option<String>,
}

impl App {
    pub fn new(api_url: &str) -> Self {
        Self {
            current_tab: Tab::Dashboard,
            client: ApiClient::new(api_url),
            tick_count: 0,
            server_status: "Connecting...".to_string(),
            server_uptime: 0,
            plugins: Vec::new(),
            connection_count: 0,
            stream_count: 0,
            streams: Vec::new(),
            last_error: None,
        }
    }

    pub fn next_tab(&mut self) {
        self.current_tab = self.current_tab.next();
    }

    pub fn prev_tab(&mut self) {
        self.current_tab = self.current_tab.prev();
    }

    pub async fn tick_async(&mut self) {
        self.tick_count += 1;

        if self.tick_count % 5 == 0 {
            match self.client.health().await {
                Ok(health) => {
                    self.server_status = health.status;
                    self.server_uptime = health.uptime;
                    self.connection_count = health.connections;
                    self.stream_count = health.streams;
                    self.last_error = None;
                }
                Err(e) => {
                    self.server_status = "Disconnected".to_string();
                    self.last_error = Some(e.to_string());
                }
            }
        }

        if self.tick_count % 10 == 0 {
            if let Ok(resp) = self.client.plugins().await {
                self.plugins = resp
                    .plugins
                    .into_iter()
                    .map(|p| PluginInfo {
                        name: p.name,
                        routes: p.routes,
                    })
                    .collect();
            }

            if let Ok(resp) = self.client.stats().await {
                self.server_uptime = resp.uptime;
                self.connection_count = resp.connections;
                self.stream_count = resp.streams;
                self.streams = resp.stream_list;
            }
        }
    }

}
