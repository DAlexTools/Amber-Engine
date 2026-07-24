# AmberEngine

AmberEngine is a work-in-progress 2D C++ engine built around SDL2, ImGui, a custom 2D physics runtime and a growing set of samples. The project is currently focused on engine/module boundaries, physics experiments, diagnostics, editor preparation and clean Visual Studio/CMake workflows.

## Features

- 2D runtime module with ECS, events, asset loading, Lua level loading and SDL rendering.
- Custom `AE::Physics` module with rigid bodies, shapes, collision detection, broad phase, solver iterations, sleeping, collision layers and physics stats.
- Editor-side ImGui utilities behind `WITH_EDITOR`, currently including OutputLog and sample diagnostics overlays.
- Samples:
  - `GameEngineApp` - Lua/content-driven SDL game sample.
  - `PhysicsLabApp` - ImGui physics sandbox with container, stack, filter, platform, pinball and bridge/rope scenes.
  - `ContainerSandboxApp` - granular container physics sample.
  - `PlatformerApp` - small Mario-like platformer sample.
  - Legacy physics demos: angry-birds style scene, ragdoll, chain, soft body and cloth.
- One-click Windows setup/build through `Setup.bat`.
- GoogleTest unit tests for runtime logging and physics behavior.

## Repository Layout

```text
Engine/
  Runtime/              Runtime modules and pure physics code
  Editor/               Editor-only ImGui modules
  ThirdParty/           Bundled headers/sources such as glm, imgui, lua and sol
  Content/              Reserved engine content
Dependencies/           Local dependency checkouts; vcpkg itself is ignored
Samples/                Runnable sample apps
Tests/                  GoogleTest unit tests
Content/                Game/project content used by GameEngineApp
Tools/Setup/            Internal setup/build helper scripts
CMakePresets.json       Visual Studio/CMake build presets
Setup.bat               One-click local setup/build entrypoint
```

## Requirements

- Windows 10/11.
- Visual Studio 2022 with the Desktop development with C++ workload.
- CMake 3.16 or newer.
- PowerShell 5 or PowerShell 7.
- Git.
- vcpkg cloned into `Dependencies/vcpkg`.

The project uses manifest dependencies from `vcpkg.json`: SDL2, SDL2_image, SDL2_mixer without extra codec features, SDL2_ttf, SDL2_gfx, Lua 5.4.8 and GoogleTest.

## Quick Start

Bootstrap dependencies only:

```powershell
.\Setup.bat -DepsOnly
```

Then build the editor-enabled sample set:

```powershell
.\Setup.bat
```

`Setup.bat` also tries to run dependency setup automatically when `Dependencies/vcpkg` is missing. If you already have vcpkg elsewhere, set `VCPKG_ROOT` and the script will use the `full-vcpkg` preset instead of the repo-local preset.

The generated solution is:

```text
Builds/Editor/AmberEngine.sln
```

CMake predefined targets such as `ALL_BUILD` and `ZERO_CHECK` are still generated, but they are grouped under the `_CMake` solution folder. Engine, editor, samples and tests are grouped to mirror the repository layout.

## Build Commands

One-click helper:

```powershell
.\Setup.bat -Target Samples
.\Setup.bat -Target Tests
.\Setup.bat -Target Samples -RunSmoke
.\Setup.bat -Mode NoEditor -Target Samples
.\Setup.bat -Mode Core -Target Core
```

Use this for automation so the `.bat` file does not pause:

```powershell
$env:AMBER_BUILD_NO_PAUSE = "1"
.\Setup.bat -Target Tests
```

Manual CMake build:

```powershell
cmake --preset full-local-vcpkg
cmake --build --preset samples-local
cmake --build --preset unit-tests-local
ctest --preset unit-tests-local
```

No-editor/shipping-like build:

```powershell
cmake --preset full-local-vcpkg-no-editor
cmake --build --preset samples-local-no-editor
ctest --preset unit-tests-local-no-editor
```

Core physics-only build without SDL/Lua:

```powershell
cmake --preset core
cmake --build --preset core-physics
.\Builds\Core\Engine\Runtime\Physics\Debug\PhysicsCollisionFilteringCheck.exe
```

More detailed build notes are in [BUILDING.md](BUILDING.md).

## Running Samples

After `.\Setup.bat`, run apps from the project root or from the build output folder:

```powershell
.\Builds\Editor\Samples\Debug\GameEngineApp.exe
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe
.\Builds\Editor\Samples\Debug\ContainerSandboxApp.exe
.\Builds\Editor\Samples\Debug\PlatformerApp.exe
```

Useful smoke checks:

```powershell
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe --smoke-test
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe --ui-smoke-test
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe --perf-test
.\Builds\Editor\Samples\Debug\PlatformerApp.exe --smoke-test
.\Builds\Editor\Samples\Debug\ContainerSandboxApp.exe --smoke-test
.\Builds\Editor\Samples\Debug\GameEngineApp.exe --smoke-test --level 1
```

Fullscreen controls:

- `F11` toggles fullscreen.
- `Alt+Enter` toggles fullscreen.
- `GameEngineApp.exe --fullscreen` starts in fullscreen desktop mode.
- `GameEngineApp.exe --windowed` starts windowed.

## Editor Build Flags

AmberEngine has two Unreal-style build gates:

```cpp
#include "Core/BuildConfig.h"

#if WITH_EDITOR
// Editor modules/tools/widgets.
#endif

#if WITH_EDITOR_ONLY_DATA
// Runtime metadata that can be stripped from shipping builds.
#endif
```

`WITH_EDITOR` controls editor code and modules. `WITH_EDITOR_ONLY_DATA` controls editor metadata stored in runtime types.

## GitHub Hygiene

Do not commit generated files or local dependency checkouts:

- `Builds/`
- `build/`, `Debug/`, `Release/`, `.vs/`
- `Dependencies/vcpkg/`
- `imgui.ini`
- logs and compiler outputs

The repository is intended to be rebuilt from source using `Setup.bat` or the CMake presets.
Line endings and binary asset handling are declared in `.gitattributes`.

Before publishing or committing, clean generated local state:

```powershell
.\Setup.bat -Clean -RemoveVcpkg
```

If Visual Studio is open, it can keep `.vs` databases inside `Builds` locked. Close Visual Studio and run the command again.

## Acknowledgements

This project started from SDL/game-programming learning work inspired by Gustavo Pezzi's educational material and has since been reorganized into the AmberEngine runtime/editor/sample structure.
