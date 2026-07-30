#include "Game/RuntimeViewerSDL.h"

#include <gtest/gtest.h>

TEST(RuntimeRendererConfigTests, BuildsSceneCameraConfigFromProjectDescriptor)
{
	AE::ProjectDescriptor descriptor;
	descriptor.name = "RendererConfigTest";
	descriptor.projectRoot = std::filesystem::path("C:/TestProject");
	descriptor.engineRoot = std::filesystem::path("C:/AmberEngine");
	descriptor.contentRoot = "Content";

	AE::Scene::Document scene;
	AE::Scene::ObjectData hiddenCamera;
	hiddenCamera.kind = AE::Scene::ObjectKind::Camera;
	hiddenCamera.visible = false;
	hiddenCamera.transform.position = AE::Scene::Vec2{10.0f, 20.0f};
	scene.objects.push_back(hiddenCamera);

	AE::Scene::ObjectData camera;
	camera.kind = AE::Scene::ObjectKind::Camera;
	camera.visible = true;
	camera.transform.position = AE::Scene::Vec2{320.0f, 180.0f};
	scene.objects.push_back(camera);

	const AE::RuntimeSceneRendererConfig config = AE::BuildRuntimeSceneRendererConfig(descriptor, scene);

	EXPECT_EQ(config.cameraPolicy, AE::RuntimeCameraPolicy::SceneCamera);
	EXPECT_EQ(config.cameraX, 320.0f);
	EXPECT_EQ(config.cameraY, 180.0f);
	EXPECT_EQ(config.zoom, 1.0f);
	EXPECT_EQ(config.projectRoot, descriptor.projectRoot);
	EXPECT_EQ(config.engineRoot, descriptor.engineRoot);
	EXPECT_EQ(config.contentRoot, descriptor.projectRoot / "Content");
	ASSERT_GE(config.assetRoots.size(), 1u);
	EXPECT_EQ(config.assetRoots.front().Name, "Project");
	EXPECT_EQ(config.assetRoots.front().Path, descriptor.projectRoot / "Content");
}

TEST(RuntimeRendererConfigTests, LeavesExplicitCameraWhenNoSceneCameraExists)
{
	AE::Scene::Document scene;
	AE::RuntimeSceneRendererConfig config;
	config.cameraPolicy = AE::RuntimeCameraPolicy::Explicit;
	config.cameraX = 42.0f;
	config.cameraY = 84.0f;

	EXPECT_FALSE(AE::ApplyRuntimeSceneCamera(scene, config));
	EXPECT_EQ(config.cameraX, 42.0f);
	EXPECT_EQ(config.cameraY, 84.0f);
}
