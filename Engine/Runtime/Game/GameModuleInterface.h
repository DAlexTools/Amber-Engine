#ifndef AMBER_RUNTIME_GAME_MODULE_INTERFACE_H
#define AMBER_RUNTIME_GAME_MODULE_INTERFACE_H

#include "Core/Platform/PlatformTypes.h"
#include "EntityComponentSystem/ECS.h"
#include "Scene/Object.h"
#include "Scene/ObjectFactory.h"
#include "Scene/SceneAsset.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#if defined(_WIN32)
#define AMBER_GAME_MODULE_EXPORT extern "C" __declspec(dllexport)
#else
#define AMBER_GAME_MODULE_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace AE
{

struct GameModuleStartContext
{
	std::string projectName;
	std::filesystem::path projectRoot;
	std::filesystem::path scenePath;
	const Scene::Document& sceneDocument;
	Registry& registry;
	Scene::ObjectFactory& objectFactory;
	std::vector<std::unique_ptr<Scene::Object>>& sceneObjects;
};

struct GameModuleTickContext
{
	Registry& registry;
	float deltaSeconds = 0.0f;
	uint64 frameIndex = 0;
};

struct GameModuleRenderContext
{
	Registry& registry;
	uint64 frameIndex = 0;
	// Runtime-specific render context; valid only for the current Render call.
	void* nativeRenderContext = nullptr;
};

class IGameModule
{
public:
	virtual ~IGameModule() = default;

	virtual const char* GetName() const = 0;
	virtual void RegisterSceneObjects(Scene::ObjectFactory& objectFactory);
	virtual bool StartPlay(const GameModuleStartContext& context, std::string* error);
	virtual void Tick(const GameModuleTickContext& context);
	virtual void Render(const GameModuleRenderContext& context);
	virtual void StopPlay();
};

using CreateGameModuleFunction = IGameModule* (*)();
using DestroyGameModuleFunction = void (*)(IGameModule*);

inline constexpr const char* CreateGameModuleSymbolName = "AmberCreateGameModule";
inline constexpr const char* DestroyGameModuleSymbolName = "AmberDestroyGameModule";

} // namespace AE

#endif
