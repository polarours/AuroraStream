use aurora_core::cli::commands;

fn main() {
    if let Err(e) = commands::run() {
        eprintln!("error: {}", e);
        std::process::exit(1);
    }
}
