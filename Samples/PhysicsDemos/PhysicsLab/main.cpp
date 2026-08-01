#include "PhysicsLabApp.h"

#include "Core/BuildConfig.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
#if SMOKE_TEST
	bool smokeTest = false;
	bool uiSmokeTest = false;
	bool perfTest = false;
#endif

	for (int i = 1; i < argc; ++i)
	{
		const std::string argument = argv[i];
#if SMOKE_TEST
		if (argument == "--smoke-test")
		{
			smokeTest = true;
		}
		else if (argument == "--ui-smoke-test")
		{
			uiSmokeTest = true;
		}
		else if (argument == "--perf-test")
		{
			perfTest = true;
		}
#endif
	}

	PhysicsLabApp app;
#if SMOKE_TEST
	if (smokeTest)
	{
		const bool passed = app.RunSmokeTest();
		if (!passed)
		{
			std::cerr << "Physics lab smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Physics lab smoke test passed." << std::endl;
		return 0;
	}
	if (uiSmokeTest)
	{
		const bool passed = app.RunUiSmokeTest();
		if (!passed)
		{
			std::cerr << "Physics lab UI smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Physics lab UI smoke test passed." << std::endl;
		return 0;
	}
	if (perfTest)
	{
		return app.RunPerfTest() ? 0 : 1;
	}
#endif

	return app.Run();
}
