#ifndef LEGACY_DIAGNOSTICS_OVERLAY_H
#define LEGACY_DIAGNOSTICS_OVERLAY_H

#include "Classes/World.h"

#include <SDL2/SDL.h>

#include "Core/Platform/PlatformTypes.h"

struct LegacyDiagnosticsData
{
	const char* sampleName = "";
	World* world = nullptr;
	SizeT particleCount = 0;
	SizeT stickCount = 0;
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

	static uint64 Counter();
	static double ElapsedMilliseconds(uint64 startCounter);

private:
	bool showPerformance = true;
	bool showControls = true;
	bool showOutputLog = true;
	bool paused = false;
	double frameMs = 0.0;
	double fps = 0.0;
	double updateMs = 0.0;
	double renderMs = 0.0;
	uint64 previousFrameCounter = 0u;
};

#endif
