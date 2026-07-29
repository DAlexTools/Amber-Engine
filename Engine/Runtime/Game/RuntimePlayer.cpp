#include "Game/RuntimePlayer.h"

#include "Game/RuntimeViewerSDL.h"
#include "Game/RuntimeWorld.h"
#include "Logging/Logger.h"
#include "Project/ProjectDescriptor.h"
#include "Scene/SceneAsset.h"

#include <SDL2/SDL.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace AE
{
namespace
{
void SetError(std::string* error, const std::string& message)
{
	if (error)
	{
		*error = message;
	}
}

bool HasAmberProjectExtension(const std::filesystem::path& path)
{
	return path.extension() == ".amberproject";
}

bool FileExists(const std::filesystem::path& path)
{
	std::error_code error;
	return std::filesystem::exists(path, error) && std::filesystem::is_regular_file(path, error);
}

std::filesystem::path WeakCanonicalIfPossible(const std::filesystem::path& path)
{
	std::error_code error;
	const std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
	return canonical.empty() ? path : canonical;
}

std::filesystem::path ResolveProjectFilePath(const std::filesystem::path& requested)
{
	if (requested.empty())
	{
		return {};
	}

	if (requested.is_absolute() && FileExists(requested))
	{
		return WeakCanonicalIfPossible(requested);
	}

	std::vector<std::filesystem::path> roots;
	std::error_code error;
	roots.push_back(std::filesystem::current_path(error));

	std::filesystem::path cursor = roots.front();
	for (int i = 0; i < 8 && !cursor.empty(); ++i)
	{
		roots.push_back(cursor.parent_path());
		if (cursor == cursor.parent_path())
		{
			break;
		}
		cursor = cursor.parent_path();
	}

	for (const std::filesystem::path& root : roots)
	{
		if (root.empty())
		{
			continue;
		}

		const std::filesystem::path candidate = root / requested;
		if (FileExists(candidate))
		{
			return WeakCanonicalIfPossible(candidate);
		}
	}

	if (!requested.has_parent_path())
	{
		for (const std::filesystem::path& root : roots)
		{
			if (root.empty())
			{
				continue;
			}

			std::error_code directoryError;
			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, directoryError))
			{
				if (directoryError)
				{
					break;
				}
				if (entry.is_regular_file(directoryError) && entry.path().filename() == requested.filename())
				{
					return WeakCanonicalIfPossible(entry.path());
				}
			}
		}
	}

	return requested;
}

std::filesystem::path ResolveScenePath(const ProjectDescriptor& descriptor, const std::filesystem::path& sceneOverride)
{
	if (!sceneOverride.empty())
	{
		return sceneOverride.is_absolute() ? sceneOverride : descriptor.ResolveProjectPath(sceneOverride);
	}
	return descriptor.ResolveProjectPath(descriptor.startupScene);
}

bool ParsePositiveInt(const char* value, int& output)
{
	if (!value)
	{
		return false;
	}

	char* end = nullptr;
	const long parsed = std::strtol(value, &end, 10);
	if (!end || *end != '\0' || parsed <= 0 || parsed > 100000)
	{
		return false;
	}

	output = static_cast<int>(parsed);
	return true;
}

bool ParsePositiveUInt64(const char* value, uint64& output)
{
	if (!value)
	{
		return false;
	}

	if (*value == '\0')
	{
		return false;
	}

	uint64 parsed = 0;
	for (const char* cursor = value; *cursor != '\0'; ++cursor)
	{
		if (*cursor < '0' || *cursor > '9')
		{
			return false;
		}

		const uint64 digit = static_cast<uint64>(*cursor - '0');
		if (parsed > (std::numeric_limits<uint64>::max() - digit) / 10)
		{
			return false;
		}
		parsed = parsed * 10 + digit;
	}

	if (parsed == 0)
	{
		return false;
	}
	output = static_cast<uint64>(parsed);
	return true;
}

void PrintUsage(const char* executableName)
{
	std::cout
		<< "Usage:\n"
		<< "  " << executableName << " [Project.amberproject]\n"
		<< "  " << executableName << " --project Project.amberproject [--scene Scene.amber.scene]\n"
		<< "  " << executableName << " --smoke-test [--frames N]\n";
}
} // namespace

int RuntimePlayer::Run(IGameModule& module, const RuntimePlayerOptions& options, std::string* error)
{
	const std::filesystem::path projectPath = ResolveProjectFilePath(options.projectFilePath);
	if (projectPath.empty() || !FileExists(projectPath))
	{
		SetError(error, "Project file was not found: " + options.projectFilePath.string());
		return 1;
	}

	ProjectDescriptor descriptor;
	if (!LoadProjectDescriptor(projectPath, descriptor, error))
	{
		return 1;
	}

	const std::filesystem::path scenePath = ResolveScenePath(descriptor, options.sceneOverride);
	Scene::Document scene;
	if (!Scene::LoadScene(scenePath, scene, error))
	{
		return 1;
	}

	RuntimeWorld world;
	if (!BuildRuntimeWorld(scene, &module, world, RuntimeWorldBuildOptions{}, error))
	{
		return 1;
	}
	Registry& registry = world.GetRegistry();
	Scene::ObjectFactory& objectFactory = world.GetObjectFactory();
	std::vector<std::unique_ptr<Scene::Object>>& sceneObjects = world.GetSceneObjects();

	RuntimeViewerSDL viewer;
	RuntimeViewerSDLConfig viewerConfig;
	viewerConfig.windowTitle = options.windowTitle.empty() ? descriptor.name : options.windowTitle;
	viewerConfig.width = options.windowWidth;
	viewerConfig.height = options.windowHeight;
	if (!viewer.Initialize(viewerConfig, error))
	{
		return 1;
	}

	bool moduleStarted = false;
	std::string startError;
	GameModuleStartContext startContext{
		descriptor.name,
		descriptor.projectRoot,
		scenePath,
		scene,
		registry,
		objectFactory,
		sceneObjects};

	if (!module.StartPlay(startContext, &startError))
	{
		SetError(error, startError.empty() ? "Game module failed to start." : startError);
		viewer.Shutdown();
		return 1;
	}
	moduleStarted = true;

	const RuntimeSceneRendererConfig rendererConfig = BuildRuntimeSceneRendererConfig(descriptor, scene);
	const Uint64 frequency = SDL_GetPerformanceFrequency();
	Uint64 previousCounter = SDL_GetPerformanceCounter();
	uint64 frame = 0;
	bool running = true;

	while (running)
	{
		running = viewer.PollEvents();

		const Uint64 currentCounter = SDL_GetPerformanceCounter();
		const float deltaSeconds = frequency > 0 ? static_cast<float>(static_cast<double>(currentCounter - previousCounter) / static_cast<double>(frequency)) : 1.0f / 60.0f;
		previousCounter = currentCounter;

		module.Tick(GameModuleTickContext{registry, deltaSeconds, frame});
		registry.Update();

		viewer.BeginFrame();
		viewer.RenderWorld(registry, scene, rendererConfig);
		RuntimeRenderContextSDL renderContext = viewer.MakeRenderContext(descriptor, scene);
		module.Render(GameModuleRenderContext{registry, frame, &renderContext});
		viewer.Present();

		++frame;
		if (options.smokeTest && frame >= options.smokeFrameCount)
		{
			running = false;
		}
	}

	if (moduleStarted)
	{
		module.StopPlay();
	}

	world.DestroyObjects();

	viewer.Shutdown();
	return 0;
}

int RuntimePlayer::RunFromArguments(
	IGameModule& module,
	int argc,
	char** argv,
	const RuntimePlayerOptions& defaults)
{
	RuntimePlayerOptions options = defaults;
	const char* executableName = argc > 0 && argv && argv[0] ? argv[0] : "RuntimePlayer";

	for (int index = 1; index < argc; ++index)
	{
		const std::string argument = argv[index] ? argv[index] : "";
		if (argument == "--help" || argument == "-h")
		{
			PrintUsage(executableName);
			return 0;
		}
		if (argument == "--project" && index + 1 < argc)
		{
			options.projectFilePath = argv[++index];
		}
		else if (argument == "--scene" && index + 1 < argc)
		{
			options.sceneOverride = argv[++index];
		}
		else if (argument == "--smoke-test")
		{
			options.smokeTest = true;
		}
		else if (argument == "--frames" && index + 1 < argc)
		{
			if (!ParsePositiveUInt64(argv[++index], options.smokeFrameCount))
			{
				std::cerr << "Invalid --frames value." << std::endl;
				return 1;
			}
		}
		else if (argument == "--width" && index + 1 < argc)
		{
			if (!ParsePositiveInt(argv[++index], options.windowWidth))
			{
				std::cerr << "Invalid --width value." << std::endl;
				return 1;
			}
		}
		else if (argument == "--height" && index + 1 < argc)
		{
			if (!ParsePositiveInt(argv[++index], options.windowHeight))
			{
				std::cerr << "Invalid --height value." << std::endl;
				return 1;
			}
		}
		else if (!argument.empty() && argument[0] != '-' && HasAmberProjectExtension(argument))
		{
			options.projectFilePath = argument;
		}
		else
		{
			std::cerr << "Unknown argument: " << argument << std::endl;
			PrintUsage(executableName);
			return 1;
		}
	}

	RuntimePlayer player;
	std::string error;
	const int exitCode = player.Run(module, options, &error);
	if (exitCode != 0 && !error.empty())
	{
		std::cerr << error << std::endl;
	}
	return exitCode;
}

} // namespace AE
