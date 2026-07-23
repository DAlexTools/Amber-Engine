# Physics Module

`Physics` is the renderer-free simulation module under `Engine/Runtime`. It owns rigid bodies, shapes, particles, cloth helpers, constraints, collision detection and contact generation.

Implementation files live in `src`. Public headers live in `include/Physics`.

Shared math lives in `Engine/Runtime/Core/Math`. The simulation world implementation lives in `Engine/Runtime/Classes/World.*` and is still compiled into the `Physics` target for now.

Include this module from other targets with paths like `Classes/World.h`, `Core/Math/Vector2D.h` or `Physics/Objects/Body.h`.

This module must not depend on SDL, Lua, ECS, gameplay systems or sample rendering.

Core smoke-test sources live in `tests`.
