#ifndef PLATFORMER_SCENE_OBJECTS_H
#define PLATFORMER_SCENE_OBJECTS_H

#include "Core/Platform/PlatformTypes.h"
#include "Scene/PrimitiveObjects.h"

#include <string>

namespace AE::Scene
{
class ObjectFactory;
}

namespace PlatformerScene
{

struct FPlayerSpawnComponent
{
};

struct FGoalComponent
{
};

struct FCoinComponent
{
};

struct FSolidPlatformComponent
{
};

struct FEnemySpawnComponent
{
    float Speed = 70.0f;
    float Direction = 1.0f;
    float PatrolWidth = 192.0f;
    int32 MaxHealth = 2;
    float JumpCooldown = 1.2f;
    float JumpVelocity = -340.0f;
    float AlertRange = 260.0f;
    bool CanShoot = false;
    float ShootCooldown = 1.45f;
    float ShootRange = 340.0f;
    float ProjectileSpeed = 360.0f;
};

struct FPhysicsBoxComponent
{
    float Mass = 1.1f;
    float Friction = 0.24f;
    float Restitution = 0.08f;
};

struct FPhysicsCircleComponent
{
    float Mass = 0.85f;
    float Friction = 0.05f;
    float Restitution = 0.32f;
};

struct FPhysicsBridgeComponent
{
    int32 SegmentCount = 10;
};

struct FPhysicsChainComponent
{
    int32 LinkCount = 7;
};

struct FMovingPlatformComponent
{
    bool VerticalMotion = false;
    float Amplitude = 92.0f;
    float Speed = 1.15f;
    float Phase = 0.0f;
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
