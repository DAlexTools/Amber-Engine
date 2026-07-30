#include "EditorPlaySession.h"
#include "Game/RuntimeRenderSystems.h"
#include "Game/RuntimeWorld.h"
#include "Project/ProjectDescriptor.h"
#include "Scene/SceneAsset.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace
{

std::filesystem::path SourceRoot()
{
	return std::filesystem::path(AMBER_TEST_SOURCE_ROOT);
}

template <typename TSystem>
AE::SizeT CountSystemEntities(const Registry& Registry)
{
	return Registry.GetSystem<TSystem>().GetSystemEntity().size();
}

std::vector<std::filesystem::path> FindProjectDescriptors()
{
	std::vector<std::filesystem::path> ProjectFiles;
	const std::filesystem::path ProjectsRoot = SourceRoot() / "Projects";
	if (!std::filesystem::exists(ProjectsRoot))
	{
		return ProjectFiles;
	}

	for (const std::filesystem::directory_entry& Entry : std::filesystem::recursive_directory_iterator(ProjectsRoot))
	{
		if (Entry.is_regular_file() && Entry.path().extension() == ".amberproject")
		{
			ProjectFiles.push_back(Entry.path());
		}
	}

	std::sort(ProjectFiles.begin(), ProjectFiles.end());
	return ProjectFiles;
}

void ExpectVec2Equal(const AE::Scene::Vec2& Actual, const AE::Scene::Vec2& Expected)
{
	EXPECT_FLOAT_EQ(Actual.x, Expected.x);
	EXPECT_FLOAT_EQ(Actual.y, Expected.y);
}

void ExpectTransformEqual(const AE::Scene::Transform& Actual, const AE::Scene::Transform& Expected)
{
	ExpectVec2Equal(Actual.position, Expected.position);
	EXPECT_FLOAT_EQ(Actual.rotationDegrees, Expected.rotationDegrees);
	ExpectVec2Equal(Actual.scale, Expected.scale);
}

void ExpectComponentsEqual(
	const std::vector<AE::Scene::ComponentData>& Actual,
	const std::vector<AE::Scene::ComponentData>& Expected)
{
	ASSERT_EQ(Actual.size(), Expected.size());
	for (AE::SizeT ComponentIndex = 0; ComponentIndex < Expected.size(); ++ComponentIndex)
	{
		SCOPED_TRACE("ComponentIndex=" + std::to_string(ComponentIndex));
		EXPECT_EQ(Actual[ComponentIndex].name, Expected[ComponentIndex].name);
		ASSERT_EQ(Actual[ComponentIndex].properties.size(), Expected[ComponentIndex].properties.size());
		for (AE::SizeT PropertyIndex = 0; PropertyIndex < Expected[ComponentIndex].properties.size(); ++PropertyIndex)
		{
			SCOPED_TRACE("PropertyIndex=" + std::to_string(PropertyIndex));
			EXPECT_EQ(Actual[ComponentIndex].properties[PropertyIndex].name, Expected[ComponentIndex].properties[PropertyIndex].name);
			EXPECT_EQ(Actual[ComponentIndex].properties[PropertyIndex].type, Expected[ComponentIndex].properties[PropertyIndex].type);
			EXPECT_EQ(Actual[ComponentIndex].properties[PropertyIndex].value, Expected[ComponentIndex].properties[PropertyIndex].value);
		}
	}
}

void ExpectObjectDataEqual(const AE::Scene::ObjectData& Actual, const AE::Scene::ObjectData& Expected)
{
	EXPECT_EQ(Actual.name, Expected.name);
	EXPECT_EQ(Actual.assetId, Expected.assetId);
	EXPECT_EQ(Actual.className, Expected.className);
	EXPECT_EQ(Actual.kind, Expected.kind);
	ExpectTransformEqual(Actual.transform, Expected.transform);
	ExpectVec2Equal(Actual.size, Expected.size);
	EXPECT_EQ(Actual.visible, Expected.visible);
	EXPECT_EQ(Actual.locked, Expected.locked);
	ExpectComponentsEqual(Actual.components, Expected.components);
}

void ExpectDocumentsEqual(const AE::Scene::Document& Actual, const AE::Scene::Document& Expected)
{
	EXPECT_EQ(Actual.name, Expected.name);
	ASSERT_EQ(Actual.objects.size(), Expected.objects.size());
	for (AE::SizeT ObjectIndex = 0; ObjectIndex < Expected.objects.size(); ++ObjectIndex)
	{
		SCOPED_TRACE("ObjectIndex=" + std::to_string(ObjectIndex));
		ExpectObjectDataEqual(Actual.objects[ObjectIndex], Expected.objects[ObjectIndex]);
	}
}

void ExpectRuntimeWorldObjectsEqual(
	const AE::RuntimeWorld& Actual,
	const AE::Scene::Document& ExpectedDocument)
{
	ASSERT_EQ(Actual.GetSceneObjects().size(), ExpectedDocument.objects.size());
	for (AE::SizeT ObjectIndex = 0; ObjectIndex < ExpectedDocument.objects.size(); ++ObjectIndex)
	{
		SCOPED_TRACE("RuntimeObjectIndex=" + std::to_string(ObjectIndex));
		ASSERT_NE(Actual.GetSceneObjects()[ObjectIndex], nullptr);
		ExpectObjectDataEqual(Actual.GetSceneObjects()[ObjectIndex]->GetData(), ExpectedDocument.objects[ObjectIndex]);
	}
}

void AssertProjectStartupRuntimeParity(const std::filesystem::path& ProjectFile)
{
	AE::ProjectDescriptor Descriptor;
	std::string Error;
	ASSERT_TRUE(AE::LoadProjectDescriptor(ProjectFile, Descriptor, &Error)) << Error;

	const std::filesystem::path ScenePath = Descriptor.ResolveProjectPath(Descriptor.startupScene);
	AE::Scene::Document StandaloneDocument;
	ASSERT_TRUE(AE::Scene::LoadScene(ScenePath, StandaloneDocument, &Error)) << Error;

	AE::RuntimeWorld StandaloneWorld;
	ASSERT_TRUE(AE::BuildRuntimeWorld(StandaloneDocument, nullptr, StandaloneWorld, AE::RuntimeWorldBuildOptions{}, &Error)) << Error;
	EXPECT_TRUE(Error.empty());
	ExpectRuntimeWorldObjectsEqual(StandaloneWorld, StandaloneDocument);

	AE::Editor::SceneDocument EditScene;
	ASSERT_TRUE(EditScene.LoadFromFile(ScenePath, &Error)) << Error;
	const AE::Scene::Document EditorRuntimeDocument = EditScene.ToRuntimeDocument();
	ExpectDocumentsEqual(EditorRuntimeDocument, StandaloneDocument);

	AE::Editor::PlayInPIERequest Request;
	Request.projectName = Descriptor.name;
	Request.projectRoot = Descriptor.projectRoot;
	Request.scenePath = ScenePath;
	Request.gameModuleTarget.clear();
	Request.playTarget = "PIE";

	AE::Editor::EditorPlaySession Pie;
	ASSERT_TRUE(Pie.PlayInPIE(Request, EditScene));
	ASSERT_TRUE(Pie.IsPlaying());
	ASSERT_NE(Pie.GetRuntimeRegistry(), nullptr);
	ASSERT_NE(Pie.GetRuntimeSceneDocument(), nullptr);

	EXPECT_EQ(Pie.GetRuntimeSceneDocument()->GetObjects().size(), EditScene.GetObjects().size());
	ExpectDocumentsEqual(Pie.GetRuntimeSceneDocument()->ToRuntimeDocument(), StandaloneDocument);
	EXPECT_EQ(Pie.GetRuntimeObjectCount(), StandaloneWorld.GetObjectCount());
	EXPECT_EQ(Pie.GetRuntimeObjectCount(), StandaloneDocument.objects.size());
	EXPECT_EQ(
		CountSystemEntities<AE::RuntimeSceneSpriteRenderSystem>(*Pie.GetRuntimeRegistry()),
		CountSystemEntities<AE::RuntimeSceneSpriteRenderSystem>(StandaloneWorld.GetRegistry()));
	EXPECT_EQ(
		CountSystemEntities<AE::RuntimeSceneShapeRenderSystem>(*Pie.GetRuntimeRegistry()),
		CountSystemEntities<AE::RuntimeSceneShapeRenderSystem>(StandaloneWorld.GetRegistry()));

	Pie.Update();
	EXPECT_EQ(Pie.GetFrameCount(), AE::uint64{1});

	Pie.Stop();
	EXPECT_FALSE(Pie.IsPlaying());

	Request.RunMode = AE::EGameModuleRunMode::Simulate;
	ASSERT_TRUE(Pie.PlayInPIE(Request, EditScene));
	ASSERT_TRUE(Pie.IsPlaying());
	EXPECT_TRUE(Pie.IsSimulating());
	EXPECT_EQ(Pie.GetRunMode(), AE::EGameModuleRunMode::Simulate);
	EXPECT_STREQ(Pie.GetRunModeName(), "Simulate");
	ASSERT_NE(Pie.GetRuntimeRegistry(), nullptr);
	ASSERT_NE(Pie.GetRuntimeSceneDocument(), nullptr);
	ExpectDocumentsEqual(Pie.GetRuntimeSceneDocument()->ToRuntimeDocument(), StandaloneDocument);

	Pie.Update();
	EXPECT_EQ(Pie.GetFrameCount(), AE::uint64{1});

	Pie.Stop();
	EXPECT_FALSE(Pie.IsPlaying());
	EXPECT_FALSE(Pie.IsSimulating());
}

} // namespace

TEST(EditorRuntimeParityTests, ProjectStartupScenesBuildSameRuntimeWorldForPIEAndStandaloneBootstrap)
{
	const std::vector<std::filesystem::path> ProjectFiles = FindProjectDescriptors();
	ASSERT_FALSE(ProjectFiles.empty());

	for (const std::filesystem::path& ProjectFile : ProjectFiles)
	{
		SCOPED_TRACE(ProjectFile.string());
		AssertProjectStartupRuntimeParity(ProjectFile);
	}
}
