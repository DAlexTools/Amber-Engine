

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
                if (event.key.keysym.sym == SDLK_ESCAPE)    running = false;
                if (event.key.keysym.sym == SDLK_UP)        pushForce.y = -50 * AE::Physics::PIXELS_PER_METER;
                if (event.key.keysym.sym == SDLK_RIGHT)     pushForce.x = 50 * AE::Physics::PIXELS_PER_METER;
                if (event.key.keysym.sym == SDLK_DOWN)      pushForce.y = 50 * AE::Physics::PIXELS_PER_METER;
                if (event.key.keysym.sym == SDLK_LEFT)      pushForce.x = -50 * AE::Physics::PIXELS_PER_METER;
                break;

            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_UP)        pushForce.y = AE::Physics::ZERO;
                if (event.key.keysym.sym == SDLK_RIGHT)     pushForce.x = AE::Physics::ZERO;
                if (event.key.keysym.sym == SDLK_DOWN)      pushForce.y = AE::Physics::ZERO;
                if (event.key.keysym.sym == SDLK_LEFT)      pushForce.x = AE::Physics::ZERO;
                break;

            case SDL_MOUSEMOTION:
                int a, n;
                mouseCursor.x = event.motion.x;
                a = mouseCursor.x;
                n = mouseCursor.y;
                mouseCursor.y = event.motion.y;
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

            default: break;
        }
    }
}

/*  Update function(called several times per second to update objects) */
void Application::Update()
{
    diagnostics.BeginFrame();
    const std::uint64_t updateStart = LegacyDiagnosticsOverlay::Counter();

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
    const std::uint64_t renderStart = LegacyDiagnosticsOverlay::Counter();

    Graphics::ClearScreen(static_cast<Uint32>(0xFF00000000));

    for (const auto& stick : cloth->GetSticks())
    {
        if (!stick->IsActive())
        {
            continue;
        }

        const Vector2D p0 = stick->GetPoint0Position();
        const Vector2D p1 = stick->GetPoint1Position();
        graphic->DrawLine(p0.x, p0.y, p1.x, p1.y, stick->GetRenderColor());
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
