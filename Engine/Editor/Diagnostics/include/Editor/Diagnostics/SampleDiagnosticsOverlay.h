#ifndef ENGINE_EDITOR_DIAGNOSTICS_SAMPLE_DIAGNOSTICS_OVERLAY_H
#define ENGINE_EDITOR_DIAGNOSTICS_SAMPLE_DIAGNOSTICS_OVERLAY_H

#include <SDL2/SDL.h>

#include <cstddef>
#include <functional>
#include <string>

#include "Editor/OutputLog/OutputLogWidget.h"

namespace AE::Editor
{

struct SamplePhysicsStats
{
    std::size_t bodies = 0;
    std::size_t contacts = 0;
    std::size_t constraints = 0;
    std::size_t broadPhasePairs = 0;
    std::size_t bruteForcePairs = 0;
    std::size_t narrowPhaseTests = 0;
    std::size_t solverIslandCount = 0;
    std::size_t largestSolverIslandBodyCount = 0;
    std::size_t largestSolverIslandConstraintCount = 0;
    std::size_t parallelNarrowPhaseJobs = 0;
    std::size_t parallelSolverJobs = 0;
    bool parallelNarrowPhaseUsed = false;
    bool parallelSolverUsed = false;
    double physicsStepMs = 0.0;
    double broadPhaseMs = 0.0;
    double narrowPhaseMs = 0.0;
    double solverMs = 0.0;
};

struct SampleDiagnosticsData
{
    const char* sampleName = "Sample";
    bool paused = false;
    int fixedSteps = 0;
    double updateMs = 0.0;
    double renderMs = 0.0;
    bool hasPhysicsStats = false;
    SamplePhysicsStats physics;
    std::string statusText;
};

class SampleDiagnosticsOverlay
{
public:
    bool Initialize(SDL_Window* window, SDL_Renderer* renderer, int logicalWidth, int logicalHeight);
    void Shutdown();
    bool IsReady() const;

    void ProcessEvent(const SDL_Event& event);
    bool WantsKeyboard() const;
    bool WantsMouse() const;

    void BeginFrame();
    void Draw(const SampleDiagnosticsData& data, const std::function<void()>& drawControls = {});
    void Render();

    bool& ShowDiagnostics();
    bool& ShowControls();
    bool& ShowOutputLog();

    double GetFrameMs() const;
    double GetFps() const;

private:
    OutputLogWidget outputLog;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    int logicalWidth = 0;
    int logicalHeight = 0;
    bool ready = false;
    bool showDiagnostics = true;
    bool showControls = true;
    bool showOutputLog = true;
    double frameMs = 0.0;
    double smoothedFrameMs = 0.0;
    double fps = 0.0;
    Uint64 previousCounter = 0;
};

}

#endif
