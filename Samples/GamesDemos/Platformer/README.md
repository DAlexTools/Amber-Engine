# Platformer Sample

`PlatformerApp` is a small original side-scrolling platformer sample. Enemy setup and behavior are loaded from Lua, player projectiles can damage enemies, and the level contains a small `AE::Physics::World` playground with crates, balls, a rope bridge, a hanging chain and a moving platform.

Controls:

- `A` / `Left`: move left
- `D` / `Right`: move right
- `W` / `Up` / `Space`: jump
- `J` / `Ctrl`: shoot
- `R`: reset level
- `F11` / `Alt+Enter`: toggle fullscreen
- `Esc`: quit

The sample uses SDL for rendering and a simple tile-based AABB controller for platformer movement. It uses `Core/Math/Vector2D.h` for shared vector math.
Jumping uses a small input buffer and coyote time, so pressing jump slightly before landing or just after leaving an edge still works.

Scripted enemies:

```text
Content/Scripts/PlatformerEnemies.lua
```

Each enemy entry can define spawn position, patrol bounds, health, color and an `on_update(enemy, player, dt)` function. C++ owns collision, damage and rendering; Lua owns behavior tuning.

Physics playground:

- Static tile bodies mirror solid level platforms.
- Dynamic crates and balls can be pushed by the player or hit by projectiles.
- Rope/chain segments use `JointConstraint`.
- The moving platform uses a kinematic static body updated before the physics step.

Build:

```powershell
cmake --build Builds\Editor --target PlatformerApp
```

Smoke test:

```powershell
.\Builds\Editor\Samples\Debug\PlatformerApp.exe --smoke-test
```
