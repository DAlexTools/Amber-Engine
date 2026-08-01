#include "PlatformerGameModule.h"

#include "Game/RuntimePlayer.h"

#include <cstring>
#include <iostream>

int main(int argc, char** argv)
{
	PlatformerGameModule module;

#if SMOKE_TEST
	for (int index = 1; index < argc; ++index)
	{
		if (argv[index] && std::strcmp(argv[index], "--gameplay-smoke-test") == 0)
		{
			if (!module.RunSmokeTest())
			{
				std::cerr << "Platformer gameplay smoke test failed." << std::endl;
				return 1;
			}

			std::cout << "Platformer gameplay smoke test passed." << std::endl;
			return 0;
		}
	}
#endif

	AE::RuntimePlayerOptions options;
	options.projectFilePath = "Platformer.amberproject";
	options.windowTitle = "Platformer";
	options.windowWidth = 960;
	options.windowHeight = 540;
	options.smokeFrameCount = 2;
	return AE::RuntimePlayer::RunFromArguments(module, argc, argv, options);
}
