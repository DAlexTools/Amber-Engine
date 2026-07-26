#include "PlatformerSceneObjects.h"

#include "Scene/ObjectFactory.h"

#include <memory>
#include <utility>

namespace PlatformerScene
{
namespace
{
    void SetClassName(AE::Scene::Object& object)
    {
        object.GetData().className = object.GetClassName();
    }
}

PlayerSpawnObject::PlayerSpawnObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* PlayerSpawnObject::GetClassName() const
{
    return "PlayerSpawnObject";
}

void PlayerSpawnObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<PlayerSpawnComponent>();
}

GoalObject::GoalObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* GoalObject::GetClassName() const
{
    return "GoalObject";
}

void GoalObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<GoalComponent>();
}

CoinObject::CoinObject(AE::Scene::ObjectData data)
    : CircleObject(std::move(data))
{
    SetClassName(*this);
}

const char* CoinObject::GetClassName() const
{
    return "CoinObject";
}

void CoinObject::ConfigureEntity(Registry& ownerRegistry)
{
    CircleObject::ConfigureEntity(ownerRegistry);
    AddComponent<CoinComponent>();
}

SolidPlatformObject::SolidPlatformObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* SolidPlatformObject::GetClassName() const
{
    return "SolidPlatformObject";
}

void SolidPlatformObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<SolidPlatformComponent>();
}

void RegisterPlatformerSceneObjects(AE::Scene::ObjectFactory& factory)
{
    factory.RegisterClass("PlayerSpawnObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<PlayerSpawnObject>(std::move(data));
    });
    factory.RegisterClass("GoalObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<GoalObject>(std::move(data));
    });
    factory.RegisterClass("CoinObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<CoinObject>(std::move(data));
    });
    factory.RegisterClass("SolidPlatformObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<SolidPlatformObject>(std::move(data));
    });
}

bool IsPlatformerGameplayClass(const std::string& className)
{
    return className == "PlayerSpawnObject" ||
        className == "GoalObject" ||
        className == "CoinObject" ||
        className == "SolidPlatformObject";
}

}
