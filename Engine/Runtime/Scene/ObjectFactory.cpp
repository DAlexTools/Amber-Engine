#include "Scene/ObjectFactory.h"

#include "Scene/SpriteObject.h"

#include <utility>

namespace AE::Scene
{

ObjectFactory::ObjectFactory()
{
    RegisterClass("Object", [](ObjectData data) {
        return std::make_unique<Object>(std::move(data));
    });
    RegisterClass("SpriteObject", [](ObjectData data) {
        return std::make_unique<SpriteObject>(std::move(data));
    });
}

void ObjectFactory::RegisterClass(std::string className, Creator creator)
{
    creators[std::move(className)] = std::move(creator);
}

std::unique_ptr<Object> ObjectFactory::CreateObject(const ObjectData& data, Registry* registry) const
{
    ObjectData objectData = data;
    if (objectData.className.empty())
    {
        objectData.className = objectData.kind == ObjectKind::AssetInstance ? "SpriteObject" : "Object";
    }

    const auto creator = creators.find(objectData.className);
    std::unique_ptr<Object> object = creator != creators.end() ?
        creator->second(std::move(objectData)) :
        std::make_unique<Object>(std::move(objectData));

    if (registry)
    {
        object->ConfigureEntity(*registry);
    }

    return object;
}

std::vector<std::unique_ptr<Object>> ObjectFactory::CreateObjects(const Document& document, Registry* registry) const
{
    std::vector<std::unique_ptr<Object>> objects;
    objects.reserve(document.objects.size());
    for (const ObjectData& objectData : document.objects)
    {
        objects.push_back(CreateObject(objectData, registry));
    }

    return objects;
}

}
