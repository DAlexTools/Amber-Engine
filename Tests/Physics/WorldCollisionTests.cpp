#include "Core/BuildConfig.h"

#if C_UNIT_TEST

#include <gtest/gtest.h>

#include <cstdint>

#include "Core/Threading/ThreadPool.h"
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
}

TEST(BodyCollisionTests, RequiresBothCollisionMasksToAllowContact)
{
    AE::Physics::Body* player = CreateBoxBody(0.0f, 0.0f, 1.0f);
    AE::Physics::Body* enemy = CreateBoxBody(0.0f, 0.0f, 1.0f);

    player->collisionCategory = Player;
    player->collisionMask = Enemy;
    enemy->collisionCategory = Enemy;
    enemy->collisionMask = Player;

    EXPECT_TRUE(player->CanCollideWith(*enemy));

    enemy->collisionMask = EnemyProjectile;

    EXPECT_FALSE(player->CanCollideWith(*enemy));

    delete player;
    delete enemy;
}

TEST(WorldCollisionTests, FilteredPairsDoNotEmitContacts)
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

    EXPECT_TRUE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, SensorsEmitContactsWithoutPersistentConstraints)
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

    EXPECT_FALSE(world.GetContacts().empty());
    EXPECT_TRUE(world.GetConstraints().empty());
}

TEST(WorldCollisionTests, DynamicBodyRespondsToStaticObstacle)
{
    AE::Physics::World world(0.0f);

    AE::Physics::Body* player = CreateBoxBody(0.0f, 0.0f, 1.0f);
    player->collisionCategory = Player;
    player->collisionMask = Obstacle;
    player->velocity = AE::Physics::FVector2D(60.0f, 0.0f);

    AE::Physics::Body* obstacle = CreateBoxBody(9.0f, 0.0f, 0.0f);
    obstacle->collisionCategory = Obstacle;
    obstacle->collisionMask = Player;

    world.AddBody(player);
    world.AddBody(obstacle);
    world.Update(1.0f / 60.0f);

    EXPECT_FALSE(world.GetContacts().empty());
    EXPECT_TRUE(world.GetConstraints().empty());
    EXPECT_LE(player->velocity.X, 0.0f);
}

TEST(WorldCollisionTests, RemoveBodyClearsContactsThatReferenceRemovedBody)
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

    ASSERT_FALSE(world.GetContacts().empty());

    world.RemoveBody(enemyProjectile);

    EXPECT_TRUE(world.GetContacts().empty());
    ASSERT_EQ(world.GetBodies().size(), 1u);
    EXPECT_EQ(world.GetBodies().front(), player);
}

TEST(WorldCollisionTests, BroadPhaseCullsSeparatedBodiesBeforeNarrowPhase)
{
    AE::Physics::World world(0.0f);
    world.SetBroadPhaseCellSize(32.0f);

    for (int index = 0; index < 40; ++index)
    {
        AE::Physics::Body* body = CreateBoxBody(static_cast<float>(index) * 120.0f, 0.0f, 0.0f);
        body->collisionCategory = Obstacle;
        body->collisionMask = Obstacle;
        world.AddBody(body);
    }

    world.Update(1.0f / 60.0f);

    const AE::Physics::WorldStats& stats = world.GetLastStats();
    EXPECT_EQ(stats.bodyCount, 40u);
    EXPECT_EQ(stats.bruteForcePairs, 780u);
    EXPECT_LT(stats.broadPhasePairs, stats.bruteForcePairs);
    EXPECT_EQ(stats.narrowPhaseTests, 0u);
    EXPECT_TRUE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, BroadPhaseKeepsOverlappingPairAcrossCellBoundary)
{
    AE::Physics::World world(0.0f);
    world.SetBroadPhaseCellSize(16.0f);

    AE::Physics::CircleShape circleShape(10.0f);
    AE::Physics::Body* first = new AE::Physics::Body(circleShape, 63.0f, 0.0f, 1.0f);
    AE::Physics::Body* second = new AE::Physics::Body(circleShape, 78.0f, 0.0f, 1.0f);
    world.AddBody(first);
    world.AddBody(second);

    for (int index = 0; index < 20; ++index)
    {
        AE::Physics::Body* body = CreateBoxBody(300.0f + static_cast<float>(index) * 90.0f, 200.0f, 0.0f);
        world.AddBody(body);
    }

    world.Update(1.0f / 60.0f);

    const AE::Physics::WorldStats& stats = world.GetLastStats();
    EXPECT_LT(stats.broadPhasePairs, stats.bruteForcePairs);
    EXPECT_GE(stats.narrowPhaseTests, 1u);
    EXPECT_FALSE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, StaticStaticPairsAreFilteredBeforeNarrowPhase)
{
    AE::Physics::World world(0.0f);

    AE::Physics::Body* first = CreateBoxBody(0.0f, 0.0f, 0.0f);
    AE::Physics::Body* second = CreateBoxBody(0.0f, 0.0f, 0.0f);
    world.AddBody(first);
    world.AddBody(second);

    world.Update(1.0f / 60.0f);

    const AE::Physics::WorldStats& stats = world.GetLastStats();
    EXPECT_EQ(stats.staticPairFilteredPairs, 1u);
    EXPECT_EQ(stats.narrowPhaseTests, 0u);
    EXPECT_TRUE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, SleepingBodyOnStaticBodySkipsContactWork)
{
    AE::Physics::World world(0.0f);

    AE::Physics::Body* sleeper = CreateBoxBody(0.0f, 0.0f, 1.0f);
    AE::Physics::Body* floor = CreateBoxBody(0.0f, 0.0f, 0.0f);
    world.AddBody(sleeper);
    world.AddBody(floor);

    sleeper->sleeping = true;
    world.Update(1.0f / 60.0f);

    const AE::Physics::WorldStats& stats = world.GetLastStats();
    EXPECT_EQ(stats.sleepingPairFilteredPairs, 1u);
    EXPECT_EQ(stats.narrowPhaseTests, 0u);
    EXPECT_TRUE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, AwakeDynamicBodyWakesSleepingDynamicBody)
{
    AE::Physics::World world(0.0f);

    AE::Physics::Body* sleeper = CreateBoxBody(0.0f, 0.0f, 1.0f);
    AE::Physics::Body* mover = CreateBoxBody(0.0f, 0.0f, 1.0f);
    mover->velocity = AE::Physics::FVector2D(40.0f, 0.0f);
    world.AddBody(sleeper);
    world.AddBody(mover);

    sleeper->sleeping = true;
    world.Update(1.0f / 60.0f);

    EXPECT_FALSE(sleeper->sleeping);
    EXPECT_GE(world.GetLastStats().narrowPhaseTests, 1u);
    EXPECT_FALSE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, MovingStaticBodyWakesSleepingDynamicBody)
{
    AE::Physics::World world(0.0f);

    AE::Physics::Body* sleeper = CreateBoxBody(0.0f, 0.0f, 1.0f);
    AE::Physics::Body* movingPlatform = CreateBoxBody(0.0f, 0.0f, 0.0f);
    movingPlatform->velocity = AE::Physics::FVector2D(30.0f, 0.0f);
    world.AddBody(sleeper);
    world.AddBody(movingPlatform);

    sleeper->sleeping = true;
    world.Update(1.0f / 60.0f);

    EXPECT_FALSE(sleeper->sleeping);
    EXPECT_GE(world.GetLastStats().narrowPhaseTests, 1u);
    EXPECT_FALSE(world.GetContacts().empty());
}

TEST(WorldCollisionTests, SolverIslandStatsKeepIndependentDynamicStaticContactsSeparate)
{
    AE::Physics::World world(0.0f);
    world.SetBroadPhaseEnabled(false);
    world.SetSleepingEnabled(false);
    world.SetParallelNarrowPhaseEnabled(false);

    world.AddBody(CreateBoxBody(0.0f, 0.0f, 1.0f));
    world.AddBody(CreateBoxBody(0.0f, 0.0f, 0.0f));
    world.AddBody(CreateBoxBody(100.0f, 0.0f, 1.0f));
    world.AddBody(CreateBoxBody(100.0f, 0.0f, 0.0f));

    world.Update(1.0f / 60.0f);

    const AE::Physics::WorldStats& stats = world.GetLastStats();
    EXPECT_EQ(stats.solverIslandCount, 2u);
    EXPECT_EQ(stats.largestSolverIslandBodyCount, 1u);
    EXPECT_GT(stats.solverConstraintCount, 0u);
    EXPECT_GT(stats.largestSolverIslandConstraintCount, 0u);
}

TEST(WorldCollisionTests, ParallelSolverMatchesSequentialForIndependentIslands)
{
    struct SolverProbe
    {
        AE::Physics::FVector2D firstPosition;
        AE::Physics::FVector2D secondPosition;
        AE::Physics::FVector2D firstVelocity;
        AE::Physics::FVector2D secondVelocity;
        AE::Physics::WorldStats stats;
    };

    const auto runProbe = [](bool parallelSolver)
    {
        AE::Physics::World world(0.0f);
        world.SetBroadPhaseEnabled(false);
        world.SetSleepingEnabled(false);
        world.SetParallelNarrowPhaseEnabled(false);
        world.SetParallelSolverEnabled(parallelSolver);
        world.SetParallelSolverMinConstraints(1u);
        world.SetSolverIterations(6);

        AE::Physics::Body* firstDynamic = CreateBoxBody(0.0f, 0.0f, 1.0f);
        AE::Physics::Body* firstStatic = CreateBoxBody(0.0f, 0.0f, 0.0f);
        AE::Physics::Body* secondDynamic = CreateBoxBody(100.0f, 0.0f, 1.0f);
        AE::Physics::Body* secondStatic = CreateBoxBody(100.0f, 0.0f, 0.0f);

        world.AddBody(firstDynamic);
        world.AddBody(firstStatic);
        world.AddBody(secondDynamic);
        world.AddBody(secondStatic);
        world.Update(1.0f / 60.0f);

        return SolverProbe{
            firstDynamic->position,
            secondDynamic->position,
            firstDynamic->velocity,
            secondDynamic->velocity,
            world.GetLastStats()
        };
    };

    AE::Threading::ThreadPool& threadPool = AE::Threading::ThreadPool::Get();
    const std::size_t originalWorkerCount = threadPool.WorkerCount();
    threadPool.SetWorkerCount(4u);

    const SolverProbe sequential = runProbe(false);
    const SolverProbe parallel = runProbe(true);

    EXPECT_EQ(parallel.stats.solverIslandCount, sequential.stats.solverIslandCount);
    EXPECT_EQ(parallel.stats.solverConstraintCount, sequential.stats.solverConstraintCount);
    EXPECT_NEAR(parallel.firstPosition.X, sequential.firstPosition.X, 0.0001f);
    EXPECT_NEAR(parallel.firstPosition.Y, sequential.firstPosition.X, 0.0001f);
    EXPECT_NEAR(parallel.secondPosition.X, sequential.secondPosition.X, 0.0001f);
    EXPECT_NEAR(parallel.secondPosition.Y, sequential.secondPosition.Y, 0.0001f);
    EXPECT_NEAR(parallel.firstVelocity.X, sequential.firstVelocity.X, 0.0001f);
    EXPECT_NEAR(parallel.firstVelocity.Y, sequential.firstVelocity.Y, 0.0001f);
    EXPECT_NEAR(parallel.secondVelocity.X, sequential.secondVelocity.X, 0.0001f);
    EXPECT_NEAR(parallel.secondVelocity.Y, sequential.secondVelocity.Y, 0.0001f);

    if (threadPool.WorkerCount() > 1u)
    {
        EXPECT_TRUE(parallel.stats.parallelSolverUsed);
        EXPECT_GT(parallel.stats.parallelSolverJobs, 1u);
    }

    threadPool.SetWorkerCount(originalWorkerCount);
}

TEST(WorldCollisionTests, ParallelNarrowPhaseMatchesSequentialContactCount)
{
    AE::Threading::ThreadPool& threadPool = AE::Threading::ThreadPool::Get();
    const std::size_t originalWorkerCount = threadPool.WorkerCount();
    threadPool.SetWorkerCount(8u);

    AE::Physics::World sequentialWorld(0.0f);
    sequentialWorld.SetBroadPhaseEnabled(false);
    sequentialWorld.SetSleepingEnabled(false);
    sequentialWorld.SetParallelNarrowPhaseEnabled(false);
    AE::Physics::CircleShape shape(100.0f);
    for (int index = 0; index < 5; ++index)
    {
        sequentialWorld.AddBody(new AE::Physics::Body(shape, static_cast<float>(index) * 4.0f, 0.0f, 1.0f));
    }
    sequentialWorld.CheckCollisions();

    AE::Physics::World parallelWorld(0.0f);
    parallelWorld.SetBroadPhaseEnabled(false);
    parallelWorld.SetSleepingEnabled(false);
    parallelWorld.SetParallelNarrowPhaseEnabled(true);
    parallelWorld.SetParallelNarrowPhaseMinPairs(2u);
    for (int index = 0; index < 5; ++index)
    {
        parallelWorld.AddBody(new AE::Physics::Body(shape, static_cast<float>(index) * 4.0f, 0.0f, 1.0f));
    }
    parallelWorld.CheckCollisions();

    const AE::Physics::WorldStats& sequentialStats = sequentialWorld.GetLastStats();
    const AE::Physics::WorldStats& parallelStats = parallelWorld.GetLastStats();

    EXPECT_EQ(parallelStats.narrowPhaseTests, sequentialStats.narrowPhaseTests);
    EXPECT_EQ(parallelWorld.GetContacts().size(), sequentialWorld.GetContacts().size());
    EXPECT_EQ(parallelStats.contactCount, sequentialStats.contactCount);

    if (threadPool.WorkerCount() > 1u)
    {
        EXPECT_TRUE(parallelStats.parallelNarrowPhaseUsed);
        EXPECT_GT(parallelStats.parallelNarrowPhaseJobs, 1u);
    }

    threadPool.SetWorkerCount(originalWorkerCount);
}

#endif
