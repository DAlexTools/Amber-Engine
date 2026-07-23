#ifndef PHYSICSBODYCOMPONENT_H
#define PHYSICSBODYCOMPONENT_H

#include <glm/glm.hpp>

namespace AE::Physics
{
    struct Body;
}

struct PhysicsBodyComponent
{
    AE::Physics::Body* body = nullptr;
    bool pullPositionFromPhysics = true;
    bool pullRotationFromPhysics = true;
    glm::vec2 localCenterOffset = glm::vec2(0.0f, 0.0f);

    PhysicsBodyComponent(
        AE::Physics::Body* body = nullptr,
        bool pullPositionFromPhysics = true,
        bool pullRotationFromPhysics = true,
        glm::vec2 localCenterOffset = glm::vec2(0.0f, 0.0f))
    {
        this->body = body;
        this->pullPositionFromPhysics = pullPositionFromPhysics;
        this->pullRotationFromPhysics = pullRotationFromPhysics;
        this->localCenterOffset = localCenterOffset;
    }
};

#endif
