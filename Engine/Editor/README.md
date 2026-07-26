# Engine Editor

Editor-only modules live here.

- `Shell` owns the standalone `AmberEditor` application shell: SDL/ImGui lifecycle, Unreal-style fixed panel layout, top toolbar, `Scene View`, `Asset Browser`, `Scene Outliner`, `Details` and Output Log placement. Its first internal services are `SceneDocument`, `SelectionService`, `EditorViewport`, `AssetRegistry` and `TextureCache`; `Scene View` supports picking, viewport pan/zoom, asset placement and a first move gizmo for selected scene objects. Scene save/load uses the runtime-neutral `AE::Scene` file format and stores an editable object class name such as `SpriteObject`.
- `OutputLog` owns the ImGui output log widget. It reads the runtime log bus and does not own gameplay/runtime logging state.
- `Diagnostics` owns reusable sample diagnostics panels while those samples are still SDL applications.

Do not put runtime systems here. Anything required by shipped samples or games belongs in `Engine/Runtime`.
