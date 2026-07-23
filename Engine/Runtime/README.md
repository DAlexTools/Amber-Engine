# Engine Runtime

`Runtime` owns code that must exist in shipped games: entity/component systems, gameplay-facing systems, asset and level loading, the game loop, the physics bridge, shared logging, shared math and the pure physics module.

Current module layout:

- `Core/Math` owns shared vector/matrix/math helpers.
- `Classes` owns shared runtime classes such as `AE::Physics::World`.
- `Logging` owns the runtime logger, physics logger adapter and shared log bus.
- `EntityComponentSystem` owns the ECS implementation.
- `Components`, `Systems`, `AssetManager`, `FileManager`, `Game`, `EventBus`, `Events` and `EnginePhysicsBridge` own their matching runtime areas.
- `Physics` contains the renderer-free simulation library.

Future public runtime headers should move into `include/Engine` once the API boundary is stable.
