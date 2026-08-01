#ifndef PHYSICSCONTACTEVENTSYSTEM_H
#define PHYSICSCONTACTEVENTSYSTEM_H

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "../Components/PhysicsBodyComponent.h"
#include "../EntityComponentSystem/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "PhysicsWorldSystem.h"

class PhysicsContactEventSystem : public System
{
public:
	PhysicsContactEventSystem()
	{
		RequireComponent<PhysicsBodyComponent>();
	}

	void Update(std::unique_ptr<EventBus>& eventBus, PhysicsWorldSystem& physicsWorldSystem)
	{
		std::vector<std::pair<int, int>> emittedPairs;

		for (const auto& contact : physicsWorldSystem.GetWorld().GetContacts())
		{
			Entity a = FindEntityByBody(contact.a);
			Entity b = FindEntityByBody(contact.b);

			if (a == b || a.GetID() < 0 || b.GetID() < 0)
			{
				continue;
			}

			const auto pair = MakeEntityPair(a, b);
			if (std::find(emittedPairs.begin(), emittedPairs.end(), pair) != emittedPairs.end())
			{
				continue;
			}

			emittedPairs.push_back(pair);
			eventBus->EmitEvent<CollisionEvent>(a, b, true);
		}
	}

private:
	Entity FindEntityByBody(const AE::Physics::Body* body) const
	{
		for (auto entity : GetSystemEntity())
		{
			const auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();
			if (physicsBody.body == body)
			{
				return entity;
			}
		}

		return Entity(-1);
	}

	std::pair<int, int> MakeEntityPair(Entity a, Entity b) const
	{
		const int aID = a.GetID();
		const int bID = b.GetID();
		return aID < bID
				   ? std::make_pair(aID, bID)
				   : std::make_pair(bID, aID);
	}
};

#endif
