# Platformer Project

`Platformer` is an in-repo game project laid out like a project created from `AmberEditor`: it has `Platformer.amberproject`, project-local `CMakeLists.txt`, `CMakePresets.json`, `Source/`, `Content/` and `Builds/`.

`PlatformerApp` is a small original side-scrolling platformer. Its gameplay lives in the reusable `PlatformerGameModule` library, while `PlatformerApp.exe` is the thin standalone launcher. Enemy setup and behavior are loaded from Lua, player projectiles can damage enemies, enemies can shoot back, and the level contains a small `AE::Physics::World` playground with crates, balls, a rope bridge, a hanging chain and moving platforms.

Controls:

- `A` / `Left`: move left
- `D` / `Right`: move right
- `W` / `Up` / `Space`: jump; press again in the air for a double jump
- `J` / `Ctrl`: shoot
- `R`: reset level
- `F11` / `Alt+Enter`: toggle fullscreen
- `Esc`: quit

The sample uses SDL for rendering and a simple tile-based AABB controller for platformer movement. It uses `Core/Math/Vector2D.h` for shared vector math.
Jumping uses a small input buffer, coyote time and one air jump, so pressing jump slightly before landing, just after leaving an edge, or once more in the air still works.

Scripted enemies:

```text
Content/Scripts/PlatformerEnemies.lua
```

Each enemy entry can define spawn position, patrol bounds, health, color, shooting parameters and an `on_update(enemy, player, dt)` function. C++ owns collision, damage, projectiles and rendering; Lua owns behavior tuning. The current script includes patrol, hopper and sentry variants across the ground route and vertical platforms.

Physics playground:

- Static tile bodies mirror solid level platforms.
- Dynamic crates and balls can be pushed by the player or hit by projectiles.
- Rope/chain segments use `JointConstraint`.
- Moving platforms use kinematic static bodies updated before the physics step and are valid player landing surfaces.

Editor scene bridge:

- `Content/Scenes/PlatformerTest.amber.scene` is loaded at startup when `SDL2_image` is available.
- `PlatformerApp` also accepts `--scene <path>` so `AmberEditor` can launch the game against the currently saved scene.
- `PlatformerGameModule` is the first PIE-oriented boundary: it implements `AE::IGameModule`, keeps the standalone launcher thin and exposes Platformer scene object registration for future embedded Editor play sessions.
- `PlatformerGameModulePlugin` is the dynamic module loaded by `AmberEditor` for PIE. It exports `AmberCreateGameModule` and `AmberDestroyGameModule`, then returns a `PlatformerGameModule` instance.
- In dynamic PIE today, the editor keeps scene object instantiation editor-side and passes the `SceneDocument` to `PlatformerGameModule::StartPlay`; plugin-owned ECS component registration waits for a shared runtime ABI.
- Platformer-specific scene object classes live in this sample (`PlatformerSceneObjects.h/.cpp`), not in the engine module.
- `PlayerSpawnObject`, `GoalObject`, `CoinObject` and `SolidPlatformObject` are registered into `ObjectFactory`, configured as ECS entities, then mapped into the sample's spawn point, finish trigger, collectible coins and level platforms. They now sit on top of engine standard `BoxObject`/`CircleObject` shapes, so Editor can display and move them as real scene geometry instead of invisible empty objects.
- `Content/Scenes/PlatformerTest.amber.scene` now owns the main level layout as boxes/circles. The old tile-built layout remains only as a fallback when no scene is available.
- Visible `SpriteObject` instances still render as decorative scene props from the editor scene.

Root build:

```powershell
.\Setup.bat -Target Platformer
```

Open in AmberEditor:

```powershell
.\Setup.bat -Target Editor
.\Builds\Editor\Engine\Editor\Shell\Debug\AmberEditor.exe .\Projects\Platformer\Platformer.amberproject
```

Register `.amberproject` for double-click opening in Windows:

```powershell
.\Setup.bat -RegisterProjectFiles
```

Project-local build:

```powershell
cd Projects\Platformer
cmake --preset editor
cmake --build --preset editor
```

Smoke tests:

```powershell
.\Builds\Editor\Projects\Platformer\Debug\PlatformerApp.exe --smoke-test
.\Projects\Platformer\Builds\Editor\Debug\PlatformerApp.exe --smoke-test
```
