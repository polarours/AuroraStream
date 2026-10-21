use super::{Cli, Commands};
use crate::Config;
use clap::Parser;

pub fn run() -> anyhow::Result<()> {
    let cli = Cli::parse();
    let api = cli.api.clone();

    match cli.command {
        Commands::Serve { config, port } => handle_serve(config, port),
        Commands::Cli => handle_cli(&api),
        Commands::Status { json } => {
            let rt = tokio::runtime::Runtime::new()?;
            rt.block_on(handle_status(json, &api))
        }
        Commands::Plugin { action } => {
            let rt = tokio::runtime::Runtime::new()?;
            rt.block_on(handle_plugin(action, &api));
            Ok(())
        }
        Commands::Config { action } => {
            let rt = tokio::runtime::Runtime::new()?;
            rt.block_on(handle_config(action));
            Ok(())
        }
        Commands::Logs { follow, level } => {
            let rt = tokio::runtime::Runtime::new()?;
            rt.block_on(handle_logs(follow, level));
            Ok(())
        }
    }
}

fn handle_serve(config: Option<String>, port: Option<u16>) -> anyhow::Result<()> {
    let mut cfg = crate::Config::load(config.as_deref())
        .unwrap_or_else(|e| {
            eprintln!("failed to load config: {}, using defaults", e);
            crate::Config::default()
        });
    if let Some(p) = port {
        cfg.server.http_port = p;
    }
    let mut engine = crate::Engine::new(cfg);
    let rt = tokio::runtime::Runtime::new()?;
    rt.block_on(async {
        if let Err(e) = engine.run().await {
            eprintln!("fatal error: {}", e);
            std::process::exit(1);
        }
    });
    Ok(())
}

fn handle_cli(api: &str) -> anyhow::Result<()> {
    let rt = tokio::runtime::Runtime::new()?;
    rt.block_on(crate::tui::run(api))
}

async fn handle_status(json: bool, api: &str) -> anyhow::Result<()> {
    let client = super::client::ApiClient::new(api);

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
    Ok(())
}

pub async fn handle_plugin(action: super::PluginAction, api: &str) {
    let client = super::client::ApiClient::new(api);

    match action {
        super::PluginAction::List { loaded: _, available: _ } => {
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
                    eprintln!("Error: Could not connect to engine: {}", e);
                    eprintln!("Make sure 'aurora serve' is running.");
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

pub async fn handle_config(action: super::ConfigAction) {
    match action {
        super::ConfigAction::Show => {
            let config = Config::load(None)
                .unwrap_or_default();
            println!("{}", toml::to_string_pretty(&config).unwrap());
        }
        super::ConfigAction::Set { key, value } => {
            println!("Setting {} = {}", key, value);
            eprintln!("Not yet implemented - edit config.toml directly");
        }
        super::ConfigAction::Get { key } => {
            let config = Config::load(None)
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

pub async fn handle_logs(follow: bool, _level: Option<String>) {
    if follow {
        println!("Following logs (Ctrl+C to stop)...");
        eprintln!("Not yet implemented - logs streaming requires WebSocket");
    } else {
        println!("Recent logs:");
        eprintln!("Not yet implemented - requires log storage");
    }
}
