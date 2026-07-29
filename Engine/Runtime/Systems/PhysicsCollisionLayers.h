#ifndef PHYSICSCOLLISIONLAYERS_H
#define PHYSICSCOLLISIONLAYERS_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

namespace EnginePhysicsCollision
{
constexpr std::uint32_t Player = 1u << 0;
constexpr std::uint32_t Enemy = 1u << 1;
constexpr std::uint32_t PlayerProjectile = 1u << 2;
constexpr std::uint32_t EnemyProjectile = 1u << 3;
constexpr std::uint32_t Obstacle = 1u << 4;
constexpr std::uint32_t All = 0xFFFFFFFFu;

inline std::string NormalizeLayerName(std::string layerName)
{
	std::transform(layerName.begin(), layerName.end(), layerName.begin(), [](unsigned char character)
				{
					if (character == '-')
					{
						return '_';
					}
					return static_cast<char>(std::tolower(character)); 
				});
	return layerName;
}

inline std::uint32_t FromName(const std::string& layerName, std::uint32_t defaultValue = All)
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
