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
- The current bridge creates runtime `AE::Scene::Object` instances through `ObjectFactory`, configures ECS entities, then renders visible `SpriteObject` instances as Platformer scene props.
- This is a visual integration pass only for now; editor props do not create gameplay collision or scripts yet.

Build:

```powershell
cmake --build Builds\Editor --target PlatformerApp
```

Smoke test:

```powershell
.\Builds\Editor\Samples\Debug\PlatformerApp.exe --smoke-test
```
