# Platformer Sample

`PlatformerApp` is a small original side-scrolling platformer sample. Enemy setup and behavior are loaded from Lua, player projectiles can damage enemies, enemies can shoot back, and the level contains a small `AE::Physics::World` playground with crates, balls, a rope bridge, a hanging chain and moving platforms.

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
- Platformer-specific scene object classes live in this sample (`PlatformerSceneObjects.h/.cpp`), not in the engine module.
- `PlayerSpawnObject`, `GoalObject`, `CoinObject` and `SolidPlatformObject` are registered into `ObjectFactory`, configured as ECS entities, then mapped into the sample's spawn point, finish trigger, collectible coins and extra solid platforms.
- Visible `SpriteObject` instances still render as decorative scene props from the editor scene.

Build:

```powershell
cmake --build Builds\Editor --target PlatformerApp
```

Smoke test:

```powershell
.\Builds\Editor\Samples\Debug\PlatformerApp.exe --smoke-test
```
