#!/usr/bin/env bash
#
# End-to-end integration test for AuroraStream.
#
# Builds the workspace, deploys the plugins, starts the engine, pushes an RTMP stream
# with ffmpeg and verifies that the HTTP API reports it. Also checks that the engine
# shuts down cleanly on SIGTERM.
#
# Usage: tests/integration/run.sh [--release]

set -euo pipefail

PROFILE="debug"
if [[ "${1:-}" == "--release" ]]; then
    PROFILE="release"
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

BIN="target/$PROFILE/aurora"
HTTP_PORT="${AURORA_TEST_HTTP_PORT:-18080}"
RTMP_PORT="${AURORA_TEST_RTMP_PORT:-11935}"
STREAM_KEY="${AURORA_TEST_STREAM_KEY:-live/integration}"
LOG="$(mktemp -t aurora-integration-XXXXXX.log)"
SERVER_PID=""

PASS=0
FAIL=0

cleanup() {
    if [[ -n "$SERVER_PID" ]] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -9 "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f "$LOG"
}
trap cleanup EXIT

ok() {
    PASS=$((PASS + 1))
    printf '  \033[32mPASS\033[0m %s\n' "$1"
}

fail() {
    FAIL=$((FAIL + 1))
    printf '  \033[31mFAIL\033[0m %s\n' "$1"
}

check() {
    local name="$1" expected="$2" actual="$3"
    if [[ "$actual" == *"$expected"* ]]; then
        ok "$name"
    else
        fail "$name (expected to contain '$expected', got '$actual')"
    fi
}

if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "ffmpeg is required for the integration test" >&2
    exit 2
fi

echo "==> building ($PROFILE)"
cargo build --workspace --profile "$PROFILE" >/dev/null

echo "==> deploying plugins"
cp "target/$PROFILE/libaurora_rtmp.so" plugins/
cp "target/$PROFILE/libaurora_remote_desktop.so" plugins/

echo "==> starting engine on http://127.0.0.1:$HTTP_PORT, rtmp://127.0.0.1:$RTMP_PORT"
cat > /tmp/aurora-integration-config.toml <<EOF
[server]
host = "127.0.0.1"
http_port = $HTTP_PORT
rtmp_port = $RTMP_PORT

[plugins]
directory = "./plugins"
auto_load = true

[logging]
level = "info"
format = "pretty"
EOF

"$BIN" serve --config /tmp/aurora-integration-config.toml >"$LOG" 2>&1 &
SERVER_PID=$!

# Wait for the API to answer.
for _ in $(seq 1 50); do
    if curl -fsS -m 1 "http://127.0.0.1:$HTTP_PORT/api/health" >/dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

if ! curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/health" >/dev/null 2>&1; then
    echo "engine did not come up:" >&2
    cat "$LOG" >&2
    exit 1
fi

echo "==> engine is up"

echo "==> checking the API before any stream"
HEALTH="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/health")"
check "health reports healthy" '"status":"healthy"' "$HEALTH"
check "health reports the uptime" '"uptime"' "$HEALTH"

PLUGINS="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/plugins")"
check "rtmp plugin is loaded" '"name":"rtmp"' "$PLUGINS"
check "remote_desktop plugin is loaded" '"name":"remote_desktop"' "$PLUGINS"

PLUGIN_INFO="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/plugins/rtmp")"
check "plugin detail route works" '"routes"' "$PLUGIN_INFO"

echo "==> pushing an RTMP stream"
ffmpeg -hide_banner -loglevel error -re \
    -f lavfi -i "testsrc=size=320x240:rate=15" \
    -f lavfi -i "sine=frequency=440" \
    -c:v libx264 -preset ultrafast -t 8 -g 30 -c:a aac \
    -f flv "rtmp://127.0.0.1:$RTMP_PORT/$STREAM_KEY" &
FFMPEG_PID=$!

# Give the publisher time to establish the session.
sleep 3

echo "==> checking the API during the stream"
STATS="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/stats")"
check "the stream is listed" "\"key\":\"$STREAM_KEY\"" "$STATS"
check "the video codec was detected" '"video_codec":"h264"' "$STATS"
check "the audio codec was detected" '"audio_codec":"aac"' "$STATS"

STREAMS="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/streams")"
check "the streams endpoint lists the stream" "$STREAM_KEY" "$STREAMS"

SESSIONS="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/rtmp/sessions")"
check "the plugin route reports the session" '"state":"publishing"' "$SESSIONS"
check "the session counted the bytes" '"bytes_received":' "$SESSIONS"

wait "$FFMPEG_PID" 2>/dev/null || true

echo "==> checking the API after the stream ended"
sleep 1
STATS="$(curl -fsS -m 2 "http://127.0.0.1:$HTTP_PORT/api/stats")"
check "the stream was removed" '"streams":0' "$STATS"

echo "==> checking the graceful shutdown"
kill -TERM "$SERVER_PID"
for _ in $(seq 1 30); do
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        break
    fi
    sleep 0.2
done

if kill -0 "$SERVER_PID" 2>/dev/null; then
    fail "the engine did not stop on SIGTERM"
    kill -9 "$SERVER_PID" 2>/dev/null || true
else
    ok "the engine stopped on SIGTERM"
    wait "$SERVER_PID" 2>/dev/null || true
    SERVER_PID=""
fi

if grep -q "all plugins stopped" "$LOG"; then
    ok "the plugins were stopped cleanly"
else
    fail "the plugins were not stopped cleanly"
fi

if grep -qi "segmentation fault\|panic" "$LOG"; then
    fail "the engine log contains a crash"
    grep -i "segmentation fault\|panic" "$LOG" >&2
else
    ok "no crash in the engine log"
fi

echo
echo "passed: $PASS, failed: $FAIL"
[[ "$FAIL" -eq 0 ]]
