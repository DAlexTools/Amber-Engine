#ifndef PHYSICSVELOCITYSYSTEM_H
#define PHYSICSVELOCITYSYSTEM_H

#include "../Components/PhysicsBodyComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../EntityComponentSystem/ECS.h"
#include "Physics/Objects/Body.h"
#include "Core/Math/Vector2D.h"

class PhysicsVelocitySystem : public System
{
public:
    PhysicsVelocitySystem()
    {
        RequireComponent<PhysicsBodyComponent>();
        RequireComponent<RigidBodyComponent>();
    }

    void Update()
    {
        for (auto entity : GetSystemEntity())
        {
            auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();
            const auto rigidBody = entity.GetComponent<RigidBodyComponent>();

            if (!physicsBody.body)
            {
                continue;
            }

            if (physicsBody.body->IsStatic())
            {
                physicsBody.body->velocity = AE::Physics::Vector2D::Zero;
                continue;
            }

            physicsBody.body->velocity = AE::Physics::Vector2D(
                rigidBody.velocity.x,
                rigidBody.velocity.y);
        }
    }
};

#endif
