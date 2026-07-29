#ifndef ENGINE_EDITOR_DIAGNOSTICS_SAMPLE_DIAGNOSTICS_OVERLAY_H
#define ENGINE_EDITOR_DIAGNOSTICS_SAMPLE_DIAGNOSTICS_OVERLAY_H

#include <SDL2/SDL.h>

#include <functional>
#include <string>

#include "Core/Platform/PlatformTypes.h"
#include "OutputLogWidget.h"

namespace AE::Editor
{

struct SamplePhysicsStats
{
	SizeT bodies = 0;
	SizeT contacts = 0;
	SizeT constraints = 0;
	SizeT broadPhasePairs = 0;
	SizeT bruteForcePairs = 0;
	SizeT narrowPhaseTests = 0;
	SizeT solverIslandCount = 0;
	SizeT largestSolverIslandBodyCount = 0;
	SizeT largestSolverIslandConstraintCount = 0;
	SizeT parallelNarrowPhaseJobs = 0;
	SizeT parallelSolverJobs = 0;
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

} // namespace AE::Editor

#endif
