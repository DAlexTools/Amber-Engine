#ifndef PROJECTILELIFECYCLESYSTEM_H
#define PROJECTILELIFECYCLESYSTEM_H

#include "../EntityComponentSystem/ECS.h"
#include "../Components/ProjectileComponent.h"
#include <SDL2/SDL.h>

/**
 *
 */
class ProjectileLifeCycleSystem : public System
{
public:
	/**
	 *
	 */
	ProjectileLifeCycleSystem()
	{
		RequireComponent<ProjectileComponent>();
	}

	/**
	 *
	 */
	void Update()
	{
		for (auto entity : GetSystemEntity())
		{
			auto& projectile = entity.GetComponent<ProjectileComponent>();
			const int currentTicks = static_cast<int>(SDL_GetTicks());

			if (projectile.startTime < 0)
			{
				projectile.startTime = currentTicks;
			}

			if (currentTicks - projectile.startTime > projectile.duration)
			{
				entity.Kill();
			}
		}
	}
};
#endif
