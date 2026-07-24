#ifndef GAME_MODULE_H
#define GAME_MODULE_H

#include "../Core/BuildConfig.h"
#include "../Classes/Engine.h"

const int FPS = 60;
const int MILLISECS_PER_FRAME = 1000 / FPS;

class GameModule
{
public:
	explicit GameModule(AE::Engine& engine);
	~GameModule();

	void SetDebugEnabled(bool enabled);
	void SetLevelNumber(int level);
	void Setup();
	void Run();
#if SMOKE_TEST
	bool RunPhysicsContactSmokeTest();
	bool RunPhysicsObstacleSmokeTest();
#endif
	void ProcessInput();
	void Update();
	void Render();

private:
	void RenderDiagnosticsUi();

	AE::Engine& engine;
	bool isDebug = false;
	bool showDiagnostics = true;
	bool showOutputLog = true;
	int levelNumber = 1;
	int millisecsPreviousFrame = 0;
	double lastUpdateMs = 0.0;
	double lastRenderMs = 0.0;

	static bool isEditMode;
};

#endif
