# Samples Module

`Samples` owns runnable demo programs and SDL demo infrastructure. Full game-style projects live under `Projects`.

Current targets include:

- `RendererSDL`
- `AngryApp`
- `RagdollApp`
- `ChainApp`
- `SoftBodyApp`
- `FabricSimulationApp`
- `Platformer2App`
- `ContainerSandboxApp`
- `PhysicsLabApp`
- `GameEngineApp`

`GameEngineApp` starts from `GamesDemos/Game/main.cpp`.
`Platformer2App` starts from `GamesDemos/Platformer2/main.cpp`.
`ContainerSandboxApp` starts from `PhysicsDemos/ContainerSandbox/main.cpp`.
`PhysicsLabApp` starts from `PhysicsDemos/PhysicsLab/main.cpp`.

Game demos live in `GamesDemos`:

- `GamesDemos/Game`
- `GamesDemos/Platformer2`

Physics demos live in `PhysicsDemos`:

- `PhysicsDemos/Renderer/SDL`
- `PhysicsDemos/Common`
- `PhysicsDemos/AngryApp`
- `PhysicsDemos/Ragdoll`
- `PhysicsDemos/Chain`
- `PhysicsDemos/SoftBody`
- `PhysicsDemos/FabricSimulation`
- `PhysicsDemos/ContainerSandbox`
- `PhysicsDemos/PhysicsLab`
- `PhysicsDemos/Content`

`Platformer` was promoted to `Projects/Platformer` so it can behave like a real game project created from the editor.
`Platformer2` is a tilemap-backed side-scrolling game demo using the Kenney 1-Bit Platformer Pack assets, with a built-in map editor, rendered level viewport, visual tileset palette, scene outliner, ladders, spikes, lifts, animated sprites, enemies and shooting.
`ContainerSandbox` is a standalone granular physics sample with movable/rotatable cup, tray and funnel containers filled with small circle bodies.
`PhysicsLab` is an ImGui-backed sandbox that groups container, stack, collision filtering, moving platform, pinball and bridge/rope physics scenes behind one parameter UI.

Diagnostics:

- All sample apps show on-screen FPS and timing data.
- Physics-backed samples also show body/contact/pair counts, phase timings, solver islands and parallel job usage.
- `ContainerSandboxApp` and `PhysicsLabApp` use ImGui OutputLog controls.
- Legacy `PhysicsDemos` use the lightweight `LegacyDiagnosticsOverlay`: `F1` performance, `F2` controls, `F3` Output Log, `F4` clear log, `P` pause.
- `GameEngineApp` uses runtime ImGui diagnostics: `F1` performance and `F3` Output Log.

`2DPhysicEngine/Makefile` remains as a compatibility wrapper for old demo build commands.
