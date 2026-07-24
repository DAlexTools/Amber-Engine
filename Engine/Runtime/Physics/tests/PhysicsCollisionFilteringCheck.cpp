#include "Core/BuildConfig.h"

#if SMOKE_TEST

#include <cassert>

#include "Physics/Objects/Body.h"
#include "Physics/Objects/Shape.h"
#include "Classes/World.h"

namespace
{
    constexpr std::uint32_t Player = 1u << 0;
    constexpr std::uint32_t Enemy = 1u << 1;
    constexpr std::uint32_t PlayerProjectile = 1u << 2;
    constexpr std::uint32_t EnemyProjectile = 1u << 3;
    constexpr std::uint32_t Obstacle = 1u << 4;

    AE::Physics::Body* CreateBoxBody(float x, float y, float mass)
    {
        AE::Physics::BoxShape shape(10.0f, 10.0f);
        return new AE::Physics::Body(shape, x, y, mass);
    }

    void CheckFilteredPairsDoNotCollide()
    {
        AE::Physics::World world(0.0f);

        AE::Physics::Body* enemy = CreateBoxBody(0.0f, 0.0f, 0.0f);
        enemy->collisionCategory = Enemy;
        enemy->collisionMask = PlayerProjectile;

        AE::Physics::Body* enemyProjectile = CreateBoxBody(0.0f, 0.0f, 1.0f);
        enemyProjectile->collisionCategory = EnemyProjectile;
        enemyProjectile->collisionMask = Player;
        enemyProjectile->isSensor = true;

        world.AddBody(enemy);
        world.AddBody(enemyProjectile);
        world.Update(1.0f / 60.0f);

        assert(world.GetContacts().empty());
    }

    void CheckSensorsEmitContactsWithoutConstraints()
    {
        AE::Physics::World world(0.0f);

        AE::Physics::Body* player = CreateBoxBody(0.0f, 0.0f, 0.0f);
        player->collisionCategory = Player;
        player->collisionMask = EnemyProjectile;
        player->isSensor = true;

        AE::Physics::Body* enemyProjectile = CreateBoxBody(0.0f, 0.0f, 1.0f);
        enemyProjectile->collisionCategory = EnemyProjectile;
        enemyProjectile->collisionMask = Player;
        enemyProjectile->isSensor = true;

        world.AddBody(player);
        world.AddBody(enemyProjectile);
        world.Update(1.0f / 60.0f);

        assert(!world.GetContacts().empty());
        assert(world.GetConstraints().empty());
    }

    void CheckDynamicBodyRespondsToStaticObstacle()
    {
        AE::Physics::World world(0.0f);

        AE::Physics::Body* player = CreateBoxBody(0.0f, 0.0f, 1.0f);
        player->collisionCategory = Player;
        player->collisionMask = Obstacle;
        player->velocity = AE::Physics::Vector2D(60.0f, 0.0f);

        AE::Physics::Body* obstacle = CreateBoxBody(9.0f, 0.0f, 0.0f);
        obstacle->collisionCategory = Obstacle;
        obstacle->collisionMask = Player;

        world.AddBody(player);
        world.AddBody(obstacle);
        world.Update(1.0f / 60.0f);

        assert(!world.GetContacts().empty());
        assert(world.GetConstraints().empty());
        assert(player->velocity.x <= 0.0f);
    }
}

int main()
{
    CheckFilteredPairsDoNotCollide();
    CheckSensorsEmitContactsWithoutConstraints();
    CheckDynamicBodyRespondsToStaticObstacle();
    return 0;
}

#endif
