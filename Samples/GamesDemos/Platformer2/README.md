# Platformer2 Sample

`Platformer2App` is a tilemap-backed platformer sample built from the Kenney 1-Bit Platformer Pack assets in `Content`. It opens into a built-in map editor with a rendered level viewport, visual tileset palette, scene outliner, ladders, spikes, vertical and horizontal lifts, coins, animated player sprites, animated enemies and player projectiles.

Controls:

- `A` / `Left`: move left
- `D` / `Right`: move right
- `W` / `Up`: climb ladders
- `S` / `Down`: climb down ladders
- `Space`: jump; press again in the air for a double jump
- `J` / `Ctrl`: shoot
- `R`: reset level
- `P`: pause
- `F1`: toggle Map Editor / play view
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

- `Map Editor`: choose tool, tile kind, tile ID, save/load and pick from the visual tileset palette.
- `Map View`: rendered editor canvas with the actual level tiles, player spawn, enemies, lifts, goal, camera sliders and zoom.
- `Scene Outliner`: select player spawn, goal, enemies and lifts, then edit their properties.
- Left mouse applies the active tool inside `Map View`.
- Right mouse erases a tile.
- `Play From Here` switches from the editor into the playable platformer.
- `Save` writes `Content/Maps/Platformer2Level.txt`.

Build:

```powershell
.\Setup.bat -Target Platformer2
```

Smoke test:

```powershell
.\Setup.bat -Target Platformer2 -RunSmoke -NoConfigure
```
