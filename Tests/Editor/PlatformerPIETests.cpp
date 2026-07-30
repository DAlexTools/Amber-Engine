#include "PlatformerGameModule.h"

#include "Game/GameModuleInterface.h"
#include "Scene/Object.h"
#include "Scene/ObjectFactory.h"
#include "Scene/SceneAsset.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#if SMOKE_TEST && AMBER_ENABLE_PLATFORMER_EDITOR_SCENE
namespace
{

AE::Scene::ObjectData MakeSceneBox(
	const std::string& Name,
	const std::string& ClassName,
	AE::Scene::Vec2 Position,
	AE::Scene::Vec2 Size)
{
	AE::Scene::ObjectData Object;
	Object.name = Name;
	Object.className = ClassName;
	Object.kind = AE::Scene::ObjectKind::Box;
	Object.transform.position = Position;
	Object.transform.scale = AE::Scene::Vec2{1.0f, 1.0f};
	Object.size = Size;
	Object.visible = true;
	return Object;
}

} // namespace

TEST(PlatformerPIETests, StartPlayUsesInMemorySceneDocument)
{
	AE::Scene::Document SceneDocument;
	SceneDocument.name = "Unsaved PIE Scene";
	SceneDocument.objects.push_back(MakeSceneBox(
		"Unsaved Player Spawn",
		"PlayerSpawnObject",
		AE::Scene::Vec2{736.0f, 352.0f},
		AE::Scene::Vec2{32.0f, 48.0f}));
	SceneDocument.objects.push_back(MakeSceneBox(
		"Unsaved Solid Platform",
		"SolidPlatformObject",
		AE::Scene::Vec2{928.0f, 448.0f},
		AE::Scene::Vec2{256.0f, 32.0f}));

	Registry RuntimeRegistry;
	AE::Scene::ObjectFactory ObjectFactory;
	std::vector<std::unique_ptr<AE::Scene::Object>> SceneObjects;
	AE::GameModuleStartContext Context{
		"PlatformerPIETest",
		std::filesystem::path("VirtualProject"),
		std::filesystem::path("ThisFileMustNotBeLoaded.amber.scene"),
		SceneDocument,
		RuntimeRegistry,
		ObjectFactory,
		SceneObjects};

	PlatformerGameModule Module;
	std::string Error;
	ASSERT_TRUE(Module.StartPlay(Context, &Error)) << Error;

	const AE::Math::FVector2D PlayerSpawn = Module.GetPlayerSpawnForTests();
	EXPECT_FLOAT_EQ(PlayerSpawn.X, 720.0f);
	EXPECT_FLOAT_EQ(PlayerSpawn.Y, 328.0f);
	EXPECT_EQ(Module.GetEditorSolidPlatformCountForTests(), SizeT{1});

	Module.StopPlay();
}
#endif
