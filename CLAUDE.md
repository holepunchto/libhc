# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

`libhc` is a C library for [hypercore](https://github.com/holepunchto/libhc) by Holepunch. The public API lives in `include/hc.h`. The build system uses CMake, driven by `bare-make` (a Node-based CMake wrapper).

## Commands

Install dependencies (provides `cmake-fetch` for CMake):
```sh
npm install
```

Configure, build, and test:
```sh
bare-make generate --platform <linux|darwin|win32> --arch <x64|arm64> --debug
bare-make build
bare-make test
```

For local development, omit `--platform`/`--arch` to target the host:
```sh
bare-make generate --debug
bare-make build
bare-make test
```

## Architecture

- `include/hc.h` — sole public header; all exported types and functions go here.
- `CMakeLists.txt` — builds three targets: `hc` (OBJECT), `hc_shared` (SHARED, output name `hc`), `hc_static` (STATIC, output name `libhc`). Source files are added to the `hc` OBJECT target; the shared/static targets link it.
- `test/CMakeLists.txt` — add test names to the `tests` list; each entry maps to a `<name>.c` file compiled as an executable and registered with CTest. Tests link against `hc_static`.
- `cmake-fetch` is resolved from `node_modules/cmake-fetch` (not system CMake packages).

## Code style

`.clang-format` is present and enforced. Notable non-defaults:
- No column limit (`ColumnLimit: 0`)
- `BlockIndent` for alignment after open brackets
- Space before parentheses on function declarations and definitions
- Top-level return types always broken onto their own line

C standard is C99 (`C_STANDARD 99`).
