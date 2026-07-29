#include "EditorPlaySession.h"
#include "Game/RuntimeRenderSystems.h"
#include "Game/RuntimeWorld.h"
#include "Project/ProjectDescriptor.h"
#include "Scene/SceneAsset.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace
{

std::filesystem::path SourceRoot()
{
	return std::filesystem::path(AMBER_TEST_SOURCE_ROOT);
}

template <typename TSystem>
AE::SizeT CountSystemEntities(const Registry& registry)
{
	return registry.GetSystem<TSystem>().GetSystemEntity().size();
}

} // namespace

TEST(EditorRuntimeParityTests, ProjectStartupSceneBuildsSameRuntimeWorldForPIEAndStandaloneBootstrap)
{
	const std::filesystem::path projectFile = SourceRoot() / "Projects" / "Platformer" / "Platformer.amberproject";

	AE::ProjectDescriptor descriptor;
	std::string error;
	ASSERT_TRUE(AE::LoadProjectDescriptor(projectFile, descriptor, &error)) << error;

	const std::filesystem::path scenePath = descriptor.ResolveProjectPath(descriptor.startupScene);
	AE::Scene::Document standaloneDocument;
	ASSERT_TRUE(AE::Scene::LoadScene(scenePath, standaloneDocument, &error)) << error;

	AE::RuntimeWorld standaloneWorld;
	ASSERT_TRUE(AE::BuildRuntimeWorld(standaloneDocument, nullptr, standaloneWorld, AE::RuntimeWorldBuildOptions{}, &error));
	EXPECT_TRUE(error.empty());

	AE::Editor::SceneDocument editScene;
	ASSERT_TRUE(editScene.LoadFromFile(scenePath, &error)) << error;

	AE::Editor::PlayInPIERequest request;
	request.projectName = descriptor.name;
	request.projectRoot = descriptor.projectRoot;
	request.scenePath = scenePath;
	request.gameModuleTarget.clear();
	request.playTarget = "PIE";

	AE::Editor::EditorPlaySession pie;
	ASSERT_TRUE(pie.PlayInPIE(request, editScene));
	ASSERT_TRUE(pie.IsPlaying());
	ASSERT_NE(pie.GetRuntimeRegistry(), nullptr);
	ASSERT_NE(pie.GetRuntimeSceneDocument(), nullptr);

	EXPECT_EQ(editScene.GetObjects().size(), standaloneDocument.objects.size());
	EXPECT_EQ(pie.GetRuntimeSceneDocument()->GetObjects().size(), editScene.GetObjects().size());
	EXPECT_EQ(pie.GetRuntimeObjectCount(), standaloneWorld.GetObjectCount());
	EXPECT_EQ(pie.GetRuntimeObjectCount(), standaloneDocument.objects.size());
	EXPECT_EQ(
		CountSystemEntities<AE::RuntimeSceneSpriteRenderSystem>(*pie.GetRuntimeRegistry()),
		CountSystemEntities<AE::RuntimeSceneSpriteRenderSystem>(standaloneWorld.GetRegistry()));
	EXPECT_EQ(
		CountSystemEntities<AE::RuntimeSceneShapeRenderSystem>(*pie.GetRuntimeRegistry()),
		CountSystemEntities<AE::RuntimeSceneShapeRenderSystem>(standaloneWorld.GetRegistry()));

	pie.Update();
	EXPECT_EQ(pie.GetFrameCount(), AE::uint64{1});

	pie.Stop();
	EXPECT_FALSE(pie.IsPlaying());
}
