#include "DamageSystem.h"
#include "../EnginePhysicsBridge/EnginePhysicsBridge.h"

void CompileCheckPhysicsBridge()
{
    PhysicsWorldSystem physicsWorldSystem;
    PhysicsBodyLifecycleSystem physicsBodyLifecycleSystem;
    PhysicsContactEventSystem physicsContactEventSystem;
    PhysicsSyncSystem physicsSyncSystem;
    PhysicsVelocitySystem physicsVelocitySystem;
    DamageSystem damageSystem;
    PhysicsBodyDefinition bodyDefinition;
    TransformComponent transform;

    bodyDefinition.collisionCategory = EnginePhysicsCollision::Enemy;
    bodyDefinition.collisionMask = EnginePhysicsCollision::PlayerProjectile;
    bodyDefinition.isSensor = true;
    bodyDefinition.collisionCategory = EnginePhysicsCollision::FromName("enemy");
    bodyDefinition.collisionMask = EnginePhysicsCollision::FromName("player-projectile");

    PhysicsBodyComponent physicsBody = PhysicsBodyFactory::Create(
        physicsWorldSystem,
        transform,
        bodyDefinition);

    (void)physicsBody.body;
    (void)physicsWorldSystem.GetWorld();
    (void)physicsBodyLifecycleSystem.GetSystemEntity();
    (void)physicsContactEventSystem.GetSystemEntity();
    (void)physicsSyncSystem.GetSystemEntity();
    (void)physicsVelocitySystem.GetSystemEntity();
    (void)damageSystem.GetSystemEntity();
}
