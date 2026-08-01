#ifndef APPLICATION_H
#define APPLICATION_H

#include <vector>

#include "Logging/Logger.h"
#include "Classes/World.h"
#include "../Common/BodyTextureStore.h"
#include "../Common/LegacyDiagnosticsOverlay.h"

/**
 * @Discrioption: Application Class (
 * @func     IsRunning();
 * @func     Setup();
 * @func     Input();
 * @func     Update();
 * @func     Render();
 * @func     Destroy();
 */
class Application
{
private:
	bool debug = false;
	bool running = false;

	World* world = nullptr;

	SDL_Texture* bgTexture = nullptr;
	BodyTextureStore bodyTextures;
	Body* bob = nullptr;
	LegacyDiagnosticsOverlay diagnostics;

public:
	Application() = default;
	~Application() = default;
	bool IsRunning() const;
	void Setup();
	void Input();
	void Update();
	void Render();
	void RenderObjects();
	void Destroy();
	float TimeDeductions();
};
#endif
