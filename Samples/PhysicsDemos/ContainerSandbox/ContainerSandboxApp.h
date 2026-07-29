#ifndef CONTAINER_SANDBOX_APP_H
#define CONTAINER_SANDBOX_APP_H

#include <SDL2/SDL.h>

#include <cstddef>
#include <memory>
#include <vector>

#include "Core/BuildConfig.h"
#include "Body.h"
#include "Core/Math/Vector2D.h"
#include "Classes/World.h"

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
#include "SampleDiagnosticsOverlay.h"
#endif

class ContainerSandboxApp
{
public:
    ContainerSandboxApp();

    int Run();
#if SMOKE_TEST
    bool RunSmokeTest();
#endif

private:
    enum class ContainerMode
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
    };

    struct WallSpec
    {
        AE::Math::FVector2D localPosition;
        float width = 0.0f;
        float height = 0.0f;
        float localAngle = 0.0f;
    };

    static constexpr int WindowWidth = 1280;
    static constexpr int WindowHeight = 720;

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* frameTexture = nullptr;
    bool running = false;
    bool paused = false;
    bool fullscreen = false;
    bool broadPhaseEnabled = true;
    bool sleepingEnabled = true;
    bool parallelNarrowPhaseEnabled = true;
    bool parallelSolverEnabled = true;
    float broadPhaseCellSize = 32.0f;
    int solverIterations = 6;
    int parallelNarrowPhaseMinPairs = 256;
    int parallelSolverMinConstraints = 64;
    double lastUpdateMs = 0.0;
    double lastRenderMs = 0.0;
    int fixedStepsThisFrame = 0;

    std::unique_ptr<AE::Physics::World> world;
    std::vector<WallSpec> wallSpecs;
    std::vector<AE::Physics::Body*> wallBodies;
    std::vector<AE::Physics::Body*> particleBodies;

    AE::Math::FVector2D containerCenter;
    float containerAngle = 0.0f;
    int particleCount = 72;
    ContainerMode containerMode = ContainerMode::Cup;

#ifdef AMBER_ENABLE_SAMPLE_DIAGNOSTICS
    AE::Editor::SampleDiagnosticsOverlay diagnostics;
#endif

    bool Initialize();
    void Shutdown();
    void ToggleFullscreen();
    void PollEvents(InputState& input);
    void Step(float dt, const InputState& input);

    void ResetSimulation(ContainerMode mode);
    void SetParticleCount(int count);
    void BuildWalls();
    void SpawnParticles();
    void UpdateContainerBodies();
    void ApplyParticleDamping();
    void RespawnEscapedParticles();
    void RespawnParticle(std::size_t index);

    std::vector<WallSpec> CreateWallSpecs(ContainerMode mode) const;
    AE::Math::FVector2D ParticleSpawnLocalPosition(std::size_t index) const;
    AE::Math::FVector2D ContainerToWorld(const AE::Math::FVector2D& localPosition) const;
    float AverageParticleX() const;
    int CountActiveContacts() const;

    void Render();
    bool CreateFrameTexture();
    void BeginFrameTexture();
    void PresentFrameTexture();
    SDL_Rect CalculateFrameViewport() const;
    void RenderDiagnostics();
    void DrawBackground() const;
    void DrawContainer() const;
    void DrawParticles() const;
    void DrawHud() const;
    void DrawFilledCircle(int centerX, int centerY, int radius, SDL_Color color) const;
    void DrawFilledPolygon(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color) const;
    void DrawPolyline(const std::vector<AE::Math::FVector2D>& vertices, SDL_Color color, bool closed) const;
    void DrawScreenRect(int x, int y, int w, int h, SDL_Color color) const;
    void UpdateWindowTitle();

    static const char* ModeName(ContainerMode mode);
    static ContainerMode NextMode(ContainerMode mode);
    static ContainerMode PreviousMode(ContainerMode mode);
    static float ClampFloat(float value, float minValue, float maxValue);
};

#endif

