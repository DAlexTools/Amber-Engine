#include "Editor/Shell/ProjectGenerator.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>

namespace AE::Editor
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

    bool WriteTextFile(const std::filesystem::path& path, const std::string& text, std::string* error)
    {
        std::error_code filesystemError;
        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent, filesystemError);
            if (filesystemError)
            {
                SetError(error, "Could not create directory: " + filesystemError.message());
                return false;
            }
        }

        std::ofstream file(path, std::ios::out | std::ios::trunc);
        if (!file)
        {
            SetError(error, "Could not write file: " + path.string());
            return false;
        }

        file << text;
        if (!file)
        {
            SetError(error, "Could not finish writing file: " + path.string());
            return false;
        }
        return true;
    }

    std::string CMakePath(const std::filesystem::path& path)
    {
        return path.generic_string();
    }

    std::string Uppercase(const std::string& value)
    {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });
        return result;
    }

    std::string HeaderGuard(const std::string& moduleName)
    {
        return Uppercase(moduleName) + "_MODULE_H";
    }

    std::string BuildRootCMakeLists(const std::string& moduleName, const std::filesystem::path& engineRoot)
    {
        std::ostringstream text;
        text
            << "cmake_minimum_required(VERSION 3.16)\n\n"
            << "project(" << moduleName << " LANGUAGES CXX)\n\n"
            << "set(CMAKE_CXX_STANDARD 17)\n"
            << "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
            << "set(CMAKE_CXX_EXTENSIONS OFF)\n\n"
            << "set(AMBER_ENGINE_ROOT \"" << CMakePath(engineRoot) << "\" CACHE PATH \"Path to AmberEngine root\")\n"
            << "if(NOT EXISTS \"${AMBER_ENGINE_ROOT}/CMakeLists.txt\")\n"
            << "    message(FATAL_ERROR \"AMBER_ENGINE_ROOT must point to an AmberEngine checkout\")\n"
            << "endif()\n\n"
            << "set(BUILD_UNIT_TESTS OFF CACHE BOOL \"\" FORCE)\n"
            << "set(BUILD_SDL_MODULES ON CACHE BOOL \"\" FORCE)\n"
            << "set(AMBER_BUILD_PROJECTS OFF CACHE BOOL \"\" FORCE)\n\n"
            << "add_subdirectory(\"${AMBER_ENGINE_ROOT}\" \"${CMAKE_BINARY_DIR}/AmberEngine\")\n\n"
            << "add_library(" << moduleName << "Module STATIC\n"
            << "    Source/" << moduleName << "/" << moduleName << "Module.cpp\n"
            << ")\n\n"
            << "set_target_properties(" << moduleName << "Module PROPERTIES POSITION_INDEPENDENT_CODE ON)\n\n"
            << "target_include_directories(" << moduleName << "Module PUBLIC\n"
            << "    Source/" << moduleName << "\n"
            << ")\n\n"
            << "target_link_libraries(" << moduleName << "Module PUBLIC\n"
            << "    AmberBuildConfig\n"
            << "    EngineRuntimeCore\n"
            << "    EngineRuntimeECS\n"
            << "    EngineRuntimeLogging\n"
            << ")\n\n"
            << "add_library(" << moduleName << "ModulePlugin SHARED\n"
            << "    Source/" << moduleName << "/" << moduleName << "ModulePlugin.cpp\n"
            << ")\n\n"
            << "target_link_libraries(" << moduleName << "ModulePlugin PRIVATE " << moduleName << "Module)\n\n"
            << "add_executable(" << moduleName << "Launcher\n"
            << "    Source/" << moduleName << "Launcher/main.cpp\n"
            << ")\n\n"
            << "target_link_libraries(" << moduleName << "Launcher PRIVATE " << moduleName << "Module)\n";
        return text.str();
    }

    std::string BuildCMakePresets(const std::string& moduleName, const std::filesystem::path& engineRoot)
    {
        const std::filesystem::path toolchain = engineRoot / "Dependencies" / "vcpkg" / "scripts" / "buildsystems" / "vcpkg.cmake";
        std::ostringstream text;
        text
            << "{\n"
            << "  \"version\": 4,\n"
            << "  \"configurePresets\": [\n"
            << "    {\n"
            << "      \"name\": \"editor\",\n"
            << "      \"displayName\": \"Editor build\",\n"
            << "      \"generator\": \"Visual Studio 17 2022\",\n"
            << "      \"architecture\": \"x64\",\n"
            << "      \"binaryDir\": \"${sourceDir}/Builds/Editor\",\n"
            << "      \"toolchainFile\": \"" << CMakePath(toolchain) << "\",\n"
            << "      \"cacheVariables\": {\n"
            << "        \"AMBER_ENGINE_ROOT\": \"" << CMakePath(engineRoot) << "\",\n"
            << "        \"AMBER_BUILD_PROJECTS\": \"OFF\",\n"
            << "        \"BUILD_SDL_MODULES\": \"ON\",\n"
            << "        \"BUILD_UNIT_TESTS\": \"OFF\"\n"
            << "      }\n"
            << "    }\n"
            << "  ],\n"
            << "  \"buildPresets\": [\n"
            << "    {\n"
            << "      \"name\": \"editor\",\n"
            << "      \"configurePreset\": \"editor\",\n"
            << "      \"targets\": [\n"
            << "        \"" << moduleName << "Launcher\",\n"
            << "        \"" << moduleName << "ModulePlugin\"\n"
            << "      ]\n"
            << "    }\n"
            << "  ]\n"
            << "}\n";
        return text.str();
    }

    std::string BuildModuleHeader(const std::string& moduleName)
    {
        std::ostringstream text;
        text
            << "#ifndef " << HeaderGuard(moduleName) << "\n"
            << "#define " << HeaderGuard(moduleName) << "\n\n"
            << "#include \"Game/GameModuleInterface.h\"\n\n"
            << "namespace " << moduleName << "\n"
            << "{\n\n"
            << "class " << moduleName << "Module final : public AE::IGameModule\n"
            << "{\n"
            << "public:\n"
            << "    const char* GetName() const override;\n"
            << "    bool StartPlay(const AE::GameModuleStartContext& context, std::string* error) override;\n"
            << "    void Tick(const AE::GameModuleTickContext& context) override;\n"
            << "    void Render(const AE::GameModuleRenderContext& context) override;\n"
            << "    void StopPlay() override;\n\n"
            << "private:\n"
            << "    unsigned long tickCount = 0;\n"
            << "    unsigned long renderCount = 0;\n"
            << "};\n\n"
            << "}\n\n"
            << "#endif\n";
        return text.str();
    }

    std::string BuildModuleSource(const std::string& moduleName)
    {
        std::ostringstream text;
        text
            << "#include \"" << moduleName << "Module.h\"\n\n"
            << "#include \"Logging/Logger.h\"\n\n"
            << "#include <string>\n\n"
            << "namespace " << moduleName << "\n"
            << "{\n\n"
            << "const char* " << moduleName << "Module::GetName() const\n"
            << "{\n"
            << "    return \"" << moduleName << "Module\";\n"
            << "}\n\n"
            << "bool " << moduleName << "Module::StartPlay(const AE::GameModuleStartContext& context, std::string*)\n"
            << "{\n"
            << "    tickCount = 0;\n"
            << "    renderCount = 0;\n"
            << "    AE::Logger::Log(\"" << moduleName << " StartPlay: \" + context.projectName, \"" << moduleName << "\");\n"
            << "    return true;\n"
            << "}\n\n"
            << "void " << moduleName << "Module::Tick(const AE::GameModuleTickContext&)\n"
            << "{\n"
            << "    ++tickCount;\n"
            << "}\n\n"
            << "void " << moduleName << "Module::Render(const AE::GameModuleRenderContext&)\n"
            << "{\n"
            << "    ++renderCount;\n"
            << "}\n\n"
            << "void " << moduleName << "Module::StopPlay()\n"
            << "{\n"
            << "    AE::Logger::Log(\"" << moduleName << " StopPlay ticks=\" + std::to_string(tickCount) + \" renders=\" + std::to_string(renderCount), \"" << moduleName << "\");\n"
            << "}\n\n"
            << "}\n";
        return text.str();
    }

    std::string BuildLauncherSource(const std::string& moduleName)
    {
        std::ostringstream text;
        text
            << "#include \"" << moduleName << "Module.h\"\n\n"
            << "#include \"EntityComponentSystem/ECS.h\"\n"
            << "#include \"Scene/ObjectFactory.h\"\n"
            << "#include \"Scene/SceneAsset.h\"\n\n"
            << "#include <filesystem>\n"
            << "#include <iostream>\n\n"
            << "#include <memory>\n"
            << "#include <string>\n"
            << "#include <vector>\n\n"
            << "int main(int, char**)\n"
            << "{\n"
            << "    " << moduleName << "::" << moduleName << "Module module;\n"
            << "    Registry registry;\n"
            << "    AE::Scene::ObjectFactory objectFactory;\n"
            << "    AE::Scene::Document scene;\n"
            << "    std::vector<std::unique_ptr<AE::Scene::Object>> sceneObjects;\n"
            << "    AE::GameModuleStartContext startContext{\n"
            << "        \"" << moduleName << "\",\n"
            << "        std::filesystem::current_path(),\n"
            << "        {},\n"
            << "        scene,\n"
            << "        registry,\n"
            << "        objectFactory,\n"
            << "        sceneObjects\n"
            << "    };\n"
            << "    std::string error;\n"
            << "    if (!module.StartPlay(startContext, &error))\n"
            << "    {\n"
            << "        std::cerr << \"StartPlay failed: \" << error << std::endl;\n"
            << "        return 1;\n"
            << "    }\n"
            << "    module.Tick(AE::GameModuleTickContext{registry, 1.0f / 60.0f, 0});\n"
            << "    module.Render(AE::GameModuleRenderContext{registry, 0, nullptr});\n"
            << "    std::cout << module.GetName() << \" launcher started.\" << std::endl;\n"
            << "    module.StopPlay();\n"
            << "    return 0;\n"
            << "}\n";
        return text.str();
    }

    std::string BuildModulePluginSource(const std::string& moduleName)
    {
        std::ostringstream text;
        text
            << "#include \"" << moduleName << "Module.h\"\n\n"
            << "#include \"Game/GameModuleInterface.h\"\n\n"
            << "AMBER_GAME_MODULE_EXPORT AE::IGameModule* AmberCreateGameModule()\n"
            << "{\n"
            << "    return new " << moduleName << "::" << moduleName << "Module();\n"
            << "}\n\n"
            << "AMBER_GAME_MODULE_EXPORT void AmberDestroyGameModule(AE::IGameModule* module)\n"
            << "{\n"
            << "    delete module;\n"
            << "}\n";
        return text.str();
    }

    std::string BuildStartupScene(const std::string& projectName)
    {
        std::ostringstream text;
        text
            << "AmberScene 1\n"
            << "name \"" << projectName << " Startup\"\n"
            << "object Camera \"Editor Camera\" \"\" \"Object\" 0.000 0.000 0.000 1.000 1.000 88.000 60.000 1 1\n"
            << "object Grid \"Grid\" \"\" \"Object\" 0.000 0.000 0.000 1.000 1.000 80.000 80.000 1 1\n"
            << "object RuntimeWorld \"Runtime World\" \"\" \"Object\" 180.000 96.000 0.000 1.000 1.000 224.000 144.000 1 1\n";
        return text.str();
    }

    std::string BuildGitIgnore()
    {
        return "Builds/\n.vs/\n*.user\n*.log\n";
    }

    std::string BuildReadme(const std::string& moduleName)
    {
        std::ostringstream text;
        text
            << "# " << moduleName << "\n\n"
            << "Generated AmberEngine game project.\n\n"
            << "Configure:\n\n"
            << "```powershell\n"
            << "cmake --preset editor\n"
            << "```\n\n"
            << "This creates `Builds/Editor/" << moduleName << ".sln`.\n\n"
            << "Build:\n\n"
            << "```powershell\n"
            << "cmake --build --preset editor\n"
            << "```\n";
        return text.str();
    }

    std::string BuildSetupBat()
    {
        return "@echo off\r\ncmake --preset editor\r\ncmake --build --preset editor\r\n";
    }

    bool IsSafePresetName(const std::string& preset)
    {
        if (preset.empty())
        {
            return false;
        }

        for (unsigned char ch : preset)
        {
            if (!std::isalnum(ch) && ch != '_' && ch != '-' && ch != '.')
            {
                return false;
            }
        }
        return true;
    }

#if defined(_WIN32)
    std::wstring WidenAscii(const std::string& value)
    {
        return std::wstring(value.begin(), value.end());
    }

    std::wstring QuoteCommandLineArgument(const std::wstring& value)
    {
        std::wstring quoted = L"\"";
        for (wchar_t ch : value)
        {
            if (ch == L'"')
            {
                quoted += L'\\';
            }
            quoted += ch;
        }
        quoted += L"\"";
        return quoted;
    }

    bool RunCMakeConfigure(
        const std::filesystem::path& projectRoot,
        const std::string& preset,
        int& exitCode,
        std::string* error)
    {
        std::wstring commandLine = L"cmake.exe --preset " + QuoteCommandLineArgument(WidenAscii(preset));
        std::wstring workingDirectory = projectRoot.wstring();

        STARTUPINFOW startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        const BOOL launched = CreateProcessW(
            nullptr,
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
            &startupInfo,
            &processInfo);

        if (!launched)
        {
            SetError(error, "Could not run cmake. Windows error: " + std::to_string(GetLastError()));
            return false;
        }

        CloseHandle(processInfo.hThread);
        const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, INFINITE);
        if (waitResult == WAIT_FAILED)
        {
            const DWORD waitError = GetLastError();
            CloseHandle(processInfo.hProcess);
            SetError(error, "CMake configure wait failed. Windows error: " + std::to_string(waitError));
            return false;
        }

        DWORD processExitCode = 1;
        if (!GetExitCodeProcess(processInfo.hProcess, &processExitCode))
        {
            const DWORD exitError = GetLastError();
            CloseHandle(processInfo.hProcess);
            SetError(error, "Could not read cmake exit code. Windows error: " + std::to_string(exitError));
            return false;
        }

        CloseHandle(processInfo.hProcess);
        exitCode = static_cast<int>(processExitCode);
        return exitCode == 0;
    }
#else
    bool RunCMakeConfigure(
        const std::filesystem::path& projectRoot,
        const std::string& preset,
        int& exitCode,
        std::string* error)
    {
        std::error_code pathError;
        const std::filesystem::path previousDirectory = std::filesystem::current_path(pathError);
        if (pathError)
        {
            SetError(error, "Could not read current directory: " + pathError.message());
            return false;
        }

        std::filesystem::current_path(projectRoot, pathError);
        if (pathError)
        {
            SetError(error, "Could not enter project directory: " + pathError.message());
            return false;
        }

        const std::string command = "cmake --preset " + preset;
        exitCode = std::system(command.c_str());
        std::filesystem::current_path(previousDirectory, pathError);
        return exitCode == 0;
    }
#endif
}

bool ProjectGenerator::CreateProject(const ProjectGenerationRequest& request, ProjectGenerationResult& result, std::string* error)
{
    const std::string moduleName = SanitizeModuleName(request.projectName);
    if (!IsValidModuleName(moduleName))
    {
        SetError(error, "Project name must produce a valid C++ module name.");
        return false;
    }

    if (request.parentDirectory.empty())
    {
        SetError(error, "Project location is empty.");
        return false;
    }

    if (request.engineRoot.empty() || !std::filesystem::exists(request.engineRoot / "CMakeLists.txt"))
    {
        SetError(error, "Engine root is invalid.");
        return false;
    }

    std::error_code filesystemError;
    std::filesystem::create_directories(request.parentDirectory, filesystemError);
    if (filesystemError)
    {
        SetError(error, "Could not create project parent directory: " + filesystemError.message());
        return false;
    }

    const std::filesystem::path projectRoot = request.parentDirectory / moduleName;
    if (std::filesystem::exists(projectRoot, filesystemError) && !std::filesystem::is_empty(projectRoot, filesystemError))
    {
        SetError(error, "Project folder already exists and is not empty: " + projectRoot.string());
        return false;
    }

    std::filesystem::create_directories(projectRoot, filesystemError);
    if (filesystemError)
    {
        SetError(error, "Could not create project folder: " + filesystemError.message());
        return false;
    }

    const std::filesystem::path moduleDirectory = projectRoot / "Source" / moduleName;
    const std::filesystem::path launcherDirectory = projectRoot / "Source" / (moduleName + "Launcher");
    const std::filesystem::path scenesDirectory = projectRoot / "Content" / "Scenes";
    const std::filesystem::path configDirectory = projectRoot / "Config";
    const std::filesystem::path scriptsDirectory = projectRoot / "Scripts";
    const std::filesystem::path testsDirectory = projectRoot / "Tests";

    std::filesystem::create_directories(moduleDirectory, filesystemError);
    std::filesystem::create_directories(launcherDirectory, filesystemError);
    std::filesystem::create_directories(scenesDirectory, filesystemError);
    std::filesystem::create_directories(configDirectory, filesystemError);
    std::filesystem::create_directories(scriptsDirectory, filesystemError);
    std::filesystem::create_directories(testsDirectory, filesystemError);
    if (filesystemError)
    {
        SetError(error, "Could not create project folders: " + filesystemError.message());
        return false;
    }

    const std::filesystem::path descriptorPath = projectRoot / (moduleName + ".amberproject");
    if (!WriteTextFile(projectRoot / "CMakeLists.txt", BuildRootCMakeLists(moduleName, request.engineRoot), error) ||
        !WriteTextFile(projectRoot / "CMakePresets.json", BuildCMakePresets(moduleName, request.engineRoot), error) ||
        !WriteTextFile(projectRoot / ".gitignore", BuildGitIgnore(), error) ||
        !WriteTextFile(projectRoot / "README.md", BuildReadme(moduleName), error) ||
        !WriteTextFile(projectRoot / "Setup.bat", BuildSetupBat(), error) ||
        !WriteTextFile(moduleDirectory / (moduleName + "Module.h"), BuildModuleHeader(moduleName), error) ||
        !WriteTextFile(moduleDirectory / (moduleName + "Module.cpp"), BuildModuleSource(moduleName), error) ||
        !WriteTextFile(moduleDirectory / (moduleName + "ModulePlugin.cpp"), BuildModulePluginSource(moduleName), error) ||
        !WriteTextFile(launcherDirectory / "main.cpp", BuildLauncherSource(moduleName), error) ||
        !WriteTextFile(scenesDirectory / "Startup.amber.scene", BuildStartupScene(moduleName), error) ||
        !WriteTextFile(configDirectory / "DefaultGame.ini", "[Project]\nName=" + moduleName + "\n", error))
    {
        return false;
    }

    ProjectDescriptor descriptor;
    descriptor.name = moduleName;
    descriptor.projectFilePath = descriptorPath;
    descriptor.projectRoot = projectRoot;
    descriptor.engineRoot = request.engineRoot;
    descriptor.gameModuleTarget = moduleName + "Module";
    descriptor.playTarget = moduleName + "Launcher";
    descriptor.startupScene = std::filesystem::path("Content") / "Scenes" / "Startup.amber.scene";
    descriptor.contentRoot = "Content";
    descriptor.buildPreset = "editor";
    descriptor.solutionPath = std::filesystem::path("Builds") / "Editor" / (moduleName + ".sln");

    if (!SaveProjectDescriptor(descriptor, descriptorPath, error))
    {
        return false;
    }

    result.descriptor = descriptor;
    result.projectRoot = projectRoot;
    result.projectFilePath = descriptorPath;
    result.expectedSolutionPath = GetExpectedSolutionPath(descriptor);
    result.configureCommand = GetConfigureCommand(descriptor);
    return true;
}

bool ProjectGenerator::ConfigureProject(const ProjectDescriptor& descriptor, ProjectConfigureResult& result, std::string* error)
{
    result = {};
    result.projectRoot = descriptor.projectRoot;
    result.expectedSolutionPath = GetExpectedSolutionPath(descriptor);
    result.configureCommand = GetConfigureCommand(descriptor);

    if (descriptor.projectRoot.empty())
    {
        SetError(error, "Project root is empty.");
        return false;
    }

    std::error_code filesystemError;
    if (!std::filesystem::exists(descriptor.projectRoot / "CMakeLists.txt", filesystemError))
    {
        SetError(error, "Project CMakeLists.txt was not found: " + descriptor.projectRoot.string());
        return false;
    }

    const std::string preset = descriptor.buildPreset.empty() ? std::string("editor") : descriptor.buildPreset;
    if (!IsSafePresetName(preset))
    {
        SetError(error, "Project build preset contains unsupported characters.");
        return false;
    }

    if (!RunCMakeConfigure(descriptor.projectRoot, preset, result.exitCode, error))
    {
        if (error && error->empty())
        {
            *error = "CMake configure failed with exit code " + std::to_string(result.exitCode) + ".";
        }
        return false;
    }

    if (!result.expectedSolutionPath.empty() &&
        !std::filesystem::exists(result.expectedSolutionPath, filesystemError))
    {
        SetError(error, "CMake completed, but solution was not found: " + result.expectedSolutionPath.string());
        return false;
    }

    return true;
}

std::filesystem::path ProjectGenerator::GetExpectedSolutionPath(const ProjectDescriptor& descriptor)
{
    if (!descriptor.solutionPath.empty())
    {
        return descriptor.ResolveProjectPath(descriptor.solutionPath);
    }

    if (descriptor.projectRoot.empty() || descriptor.name.empty())
    {
        return {};
    }

    return descriptor.projectRoot / "Builds" / "Editor" / (descriptor.name + ".sln");
}

std::string ProjectGenerator::GetConfigureCommand(const ProjectDescriptor& descriptor)
{
    const std::string preset = descriptor.buildPreset.empty() ? std::string("editor") : descriptor.buildPreset;
    return "cmake --preset " + preset;
}

std::string ProjectGenerator::SanitizeModuleName(const std::string& name)
{
    std::string result;
    bool capitalizeNext = true;
    for (unsigned char ch : name)
    {
        if (std::isalnum(ch))
        {
            char output = static_cast<char>(ch);
            if (capitalizeNext)
            {
                output = static_cast<char>(std::toupper(ch));
                capitalizeNext = false;
            }
            result.push_back(output);
        }
        else
        {
            capitalizeNext = true;
        }
    }

    if (result.empty())
    {
        return {};
    }
    if (std::isdigit(static_cast<unsigned char>(result.front())))
    {
        result.insert(result.begin(), 'G');
    }
    return result;
}

bool ProjectGenerator::IsValidModuleName(const std::string& name)
{
    if (name.empty())
    {
        return false;
    }
    const unsigned char first = static_cast<unsigned char>(name.front());
    if (!std::isalpha(first) && name.front() != '_')
    {
        return false;
    }
    for (unsigned char ch : name)
    {
        if (!std::isalnum(ch) && ch != '_')
        {
            return false;
        }
    }
    return true;
}

}
