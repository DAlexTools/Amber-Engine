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
- One-click Windows build through `Build.bat`.
- GoogleTest unit tests for runtime logging and physics behavior.

## Repository Layout

```text
Engine/
  Runtime/              Runtime modules and pure physics code
  Editor/               Editor-only ImGui modules
  ThirdParty/           Bundled headers/sources such as glm, imgui, lua and sol
  Content/              Reserved engine content
external/               Local dependency checkouts; vcpkg itself is ignored
Samples/                Runnable sample apps
Tests/                  GoogleTest unit tests
Content/                Game/project content used by GameEngineApp
Scripts/                Build helper scripts
CMakePresets.json       Visual Studio/CMake build presets
Build.bat               One-click local build entrypoint
Clean.bat               Removes generated local files before committing
SetupDependencies.bat   Bootstraps repo-local vcpkg
```

## Requirements

- Windows 10/11.
- Visual Studio 2022 with the Desktop development with C++ workload.
- CMake 3.16 or newer.
- PowerShell 5 or PowerShell 7.
- Git.
- vcpkg cloned into `external/vcpkg`.

The project uses manifest dependencies from `vcpkg.json`: SDL2, SDL2_image, SDL2_mixer without extra codec features, SDL2_ttf, SDL2_gfx, Lua 5.4.8 and GoogleTest.

## Quick Start

Bootstrap vcpkg once:

```powershell
.\SetupDependencies.bat
```

Then build the editor-enabled sample set:

```powershell
.\Build.bat
```

`Build.bat` also tries to run dependency setup automatically when `external/vcpkg` is missing. If you already have vcpkg elsewhere, set `VCPKG_ROOT` and the script will use the `full-vcpkg` preset instead of the repo-local preset.

The generated solution is:

```text
build-cmake-vcpkg/AmberEngine.sln
```

CMake predefined targets such as `ALL_BUILD` and `ZERO_CHECK` are still generated, but they are grouped under the `_CMake` solution folder. Engine, editor, samples and tests are grouped to mirror the repository layout.

## Build Commands

One-click helper:

```powershell
.\SetupDependencies.bat
.\Build.bat -Target Samples
.\Build.bat -Target Tests
.\Build.bat -Target Samples -RunSmoke
.\Build.bat -Mode NoEditor -Target Samples
.\Build.bat -Mode Core -Target Core
```

Use this for automation so the `.bat` file does not pause:

```powershell
$env:AMBER_BUILD_NO_PAUSE = "1"
.\Build.bat -Target Tests
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
.\build-cmake\Engine\Runtime\Physics\Debug\PhysicsCollisionFilteringCheck.exe
```

More detailed build notes are in [BUILDING.md](BUILDING.md).

## Running Samples

After `.\Build.bat`, run apps from the project root or from the build output folder:

```powershell
.\build-cmake-vcpkg\Samples\Debug\GameEngineApp.exe
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe
.\build-cmake-vcpkg\Samples\Debug\ContainerSandboxApp.exe
.\build-cmake-vcpkg\Samples\Debug\PlatformerApp.exe
```

Useful smoke checks:

```powershell
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe --smoke-test
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe --ui-smoke-test
.\build-cmake-vcpkg\Samples\Debug\PhysicsLabApp.exe --perf-test
.\build-cmake-vcpkg\Samples\Debug\PlatformerApp.exe --smoke-test
.\build-cmake-vcpkg\Samples\Debug\ContainerSandboxApp.exe --smoke-test
.\build-cmake-vcpkg\Samples\Debug\GameEngineApp.exe --smoke-test --level 1
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

- `build-cmake*/`
- `build/`, `Debug/`, `Release/`, `.vs/`
- `external/vcpkg/`
- `imgui.ini`
- logs and compiler outputs

The repository is intended to be rebuilt from source using `Build.bat` or the CMake presets.
Line endings and binary asset handling are declared in `.gitattributes`.

Before publishing or committing, clean generated local state:

```powershell
.\Clean.bat -RemoveVcpkg
```

If Visual Studio is open, it can keep `.vs` databases inside `build-cmake-vcpkg` locked. Close Visual Studio and run the command again.

## Acknowledgements

This project started from SDL/game-programming learning work inspired by Gustavo Pezzi's educational material and has since been reorganized into the AmberEngine runtime/editor/sample structure.
