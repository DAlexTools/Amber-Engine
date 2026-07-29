#pragma once

#include "Core/Platform/PlatformTypes.h"
#include "Core/Math/Vector2D.h"
#include "Shape.h"

namespace AE::Physics
{

struct Body
{
	// Linear motion
	FVector2D position;
	FVector2D velocity;
	FVector2D acceleration;

	// Angular motion
	float rotation;
	float angularVelocity;
	float angularAcceleration;

	// Forces and torque
	FVector2D sumForces;
	float sumTorque;

	// Mass and Moment of Inertia
	float mass;
	float invMass;
	float I;
	float invI;

	// Coefficient of restitution (elasticity)
	float restitution;

	// Coefficient of friction
	float friction;

	// Pointer to the shape/geometry of this rigid body
	Shape* shape = nullptr;

	uint32 collisionCategory = 0x00000001u;
	uint32 collisionMask = 0xFFFFFFFFu;
	bool isSensor = false;
	bool canSleep = true;
	bool sleeping = false;
	float sleepTime = 0.0f;

	Body(const Shape& shape, float x, float y, float mass);
	~Body();

	bool IsStatic() const;
	bool CanCollideWith(const Body& other) const;

	void AddForce(const FVector2D& force);
	void AddTorque(float torque);
	void ClearForces();
	void ClearTorque();

	FVector2D LocalSpaceToWorldSpace(const FVector2D& point) const;
	FVector2D WorldSpaceToLocalSpace(const FVector2D& point) const;

	void ApplyImpulseLinear(const FVector2D& j);
	void ApplyImpulseAngular(const float j);
	void ApplyImpulseAtPoint(const FVector2D& j, const FVector2D& r);

	void IntegrateLinear(float dt);
	void IntegrateAngular(float dt);

	void IntegrateForces(const float dt);
	void IntegrateVelocities(const float dt);
};

} // namespace AE::Physics

using AE::Physics::Body;
