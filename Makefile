.PHONY: build build-release plugins plugins-release test check clean help

PROFILE ?= debug

help:
	@echo "AuroraStream"
	@echo
	@echo "  make build           build the workspace (debug)"
	@echo "  make build-release   build the workspace (release)"
	@echo "  make plugins         build and deploy the plugin .so files to plugins/"
	@echo "  make plugins-release build and deploy the release plugins"
	@echo "  make test            run the integration test"
	@echo "  make check           build + clippy style warning check"
	@echo "  make clean           remove the build output"

build:
	cargo build --workspace

build-release:
	cargo build --workspace --release

# The plugins are loaded from plugins/ at runtime, so they have to be copied there
# after every build.
plugins: build
	cp target/$(PROFILE)/libaurora_rtmp.so plugins/
	cp target/$(PROFILE)/libaurora_remote_desktop.so plugins/
	@echo "deployed plugins to plugins/"

plugins-release: build-release
	cp target/release/libaurora_rtmp.so plugins/
	cp target/release/libaurora_remote_desktop.so plugins/
	@echo "deployed release plugins to plugins/"

test:
	./tests/integration/run.sh $(if $(filter release,$(PROFILE)),--release,)

check:
	cargo build --workspace 2>&1 | grep -E "^(warning|error)" || echo "no warnings"

clean:
	cargo clean
