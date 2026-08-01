#include "Scene/SpriteObject.h"

#include "Scene/SceneObjectComponents.h"

#include <utility>

namespace AE::Scene
{

SpriteObject::SpriteObject(ObjectData objectData)
	: Object(std::move(objectData))
{
	data.className = GetClassName();
}

const char* SpriteObject::GetClassName() const
{
	return "SpriteObject";
}

void SpriteObject::ConfigureEntity(Registry& ownerRegistry)
{
	Object::ConfigureEntity(ownerRegistry);
	entity.AddComponent<SceneSpriteComponent>(data.assetId, data.size.x, data.size.y);
}

} // namespace AE::Scene
