#ifndef PHYSICSCOLLISIONLAYERS_H
#define PHYSICSCOLLISIONLAYERS_H

#include <algorithm>
#include <cctype>
#include <string>

#include "Core/Platform/PlatformTypes.h"

namespace EnginePhysicsCollision
{
constexpr uint32 Player = 1u << 0;
constexpr uint32 Enemy = 1u << 1;
constexpr uint32 PlayerProjectile = 1u << 2;
constexpr uint32 EnemyProjectile = 1u << 3;
constexpr uint32 Obstacle = 1u << 4;
constexpr uint32 All = 0xFFFFFFFFu;

inline std::string NormalizeLayerName(std::string layerName)
{
	std::transform(layerName.begin(), layerName.end(), layerName.begin(), [](unsigned char character)
				   {
					if (character == '-')
					{
						return '_';
					}
					return static_cast<char>(std::tolower(character)); });
	return layerName;
}

inline uint32 FromName(const std::string& layerName, uint32 defaultValue = All)
{
	const std::string normalizedName = NormalizeLayerName(layerName);
	if (normalizedName == "player")
	{
		return Player;
	}
	if (normalizedName == "enemy")
	{
		return Enemy;
	}
	if (normalizedName == "player_projectile")
	{
		return PlayerProjectile;
	}
	if (normalizedName == "enemy_projectile")
	{
		return EnemyProjectile;
	}
	if (normalizedName == "obstacle")
	{
		return Obstacle;
	}
	if (normalizedName == "all")
	{
		return All;
	}

	return defaultValue;
}
} // namespace EnginePhysicsCollision

#endif
