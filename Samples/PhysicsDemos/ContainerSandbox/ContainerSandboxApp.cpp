#include "ContainerSandboxApp.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

#include "Logging/Logger.h"
#include "Physics/Geometry/Shape.h"

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "imgui.h"
#endif

namespace
{
constexpr float FixedTimeStep = 1.0f / 120.0f;
constexpr float MaxFrameTime = 0.05f;
constexpr int MaxPhysicsStepsPerFrame = 4;
constexpr float MoveSpeed = 260.0f;
constexpr float RotateSpeed = 1.2f;
constexpr float MaxContainerAngle = 1.2f;
constexpr float ParticleRadius = 7.0f;
constexpr int MinParticleCount = 24;
constexpr int MaxParticleCount = 132;
constexpr int ParticleCountStep = 12;

SDL_Color BackgroundTop{30, 39, 48, 255};
SDL_Color BackgroundBottom{44, 54, 63, 255};
SDL_Color GridColor{64, 75, 84, 90};
SDL_Color WallFill{202, 211, 218, 255};
SDL_Color WallEdge{96, 107, 116, 255};
SDL_Color ParticleWarm{242, 183, 72, 255};
SDL_Color ParticleCool{78, 166, 196, 255};
SDL_Color ParticleDark{34, 53, 65, 255};
SDL_Color HudBack{18, 24, 30, 218};
SDL_Color HudAccent{116, 190, 210, 255};

int RoundToInt(float value)
{
	return static_cast<int>(std::round(value));
}

double ElapsedMs(Uint64 startCounter, Uint64 endCounter)
{
	return static_cast<double>(endCounter - startCounter) * 1000.0 /
		   static_cast<double>(SDL_GetPerformanceFrequency());
}

bool IsFullscreenToggleKey(const SDL_KeyboardEvent& keyEvent)
{
	const SDL_Keycode key = keyEvent.keysym.sym;
	const bool altPressed = (keyEvent.keysym.mod & KMOD_ALT) != 0;
	return key == SDLK_F11 || (altPressed && (key == SDLK_RETURN || key == SDLK_KP_ENTER));
}
} // namespace

ContainerSandboxApp::ContainerSandboxApp()
	: containerCenter(static_cast<float>(WindowWidth) * 0.5f, static_cast<float>(WindowHeight) * 0.54f)
{
	AE::Logger::SetConsoleEnabled(false);
	ResetSimulation(ContainerMode::Cup);
}

int ContainerSandboxApp::Run()
{
	if (!Initialize())
	{
		return 1;
	}

	std::cout << "Container Sandbox controls:" << std::endl;
	std::cout << "  Arrow keys / WASD: move container" << std::endl;
	std::cout << "  Q/E: rotate container" << std::endl;
	std::cout << "  1/2/3: cup/tray/funnel" << std::endl;
	std::cout << "  +/-: change particle count" << std::endl;
	std::cout << "  Space: pause, R: reset, Escape: quit" << std::endl;

	InputState input;
	Uint64 previousCounter = SDL_GetPerformanceCounter();
	float accumulator = 0.0f;
	running = true;

	while (running)
	{
		PollEvents(input);

		const Uint64 currentCounter = SDL_GetPerformanceCounter();
		const float elapsed = static_cast<float>(currentCounter - previousCounter) /
							  static_cast<float>(SDL_GetPerformanceFrequency());
		previousCounter = currentCounter;
		accumulator += ClampFloat(elapsed, 0.0f, MaxFrameTime);

		const Uint64 updateStart = SDL_GetPerformanceCounter();
		int physicsSteps = 0;
		while (accumulator >= FixedTimeStep && physicsSteps < MaxPhysicsStepsPerFrame)
		{
			Step(FixedTimeStep, input);
			accumulator -= FixedTimeStep;
			++physicsSteps;
		}
		if (physicsSteps == MaxPhysicsStepsPerFrame && accumulator >= FixedTimeStep)
		{
			accumulator = FixedTimeStep;
		}
		fixedStepsThisFrame = physicsSteps;
		lastUpdateMs = ElapsedMs(updateStart, SDL_GetPerformanceCounter());

		Render();
	}

	Shutdown();
	return 0;
}

#if SMOKE_TEST
bool ContainerSandboxApp::RunSmokeTest()
{
	ResetSimulation(ContainerMode::Cup);

	for (int frame = 0; frame < 260; ++frame)
	{
		Step(FixedTimeStep, InputState{});
	}

	const float settledAverageX = AverageParticleX();
	const int settledContacts = CountActiveContacts();

	InputState tiltRight;
	tiltRight.rotateRight = true;
	for (int frame = 0; frame < 120; ++frame)
	{
		Step(FixedTimeStep, tiltRight);
	}
	for (int frame = 0; frame < 260; ++frame)
	{
		Step(FixedTimeStep, InputState{});
	}

	const float tiltedAverageX = AverageParticleX();
	const float tiltedAngle = containerAngle;
	const bool particlesShifted = tiltedAverageX > settledAverageX + 8.0f;
	const bool angleUpdated = tiltedAngle > 0.25f;
	const bool contactsDetected = settledContacts > 0 || CountActiveContacts() > 0;
	const bool particleCountStable = static_cast<int>(particleBodies.size()) == particleCount;

	ResetSimulation(ContainerMode::Funnel);
	for (int frame = 0; frame < 120; ++frame)
	{
		Step(FixedTimeStep, InputState{});
	}
	const bool funnelShapeCreated = wallBodies.size() == 3;
	const bool funnelHasParticles = particleBodies.size() == static_cast<SizeT>(particleCount);

	const bool passed = particlesShifted && angleUpdated && contactsDetected && particleCountStable &&
						funnelShapeCreated && funnelHasParticles;

	if (!passed)
	{
		std::cerr << "Container sandbox smoke diagnostics: settledAverageX=" << settledAverageX
				  << " tiltedAverageX=" << tiltedAverageX
				  << " angle=" << tiltedAngle
				  << " settledContacts=" << settledContacts
				  << " currentContacts=" << CountActiveContacts()
				  << " particleBodies=" << particleBodies.size()
				  << " particleCount=" << particleCount
				  << " wallBodies=" << wallBodies.size()
				  << std::endl;
	}

	return passed;
}
#endif

bool ContainerSandboxApp::Initialize()
{
	SDL_SetMainReady();
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
		return false;
	}

	window = SDL_CreateWindow(
		"Container Sandbox",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WindowWidth,
		WindowHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
		Shutdown();
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
	if (!renderer)
	{
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE);
	}
	if (!renderer)
	{
		std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
		Shutdown();
		return false;
	}
	if (!CreateFrameTexture())
	{
		Shutdown();
		return false;
	}

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	diagnostics.Initialize(window, renderer, WindowWidth, WindowHeight);
#endif

	ResetSimulation(containerMode);
	return true;
}

void ContainerSandboxApp::Shutdown()
{
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	diagnostics.Shutdown();
#endif

	if (frameTexture)
	{
		SDL_DestroyTexture(frameTexture);
		frameTexture = nullptr;
	}
	if (renderer)
	{
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
	if (window)
	{
		SDL_DestroyWindow(window);
		window = nullptr;
	}
	SDL_Quit();
}

void ContainerSandboxApp::ToggleFullscreen()
{
	if (!window)
	{
		return;
	}

	fullscreen = !fullscreen;
	SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
	if (!fullscreen)
	{
		SDL_SetWindowSize(window, WindowWidth, WindowHeight);
		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
}

void ContainerSandboxApp::PollEvents(InputState& input)
{
	input = InputState{};

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
		diagnostics.ProcessEvent(event);
#endif

		if (event.type == SDL_QUIT)
		{
			running = false;
		}
		else if (event.type == SDL_KEYDOWN && !event.key.repeat)
		{
			if (IsFullscreenToggleKey(event.key))
			{
				ToggleFullscreen();
				continue;
			}

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
			if (event.key.keysym.sym == SDLK_F1)
			{
				diagnostics.ShowDiagnostics() = !diagnostics.ShowDiagnostics();
				continue;
			}
			if (event.key.keysym.sym == SDLK_F2)
			{
				diagnostics.ShowControls() = !diagnostics.ShowControls();
				continue;
			}
			if (event.key.keysym.sym == SDLK_F3)
			{
				diagnostics.ShowOutputLog() = !diagnostics.ShowOutputLog();
				continue;
			}
			if (diagnostics.WantsKeyboard())
			{
				continue;
			}
#endif

			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				running = false;
				break;
			case SDLK_SPACE:
				paused = !paused;
				UpdateWindowTitle();
				break;
			case SDLK_r:
				ResetSimulation(containerMode);
				break;
			case SDLK_1:
				ResetSimulation(ContainerMode::Cup);
				break;
			case SDLK_2:
				ResetSimulation(ContainerMode::Tray);
				break;
			case SDLK_3:
				ResetSimulation(ContainerMode::Funnel);
				break;
			case SDLK_TAB:
				ResetSimulation(NextMode(containerMode));
				break;
			case SDLK_BACKQUOTE:
				ResetSimulation(PreviousMode(containerMode));
				break;
			case SDLK_EQUALS:
			case SDLK_PLUS:
			case SDLK_KP_PLUS:
				SetParticleCount(particleCount + ParticleCountStep);
				break;
			case SDLK_MINUS:
			case SDLK_KP_MINUS:
				SetParticleCount(particleCount - ParticleCountStep);
				break;
			default:
				break;
			}
		}
	}

	const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	if (diagnostics.WantsKeyboard())
	{
		return;
	}
#endif

	input.moveLeft = keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_A];
	input.moveRight = keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_D];
	input.moveUp = keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W];
	input.moveDown = keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S];
	input.rotateLeft = keyboard[SDL_SCANCODE_Q];
	input.rotateRight = keyboard[SDL_SCANCODE_E];
}

void ContainerSandboxApp::Step(float dt, const InputState& input)
{
	const AE::Math::FVector2D previousCenter = containerCenter;
	const float previousAngle = containerAngle;

	if (input.moveLeft != input.moveRight)
	{
		containerCenter.X += (input.moveRight ? MoveSpeed : -MoveSpeed) * dt;
	}
	if (input.moveUp != input.moveDown)
	{
		containerCenter.Y += (input.moveDown ? MoveSpeed : -MoveSpeed) * dt;
	}
	if (input.rotateLeft != input.rotateRight)
	{
		containerAngle += (input.rotateRight ? RotateSpeed : -RotateSpeed) * dt;
		containerAngle = ClampFloat(containerAngle, -MaxContainerAngle, MaxContainerAngle);
		UpdateWindowTitle();
	}

	containerCenter.X = ClampFloat(containerCenter.X, 260.0f, static_cast<float>(WindowWidth) - 260.0f);
	containerCenter.Y = ClampFloat(containerCenter.Y, 245.0f, static_cast<float>(WindowHeight) - 110.0f);
	UpdateContainerBodies();

	const bool movedContainer = (containerCenter - previousCenter).MagnitudeSquared() > 0.01f ||
								std::abs(containerAngle - previousAngle) > 0.0001f;
	if (movedContainer && world)
	{
		world->WakeAllBodies();
	}

	if (!paused && world)
	{
		ApplyParticleDamping();
		world->Update(dt);
		RespawnEscapedParticles();
	}
}

void ContainerSandboxApp::ResetSimulation(ContainerMode mode)
{
	containerMode = mode;
	containerAngle = 0.0f;
	containerCenter = AE::Math::FVector2D(
		static_cast<float>(WindowWidth) * 0.5f,
		static_cast<float>(WindowHeight) * 0.54f);
	paused = false;

	world = std::make_unique<AE::Physics::World>(-9.8f);
	world->SetSolverIterations(solverIterations);
	world->SetBroadPhaseEnabled(broadPhaseEnabled);
	world->SetBroadPhaseCellSize(broadPhaseCellSize);
	world->SetSleepingEnabled(sleepingEnabled);
	world->SetParallelNarrowPhaseEnabled(parallelNarrowPhaseEnabled);
	world->SetParallelNarrowPhaseMinPairs(static_cast<SizeT>(parallelNarrowPhaseMinPairs));
	world->SetParallelSolverEnabled(parallelSolverEnabled);
	world->SetParallelSolverMinConstraints(static_cast<SizeT>(parallelSolverMinConstraints));
	wallSpecs = CreateWallSpecs(mode);
	wallBodies.clear();
	particleBodies.clear();

	BuildWalls();
	SpawnParticles();
	UpdateWindowTitle();
}

void ContainerSandboxApp::SetParticleCount(int count)
{
	particleCount = std::max(MinParticleCount, std::min(MaxParticleCount, count));
	ResetSimulation(containerMode);
}

void ContainerSandboxApp::BuildWalls()
{
	if (!world)
	{
		return;
	}

	for (const WallSpec& spec : wallSpecs)
	{
		AE::Physics::BoxShape shape(spec.width, spec.height);
		AE::Math::FVector2D position = ContainerToWorld(spec.localPosition);
		AE::Physics::Body* body = new AE::Physics::Body(shape, position.X, position.Y, 0.0f);
		body->friction = 0.02f;
		body->restitution = 0.08f;
		body->rotation = containerAngle + spec.localAngle;
		body->shape->UpdateVertices(body->rotation, body->position);
		world->AddBody(body);
		wallBodies.push_back(body);
	}
}

void ContainerSandboxApp::SpawnParticles()
{
	if (!world)
	{
		return;
	}

	for (int index = 0; index < particleCount; ++index)
	{
		const AE::Math::FVector2D position = ContainerToWorld(ParticleSpawnLocalPosition(static_cast<SizeT>(index)));
		AE::Physics::CircleShape shape(ParticleRadius);
		AE::Physics::Body* body = new AE::Physics::Body(shape, position.X, position.Y, 1.0f);
		body->friction = 0.02f;
		body->restitution = 0.12f;
		body->angularVelocity = ((index % 7) - 3) * 0.04f;
		world->AddBody(body);
		particleBodies.push_back(body);
	}
}

void ContainerSandboxApp::UpdateContainerBodies()
{
	for (SizeT index = 0; index < wallBodies.size() && index < wallSpecs.size(); ++index)
	{
		AE::Physics::Body* body = wallBodies[index];
		const WallSpec& spec = wallSpecs[index];
		body->position = ContainerToWorld(spec.localPosition);
		body->rotation = containerAngle + spec.localAngle;
		body->velocity = AE::Math::FVector2D::Zero;
		body->angularVelocity = 0.0f;
		body->shape->UpdateVertices(body->rotation, body->position);
	}
}

void ContainerSandboxApp::ApplyParticleDamping()
{
	for (AE::Physics::Body* body : particleBodies)
	{
		body->velocity *= 0.998f;
		body->angularVelocity *= 0.998f;

		constexpr float MaxSpeed = 1450.0f;
		const float speedSq = body->velocity.MagnitudeSquared();
		if (speedSq > MaxSpeed * MaxSpeed)
		{
			body->velocity = body->velocity.UnitVector() * MaxSpeed;
		}
	}
}

void ContainerSandboxApp::RespawnEscapedParticles()
{
	for (SizeT index = 0; index < particleBodies.size(); ++index)
	{
		const AE::Physics::Body* body = particleBodies[index];
		const bool outsideX = body->position.X < -240.0f || body->position.X > static_cast<float>(WindowWidth) + 240.0f;
		const bool outsideY = body->position.Y > static_cast<float>(WindowHeight) + 260.0f;
		if (outsideX || outsideY)
		{
			RespawnParticle(index);
		}
	}
}

void ContainerSandboxApp::RespawnParticle(SizeT index)
{
	if (index >= particleBodies.size())
	{
		return;
	}

	AE::Physics::Body* body = particleBodies[index];
	body->position = ContainerToWorld(ParticleSpawnLocalPosition(index));
	body->velocity = AE::Math::FVector2D(((static_cast<int>(index) % 5) - 2) * 18.0f, -40.0f);
	body->rotation = 0.0f;
	body->angularVelocity = 0.0f;
	body->shape->UpdateVertices(body->rotation, body->position);
}

std::vector<ContainerSandboxApp::WallSpec> ContainerSandboxApp::CreateWallSpecs(ContainerMode mode) const
{
	switch (mode)
	{
	case ContainerMode::Tray:
		return {
			WallSpec{AE::Math::FVector2D(0.0f, 122.0f), 620.0f, 28.0f, 0.0f},
			WallSpec{AE::Math::FVector2D(-310.0f, 44.0f), 28.0f, 174.0f, 0.0f},
			WallSpec{AE::Math::FVector2D(310.0f, 44.0f), 28.0f, 174.0f, 0.0f}};
	case ContainerMode::Funnel:
		return {
			WallSpec{AE::Math::FVector2D(0.0f, 152.0f), 132.0f, 28.0f, 0.0f},
			WallSpec{AE::Math::FVector2D(-146.0f, 26.0f), 28.0f, 336.0f, -0.48f},
			WallSpec{AE::Math::FVector2D(146.0f, 26.0f), 28.0f, 336.0f, 0.48f}};
	case ContainerMode::Cup:
	default:
		return {
			WallSpec{AE::Math::FVector2D(0.0f, 142.0f), 452.0f, 30.0f, 0.0f},
			WallSpec{AE::Math::FVector2D(-226.0f, 0.0f), 30.0f, 304.0f, 0.0f},
			WallSpec{AE::Math::FVector2D(226.0f, 0.0f), 30.0f, 304.0f, 0.0f}};
	}
}

AE::Math::FVector2D ContainerSandboxApp::ParticleSpawnLocalPosition(SizeT index) const
{
	const int columns = containerMode == ContainerMode::Tray ? 12 : 9;
	const float spacing = ParticleRadius * 2.35f;
	const int column = static_cast<int>(index % static_cast<SizeT>(columns));
	const int row = static_cast<int>(index / static_cast<SizeT>(columns));
	const float jitterX = static_cast<float>((static_cast<int>(index) * 17) % 5 - 2) * 1.35f;
	const float jitterY = static_cast<float>((static_cast<int>(index) * 11) % 5 - 2) * 1.1f;

	return AE::Math::FVector2D(
		(static_cast<float>(column) - (static_cast<float>(columns) - 1.0f) * 0.5f) * spacing + jitterX,
		94.0f - static_cast<float>(row) * spacing + jitterY);
}

AE::Math::FVector2D ContainerSandboxApp::ContainerToWorld(const AE::Math::FVector2D& localPosition) const
{
	return containerCenter + localPosition.Rotate(containerAngle);
}

float ContainerSandboxApp::AverageParticleX() const
{
	if (particleBodies.empty())
	{
		return 0.0f;
	}

	float sum = 0.0f;
	for (const AE::Physics::Body* body : particleBodies)
	{
		sum += body->position.X;
	}
	return sum / static_cast<float>(particleBodies.size());
}

int ContainerSandboxApp::CountActiveContacts() const
{
	if (!world)
	{
		return 0;
	}
	return static_cast<int>(world->GetContacts().size());
}

void ContainerSandboxApp::Render()
{
	if (!renderer || !frameTexture)
	{
		return;
	}

	const Uint64 renderStart = SDL_GetPerformanceCounter();

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	diagnostics.BeginFrame();
#endif

	BeginFrameTexture();
	DrawBackground();
	DrawContainer();
	DrawParticles();
	DrawHud();
	RenderDiagnostics();
	lastRenderMs = ElapsedMs(renderStart, SDL_GetPerformanceCounter());
	PresentFrameTexture();
}

bool ContainerSandboxApp::CreateFrameTexture()
{
	frameTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET,
		WindowWidth,
		WindowHeight);
	if (!frameTexture)
	{
		std::cerr << "SDL_CreateTexture frame target failed: " << SDL_GetError() << std::endl;
		return false;
	}

	SDL_SetTextureBlendMode(frameTexture, SDL_BLENDMODE_NONE);
#if SDL_VERSION_ATLEAST(2, 0, 12)
	SDL_SetTextureScaleMode(frameTexture, SDL_ScaleModeLinear);
#endif
	return true;
}

void ContainerSandboxApp::BeginFrameTexture()
{
	SDL_SetRenderTarget(renderer, frameTexture);
	SDL_RenderSetLogicalSize(renderer, 0, 0);
	SDL_RenderSetViewport(renderer, nullptr);
	SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void ContainerSandboxApp::PresentFrameTexture()
{
	SDL_SetRenderTarget(renderer, nullptr);
	SDL_RenderSetLogicalSize(renderer, 0, 0);
	SDL_RenderSetViewport(renderer, nullptr);
	SDL_RenderSetScale(renderer, 1.0f, 1.0f);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	const SDL_Rect destination = CalculateFrameViewport();
	SDL_RenderCopy(renderer, frameTexture, nullptr, &destination);
	SDL_RenderPresent(renderer);
}

SDL_Rect ContainerSandboxApp::CalculateFrameViewport() const
{
	int outputWidth = WindowWidth;
	int outputHeight = WindowHeight;
	SDL_GetRendererOutputSize(renderer, &outputWidth, &outputHeight);

	const float scale = std::min(
		static_cast<float>(outputWidth) / static_cast<float>(WindowWidth),
		static_cast<float>(outputHeight) / static_cast<float>(WindowHeight));
	const int width = static_cast<int>(std::round(static_cast<float>(WindowWidth) * scale));
	const int height = static_cast<int>(std::round(static_cast<float>(WindowHeight) * scale));
	return SDL_Rect{(outputWidth - width) / 2, (outputHeight - height) / 2, width, height};
}

void ContainerSandboxApp::RenderDiagnostics()
{
#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
	AE::Editor::SampleDiagnosticsData data;
	data.sampleName = "ContainerSandboxApp";
	data.paused = paused;
	data.fixedSteps = fixedStepsThisFrame;
	data.updateMs = lastUpdateMs;
	data.renderMs = lastRenderMs;
	data.statusText =
		std::string(ModeName(containerMode)) +
		" | angle " + std::to_string(static_cast<int>(std::round(containerAngle * 57.29578f))) +
		" deg | particles " + std::to_string(particleCount);

	if (world)
	{
		const AE::Physics::WorldStats& stats = world->GetLastStats();
		data.hasPhysicsStats = true;
		data.physics.bodies = world->GetBodies().size();
		data.physics.contacts = world->GetContacts().size();
		data.physics.constraints = world->GetConstraints().size();
		data.physics.bruteForcePairs = stats.bruteForcePairs;
		data.physics.broadPhasePairs = stats.broadPhasePairs;
		data.physics.narrowPhaseTests = stats.narrowPhaseTests;
		data.physics.solverIslandCount = stats.solverIslandCount;
		data.physics.largestSolverIslandBodyCount = stats.largestSolverIslandBodyCount;
		data.physics.largestSolverIslandConstraintCount = stats.largestSolverIslandConstraintCount;
		data.physics.parallelNarrowPhaseUsed = stats.parallelNarrowPhaseUsed;
		data.physics.parallelNarrowPhaseJobs = stats.parallelNarrowPhaseJobs;
		data.physics.parallelSolverUsed = stats.parallelSolverUsed;
		data.physics.parallelSolverJobs = stats.parallelSolverJobs;
		data.physics.physicsStepMs = stats.totalStepMs;
		data.physics.broadPhaseMs = stats.broadPhaseMs;
		data.physics.narrowPhaseMs = stats.narrowPhaseMs;
		data.physics.solverMs = stats.solverPhaseMs;
	}

	diagnostics.Draw(data, [this]()
					 {
        if (ImGui::Button(paused ? "Resume" : "Pause"))
        {
            paused = !paused;
            UpdateWindowTitle();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset"))
        {
            ResetSimulation(containerMode);
        }
        ImGui::SameLine();
        if (ImGui::Button(fullscreen ? "Windowed" : "Fullscreen"))
        {
            ToggleFullscreen();
        }

        int modeIndex = static_cast<int>(containerMode);
        const char* modes[] = {"Cup", "Tray", "Funnel"};
        if (ImGui::Combo("Container", &modeIndex, modes, IM_ARRAYSIZE(modes)))
        {
            ResetSimulation(static_cast<ContainerMode>(modeIndex));
        }

        int uiParticleCount = particleCount;
        if (ImGui::SliderInt("Particles", &uiParticleCount, MinParticleCount, MaxParticleCount))
        {
            SetParticleCount(uiParticleCount);
        }

        if (ImGui::SliderInt("Solver iterations", &solverIterations, 1, 20) && world)
        {
            world->SetSolverIterations(solverIterations);
        }
        if (ImGui::Checkbox("Broad phase", &broadPhaseEnabled) && world)
        {
            world->SetBroadPhaseEnabled(broadPhaseEnabled);
        }
        if (ImGui::SliderFloat("Grid cell", &broadPhaseCellSize, 16.0f, 256.0f, "%.0f") && world)
        {
            world->SetBroadPhaseCellSize(broadPhaseCellSize);
        }
        if (ImGui::Checkbox("Sleeping", &sleepingEnabled) && world)
        {
            world->SetSleepingEnabled(sleepingEnabled);
        }
        if (ImGui::Checkbox("Parallel narrow", &parallelNarrowPhaseEnabled) && world)
        {
            world->SetParallelNarrowPhaseEnabled(parallelNarrowPhaseEnabled);
        }
        if (ImGui::SliderInt("Narrow threshold", &parallelNarrowPhaseMinPairs, 32, 2048) && world)
        {
            world->SetParallelNarrowPhaseMinPairs(static_cast<SizeT>(parallelNarrowPhaseMinPairs));
        }
        if (ImGui::Checkbox("Parallel solver", &parallelSolverEnabled) && world)
        {
            world->SetParallelSolverEnabled(parallelSolverEnabled);
        }
        if (ImGui::SliderInt("Solver threshold", &parallelSolverMinConstraints, 1, 512) && world)
        {
            world->SetParallelSolverMinConstraints(static_cast<SizeT>(parallelSolverMinConstraints));
        }
        ImGui::Checkbox("Output Log", &diagnostics.ShowOutputLog()); });
	diagnostics.Render();
#endif
}

void ContainerSandboxApp::DrawBackground() const
{
	DrawScreenRect(0, 0, WindowWidth, WindowHeight / 2, BackgroundTop);
	DrawScreenRect(0, WindowHeight / 2, WindowWidth, WindowHeight / 2, BackgroundBottom);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, GridColor.r, GridColor.g, GridColor.b, GridColor.a);
	for (int x = 0; x < WindowWidth; x += 40)
	{
		SDL_RenderDrawLine(renderer, x, 0, x, WindowHeight);
	}
	for (int y = 0; y < WindowHeight; y += 40)
	{
		SDL_RenderDrawLine(renderer, 0, y, WindowWidth, y);
	}
}

void ContainerSandboxApp::DrawContainer() const
{
	for (const AE::Physics::Body* body : wallBodies)
	{
		const AE::Physics::PolygonShape* shape = static_cast<const AE::Physics::PolygonShape*>(body->shape);
		DrawFilledPolygon(shape->worldVertices, WallFill);
		DrawPolyline(shape->worldVertices, WallEdge, true);
	}
}

void ContainerSandboxApp::DrawParticles() const
{
	for (SizeT index = 0; index < particleBodies.size(); ++index)
	{
		const AE::Physics::Body* body = particleBodies[index];
		const SDL_Color color = index % 3 == 0 ? ParticleWarm : (index % 3 == 1 ? ParticleCool : ParticleDark);
		DrawFilledCircle(
			RoundToInt(body->position.X),
			RoundToInt(body->position.Y),
			RoundToInt(ParticleRadius),
			color);
	}
}

void ContainerSandboxApp::DrawHud() const
{
	DrawScreenRect(18, 18, 330, 42, HudBack);
	const int angleWidth = static_cast<int>(std::abs(containerAngle) / MaxContainerAngle * 118.0f);
	const int angleX = containerAngle >= 0.0f ? 184 : 184 - angleWidth;
	DrawScreenRect(44, 33, 86, 10, WallFill);
	DrawScreenRect(154, 33, 60, 10, HudAccent);
	DrawScreenRect(angleX, 33, angleWidth, 10, ParticleWarm);

	for (int index = 0; index < particleCount / ParticleCountStep; ++index)
	{
		const int x = 244 + (index % 9) * 8;
		const int y = 28 + (index / 9) * 11;
		DrawFilledCircle(x, y, 3, ParticleCool);
	}
}

void ContainerSandboxApp::DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	for (int y = -radius; y <= radius; ++y)
	{
		const int span = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
		SDL_RenderDrawLine(renderer, centerX - span, centerY + y, centerX + span, centerY + y);
	}
}

void ContainerSandboxApp::DrawFilledPolygon(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color) const
{
	if (vertices.size() < 3)
	{
		return;
	}

	float minY = vertices.front().Y;
	float maxY = vertices.front().Y;
	for (const AE::Math::FVector2D& vertex : vertices)
	{
		minY = std::min(minY, vertex.Y);
		maxY = std::max(maxY, vertex.Y);
	}

	const int startY = std::max(0, static_cast<int>(std::ceil(minY)));
	const int endY = std::min(WindowHeight - 1, static_cast<int>(std::floor(maxY)));

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	std::vector<float> intersections;
	intersections.reserve(vertices.size());

	for (int y = startY; y <= endY; ++y)
	{
		intersections.clear();
		const float scanY = static_cast<float>(y) + 0.5f;

		for (SizeT i = 0; i < vertices.size(); ++i)
		{
			const AE::Math::FVector2D& a = vertices[i];
			const AE::Math::FVector2D& b = vertices[(i + 1) % vertices.size()];
			if ((a.Y <= scanY && b.Y > scanY) || (b.Y <= scanY && a.Y > scanY))
			{
				const float t = (scanY - a.Y) / (b.Y - a.Y);
				intersections.push_back(a.X + t * (b.X - a.X));
			}
		}

		std::sort(intersections.begin(), intersections.end());
		for (SizeT i = 0; i + 1 < intersections.size(); i += 2)
		{
			const int x0 = std::max(0, static_cast<int>(std::ceil(intersections[i])));
			const int x1 = std::min(WindowWidth - 1, static_cast<int>(std::floor(intersections[i + 1])));
			SDL_RenderDrawLine(renderer, x0, y, x1, y);
		}
	}
}

void ContainerSandboxApp::DrawPolyline(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color, bool closed) const
{
	if (vertices.size() < 2)
	{
		return;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

	for (SizeT i = 0; i + 1 < vertices.size(); ++i)
	{
		SDL_RenderDrawLine(
			renderer,
			RoundToInt(vertices[i].X),
			RoundToInt(vertices[i].Y),
			RoundToInt(vertices[i + 1].X),
			RoundToInt(vertices[i + 1].Y));
	}

	if (closed)
	{
		SDL_RenderDrawLine(
			renderer,
			RoundToInt(vertices.back().X),
			RoundToInt(vertices.back().Y),
			RoundToInt(vertices.front().X),
			RoundToInt(vertices.front().Y));
	}
}

void ContainerSandboxApp::DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const
{
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	SDL_Rect rect{x, y, w, h};
	SDL_RenderFillRect(renderer, &rect);
}

void ContainerSandboxApp::UpdateWindowTitle()
{
	if (!window)
	{
		return;
	}

	std::ostringstream title;
	title << "Container Sandbox - " << ModeName(containerMode)
		  << " | angle " << static_cast<int>(std::round(containerAngle * 57.29578f))
		  << " deg | particles " << particleCount;
	if (paused)
	{
		title << " | paused";
	}
	SDL_SetWindowTitle(window, title.str().c_str());
}

const char* ContainerSandboxApp::ModeName(ContainerMode mode)
{
	switch (mode)
	{
	case ContainerMode::Cup:
		return "Cup";
	case ContainerMode::Tray:
		return "Tray";
	case ContainerMode::Funnel:
		return "Funnel";
	default:
		return "Unknown";
	}
}

ContainerSandboxApp::ContainerMode ContainerSandboxApp::NextMode(ContainerMode mode)
{
	switch (mode)
	{
	case ContainerMode::Cup:
		return ContainerMode::Tray;
	case ContainerMode::Tray:
		return ContainerMode::Funnel;
	case ContainerMode::Funnel:
	default:
		return ContainerMode::Cup;
	}
}

ContainerSandboxApp::ContainerMode ContainerSandboxApp::PreviousMode(ContainerMode mode)
{
	switch (mode)
	{
	case ContainerMode::Cup:
		return ContainerMode::Funnel;
	case ContainerMode::Tray:
		return ContainerMode::Cup;
	case ContainerMode::Funnel:
	default:
		return ContainerMode::Tray;
	}
}

float ContainerSandboxApp::ClampFloat(float value, float minValue, float maxValue)
{
	return std::max(minValue, std::min(maxValue, value));
}
