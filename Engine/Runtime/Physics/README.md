# Physics Module

`Physics` is the renderer-free simulation module under `Engine/Runtime`. It owns rigid bodies, shapes, particles, cloth helpers, constraints, collision detection and contact generation.

The module is split by simulation concern:

- `Core`: constants and low-level compatibility macros.
- `Geometry`: reusable shape definitions.
- `Dynamics`: rigid bodies and force helpers.
- `Collision`: contact data and collision detection.
- `Constraints`: joints and penetration constraints.
- `Particles`: particles, points, sticks and cloth helpers.
- `Input`: input-side helpers used by legacy demos.
- `Serialization`: physics-local serialization helpers.
- `Utilities`: physics-local utility code.
- `Checks`: small smoke-check executables.

Shared math lives in `Engine/Runtime/Core/Math`. The simulation world implementation lives in `Engine/Runtime/Classes/World.*` and is still compiled into the `Physics` target for now.

Include this module from other targets with paths like `Classes/World.h`, `Core/Math/Vector2D.h`, `Physics/Dynamics/Body.h`, `Physics/Geometry/Shape.h` or `Physics/Collision/Contact.h`.

This module must not depend on SDL, Lua, ECS, gameplay systems or sample rendering.
