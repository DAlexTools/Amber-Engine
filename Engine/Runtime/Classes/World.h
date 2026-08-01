#ifndef WORLD_H
#define WORLD_H

#include "Physics/Dynamics/Body.h"
#include "Core/Platform/PlatformTypes.h"
#include "Physics/Constraints/Constraint.h"
#include "Physics/Collision/Contact.h"
#include <utility>
#include <vector>

namespace AE::Physics
{

struct WorldStats
{
	SizeT bodyCount = 0;
	SizeT bruteForcePairs = 0;
	SizeT broadPhasePairs = 0;
	SizeT maskFilteredPairs = 0;
	SizeT narrowPhaseTests = 0;
	SizeT contactCount = 0;
	SizeT penetrationConstraintCount = 0;
	SizeT staticPairFilteredPairs = 0;
	SizeT sleepingPairFilteredPairs = 0;
	SizeT sleepingBodyCount = 0;
	SizeT parallelNarrowPhaseJobs = 0;
	SizeT solverConstraintCount = 0;
	SizeT solverIslandCount = 0;
	SizeT largestSolverIslandBodyCount = 0;
	SizeT largestSolverIslandConstraintCount = 0;
	SizeT parallelSolverJobs = 0;
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
		SizeT bodyCount = 0;

		SizeT ConstraintCount() const
		{
			return constraints.size() + penetrations.size();
		}
	};

	float G = 9.8f;
	std::vector<Body*> bodies;
	std::vector<Constraint*> constraints;
	std::vector<Contact> lastContacts;

	std::vector<FVector2D> forces;
	std::vector<float> torques;
	WorldStats lastStats;
	bool broadPhaseEnabled = true;
	float broadPhaseCellSize = 32.0f;
	bool sleepingEnabled = true;
	float sleepLinearVelocityThreshold = 12.0f;
	float sleepAngularVelocityThreshold = 0.05f;
	float sleepTimeThreshold = 0.6f;
	bool parallelNarrowPhaseEnabled = true;
	SizeT parallelNarrowPhaseMinPairs = 256u;
	bool parallelSolverEnabled = true;
	SizeT parallelSolverMinConstraints = 64u;
	int solverIterations = 10;

	std::vector<std::pair<SizeT, SizeT>> BuildPotentialPairs();
	std::vector<std::pair<SizeT, SizeT>> BuildNarrowPhasePairs(
		const std::vector<std::pair<SizeT, SizeT>>& potentialPairs);
	void DetectContacts(
		const std::vector<std::pair<SizeT, SizeT>>& narrowPhasePairs,
		std::vector<Contact>& contactsOut);
	void DetectContactsSequential(
		const std::vector<std::pair<SizeT, SizeT>>& narrowPhasePairs,
		std::vector<Contact>& contactsOut);
	void DetectContactsParallel(
		const std::vector<std::pair<SizeT, SizeT>>& narrowPhasePairs,
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
	void SetParallelNarrowPhaseMinPairs(SizeT minPairs);
	SizeT GetParallelNarrowPhaseMinPairs() const;
	void SetParallelSolverEnabled(bool enabled);
	bool IsParallelSolverEnabled() const;
	void SetParallelSolverMinConstraints(SizeT minConstraints);
	SizeT GetParallelSolverMinConstraints() const;
	void WakeBody(Body& body);
	void WakeAllBodies();

	void AddForce(const FVector2D& force);
	void AddTorque(float torque);

	void Update(float dt);

	void CheckCollisions();
};

} // namespace AE::Physics

using AE::Physics::World;

#endif
