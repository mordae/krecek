# Krecek

## Combined Firmware Image

Every game gets 512 KiB.

| ID | Game        |     Offset |
|---:|-------------|-----------:|
|  0 | Menu        | 0x10000000 |
|  1 | Cerviiik    | 0x10080000 |
|  2 | Pong        | 0x10100000 |
|  3 | Krectris    | 0x10180000 |
|  4 | Minesweeper | 0x10200000 |
|  5 | Pacman      | 0x10280000 |
|  6 | Peckovana   | 0x10300000 |
|  7 | Platform    | 0x10380000 |
|  8 | Tecka       | 0x10400000 |
|  9 | Janek       | 0x10480000 |
| 10 | Kalkulacka  | 0x10500000 |
| 11 | Scoundrel   | 0x10580000 |
| 12 | Smileyrace  | 0x10600000 |
| 13 |             | 0x10680000 |
| 14 | Deep        | 0x10700000 |
| 15 | Experiment  | 0x10780000 |
| 16 |             | 0x10800000 |
| 17 |             | 0x10880000 |
| 18 |             | 0x10900000 |
| 19 |             | 0x10980000 |
| 20 |             | 0x10a00000 |
| 21 |             | 0x10a80000 |
| 22 |             | 0x10b00000 |
| 23 |             | 0x10b80000 |
| 24 |             | 0x10c00000 |
| 25 |             | 0x10c80000 |
| 26 |             | 0x10d00000 |
| 27 |             | 0x10d80000 |
| 28 |             | 0x10e00000 |
| 29 |             | 0x10e80000 |
| 39 |             | 0x10f00000 |
| 31 |             | 0x10f80000 |

## Development on Krecek

```bash
# Prepare build for Krecek
cmake -B build src -G Ninja

# Build all games for Krecek
cmake --build build

# Load the game we want
picotool load -f build/pong/pong.uf2

# Connect to virtual serial line for debugging messages
minicom -c on -D /dev/ttyACM0
```

## Development on PC

```bash
# Install dependencies
sudo dnf install SDL2-devel SDL2_Pango-devel pulseaudio-libs-devel libasan

# Prepare the project in Debug mode with address sanitizer
cmake -B build/host host -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build the project for host
cmake --build build/host

# Ask address sanitizer to ignore allocation issues.
export ASAN_OPTIONS='detect_leaks=0'

# Run a game
build/host/pong/pong
```

## Git

```bash
# Review modified files
git status

# Reset changes to a file we don't want to commit
git checkout path/to/file

# Add all changes
git add .

# Commit them
git commit -m 'pong: changed paddle colors'

# Pull and rebase our changes on top of what others did
git pull --rebase

# Push our changes
git push
```

## Add New Game

- Copy both `src/game` and `host/game`.
- Rename game in `src/game/CMakeLists.txt` and `host/game/CMakeLists.txt`.
- Adjust `src/CMakeLists.txt` and `host/CMakeLists.txt`.
- Create symbolic link to assets, if needed:
  ```bash
  cd host/game
  ln -svf ../../src/game/assets
  ```
