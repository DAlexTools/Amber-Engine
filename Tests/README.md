# Unit Tests

GoogleTest-based unit tests live here.

Smoke executables still cover end-to-end app behavior. Unit tests should stay narrower and verify module-level behavior directly.

Current target:

- `PhysicsUnitTests`
- `EngineUnitTests`
- `EditorUnitTests` when editor targets are enabled

Build and run:

```powershell
cmake --preset full-local-vcpkg
cmake --build --preset unit-tests-local
ctest --preset unit-tests-local
```
