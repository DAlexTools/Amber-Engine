#ifndef AMBER_RUNTIME_SCENE_SCENE_OBJECT_COMPONENTS_H
#define AMBER_RUNTIME_SCENE_SCENE_OBJECT_COMPONENTS_H

#include <string>

namespace AE::Scene
{

enum class SceneShapeType
{
	Box,
	Circle
};

struct SceneObjectComponent
{
	std::string name;
	std::string className;
	std::string assetId;
	bool visible = true;

	SceneObjectComponent(
		std::string objectName = {},
		std::string objectClassName = "Object",
		std::string objectAssetId = {},
		bool objectVisible = true);
};

struct SceneSpriteComponent
{
	std::string assetId;
	float width = 0.0f;
	float height = 0.0f;

	SceneSpriteComponent(std::string spriteAssetId = {}, float spriteWidth = 0.0f, float spriteHeight = 0.0f);
};

struct SceneShapeComponent
{
	SceneShapeType shapeType = SceneShapeType::Box;
	float width = 0.0f;
	float height = 0.0f;

	SceneShapeComponent(
		SceneShapeType sceneShapeType = SceneShapeType::Box,
		float shapeWidth = 0.0f,
		float shapeHeight = 0.0f);
};

} // namespace AE::Scene

#endif
