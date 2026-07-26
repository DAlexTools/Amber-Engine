# Projects

`Projects` owns in-repo game projects that should behave like games created from `AmberEditor`.

Each project should keep its own descriptor, source, content, build presets and generated build output:

```text
Projects/MyGame/
  MyGame.amberproject
  CMakeLists.txt
  CMakePresets.json
  Source/
  Content/
  Config/
  Scripts/
  Tests/
  Builds/
```

`Builds/` is project-local generated output and should stay ignored.

Current in-repo projects:

- `Platformer`
