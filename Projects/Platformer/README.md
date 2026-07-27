# Platformer Project

`Platformer` is an in-repo game project laid out like a project created from `AmberEditor`: it has `Platformer.amberproject`, project-local `CMakeLists.txt`, `CMakePresets.json`, `Source/`, `Content/` and `Builds/`.

`PlatformerApp` is a small original side-scrolling platformer. Its gameplay lives in the reusable `PlatformerGameModule` library, while `PlatformerApp.exe` is the thin standalone launcher. The project can load its authored `startupScene` from `Platformer.amberproject`; enemies, solid platforms, coins, decorative sprites and physics playground objects can now come from the scene instead of being placed directly in C++.

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

Scripted enemy fallback:

```text
Content/Scripts/PlatformerEnemies.lua
```

Each enemy entry can define spawn position, patrol bounds, health, color, shooting parameters and an `on_update(enemy, player, dt)` function. C++ owns collision, damage, projectiles and rendering; Lua owns behavior tuning. Scene-authored `EnemySpawnObject` entries now take priority when the project scene is loaded; the Lua file remains the missing-scene fallback.

Physics playground:

- Static tile bodies mirror solid level platforms.
- Scene-authored `PhysicsBoxObject` and `PhysicsCircleObject` instances become dynamic crates and balls that can be pushed by the player or hit by projectiles.
- Scene-authored `PhysicsBridgeObject` and `PhysicsChainObject` instances build rope/chain segments with `JointConstraint`.
- Scene-authored `MovingPlatformObject` instances use kinematic static bodies updated before the physics step and are valid player landing surfaces.
- The old hardcoded physics playground remains as a fallback when the scene has no physics objects.

Editor scene bridge:

- `PlatformerApp` can open `Platformer.amberproject` directly, then resolves `startupScene` from the project descriptor and loads `Content/Scenes/PlatformerTest.amber.scene` when `SDL2_image` is available.
- `PlatformerApp` also accepts `--scene <path>` as an explicit override, so `AmberEditor` or tests can launch the game against a specific saved scene.
- `PlatformerGameModule` is the first PIE-oriented boundary: it implements `AE::IGameModule`, keeps the standalone launcher thin and exposes Platformer scene object registration for future embedded Editor play sessions.
- `PlatformerGameModulePlugin` is the dynamic module loaded by `AmberEditor` for PIE. It exports `AmberCreateGameModule` and `AmberDestroyGameModule`, then returns a `PlatformerGameModule` instance.
- In dynamic PIE today, the editor keeps scene object instantiation editor-side and passes the `SceneDocument` to `PlatformerGameModule::StartPlay`; plugin-owned ECS component registration waits for a shared runtime ABI.
- Platformer-specific scene object classes live in this sample (`PlatformerSceneObjects.h/.cpp`), not in the engine module.
- `PlayerSpawnObject`, `GoalObject`, `CoinObject`, `SolidPlatformObject`, `EnemySpawnObject`, `PhysicsBoxObject`, `PhysicsCircleObject`, `PhysicsBridgeObject`, `PhysicsChainObject` and `MovingPlatformObject` are registered into `ObjectFactory`, configured as ECS entities, then mapped into the sample runtime.
- `Content/Scenes/PlatformerTest.amber.scene` now owns the main level layout and the first gameplay/physics object pass as boxes/circles. The old tile-built layout and old hardcoded physics playground remain only as fallbacks when no scene data is available.
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
.\Builds\Editor\Projects\Platformer\Debug\PlatformerApp.exe .\Projects\Platformer\Platformer.amberproject --smoke-test
.\Projects\Platformer\Builds\Editor\Debug\PlatformerApp.exe .\Projects\Platformer\Platformer.amberproject --smoke-test
```
