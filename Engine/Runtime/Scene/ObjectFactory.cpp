#include "Scene/ObjectFactory.h"

#include "Scene/PrimitiveObjects.h"
#include "Scene/SpriteObject.h"

#include <utility>

namespace AE::Scene
{
namespace
{
    const char* DefaultClassNameForKind(ObjectKind kind)
    {
        switch (kind)
        {
            case ObjectKind::AssetInstance:
                return "SpriteObject";
            case ObjectKind::Box:
                return "BoxObject";
            case ObjectKind::Circle:
                return "CircleObject";
            default:
                return "Object";
        }
    }
}

ObjectFactory::ObjectFactory()
{
    RegisterClass("Object", [](ObjectData data) {
        return std::make_unique<Object>(std::move(data));
    });
    RegisterClass("SpriteObject", [](ObjectData data) {
        return std::make_unique<SpriteObject>(std::move(data));
    });
    RegisterClass("BoxObject", [](ObjectData data) {
        return std::make_unique<BoxObject>(std::move(data));
    });
    RegisterClass("CircleObject", [](ObjectData data) {
        return std::make_unique<CircleObject>(std::move(data));
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
        objectData.className = DefaultClassNameForKind(objectData.kind);
    }

    const auto creator = creators.find(objectData.className);
    std::unique_ptr<Object> object;
    if (creator != creators.end())
    {
        object = creator->second(std::move(objectData));
    }
    else
    {
        const auto defaultCreator = creators.find(DefaultClassNameForKind(objectData.kind));
        object = defaultCreator != creators.end() ?
            defaultCreator->second(std::move(objectData)) :
            std::make_unique<Object>(std::move(objectData));
    }

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
