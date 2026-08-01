#ifndef AMBER_EDITOR_PROJECT_GENERATOR_H
#define AMBER_EDITOR_PROJECT_GENERATOR_H

#include "Project/ProjectDescriptor.h"

#include <filesystem>
#include <string>

namespace AE::Editor
{

enum class ProjectTemplate
{
	BlankCppGame
};

struct ProjectGenerationRequest
{
	std::string projectName;
	std::filesystem::path parentDirectory;
	std::filesystem::path engineRoot;
	ProjectTemplate projectTemplate = ProjectTemplate::BlankCppGame;
};

struct ProjectGenerationResult
{
	ProjectDescriptor descriptor;
	std::filesystem::path projectRoot;
	std::filesystem::path projectFilePath;
	std::filesystem::path expectedSolutionPath;
	std::string configureCommand;
};

struct ProjectConfigureResult
{
	std::filesystem::path projectRoot;
	std::filesystem::path expectedSolutionPath;
	std::string configureCommand;
	int exitCode = -1;
};

class ProjectGenerator
{
public:
	static bool CreateProject(const ProjectGenerationRequest& request, ProjectGenerationResult& result, std::string* error = nullptr);
	static bool ConfigureProject(const ProjectDescriptor& descriptor, ProjectConfigureResult& result, std::string* error = nullptr);
	static std::filesystem::path GetExpectedSolutionPath(const ProjectDescriptor& descriptor);
	static std::string GetConfigureCommand(const ProjectDescriptor& descriptor);
	static std::string SanitizeModuleName(const std::string& name);
	static bool IsValidModuleName(const std::string& name);
};

} // namespace AE::Editor

#endif
