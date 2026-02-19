BUILD_DIR ?= build
CMAKE_GENERATOR ?= Unix Makefiles

.PHONY: all configure build run run-sample check clean

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -G "$(CMAKE_GENERATOR)" -DCLOT_ENABLE_LLVM=ON

build: configure
	cmake --build $(BUILD_DIR)

run: build
	./$(BUILD_DIR)/clot examples/basic.clot

run-sample: build
	./$(BUILD_DIR)/clot examples/basic.clot

check:
	./scripts/check.sh

clean:
	rm -rf $(BUILD_DIR)
