# Platformer2 Sample

`Platformer2App` is a tilemap-backed platformer sample built from the Kenney 1-Bit Platformer Pack assets in `Content`. It includes ladders, spikes, vertical and horizontal lifts, coins, animated player sprites, animated enemies, player projectiles and an in-app map editor.

Controls:

- `A` / `Left`: move left
- `D` / `Right`: move right
- `W` / `Up`: climb ladders
- `S` / `Down`: climb down ladders
- `Space`: jump; press again in the air for a double jump
- `J` / `Ctrl`: shoot
- `R`: reset level
- `P`: pause
- `F1`: toggle Map Editor / Scene Outliner
- `F11` / `Alt+Enter`: toggle fullscreen
- `Esc`: quit

Content:

```text
Content/Tilemap/monochrome_tilemap_transparent_packed.png
Content/Tiles/
Content/Maps/Platformer2Level.txt
```

The game renders from the packed tilemap, while the separate `Tiles` folders remain available for future editor/import tooling. If `Content/Maps/Platformer2Level.txt` exists, the app loads it; otherwise it falls back to the built-in sample level.

Map Editor:

- `Map Editor`: choose tool, tile kind, tile ID, camera position and save/load.
- `Scene Outliner`: select player spawn, goal, enemies and lifts.
- The level itself is the editor canvas.
- `Tileset Palette` shows the full tilemap visually; click a tile to select it for painting.
- Left mouse applies the active tool.
- Right mouse erases a tile.
- `Save` writes `Content/Maps/Platformer2Level.txt`.

Build:

```powershell
.\Setup.bat -Target Platformer2
```

Smoke test:

```powershell
.\Setup.bat -Target Platformer2 -RunSmoke -NoConfigure
```
