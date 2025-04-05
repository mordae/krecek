# Krecek

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
