#include "Game/RuntimeWorld.h"

#include "Game/RuntimeRenderSystems.h"

namespace AE
{

RuntimeWorld::RuntimeWorld()
	: registry(std::make_unique<Registry>())
{
}

RuntimeWorld::~RuntimeWorld()
{
	DestroyObjects();
}

void RuntimeWorld::Reset()
{
	DestroyObjects();
	registry = std::make_unique<Registry>();
	objectFactory = Scene::ObjectFactory();
}

void RuntimeWorld::DestroyObjects()
{
	for (std::unique_ptr<Scene::Object>& object : sceneObjects)
	{
		if (object)
		{
			object->OnDestroy();
		}
	}
	sceneObjects.clear();
}

Registry& RuntimeWorld::GetRegistry()
{
	return *registry;
}

const Registry& RuntimeWorld::GetRegistry() const
{
	return *registry;
}

Scene::ObjectFactory& RuntimeWorld::GetObjectFactory()
{
	return objectFactory;
}

const Scene::ObjectFactory& RuntimeWorld::GetObjectFactory() const
{
	return objectFactory;
}

std::vector<std::unique_ptr<Scene::Object>>& RuntimeWorld::GetSceneObjects()
{
	return sceneObjects;
}

const std::vector<std::unique_ptr<Scene::Object>>& RuntimeWorld::GetSceneObjects() const
{
	return sceneObjects;
}

std::size_t RuntimeWorld::GetObjectCount() const
{
	return sceneObjects.size();
}

bool BuildRuntimeWorld(
	const Scene::Document& scene,
	IGameModule* gameModule,
	RuntimeWorld& world,
	const RuntimeWorldBuildOptions& options,
	std::string* error)
{
	world.Reset();

	if (options.registerRenderSystems)
	{
		RegisterRuntimeRenderSystems(world.GetRegistry());
	}

	if (gameModule && options.registerGameModuleSceneObjects)
	{
		gameModule->RegisterSceneObjects(world.GetObjectFactory());
	}

	world.GetSceneObjects() = world.GetObjectFactory().CreateObjects(scene, &world.GetRegistry());
	world.GetRegistry().Update();

	if (error)
	{
		error->clear();
	}
	return true;
}

} // namespace AE
