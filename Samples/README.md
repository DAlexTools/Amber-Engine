# Samples Module

`Samples` owns runnable programs and SDL demo infrastructure.

Current targets include:

- `RendererSDL`
- `AngryApp`
- `RagdollApp`
- `ChainApp`
- `SoftBodyApp`
- `FabricSimulationApp`
- `PlatformerApp`
- `ContainerSandboxApp`
- `PhysicsLabApp`
- `GameEngineApp`

`GameEngineApp` starts from `Game/main.cpp`.
`PlatformerApp` starts from `Platformer/main.cpp`.
`ContainerSandboxApp` starts from `ContainerSandbox/main.cpp`.
`PhysicsLabApp` starts from `PhysicsLab/main.cpp`.

Physics demos live in `PhysicsDemos`:

- `PhysicsDemos/Renderer/SDL`
- `PhysicsDemos/Common`
- `PhysicsDemos/AngryApp`
- `PhysicsDemos/Ragdoll`
- `PhysicsDemos/Chain`
- `PhysicsDemos/SoftBody`
- `PhysicsDemos/FabricSimulation`
- `PhysicsDemos/Content`

`ContainerSandbox` is a standalone granular physics sample with movable/rotatable cup, tray and funnel containers filled with small circle bodies.
`PhysicsLab` is an ImGui-backed sandbox that groups container, stack, collision filtering, moving platform, pinball and bridge/rope physics scenes behind one parameter UI.

Diagnostics:

- All sample apps show on-screen FPS and timing data.
- Physics-backed samples also show body/contact/pair counts, phase timings, solver islands and parallel job usage.
- `PlatformerApp`, `ContainerSandboxApp` and `PhysicsLabApp` use ImGui OutputLog controls.
- Legacy `PhysicsDemos` use the lightweight `LegacyDiagnosticsOverlay`: `F1` performance, `F2` controls, `F3` Output Log, `F4` clear log, `P` pause.
- `GameEngineApp` uses runtime ImGui diagnostics: `F1` performance and `F3` Output Log.

`2DPhysicEngine/Makefile` remains as a compatibility wrapper for old demo build commands.
