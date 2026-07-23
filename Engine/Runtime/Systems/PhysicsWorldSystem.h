#ifndef PHYSICSWORLDSYSTEM_H
#define PHYSICSWORLDSYSTEM_H

#include <memory>

#include "../Components/PhysicsBodyComponent.h"
#include "../EntityComponentSystem/ECS.h"
#include "Physics/Constraint.h"
#include "Classes/World.h"
#include "Physics/Objects/Body.h"
#include "Physics/Objects/Shape.h"

class PhysicsWorldSystem : public System
{
private:
    std::unique_ptr<AE::Physics::World> world;

public:
    explicit PhysicsWorldSystem(float gravity = -9.8f)
    {
        RequireComponent<PhysicsBodyComponent>();
        world = std::make_unique<AE::Physics::World>(gravity);
    }

    AE::Physics::World& GetWorld()
    {
        return *world;
    }

    const AE::Physics::World& GetWorld() const
    {
        return *world;
    }

    AE::Physics::Body* CreateBody(const AE::Physics::Shape& shape, float x, float y, float mass)
    {
        AE::Physics::Body* body = new AE::Physics::Body(shape, x, y, mass);
        world->AddBody(body);
        return body;
    }

    void AddBody(AE::Physics::Body* body)
    {
        world->AddBody(body);
    }

    void RemoveBody(AE::Physics::Body* body)
    {
        world->RemoveBody(body);
    }

    void AddConstraint(AE::Physics::Constraint* constraint)
    {
        world->AddConstraint(constraint);
    }

    void Update(double deltaTime)
    {
        world->Update(static_cast<float>(deltaTime));
    }
};

#endif
