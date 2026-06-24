.PHONY: all release debug test

all: release

release:
	cargo build --release

debug:
	cargo build

test:
	cargo test
