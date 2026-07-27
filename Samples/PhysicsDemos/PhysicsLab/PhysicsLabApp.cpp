#include "PhysicsLabApp.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

#include "Physics/Constraint.h"
#include "Logging/Logger.h"
#include "Physics/Objects/Shape.h"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"

namespace
{
    constexpr float FixedTimeStep = 1.0f / 120.0f;
    constexpr float MaxFrameTime = 0.05f;
    constexpr int MaxPhysicsStepsPerFrame = 4;
    constexpr float MoveSpeed = 260.0f;
    constexpr float RotateSpeed = 1.2f;
    constexpr float MaxContainerAngle = 1.2f;
    constexpr float DegreesPerRadian = 57.2957795f;

    constexpr std::uint32_t CategoryDefault = 0x00000001u;
    constexpr std::uint32_t CategoryRed = 0x00000002u;
    constexpr std::uint32_t CategoryBlue = 0x00000004u;
    constexpr std::uint32_t CategorySensor = 0x00000008u;

    SDL_Color BackgroundTop{28, 34, 40, 255};
    SDL_Color BackgroundBottom{42, 49, 56, 255};
    SDL_Color GridColor{73, 83, 91, 75};
    SDL_Color WallFill{205, 211, 216, 255};
    SDL_Color WallEdge{90, 100, 108, 255};
    SDL_Color Warm{241, 178, 70, 255};
    SDL_Color Cool{82, 170, 198, 255};
    SDL_Color Dark{34, 48, 58, 255};
    SDL_Color Red{220, 76, 75, 255};
    SDL_Color Blue{75, 138, 220, 255};
    SDL_Color Green{104, 184, 116, 255};
    SDL_Color Violet{151, 110, 204, 255};
    SDL_Color SensorFill{240, 100, 160, 70};
    SDL_Color SensorEdge{240, 100, 160, 190};
    SDL_Color ContactColor{255, 245, 145, 235};
    SDL_Color JointColor{135, 215, 195, 220};

    int RoundToInt(float value)
    {
        return static_cast<int>(std::round(value));
    }

    double ElapsedMs(Uint64 startCounter, Uint64 endCounter)
    {
        return static_cast<double>(endCounter - startCounter) * 1000.0 /
            static_cast<double>(SDL_GetPerformanceFrequency());
    }

    SDL_Color WithAlpha(SDL_Color color, Uint8 alpha)
    {
        color.a = alpha;
        return color;
    }

    bool IsFullscreenToggleKey(const SDL_KeyboardEvent& keyEvent)
    {
        const SDL_Keycode key = keyEvent.keysym.sym;
        const bool altPressed = (keyEvent.keysym.mod & KMOD_ALT) != 0;
        return key == SDLK_F11 || (altPressed && (key == SDLK_RETURN || key == SDLK_KP_ENTER));
    }
}

PhysicsLabApp::PhysicsLabApp() :
    containerCenter(static_cast<float>(WindowWidth) * 0.5f, static_cast<float>(WindowHeight) * 0.54f)
{
    AE::Logger::SetConsoleEnabled(false);
    ResetScene(scene);
}

int PhysicsLabApp::Run()
{
    if (!Initialize())
    {
        return 1;
    }

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
bool PhysicsLabApp::RunSmokeTest()
{
    bool passed = true;

    ResetScene(Scene::Container);
    for (int frame = 0; frame < 260; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    const float settledContainerX = containerParticles.empty() ? 0.0f : containerParticles.front()->position.X;
    InputState tiltRight;
    tiltRight.rotateRight = true;
    for (int frame = 0; frame < 100; ++frame)
    {
        Step(FixedTimeStep, tiltRight);
    }
    passed = passed && containerAngle > 0.2f && !containerParticles.empty() &&
        std::abs(containerParticles.front()->position.X - settledContainerX) > 0.5f;

    ResetScene(Scene::StackStress);
    const int stackBodies = BodyCount();
    for (int frame = 0; frame < 180; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    passed = passed && stackBodies > 20 && ContactCount() > 0;

    ResetScene(Scene::CollisionFilters);
    for (int frame = 0; frame < 160; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    passed = passed && BodyCount() >= 10;

    ResetScene(Scene::MovingPlatforms);
    const float platformStartX = movingPlatforms.empty() ? 0.0f : movingPlatforms.front().body->position.X;
    for (int frame = 0; frame < 160; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    passed = passed && !movingPlatforms.empty() &&
        std::abs(movingPlatforms.front().body->position.X - platformStartX) > 12.0f;

    ResetScene(Scene::Pinball);
    launchRequested = true;
    for (int frame = 0; frame < 120; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    passed = passed && !pinballBalls.empty() && pinballBalls.front()->velocity.MagnitudeSquared() > 100.0f;

    ResetScene(Scene::BridgeRope);
    for (int frame = 0; frame < 180; ++frame)
    {
        Step(FixedTimeStep, InputState{});
    }
    passed = passed && ConstraintCount() >= bridgeSegments && BodyCount() >= bridgeSegments;

    if (!passed)
    {
        std::cerr << "Physics lab smoke diagnostics: scene=" << SceneName(scene)
                  << " bodies=" << BodyCount()
                  << " contacts=" << ContactCount()
                  << " constraints=" << ConstraintCount()
                  << std::endl;
    }

    return passed;
}
#endif

#if SMOKE_TEST
bool PhysicsLabApp::RunUiSmokeTest()
{
    if (!Initialize())
    {
        return false;
    }

    InputState input;
    for (int frame = 0; frame < 12; ++frame)
    {
        Step(FixedTimeStep, input);
        Render();
    }

    Shutdown();
    return true;
}
#endif

#if SMOKE_TEST
bool PhysicsLabApp::RunPerfTest()
{
    struct PerfTotals
    {
        double wallMs = 0.0;
        double physicsMs = 0.0;
        double broadPhaseMs = 0.0;
        double narrowPhaseMs = 0.0;
        double solverMs = 0.0;
        double velocityMs = 0.0;
        std::size_t contacts = 0u;
        std::size_t constraints = 0u;
    };

    const auto runScene = [this](Scene perfScene, const char* label, int warmupSteps, int measuredSteps)
    {
        ResetScene(perfScene);

        PerfTotals totals;
        InputState input{};
        for (int step = 0; step < warmupSteps; ++step)
        {
            Step(FixedTimeStep, input);
        }

        for (int step = 0; step < measuredSteps; ++step)
        {
            const auto start = std::chrono::steady_clock::now();
            Step(FixedTimeStep, input);
            const auto end = std::chrono::steady_clock::now();
            const AE::Physics::WorldStats stats = world ? world->GetLastStats() : AE::Physics::WorldStats{};

            totals.wallMs += std::chrono::duration<double, std::milli>(end - start).count();
            totals.physicsMs += stats.totalStepMs;
            totals.broadPhaseMs += stats.broadPhaseMs;
            totals.narrowPhaseMs += stats.narrowPhaseMs;
            totals.solverMs += stats.solverPhaseMs;
            totals.velocityMs += stats.velocityPhaseMs;
            totals.contacts += stats.contactCount;
            totals.constraints += stats.solverConstraintCount;
        }

        const double divisor = static_cast<double>(std::max(1, measuredSteps));
        std::cout
            << "PhysicsLab perf " << label
            << ": steps=" << measuredSteps
            << " bodies=" << BodyCount()
            << " solverIterations=" << solverIterations
            << " avgWallMs=" << totals.wallMs / divisor
            << " avgPhysicsMs=" << totals.physicsMs / divisor
            << " avgBroadMs=" << totals.broadPhaseMs / divisor
            << " avgNarrowMs=" << totals.narrowPhaseMs / divisor
            << " avgSolverMs=" << totals.solverMs / divisor
            << " avgVelocityMs=" << totals.velocityMs / divisor
            << " avgContacts=" << static_cast<double>(totals.contacts) / divisor
            << " avgConstraints=" << static_cast<double>(totals.constraints) / divisor
            << std::endl;
    };

    solverIterations = 6;
    particleCount = 132;
    containerShape = ContainerShape::Cup;
    runScene(Scene::Container, "Container132", 120, 360);

    stackRows = 12;
    stackColumns = 14;
    runScene(Scene::StackStress, "Stack12x14", 120, 360);
    return true;
}
#endif

bool PhysicsLabApp::Initialize()
{
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(
        "Physics Lab",
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

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);
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

    InitializeImGui();
    ResetScene(scene);
    return true;
}

void PhysicsLabApp::Shutdown()
{
    ShutdownImGui();

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

void PhysicsLabApp::ToggleFullscreen()
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

    if (imguiReady)
    {
        ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(WindowWidth), static_cast<float>(WindowHeight));
    }
}

void PhysicsLabApp::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForD3D(window);
    ImGuiSDL::Initialize(renderer, WindowWidth, WindowHeight);
    imguiReady = true;
}

void PhysicsLabApp::ShutdownImGui()
{
    if (!imguiReady)
    {
        return;
    }

    ImGuiSDL::Deinitialize();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    imguiReady = false;
}

void PhysicsLabApp::PollEvents(InputState& input)
{
    input = InputState{};

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (imguiReady)
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }

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

            const bool uiWantsKeyboard = imguiReady && ImGui::GetIO().WantCaptureKeyboard;
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                running = false;
            }
            else if (!uiWantsKeyboard)
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_r:
                        ResetScene(scene);
                        break;
                    case SDLK_SPACE:
                        if (scene == Scene::Pinball)
                        {
                            launchRequested = true;
                        }
                        else
                        {
                            paused = !paused;
                            UpdateWindowTitle();
                        }
                        break;
                    case SDLK_1:
                        ResetScene(Scene::Container);
                        break;
                    case SDLK_2:
                        ResetScene(Scene::StackStress);
                        break;
                    case SDLK_3:
                        ResetScene(Scene::CollisionFilters);
                        break;
                    case SDLK_4:
                        ResetScene(Scene::MovingPlatforms);
                        break;
                    case SDLK_5:
                        ResetScene(Scene::Pinball);
                        break;
                    case SDLK_6:
                        ResetScene(Scene::BridgeRope);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    const bool uiWantsKeyboard = imguiReady && ImGui::GetIO().WantCaptureKeyboard;
    if (uiWantsKeyboard)
    {
        return;
    }

    const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
    input.moveLeft = keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_A];
    input.moveRight = keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_D];
    input.moveUp = keyboard[SDL_SCANCODE_UP] || keyboard[SDL_SCANCODE_W];
    input.moveDown = keyboard[SDL_SCANCODE_DOWN] || keyboard[SDL_SCANCODE_S];
    input.rotateLeft = keyboard[SDL_SCANCODE_Q];
    input.rotateRight = keyboard[SDL_SCANCODE_E];
    input.leftFlipper = keyboard[SDL_SCANCODE_Z];
    input.rightFlipper = keyboard[SDL_SCANCODE_SLASH] || keyboard[SDL_SCANCODE_X];
}

void PhysicsLabApp::Step(float dt, const InputState& input)
{
    sceneTime += dt;

    switch (scene)
    {
        case Scene::Container:
            StepContainer(dt, input);
            break;
        case Scene::CollisionFilters:
            StepCollisionFilters();
            break;
        case Scene::MovingPlatforms:
            StepMovingPlatforms();
            break;
        case Scene::Pinball:
            StepPinball(dt, input);
            break;
        case Scene::StackStress:
        case Scene::BridgeRope:
        default:
            break;
    }

    if (!paused && world)
    {
        ApplyDamping();
        world->Update(dt);
        RespawnLostBodies();
    }
}

void PhysicsLabApp::UpdateWindowTitle()
{
    if (!window)
    {
        return;
    }

    std::ostringstream title;
    title << "Physics Lab - " << SceneName(scene);
    if (paused)
    {
        title << " | paused";
    }
    SDL_SetWindowTitle(window, title.str().c_str());
}

void PhysicsLabApp::ResetScene(Scene newScene)
{
    scene = newScene;
    paused = false;
    sceneTime = 0.0f;
    containerAngle = 0.0f;
    containerCenter = AE::Physics::FVector2D(
        static_cast<float>(WindowWidth) * 0.5f,
        static_cast<float>(WindowHeight) * 0.54f);

    world = std::make_unique<AE::Physics::World>(-gravity);
    world->SetBroadPhaseEnabled(broadPhaseEnabled);
    world->SetBroadPhaseCellSize(broadPhaseCellSize);
    world->SetSleepingEnabled(sleepingEnabled);
    world->SetSleepThresholds(sleepLinearThreshold, sleepAngularThreshold, sleepTimeThreshold);
    world->SetParallelNarrowPhaseEnabled(parallelNarrowPhaseEnabled);
    world->SetParallelNarrowPhaseMinPairs(static_cast<std::size_t>(parallelNarrowPhaseMinPairs));
    world->SetParallelSolverEnabled(parallelSolverEnabled);
    world->SetParallelSolverMinConstraints(static_cast<std::size_t>(parallelSolverMinConstraints));
    world->SetSolverIterations(solverIterations);
    visuals.clear();
    containerWalls.clear();
    containerParticles.clear();
    containerWallSpecs.clear();
    movingPlatforms.clear();
    pinballBalls.clear();
    flippers.clear();
    launchRequested = false;

    BuildScene();
    UpdateWindowTitle();
}

void PhysicsLabApp::BuildScene()
{
    switch (scene)
    {
        case Scene::Container:
            BuildContainerScene();
            break;
        case Scene::StackStress:
            BuildStackStressScene();
            break;
        case Scene::CollisionFilters:
            BuildCollisionFiltersScene();
            break;
        case Scene::MovingPlatforms:
            BuildMovingPlatformsScene();
            break;
        case Scene::Pinball:
            BuildPinballScene();
            break;
        case Scene::BridgeRope:
            BuildBridgeRopeScene();
            break;
    }
}

void PhysicsLabApp::BuildContainerScene()
{
    containerWallSpecs = CreateContainerWallSpecs(containerShape);
    for (const WallSpec& wall : containerWallSpecs)
    {
        AE::Physics::Body* body = AddBox(
            ContainerToWorld(wall.localPosition),
            wall.width,
            wall.height,
            0.0f,
            WallFill,
            WallEdge,
            containerAngle + wall.localAngle);
        body->friction = 0.02f;
        body->restitution = 0.08f;
        containerWalls.push_back(body);
    }

    for (int index = 0; index < particleCount; ++index)
    {
        const AE::Physics::FVector2D position = ContainerToWorld(ContainerParticleLocalPosition(static_cast<std::size_t>(index)));
        const SDL_Color fill = index % 3 == 0 ? Warm : (index % 3 == 1 ? Cool : Dark);
        AE::Physics::Body* particle = AddCircle(position, particleRadius, 1.0f, fill, WithAlpha(fill, 220));
        particle->friction = particleFriction;
        particle->restitution = particleRestitution;
        containerParticles.push_back(particle);
    }
}

void PhysicsLabApp::BuildStackStressScene()
{
    AddBox(AE::Physics::FVector2D(640.0f, 665.0f), 900.0f, 32.0f, 0.0f, WallFill, WallEdge);
    AddBox(AE::Physics::FVector2D(184.0f, 510.0f), 28.0f, 310.0f, 0.0f, WallFill, WallEdge, -0.08f);
    AddBox(AE::Physics::FVector2D(1096.0f, 510.0f), 28.0f, 310.0f, 0.0f, WallFill, WallEdge, 0.08f);

    const float boxWidth = 46.0f;
    const float boxHeight = 26.0f;
    const float startX = 640.0f - (static_cast<float>(stackColumns) - 1.0f) * boxWidth * 0.52f;
    for (int row = 0; row < stackRows; ++row)
    {
        for (int column = 0; column < stackColumns - row / 2; ++column)
        {
            const float x = startX + static_cast<float>(column) * boxWidth * 1.05f +
                (row % 2 == 0 ? 0.0f : boxWidth * 0.5f);
            const float y = 610.0f - static_cast<float>(row) * boxHeight * 1.18f;
            AE::Physics::Body* body = AddBox(
                AE::Physics::FVector2D(x, y),
                boxWidth,
                boxHeight,
                1.0f,
                row % 2 == 0 ? Warm : Cool,
                Dark,
                0.02f * static_cast<float>(column % 3 - 1));
            body->friction = stackFriction;
            body->restitution = stackRestitution;
        }
    }
}

void PhysicsLabApp::BuildCollisionFiltersScene()
{
    AddBox(AE::Physics::FVector2D(640.0f, 665.0f), 940.0f, 32.0f, 0.0f, WallFill, WallEdge);
    AddBox(
        AE::Physics::FVector2D(640.0f, 470.0f),
        340.0f,
        160.0f,
        0.0f,
        SensorFill,
        SensorEdge,
        0.0f,
        CategorySensor,
        sensorsEnabled ? 0xFFFFFFFFu : 0u,
        sensorsEnabled,
        true);

    const std::uint32_t redMask = filterRedBlue ? (0xFFFFFFFFu & ~CategoryBlue) : 0xFFFFFFFFu;
    const std::uint32_t blueMask = filterRedBlue ? (0xFFFFFFFFu & ~CategoryRed) : 0xFFFFFFFFu;

    for (int index = 0; index < 6; ++index)
    {
        AE::Physics::Body* red = AddCircle(
            AE::Physics::FVector2D(460.0f + index * 26.0f, 210.0f - index * 18.0f),
            16.0f,
            1.0f,
            Red,
            Dark,
            CategoryRed,
            redMask);
        red->velocity.
            X = 80.0f;

        AE::Physics::Body* blue = AddBox(
            AE::Physics::FVector2D(820.0f - index * 28.0f, 210.0f - index * 18.0f),
            30.0f,
            30.0f,
            1.0f,
            Blue,
            Dark,
            0.0f,
            CategoryBlue,
            blueMask);
        blue->velocity.X = -80.0f;
    }
}

void PhysicsLabApp::BuildMovingPlatformsScene()
{
    AddBox(AE::Physics::FVector2D(640.0f, 665.0f), 940.0f, 32.0f, 0.0f, WallFill, WallEdge);

    AE::Physics::Body* first = AddBox(AE::Physics::FVector2D(420.0f, 520.0f), 210.0f, 24.0f, 0.0f, WallFill, WallEdge);
    AE::Physics::Body* second = AddBox(AE::Physics::FVector2D(810.0f, 400.0f), 190.0f, 24.0f, 0.0f, WallFill, WallEdge, 0.12f);
    movingPlatforms.push_back(KinematicBody{first, first->position, AE::Physics::FVector2D(1.0f, 0.0f), 0.0f});
    movingPlatforms.push_back(KinematicBody{second, second->position, AE::Physics::FVector2D(0.0f, 1.0f), 1.7f});

    for (int index = 0; index < 10; ++index)
    {
        const float x = 360.0f + static_cast<float>(index % 5) * 42.0f;
        const float y = 210.0f - static_cast<float>(index / 5) * 38.0f;
        AE::Physics::Body* body = index % 2 == 0
            ? AddBox(AE::Physics::FVector2D(x, y), 32.0f, 32.0f, 1.0f, Warm, Dark)
            : AddCircle(AE::Physics::FVector2D(x, y), 16.0f, 1.0f, Cool, Dark);
        body->friction = 0.04f;
        body->restitution = 0.08f;
    }
}

void PhysicsLabApp::BuildPinballScene()
{
    AddBox(AE::Physics::FVector2D(640.0f, 668.0f), 520.0f, 28.0f, 0.0f, WallFill, WallEdge);
    AddBox(AE::Physics::FVector2D(380.0f, 410.0f), 28.0f, 520.0f, 0.0f, WallFill, WallEdge, -0.12f);
    AddBox(AE::Physics::FVector2D(900.0f, 410.0f), 28.0f, 520.0f, 0.0f, WallFill, WallEdge, 0.12f);
    AddBox(AE::Physics::FVector2D(520.0f, 570.0f), 170.0f, 24.0f, 0.0f, WallFill, WallEdge, 0.45f);
    AddBox(AE::Physics::FVector2D(760.0f, 570.0f), 170.0f, 24.0f, 0.0f, WallFill, WallEdge, -0.45f);

    for (int index = 0; index < 4; ++index)
    {
        const float x = 520.0f + static_cast<float>(index % 2) * 230.0f;
        const float y = 225.0f + static_cast<float>(index / 2) * 145.0f;
        AE::Physics::Body* bumper = AddCircle(AE::Physics::FVector2D(x, y), 34.0f, 0.0f, Violet, WallEdge);
        bumper->restitution = 1.1f;
        bumper->friction = 0.0f;
    }

    AE::Physics::Body* leftFlipper = AddBox(AE::Physics::FVector2D(550.0f, 615.0f), 140.0f, 20.0f, 0.0f, Green, Dark, 0.22f);
    AE::Physics::Body* rightFlipper = AddBox(AE::Physics::FVector2D(730.0f, 615.0f), 140.0f, 20.0f, 0.0f, Green, Dark, -0.22f);
    flippers.push_back(leftFlipper);
    flippers.push_back(rightFlipper);

    AE::Physics::Body* ball = AddCircle(AE::Physics::FVector2D(640.0f, 515.0f), 18.0f, 1.0f, Warm, Dark);
    ball->restitution = 0.85f;
    ball->friction = 0.0f;
    pinballBalls.push_back(ball);
}

void PhysicsLabApp::BuildBridgeRopeScene()
{
    AE::Physics::Body* leftAnchor = AddBox(AE::Physics::FVector2D(310.0f, 265.0f), 54.0f, 30.0f, 0.0f, WallFill, WallEdge);
    AE::Physics::Body* rightAnchor = AddBox(AE::Physics::FVector2D(970.0f, 265.0f), 54.0f, 30.0f, 0.0f, WallFill, WallEdge);
    AddBox(AE::Physics::FVector2D(640.0f, 675.0f), 980.0f, 28.0f, 0.0f, WallFill, WallEdge);

    std::vector<AE::Physics::Body*> links;
    const float startX = 350.0f;
    const float spacing = (940.0f - startX) / static_cast<float>(std::max(1, bridgeSegments - 1));
    for (int index = 0; index < bridgeSegments; ++index)
    {
        const float x = startX + spacing * static_cast<float>(index);
        const float y = 310.0f + std::sin(static_cast<float>(index) * 0.55f) * 8.0f;
        AE::Physics::Body* link = AddBox(AE::Physics::FVector2D(x, y), 34.0f, 16.0f, 1.0f, index % 2 == 0 ? Cool : Warm, Dark);
        link->friction = 0.18f;
        links.push_back(link);
    }

    if (!links.empty())
    {
        AddJoint(leftAnchor, links.front(), (leftAnchor->position + links.front()->position) * 0.5f);
        for (std::size_t index = 1; index < links.size(); ++index)
        {
            AddJoint(links[index - 1], links[index], (links[index - 1]->position + links[index]->position) * 0.5f);
        }
        AddJoint(links.back(), rightAnchor, (links.back()->position + rightAnchor->position) * 0.5f);
    }

    AE::Physics::Body* load = AddCircle(AE::Physics::FVector2D(640.0f, 155.0f), 34.0f, bridgeLoadMass, Red, Dark);
    load->restitution = 0.05f;
    load->friction = 0.12f;
}

void PhysicsLabApp::StepContainer(float dt, const InputState& input)
{
    const AE::Physics::FVector2D previousCenter = containerCenter;
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
    }

    containerCenter.X = ClampFloat(containerCenter.X, 260.0f, static_cast<float>(WindowWidth) - 260.0f);
    containerCenter.Y = ClampFloat(containerCenter.Y, 245.0f, static_cast<float>(WindowHeight) - 110.0f);
    UpdateContainerWalls();

    const bool movedContainer = (containerCenter - previousCenter).MagnitudeSquared() > 0.01f ||
        std::abs(containerAngle - previousAngle) > 0.0001f;
    if (movedContainer && world)
    {
        world->WakeAllBodies();
    }
}

void PhysicsLabApp::StepCollisionFilters()
{
    const std::uint32_t redMask = filterRedBlue ? (0xFFFFFFFFu & ~CategoryBlue) : 0xFFFFFFFFu;
    const std::uint32_t blueMask = filterRedBlue ? (0xFFFFFFFFu & ~CategoryRed) : 0xFFFFFFFFu;

    for (BodyVisual& visual : visuals)
    {
        if (!visual.body)
        {
            continue;
        }
        if (visual.body->collisionCategory == CategoryRed)
        {
            visual.body->collisionMask = redMask;
        }
        else if (visual.body->collisionCategory == CategoryBlue)
        {
            visual.body->collisionMask = blueMask;
        }
        else if (visual.body->collisionCategory == CategorySensor)
        {
            visual.body->isSensor = sensorsEnabled;
            visual.body->collisionMask = sensorsEnabled ? 0xFFFFFFFFu : 0u;
        }
    }
}

void PhysicsLabApp::StepMovingPlatforms()
{
    for (KinematicBody& platform : movingPlatforms)
    {
        const float offset = std::sin(sceneTime * platformSpeed + platform.phase) * platformAmplitude;
        const AE::Physics::FVector2D previousPosition = platform.body->position;
        platform.body->position = platform.basePosition + platform.axis * offset;
        platform.body->velocity = (platform.body->position - previousPosition) * (1.0f / FixedTimeStep);
        platform.body->shape->UpdateVertices(platform.body->rotation, platform.body->position);
    }
}

void PhysicsLabApp::StepPinball(float dt, const InputState& input)
{
    if (flippers.size() >= 2)
    {
        const float autoValue = autoFlippers ? (std::sin(sceneTime * 5.5f) * 0.5f + 0.5f) : 0.0f;
        const float leftTarget = (input.leftFlipper || autoValue > 0.65f) ? -0.45f : 0.22f;
        const float rightTarget = (input.rightFlipper || autoValue < 0.35f) ? 0.45f : -0.22f;
        flippers[0]->rotation += (leftTarget - flippers[0]->rotation) * std::min(1.0f, 14.0f * dt);
        flippers[1]->rotation += (rightTarget - flippers[1]->rotation) * std::min(1.0f, 14.0f * dt);
        for (AE::Physics::Body* flipper : flippers)
        {
            flipper->shape->UpdateVertices(flipper->rotation, flipper->position);
        }
        if ((input.leftFlipper || input.rightFlipper || autoFlippers) && world)
        {
            world->WakeAllBodies();
        }
    }

    if (launchRequested && !pinballBalls.empty())
    {
        AE::Physics::Body* ball = pinballBalls.front();
        ball->position = AE::Physics::FVector2D(640.0f, 515.0f);
        ball->velocity = AE::Physics::FVector2D(0.0f, -pinballLaunchSpeed);
        ball->shape->UpdateVertices(ball->rotation, ball->position);
        launchRequested = false;
    }
}

void PhysicsLabApp::RespawnLostBodies()
{
    if (scene == Scene::Container)
    {
        for (std::size_t index = 0; index < containerParticles.size(); ++index)
        {
            AE::Physics::Body* body = containerParticles[index];
            const bool lost = body->position.Y > static_cast<float>(WindowHeight) + 260.0f ||
                body->position.X < -260.0f ||
                body->position.X > static_cast<float>(WindowWidth) + 260.0f;
            if (lost)
            {
                body->position = ContainerToWorld(ContainerParticleLocalPosition(index));
                body->velocity = AE::Physics::FVector2D(0.0f, -35.0f);
                body->angularVelocity = 0.0f;
                body->shape->UpdateVertices(body->rotation, body->position);
            }
        }
    }
    else if (scene == Scene::Pinball)
    {
        for (AE::Physics::Body* body : pinballBalls)
        {
            if (body->position.Y > static_cast<float>(WindowHeight) + 120.0f)
            {
                body->position = AE::Physics::FVector2D(640.0f, 515.0f);
                body->velocity = AE::Physics::FVector2D(0.0f, -pinballLaunchSpeed * 0.6f);
                body->shape->UpdateVertices(body->rotation, body->position);
            }
        }
    }
}

void PhysicsLabApp::ApplyDamping()
{
    for (BodyVisual& visual : visuals)
    {
        AE::Physics::Body* body = visual.body;
        if (!body || body->IsStatic())
        {
            continue;
        }

        body->velocity *= damping;
        body->angularVelocity *= damping;
        constexpr float MaxSpeed = 1600.0f;
        if (body->velocity.MagnitudeSquared() > MaxSpeed * MaxSpeed)
        {
            body->velocity = body->velocity.UnitVector() * MaxSpeed;
        }
    }
}

AE::Physics::Body* PhysicsLabApp::AddCircle(
    AE::Physics::FVector2D position,
    float radius,
    float mass,
    SDL_Color fill,
    SDL_Color edge,
    std::uint32_t category,
    std::uint32_t mask,
    bool sensor)
{
    AE::Physics::CircleShape shape(radius);
    AE::Physics::Body* body = new AE::Physics::Body(shape, position.X, position.Y, mass);
    body->collisionCategory = category;
    body->collisionMask = mask;
    body->isSensor = sensor;
    world->AddBody(body);
    visuals.push_back(BodyVisual{body, fill, edge, false});
    return body;
}

AE::Physics::Body* PhysicsLabApp::AddBox(
    AE::Physics::FVector2D position,
    float width,
    float height,
    float mass,
    SDL_Color fill,
    SDL_Color edge,
    float rotation,
    std::uint32_t category,
    std::uint32_t mask,
    bool sensor,
    bool outlineOnly)
{
    AE::Physics::BoxShape shape(width, height);
    AE::Physics::Body* body = new AE::Physics::Body(shape, position.X, position.Y, mass);
    body->rotation = rotation;
    body->collisionCategory = category;
    body->collisionMask = mask;
    body->isSensor = sensor;
    body->shape->UpdateVertices(body->rotation, body->position);
    world->AddBody(body);
    visuals.push_back(BodyVisual{body, fill, edge, outlineOnly});
    return body;
}

void PhysicsLabApp::AddJoint(AE::Physics::Body* first, AE::Physics::Body* second, const AE::Physics::FVector2D& anchor)
{
    world->AddConstraint(new AE::Physics::JointConstraint(first, second, anchor));
}

std::vector<PhysicsLabApp::WallSpec> PhysicsLabApp::CreateContainerWallSpecs(ContainerShape shape) const
{
    switch (shape)
    {
        case ContainerShape::Tray:
            return 
            {
                WallSpec{AE::Physics::FVector2D(0.0f, 122.0f), 620.0f, 28.0f, 0.0f},
                WallSpec{AE::Physics::FVector2D(-310.0f, 44.0f), 28.0f, 174.0f, 0.0f},
                WallSpec{AE::Physics::FVector2D(310.0f, 44.0f), 28.0f, 174.0f, 0.0f}
            };
        case ContainerShape::Funnel:
            return 
            {
                WallSpec{AE::Physics::FVector2D(0.0f, 152.0f), 132.0f, 28.0f, 0.0f},
                WallSpec{AE::Physics::FVector2D(-146.0f, 26.0f), 28.0f, 336.0f, -0.48f},
                WallSpec{AE::Physics::FVector2D(146.0f, 26.0f), 28.0f, 336.0f, 0.48f}
            };
        case ContainerShape::Cup:
        default:
            return 
            {
                WallSpec{AE::Physics::FVector2D(0.0f, 142.0f), 452.0f, 30.0f, 0.0f},
                WallSpec{AE::Physics::FVector2D(-226.0f, 0.0f), 30.0f, 304.0f, 0.0f},
                WallSpec{AE::Physics::FVector2D(226.0f, 0.0f), 30.0f, 304.0f, 0.0f}
            };
    }
}

AE::Physics::FVector2D PhysicsLabApp::ContainerToWorld(const AE::Physics::FVector2D& localPosition) const
{
    return containerCenter + localPosition.Rotate(containerAngle);
}

AE::Physics::FVector2D PhysicsLabApp::ContainerParticleLocalPosition(std::size_t index) const
{
    const int columns = containerShape == ContainerShape::Tray ? 12 : 9;
    const float spacing = particleRadius * 2.35f;
    const int column = static_cast<int>(index % static_cast<std::size_t>(columns));
    const int row = static_cast<int>(index / static_cast<std::size_t>(columns));
    const float jitterX = static_cast<float>((static_cast<int>(index) * 17) % 5 - 2) * 1.35f;
    const float jitterY = static_cast<float>((static_cast<int>(index) * 11) % 5 - 2) * 1.1f;
    return AE::Physics::FVector2D(
        (static_cast<float>(column) - (static_cast<float>(columns) - 1.0f) * 0.5f) * spacing + jitterX,
        94.0f - static_cast<float>(row) * spacing + jitterY);
}

void PhysicsLabApp::UpdateContainerWalls()
{
    for (std::size_t index = 0; index < containerWalls.size() && index < containerWallSpecs.size(); ++index)
    {
        AE::Physics::Body* body = containerWalls[index];
        const WallSpec& wall = containerWallSpecs[index];
        body->position = ContainerToWorld(wall.localPosition);
        body->rotation = containerAngle + wall.localAngle;
        body->velocity = AE::Physics::FVector2D::Zero;
        body->angularVelocity = 0.0f;
        body->shape->UpdateVertices(body->rotation, body->position);
    }
}

void PhysicsLabApp::Render()
{
    if (!renderer || !frameTexture)
    {
        return;
    }

    const Uint64 renderStart = SDL_GetPerformanceCounter();

    BeginUiFrame();
    BeginFrameTexture();
    RenderScene();
    RenderUi();
    lastRenderMs = ElapsedMs(renderStart, SDL_GetPerformanceCounter());
    PresentFrameTexture();
}

bool PhysicsLabApp::CreateFrameTexture()
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

void PhysicsLabApp::BeginFrameTexture()
{
    SDL_SetRenderTarget(renderer, frameTexture);
    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderSetViewport(renderer, nullptr);
    SDL_RenderSetScale(renderer, 1.0f, 1.0f);
}

void PhysicsLabApp::PresentFrameTexture()
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

SDL_Rect PhysicsLabApp::CalculateFrameViewport() const
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

void PhysicsLabApp::RenderScene()
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

    DrawConstraints();
    for (const BodyVisual& visual : visuals)
    {
        DrawBody(visual);
    }
    if (showContacts)
    {
        DrawContacts();
    }
}

void PhysicsLabApp::BeginUiFrame()
{
    if (!imguiReady)
    {
        return;
    }

    ImGui_ImplSDL2_NewFrame(window);
    ImGuiSDL::ApplyLogicalDisplaySize(window, renderer, WindowWidth, WindowHeight);
    ImGui::NewFrame();
}

void PhysicsLabApp::RenderUi()
{
    if (!imguiReady)
    {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(18.0f, 18.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340.0f, 430.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Physics Lab");

    const char* sceneNames[] = {
        "Container",
        "Stack Stress",
        "Collision Filters",
        "Moving Platforms",
        "Pinball",
        "Bridge / Rope"
    };
    int sceneIndex = static_cast<int>(scene);
    if (ImGui::Combo("Scene", &sceneIndex, sceneNames, IM_ARRAYSIZE(sceneNames)))
    {
        ResetScene(static_cast<Scene>(sceneIndex));
    }

    if (ImGui::Button(paused ? "Resume" : "Pause"))
    {
        paused = !paused;
        UpdateWindowTitle();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        ResetScene(scene);
    }
    ImGui::SameLine();
    if (ImGui::Button("Launch"))
    {
        launchRequested = true;
    }

    bool resetRequested = false;
    resetRequested |= ImGui::SliderFloat("Gravity", &gravity, 0.0f, 24.0f, "%.2f");
    ImGui::SliderFloat("Damping", &damping, 0.970f, 1.000f, "%.3f");
    if (ImGui::SliderInt("Solver iterations", &solverIterations, 1, 20) && world)
    {
        world->SetSolverIterations(solverIterations);
    }
    if (ImGui::Checkbox("Parallel solver", &parallelSolverEnabled) && world)
    {
        world->SetParallelSolverEnabled(parallelSolverEnabled);
    }
    if (ImGui::SliderInt("Solver threshold", &parallelSolverMinConstraints, 1, 512) && world)
    {
        world->SetParallelSolverMinConstraints(static_cast<std::size_t>(parallelSolverMinConstraints));
    }
    if (ImGui::Checkbox("Broad phase", &broadPhaseEnabled) && world)
    {
        world->SetBroadPhaseEnabled(broadPhaseEnabled);
    }
    if (ImGui::SliderFloat("Grid cell", &broadPhaseCellSize, 16.0f, 256.0f, "%.0f") && world)
    {
        world->SetBroadPhaseCellSize(broadPhaseCellSize);
    }
    if (ImGui::Checkbox("Parallel narrow", &parallelNarrowPhaseEnabled) && world)
    {
        world->SetParallelNarrowPhaseEnabled(parallelNarrowPhaseEnabled);
    }
    if (ImGui::SliderInt("Parallel threshold", &parallelNarrowPhaseMinPairs, 32, 2048) && world)
    {
        world->SetParallelNarrowPhaseMinPairs(static_cast<std::size_t>(parallelNarrowPhaseMinPairs));
    }
    if (ImGui::Checkbox("Sleeping", &sleepingEnabled) && world)
    {
        world->SetSleepingEnabled(sleepingEnabled);
    }
    if (ImGui::SliderFloat("Sleep linear", &sleepLinearThreshold, 0.0f, 40.0f, "%.1f") && world)
    {
        world->SetSleepThresholds(sleepLinearThreshold, sleepAngularThreshold, sleepTimeThreshold);
    }
    if (ImGui::SliderFloat("Sleep angular", &sleepAngularThreshold, 0.0f, 0.5f, "%.3f") && world)
    {
        world->SetSleepThresholds(sleepLinearThreshold, sleepAngularThreshold, sleepTimeThreshold);
    }
    if (ImGui::SliderFloat("Sleep time", &sleepTimeThreshold, 0.0f, 2.0f, "%.2f") && world)
    {
        world->SetSleepThresholds(sleepLinearThreshold, sleepAngularThreshold, sleepTimeThreshold);
    }
    ImGui::Checkbox("Contacts", &showContacts);
#ifdef AMBER_ENABLE_PHYSICS_LAB_OUTPUT_LOG
    ImGui::Checkbox("Output Log", &showOutputLog);
#endif

    ImGui::Separator();

    switch (scene)
    {
        case Scene::Container:
        {
            const char* shapeNames[] = {"Cup", "Tray", "Funnel"};
            int shapeIndex = static_cast<int>(containerShape);
            if (ImGui::Combo("Shape", &shapeIndex, shapeNames, IM_ARRAYSIZE(shapeNames)))
            {
                containerShape = static_cast<ContainerShape>(shapeIndex);
                resetRequested = true;
            }
            resetRequested |= ImGui::SliderInt("Particles", &particleCount, 24, 2000);
            resetRequested |= ImGui::SliderFloat("Radius", &particleRadius, 4.0f, 12.0f, "%.1f");
            resetRequested |= ImGui::SliderFloat("Friction", &particleFriction, 0.0f, 0.8f, "%.2f");
            resetRequested |= ImGui::SliderFloat("Restitution", &particleRestitution, 0.0f, 1.0f, "%.2f");
            if (ImGui::SliderFloat("Angle", &containerAngle, -MaxContainerAngle, MaxContainerAngle, "%.2f"))
            {
                UpdateContainerWalls();
            }
            break;
        }
        case Scene::StackStress:
            resetRequested |= ImGui::SliderInt("Rows", &stackRows, 2, 12);
            resetRequested |= ImGui::SliderInt("Columns", &stackColumns, 3, 14);
            resetRequested |= ImGui::SliderFloat("Friction", &stackFriction, 0.0f, 0.9f, "%.2f");
            resetRequested |= ImGui::SliderFloat("Restitution", &stackRestitution, 0.0f, 1.0f, "%.2f");
            break;
        case Scene::CollisionFilters:
            ImGui::Checkbox("Red ignores blue", &filterRedBlue);
            ImGui::Checkbox("Sensor enabled", &sensorsEnabled);
            break;
        case Scene::MovingPlatforms:
            ImGui::SliderFloat("Amplitude", &platformAmplitude, 20.0f, 180.0f, "%.1f");
            ImGui::SliderFloat("Speed", &platformSpeed, 0.2f, 3.5f, "%.2f");
            break;
        case Scene::Pinball:
            ImGui::SliderFloat("Launch speed", &pinballLaunchSpeed, 250.0f, 1500.0f, "%.0f");
            ImGui::Checkbox("Auto flippers", &autoFlippers);
            break;
        case Scene::BridgeRope:
            resetRequested |= ImGui::SliderInt("Segments", &bridgeSegments, 5, 26);
            resetRequested |= ImGui::SliderFloat("Load mass", &bridgeLoadMass, 1.0f, 15.0f, "%.1f");
            break;
    }

    if (resetRequested)
    {
        ResetScene(scene);
    }

    ImGui::Separator();
    const AE::Physics::WorldStats stats = world ? world->GetLastStats() : AE::Physics::WorldStats{};
    SDL_RendererInfo rendererInfo{};
    SDL_GetRendererInfo(renderer, &rendererInfo);
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame: %.3f ms", io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);
    ImGui::Text("Renderer: %s", rendererInfo.name ? rendererInfo.name : "unknown");
    ImGui::Text("Update/Render: %.3f / %.3f", lastUpdateMs, lastRenderMs);
    ImGui::Text("Fixed steps: %d", fixedStepsThisFrame);
    ImGui::Text("Bodies: %d", BodyCount());
    ImGui::Text("Contacts: %d", ContactCount());
    ImGui::Text("Constraints: %d", ConstraintCount());
    ImGui::Text("Pairs: %zu -> %zu", stats.bruteForcePairs, stats.broadPhasePairs);
    ImGui::Text("Narrow phase: %zu", stats.narrowPhaseTests);
    ImGui::Text("Mask filtered: %zu", stats.maskFilteredPairs);
    ImGui::Text("Static skipped: %zu", stats.staticPairFilteredPairs);
    ImGui::Text("Sleeping skipped: %zu", stats.sleepingPairFilteredPairs);
    ImGui::Text("Sleeping bodies: %zu", stats.sleepingBodyCount);
    ImGui::Text("Solver iters: %d", stats.solverIterations);
    ImGui::Text("Solver constraints: %zu", stats.solverConstraintCount);
    ImGui::Text(
        "Solver islands: %zu (max %zu bodies / %zu constraints)",
        stats.solverIslandCount,
        stats.largestSolverIslandBodyCount,
        stats.largestSolverIslandConstraintCount);
    ImGui::Text("Physics ms: %.3f", stats.totalStepMs);
    ImGui::Text("Broad/Narrow: %.3f / %.3f", stats.broadPhaseMs, stats.narrowPhaseMs);
    ImGui::Text("Solver: %.3f", stats.solverPhaseMs);
    ImGui::Text("Parallel narrow: %s (%zu jobs)", stats.parallelNarrowPhaseUsed ? "on" : "off", stats.parallelNarrowPhaseJobs);
    ImGui::Text("Parallel solver: %s (%zu jobs)", stats.parallelSolverUsed ? "on" : "off", stats.parallelSolverJobs);
    ImGui::Text("Angle: %.1f deg", containerAngle * DegreesPerRadian);
    ImGui::End();

#ifdef AMBER_ENABLE_PHYSICS_LAB_OUTPUT_LOG
    if (showOutputLog)
    {
        outputLog.Draw(&showOutputLog);
    }
#endif

    ImGui::Render();
    ImGuiSDL::Render(ImGui::GetDrawData());
}

void PhysicsLabApp::DrawBody(const BodyVisual& visual) const
{
    if (!visual.body || !visual.body->shape)
    {
        return;
    }

    if (visual.body->shape->GetType() == AE::Physics::CIRCLE)
    {
        const AE::Physics::CircleShape* circle = static_cast<const AE::Physics::CircleShape*>(visual.body->shape);
        if (!visual.outlineOnly)
        {
            DrawFilledCircle(RoundToInt(visual.body->position.X), RoundToInt(visual.body->position.Y), RoundToInt(circle->radius), visual.fill);
        }
        DrawFilledCircle(RoundToInt(visual.body->position.X), RoundToInt(visual.body->position.Y), 2, visual.edge);
    }
    else
    {
        const AE::Physics::PolygonShape* polygon = static_cast<const AE::Physics::PolygonShape*>(visual.body->shape);
        if (!visual.outlineOnly)
        {
            DrawFilledPolygon(polygon->worldVertices, visual.fill);
        }
        DrawPolyline(polygon->worldVertices, visual.edge, true);
    }
}

void PhysicsLabApp::DrawConstraints() const
{
    if (!world)
    {
        return;
    }

    for (const AE::Physics::Constraint* constraint : world->GetConstraints())
    {
        if (constraint && constraint->a && constraint->b)
        {
            DrawLine(constraint->a->position, constraint->b->position, JointColor);
        }
    }
}

void PhysicsLabApp::DrawContacts() const
{
    if (!world)
    {
        return;
    }

    for (const AE::Physics::Contact& contact : world->GetContacts())
    {
        DrawLine(contact.start, contact.end, ContactColor);
        DrawFilledCircle(RoundToInt(contact.start.X), RoundToInt(contact.start.Y), 3, ContactColor);
    }
}

void PhysicsLabApp::DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int y = -radius; y <= radius; ++y)
    {
        const int span = static_cast<int>(std::sqrt(static_cast<float>(radius * radius - y * y)));
        SDL_RenderDrawLine(renderer, centerX - span, centerY + y, centerX + span, centerY + y);
    }
}

void PhysicsLabApp::DrawFilledPolygon(const std::vector<AE::Physics::FVector2D>& vertices, SDL_Color color) const
{
    if (vertices.size() < 3)
    {
        return;
    }

    float minY = vertices.front().Y;
    float maxY = vertices.front().Y;
    for (const AE::Physics::FVector2D& vertex : vertices)
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

        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            const AE::Physics::FVector2D& a = vertices[i];
            const AE::Physics::FVector2D& b = vertices[(i + 1) % vertices.size()];
            if ((a.Y <= scanY && b.Y > scanY) || (b.Y <= scanY && a.Y > scanY))
            {
                const float t = (scanY - a.Y) / (b.Y - a.Y);
                intersections.push_back(a.X + t * (b.X - a.X));
            }
        }

        std::sort(intersections.begin(), intersections.end());
        for (std::size_t i = 0; i + 1 < intersections.size(); i += 2)
        {
            const int x0 = std::max(0, static_cast<int>(std::ceil(intersections[i])));
            const int x1 = std::min(WindowWidth - 1, static_cast<int>(std::floor(intersections[i + 1])));
            SDL_RenderDrawLine(renderer, x0, y, x1, y);
        }
    }
}

void PhysicsLabApp::DrawPolyline(const std::vector<AE::Physics::FVector2D>& vertices, SDL_Color color, bool closed) const
{
    if (vertices.size() < 2)
    {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (std::size_t i = 0; i + 1 < vertices.size(); ++i)
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

void PhysicsLabApp::DrawLine(const AE::Physics::FVector2D& from, const AE::Physics::FVector2D& to, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(renderer, RoundToInt(from.X), RoundToInt(from.Y), RoundToInt(to.X), RoundToInt(to.Y));
}

void PhysicsLabApp::DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect{x, y, w, h};
    SDL_RenderFillRect(renderer, &rect);
}

int PhysicsLabApp::BodyCount() const
{
    return world ? static_cast<int>(world->GetBodies().size()) : 0;
}

int PhysicsLabApp::ContactCount() const
{
    return world ? static_cast<int>(world->GetContacts().size()) : 0;
}

int PhysicsLabApp::ConstraintCount() const
{
    return world ? static_cast<int>(world->GetConstraints().size()) : 0;
}

const char* PhysicsLabApp::SceneName(Scene value)
{
    switch (value)
    {
        case Scene::Container:
            return "Container";
        case Scene::StackStress:
            return "Stack Stress";
        case Scene::CollisionFilters:
            return "Collision Filters";
        case Scene::MovingPlatforms:
            return "Moving Platforms";
        case Scene::Pinball:
            return "Pinball";
        case Scene::BridgeRope:
            return "Bridge / Rope";
        default:
            return "Unknown";
    }
}

const char* PhysicsLabApp::ContainerShapeName(ContainerShape value)
{
    switch (value)
    {
        case ContainerShape::Cup:
            return "Cup";
        case ContainerShape::Tray:
            return "Tray";
        case ContainerShape::Funnel:
            return "Funnel";
        default:
            return "Unknown";
    }
}

float PhysicsLabApp::ClampFloat(float value, float minValue, float maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}

int PhysicsLabApp::ClampInt(int value, int minValue, int maxValue)
{
    return std::max(minValue, std::min(maxValue, value));
}
