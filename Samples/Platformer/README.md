# Platformer Sample

`PlatformerApp` is a small original side-scrolling platformer sample.

Controls:

- `A` / `Left`: move left
- `D` / `Right`: move right
- `W` / `Up` / `Space`: jump
- `R`: reset level
- `F11` / `Alt+Enter`: toggle fullscreen
- `Esc`: quit

The sample uses SDL for rendering and a simple tile-based AABB controller for platformer movement. It uses `Core/Math/Vector2D.h` for shared vector math.
Jumping uses a small input buffer and coyote time, so pressing jump slightly before landing or just after leaving an edge still works.

Build:

```powershell
cmake --build build-cmake-vcpkg --target PlatformerApp
```

Smoke test:

```powershell
.\build-cmake-vcpkg\Samples\Debug\PlatformerApp.exe --smoke-test
```
