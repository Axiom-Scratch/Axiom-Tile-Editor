# Building

## Build Configurations
- Debug: `./scripts/build.sh debug`
- Release: `./scripts/build.sh release`
- Clean build: `./scripts/build.sh clean debug` or `./scripts/build.sh clean release`

## Scripts
- `./scripts/build.sh`: configures and builds with CMake
- `./scripts/run.sh`: runs the built binary from `build/<config>/tile_editor`

## Dependency Modes
The project supports three dependency modes:
- FetchContent (default): downloads ImGui and GLFW during configure
- Submodules: uses `third_party/glfw` if present and `USE_SUBMODULE_DEPS=ON`
- System GLFW: set `USE_SYSTEM_GLFW=1` to use your distro package

Submodules missing? Run: `git submodule update --init --recursive`

## Recommended Build Flow
1. Initialize submodules if you want offline builds.
2. Build with the default scripts.

Example:
```bash
git submodule update --init --recursive
./scripts/build.sh clean debug
./scripts/run.sh debug
```

## Common Build Errors and Fixes
- FetchContent download fails: initialize submodules or set `USE_SYSTEM_GLFW=1`.
- Missing X11 headers or GLFW: install `libx11 libxrandr libxinerama libxcursor libxi`.
- Configure timeout: retry with `USE_SYSTEM_GLFW=1` or set `FETCHCONTENT_BASE_DIR` to a cached location.
- `timeout` not found: install `coreutils`.

## CMake Options
- `USE_SUBMODULE_DEPS=ON`: prefer deps in `third_party/` when present
- `USE_SYSTEM_GLFW=ON`: use system-installed GLFW
- `IMGUI_DOCKING=ON`: build ImGui from the docking branch

## Updating Dependencies
### ImGui
- Update the tag in `CMakeLists.txt` or switch `IMGUI_DOCKING`.
- Rebuild and verify `imgui_impl_glfw` and `imgui_impl_opengl3` API changes.

### GLFW
- If using submodules: update `third_party/glfw` and rebuild.
- If using system GLFW: update via your package manager and rebuild.

## Troubleshooting Tips
- Use `VERBOSE=1 ./scripts/build.sh debug` for detailed logs.
- Check `build/<config>/CMakeFiles/CMakeOutput.log` and `CMakeError.log` on configure failures.
