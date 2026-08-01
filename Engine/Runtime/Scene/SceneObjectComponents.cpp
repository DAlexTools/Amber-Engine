#include "Scene/SceneObjectComponents.h"

#include <utility>

namespace AE::Scene
{

SceneObjectComponent::SceneObjectComponent(
	std::string objectName,
	std::string objectClassName,
	std::string objectAssetId,
	bool objectVisible)
	: name(std::move(objectName))
	, className(std::move(objectClassName))
	, assetId(std::move(objectAssetId))
	, visible(objectVisible)
{
}

SceneSpriteComponent::SceneSpriteComponent(std::string spriteAssetId, float spriteWidth, float spriteHeight)
	: assetId(std::move(spriteAssetId))
	, width(spriteWidth)
	, height(spriteHeight)
{
}

SceneShapeComponent::SceneShapeComponent(SceneShapeType sceneShapeType, float shapeWidth, float shapeHeight)
	: shapeType(sceneShapeType)
	, width(shapeWidth)
	, height(shapeHeight)
{
}

} // namespace AE::Scene
