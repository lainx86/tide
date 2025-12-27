# tide

A modern, GPU-accelerated terminal emulator for Linux.

**Status:** Initial project setup (work in progress)

## Features (Planned)

- GPU-accelerated rendering via OpenGL
- Native Linux PTY support (forkpty)
- Built-in color themes
- Clean, modular architecture
- C++20 codebase

## Architecture

```
tide/
├── core/           # PTY management, ANSI parser, grid buffer
├── render/         # OpenGL renderer, FreeType font handling
├── input/          # Keyboard and mouse input
├── theme/          # Color theme system
├── platform/linux/ # GLFW window and event loop
└── app/            # Main entry point
```

## Building (WSL2 Ubuntu)

### Install Dependencies

```bash
sudo apt update
sudo apt install -y cmake g++ libglfw3-dev libfreetype-dev libgl1-mesa-dev
```

### Build

```bash
cd /path/to/tide
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
./tide
```

## Current State

This is the initial project scaffold. The following is implemented:

- ✅ CMake build system (C++20)
- ✅ PTY spawning with forkpty (functional)
- ✅ GLFW window with OpenGL 3.3 core profile
- ✅ Basic keyboard input forwarding to PTY
- ✅ Theme system with Tokyo Night and Dracula themes
- ⏳ ANSI parser (stub only)
- ⏳ Grid buffer rendering (stub only)
- ⏳ Font rendering with FreeType (stub only)

## TODO

See `TODO` comments throughout the codebase for implementation details.

Key next steps:
1. Implement full ANSI escape sequence parser
2. Add FreeType glyph atlas rendering
3. Implement instanced character grid rendering
4. Add scrollback buffer support
5. Implement text selection

## License

MIT
