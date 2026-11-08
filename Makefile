# Makefile for Captain Comic II: Fractured Reality
#
# NOTE: comic2.asm is an IDA Pro MASM-style disassembly and cannot be
# assembled with NASM. The goal of this project is a C++ reimplementation,
# not a reassemblable binary.

.DEFAULT_GOAL := build

# Build the djlink/bindiff tools from source
tools: tools/djlink/djlink tools/djlink/bindiff

build:
	@cmake -S . -B build
	@cmake --build build --parallel

run: build
	@./build/comic2_renderer_demo

tools/djlink/djlink tools/djlink/bindiff:
	$(MAKE) -C tools/djlink djlink bindiff

clean-tools:
	$(MAKE) -C tools/djlink clean

.PHONY: tools clean-tools quality cppcheck format-check format build run check-compiler_warnings

quality: cppcheck format-check check-compiler_warnings

cppcheck:
	@docker run --name cppcheck_$(shell basename ${PWD}) --rm -v ${PWD}:/workspace \
		--workdir /workspace \
		--pull always \
		alpine:edge \
		sh -c "apk add --no-cache cppcheck > /dev/null 2>&1; \
		cppcheck --enable=all \
		--inconclusive \
		--suppress=missingIncludeSystem \
		--check-level=exhaustive \
		--inline-suppr \
		-I include \
		src tests"

format-check:
	@if ! command -v clang-format > /dev/null; then \
		echo "Error: clang-format is not installed." >&2; \
		exit 1; \
	fi
	@find src include tests \
        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.h" \) \
        -print0 | xargs -0 clang-format --dry-run --Werror

format:
	@if ! command -v clang-format > /dev/null; then \
		echo "Error: clang-format is not installed." >&2; \
		exit 1; \
	fi
	@find src include tests \
        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cc" -o -name "*.h" \) \
        -print0 | xargs -0 clang-format -i

check-compiler_warnings:
	@cmake -B build -S . \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"

	@cmake --build build --config Debug

runapp: build
	@./build/comic2_reimpl reference/original