#include "Project/ProjectDescriptor.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

namespace AE
{
namespace
{
    constexpr const char* ProjectMagic = "AmberProject";
    constexpr int ProjectVersion = 1;

    void SetError(std::string* error, const std::string& message)
    {
        if (error)
        {
            *error = message;
        }
    }

    std::string GenericPathString(const std::filesystem::path& path)
    {
        return path.generic_string();
    }

    std::filesystem::path CanonicalIfPossible(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
        return canonical.empty() ? path : canonical;
    }
}

std::filesystem::path ProjectDescriptor::ResolveProjectPath(const std::filesystem::path& path) const
{
    if (path.empty() || path.is_absolute())
    {
        return path;
    }
    return projectRoot / path;
}

bool SaveProjectDescriptor(const ProjectDescriptor& descriptor, const std::filesystem::path& path, std::string* error)
{
    std::error_code filesystemError;
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError)
        {
            SetError(error, "Could not create project directory: " + filesystemError.message());
            return false;
        }
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file)
    {
        SetError(error, "Could not open project descriptor for writing: " + path.string());
        return false;
    }

    file << ProjectMagic << ' ' << ProjectVersion << '\n';
    file << "name " << std::quoted(descriptor.name) << '\n';
    file << "engineRoot " << std::quoted(GenericPathString(descriptor.engineRoot)) << '\n';
    file << "gameModuleTarget " << std::quoted(descriptor.gameModuleTarget) << '\n';
    file << "playTarget " << std::quoted(descriptor.playTarget) << '\n';
    file << "startupScene " << std::quoted(GenericPathString(descriptor.startupScene)) << '\n';
    file << "contentRoot " << std::quoted(GenericPathString(descriptor.contentRoot)) << '\n';
    file << "buildPreset " << std::quoted(descriptor.buildPreset) << '\n';
    file << "solutionPath " << std::quoted(GenericPathString(descriptor.solutionPath)) << '\n';

    if (!file)
    {
        SetError(error, "Could not finish writing project descriptor: " + path.string());
        return false;
    }

    return true;
}

bool LoadProjectDescriptor(const std::filesystem::path& path, ProjectDescriptor& descriptor, std::string* error)
{
    std::ifstream file(path);
    if (!file)
    {
        SetError(error, "Could not open project descriptor: " + path.string());
        return false;
    }

    std::string magic;
    int version = 0;
    file >> magic >> version;
    if (magic != ProjectMagic || version != ProjectVersion)
    {
        SetError(error, "Unsupported project descriptor header: " + path.string());
        return false;
    }

    ProjectDescriptor loaded;
    loaded.projectFilePath = CanonicalIfPossible(path);
    loaded.projectRoot = loaded.projectFilePath.parent_path();
    loaded.startupScene = std::filesystem::path("Content") / "Scenes" / "Startup.amber.scene";
    loaded.contentRoot = "Content";
    loaded.buildPreset = "editor";

    std::string line;
    std::getline(file, line);
    std::size_t lineNumber = 1;
    while (std::getline(file, line))
    {
        ++lineNumber;
        if (line.empty())
        {
            continue;
        }

        std::istringstream stream(line);
        std::string key;
        std::string value;
        stream >> key >> std::quoted(value);
        if (!stream)
        {
            SetError(error, "Invalid project descriptor line " + std::to_string(lineNumber));
            return false;
        }

        if (key == "name")
        {
            loaded.name = value;
        }
        else if (key == "engineRoot")
        {
            const std::filesystem::path engineRoot = value;
            loaded.engineRoot = engineRoot.is_absolute() ? engineRoot : loaded.projectRoot / engineRoot;
        }
        else if (key == "gameModuleTarget")
        {
            loaded.gameModuleTarget = value;
        }
        else if (key == "playTarget")
        {
            loaded.playTarget = value;
        }
        else if (key == "startupScene")
        {
            loaded.startupScene = value;
        }
        else if (key == "contentRoot")
        {
            loaded.contentRoot = value;
        }
        else if (key == "buildPreset")
        {
            loaded.buildPreset = value;
        }
        else if (key == "solutionPath")
        {
            loaded.solutionPath = value;
        }
    }

    if (loaded.name.empty())
    {
        SetError(error, "Project descriptor is missing a project name.");
        return false;
    }
    if (loaded.engineRoot.empty())
    {
        SetError(error, "Project descriptor is missing engineRoot.");
        return false;
    }
    if (loaded.gameModuleTarget.empty())
    {
        SetError(error, "Project descriptor is missing gameModuleTarget.");
        return false;
    }
    if (loaded.playTarget.empty())
    {
        loaded.playTarget = loaded.name + "Launcher";
    }

    loaded.engineRoot = CanonicalIfPossible(loaded.engineRoot);
    descriptor = std::move(loaded);
    return true;
}

}
