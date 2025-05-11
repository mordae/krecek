# Krecek

## Combined Firmware Image

Every game gets 1 MiB.

| ID | Game        |
|---:|-------------|
|  0 | Menu        |
|  1 | Cerviiik    |
|  2 | Pong        |
|  3 | Krectris    |
|  4 | Minesweeper |
|  5 | Pacman      |
|  6 | Peckovana   |
|  7 | Platform    |
|  8 | Tecka       |
|  9 | Janek       |
| 10 | Kalkulacka  |
| 11 | Scoundrel   |
| 12 | Smileyrace  |
| 13 |             |
| 14 |             |
| 15 | Experiment  |

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
