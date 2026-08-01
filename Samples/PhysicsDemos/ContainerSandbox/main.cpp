#include "ContainerSandboxApp.h"

#include "Core/BuildConfig.h"

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

	ContainerSandboxApp app;
#if SMOKE_TEST
	if (smokeTest)
	{
		const bool passed = app.RunSmokeTest();
		if (!passed)
		{
			std::cerr << "Container sandbox smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Container sandbox smoke test passed." << std::endl;
		return 0;
	}
#endif

	return app.Run();
}
