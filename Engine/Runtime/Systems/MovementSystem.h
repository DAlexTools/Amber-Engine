#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H

#include "../EntityComponentSystem/ECS.h"
#include "../Components/PhysicsBodyComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/TransformComponent.h"
#include "../Game/Game.h"



class MovementSystem : public System
{
	
	public:
		MovementSystem()
		{
			RequireComponent<TransformComponent>();
			RequireComponent<RigidBodyComponent>();
		}

		void Update(double deltaTime)
		{
			for (auto entity : GetSystemEntity())
			{
				auto& transform = entity.GetComponent<TransformComponent>();
				const auto rigidbody = entity.GetComponent<RigidBodyComponent>();
				bool isMovedByPhysics = false;

				if (entity.HasComponent<PhysicsBodyComponent>())
				{
					const auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();
					isMovedByPhysics = physicsBody.body && physicsBody.pullPositionFromPhysics;
				}

				if (!isMovedByPhysics)
				{
					transform.position.x += rigidbody.velocity.x * deltaTime;
					transform.position.y += rigidbody.velocity.y * deltaTime;
				}

				if (entity.HasTag("player"))
				{
					int paddingLeft = 10;
					int paddingTop = 10;
					int paddingRight = 50;
					int paddingBottom = 50;
					transform.position.x = transform.position.x < paddingLeft ? paddingLeft : transform.position.x;
					transform.position.x = transform.position.x > Game::MapWidth - paddingRight ? Game::MapWidth - paddingRight : transform.position.x;
					transform.position.y = transform.position.y < paddingTop ? paddingTop : transform.position.y;
					transform.position.y = transform.position.y > Game::MapHeight - paddingBottom ? Game::MapHeight - paddingBottom : transform.position.y;
				}

				bool isEntityOutsideMap = 
				(
					transform.position.x < 0 || 
					transform.position.x > Game::MapWidth ||
					transform.position.y < 0 || 
					transform.position.y > Game::MapHeight
				);

				if (isEntityOutsideMap && !entity.HasTag("player"))
				{
					entity.Kill();
				}
			}
		}
};


#endif
