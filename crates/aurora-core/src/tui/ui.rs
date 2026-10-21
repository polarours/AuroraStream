use ratatui::prelude::*;
use ratatui::widgets::*;

use super::app::{App, Tab};

pub fn render(f: &mut Frame, app: &mut App) {
    let area = f.area();

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3),
            Constraint::Min(1),
            Constraint::Length(1),
        ])
        .split(area);

    render_header(f, app, chunks[0]);
    render_content(f, app, chunks[1]);
    render_footer(f, chunks[2]);
}

fn render_header(f: &mut Frame, app: &App, area: Rect) {
    let titles: Vec<Line> = Tab::all()
        .iter()
        .map(|t| {
            let style = if *t == app.current_tab {
                Style::default()
                    .fg(Color::Yellow)
                    .add_modifier(Modifier::BOLD)
            } else {
                Style::default().fg(Color::White)
            };
            Line::from(t.title()).style(style)
        })
        .collect();

    let tabs = Tabs::new(titles)
        .block(
            Block::default()
                .borders(Borders::ALL)
                .title("AuroraStream"),
        )
        .select(
            Tab::all()
                .iter()
                .position(|t| *t == app.current_tab)
                .unwrap_or(0),
        )
        .highlight_style(
            Style::default()
                .fg(Color::Yellow)
                .add_modifier(Modifier::BOLD),
        )
        .divider("|");

    f.render_widget(tabs, area);
}

fn render_content(f: &mut Frame, app: &mut App, area: Rect) {
    match app.current_tab {
        Tab::Dashboard => render_dashboard(f, app, area),
        Tab::Connections => render_connections(f, app, area),
        Tab::Plugins => render_plugins(f, app, area),
        Tab::Logs => render_logs(f, app, area),
        Tab::Performance => render_performance(f, app, area),
    }
}

fn render_footer(f: &mut Frame, area: Rect) {
    let footer = Line::from(vec![
        Span::styled(" Tab/↑↓: navigate", Style::default().fg(Color::Yellow)),
        Span::raw(" | "),
        Span::styled("q: quit", Style::default().fg(Color::Yellow)),
    ]);
    let paragraph = Paragraph::new(footer)
        .style(Style::default().fg(Color::White))
        .alignment(Alignment::Center);
    f.render_widget(paragraph, area);
}

fn render_dashboard(f: &mut Frame, app: &App, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Horizontal)
        .constraints([Constraint::Percentage(50), Constraint::Percentage(50)])
        .margin(1)
        .split(area);

    let status_color = match app.server_status.as_str() {
        "healthy" | "ok" | "Connected" => Color::Green,
        "Disconnected" => Color::Red,
        _ => Color::Yellow,
    };

    let status_block = Block::default()
        .borders(Borders::ALL)
        .title("Engine Status");
    let status_text = Paragraph::new(vec![
        Line::from(vec![
            Span::raw("Status: "),
            Span::styled(&app.server_status, Style::default().fg(status_color)),
        ]),
        Line::from(format!("Uptime: {}s", app.server_uptime)),
        Line::from(format!("Tick: {}", app.tick_count)),
    ])
    .block(status_block);
    f.render_widget(status_text, chunks[0]);

    let info_block = Block::default()
        .borders(Borders::ALL)
        .title("Quick Info");
    let info_lines = vec![
        Line::from(format!("Plugins: {}", app.plugins.len())),
        Line::from(format!("Connections: {}", app.connection_count)),
        Line::from(format!("Streams: {}", app.stream_count)),
        Line::from(""),
        Line::from("Tab/↑↓: navigate | q: quit"),
    ];
    let info_text = Paragraph::new(info_lines).block(info_block);
    f.render_widget(info_text, chunks[1]);
}

fn render_connections(f: &mut Frame, app: &App, area: Rect) {
    let block = Block::default()
        .borders(Borders::ALL)
        .title("Connections");

    let mut items = vec![
        ListItem::new(format!("Active connections: {}", app.connection_count)),
        ListItem::new(format!("Active streams: {}", app.stream_count)),
        ListItem::new(""),
    ];

    if app.streams.is_empty() {
        items.push(ListItem::new("No live streams"));
    } else {
        items.push(ListItem::new("Live streams:"));
        for stream in &app.streams {
            items.push(ListItem::new(format!(
                "  {}  video={}  audio={}  up {}s",
                stream.key,
                stream.video_codec.as_deref().unwrap_or("-"),
                stream.audio_codec.as_deref().unwrap_or("-"),
                stream.uptime
            )));
        }
    }

    let list = List::new(items).block(block).highlight_style(
        Style::default()
            .bg(Color::DarkGray)
            .add_modifier(Modifier::BOLD),
    );

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1)])
        .margin(1)
        .split(area);

    f.render_widget(list, chunks[0]);
}

fn render_plugins(f: &mut Frame, app: &App, area: Rect) {
    let block = Block::default()
        .borders(Borders::ALL)
        .title(format!("Plugins ({})", app.plugins.len()));

    let items: Vec<ListItem> = if app.plugins.is_empty() {
        vec![
            ListItem::new("No plugins loaded"),
            ListItem::new(""),
            ListItem::new("Start engine with: aurora serve"),
        ]
    } else {
        app.plugins
            .iter()
            .map(|p| {
                ListItem::new(format!("{} [{}]", p.name, p.routes.join(", ")))
            })
            .collect()
    };

    let list = List::new(items).block(block);

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1)])
        .margin(1)
        .split(area);

    f.render_widget(list, chunks[0]);
}

fn render_logs(f: &mut Frame, app: &App, area: Rect) {
    let block = Block::default()
        .borders(Borders::ALL)
        .title("Logs");

    let mut lines = vec![
        Line::from("[INFO] AuroraStream TUI started"),
    ];

    if let Some(ref err) = app.last_error {
        lines.push(Line::from(format!("[WARN] {}", err)));
    }

    if app.server_status == "Disconnected" {
        lines.push(Line::from("[WARN] Engine not connected - limited functionality"));
    } else if app.server_status == "Connecting..." {
        lines.push(Line::from("[INFO] Connecting to engine..."));
    } else {
        lines.push(Line::from(format!("[INFO] Engine: {}", app.server_status)));
    }

    let paragraph = Paragraph::new(lines)
        .block(block)
        .wrap(Wrap { trim: true });

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1)])
        .margin(1)
        .split(area);

    f.render_widget(paragraph, chunks[0]);
}

fn render_performance(f: &mut Frame, app: &App, area: Rect) {
    let block = Block::default()
        .borders(Borders::ALL)
        .title("Performance Metrics");

    let text = vec![
        Line::from(format!("Tick count: {}", app.tick_count)),
        Line::from(format!("Server uptime: {}s", app.server_uptime)),
        Line::from("CPU: N/A"),
        Line::from("Memory: N/A"),
        Line::from("Latency: N/A"),
    ];

    let paragraph = Paragraph::new(text)
        .block(block)
        .wrap(Wrap { trim: true });

    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([Constraint::Min(1)])
        .margin(1)
        .split(area);

    f.render_widget(paragraph, chunks[0]);
}
