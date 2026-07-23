#include "Game/Game.h"
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
	bool smokeTest = false;
	bool physicsContactSmokeTest = false;
	bool physicsObstacleSmokeTest = false;
	bool debugPhysics = false;
	bool fullscreen = true;
	int levelNumber = 1;

	for (int i = 1; i < argc; ++i)
	{
		const std::string argument = argv[i];
		if (argument == "--smoke-test")
		{
			smokeTest = true;
		}
		else if (argument == "--physics-contact-smoke-test")
		{
			physicsContactSmokeTest = true;
		}
		else if (argument == "--physics-obstacle-smoke-test")
		{
			physicsObstacleSmokeTest = true;
		}
		else if (argument == "--debug-physics")
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

	Game game;
	game.SetFullscreenEnabled(fullscreen);
	
	game.Initialize();
	if (!game.IsRunning())
	{
		return 1;
	}

	game.SetDebugEnabled(debugPhysics);
	game.SetLevelNumber(levelNumber);

	if (physicsContactSmokeTest)
	{
		const bool passed = game.RunPhysicsContactSmokeTest();
		game.Destroy();

		if (!passed)
		{
			std::cerr << "Physics contact smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Physics contact smoke test passed." << std::endl;
		return 0;
	}

	if (physicsObstacleSmokeTest)
	{
		const bool passed = game.RunPhysicsObstacleSmokeTest();
		game.Destroy();

		if (!passed)
		{
			std::cerr << "Physics obstacle smoke test failed." << std::endl;
			return 1;
		}

		std::cout << "Physics obstacle smoke test passed." << std::endl;
		return 0;
	}

	if (smokeTest)
	{
		game.Setup();
		game.Update();
		game.Render();
		game.Destroy();
		return 0;
	}

	game.Run();
	game.Destroy();

	return 0;
}
