

#include "ClothApplication.h"

int main(int argc, char* args[])
{
    Application app;

    app.Setup(600, 250, 20);

    app.Setup();
    while (app.IsRunning())
    {
        app.Input();
        app.Update();
        app.Render();
    }

    app.Destroy();

    return 0;
}