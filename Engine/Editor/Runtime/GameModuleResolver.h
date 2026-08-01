#ifndef AMBER_EDITOR_GAME_MODULE_RESOLVER_H
#define AMBER_EDITOR_GAME_MODULE_RESOLVER_H

#include "Game/GameModuleInterface.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace AE::Editor
{

class LoadedGameModule
{
public:
	LoadedGameModule() = default;
	~LoadedGameModule();

	LoadedGameModule(const LoadedGameModule&) = delete;
	LoadedGameModule& operator=(const LoadedGameModule&) = delete;
	LoadedGameModule(LoadedGameModule&& other) noexcept;
	LoadedGameModule& operator=(LoadedGameModule&& other) noexcept;

	AE::IGameModule* Get() const;
	const std::filesystem::path& GetLibraryPath() const;
	bool IsDynamic() const;

private:
	friend class GameModuleResolver;

	explicit LoadedGameModule(std::unique_ptr<AE::IGameModule> fallbackModule);
	LoadedGameModule(
		void* loadedLibrary,
		AE::IGameModule* loadedModule,
		AE::DestroyGameModuleFunction destroyFunction,
		std::filesystem::path loadedLibraryPath);

	void Reset();

	void* library = nullptr;
	AE::IGameModule* module = nullptr;
	AE::DestroyGameModuleFunction destroy = nullptr;
	std::unique_ptr<AE::IGameModule> fallback;
	std::filesystem::path libraryPath;
};

class GameModuleResolver
{
public:
	std::unique_ptr<LoadedGameModule> Resolve(
		const std::string& gameModuleTarget,
		const std::filesystem::path& projectRoot,
		std::string* warning = nullptr) const;

	std::vector<std::filesystem::path> BuildCandidatePaths(
		const std::string& gameModuleTarget,
		const std::filesystem::path& projectRoot) const;

private:
	std::unique_ptr<LoadedGameModule> CreatePreviewModule(const std::string& requestedTarget) const;
};

} // namespace AE::Editor

#endif
