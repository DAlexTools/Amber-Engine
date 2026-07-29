#ifndef AMBER_RUNTIME_GAME_RUNTIME_WORLD_H
#define AMBER_RUNTIME_GAME_RUNTIME_WORLD_H

#include "EntityComponentSystem/ECS.h"
#include "Game/GameModuleInterface.h"
#include "Scene/ObjectFactory.h"

#include <memory>
#include <string>
#include <vector>

namespace AE
{

struct RuntimeWorldBuildOptions
{
	bool registerRenderSystems = true;
	bool registerGameModuleSceneObjects = true;
};

class RuntimeWorld
{
public:
	RuntimeWorld();
	~RuntimeWorld();

	RuntimeWorld(const RuntimeWorld&) = delete;
	RuntimeWorld& operator=(const RuntimeWorld&) = delete;

	void Reset();
	void DestroyObjects();

	Registry& GetRegistry();
	const Registry& GetRegistry() const;
	Scene::ObjectFactory& GetObjectFactory();
	const Scene::ObjectFactory& GetObjectFactory() const;
	std::vector<std::unique_ptr<Scene::Object>>& GetSceneObjects();
	const std::vector<std::unique_ptr<Scene::Object>>& GetSceneObjects() const;
	std::size_t GetObjectCount() const;

private:
	std::unique_ptr<Registry> registry;
	Scene::ObjectFactory objectFactory;
	std::vector<std::unique_ptr<Scene::Object>> sceneObjects;
};

bool BuildRuntimeWorld(
	const Scene::Document& scene,
	IGameModule* gameModule,
	RuntimeWorld& world,
	const RuntimeWorldBuildOptions& options = RuntimeWorldBuildOptions{},
	std::string* error = nullptr);

} // namespace AE

#endif
