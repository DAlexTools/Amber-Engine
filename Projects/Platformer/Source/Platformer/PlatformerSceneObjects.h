#ifndef PLATFORMER_SCENE_OBJECTS_H
#define PLATFORMER_SCENE_OBJECTS_H

#include "Scene/PrimitiveObjects.h"

#include <string>

namespace AE::Scene
{
class ObjectFactory;
}

namespace PlatformerScene
{

struct PlayerSpawnComponent
{
};

struct GoalComponent
{
};

struct CoinComponent
{
};

struct SolidPlatformComponent
{
};

struct EnemySpawnComponent
{
};

struct PhysicsBoxComponent
{
};

struct PhysicsCircleComponent
{
};

struct PhysicsBridgeComponent
{
};

struct PhysicsChainComponent
{
};

struct MovingPlatformComponent
{
};

class PlayerSpawnObject final : public AE::Scene::BoxObject
{
public:
    explicit PlayerSpawnObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class GoalObject final : public AE::Scene::BoxObject
{
public:
    explicit GoalObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class CoinObject final : public AE::Scene::CircleObject
{
public:
    explicit CoinObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class SolidPlatformObject final : public AE::Scene::BoxObject
{
public:
    explicit SolidPlatformObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class EnemySpawnObject final : public AE::Scene::BoxObject
{
public:
    explicit EnemySpawnObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class PhysicsBoxObject final : public AE::Scene::BoxObject
{
public:
    explicit PhysicsBoxObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class PhysicsCircleObject final : public AE::Scene::CircleObject
{
public:
    explicit PhysicsCircleObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class PhysicsBridgeObject final : public AE::Scene::BoxObject
{
public:
    explicit PhysicsBridgeObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class PhysicsChainObject final : public AE::Scene::BoxObject
{
public:
    explicit PhysicsChainObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class MovingPlatformObject final : public AE::Scene::BoxObject
{
public:
    explicit MovingPlatformObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

void RegisterPlatformerSceneObjects(AE::Scene::ObjectFactory& factory);
bool IsPlatformerGameplayClass(const std::string& className);

}

#endif
