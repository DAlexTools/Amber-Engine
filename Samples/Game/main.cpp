#include "Classes/Engine.h"
#include "Core/BuildConfig.h"
#include "Game/GameModule.h"
#include <iostream>
#include <string>

namespace
{
	int ParseLevelNumber(const std::string& value)
	{
		try
		{
			const int parsedLevel = std::stoi(value);
			return parsedLevel > 0 ? parsedLevel : 1;
		}
		catch (...)
		{
			return 1;
		}
	}
}

int main(int argc, char* argv[])
{
	bool debugPhysics = false;
	bool fullscreen = true;
	int levelNumber = 1;
#if SMOKE_TEST
	bool smokeTest = false;
	bool physicsContactSmokeTest = false;
	bool physicsObstacleSmokeTest = false;
#endif

	for (int i = 1; i < argc; ++i)
	{
		const std::string argument = argv[i];
		bool handledArgument = false;
#if SMOKE_TEST
		if (argument == "--smoke-test")
		{
			smokeTest = true;
			handledArgument = true;
		}
		else if (argument == "--physics-contact-smoke-test")
		{
			physicsContactSmokeTest = true;
			handledArgument = true;
		}
		else if (argument == "--physics-obstacle-smoke-test")
		{
			physicsObstacleSmokeTest = true;
			handledArgument = true;
		}
#endif
		if (handledArgument)
		{
			continue;
		}

		if (argument == "--debug-physics")
		{
			debugPhysics = true;
		}
		else if (argument == "--fullscreen")
		{
			fullscreen = true;
		}
		else if (argument == "--windowed")
		{
			fullscreen = false;
		}
		else if (argument == "--level" && i + 1 < argc)
		{
			levelNumber = ParseLevelNumber(argv[++i]);
		}
		else if (argument.rfind("--level=", 0) == 0)
		{
			levelNumber = ParseLevelNumber(argument.substr(8));
		}
	}

	AE::Engine engine;
	GameModule gameModule(engine);

	AE::EngineConfig engineConfig;
	engineConfig.startFullscreen = fullscreen;

	if (!engine.Initialize(engineConfig))
	{
		return 1;
	}

	gameModule.SetDebugEnabled(debugPhysics);
	gameModule.SetLevelNumber(levelNumber);

#if SMOKE_TEST
	if (physicsContactSmokeTest)
	{
		const bool passed = gameModule.RunPhysicsContactSmokeTest();
		engine.Shutdown();

		if (!passed)
		{
			std::cerr << "Physics contact smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Physics contact smoke test passed." << std::endl;
		return 0;
	}
#endif

#if SMOKE_TEST
	if (physicsObstacleSmokeTest)
	{
		const bool passed = gameModule.RunPhysicsObstacleSmokeTest();
		engine.Shutdown();

		if (!passed)
		{
			std::cerr << "Physics obstacle smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Physics obstacle smoke test passed." << std::endl;
		return 0;
	}
#endif

#if SMOKE_TEST
	if (smokeTest)
	{
		gameModule.Setup();
		gameModule.Update();
		gameModule.Render();
		engine.Shutdown();
		return 0;
	}
#endif

	gameModule.Run();
	engine.Shutdown();

	return 0;
}
