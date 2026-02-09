# Architecture

## Philosophy
The editor is split into three layers that do not depend on each other directly:
- Editor core: data model, tools, commands, and file IO
- Renderer: OpenGL rendering and GPU resources
- UI: Dear ImGui panels and layout

This separation keeps editor logic testable and makes UI changes safe without touching rendering or core state.

## Main Loop
The application follows a predictable frame order:
1. Poll window events and update input state.
2. Begin ImGui frame and build the editor UI.
3. Gate input based on ImGui capture and Scene View hover.
4. Update editor tools using normalized input.
5. Render the Scene View to an offscreen framebuffer.
6. Render ImGui and present the final frame.

## Scene View vs UI
The Scene View is rendered into an offscreen framebuffer and displayed via `ImGui::Image` in the Scene View panel. This keeps the scene color stable and unaffected by ImGui window backgrounds or alpha. The UI only displays the framebuffer texture.

## Input Gating
Input is gated at the app layer using two checks:
- `ImGuiIO::WantCaptureMouse` and `ImGuiIO::WantCaptureKeyboard` for UI focus
- Scene View hover state for editor-specific mouse actions

Only when the Scene View is hovered and ImGui does not capture mouse input do editor tools receive mouse input.

## Editor Core
The editor state (`EditorState`) owns the tilemap, atlas metadata, selection, and undo history. Tools translate input into commands:
- Paint/Erase: per-cell changes grouped into a single stroke command
- Rect: preview while dragging, apply on release
- Fill: flood fill contiguous regions

## Undo / Command Model
Undo history stores only modified cells. Each command contains a list of changes with before/after values, which keeps memory usage reasonable and makes undo/redo deterministic.

## Rendering
Rendering uses a small 2D renderer for quads and lines. Tile rendering uses atlas UVs when a valid texture is present, otherwise falls back to a debug color palette.

## Why OpenGL + ImGui
OpenGL provides a minimal, portable rendering layer, and Dear ImGui enables a fast iteration cycle for editor UI. The combination keeps the project lightweight while still supporting a Unity-like docking layout.

## Extending the Editor
Common extension points:
- Add tools in `src/editor/Tools.*` and register hotkeys in the app layer
- Add panels in `src/ui/Panels.*`
- Extend file formats in `util/JsonLite.*` and update save/load flows
- Add rendering features in `src/render`
