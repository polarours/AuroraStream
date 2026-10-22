# P0 Core Features Implementation Plan

> **Status: completed (2026-10-21).** The engine starts, loads plugins, accepts RTMP
> publishes and the TUI shows live data. `tests/integration/run.sh` covers it end to end.
> The plugin ABI constraints that came out of this work are documented in
> `docs/architecture/plugin-abi.md`.

**Goal:** Make AuroraStream actually work — engine starts, RTMP streams flow, plugins load, filters process, TUI shows live data.

**Architecture:** Wire existing components together, implement missing protocol handlers, add FFI exports.

---

### Task 1: Wire aurora serve Command

**Files:**
- Modify: `crates/aurora-core/src/cli/commands.rs`

**Status:** done. `handle_serve` loads the config, starts the engine and reports fatal
errors. The config file in the working directory is picked up automatically, `--config`
still overrides it.

---

### Task 2: Implement RTMP Session Data Loop

**Files:**
- Modify: `crates/aurora-rtmp/src/rtmp_session.rs`
- Modify: `crates/aurora-rtmp/src/rtmp_chunk.rs`
- Modify: `crates/aurora-rtmp/src/rtmp_amf.rs` (new)
- Modify: `crates/aurora-rtmp/src/rtmp_handshake.rs`

**Status:** done.

- The handshake sends `S0 S1 S2` and reads `C2`. The previous implementation only sent
  `S0 S1`, which made every client abort with "Cannot read RTMP handshake response".
- `rtmp_chunk.rs` implements a real chunk decoder: per chunk stream state, `fmt` 0/1/2/3,
  extended timestamps and payload splitting at the negotiated chunk size. The previous
  parser treated every chunk as the start of a message, so any frame larger than the
  chunk size was reassembled incorrectly.
- `rtmp_amf.rs` is a new AMF0 codec. The `publish` and `play` commands are parsed, so the
  stream key (`live/test`) is now known - the previous implementation never assigned
  `stream_key` and every event carried an empty key.
- The encoder writes the correct basic header (chunk stream id and `fmt`) and splits
  large payloads into continuation chunks.
- The server answers `connect`, `createStream`, `publish`, `play` and `_checkbw`, and
  sends the `NetStream.Publish.Start` / `NetStream.Play.Start` statuses.
- The codec of the stream is detected from the first media packet and reported back
  once it is known.

---

### Task 3: Add Plugin FFI Exports

**Files:**
- Modify: `crates/aurora-rtmp/src/lib.rs`
- Modify: `crates/aurora-remote-desktop/src/lib.rs`

**Status:** done, but the ABI had to be reworked to work at all.

- `aurora_plugin_create` / `aurora_plugin_destroy` are exported by both plugins and the
  `.so` files are built and deployed into `plugins/` (`make plugins`).
- The previous exports crashed the process on startup: the plugin shared object
  statically links its own Tokio, so `TcpListener::from_std` panicked with "there is no
  reactor running" and the panic could not unwind across the `extern "C"` boundary.
- The plugin now runs its own Tokio runtime on a dedicated thread.
- Plugin log output was invisible because the plugin carries its own `tracing` copy. The
  host now provides a logging sink through `PluginContext`.
- The shared objects are no longer `dlclose`d: the HTTP handlers the plugin registers
  carry vtables that point into the library, and unloading it left dangling pointers.

See `docs/architecture/plugin-abi.md` for the full set of rules.

---

### Task 4: Implement FFmpeg Filter

**Files:**
- Modify: `cpp/src/filter.cpp`
- Modify: `cpp/include/aurora_media.h`

**Status:** already implemented before this round. `aurora_filter_create` builds a real
`AVFilterGraph` and `aurora_filter_apply` runs the frames through it.

**Still open:** the `aurora-ffi` crate is not wired into anything yet - it is built and
linked against the C++ library, but no code path uses it, and the bindings that
`bindgen` generates in `build.rs` are never included (`aurora-ffi/src/lib.rs` uses a
hand written `aurora_media_sys` module instead).

---

### Task 5: TUI Real-time Data

**Files:**
- Modify: `crates/aurora-core/src/tui/app.rs`
- Modify: `crates/aurora-core/src/tui/ui.rs`
- Modify: `crates/aurora-core/src/cli/client.rs`

**Status:** done.

- The TUI polls `/api/health` and `/api/stats` and shows the real connection and stream
  counts, the list of live streams and their codecs.
- The status colour was checked against `"ok"` while the API answers `"healthy"`, so the
  panel never turned green.

---

### Task 6: Integration Test

**Status:** done - `tests/integration/run.sh` (also `make test`).

It builds the workspace, deploys the plugins, starts the engine, pushes an RTMP stream
with ffmpeg and asserts that:

- the API reports the engine as healthy and both plugins as loaded
- the plugin detail route `/api/plugins/rtmp` answers
- the stream shows up in `/api/stats` and `/api/streams` with the detected `h264`/`aac`
  codecs while it is live, and disappears when the publisher stops
- the plugin route `/api/rtmp/sessions` reports the session state and the byte count
- the engine shuts down cleanly on `SIGTERM`, stopping all plugins without a crash

The script passes in both the debug and the release profile.

---

## Follow-up work

1. **`aurora-ffi` is an island.** Wire the FFmpeg codec/muxer/filter layer into the
   pipeline, or drop the crate. The `bindgen` output is unused.
2. **Plugin ABI is not stable.** `*mut dyn Plugin` and `Result<(), AuroraError>` cross
   the shared object boundary. A `#[repr(C)]` versioned interface with a host provided
   service table is the planned replacement.
3. **The event bus is publish-only.** `EventBus::subscribe` has no callers; the engine
   drains the queue and fans the events out to `Plugin::on_event` instead.
4. **`MemoryPool` is unused.** It is created by the engine but nothing acquires from it.
5. **A live stream can disappear from the registry while its publisher is still
   running.** The publisher keeps sending frames and `ensure()` re-creates the entry, but
   a client that subscribes in the moment after the entry was removed gets a 404. The
   entry is only ever removed by `StreamRegistry::remove`, which the publisher session
   calls when it ends - so either a publisher session ends without a trace in the log,
   or the registry the clients see is not the one the publisher writes to. The pointers of
   both were compared and are identical, which points at the second. This still needs a
   proper diagnosis; the symptom is visible in the integration test as a 404 on the
   second HTTP-FLV request.

## What the second round added

- **RTMP playback.** A `StreamRegistry` holds the live streams: the metadata, the
  cached AVC and AAC sequence headers and the subscribers. A player receives the
  `onMetaData` script message and the sequence headers before the live frames start.
- **HTTP-FLV.** `GET /flv/<key>.flv` serves the same stream as an FLV stream, so a
  browser can play it with flv.js. The FLV container is written in Rust.
- **The dimensions of a stream.** The AVC sequence header is parsed down to the picture
  size, the AAC configuration yields the sample rate and the channel count.
- **Timeline rebasing.** A player that joins a running stream gets the timestamps
  rebased to zero. Audio and video carry their own timestamp chains and drift apart, so
  every stream is rebased on its own - rebasing them together let one of them run
  backwards.
- **Unit tests** for the chunk decoder, the AMF0 codec and the FLV parsing, using bytes
  taken from a real ffmpeg stream.
