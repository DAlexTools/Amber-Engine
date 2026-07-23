#ifndef PARTICLE_H
#define PARTICLE_H

#include "Core/Math/Vector2D.h"

namespace AE::Physics
{

struct Particle 
{
    Particle(float x, float y, float mass);
    ~Particle();

    int         radius;
    float       mass;
    float       invMass;

    Vector2D    position;
    Vector2D    velocity;
    Vector2D    acceleration;
    Vector2D    sumForces;

    void        AddForce(const Vector2D& force);
    void        ClearForces();
    void        Integrate(float dt);
};

}

using AE::Physics::Particle;

#endif
