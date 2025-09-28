# XOCHIP

A small, single-header, embeddable XO-CHIP/CHIP-8 emulator.

XOCHIP is designed to be embedded in existing projects. I originally created it to for physical handheld XO-CHIP
emulators on microcontrollers. I didn't want to rewrite the hard parts for each platform, so I created an emulator with
just enough API functions to make it flexible across them.

This project includes an optional SDL3 desktop demo build with CMake. However, all you need for your project is to
include `xochip.h`. Include as needed and define `XOCHIP_IMPLEMENTATION` in exactly one C/C++ translation unit to
compile the implementation.

## Overview

- Core: header-only C99 implementation providing an `xochip_t` emulator with helpers to initialize, load ROMs, execute
  cycles, and interact with keypad and display.
- Desktop demo (optional): SDL3-based example in `emulator.c` showcasing a simple windowed emulator using SDL's app
  callbacks.
- Tests: Uses the SDL3 demo and [Timendus](https://github.com/Timendus/chip8-test-suite)' ROM test suite

## Requirements

- A C toolchain with C99 support (e.g., MSVC, Clang, GCC)
- CMake (see note below)

Notes:

- SDL3 is only required if you enable the desktop emulator build flag (see below). Unity is auto-fetched for tests.

## Build and run the demo.

This project uses standard CMake. Examples assume a shell at the repository root.

### Configure

First, create a build directory with the BUILD_DESKTOP_EMULATOR option.

```bash
cmake -S . -B build -DBUILD_DESKTOP_EMULATOR=ON
```

### Build

Build everything for your chosen config:

- `cmake --build build --config Debug`
- or `cmake --build build --config Release`

### Run the desktop emulator (optional)

- Target name: `xochip-emulator`
- Binary location (example):
    - Windows (Multi-config generators like Visual Studio): `build/Debug/xochip-emulator.exe` or
      `build/Release/xochip-emulator.exe`
    - Single-config generators (e.g., Ninja/Unix Makefiles): `build/xochip-emulator`

On Windows, the CMake script copies the SDL3 shared library next to the executable after build.

## How to use it.

Include `xochip.h` everywhere you need the API. In exactly one source file (probably your main), define
`XOCHIP_IMPLEMENTATION` before including it:

```c
// xochip_impl.c
#define XOCHIP_IMPLEMENTATION
#include "xochip.h"
```

Here are the key functions:

- `xochip_init(xochip_t*)`/`xochip_reset(xochip_t*)` to initialize/reset the emulator, assuming `xochip_t` is not NULL.
- `xochip_load_rom(xochip_t*, const uint8_t *data, uint16_t size)` to load a ROM into the emulator.
- `xochip_cycle(xochip_t*)` to execute the next instruction. Timing is up to you, ~500 Hz is a good starting point.
- `xochip_tick(xochip_t*)` to tick the sound and delay counters, recommended you call this function at 60 Hz.
- `xochip_key_down(...)`/`xochip_key_up(...)` for input.
- Inspect `xochip_t.display` fields for pixel planes and update flag (TODO: add function for this, because fields are
  supposed to be "private")

See `emulator.c` for usage examples.

## Tests

Timendus' test ROMs are bundled in `tests/`. To test the emulator, build the desktop demo and load the test ROMs.

TODO: create a command line API for the following options:

- Instruction frequency (run `xochip_cycle()` at X Hz, probably ~500 Hz)
- Tick frequency (run `xochip_tick()` at X Hz, 60 Hz to match specs)
- The ROM to load

## Configuration and environment variables

- Build flag: `BUILD_DESKTOP_EMULATOR` (OFF by default)
    - OFF: build only the core and tests (Unity is fetched automatically)
    - ON: also fetch SDL3 and build the `xochip-emulator` demo
- No project-specific runtime environment variables are required. SDL provides optional environment variables for
  debugging, but none are required by this repo.

## Project structure

- `xochip.h` — Header-only XO-CHIP/CHIP-8 core (define `XOCHIP_IMPLEMENTATION` in one TU)
- `emulator.c` — SDL3 desktop demo (built when `BUILD_DESKTOP_EMULATOR=ON`)
- `CMakeLists.txt` — Build configuration (FetchContent SDL3)
- `tests/*.ch8` — Timendus' test ROMs

## Targets (CMake)

- `xochip-emulator` (executable) — SDL3 desktop demo (only if `BUILD_DESKTOP_EMULATOR=ON`)
- SDL3 libraries are added via FetchContent as needed

## Known issues / TODOs

- The desktop demo (`emulator.c`) is a skeleton and currently doesn’t implement full timing, input mapping, drawing, or
  audio. TODO: complete the event loop and rendering using `xochip_display` planes and implement keypad mapping via the
  provided `xochip_keys_t` enum.

