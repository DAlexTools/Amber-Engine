#include "Scene/PrimitiveObjects.h"

#include "Scene/SceneObjectComponents.h"

#include <utility>

namespace AE::Scene
{
namespace
{
void SetDefaultClassName(ObjectData& data, const char* className)
{
	if (data.className.empty() || data.className == "Object")
	{
		data.className = className;
	}
}
} // namespace

BoxObject::BoxObject(ObjectData objectData)
	: Object(std::move(objectData))
{
	data.kind = ObjectKind::Box;
	SetDefaultClassName(data, GetClassName());
}

const char* BoxObject::GetClassName() const
{
	return "BoxObject";
}

void BoxObject::ConfigureEntity(Registry& ownerRegistry)
{
	Object::ConfigureEntity(ownerRegistry);
	AddComponent<SceneShapeComponent>(SceneShapeType::Box, data.size.x, data.size.y);
}

CircleObject::CircleObject(ObjectData objectData)
	: Object(std::move(objectData))
{
	data.kind = ObjectKind::Circle;
	SetDefaultClassName(data, GetClassName());
}

const char* CircleObject::GetClassName() const
{
	return "CircleObject";
}

void CircleObject::ConfigureEntity(Registry& ownerRegistry)
{
	Object::ConfigureEntity(ownerRegistry);
	AddComponent<SceneShapeComponent>(SceneShapeType::Circle, data.size.x, data.size.y);
}

} // namespace AE::Scene
