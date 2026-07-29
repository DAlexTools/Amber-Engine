#ifndef AMBER_RUNTIME_GAME_RUNTIME_RENDER_SYSTEMS_H
#define AMBER_RUNTIME_GAME_RUNTIME_RENDER_SYSTEMS_H

#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "EntityComponentSystem/ECS.h"
#include "Scene/SceneObjectComponents.h"

namespace AE
{

class RuntimeSceneSpriteRenderSystem : public System
{
public:
	RuntimeSceneSpriteRenderSystem()
	{
		RequireComponent<AE::Scene::SceneObjectComponent>();
		RequireComponent<TransformComponent>();
		RequireComponent<AE::Scene::SceneSpriteComponent>();
	}
};

class RuntimeSceneShapeRenderSystem : public System
{
public:
	RuntimeSceneShapeRenderSystem()
	{
		RequireComponent<AE::Scene::SceneObjectComponent>();
		RequireComponent<TransformComponent>();
		RequireComponent<AE::Scene::SceneShapeComponent>();
	}
};

class RuntimeLegacySpriteRenderSystem : public System
{
public:
	RuntimeLegacySpriteRenderSystem()
	{
		RequireComponent<TransformComponent>();
		RequireComponent<SpriteComponent>();
	}
};

inline void RegisterRuntimeRenderSystems(Registry& registry)
{
	if (!registry.HasSystem<RuntimeSceneSpriteRenderSystem>())
	{
		registry.AddSystem<RuntimeSceneSpriteRenderSystem>();
	}
	if (!registry.HasSystem<RuntimeSceneShapeRenderSystem>())
	{
		registry.AddSystem<RuntimeSceneShapeRenderSystem>();
	}
	if (!registry.HasSystem<RuntimeLegacySpriteRenderSystem>())
	{
		registry.AddSystem<RuntimeLegacySpriteRenderSystem>();
	}
}

} // namespace AE

#endif
