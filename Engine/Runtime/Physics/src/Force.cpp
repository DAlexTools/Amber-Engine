#include "Force.h"
#include "Constants.h"
#include <algorithm>
#include <iostream>
#include "Core/Math/PhysicsMath.h"

namespace AE::Physics
{

/**
 * 
 */
Vector2D Force::GenerateDragForce(const Body& Body, float k)
{
    Vector2D dragForce = Vector2D::Zero;

    if (Body.velocity.MagnitudeSquared() > AE::Physics::ZERO)
    {
        /* Calculate the drag direction (inverse of velocity unit vector).*/
        Vector2D dragDirection = Body.velocity.UnitVector() * AE::Physics::NEGATIVE_FLOAT;
        
        /* Calculate the drag magnitude , k * Lvl^2.*/
        float dragMagnitude = k * Body.velocity.MagnitudeSquared();
        
        /* Generate the final drag force with direction and magnitude.*/
        dragForce = dragDirection * dragMagnitude;
    }

    return dragForce;
}

Vector2D Force::GenerateDragForce(const Particle& particle, float k) 
{
    Vector2D dragForce = Vector2D(0, 0);
    if (particle.velocity.MagnitudeSquared() > 0) 
    {
        // Calculate the drag direction (inverse of velocity unit vector)
        Vector2D dragDirection = particle.velocity.UnitVector() * -1.0;

        // Calculate the drag magnitude, k * |v|^2
        float dragMagnitude = k * particle.velocity.MagnitudeSquared();

        // Generate the final drag force with direction and magnitude
        dragForce = dragDirection * dragMagnitude;
    }
    return dragForce;
}


/**
 * 
 */
Vector2D Force::GenerateFrictionForce(const Body& Body, float k)
{
    Vector2D frictionForce = Vector2D::Zero;

    /* Calculate the friction direction (inverse of velocity unit vector) */
    const Vector2D frictionDirection = Body.velocity.UnitVector() * AE::Physics::NEGATIVE_FLOAT;

    /* Calculate the friction Magnitude */
    float frictionMagnitude = k;

    /* Calculate the final friction force */
    frictionForce = frictionDirection * frictionMagnitude;

    return frictionForce;
}

/**
 * 
 */
Vector2D Force::GenerateGravitationalForce(const Body& a, const Body& b, float G, float minDistance, float maxDistance)
{
    /* Calculate the distance between the two objects.*/
    const Vector2D d = (b.position - a.position);

    float distanceSquared = d.MagnitudeSquared();

    /* Clamp the values of the distance (to allow for some insteresting visual effects)*/
    /* distanceSquared = std::clamp(distanceSquared, minDistance, maxDistance); use std library or Math library */
    distanceSquared = Math::Clamp(distanceSquared, minDistance, maxDistance);

    /* Calculate the direction of the attraction force*/
    Vector2D attractionDirection = d.UnitVector();

    /* Calculate the strength of the attraction force */
    const float attractionMagnitude = G * (a.mass * b.mass) / distanceSquared;

    /* Calculate the final resulting attraction force vector*/
    const Vector2D attractionForce = attractionDirection * attractionMagnitude;

    return attractionForce; 
}

/**
 * 
 */
Vector2D Force::GenerateSpringForce(const Body& Body, Vector2D anchor, float restLength, float k) 
{
    /* Calculate the distance between the anchor and the object*/   
    const Vector2D d = Body.position - anchor;

    /* Find the spring displacement considering the rest length*/
    const float displacement = d.Magnitude() - restLength;

    /* Calculate the direction and the magnitude of the spring force */
    const Vector2D springDirection = d.UnitVector();
    const float springMagnitude = -k * displacement;

    /* Calculate the final resulting spring force vector*/
    const Vector2D springForce = springDirection * springMagnitude;
    return springForce;
}

/**
 * 
 */
Vector2D Force::GenerateSpringForce(const Body& a, const Body& b, float restLength, float k)
{
    /* Calculate the distance between the two Body */
    const Vector2D d = a.position - b.position;

    /* Find the spring displacement considering the rest length */
    const float displacement = d.Magnitude() - restLength;

    /* Calculate the direction of the spring force */
    const Vector2D springDirection = d.UnitVector();

    /* Calculate the magnitude of the spring force */   
    const float springMagnutude = -k * displacement;

    /* Calculate the final resulting spring force vector*/
    const Vector2D springForce = springDirection * springMagnutude;

    return springForce;
}

Vector2D Force::GenerateSpringForce(const Particle& particle, Vector2D anchor, float restLength, float k) 
{
    // Calculate the distance between the anchor and the object
    Vector2D d = particle.position - anchor;

    // Find the spring displacement considering the rest length
    float displacement = d.Magnitude() - restLength;

    // Calculate the direction of the spring force
    Vector2D springDirection = d.UnitVector();

    // Calculate the magnitude of the spring force
    float springMagnitude = -k * displacement;

    // Calculate the final resulting spring force vector
    Vector2D springForce = springDirection * springMagnitude;

    return springForce;
}

Vector2D Force::GenerateSpringForce(const Particle& a, const Particle& b, float restLength, float k) 
{
    // Calculate the distance between the two particles
    Vector2D d = a.position - b.position;

    // Find the spring displacement considering the rest length
    float displacement = d.Magnitude() - restLength;

    // Calculate the direction of the spring force
    Vector2D springDirection = d.UnitVector();

    // Calculate the magnitude of the spring force
    float springMagnitude = -k * displacement;

    // Calculate the final resulting spring force vector
    Vector2D springForce = springDirection * springMagnitude;

    return springForce;
}

}
