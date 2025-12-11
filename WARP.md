# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## Build and run

### Firmware (Křeček board / RP2040)

Prereqs:
- Pico SDK available and `PICO_SDK_PATH` set (used by `src/CMakeLists.txt`).
- `cmake`, `ninja`, `picotool`, and a serial terminal (e.g. `minicom`).

Configure and build all firmware images:

```bash
cmake -B build src -G Ninja
cmake --build build
```

Flash a single game (example: `pong`) to the board:

```bash
picotool load -f build/pong/pong.uf2
```

Connect to the virtual serial port for logs/debugging (device path may vary):

```bash
minicom -c on -D /dev/ttyACM0
```

To build games with fixed flash offsets for a combined multi‑game image, enable the `ENABLE_OFFSETS` CMake option when configuring `src/`.
The root `README.md` contains the ID → flash offset table for these slots (each game gets 512 KiB).

### Host (PC simulator)

The `host/` tree builds SDL2-based desktop binaries that simulate the Křeček hardware.
On Fedora, typical dependencies are:

```bash
sudo dnf install SDL2-devel SDL2_Pango-devel pulseaudio-libs-devel libasan
```

Configure and build the host simulator in Debug mode with address sanitizer:

```bash
cmake -B build/host host -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host
```

Run an individual game on the host (example: `pong`):

```bash
# from the repo root after building
build/host/pong/pong
```

Address sanitizer is typically configured to ignore leak reports:

```bash
export ASAN_OPTIONS='detect_leaks=0'
```

There is no centralized CTest or unit-test target; testing is done by running the individual game binaries (either on hardware or via the host simulator).

## Project layout and architecture

### Two CMake roots: `src/` (firmware) and `host/` (PC simulator)

- `src/CMakeLists.txt` configures the RP2040 firmware build for board `krecek-011`, pulls in the Pico SDK via `PICO_SDK_PATH`, and wires up common configuration (C standard, warnings, Pico stdio, etc.) through the `krecek_set_target_options` helper.
- `host/CMakeLists.txt` configures the desktop build, discovers SDL2 via `find_package`, sets screen simulation options via preprocessor defines (driver type, scaling, axis swap, vsync), and adds a subdirectory for each host-side game.
- Both roots include shared CMake utilities from `tools/` (`png_to_header.cmake` and `bin_to_header.cmake`) and rely on a `generate_png_headers()` macro to convert PNG assets into C headers that are linked into games.

### Shared SDKs: runtime for games

**Firmware SDK (`src/sdk/`):**
- Built as a library target `krecek` (see `src/sdk/CMakeLists.txt`).
- Implements the core runtime for games: startup (`main.c`), save handling (`save.c`), audio (`audio.c`, `melody*.c`), scene management, video and input, inter‑core communication (`slave.c`, `remote.c`, `dap.c`, `comms.c`), and SD‑card / FAT filesystem support (`sdcard*.c`, `fatfs/*`).
- Links against Pico-specific libraries (`pico_*`, `hardware_*`) and vendor components such as `pico_task` and `pico_tft`.
- Exports the `sdk_*` API and TFT helpers used by every game (for input, drawing, timing, audio, randomness, etc.).

**Host SDK (`host/sdk/`):**
- Library target `sdk` that reuses game-agnostic parts from `src/sdk` (e.g. `melody.c`, `scene.c`) and vendor font code, and replaces hardware access with SDL2-based implementations (`audio.c`, `video.c`, `input.c`, `sdcard.c`, `tft.c`, `pico.c`).
- `host/sdk/main.c` provides the main event loop: it initializes SDL, creates a window sized to match the TFT, calls `game_start()` / `game_reset()`, and repeatedly:
  - polls SDL events and forwards them to the SDK input layer,
  - advances game input and audio (`sdk_input_commit`, `sdk_audio_*`),
  - polls communications (`sdk_comms_poll`),
  - calls `game_paint(dt)` and pushes the rendered frame into an SDL texture.
- Declares weak default implementations of `game_start`, `game_reset`, `game_audio`, `game_input`, `game_paint`, and `game_inbox`; each actual game overrides these by defining its own versions in the shared game source.

### Games

- Each game has **two CMake entries** but largely **shares one codebase**:
  - `src/<game>/` contains the actual game source (e.g. `src/pong/main.c`).
  - `host/<game>/CMakeLists.txt` builds a desktop executable by compiling the same `src/<game>/main.c` against the host `sdk` library.
- A typical firmware game CMake file (e.g. `src/pong/CMakeLists.txt`):
  - `add_executable(<game> main.c)`
  - `target_include_directories(<game> PRIVATE include)`
  - `generate_png_headers()` and `target_link_libraries(<game> krecek ${PNG_HEADERS_TARGET})` to hook into the firmware SDK and asset system.
  - `pico_add_extra_outputs(<game>)` to generate UF2 and other Pico outputs; optionally `pico_set_linker_script` when `ENABLE_OFFSETS` is enabled.
- The corresponding host game CMake file (e.g. `host/pong/CMakeLists.txt`) builds a native binary from `../../src/pong/main.c`, linking with `sdk` and the generated PNG header target, and installs it into `bin/`.
- Game code uses the `sdk_*` API and TFT drawing helpers; for example, `src/pong/main.c` defines `game_input` and `game_paint`, reads from `sdk_inputs`, and renders into the TFT buffer using `tft_*` primitives. It then calls `sdk_main(&config)` in `main()` to enter the SDK’s main loop on both firmware and host.

### Vendors, tools, and assets

- `src/vendor/` contains third‑party libraries used by the firmware (e.g. `pico-task`, `pico-tft`) and is pulled in as a subdirectory by `src/CMakeLists.txt`.
- `tools/png_to_header.cmake` and `tools/bin_to_header.cmake` define macros used by both `src/` and `host/` to convert assets into C headers.
- Many games expect assets (sprites, maps, etc.) to live under `src/<game>/assets/`; host builds typically access these via symlinks from `host/<game>/assets` back into the `src` tree.

## Adding a new game

The root `README.md` documents the expected workflow for introducing a new game; in practice this looks like:

1. Copy an existing game directory under both trees, e.g. duplicate `src/pong` → `src/mygame` and `host/pong` → `host/mygame`.
2. Rename the CMake targets inside `src/mygame/CMakeLists.txt` and `host/mygame/CMakeLists.txt` to use your new game name.
3. Register the new subdirectory in both top‑level CMake files (`src/CMakeLists.txt` and `host/CMakeLists.txt`) so it is built for firmware and host.
4. If the game has assets, create a symlink from the host tree back to the shared asset directory so both builds see the same files, e.g. from `host/mygame`:
   - `ln -svf ../../src/mygame/assets`

When using flash offsets (combined firmware image), assign your game an unused slot from the table in `README.md` and configure its linker script accordingly.

## Experiment editor notes

The `experiment-editor` game has additional usage details in `src/experiment-editor/README.md`:
- It runs both on firmware (`src/experiment-editor`) and on the host (`host/experiment-editor`).
- On desktop, it defines a custom keymap for editing tile-based maps (e.g. cycling tiles, toggling collision flags, saving tile IDs and maps).
- Maps are stored as binary files under `src/experiment/assets/maps/`; you can create a new, empty map by touching the desired file path from the terminal (as described in that README).