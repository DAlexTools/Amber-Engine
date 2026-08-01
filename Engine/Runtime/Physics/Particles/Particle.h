#ifndef PARTICLE_H
#define PARTICLE_H

#include "Core/Math/Vector2D.h"

namespace AE::Physics
{

struct Particle
{
	Particle(float x, float y, float mass);
	~Particle();

	int radius;
	float mass;
	float invMass;

	FVector2D position;
	FVector2D velocity;
	FVector2D acceleration;
	FVector2D sumForces;

	void AddForce(const FVector2D& force);
	void ClearForces();
	void Integrate(float dt);
};

} // namespace AE::Physics

using AE::Physics::Particle;

#endif
