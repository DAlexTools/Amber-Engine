#ifndef LEGACY_DIAGNOSTICS_OVERLAY_H
#define LEGACY_DIAGNOSTICS_OVERLAY_H

#include "Classes/World.h"

#include <SDL2/SDL.h>

#include <cstddef>
#include <cstdint>

struct LegacyDiagnosticsData
{
    const char* sampleName = "";
    World* world = nullptr;
    std::size_t particleCount = 0;
    std::size_t stickCount = 0;
    bool debugDraw = false;
    const char* controls = "";
};

class LegacyDiagnosticsOverlay
{
public:
    void BeginFrame();
    void HandleEvent(const SDL_Event& event);
    void SetUpdateMs(double ms);
    void SetRenderMs(double ms);
    void Draw(const LegacyDiagnosticsData& data) const;

    bool IsPaused() const;
    bool& ShowPerformance();
    bool& ShowControls();
    bool& ShowOutputLog();
    bool& Paused();

    static std::uint64_t Counter();
    static double ElapsedMilliseconds(std::uint64_t startCounter);

private:
    bool showPerformance = true;
    bool showControls = true;
    bool showOutputLog = true;
    bool paused = false;
    double frameMs = 0.0;
    double fps = 0.0;
    double updateMs = 0.0;
    double renderMs = 0.0;
    std::uint64_t previousFrameCounter = 0u;
};

#endif
