# Engine Module

`Engine` is the top-level engine container. Runtime code lives under `Runtime`, editor-only code will live under `Editor`, and bundled third-party dependencies live under `ThirdParty`.

- `Runtime` owns shipped-game code split into runtime modules: `Core/Math`, `Classes`, `Logging`, `EntityComponentSystem`, `Scene`, `Components`, `Systems`, `AssetManager`, `FileManager`, `Game`, `EventBus`, `Events`, `EnginePhysicsBridge` and `Physics`.
- `Runtime/Scene` owns runtime scene data and object authoring glue: `.amber.scene` load/save, `Object`, `SpriteObject`, `ObjectFactory` and lightweight scene ECS components.
- Game-specific object classes should be registered by the game/sample target that owns their behavior; for example `PlatformerApp` owns its spawn, goal, coin and solid-platform object classes under `Samples/GamesDemos/Platformer`.
- `Runtime/Physics` owns renderer-free physics simulation objects, collision detection, particles, constraints and its smoke tests.
- `Content` is reserved for built-in engine resources, not project/game assets.
- `ThirdParty` owns bundled dependencies such as `glm`, `imgui`, `lua` headers and `sol`.
- `Editor/OutputLog` owns the ImGui output log widget. Runtime and Physics loggers write into a shared runtime log bus that editor widgets can inspect.

The physics boundary is `EnginePhysicsBridge`: engine code consumes the `Physics` target through that bridge instead of reaching directly into app or sample targets.

The next migration step is to tighten include directories around explicit module boundaries and move stable public APIs under a versioned `Runtime/include/Engine` surface.
