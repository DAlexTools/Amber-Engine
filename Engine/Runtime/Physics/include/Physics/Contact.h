#ifndef CONTACT_H
#define CONTACT_H

#include "Core/Math/Vector2D.h"
#include "Objects/Body.h"

namespace AE::Physics
{

/**
 * Contact struct 
 */
struct Contact
{
    Body* a;
    Body* b;

    Vector2D start;
    Vector2D end;

    Vector2D normal;
    float depth;

    Contact() = default;
    ~Contact() = default;

    void ResolvePenetration();
    void ResolveCollision();
};

}

using AE::Physics::Contact;

#endif
