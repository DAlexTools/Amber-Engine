#include "PlatformerApp.h"

#include "Core/BuildConfig.h"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
#if SMOKE_TEST
    bool smokeTest = false;
#endif
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    std::filesystem::path scenePath;
#endif

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
#if SMOKE_TEST
        if (argument == "--smoke-test")
        {
            smokeTest = true;
        }
#endif
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
        if (argument == "--scene" && i + 1 < argc)
        {
            scenePath = argv[++i];
        }
#endif
    }

    PlatformerApp app;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    if (!scenePath.empty())
    {
        app.SetEditorScenePath(scenePath);
    }
#endif
#if SMOKE_TEST
    if (smokeTest)
    {
        const bool passed = app.RunSmokeTest();
        if (!passed)
        {
            std::cerr << "Platformer smoke test failed." << std::endl;
            return 1;
        }

        std::cout << "Platformer smoke test passed." << std::endl;
        return 0;
    }
#endif

    return app.Run();
}
