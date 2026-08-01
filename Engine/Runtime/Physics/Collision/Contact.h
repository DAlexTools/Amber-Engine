#ifndef CONTACT_H
#define CONTACT_H

#include "Core/Math/Vector2D.h"
#include "Physics/Dynamics/Body.h"

namespace AE::Physics
{

/**
 * Contact struct
 */
struct Contact
{
	Body* a;
	Body* b;

	FVector2D start;
	FVector2D end;

	FVector2D normal;
	float depth;

	Contact() = default;
	~Contact() = default;

	void ResolvePenetration();
	void ResolveCollision();
};

} // namespace AE::Physics

using AE::Physics::Contact;

#endif
