# AuroraStream

A lightweight, plugin-based streaming server written in Rust, with an FFmpeg powered
media core.

AuroraStream accepts RTMP publishes, exposes a control API, and lets plugins extend the
server at runtime. The engine, the HTTP API, the CLI and the TUI all live in a single
`aurora` binary.

## Features

- **RTMP ingest** — handshake, chunked message decoding, AMF0 commands. Publishers can
  push with ffmpeg and the server detects the codecs from the first media packet.
- **RTMP playback** — players can pull a live stream back. The server hands out the
  cached sequence headers and the `onMetaData` script message, then forwards the frames.
- **HTTP-FLV** — the same live stream over HTTP, so browsers can play it with flv.js.
- **Plugin system** — plugins are shared objects loaded with `dlopen` at runtime. They
  can serve their own HTTP routes and receive every event the engine publishes.
- **HTTP control API** — health, statistics, live streams and plugin information.
- **CLI and TUI** — a terminal dashboard that shows the live state of the server.
- **FFmpeg media core** — codec, muxer and filter layer in C++ behind a Rust FFI.

## Building

Requirements: Rust (stable), CMake, a C++17 compiler, and the FFmpeg development
libraries (`libavcodec`, `libavformat`, `libavutil`, `libswresample`, `libavfilter`).

```sh
make build-release     # cargo build --workspace --release
make plugins-release   # build and deploy the plugin .so files into plugins/
```

The plugins have to be deployed into `plugins/` after every build, because the engine
loads them from there at startup.

> The workspace builds with `-C prefer-dynamic` (see `.cargo/config.toml`) so that the
> host and the plugins share one copy of `std`. The host binary and the deployed plugin
> libraries therefore have to be rebuilt together after a toolchain update.

## Running

```sh
./target/release/aurora serve            # uses ./config.toml
./target/release/aurora serve --config /path/to/config.toml
./target/release/aurora status           # talks to http://127.0.0.1:8080
./target/release/aurora --api http://127.0.0.1:9000 status
./target/release/aurora cli              # interactive TUI
./target/release/aurora plugin list
```

`aurora serve` listens on the HTTP and RTMP ports from `config.toml` (8080 and 1935 by
default). It shuts down cleanly on `SIGINT` and `SIGTERM`.

## Pushing a stream

```sh
ffmpeg -re -i input.mp4 \
  -c:v libx264 -preset ultrafast -g 50 -c:a aac \
  -f flv rtmp://127.0.0.1:1935/live/demo
```

The stream shows up under `/api/streams` with the detected codecs, and in the TUI under
the *Connections* tab.

Playing it back over RTMP:

```sh
ffmpeg -i rtmp://127.0.0.1:1935/live/demo -f null -
```

Or over HTTP-FLV, which browsers can play with
[flv.js](https://github.com/xgwang/ flv.js):

```
http://127.0.0.1:8080/flv/live/demo.flv
```

## HTTP API

| Endpoint | Description |
|---|---|
| `GET /api/health` | status, uptime, connection and stream counts |
| `GET /api/stats` | uptime, counts and the list of live streams |
| `GET /api/streams` | the live streams with their codecs |
| `GET /api/plugins` | the loaded plugins and the routes they advertise |
| `GET /api/plugins/:name` | the routes of a single plugin |

Plugins mount their own routes, for example `GET /api/rtmp/sessions`,
`GET /api/rtmp/streams` and `GET /flv/:key`.

## Tests

```sh
make test                     # debug profile
./tests/integration/run.sh --release
```

The integration test builds the workspace, deploys the plugins, starts the engine,
pushes an RTMP stream with ffmpeg and asserts that the API reports it, that a player can
pull it back, that the HTTP-FLV endpoint serves the same stream, that the stream
disappears when the publisher stops and that the engine shuts down cleanly.

The unit tests cover the parts that are easy to get subtly wrong: the chunk decoder
(header formats, chunk splitting, per stream state), the AMF0 codec and the parsing of
the H.264 SPS and the AAC configuration.

```sh
cargo test --workspace
```

## Writing a plugin

A plugin is a `cdylib` that implements `aurora_core::plugin::Plugin` and exports:

```rust
#[no_mangle]
pub extern "C" fn aurora_plugin_create() -> *mut dyn Plugin { /* ... */ }

#[no_mangle]
pub extern "C" fn aurora_plugin_destroy(plugin: *mut dyn Plugin) { /* ... */ }
```

Because a plugin is a separate shared object, it carries its own copy of every Rust
dependency except `std`. That imposes a few hard rules — the plugin must run its own
Tokio runtime, must log through the sink the host provides, and must not pass non-`std`
types across the boundary.

**Read [docs/architecture/plugin-abi.md](docs/architecture/plugin-abi.md) before writing
one.** It explains why each rule exists and how to diagnose a violation.

## Layout

```
crates/aurora-core              engine, event bus, config, plugin loader, HTTP API, CLI, TUI
crates/aurora-rtmp              RTMP protocol plugin
crates/aurora-remote-desktop    remote desktop plugin
crates/aurora-ffi               Rust bindings for the C++ media core
cpp/                            FFmpeg based media core (codec, muxer, filter, pool)
plugins/                        shared objects loaded at runtime
tests/integration/              end to end test
docs/architecture/              plugin ABI rules
```

## License

GPL-3.0. See [LICENSE](LICENSE).
