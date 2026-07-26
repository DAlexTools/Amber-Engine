#include "Scene/Object.h"

#include "Components/TransformComponent.h"
#include "Scene/SceneObjectComponents.h"

#include <glm/glm.hpp>
#include <utility>

namespace AE::Scene
{

Object::Object(ObjectData objectData)
    : data(std::move(objectData))
{
    if (data.className.empty())
    {
        data.className = GetClassName();
    }
}

const char* Object::GetClassName() const
{
    return "Object";
}

void Object::ConfigureEntity(Registry& ownerRegistry)
{
    registry = &ownerRegistry;
    entity = ownerRegistry.CreateEntity();
    entity.SetName(data.name);
    entity.AddComponent<SceneObjectComponent>(data.name, data.className, data.assetId, data.visible);
    entity.AddComponent<TransformComponent>(
        glm::vec2(data.transform.position.x, data.transform.position.y),
        glm::vec2(data.transform.scale.x, data.transform.scale.y),
        data.transform.rotationDegrees);
    entityConfigured = true;
    OnCreate();
}

void Object::OnCreate()
{
}

void Object::OnDestroy()
{
}

bool Object::HasEntity() const
{
    return entityConfigured;
}

Entity Object::GetEntity() const
{
    return entity;
}

Registry* Object::GetRegistry() const
{
    return registry;
}

const ObjectData& Object::GetData() const
{
    return data;
}

ObjectData& Object::GetData()
{
    return data;
}

const std::string& Object::GetName() const
{
    return data.name;
}

const std::string& Object::GetAssetId() const
{
    return data.assetId;
}

ObjectKind Object::GetKind() const
{
    return data.kind;
}

const Transform& Object::GetTransform() const
{
    return data.transform;
}

const Vec2& Object::GetSize() const
{
    return data.size;
}

bool Object::IsVisible() const
{
    return data.visible;
}

}
