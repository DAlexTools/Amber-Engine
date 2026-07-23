#ifndef PHYSICSBODYLIFECYCLESYSTEM_H
#define PHYSICSBODYLIFECYCLESYSTEM_H

#include <memory>

#include "../Components/PhysicsBodyComponent.h"
#include "../EntityComponentSystem/ECS.h"
#include "PhysicsWorldSystem.h"

class PhysicsBodyLifecycleSystem : public System
{
public:
    PhysicsBodyLifecycleSystem()
    {
        RequireComponent<PhysicsBodyComponent>();
    }

    void Update(const std::unique_ptr<Registry>& registry, PhysicsWorldSystem& physicsWorldSystem)
    {
        for (auto entity : registry->GetEntitiesToBeKilled())
        {
            if (!entity.HasComponent<PhysicsBodyComponent>())
            {
                continue;
            }

            auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();
            physicsWorldSystem.RemoveBody(physicsBody.body);
            physicsBody.body = nullptr;
        }
    }
};

#endif
