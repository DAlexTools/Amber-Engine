#ifndef COLLISIONDETECTION_H
#define COLLISIONDETECTION_H

#include "Physics/Collision/Contact.h"
#include "Physics/Dynamics/Body.h"

namespace AE::Physics
{

struct CollisionDetection
{
	static bool IsColliding(Body* a, Body* b, std::vector<Contact>& contacts);
	static bool IsCollidingCircleCircle(Body* a, Body* b, std::vector<Contact>& contacts);
	static bool IsCollidingPolygonPolygon(Body* a, Body* b, std::vector<Contact>& contacts);
	static bool IsCollidingPolygonCircle(Body* polygon, Body* circle, std::vector<Contact>& contacts);
};

} // namespace AE::Physics

using AE::Physics::CollisionDetection;

#endif
