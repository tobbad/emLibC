# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`emLibC` — a portable embedded-C utility library (`github.com/tobbad/emLibC`). It is its **own git repo**, vendored as a submodule into the firmware project at `lib/emLibC`. The parent project (`../../CLAUDE.md`) consumes this code from the STM32 firmware and via a Cython wrapper; that file covers the firmware/Python layers and the shared `state_t`/`payload_t`/`AppliFrame_t` wire structs. This file is only about the library itself.

`test/googletest` is a nested git submodule — run `git submodule update --init --recursive` before building tests.

## Source layout

- `common/` — portable utilities, the bulk of the library. `src/`, `inc/`, and host-side `test/*.cpp` (GoogleTest). Modules: `common`, `buffer`, `buffer_pool`, `ringbuffer`, `cycle`, `state`, `stateled`, `keyboard`, `xpad`, `packet`, `slip`, `device`, `memory_device`, `terminal`, `_time`, `mutex`, `endian`, `fifo`, `stream`.
- `wire/` — byte transports: `i2c`, `spi`, `serial`.
- `port/` — HAL abstraction. `port/inc/` declares the MCU-agnostic GPIO API (`gpio_pin_t`/`gpio_port_t`, `GpioPin*`/`GpioPort*`); `port/src/stm/stm32{l4,f3,f4}/hal_port.h` each just include the right `stm32XXxx.h` and define `MCU`. The build picks one family's `hal_port.h` include path.

## The `UNIT_TEST` switch (central mechanism)

`common.h` gates all MCU coupling on `UNIT_TEST`:
- **Defined** (host build / unit tests): skips `#include "hal_port.h"`, so no vendor HAL is needed, and `#define STATIC` is empty — file-scope `static` functions become externally linkable and therefore testable.
- **Undefined** (firmware build): pulls in `hal_port.h` (→ the STM32 HAL) and `STATIC` is real `static`.

When adding code that touches hardware, wrap it so the host test build still compiles with `UNIT_TEST`.

## Building / running tests (CMake + GoogleTest, host)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ctest -V
./build/cycle_test           # run a single test binary directly
```
Debug builds compile with AddressSanitizer (`-fsanitize=address`). C++17, `UNIT_TEST` is defined automatically when `DEBUG` is on.

**`CMakeLists.txt` enables tests/sources by exclusion, not inclusion.** It `file(GLOB ...)`s everything under `common/src` and `common/test`, then `list(REMOVE_ITEM ...)` disables the rest. So which tests actually build is whatever is *not* removed — currently only `cycle_test` (its `REMOVE_ITEM` at line ~63 is commented out; all other `*_test.cpp` are removed). To work on another module, comment out its `REMOVE_ITEM` line for both the test `.cpp` (lines ~56-63) and any source `.c` it needs (lines ~45-53). Each surviving `*_test.cpp` is linked with `test/AllTests.cpp` (the `main`) into its own executable named after the file. Note: the parent project's CLAUDE.md names different "active" tests — trust the current `CMakeLists.txt` over that.

`PROJECT_INC_DIR` points at `../../Core/Inc` (the firmware's headers), so the host test build expects this repo to sit inside the parent project tree.

## Alternate build (SConstruct)

`SConstruct` is a second, partly-stale build path driven by `scons target=<...> debug=<0|1>`. `target=test_common` builds the common host tests; `target=emlib` cross-compiles with `arm-none-eabi-*`. CMake is the primary, working path; reach for SCons only if explicitly asked.

## Conventions

- Functions return `em_msg` (`EM_ERR=-1`, `EM_OK=0`, `EM_TRUE`) for status.
- `.clang-format` is checked in; formatting/tidy is driven from the **parent** project's root `Makefile` (`make format` / `make tidy`), which targets `arm-none-eabi`/C11.
- Build/library metadata: `library.json` (PlatformIO).
