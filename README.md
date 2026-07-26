# AmberEngine

AmberEngine is a work-in-progress 2D C++ engine built around SDL2, ImGui, a custom 2D physics runtime and a growing set of samples. The project is currently focused on engine/module boundaries, Unreal-style runtime organization, physics experiments, diagnostics, editor preparation and clean Visual Studio/CMake workflows.

## Features

- 2D runtime module with `AE::Engine`, ECS, events, asset loading, Lua level loading and SDL rendering.
- Game-specific startup and gameplay flow isolated in `GameModule`.
- Custom `AE::Physics` module with rigid bodies, shapes, collision detection, broad phase, solver iterations, sleeping, collision layers and physics stats.
- Editor-side ImGui tools behind `WITH_EDITOR`, currently including the standalone `AmberEditor` shell, OutputLog and sample diagnostics overlays.
- Game projects and demos:
  - `PlatformerApp` - Lua-scripted project platformer with double jump, player/enemy shooting and embedded physics bodies.
  - `GameEngineApp` - Lua/content-driven SDL game sample.
  - `Platformer2App` - tilemap-backed platformer with ladders, spikes, lifts and animated sprites.
- Physics demos:
  - `PhysicsLabApp` - ImGui physics sandbox with container, stack, filter, platform, pinball and bridge/rope scenes.
  - `ContainerSandboxApp` - granular container physics sample.
  - Legacy physics demos: angry-birds style scene, ragdoll, chain, soft body and cloth.
- One-click Windows setup/build through `Setup.bat`.
- GoogleTest unit tests for runtime logging and physics behavior.

## Repository Layout

```text
Engine/
  Runtime/              Runtime modules, AE::Engine, GameModule and pure physics code
  Editor/               Editor-only ImGui modules and AmberEditor shell
  ThirdParty/           Bundled headers/sources such as glm, imgui, lua and sol
  Content/              Reserved engine content
Dependencies/           Local dependency checkouts; vcpkg itself is ignored
Samples/                Runnable sample apps
  GamesDemos/           Game-facing demos: GameEngineApp and Platformer2App
  PhysicsDemos/         Physics demos, shared physics sample renderer and content
Projects/               In-repo game projects with .amberproject files
  Platformer/           Platformer project, source, content and local build presets
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
.\Setup.bat -Target Editor
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

## Running The Editor

Build and run the standalone editor shell:

```powershell
.\Setup.bat -Target Editor
.\Builds\Editor\Engine\Editor\Shell\Debug\AmberEditor.exe
```

Open a project directly, the same way a `.uproject` is passed to Unreal Editor:

```powershell
.\Builds\Editor\Engine\Editor\Shell\Debug\AmberEditor.exe .\Projects\Platformer\Platformer.amberproject
.\Builds\Editor\Engine\Editor\Shell\Debug\AmberEditor.exe --project .\Projects\Platformer\Platformer.amberproject
```

To make double-clicking `.amberproject` files open `AmberEditor` for the current Windows user:

```powershell
.\Setup.bat -RegisterProjectFiles
```

The first editor milestone opens a dock-like Unreal-style layout with a top toolbar, `Scene View`, `Asset Browser`, `Scene Outliner`, `Details` and `Output Log`. `Asset Browser` exposes the active project's `Content` folder and engine content from `AmberEngine/Engine/Content`; texture assets show previews and can be dragged into `Scene View`, and standard `BoxObject` / `CircleObject` primitives can be created from the `Add` menu. The in-repo `Platformer` project lives under `Projects/Platformer` and is the current PIE test project. Real engine scene rendering, full asset import and scene serialization are tracked in [ENGINE_ROADMAP.md](ENGINE_ROADMAP.md).

## Running Projects And Samples

After `.\Setup.bat`, run apps from the project root or from the build output folder:

```powershell
.\Builds\Editor\Samples\Debug\GameEngineApp.exe
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe
.\Builds\Editor\Samples\Debug\ContainerSandboxApp.exe
.\Builds\Editor\Projects\Platformer\Debug\PlatformerApp.exe
.\Builds\Editor\Samples\Debug\Platformer2App.exe
```

Sample source layout:

```text
Samples/GamesDemos/Game/                 GameEngineApp entrypoint
Samples/GamesDemos/Platformer2/          Platformer2App source and Kenney tile content
Projects/Platformer/                     Platformer game project, module, launcher and content
Samples/PhysicsDemos/PhysicsLab/         PhysicsLabApp source
Samples/PhysicsDemos/ContainerSandbox/   ContainerSandboxApp source
Samples/PhysicsDemos/AngryApp/           Legacy angry-birds style physics demo
Samples/PhysicsDemos/Ragdoll/            Legacy ragdoll demo
Samples/PhysicsDemos/Chain/              Legacy chain demo
Samples/PhysicsDemos/SoftBody/           Legacy soft-body demo
Samples/PhysicsDemos/FabricSimulation/   Legacy cloth/fabric demo
Samples/PhysicsDemos/Common/             Shared legacy demo utilities
Samples/PhysicsDemos/Renderer/SDL/       Shared SDL renderer for legacy physics demos
Samples/PhysicsDemos/Content/            Shared physics demo assets
```

Useful smoke checks:

```powershell
.\Builds\Editor\Engine\Editor\Shell\Debug\AmberEditor.exe --smoke-test
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe --smoke-test
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe --ui-smoke-test
.\Builds\Editor\Samples\Debug\PhysicsLabApp.exe --perf-test
.\Builds\Editor\Projects\Platformer\Debug\PlatformerApp.exe --smoke-test
.\Builds\Editor\Samples\Debug\ContainerSandboxApp.exe --smoke-test
.\Builds\Editor\Samples\Debug\GameEngineApp.exe --smoke-test --level 1
```

Fullscreen controls:

- `F11` toggles fullscreen.
- `Alt+Enter` toggles fullscreen.
- `GameEngineApp.exe --fullscreen` starts in fullscreen desktop mode.
- `GameEngineApp.exe --windowed` starts windowed.

## Build Flags

AmberEngine uses Unreal-style compile gates through `Core/BuildConfig.h`:

```cpp
#include "Core/BuildConfig.h"

#if WITH_EDITOR
// Editor modules/tools/widgets.
#endif

#if WITH_EDITOR_ONLY_DATA
// Runtime metadata that can be stripped from shipping builds.
#endif

#if SMOKE_TEST
// Smoke-test-only code paths and executable modes.
#endif

#if C_UNIT_TEST
// GoogleTest-only code.
#endif
```

`WITH_EDITOR` controls editor code and modules. `WITH_EDITOR_ONLY_DATA` controls editor metadata stored in runtime types.
`SMOKE_TEST` is enabled by the sample and smoke-check targets that expose smoke modes.
`C_UNIT_TEST` is enabled by the GoogleTest targets.
All four macros default to `0` when a target does not define them.

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
