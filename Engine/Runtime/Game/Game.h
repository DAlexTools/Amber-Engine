#ifndef GAME_H
#define GAME_H

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>
#include "../EntityComponentSystem/ECS.h"
#include "../AssetManager/AssetManager.h"
#include "../EventBus/EventBus.h"


const int FPS = 60;
const int MILLISECS_PER_FRAME = 1000 / FPS;

class Game
{
	
	bool isRunning = false;
	bool isDebug = false;
	bool isFullscreen = true;
	bool showDiagnostics = true;
	bool showOutputLog = true;
	int levelNumber = 1;
	int millisecsPreviousFrame = 0;
	double lastUpdateMs = 0.0;
	double lastRenderMs = 0.0;

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Rect camera;

	std::unique_ptr<Registry> registry = nullptr;
	std::unique_ptr<AssetManager> assetManager = nullptr;
	std::unique_ptr<EventBus> eventBus = nullptr;

public:
	Game();
	~Game();

	void Initialize();
	bool IsRunning() const;
	void SetDebugEnabled(bool enabled);
	void SetFullscreenEnabled(bool enabled);
	bool IsFullscreenEnabled() const;
	void SetLevelNumber(int level);
	void Setup();	
	void Run();
	bool RunPhysicsContactSmokeTest();
	bool RunPhysicsObstacleSmokeTest();
	void ProcessInput();
	void Update();
	void Render();
	void Destroy();

	static int MapWidth;
	static int MapHeight;
	static int WindowHeight;
	static int WindowWidth;
private:
	void InitializeSystems();
	void ApplyFullscreenMode();
	void ToggleFullscreen();
	void UpdateWindowDimensions();
	void RenderDiagnosticsUi();
	
	static bool isEditMode;

};

#endif
