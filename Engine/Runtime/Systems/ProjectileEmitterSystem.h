#ifndef PROJECTILEEMMITERSYSTEM_H
#define PROJECTILEEMMITERSYSTEM_H

#include "../EntityComponentSystem/ECS.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/ProjectileComponent.h"
#include "../Components/CameraFollowComponent.h"
#include "../EnginePhysicsBridge/EnginePhysicsBridge.h"
#include "../EventBus/EventBus.h"
#include "../Events/KeyPressedEvent.h"
#include <SDL2/SDL.h>

/**
 *
 */
class ProjectileEmitterSystem : public System
{
public:
	ProjectileEmitterSystem()
	{
		RequireComponent<ProjectileEmitterComponent>();
		RequireComponent<TransformComponent>();
	}

	void SubscribeToEvents(std::unique_ptr<EventBus>& eventBus)
	{
		eventBus->SubscribeToEvent<KeyPressedEvent>(this, &ProjectileEmitterSystem::OnKeyPressed);
	}

	void OnKeyPressed(KeyPressedEvent& event)
	{
		if (event.symbol == SDLK_SPACE)
		{
			for (auto entity : GetSystemEntity())
			{
				if (entity.HasTag("player"))
				{
					const auto projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
					const auto transform = entity.GetComponent<TransformComponent>();
					const auto rigidbody = entity.GetComponent<RigidBodyComponent>();

					// If parent entity has sprite, start the projectile position in the middle of the entity
					glm::vec2 projectilePosition = transform.position;
					if (entity.HasComponent<SpriteComponent>())
					{
						auto sprite = entity.GetComponent<SpriteComponent>();
						projectilePosition.x += (transform.scale.x * sprite.width / 2);
						projectilePosition.y += (transform.scale.y * sprite.height / 2);
					}

					// If parent entity direction is controlled by the keyboard keys, modify the direction of the projectile accordingly
					glm::vec2 projectileVelocity = projectileEmitter.projectileVelocity;
					int directionX = 0;
					int directionY = 0;
					if (rigidbody.velocity.x > 0)
						directionX = +1;
					if (rigidbody.velocity.x < 0)
						directionX = -1;
					if (rigidbody.velocity.y > 0)
						directionY = +1;
					if (rigidbody.velocity.y < 0)
						directionY = -1;

					projectileVelocity.x = projectileEmitter.projectileVelocity.x * directionX;
					projectileVelocity.y = projectileEmitter.projectileVelocity.y * directionY;

					CreateProjectile(
						*entity.registry,
						projectilePosition,
						projectileVelocity,
						projectileEmitter.isFriendly,
						projectileEmitter.hitPercentDamage,
						projectileEmitter.projectileDuration);
				}
			}
		}
	}

	void Update(std::unique_ptr<Registry>& registry)
	{
		for (auto entity : GetSystemEntity())
		{
			auto& projectileEmitter = entity.GetComponent<ProjectileEmitterComponent>();
			const auto transform = entity.GetComponent<TransformComponent>();

			// If emission frequency is zero, bypass re-emission logic
			if (projectileEmitter.repeatFrequency == 0)
				continue;

			const int currentTicks = static_cast<int>(SDL_GetTicks());
			if (projectileEmitter.lastEmissionTime < 0)
			{
				projectileEmitter.lastEmissionTime = currentTicks;
			}

			// Check if its time to re-emit a new projectile
			if (currentTicks - projectileEmitter.lastEmissionTime > projectileEmitter.repeatFrequency)
			{
				glm::vec2 projectilePosition = transform.position;

				if (entity.HasComponent<SpriteComponent>())
				{
					const auto sprite = entity.GetComponent<SpriteComponent>();
					projectilePosition.x += (transform.scale.x * sprite.width / 2);
					projectilePosition.y += (transform.scale.y * sprite.height / 2);
				}

				CreateProjectile(
					*registry,
					projectilePosition,
					projectileEmitter.projectileVelocity,
					projectileEmitter.isFriendly,
					projectileEmitter.hitPercentDamage,
					projectileEmitter.projectileDuration);

				// Update the projectile emitter component last emission to the current milliseconds
				projectileEmitter.lastEmissionTime = currentTicks;
			}
		}
	}

private:
	void CreateProjectile(
		Registry& registry,
		const glm::vec2& position,
		const glm::vec2& velocity,
		bool isFriendly,
		int hitPercentDamage,
		int duration)
	{
		Entity projectile = registry.CreateEntity();
		projectile.Group("projectiles");

		TransformComponent transform(position, glm::vec2(1.0f, 1.0f), 0.0);
		projectile.AddComponent<TransformComponent>(transform);
		projectile.AddComponent<RigidBodyComponent>(velocity);
		projectile.AddComponent<SpriteComponent>("bullet-texture", 4, 4, 4);
		projectile.AddComponent<BoxCollisionComponent>(4, 4);
		projectile.AddComponent<ProjectileComponent>(isFriendly, hitPercentDamage, duration, static_cast<int>(SDL_GetTicks()));

		if (registry.HasSystem<PhysicsWorldSystem>())
		{
			PhysicsBodyDefinition bodyDefinition;
			bodyDefinition.shapeType = PhysicsBodyShapeType::Box;
			bodyDefinition.mass = 1.0f;
			bodyDefinition.width = 4.0f;
			bodyDefinition.height = 4.0f;
			bodyDefinition.velocity = velocity;
			bodyDefinition.collisionCategory = isFriendly
												   ? EnginePhysicsCollision::PlayerProjectile
												   : EnginePhysicsCollision::EnemyProjectile;
			bodyDefinition.collisionMask = isFriendly
											   ? EnginePhysicsCollision::Enemy
											   : EnginePhysicsCollision::Player;
			bodyDefinition.isSensor = true;

			auto& physicsWorldSystem = registry.GetSystem<PhysicsWorldSystem>();
			PhysicsBodyComponent physicsBody = PhysicsBodyFactory::Create(
				physicsWorldSystem,
				transform,
				bodyDefinition);

			projectile.AddComponent<PhysicsBodyComponent>(physicsBody);
		}
	}
};

#endif
