#ifndef COLLISIONEVENT_H
#define COLLISIONEVENT_H

#include "../EntityComponentSystem/ECS.h"
#include "../EventBus/Event.h"

class CollisionEvent : public Event
{
public:
	Entity a;
	Entity b;
	bool fromPhysics;

	CollisionEvent(Entity a, Entity b, bool fromPhysics = false) : a(a), b(b), fromPhysics(fromPhysics) {}
};


#endif
