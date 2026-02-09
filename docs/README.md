# Tile Editor Starter (C++20, GLFW + OpenGL Core)

## Dependencies (Arch/CachyOS)
- `cmake`, `ninja`, `git`, `gcc`
- OpenGL + X11 development libs (Mesa + Xorg):
  - `mesa`, `libx11`, `libxrandr`, `libxinerama`, `libxcursor`, `libxi`

GLFW is pulled via CMake FetchContent, so a system GLFW package is optional.

Minimum Arch deps for system GLFW mode:
`sudo pacman -S --needed cmake ninja pkgconf glfw-x11`

If needed, add X11/Wayland libs:
`sudo pacman -S --needed libx11 libxrandr libxinerama libxcursor libxi wayland`

## Build
From the repo root:
```bash
./tile-editor/scripts/build.sh
```

System GLFW mode:
```bash
USE_SYSTEM_GLFW=1 ./scripts/build.sh
```

If configure hangs, use system glfw mode:
```bash
USE_SYSTEM_GLFW=1 ./scripts/build.sh clean debug
```

## Fast Build
Default (offline submodule deps):
```bash
git submodule update --init --recursive
./scripts/build.sh clean debug
```

System deps:
```bash
USE_SYSTEM_GLFW=1 ./scripts/build.sh clean debug
```

FetchContent cache:
```bash
FETCHCONTENT_BASE_DIR="$PWD/.cache/fetchcontent" ./scripts/build.sh clean debug
```

## Run
```bash
./scripts/run.sh debug
```

## Controls
- `WASD`: pan camera
- Mouse wheel: zoom
- Middle-mouse drag: pan
- `1-9`: select tile id
- Left click: paint tile
- Right click: erase (tile id = 0)
- `Ctrl+S`: save to `assets/maps/map.json`
- `Ctrl+O`: load from `assets/maps/map.json`
- `Ctrl+Z`: undo
- `Ctrl+Y`: redo

## File Format
`assets/maps/map.json` is a tiny, human-readable JSON payload:
```json
{
  "width": 32,
  "height": 32,
  "tileSize": 32,
  "data": [0, 1, 0, 2, ...]
}
```
`data` is a row-major array of `width * height` integers.

## Design Notes
- OpenGL core context: try 4.6, fallback to 3.3 if needed.
- Debug output is enabled in debug builds when supported.
- Renderer uses a simple batched 2D quad/line pipeline.
- sRGB framebuffer is enabled when available.
- `stb_image` is stubbed for now; replace `external/stb/stb_image.h` with the official header to enable texture loading.
- ImGui is fetched via CMake; `IMGUI_DOCKING=ON` (default) pulls the docking branch. Set `IMGUI_DOCKING=OFF` to use a stable tag if the docking branch cannot be fetched.

## Future: Dear ImGui Docking Integration
This project is structured to add an ImGui layer without touching core editor state:
- Add `ui/ImGuiLayer.*` and insert `BeginFrame()` / `EndFrame()` calls in the App loop.
- Use the Dear ImGui docking branch and create a `DockSpace` root window.
- Optional: enable multi-viewport for native OS windows.

The `render/` module already exposes a clean begin/end frame split to slot ImGui’s frame lifecycle.
