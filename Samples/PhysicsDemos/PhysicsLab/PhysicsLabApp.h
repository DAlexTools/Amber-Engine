#ifndef PHYSICS_LAB_APP_H
#define PHYSICS_LAB_APP_H

#include <SDL2/SDL.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "Core/BuildConfig.h"
#include "Body.h"
#include "Core/Math/Vector2D.h"
#include "Classes/World.h"

#ifdef AMBER_ENABLE_PHYSICS_LAB_OUTPUT_LOG
#include "OutputLogWidget.h"
#endif

class PhysicsLabApp
{
public:
    PhysicsLabApp();

    int Run();
#if SMOKE_TEST
    bool RunSmokeTest();
    bool RunUiSmokeTest();
    bool RunPerfTest();
#endif

private:
    enum class Scene
    {
        Container,
        StackStress,
        CollisionFilters,
        MovingPlatforms,
        Pinball,
        BridgeRope
    };

    enum class ContainerShape
    {
        Cup,
        Tray,
        Funnel
    };

    struct InputState
    {
        bool moveLeft = false;
        bool moveRight = false;
        bool moveUp = false;
        bool moveDown = false;
        bool rotateLeft = false;
        bool rotateRight = false;
        bool leftFlipper = false;
        bool rightFlipper = false;
    };

    struct WallSpec
    {
        AE::Math::FVector2D localPosition;
        float width = 0.0f;
        float height = 0.0f;
        float localAngle = 0.0f;
    };

    struct BodyVisual
    {
        AE::Physics::Body* body = nullptr;
        SDL_Color fill{};
        SDL_Color edge{};
        bool outlineOnly = false;
    };

    struct KinematicBody
    {
        AE::Physics::Body* body = nullptr;
        AE::Math::FVector2D basePosition;
        AE::Math::FVector2D axis;
        float phase = 0.0f;
    };

    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 720;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
    bool running = false;
    bool paused = false;
    bool imguiReady = false;
    bool fullscreen = false;
    bool showContacts = true;
#ifdef AMBER_ENABLE_PHYSICS_LAB_OUTPUT_LOG
    bool showOutputLog = true;
#endif
    bool launchRequested = false;
    bool broadPhaseEnabled = true;
    bool sleepingEnabled = true;
    bool parallelNarrowPhaseEnabled = true;
    bool parallelSolverEnabled = true;

#ifdef AMBER_ENABLE_PHYSICS_LAB_OUTPUT_LOG
    AE::Editor::OutputLogWidget outputLog;
#endif
    std::unique_ptr<AE::Physics::World> world;
    std::vector<BodyVisual> visuals;
    std::vector<AE::Physics::Body*> containerWalls;
    std::vector<AE::Physics::Body*> containerParticles;
    std::vector<WallSpec> containerWallSpecs;
    std::vector<KinematicBody> movingPlatforms;
    std::vector<AE::Physics::Body*> pinballBalls;
    std::vector<AE::Physics::Body*> flippers;

    Scene scene = Scene::Container;
    ContainerShape containerShape = ContainerShape::Cup;
    AE::Math::FVector2D containerCenter;
    float containerAngle = 0.0f;
    float sceneTime = 0.0f;
    double lastUpdateMs = 0.0;
    double lastRenderMs = 0.0;
    int fixedStepsThisFrame = 0;

    float gravity = 9.8f;
    float damping = 0.998f;
    float broadPhaseCellSize = 32.0f;
    float sleepLinearThreshold = 12.0f;
    float sleepAngularThreshold = 0.05f;
    float sleepTimeThreshold = 0.6f;
    int parallelNarrowPhaseMinPairs = 256;
    int parallelSolverMinConstraints = 64;
    int solverIterations = 6;

    int particleCount = 72;
    float particleRadius = 7.0f;
    float particleFriction = 0.02f;
    float particleRestitution = 0.12f;

    int stackRows = 7;
    int stackColumns = 8;
    float stackFriction = 0.22f;
    float stackRestitution = 0.05f;

    bool filterRedBlue = true;
    bool sensorsEnabled = true;

    float platformAmplitude = 96.0f;
    float platformSpeed = 1.0f;

    float pinballLaunchSpeed = 920.0f;
    bool autoFlippers = false;

    int bridgeSegments = 14;
    float bridgeLoadMass = 5.0f;

    bool Initialize();
    void Shutdown();
    void ToggleFullscreen();
    void InitializeImGui();
    void ShutdownImGui();
    void PollEvents(InputState& input);
    void Step(float dt, const InputState& input);
    void UpdateWindowTitle();

    void ResetScene(Scene newScene);
    void BuildScene();
    void BuildContainerScene();
    void BuildStackStressScene();
    void BuildCollisionFiltersScene();
    void BuildMovingPlatformsScene();
    void BuildPinballScene();
    void BuildBridgeRopeScene();

    void StepContainer(float dt, const InputState& input);
    void StepCollisionFilters();
    void StepMovingPlatforms();
    void StepPinball(float dt, const InputState& input);
    void RespawnLostBodies();
    void ApplyDamping();

    AE::Physics::Body* AddCircle(
        AE::Math::FVector2D position,
        float radius,
        float mass,
        SDL_Color fill,
        SDL_Color edge,
        std::uint32_t category = 0x00000001u,
        std::uint32_t mask = 0xFFFFFFFFu,
        bool sensor = false);
    AE::Physics::Body* AddBox(
        AE::Math::FVector2D position,
        float width,
        float height,
        float mass,
        SDL_Color fill,
        SDL_Color edge,
        float rotation = 0.0f,
        std::uint32_t category = 0x00000001u,
        std::uint32_t mask = 0xFFFFFFFFu,
        bool sensor = false,
        bool outlineOnly = false);
    void AddJoint(AE::Physics::Body* first, AE::Physics::Body* second, const AE::Math::FVector2D& anchor);

    std::vector<WallSpec> CreateContainerWallSpecs(ContainerShape shape) const;
    AE::Math::FVector2D ContainerToWorld(const AE::Math::FVector2D& localPosition) const;
    AE::Math::FVector2D ContainerParticleLocalPosition(std::size_t index) const;
    void UpdateContainerWalls();

    void Render();
    bool CreateFrameTexture();
    void BeginFrameTexture();
    void PresentFrameTexture();
    SDL_Rect CalculateFrameViewport() const;
    void RenderScene();
    void BeginUiFrame();
    void RenderUi();
    void DrawBody(const BodyVisual& visual) const;
    void DrawConstraints() const;
    void DrawContacts() const;
    void DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const;
    void DrawFilledPolygon(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color) const;
    void DrawPolyline(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color, bool closed) const;
    void DrawLine(const AE::Math::FVector2D& from, const AE::Math::FVector2D& to, SDL_Color color) const;
    void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;

    int BodyCount() const;
    int ContactCount() const;
    int ConstraintCount() const;

    static const char* SceneName(Scene value);
    static const char* ContainerShapeName(ContainerShape value);
    static float ClampFloat(float value, float minValue, float maxValue);
    static int ClampInt(int value, int minValue, int maxValue);
};

#endif

