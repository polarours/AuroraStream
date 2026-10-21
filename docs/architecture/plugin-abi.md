# Plugin ABI constraints

AuroraStream loads plugins at runtime with `dlopen` (see
`crates/aurora-core/src/plugin_loader.rs`). A plugin is a `cdylib` that exports
`aurora_plugin_create` / `aurora_plugin_destroy` and returns a `Box<dyn Plugin>`.

This document records the constraints a plugin has to respect. They are not obvious,
and violating them does not produce a compile error - it produces a runtime crash.

## Why the rules exist

The host executable and the plugin shared object each contain their **own statically
linked copy** of every Rust dependency except `std`:

| Crate | Shared? | Consequence |
|---|---|---|
| `std` | yes | `Arc`, `Box`, `Vec`, `String`, `Mutex`, `mpsc` behave identically |
| `tokio` | **no** | the plugin has its own runtime context and its own `mpsc` |
| `tracing` | **no** | the plugin's global subscriber is not the host's |
| `parking_lot` | **no** | separate `RwLock` implementations |

`std` is shared because `.cargo/config.toml` sets `-C prefer-dynamic`, which makes the
host and the plugin resolve `libstd-<hash>.so` to the same object at load time.

## Rules

### 1. A plugin must not use the host's Tokio runtime

Tokio's runtime context lives in a thread local of the *plugin's* copy of Tokio, so the
plugin never sees the host's runtime. Calling `tokio::net::TcpListener::from_std` or
`tokio::spawn` from a callback the host invokes panics with:

```
there is no reactor running, must be called from the context of a Tokio 1.x runtime
```

A panic cannot unwind across the `extern "C"` plugin boundary, so the whole process
aborts.

**Do this instead:** give the plugin its own runtime, driven from a dedicated thread.
`crates/aurora-rtmp/src/lib.rs` shows the pattern:

```rust
std::thread::Builder::new().name("aurora-rtmp".into()).spawn(move || {
    let runtime = tokio::runtime::Runtime::new().unwrap();
    runtime.block_on(async move { /* accept loop */ });
})?;
```

`Plugin::on_start` is synchronous, so bind the socket with `std::net::TcpListener`
first - that way a port clash is reported to the engine as an error instead of only
being logged from a background thread.

### 2. A plugin must not log through `tracing`

The plugin's `tracing` copy has its own global subscriber, so `tracing::info!` inside a
plugin is never seen by the host.

**Do this instead:** use the logging sink the host provides in `PluginContext`:

```rust
ctx.info(format!("stream published: {}", stream_key));
```

The host routes the message into its own subscriber with `target: "plugin"`.

### 3. Only `std` types may cross the boundary

Anything that lives in a Rust crate that is not shared has two implementations in the
process. Sharing an object between them means two different implementations operate on
the same memory.

The types that cross the boundary are `PluginContext` and the `Event` enum. They must
only contain `std` types:

- `EventBus` uses `std::sync::mpsc` (not `tokio::sync::mpsc`) and
  `std::sync::Mutex` to wrap the sender, because `std::sync::mpsc::Sender` is `Send`
  but not `Sync`.
- The media frames carry `Vec<u8>` (not `bytes::Bytes`).
- `PluginContext` holds `Arc` (shared `std`), plain config data, and the log sink.

### 4. The plugin library must stay loaded

A plugin hands the host callbacks - HTTP handlers are `Arc<dyn Fn(..)>`, and the vtable
of such a closure points into the plugin shared object. If the library is `dlclose`d
while the host still holds such a callback, dropping it later dereferences unmapped
memory and crashes.

`PluginLoader::drop` therefore leaks the libraries on purpose (`std::mem::forget`).
The engine additionally calls `HttpGateway::clear_plugin_handlers` once the plugins are
stopped.

### 5. The ABI is not stable

`aurora_plugin_create` returns `*mut dyn Plugin` and the trait methods return
`Result<(), AuroraError>`. Neither type is `#[repr(C)]`, so the layout is only guaranteed
to match as long as the host and the plugin are built from the same sources with the
same compiler. The compiler warns about this (`improper_ctypes_definitions`, silenced
with an explanatory `#[allow]`).

A `#[repr(C)]` versioned interface with a host-provided service table is the planned
replacement.

## Deployment note

`.cargo/config.toml` sets `-C prefer-dynamic`, so both the host and the plugins link
`libstd-<hash>.so` from the toolchain. The hash changes with every Rust update, so the
host binary and the deployed plugin `.so` files have to be rebuilt together. A plugin
built against a different `libstd` than the host will fail to load.

Run `make plugins` after every build; the plugin `.so` files in `plugins/` are build
artifacts and are not interchangeable between toolchain versions.

## Checklist for a new plugin

- [ ] Runs its own Tokio runtime, or does not use Tokio at all
- [ ] Binds sockets with `std::net` and reports failures through `Result`
- [ ] Logs through `ctx.info` / `ctx.warn` / `ctx.error`
- [ ] Only passes `std` types through `PluginContext` and `Event`
- [ ] Exports `aurora_plugin_create` / `aurora_plugin_destroy`
- [ ] Is built with the same `.cargo/config.toml` as the host (`prefer-dynamic`)
