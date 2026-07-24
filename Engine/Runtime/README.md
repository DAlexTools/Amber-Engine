# Engine Runtime

`Runtime` owns code that must exist in shipped games: engine lifecycle, entity/component systems, gameplay-facing systems, asset and level loading, the physics bridge, shared logging, shared math and the pure physics module.

Current module layout:

- `Core/Math` owns shared vector/matrix/math helpers.
- `Classes` owns shared runtime classes such as `AE::Engine` and `AE::Physics::World`.
- `Logging` owns the runtime logger, physics logger adapter and shared log bus.
- `EntityComponentSystem` owns the ECS implementation.
- `Components`, `Systems`, `AssetManager`, `FileManager`, `Game`, `EventBus`, `Events` and `EnginePhysicsBridge` own their matching runtime areas. `Game` contains game-facing modules such as `GameModule`; engine bootstrap belongs to `Classes/Engine`.
- `Physics` contains the renderer-free simulation library.

Future public runtime headers should move into `include/Engine` once the API boundary is stable.
