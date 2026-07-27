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

EnemySpawnObject::EnemySpawnObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* EnemySpawnObject::GetClassName() const
{
    return "EnemySpawnObject";
}

void EnemySpawnObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<EnemySpawnComponent>();
}

PhysicsBoxObject::PhysicsBoxObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* PhysicsBoxObject::GetClassName() const
{
    return "PhysicsBoxObject";
}

void PhysicsBoxObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<PhysicsBoxComponent>();
}

PhysicsCircleObject::PhysicsCircleObject(AE::Scene::ObjectData data)
    : CircleObject(std::move(data))
{
    SetClassName(*this);
}

const char* PhysicsCircleObject::GetClassName() const
{
    return "PhysicsCircleObject";
}

void PhysicsCircleObject::ConfigureEntity(Registry& ownerRegistry)
{
    CircleObject::ConfigureEntity(ownerRegistry);
    AddComponent<PhysicsCircleComponent>();
}

PhysicsBridgeObject::PhysicsBridgeObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* PhysicsBridgeObject::GetClassName() const
{
    return "PhysicsBridgeObject";
}

void PhysicsBridgeObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<PhysicsBridgeComponent>();
}

PhysicsChainObject::PhysicsChainObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* PhysicsChainObject::GetClassName() const
{
    return "PhysicsChainObject";
}

void PhysicsChainObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<PhysicsChainComponent>();
}

MovingPlatformObject::MovingPlatformObject(AE::Scene::ObjectData data)
    : BoxObject(std::move(data))
{
    SetClassName(*this);
}

const char* MovingPlatformObject::GetClassName() const
{
    return "MovingPlatformObject";
}

void MovingPlatformObject::ConfigureEntity(Registry& ownerRegistry)
{
    BoxObject::ConfigureEntity(ownerRegistry);
    AddComponent<MovingPlatformComponent>();
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
    factory.RegisterClass("EnemySpawnObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<EnemySpawnObject>(std::move(data));
    });
    factory.RegisterClass("PhysicsBoxObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<PhysicsBoxObject>(std::move(data));
    });
    factory.RegisterClass("PhysicsCircleObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<PhysicsCircleObject>(std::move(data));
    });
    factory.RegisterClass("PhysicsBridgeObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<PhysicsBridgeObject>(std::move(data));
    });
    factory.RegisterClass("PhysicsChainObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<PhysicsChainObject>(std::move(data));
    });
    factory.RegisterClass("MovingPlatformObject", [](AE::Scene::ObjectData data) {
        return std::make_unique<MovingPlatformObject>(std::move(data));
    });
}

bool IsPlatformerGameplayClass(const std::string& className)
{
    return className == "PlayerSpawnObject" ||
        className == "GoalObject" ||
        className == "CoinObject" ||
        className == "SolidPlatformObject" ||
        className == "EnemySpawnObject" ||
        className == "PhysicsBoxObject" ||
        className == "PhysicsCircleObject" ||
        className == "PhysicsBridgeObject" ||
        className == "PhysicsChainObject" ||
        className == "MovingPlatformObject";
}

}
