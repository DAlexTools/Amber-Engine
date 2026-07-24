#include "Platformer2App.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
#if SMOKE_TEST
    bool smokeTest = false;
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
    }

    Platformer2App app;
#if SMOKE_TEST
    if (smokeTest)
    {
        const bool passed = app.RunSmokeTest();
        if (!passed)
        {
            std::cerr << "Platformer2 smoke test failed." << std::endl;
            return 1;
        }

        std::cout << "Platformer2 smoke test passed." << std::endl;
        return 0;
    }
#endif

    return app.Run();
}
