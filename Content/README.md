# Project Content

Game/project assets used by `GameEngineApp` live here.

Current folders:

- `images`
- `fonts`
- `sounds`
- `tilemaps`
- `scripts`
- `Scenes`

Runtime code loads Lua levels from `Content/scripts` and those levels reference assets with `./Content/...` paths.
Editor scene files live under `Content/Scenes`; `PlatformerTest.amber.scene` is the current editor-to-Platformer integration test scene.
