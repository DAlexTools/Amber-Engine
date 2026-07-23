#ifndef APPLICATION_H
#define APPLICATION_H

#include "../Renderer/SDL/Graphics.h"
#include "Classes/World.h"
#include "../Common/BodyTextureStore.h"
#include "../Common/LegacyDiagnosticsOverlay.h"
#include <vector>

class Application {
    private:
        bool debug = true;
        bool running = false;
        World* world;
        BodyTextureStore bodyTextures;
        LegacyDiagnosticsOverlay diagnostics;

    public:
        Application() = default;
        ~Application() = default;
        bool IsRunning();
        void Setup();
        void Input();
        void Update();
        void Render();
        void Destroy();
};

#endif
