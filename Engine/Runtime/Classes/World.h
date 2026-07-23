#ifndef WORLD_H
#define WORLD_H

#include "Physics/Objects/Body.h"
#include "Physics/Constraint.h"
#include "Physics/Contact.h"
#include <cstddef>
#include <utility>
#include <vector>

namespace AE::Physics
{

struct WorldStats
{
    std::size_t bodyCount = 0;
    std::size_t bruteForcePairs = 0;
    std::size_t broadPhasePairs = 0;
    std::size_t maskFilteredPairs = 0;
    std::size_t narrowPhaseTests = 0;
    std::size_t contactCount = 0;
    std::size_t penetrationConstraintCount = 0;
    std::size_t staticPairFilteredPairs = 0;
    std::size_t sleepingPairFilteredPairs = 0;
    std::size_t sleepingBodyCount = 0;
    std::size_t parallelNarrowPhaseJobs = 0;
    std::size_t solverConstraintCount = 0;
    std::size_t solverIslandCount = 0;
    std::size_t largestSolverIslandBodyCount = 0;
    std::size_t largestSolverIslandConstraintCount = 0;
    std::size_t parallelSolverJobs = 0;
    bool parallelNarrowPhaseUsed = false;
    bool parallelSolverUsed = false;
    int solverIterations = 0;
    double forcePhaseMs = 0.0;
    double broadPhaseMs = 0.0;
    double narrowPhaseMs = 0.0;
    double solverPhaseMs = 0.0;
    double velocityPhaseMs = 0.0;
    double sleepingPhaseMs = 0.0;
    double totalStepMs = 0.0;
};

class World
{
private:
    struct SolverIsland
    {
        std::vector<Constraint*> constraints;
        std::vector<PenetrationConstraint*> penetrations;
        std::size_t bodyCount = 0;

        std::size_t ConstraintCount() const
        {
            return constraints.size() + penetrations.size();
        }
    };

    float G = 9.8f;
    std::vector<Body*> bodies;
    std::vector<Constraint*> constraints;
    std::vector<Contact> lastContacts;

    std::vector<Vector2D> forces;
    std::vector<float> torques;
    WorldStats lastStats;
    bool broadPhaseEnabled = true;
    float broadPhaseCellSize = 32.0f;
    bool sleepingEnabled = true;
    float sleepLinearVelocityThreshold = 12.0f;
    float sleepAngularVelocityThreshold = 0.05f;
    float sleepTimeThreshold = 0.6f;
    bool parallelNarrowPhaseEnabled = true;
    std::size_t parallelNarrowPhaseMinPairs = 256u;
    bool parallelSolverEnabled = true;
    std::size_t parallelSolverMinConstraints = 64u;
    int solverIterations = 10;

    std::vector<std::pair<std::size_t, std::size_t>> BuildPotentialPairs();
    std::vector<std::pair<std::size_t, std::size_t>> BuildNarrowPhasePairs(
        const std::vector<std::pair<std::size_t, std::size_t>>& potentialPairs);
    void DetectContacts(
        const std::vector<std::pair<std::size_t, std::size_t>>& narrowPhasePairs,
        std::vector<Contact>& contactsOut);
    void DetectContactsSequential(
        const std::vector<std::pair<std::size_t, std::size_t>>& narrowPhasePairs,
        std::vector<Contact>& contactsOut);
    void DetectContactsParallel(
        const std::vector<std::pair<std::size_t, std::size_t>>& narrowPhasePairs,
        std::vector<Contact>& contactsOut);
    std::vector<SolverIsland> BuildSolverIslands(std::vector<PenetrationConstraint>& penetrations);
    void UpdateSolverIslandStats(const std::vector<SolverIsland>& islands);
    void SolveConstraints(
        float dt,
        std::vector<PenetrationConstraint>& penetrations,
        std::vector<SolverIsland>& islands);
    void SolveConstraintsSequential(float dt, std::vector<PenetrationConstraint>& penetrations);
    void SolveConstraintsParallel(float dt, std::vector<SolverIsland>& islands);
    void SolveIsland(float dt, SolverIsland& island);
    bool ShouldSkipCollisionPair(Body& first, Body& second);
    void UpdateSleepingBodies(float dt);

public:
    World(float gravity);
    ~World();

    void AddBody(Body* body);
    void RemoveBody(Body* body);
    std::vector<Body*>& GetBodies();

    void AddConstraint(Constraint* constraint);
    std::vector<Constraint*>& GetConstraints();

    std::vector<Contact>& GetContacts();
    const std::vector<Contact>& GetContacts() const;
    const WorldStats& GetLastStats() const;

    void SetBroadPhaseEnabled(bool enabled);
    bool IsBroadPhaseEnabled() const;
    void SetBroadPhaseCellSize(float cellSize);
    float GetBroadPhaseCellSize() const;
    void SetSolverIterations(int iterations);
    int GetSolverIterations() const;
    void SetSleepingEnabled(bool enabled);
    bool IsSleepingEnabled() const;
    void SetSleepThresholds(float linearVelocity, float angularVelocity, float time);
    void SetParallelNarrowPhaseEnabled(bool enabled);
    bool IsParallelNarrowPhaseEnabled() const;
    void SetParallelNarrowPhaseMinPairs(std::size_t minPairs);
    std::size_t GetParallelNarrowPhaseMinPairs() const;
    void SetParallelSolverEnabled(bool enabled);
    bool IsParallelSolverEnabled() const;
    void SetParallelSolverMinConstraints(std::size_t minConstraints);
    std::size_t GetParallelSolverMinConstraints() const;
    void WakeBody(Body& body);
    void WakeAllBodies();

    void AddForce(const Vector2D& force);
    void AddTorque(float torque);

    void Update(float dt);

    void CheckCollisions();
};

}

using AE::Physics::World;

#endif
