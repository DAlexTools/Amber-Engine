#ifndef AMBER_RUNTIME_PROJECT_PROJECT_DESCRIPTOR_H
#define AMBER_RUNTIME_PROJECT_PROJECT_DESCRIPTOR_H

#include <filesystem>
#include <string>

namespace AE
{

struct ProjectDescriptor
{
	std::string name;
	std::filesystem::path projectFilePath;
	std::filesystem::path projectRoot;
	std::filesystem::path engineRoot;
	std::string gameModuleTarget;
	std::string playTarget;
	std::filesystem::path startupScene;
	std::filesystem::path actorTypes;
	std::filesystem::path contentRoot;
	std::string buildPreset;
	std::filesystem::path solutionPath;

	std::filesystem::path ResolveProjectPath(const std::filesystem::path& path) const;
};

bool SaveProjectDescriptor(const ProjectDescriptor& descriptor, const std::filesystem::path& path, std::string* error = nullptr);
bool LoadProjectDescriptor(const std::filesystem::path& path, ProjectDescriptor& descriptor, std::string* error = nullptr);

} // namespace AE

#endif
