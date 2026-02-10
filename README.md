# Axiom Tile Editor
Unity-style 2D tile editor built with C++20, GLFW, OpenGL, Dear ImGui.

## Features
- Unity-style dockable editor UI
- Scene View rendered with OpenGL
- Tile painting / erase / pan
- Grid + zoom + camera
- Undo / redo
- Save / load tilemaps (JSON)
- Dark editor theme
- Designed for extension (layers, atlas, tools)

## Screenshots
Screenshots go here.

## Build Requirements
- Linux (X11)
- CMake >= 3.20
- Ninja (recommended)
- OpenGL 4.1+ capable GPU
- Packages (Arch example):
  cmake ninja glfw-x11 libx11 libxrandr libxinerama libxcursor libxi

## Clone & Build
Submodules missing? Run: `git submodule update --init --recursive`

Normal clone:
```bash
git clone <repo-url>
cd Axiom-Tile-Editor
./scripts/build.sh debug
./scripts/run.sh debug
```

Clone with submodules:
```bash
git clone --recurse-submodules <repo-url>
cd Axiom-Tile-Editor
./scripts/build.sh debug
./scripts/run.sh debug
```

Offline/system deps (no network):
```bash
# Install GLFW + spdlog from your package manager.
# Ensure ImGui is vendored in third_party/imgui.
USE_SYSTEM_DEPS=1 ./scripts/build.sh debug
./scripts/run.sh debug
```

## Controls (Quick Start)
- Mouse: left paint, right erase, middle drag pan
- Keyboard: WASD pan, mouse wheel zoom, 1-9 select tile
- Shortcuts: Ctrl+S save, Ctrl+O open, Ctrl+Z undo, Ctrl+Y redo
- Toolbar: switch tools and access Save/Load quickly
- Scene View focus: painting and camera input only work when the Scene View is hovered

## Project Structure (High-Level)
- `src/app`: application lifecycle, main loop, window management
- `src/editor`: editor state, tools, commands, selection, file IO
- `src/render`: OpenGL renderer, textures, framebuffer
- `src/ui`: Dear ImGui panels, layout, theme
- `src/platform`: GLFW input, actions, and platform glue
- `external` / `third_party`: bundled or optional third-party deps
- `scripts/`: build and run scripts

## Documentation
- `docs/ARCHITECTURE.md`
- `docs/USAGE.md`
- `docs/BUILDING.md`

## File Formats
- Tilemap JSON: stores map dimensions, tile size, atlas metadata, and tile index data in row-major order.
- Config files: editor settings and recents live in `assets/config/editor.json`; ImGui layout in `assets/config/imgui.ini`.

## Roadmap
- Tile atlas support
- Layers
- Selection + rectangle paint
- Flood fill
- Framebuffer-based Scene View
- Export / runtime integration

## License
MIT / TBD
