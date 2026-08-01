#ifndef FORCE_H
#define FORCE_H

#include "Core/Math/Vector2D.h"
#include "Physics/Dynamics/Body.h"
#include "Physics/Particles/Particle.h"

namespace AE::Physics
{

/**
 * Force struct
 */
struct Force
{
	static FVector2D GenerateDragForce(const Body& Body, float k);
	static FVector2D GenerateDragForce(const Particle& particle, float k);
	static FVector2D GenerateFrictionForce(const Body& Body, float k);
	static FVector2D GenerateGravitationalForce(const Body& a, const Body& b, float G, float minDistance, float maxDistance);
	static FVector2D GenerateSpringForce(const Body& Body, FVector2D anchor, float restLength, float k);
	static FVector2D GenerateSpringForce(const Body& a, const Body& b, float restLength, float k);
	static FVector2D GenerateSpringForce(const Particle& a, const Particle& b, float restLength, float k);
	static FVector2D GenerateSpringForce(const Particle& particle, FVector2D anchor, float restLength, float k);
};

} // namespace AE::Physics

using AE::Physics::Force;

#endif
