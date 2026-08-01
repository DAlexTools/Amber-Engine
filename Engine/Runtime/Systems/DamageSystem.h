#ifndef DAMAGESYSTEM_H
#define DAMAGESYSTEM_H

#include <algorithm>
#include <utility>
#include <vector>

#include "../EntityComponentSystem/ECS.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/ProjectileComponent.h"
#include "../Components/HealthComponent.h"
#include "../Events/CollisionEvent.h"
#include "../EventBus/EventBus.h"

class DamageSystem : public System
{
public:
	DamageSystem()
	{
		RequireComponent<BoxCollisionComponent>();
	}

	void SubscribeToEvents(std::unique_ptr<EventBus>& eventBus)
	{
		processedProjectileCollisions.clear();
		eventBus->SubscribeToEvent<CollisionEvent>(this, &DamageSystem::OnCollision);
	}

	void OnCollision(CollisionEvent& event)
	{
		Entity a = event.a;
		Entity b = event.b;
		AE::Logger::Log(std::string(event.fromPhysics ? "Physics" : "AABB") + " collision event emitted: " + std::to_string(a.GetID()) + " and " + std::to_string(b.GetID()));

		if (!event.fromPhysics && IsProjectileDamagePair(a, b))
		{
			AE::Logger::Log("Ignoring AABB projectile damage collision; projectile damage is handled by physics contacts.");
			return;
		}

		if (TryApplyProjectileHit(a, b))
		{
			return;
		}

		TryApplyProjectileHit(b, a);
	}

	void OnProjectileHitsPlayer(Entity projectile, Entity player)
	{
		if (!projectile.HasComponent<ProjectileComponent>() || !player.HasComponent<HealthComponent>())
		{
			return;
		}

		const auto projectileComponent = projectile.GetComponent<ProjectileComponent>();

		if (!projectileComponent.isFriendly)
		{
			// Reduce the health of the player by the projectile hitPercentDamage
			auto& health = player.GetComponent<HealthComponent>();

			// Subtract the health of the player
			health.healthPercentage -= projectileComponent.hitPercentDamage;

			// Kills the player when health reaches zero
			if (health.healthPercentage <= 0)
			{
				player.Kill();
			}

			// Kill the projectile
			projectile.Kill();
		}
	}

	void OnProjectileHitsEnemy(Entity projectile, Entity enemy)
	{
		if (!projectile.HasComponent<ProjectileComponent>() || !enemy.HasComponent<HealthComponent>())
		{
			return;
		}

		const auto projectileComponent = projectile.GetComponent<ProjectileComponent>();

		// Only damage the enemy if projectile is friendly
		if (projectileComponent.isFriendly)
		{
			auto& health = enemy.GetComponent<HealthComponent>();

			// Subtract from enemy health
			health.healthPercentage -= projectileComponent.hitPercentDamage;

			// Kills the enemy if health reaches zero
			if (health.healthPercentage <= 0)
			{
				enemy.Kill();
			}

			// Destroy projectile
			projectile.Kill();
		}
	}

private:
	std::vector<std::pair<int, int>> processedProjectileCollisions;

	bool TryApplyProjectileHit(Entity projectile, Entity target)
	{
		if (!projectile.BelongsToGroup("projectiles") || !projectile.HasComponent<ProjectileComponent>())
		{
			return false;
		}

		if (!target.HasTag("player") && !target.BelongsToGroup("enemies"))
		{
			return false;
		}

		if (HasProcessedProjectileCollision(projectile, target))
		{
			return true;
		}

		if (target.HasTag("player"))
		{
			OnProjectileHitsPlayer(projectile, target);
			return true;
		}

		OnProjectileHitsEnemy(projectile, target);
		return true;
	}

	bool HasProcessedProjectileCollision(Entity projectile, Entity target)
	{
		const auto pair = MakeEntityPair(projectile, target);
		if (std::find(processedProjectileCollisions.begin(), processedProjectileCollisions.end(), pair) != processedProjectileCollisions.end())
		{
			return true;
		}

		processedProjectileCollisions.push_back(pair);
		return false;
	}

	std::pair<int, int> MakeEntityPair(Entity a, Entity b) const
	{
		const int aID = a.GetID();
		const int bID = b.GetID();
		return aID < bID
				   ? std::make_pair(aID, bID)
				   : std::make_pair(bID, aID);
	}

	bool IsProjectileDamagePair(Entity a, Entity b) const
	{
		return (IsProjectile(a) && IsProjectileTarget(b)) ||
			   (IsProjectile(b) && IsProjectileTarget(a));
	}

	bool IsProjectile(Entity entity) const
	{
		return entity.BelongsToGroup("projectiles") && entity.HasComponent<ProjectileComponent>();
	}

	bool IsProjectileTarget(Entity entity) const
	{
		return entity.HasTag("player") || entity.BelongsToGroup("enemies");
	}
};

#endif
