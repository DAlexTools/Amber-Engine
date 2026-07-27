#include "PlatformerApp.h"

#include "Core/BuildConfig.h"
#include "Project/ProjectDescriptor.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <string>

namespace
{
    struct LaunchOptions
    {
#if SMOKE_TEST
        bool smokeTest = false;
#endif
        bool showHelp = false;
        std::filesystem::path projectPath;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
        std::filesystem::path scenePath;
#endif
    };

    void PrintUsage()
    {
        std::cout
            << "Usage:\n"
            << "  PlatformerApp.exe [Platformer.amberproject]\n"
            << "  PlatformerApp.exe --project Platformer.amberproject [--scene Scene.amber.scene]\n"
            << "  PlatformerApp.exe --smoke-test [--project Platformer.amberproject]\n";
    }

    bool IsProjectFile(const std::filesystem::path& path)
    {
        return path.extension() == ".amberproject";
    }

    std::filesystem::path CanonicalIfPossible(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
        return canonical.empty() ? path : canonical;
    }

    bool TryFindProjectFromRoot(const std::filesystem::path& start, std::filesystem::path& result)
    {
        namespace fs = std::filesystem;

        std::error_code error;
        fs::path current = fs::weakly_canonical(start, error);
        if (current.empty())
        {
            current = start;
        }
        if (!current.empty() && fs::is_regular_file(current, error))
        {
            current = current.parent_path();
        }

        for (int depth = 0; depth < 10 && !current.empty(); ++depth)
        {
            const std::array<fs::path, 2> candidates = {
                current / "Platformer.amberproject",
                current / "Projects" / "Platformer" / "Platformer.amberproject"
            };

            for (const fs::path& candidate : candidates)
            {
                if (fs::exists(candidate, error) && fs::is_regular_file(candidate, error))
                {
                    result = CanonicalIfPossible(candidate);
                    return true;
                }
            }

            if (!current.has_parent_path() || current.parent_path() == current)
            {
                break;
            }
            current = current.parent_path();
        }

        return false;
    }

    std::filesystem::path FindDefaultPlatformerProject(const char* executablePath)
    {
        std::filesystem::path projectPath;
        if (executablePath && TryFindProjectFromRoot(executablePath, projectPath))
        {
            return projectPath;
        }
        if (TryFindProjectFromRoot(std::filesystem::current_path(), projectPath))
        {
            return projectPath;
        }
        return {};
    }

    bool ParseLaunchOptions(int argc, char* argv[], LaunchOptions& options)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument == "--help" || argument == "-h")
            {
                options.showHelp = true;
            }
#if SMOKE_TEST
            else if (argument == "--smoke-test")
            {
                options.smokeTest = true;
            }
#endif
            else if (argument == "--project")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "--project requires a .amberproject path." << std::endl;
                    return false;
                }
                options.projectPath = argv[++i];
            }
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
            else if (argument == "--scene")
            {
                if (i + 1 >= argc)
                {
                    std::cerr << "--scene requires a .amber.scene path." << std::endl;
                    return false;
                }
                options.scenePath = argv[++i];
            }
#endif
            else if (IsProjectFile(argument))
            {
                options.projectPath = argument;
            }
            else
            {
                std::cerr << "Unknown argument: " << argument << std::endl;
                return false;
            }
        }

        return true;
    }
}

int main(int argc, char* argv[])
{
    LaunchOptions options;
    if (!ParseLaunchOptions(argc, argv, options))
    {
        PrintUsage();
        return 1;
    }

    if (options.showHelp)
    {
        PrintUsage();
        return 0;
    }

    AE::ProjectDescriptor project;
    bool projectLoaded = false;
    if (options.projectPath.empty())
    {
        options.projectPath = FindDefaultPlatformerProject(argc > 0 ? argv[0] : nullptr);
    }

    if (!options.projectPath.empty())
    {
        std::string error;
        if (!AE::LoadProjectDescriptor(options.projectPath, project, &error))
        {
            std::cerr << "Project open failed: " << error << std::endl;
            return 1;
        }
        projectLoaded = true;

        std::error_code currentPathError;
        std::filesystem::current_path(project.projectRoot, currentPathError);

#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
        if (options.scenePath.empty())
        {
            options.scenePath = project.ResolveProjectPath(project.startupScene);
        }
        else if (options.scenePath.is_relative())
        {
            options.scenePath = project.ResolveProjectPath(options.scenePath);
        }
#endif
    }

    PlatformerApp app;
#if AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
    if (!options.scenePath.empty())
    {
        app.SetEditorScenePath(CanonicalIfPossible(options.scenePath));
    }
#endif
#if SMOKE_TEST
    if (options.smokeTest)
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

    if (projectLoaded)
    {
        std::cout << "Opened Amber project: " << project.name << std::endl;
    }
    return app.Run();
}
