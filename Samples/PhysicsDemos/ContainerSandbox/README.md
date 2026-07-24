# Container Sandbox

`ContainerSandboxApp` is a granular physics sample: many circle bodies settle inside a movable container built from static box bodies. It stress-tests circle-vs-circle and circle-vs-box contacts with a shape that can be moved and tilted at runtime.

Controls:

- Arrow keys / `WASD`: move the container.
- `Q` / `E`: rotate the container.
- `1`, `2`, `3`: switch between cup, tray and funnel shapes.
- `+` / `-`: change particle count.
- `Space`: pause.
- `R`: reset.
- `F11` / `Alt+Enter`: toggle fullscreen.
- `Escape`: quit.

Build and run:

```powershell
cmake --build --preset container-sandbox-local
.\Builds\Editor\Samples\Debug\ContainerSandboxApp.exe
```

Smoke check:

```powershell
.\Builds\Editor\Samples\Debug\ContainerSandboxApp.exe --smoke-test
```
