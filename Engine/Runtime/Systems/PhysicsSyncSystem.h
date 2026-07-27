#ifndef PHYSICSSYNCSYSTEM_H
#define PHYSICSSYNCSYSTEM_H

#include "../Components/PhysicsBodyComponent.h"
#include "../Components/TransformComponent.h"
#include "../EntityComponentSystem/ECS.h"
#include "Physics/Objects/Body.h"
#include "PhysicsTransformConversions.h"

class PhysicsSyncSystem : public System
{
public:
    PhysicsSyncSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<PhysicsBodyComponent>();
    }

    void PullFromPhysics()
    {
        for (auto entity : GetSystemEntity())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();

            if (!physicsBody.body)
            {
                continue;
            }

            if (physicsBody.pullPositionFromPhysics)
            {
                transform.position.x = physicsBody.body->position.X - physicsBody.localCenterOffset.x;
                transform.position.y= physicsBody.body->position.Y- physicsBody.localCenterOffset.y;
            }

            if (physicsBody.pullRotationFromPhysics)
            {
                transform.rotation = EnginePhysics::RadiansToDegrees(physicsBody.body->rotation);
            }
        }
    }

    void PushToPhysics()
    {
        for (auto entity : GetSystemEntity())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();

            if (!physicsBody.body)
            {
                continue;
            }

            physicsBody.body->position = AE::Physics::FVector2D(
                transform.position.x + physicsBody.localCenterOffset.x,
                transform.position.y + physicsBody.localCenterOffset.y);
            physicsBody.body->rotation = EnginePhysics::DegreesToRadians(static_cast<float>(transform.rotation));
            if (physicsBody.body->shape)
            {
                physicsBody.body->shape->UpdateVertices(physicsBody.body->rotation, physicsBody.body->position);
            }
        }
    }
};

#endif
