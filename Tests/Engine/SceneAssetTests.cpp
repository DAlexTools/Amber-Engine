#include "Scene/SceneAsset.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
std::filesystem::path MakeSceneAssetTestPath(const char* Name)
{
	const auto Stamp = std::chrono::steady_clock::now().time_since_epoch().count();
	return std::filesystem::temp_directory_path() /
		   ("AmberSceneAsset_" + std::string(Name) + "_" + std::to_string(Stamp) + ".amber.scene");
}
} // namespace

TEST(SceneAssetTests, SavesAndLoadsObjectComponentProperties)
{
	AE::Scene::Document Document;
	Document.name = "Component Scene";

	AE::Scene::ObjectData Object;
	Object.kind = AE::Scene::ObjectKind::Box;
	Object.name = "Enemy Sentry";
	Object.className = "EnemySpawnObject";
	Object.components.push_back(AE::Scene::ComponentData{
		"FEnemySpawnComponent",
		{
			AE::Scene::ComponentPropertyData{"Speed", AE::Scene::ComponentPropertyType::Float, "88.500"},
			AE::Scene::ComponentPropertyData{"MaxHealth", AE::Scene::ComponentPropertyType::Int, "6"},
			AE::Scene::ComponentPropertyData{"CanShoot", AE::Scene::ComponentPropertyType::Bool, "true"},
			AE::Scene::ComponentPropertyData{"Script", AE::Scene::ComponentPropertyType::String, "enemy.lua"},
		}});
	Document.objects.push_back(Object);

	const std::filesystem::path ScenePath = MakeSceneAssetTestPath("Properties");
	std::string Error;
	ASSERT_TRUE(AE::Scene::SaveScene(Document, ScenePath, &Error)) << Error;

	AE::Scene::Document Loaded;
	ASSERT_TRUE(AE::Scene::LoadScene(ScenePath, Loaded, &Error)) << Error;
	std::filesystem::remove(ScenePath);

	ASSERT_EQ(Loaded.objects.size(), AE::SizeT{1});
	const AE::Scene::ObjectData& LoadedObject = Loaded.objects.front();
	ASSERT_EQ(LoadedObject.components.size(), AE::SizeT{1});
	EXPECT_FLOAT_EQ(AE::Scene::GetComponentPropertyFloat(LoadedObject, "FEnemySpawnComponent", "Speed", 0.0f), 88.5f);
	EXPECT_EQ(AE::Scene::GetComponentPropertyInt(LoadedObject, "FEnemySpawnComponent", "MaxHealth", 0), 6);
	EXPECT_TRUE(AE::Scene::GetComponentPropertyBool(LoadedObject, "FEnemySpawnComponent", "CanShoot", false));
	EXPECT_EQ(AE::Scene::GetComponentPropertyString(LoadedObject, "FEnemySpawnComponent", "Script", ""), "enemy.lua");
}

TEST(SceneAssetTests, LoadsVersionOneScenesWithoutComponentProperties)
{
	const std::filesystem::path ScenePath = MakeSceneAssetTestPath("VersionOne");
	{
		std::ofstream File(ScenePath, std::ios::out | std::ios::trunc);
		ASSERT_TRUE(File);
		File << "AmberScene 1\n";
		File << "name \"Legacy Scene\"\n";
		File << "object Box \"Box\" \"\" \"BoxObject\" 0.000 0.000 0.000 1.000 1.000 80.000 80.000 1 0\n";
	}

	std::string Error;
	AE::Scene::Document Loaded;
	ASSERT_TRUE(AE::Scene::LoadScene(ScenePath, Loaded, &Error)) << Error;
	std::filesystem::remove(ScenePath);

	ASSERT_EQ(Loaded.objects.size(), AE::SizeT{1});
	EXPECT_TRUE(Loaded.objects.front().components.empty());
}
