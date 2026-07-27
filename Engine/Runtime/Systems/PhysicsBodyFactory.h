#ifndef PHYSICSBODYFACTORY_H
#define PHYSICSBODYFACTORY_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include "../Components/PhysicsBodyComponent.h"
#include "../Components/TransformComponent.h"
#include "PhysicsCollisionLayers.h"
#include "PhysicsTransformConversions.h"
#include "PhysicsWorldSystem.h"

enum class PhysicsBodyShapeType
{
    Box,
    Circle
};

struct PhysicsBodyDefinition
{
    PhysicsBodyShapeType shapeType = PhysicsBodyShapeType::Box;
    float mass = 1.0f;
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    glm::vec2 offset = glm::vec2(0.0f, 0.0f);
    glm::vec2 velocity = glm::vec2(0.0f, 0.0f);
    float angularVelocity = 0.0f;
    bool pullPositionFromPhysics = true;
    bool pullRotationFromPhysics = true;
    std::uint32_t collisionCategory = EnginePhysicsCollision::Player;
    std::uint32_t collisionMask = EnginePhysicsCollision::All;
    bool isSensor = false;
};

class PhysicsBodyFactory
{
public:
    static PhysicsBodyShapeType ShapeTypeFromString(std::string shapeName)
    {
        std::transform(shapeName.begin(), shapeName.end(), shapeName.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });

        if (shapeName == "circle")
        {
            return PhysicsBodyShapeType::Circle;
        }

        return PhysicsBodyShapeType::Box;
    }

    static PhysicsBodyComponent Create(
        PhysicsWorldSystem& worldSystem,
        const TransformComponent& transform,
        const PhysicsBodyDefinition& definition)
    {
        glm::vec2 localCenterOffset = definition.offset;
        AE::Physics::Body* body = nullptr;

        if (definition.shapeType == PhysicsBodyShapeType::Circle)
        {
            localCenterOffset += glm::vec2(definition.radius, definition.radius);
            AE::Physics::CircleShape shape(definition.radius);
            body = worldSystem.CreateBody(
                shape,
                transform.position.x + localCenterOffset.x,
                transform.position.y + localCenterOffset.y,
                definition.mass);
        }
        else
        {
            localCenterOffset += glm::vec2(definition.width * 0.5f, definition.height * 0.5f);
            AE::Physics::BoxShape shape(definition.width, definition.height);
            body = worldSystem.CreateBody(
                shape,
                transform.position.x + localCenterOffset.x,
                transform.position.y + localCenterOffset.y,
                definition.mass);
        }

        body->rotation = EnginePhysics::DegreesToRadians(static_cast<float>(transform.rotation));
        body->angularVelocity = EnginePhysics::DegreesToRadians(definition.angularVelocity);
        body->velocity = AE::Physics::FVector2D(definition.velocity.x, definition.velocity.y);
        body->collisionCategory = definition.collisionCategory;
        body->collisionMask = definition.collisionMask;
        body->isSensor = definition.isSensor;
        body->shape->UpdateVertices(body->rotation, body->position);

        return PhysicsBodyComponent(
            body,
            definition.pullPositionFromPhysics,
            definition.pullRotationFromPhysics,
            localCenterOffset);
    }
};

#endif
