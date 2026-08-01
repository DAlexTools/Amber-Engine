#include "Physics/Particles/Particle.h"
#include <iostream>
#include "Logging/Logger.h"

namespace AE::Physics
{

Particle::Particle(float x, float y, float mass)
{
	this->position = FVector2D(x, y);
	this->mass = mass;

	if (mass != 0.0)
	{
		this->invMass = 1.0 / mass;
	}
	else
	{
		this->invMass = 0.0;
	}
	AE::Logger::Log("Particle constructor called", "Physics");
}

Particle::~Particle()
{
	AE::Logger::Log("Particle destructor called!", "Physics");
}

void Particle::AddForce(const FVector2D& force)
{
	sumForces += force;
}

void Particle::ClearForces()
{
	sumForces = FVector2D(0.0, 0.0);
}

void Particle::Integrate(float dt)
{
	// Find the acceleration based on the forces that are being applied and the mass
	acceleration = sumForces * invMass;

	// Integrate the acceleration to find the new velocity
	velocity += acceleration * dt;

	// Integrate the velocity to find the acceleration
	position += velocity * dt;

	ClearForces();
}

} // namespace AE::Physics
