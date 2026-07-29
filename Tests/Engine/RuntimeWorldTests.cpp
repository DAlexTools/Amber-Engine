#include "Game/RuntimeRenderSystems.h"
#include "Game/RuntimeWorld.h"
#include "Scene/PrimitiveObjects.h"

#include <gtest/gtest.h>

#include <memory>
#include <utility>

namespace
{

class RuntimeWorldTestModule final : public AE::IGameModule
{
public:
	void RegisterSceneObjects(AE::Scene::ObjectFactory& objectFactory) override
	{
		++registerSceneObjectCalls;
		objectFactory.RegisterClass("RuntimeWorldTestBox", [](AE::Scene::ObjectData data)
									{ return std::make_unique<AE::Scene::BoxObject>(std::move(data)); });
	}

	const char* GetName() const override
	{
		return "RuntimeWorldTestModule";
	}

	int registerSceneObjectCalls = 0;
};

AE::Scene::ObjectData MakeObject(
	std::string name,
	AE::Scene::ObjectKind kind,
	std::string className,
	AE::Scene::Vec2 size = AE::Scene::Vec2{32.0f, 32.0f})
{
	AE::Scene::ObjectData object;
	object.name = std::move(name);
	object.kind = kind;
	object.className = std::move(className);
	object.size = size;
	if (kind == AE::Scene::ObjectKind::AssetInstance)
	{
		object.assetId = "Project/Sprites/Test.png";
	}
	return object;
}

template <typename TSystem>
AE::SizeT CountSystemEntities(const Registry& registry)
{
	return registry.GetSystem<TSystem>().GetSystemEntity().size();
}

} // namespace

TEST(RuntimeWorldTests, BuildsRegistryObjectsAndRuntimeRenderSystems)
{
	AE::Scene::Document scene;
	scene.objects.push_back(MakeObject("Camera", AE::Scene::ObjectKind::Camera, "Object"));
	scene.objects.push_back(MakeObject("Sprite", AE::Scene::ObjectKind::AssetInstance, "SpriteObject"));
	scene.objects.push_back(MakeObject("Box", AE::Scene::ObjectKind::Box, "RuntimeWorldTestBox"));
	scene.objects.push_back(MakeObject("Circle", AE::Scene::ObjectKind::Circle, "CircleObject"));

	RuntimeWorldTestModule module;
	AE::RuntimeWorld world;
	std::string error;

	ASSERT_TRUE(AE::BuildRuntimeWorld(scene, &module, world, AE::RuntimeWorldBuildOptions{}, &error));
	EXPECT_TRUE(error.empty());
	EXPECT_EQ(module.registerSceneObjectCalls, 1);
	EXPECT_EQ(world.GetObjectCount(), scene.objects.size());
	EXPECT_TRUE(world.GetRegistry().HasSystem<AE::RuntimeSceneSpriteRenderSystem>());
	EXPECT_TRUE(world.GetRegistry().HasSystem<AE::RuntimeSceneShapeRenderSystem>());
	EXPECT_TRUE(world.GetRegistry().HasSystem<AE::RuntimeLegacySpriteRenderSystem>());
	EXPECT_EQ(CountSystemEntities<AE::RuntimeSceneSpriteRenderSystem>(world.GetRegistry()), AE::SizeT{1});
	EXPECT_EQ(CountSystemEntities<AE::RuntimeSceneShapeRenderSystem>(world.GetRegistry()), AE::SizeT{2});
}

TEST(RuntimeWorldTests, CanSkipGameModuleSceneObjectRegistrationForEditorDynamicBoundary)
{
	AE::Scene::Document scene;
	scene.objects.push_back(MakeObject("Box", AE::Scene::ObjectKind::Box, "RuntimeWorldTestBox"));

	RuntimeWorldTestModule module;
	AE::RuntimeWorld world;
	AE::RuntimeWorldBuildOptions options;
	options.registerGameModuleSceneObjects = false;

	ASSERT_TRUE(AE::BuildRuntimeWorld(scene, &module, world, options));
	EXPECT_EQ(module.registerSceneObjectCalls, 0);
	EXPECT_EQ(world.GetObjectCount(), AE::SizeT{1});
	EXPECT_EQ(CountSystemEntities<AE::RuntimeSceneShapeRenderSystem>(world.GetRegistry()), AE::SizeT{1});
}
