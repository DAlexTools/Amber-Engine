#include "Physics/Dynamics/Body.h"
#include <math.h>
#include <iostream>

#include "Physics/Core/Constants.h"
#include "Logging/Logger.h"

namespace AE::Physics
{

namespace BodyConstants
{
float Init_Friction = 0.7;
float Init_Restitution = 0.6f;
const float Epsilon = 0.005f;
}; // namespace BodyConstants

Body::Body(const Shape& shape, float x, float y, float mass)
{
	this->shape = shape.Clone();
	this->position = FVector2D(x, y);
	this->velocity = FVector2D::Zero;
	this->acceleration = FVector2D::Zero;
	this->rotation = AE::Physics::ZERO;
	this->angularVelocity = AE::Physics::ZERO;
	this->angularAcceleration = AE::Physics::ZERO;
	this->sumForces = FVector2D::Zero;
	this->sumTorque = AE::Physics::ZERO;
	this->restitution = BodyConstants::Init_Restitution;
	this->friction = BodyConstants::Init_Friction;
	this->mass = mass;

	if (mass != AE::Physics::ZERO)
	{
		this->invMass = AE::Physics::POSITIVE / mass;
	}
	else
	{
		this->invMass = AE::Physics::ZERO_FLOAT;
	}
	I = shape.GetMomentOfInertia() * mass;
	if (I != AE::Physics::ZERO_FLOAT)
	{
		this->invI = AE::Physics::POSITIVE_FLOAT / I;
	}
	else
	{
		this->invI = AE::Physics::ZERO_FLOAT;
	}
	this->shape->UpdateVertices(rotation, position);

	AE::Logger::Log("Body constructor called", "Physics");
}

Body::~Body()
{
	delete shape;
	AE::Logger::Log("Body destructor called", "Physics");
}

bool Body::IsStatic() const
{
	const float epsilon = BodyConstants::Epsilon;
	return fabs(invMass - AE::Physics::ZERO_FLOAT) < epsilon;
}

bool Body::CanCollideWith(const Body& other) const
{
	return (collisionMask & other.collisionCategory) != 0 &&
		   (other.collisionMask & collisionCategory) != 0;
}

void Body::AddForce(const FVector2D& force)
{
	sumForces += force;
}

void Body::AddTorque(float torque)
{
	sumTorque += torque;
}

void Body::ClearForces()
{
	sumForces = FVector2D::Zero;
}

void Body::ClearTorque()
{
	sumTorque = AE::Physics::ZERO_FLOAT;
}

FVector2D Body::LocalSpaceToWorldSpace(const FVector2D& point) const
{
	FVector2D rotated = point.Rotate(rotation);
	return rotated + position;
}

FVector2D Body::WorldSpaceToLocalSpace(const FVector2D& point) const
{
	float translatedX = point.X - position.X;
	float translatedY = point.Y - position.Y;
	float rotatedX = cos(-rotation) * translatedX - sin(-rotation) * translatedY;
	float rotatedY = cos(-rotation) * translatedY + sin(-rotation) * translatedX;
	return FVector2D(rotatedX, rotatedY);
}

void Body::ApplyImpulseLinear(const FVector2D& j)
{
	if (IsStatic())
		return;
	velocity += j * invMass;
}

void Body::ApplyImpulseAngular(const float j)
{
	if (IsStatic())
		return;
	angularVelocity += j * invI;
}

void Body::ApplyImpulseAtPoint(const FVector2D& j, const FVector2D& r)
{
	if (IsStatic())
		return;
	velocity += j * invMass;
	angularVelocity += r.CrossProduct(j) * invI;
}

void Body::IntegrateForces(const float dt)
{
	if (IsStatic())
		return;

	// Find the acceleration based on the forces that are being applied and the mass
	acceleration = sumForces * invMass;

	// Integrate the acceleration to find the new velocity
	velocity += acceleration * dt;

	// Find the angular acceleration based on the torque that is being applied and the moment of inertia
	angularAcceleration = sumTorque * invI;

	// Integrate the angular acceleration to find the new angular velocity
	angularVelocity += angularAcceleration * dt;

	// Clear all the forces and torque acting on the object before the next physics step
	ClearForces();
	ClearTorque();
}

void Body::IntegrateVelocities(const float dt)
{
	if (IsStatic())
		return;

	// Integrate the velocity to find the new position
	position += velocity * dt;

	// Integrate the angular velocity to find the new rotation angle
	rotation += angularVelocity * dt;

	// Update the vertices to adjust them to the new position/rotation
	shape->UpdateVertices(rotation, position);
}

} // namespace AE::Physics
