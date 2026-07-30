#include "GameModuleResolver.h"

#include "Logging/LogBus.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <algorithm>
#include <array>
#include <system_error>
#include <utility>

namespace AE::Editor
{
namespace
{
    class EditorScenePreviewGameModule final : public AE::IGameModule
    {
    public:
        explicit EditorScenePreviewGameModule(std::string targetName)
            : requestedTarget(std::move(targetName))
        {
        }

        const char* GetName() const override
        {
            return "EditorScenePreviewGameModule";
        }

        bool StartPlay(const AE::GameModuleStartContext& context, std::string*) override
        {
            LogBus::Add(
                LogLevel::Info,
                "Editor",
                "Game module started: EditorScenePreviewGameModule. Requested target: " +
                    (requestedTarget.empty() ? std::string("<none>") : requestedTarget) +
                    ". Scene objects: " + std::to_string(context.sceneObjects.size()));
            return true;
        }

        void StopPlay() override
        {
            LogBus::Add(LogLevel::Info, "Editor", "Game module stopped: EditorScenePreviewGameModule");
        }

    private:
        std::string requestedTarget;
    };

    std::filesystem::path ModuleFileName(const std::string& target)
    {
        std::filesystem::path fileName = std::filesystem::path(target).filename();
#if defined(_WIN32)
        if (fileName.extension().empty())
        {
            fileName += ".dll";
        }
#elif defined(__APPLE__)
        if (fileName.extension().empty())
        {
            fileName = "lib" + fileName.string() + ".dylib";
        }
#else
        if (fileName.extension().empty())
        {
            fileName = "lib" + fileName.string() + ".so";
        }
#endif
        return fileName;
    }

    std::vector<std::string> ModuleTargetNames(const std::string& gameModuleTarget)
    {
        std::vector<std::string> names;
        if (gameModuleTarget.empty())
        {
            return names;
        }

        names.push_back(gameModuleTarget);
        if (gameModuleTarget.size() < 6 ||
            gameModuleTarget.compare(gameModuleTarget.size() - 6, 6, "Plugin") != 0)
        {
            names.push_back(gameModuleTarget + "Plugin");
        }
        return names;
    }

    std::filesystem::path CanonicalIfPossible(const std::filesystem::path& path)
    {
        std::error_code error;
        std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
        return canonical.empty() ? path : canonical;
    }

    void UnloadLibraryHandle(void* library)
    {
        if (!library)
        {
            return;
        }
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(library));
#else
        dlclose(library);
#endif
    }

    void* LoadLibraryHandle(const std::filesystem::path& path, std::string* error)
    {
#if defined(_WIN32)
        HMODULE library = LoadLibraryExW(path.wstring().c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!library && error)
        {
            *error = "Windows error " + std::to_string(GetLastError());
        }
        return static_cast<void*>(library);
#else
        void* library = dlopen(path.string().c_str(), RTLD_NOW);
        if (!library && error)
        {
            const char* loadError = dlerror();
            *error = loadError ? loadError : "unknown dlopen error";
        }
        return library;
#endif
    }

    void* LoadSymbol(void* library, const char* symbolName)
    {
        if (!library)
        {
            return nullptr;
        }
#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), symbolName));
#else
        return dlsym(library, symbolName);
#endif
    }
}

LoadedGameModule::LoadedGameModule(std::unique_ptr<AE::IGameModule> fallbackModule)
    : fallback(std::move(fallbackModule))
{
}

LoadedGameModule::LoadedGameModule(
    void* loadedLibrary,
    AE::IGameModule* loadedModule,
    AE::DestroyGameModuleFunction destroyFunction,
    std::filesystem::path loadedLibraryPath)
    : library(loadedLibrary)
    , module(loadedModule)
    , destroy(destroyFunction)
    , libraryPath(std::move(loadedLibraryPath))
{
}

LoadedGameModule::~LoadedGameModule()
{
    Reset();
}

LoadedGameModule::LoadedGameModule(LoadedGameModule&& other) noexcept
{
    *this = std::move(other);
}

LoadedGameModule& LoadedGameModule::operator=(LoadedGameModule&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Reset();
    library = other.library;
    module = other.module;
    destroy = other.destroy;
    fallback = std::move(other.fallback);
    libraryPath = std::move(other.libraryPath);
    other.library = nullptr;
    other.module = nullptr;
    other.destroy = nullptr;
    return *this;
}

AE::IGameModule* LoadedGameModule::Get() const
{
    return fallback ? fallback.get() : module;
}

const std::filesystem::path& LoadedGameModule::GetLibraryPath() const
{
    return libraryPath;
}

bool LoadedGameModule::IsDynamic() const
{
    return library != nullptr;
}

void LoadedGameModule::Reset()
{
    fallback.reset();

    if (module && destroy)
    {
        destroy(module);
    }
    module = nullptr;
    destroy = nullptr;

    UnloadLibraryHandle(library);
    library = nullptr;
    libraryPath.clear();
}

std::unique_ptr<LoadedGameModule> GameModuleResolver::Resolve(
    const std::string& gameModuleTarget,
    const std::filesystem::path& projectRoot,
    std::string* warning) const
{
    std::string lastLoadError;
    std::vector<std::filesystem::path> ExistingCandidates;
    for (const std::filesystem::path& Candidate : BuildCandidatePaths(gameModuleTarget, projectRoot))
    {
        std::error_code FilesystemError;
        if (!std::filesystem::exists(Candidate, FilesystemError) ||
            !std::filesystem::is_regular_file(Candidate, FilesystemError))
        {
            continue;
        }

        ExistingCandidates.push_back(Candidate);
    }

    std::stable_sort(
        ExistingCandidates.begin(),
        ExistingCandidates.end(),
        [](const std::filesystem::path& Left, const std::filesystem::path& Right)
        {
            std::error_code LeftError;
            std::error_code RightError;
            const std::filesystem::file_time_type LeftWriteTime = std::filesystem::last_write_time(Left, LeftError);
            const std::filesystem::file_time_type RightWriteTime = std::filesystem::last_write_time(Right, RightError);
            if (LeftError || RightError)
            {
                return !LeftError && RightError;
            }
            return LeftWriteTime > RightWriteTime;
        });

    for (const std::filesystem::path& Candidate : ExistingCandidates)
    {
        std::string loadError;
        void* library = LoadLibraryHandle(Candidate, &loadError);
        if (!library)
        {
            lastLoadError = Candidate.string() + ": " + loadError;
            continue;
        }

        auto create = reinterpret_cast<AE::CreateGameModuleFunction>(
            LoadSymbol(library, AE::CreateGameModuleSymbolName));
        auto destroy = reinterpret_cast<AE::DestroyGameModuleFunction>(
            LoadSymbol(library, AE::DestroyGameModuleSymbolName));
        if (!create || !destroy)
        {
            lastLoadError = Candidate.string() + ": missing Amber game module exports.";
            UnloadLibraryHandle(library);
            continue;
        }

        AE::IGameModule* module = create();
        if (!module)
        {
            lastLoadError = Candidate.string() + ": AmberCreateGameModule returned null.";
            UnloadLibraryHandle(library);
            continue;
        }

        return std::unique_ptr<LoadedGameModule>(
            new LoadedGameModule(library, module, destroy, CanonicalIfPossible(Candidate)));
    }

    if (warning)
    {
        *warning = "Could not load game module target '" + gameModuleTarget + "'.";
        if (!lastLoadError.empty())
        {
            *warning += " Last error: " + lastLoadError;
        }
        *warning += " Falling back to EditorScenePreviewGameModule.";
    }
    return CreatePreviewModule(gameModuleTarget);
}

std::vector<std::filesystem::path> GameModuleResolver::BuildCandidatePaths(
    const std::string& gameModuleTarget,
    const std::filesystem::path& projectRoot) const
{
    std::vector<std::filesystem::path> candidates;
    const std::vector<std::string> targetNames = ModuleTargetNames(gameModuleTarget);
    if (targetNames.empty())
    {
        return candidates;
    }

    std::vector<std::filesystem::path> roots;
    std::filesystem::path projectFolderName;
    if (!projectRoot.empty())
    {
        roots.push_back(projectRoot / "Builds" / "Editor");
        projectFolderName = std::filesystem::path(projectRoot).filename();
    }
    roots.push_back(std::filesystem::current_path() / "Builds" / "Editor");

    const std::array<std::string, 4> configurations = {"Debug", "Release", "RelWithDebInfo", "MinSizeRel"};
    for (const std::string& targetName : targetNames)
    {
        const std::filesystem::path fileName = ModuleFileName(targetName);
        if (fileName.empty())
        {
            continue;
        }

        for (const std::filesystem::path& root : roots)
        {
            for (const std::string& configuration : configurations)
            {
                candidates.push_back(root / configuration / fileName);
                candidates.push_back(root / "Samples" / configuration / fileName);
                candidates.push_back(root / "Projects" / configuration / fileName);
                if (!projectFolderName.empty())
                {
                    candidates.push_back(root / "Projects" / projectFolderName / configuration / fileName);
                }
            }
            candidates.push_back(root / fileName);
            candidates.push_back(root / "Samples" / fileName);
            candidates.push_back(root / "Projects" / fileName);
            if (!projectFolderName.empty())
            {
                candidates.push_back(root / "Projects" / projectFolderName / fileName);
            }
        }
    }

    return candidates;
}

std::unique_ptr<LoadedGameModule> GameModuleResolver::CreatePreviewModule(const std::string& requestedTarget) const
{
    return std::unique_ptr<LoadedGameModule>(
        new LoadedGameModule(std::make_unique<EditorScenePreviewGameModule>(requestedTarget)));
}

}
