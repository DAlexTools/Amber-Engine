# Engine Editor

Editor-only modules live here.

- `Shell` owns the standalone `AmberEditor` application shell: SDL/ImGui lifecycle, Unreal-style fixed panel layout, top toolbar, `Scene View`, `Asset Browser`, `Scene Outliner`, `Details` and Output Log placement. Its first internal services are `SceneDocument`, `SelectionService` and `EditorViewport`.
- `OutputLog` owns the ImGui output log widget. It reads the runtime log bus and does not own gameplay/runtime logging state.
- `Diagnostics` owns reusable sample diagnostics panels while those samples are still SDL applications.

Do not put runtime systems here. Anything required by shipped samples or games belongs in `Engine/Runtime`.
