# Usage

## Creating a New Map
1. Open the editor.
2. In the Inspector panel, select the Tilemap root if needed.
3. Set Width and Height, then click Apply.
4. Use Save As to create a new map file under `assets/maps`.

## Painting Tiles
- Select a tile from the Tile Palette or press `1` through `9` for quick selection.
- Use the toolbar to choose a tool.
- Paint: left click to paint the selected tile.
- Erase: right click to clear a tile.
- Rect: drag to preview a rectangle, release to apply. Right-drag erases.
- Fill: left click to flood-fill contiguous tiles.

## Selection
- Right-drag in the Scene View to select a rectangle of tiles.
- Hold Shift to add to selection.
- Hold Ctrl to toggle tiles in the selection.

## Camera Controls
- `W`, `A`, `S`, `D`: pan camera
- Mouse wheel: zoom
- Middle mouse drag: pan
- Hold Space and drag: pan (temporary)

## Saving and Loading
- `Ctrl+S`: save the current map
- `Ctrl+O`: open the current map
- File menu provides Save, Load, and Save As
- Recent files are listed in the Saves tab

## Theme Customization
- Open the Settings panel.
- Choose a preset or adjust opacity and rounding.
- Changes persist in `assets/config/editor.json`.

## Common Pitfalls
- Painting only works when the mouse is inside the Scene View panel.
- UI widgets capture input, which temporarily disables editor shortcuts.
- If a file is missing or invalid, the editor logs an error in the Console.
