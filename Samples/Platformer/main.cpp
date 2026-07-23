#include "PlatformerApp.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    bool smokeTest = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--smoke-test")
        {
            smokeTest = true;
        }
    }

    PlatformerApp app;
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

    return app.Run();
}
