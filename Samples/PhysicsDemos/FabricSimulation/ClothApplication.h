#ifndef CLOTHAPPLICATION_H
#define CLOTHAPPLICATION_H


#include "../Renderer/SDL/Graphics.h"
#include "../Common/LegacyDiagnosticsOverlay.h"
#include <vector>
#include "Physics/Objects/Cloth.h"
#include "Physics/Mouse.h"

class Application
{
private:
    bool running = false;
    bool leftMouseButtonDown = false;
    bool rightMouseButtonDown = false;

    float k = 1500;
    float restLength = 200;
    const int NUM_BodyS = 4;


    FVector2D pushForce = FVector2D(0.0, 0.0);
    FVector2D mouseCursor = FVector2D(0, 0);
    FVector2D anchor;

    SDL_Rect liquid;

    Graphics* graphic = nullptr;
    Mouse* mouse = nullptr;
    Cloth* cloth = nullptr;
    LegacyDiagnosticsOverlay diagnostics;

    Uint32 lastUpdateTime;

public:
    Application() = default;
    virtual ~Application() = default;
    bool IsRunning();
    void Setup();

    void Setup(int clothWidth, int clothHeight, int clothSpacing);
    void Input();
    void Update();
    void Render();
    void Destroy();
};
#endif
