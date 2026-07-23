# Physics Lab

`PhysicsLabApp` is an ImGui-driven physics sandbox. It collects several focused runtime samples in one executable:

- Container: granular particles inside cup, tray and funnel shapes.
- Stack Stress: many boxes settling under gravity.
- Collision Filters: categories, masks and sensor contacts.
- Moving Platforms: manually animated static bodies carrying dynamic objects.
- Pinball: circles, sloped walls, bumpers and flippers.
- Bridge/Rope: joint constraints under load.

The UI exposes global physics controls including gravity, damping, solver iterations, broad phase on/off, broad phase grid cell size, sleeping thresholds, contacts and per-scene parameters.

Use `F11` or `Alt+Enter` to toggle fullscreen at runtime.

Build and run:

```powershell
cmake --build --preset physics-lab-local
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe
```

Smoke check:

```powershell
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe --smoke-test
```

UI smoke check:

```powershell
$env:SDL_VIDEODRIVER = "dummy"
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe --ui-smoke-test
```
