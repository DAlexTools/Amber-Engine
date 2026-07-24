# Platformer2 Sample

`Platformer2App` is a tilemap-backed platformer sample built from the Kenney 1-Bit Platformer Pack assets in `Content`. The first level is authored in code from the packed 16x16 tileset and includes ladders, spikes, vertical and horizontal lifts, coins, animated player sprites, animated enemies and player projectiles.

Controls:

- `A` / `Left`: move left
- `D` / `Right`: move right
- `W` / `Up`: climb ladders
- `S` / `Down`: climb down ladders
- `Space`: jump; press again in the air for a double jump
- `J` / `Ctrl`: shoot
- `R`: reset level
- `P`: pause
- `F11` / `Alt+Enter`: toggle fullscreen
- `Esc`: quit

Content:

```text
Content/Tilemap/monochrome_tilemap_transparent_packed.png
Content/Tiles/
```

The game renders from the packed tilemap, while the separate `Tiles` folders remain available for future editor/import tooling.

Build:

```powershell
.\Setup.bat -Target Platformer2
```

Smoke test:

```powershell
.\Setup.bat -Target Platformer2 -RunSmoke -NoConfigure
```
