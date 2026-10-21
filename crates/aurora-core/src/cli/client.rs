use serde::Deserialize;

#[derive(Debug, Deserialize)]
pub struct HealthResponse {
    pub status: String,
    pub uptime: u64,
    #[serde(default)]
    pub connections: u32,
    #[serde(default)]
    pub streams: u32,
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

/// A stream that is currently live on the server.
#[derive(Debug, Clone, Deserialize)]
pub struct StreamInfo {
    pub key: String,
    #[serde(default)]
    pub video_codec: Option<String>,
    #[serde(default)]
    pub audio_codec: Option<String>,
    #[serde(default)]
    pub uptime: u64,
}

#[derive(Debug, Deserialize)]
pub struct StatsResponse {
    pub uptime: u64,
    pub connections: u32,
    pub streams: u32,
    #[serde(default)]
    pub stream_list: Vec<StreamInfo>,
}

#[derive(Debug, Deserialize)]
pub struct StreamsResponse {
    pub count: usize,
    pub streams: Vec<StreamInfo>,
}

pub struct ApiClient {
    client: reqwest::Client,
    base_url: String,
}

impl ApiClient {
    pub fn new(base_url: &str) -> Self {
        Self {
            client: reqwest::Client::new(),
            base_url: base_url.trim_end_matches('/').to_string(),
        }
    }

    pub async fn health(&self) -> reqwest::Result<HealthResponse> {
        self.client
            .get(format!("{}/api/health", self.base_url))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn root(&self) -> reqwest::Result<RootResponse> {
        self.client
            .get(format!("{}/", self.base_url))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn plugins(&self) -> reqwest::Result<PluginsResponse> {
        self.client
            .get(format!("{}/api/plugins", self.base_url))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn plugin_info(&self, name: &str) -> reqwest::Result<PluginInfo> {
        self.client
            .get(format!("{}/api/plugins/{}", self.base_url, name))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn stats(&self) -> reqwest::Result<StatsResponse> {
        self.client
            .get(format!("{}/api/stats", self.base_url))
            .send()
            .await?
            .json()
            .await
    }

    pub async fn streams(&self) -> reqwest::Result<StreamsResponse> {
        self.client
            .get(format!("{}/api/streams", self.base_url))
            .send()
            .await?
            .json()
            .await
    }
}
