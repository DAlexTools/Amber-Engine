# Engine Editor

Editor-only modules live here.

- `Application` owns the standalone `AmberEditor` entry point, SDL/ImGui lifecycle, Project Browser, main menu, dock layout, toolbar and panel orchestration.
- `Viewport` owns `Scene View`: runtime preview rendering, editor camera, picking, drag/drop placement, transform gizmos and context-menu requests.
- `Scene` owns the editable `SceneDocument` wrapper around the runtime-neutral `AE::Scene` file format.
- `Actors` owns `ActorTypeRegistry` and project-provided actor type schemas.
- `Assets` owns editor asset scanning and texture preview caching on top of runtime asset/texture services.
- `Runtime` owns PIE orchestration and game-module loading through `EditorPlaySession` and `GameModuleResolver`.
- `Project` owns editor project generation helpers. Runtime project descriptors live under `Engine/Runtime/Project`.
- `Selection` owns editor selection state.
- Project Browser can create a `Blank C++ Game` in a separate folder with `.amberproject`, `CMakeLists.txt`, `CMakePresets.json`, source launcher/module stubs, `Content/Scenes/Startup.amber.scene`, `Config`, `Scripts` and `Tests`. It can run `cmake --preset <preset>` from the editor to generate the project-local Visual Studio solution, and it can open the current repo workspace as a legacy project. `AmberEditor.exe` also accepts a positional `.amberproject` path or `--project <path>` so Windows file associations can launch projects directly.
- `OutputLog` owns the ImGui output log widget. It reads the runtime log bus and does not own gameplay/runtime logging state.
- `Diagnostics` owns reusable sample diagnostics panels while those samples are still SDL applications.

Do not put runtime systems here. Anything required by shipped samples or games belongs in `Engine/Runtime`.
