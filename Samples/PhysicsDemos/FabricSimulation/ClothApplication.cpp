

#include "ClothApplication.h"
#include "Physics/Constants.h"
#include "Logging/Logger.h"
#include "iostream"

bool Application::IsRunning()
{
	return running;
}

/* Setup function(executed once in the beginning of the simulation)*/
void Application::Setup()
{
	AE::Logger::Log("Application initialization", "Physics");
}

void Application::Setup(int clothWidth, int clothHeight, int clothSpacing)
{
	graphic = new Graphics();
	mouse = new Mouse();
	running = Graphics::OpenWindow();

	clothWidth /= clothSpacing;
	clothHeight /= clothSpacing;

	int startX = graphic->Width() * 0.5f - clothWidth * clothSpacing * 0.5f;
	int startY = graphic->Height() * 0.1f;

	cloth = new Cloth(clothWidth, clothHeight, clothSpacing, startX, startY);

	lastUpdateTime = SDL_GetTicks();
}

/* Input processing */
void Application::Input()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		diagnostics.HandleEvent(event);

		switch (event.type)
		{
		case SDL_QUIT:
			running = false;
			break;

		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				running = false;
			if (event.key.keysym.sym == SDLK_UP)
				pushForce.Y = -50 * AE::Physics::PIXELS_PER_METER;
			if (event.key.keysym.sym == SDLK_RIGHT)
				pushForce.X = 50 * AE::Physics::PIXELS_PER_METER;
			if (event.key.keysym.sym == SDLK_DOWN)
				pushForce.Y = 50 * AE::Physics::PIXELS_PER_METER;
			if (event.key.keysym.sym == SDLK_LEFT)
				pushForce.X = -50 * AE::Physics::PIXELS_PER_METER;
			break;

		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_UP)
				pushForce.Y = AE::Physics::ZERO;
			if (event.key.keysym.sym == SDLK_RIGHT)
				pushForce.X = AE::Physics::ZERO;
			if (event.key.keysym.sym == SDLK_DOWN)
				pushForce.Y = AE::Physics::ZERO;
			if (event.key.keysym.sym == SDLK_LEFT)
				pushForce.X = AE::Physics::ZERO;
			break;

		case SDL_MOUSEMOTION:
			int a, n;
			mouseCursor.X = event.motion.x;
			a = mouseCursor.X;
			n = mouseCursor.Y;
			mouseCursor.Y = event.motion.y;
			mouse->UpdatePosition(a, n);
			break;

		case SDL_MOUSEBUTTONDOWN:
			int x, y;
			SDL_GetMouseState(&x, &y);
			mouse->UpdatePosition(x, y);

			if (!mouse->GetLeftButtonDown() && event.button.button == SDL_BUTTON_LEFT)
			{
				mouse->SetLeftMouseButton(true);
			}
			if (!mouse->GetRightMouseButton() && event.button.button == SDL_BUTTON_RIGHT)
			{
				mouse->SetRightMouseButton(true);
			}
			break;

		case SDL_MOUSEBUTTONUP:
			if (mouse->GetLeftButtonDown() && event.button.button == SDL_BUTTON_LEFT)
			{
				mouse->SetLeftMouseButton(false);
			}
			if (mouse->GetRightMouseButton() && event.button.button == SDL_BUTTON_RIGHT)
			{
				mouse->SetRightMouseButton(false);
			}
			break;

		case SDL_MOUSEWHEEL:
			if (event.wheel.y > AE::Physics::ZERO)
			{
				mouse->IncreaseCursorSize(10);
			}
			else if (event.wheel.y < AE::Physics::ZERO)
			{
				mouse->IncreaseCursorSize(-10);
			}
			break;

		default:
			break;
		}
	}
}

/*  Update function(called several times per second to update objects) */
void Application::Update()
{
	diagnostics.BeginFrame();
	const uint64 updateStart = LegacyDiagnosticsOverlay::Counter();

	Uint32 currentTime = SDL_GetTicks();
	float deltaTime = (currentTime - lastUpdateTime) / AE::Physics::DotPerTimeSeconds;

	if (!diagnostics.IsPaused())
	{
		cloth->Update(mouse, deltaTime, graphic->Width(), graphic->Height());
	}

	lastUpdateTime = currentTime;
	diagnostics.SetUpdateMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(updateStart));
}

/*  Render function (called several times per second to draw objects) */
void Application::Render()
{
	const uint64 renderStart = LegacyDiagnosticsOverlay::Counter();

	Graphics::ClearScreen(static_cast<Uint32>(0xFF00000000));

	for (const auto& stick : cloth->GetSticks())
	{
		if (!stick->IsActive())
		{
			continue;
		}

		const FVector2D p0 = stick->GetPoint0Position();
		const FVector2D p1 = stick->GetPoint1Position();
		graphic->DrawLine(p0.X, p0.Y, p1.X, p1.Y, stick->GetRenderColor());
	}

	diagnostics.SetRenderMs(LegacyDiagnosticsOverlay::ElapsedMilliseconds(renderStart));
	LegacyDiagnosticsData diagnosticsData;
	diagnosticsData.sampleName = "FabricSimulationApp";
	diagnosticsData.stickCount = cloth ? cloth->GetSticks().size() : 0u;
	diagnosticsData.controls = "Mouse cuts/drags cloth, wheel changes cursor, arrows force, Esc quit";
	diagnostics.Draw(diagnosticsData);

	Graphics::RenderFrame();
}
/* Destroy function to delete objects and close the window*/
void Application::Destroy()
{
	delete mouse;
	delete graphic;
	delete cloth;

	Graphics::CloseWindow();
}
