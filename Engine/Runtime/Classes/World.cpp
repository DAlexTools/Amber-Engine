#include "Classes/World.h"
#include "Core/Threading/ThreadPool.h"
#include "Physics/Constants.h"
#include "Physics/CollisionDetection.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <unordered_map>
#include "Logging/Logger.h"

namespace AE::Physics
{

namespace
{
struct AABB
{
	float minX = 0.0f;
	float minY = 0.0f;
	float maxX = 0.0f;
	float maxY = 0.0f;
};

struct CellCoord
{
	int x = 0;
	int y = 0;

	bool operator==(const CellCoord& other) const
	{
		return x == other.x && y == other.y;
	}
};

struct CellCoordHash
{
	SizeT operator()(const CellCoord& cell) const
	{
		const uint64 x = static_cast<uint32>(cell.x);
		const uint64 y = static_cast<uint32>(cell.y);
		return static_cast<SizeT>((x << 32u) ^ y);
	}
};

using Clock = std::chrono::steady_clock;

double ElapsedMilliseconds(Clock::time_point start, Clock::time_point end)
{
	return std::chrono::duration<double, std::milli>(end - start).count();
}

AABB ComputeAABB(const Body& body)
{
	if (body.shape->GetType() == CIRCLE)
	{
		const CircleShape* circle = static_cast<const CircleShape*>(body.shape);
		return AABB{
			body.position.X - circle->radius,
			body.position.Y - circle->radius,
			body.position.X + circle->radius,
			body.position.Y + circle->radius};
	}

	const PolygonShape* polygon = static_cast<const PolygonShape*>(body.shape);
	AABB bounds{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest()};

	for (const FVector2D& vertex : polygon->worldVertices)
	{
		bounds.minX = std::min(bounds.minX, vertex.X);
		bounds.minY = std::min(bounds.minY, vertex.Y);
		bounds.maxX = std::max(bounds.maxX, vertex.X);
		bounds.maxY = std::max(bounds.maxY, vertex.Y);
	}

	return bounds;
}

bool Overlaps(const AABB& first, const AABB& second)
{
	return first.minX <= second.maxX &&
		   first.maxX >= second.minX &&
		   first.minY <= second.maxY &&
		   first.maxY >= second.minY;
}

int CellFor(float value, float cellSize)
{
	return static_cast<int>(std::floor(value / cellSize));
}

SizeT BruteForcePairCount(SizeT bodyCount)
{
	return bodyCount < 2 ? 0 : bodyCount * (bodyCount - 1u) / 2u;
}
} // namespace

World::World(float gravity)
{
	G = -gravity;
	AE::Logger::Log("World constructor called", "Physics");
}

World::~World()
{
	for (auto body : bodies)
	{
		delete body;
	}
	for (auto constraint : constraints)
	{
		delete constraint;
	}

	AE::Logger::Log("World destructor called", "Physics");
}

void World::AddBody(Body* body)
{
	if (body)
	{
		body->sleeping = false;
		body->sleepTime = 0.0f;
	}
	bodies.push_back(body);
}

void World::RemoveBody(Body* body)
{
	if (!body)
	{
		return;
	}

	auto bodyIterator = std::find(bodies.begin(), bodies.end(), body);
	if (bodyIterator == bodies.end())
	{
		return;
	}

	bodies.erase(bodyIterator);
	constraints.erase(
		std::remove_if(
			constraints.begin(),
			constraints.end(),
			[body](Constraint* constraint)
			{
				const bool referencesBody = constraint && (constraint->a == body || constraint->b == body);
				if (referencesBody)
				{
					delete constraint;
				}
				return referencesBody;
			}),
		constraints.end());
	lastContacts.erase(
		std::remove_if(
			lastContacts.begin(),
			lastContacts.end(),
			[body](const Contact& contact)
			{
				return contact.a == body || contact.b == body;
			}),
		lastContacts.end());

	delete body;
}

std::vector<Body*>& World::GetBodies()
{
	return bodies;
}

void World::AddConstraint(Constraint* constraint)
{
	constraints.push_back(constraint);
}

std::vector<Constraint*>& World::GetConstraints()
{
	return constraints;
}

std::vector<Contact>& World::GetContacts()
{
	return lastContacts;
}

const std::vector<Contact>& World::GetContacts() const
{
	return lastContacts;
}

const WorldStats& World::GetLastStats() const
{
	return lastStats;
}

void World::SetBroadPhaseEnabled(bool enabled)
{
	broadPhaseEnabled = enabled;
}

bool World::IsBroadPhaseEnabled() const
{
	return broadPhaseEnabled;
}

void World::SetBroadPhaseCellSize(float cellSize)
{
	broadPhaseCellSize = std::max(16.0f, cellSize);
}

float World::GetBroadPhaseCellSize() const
{
	return broadPhaseCellSize;
}

void World::SetSolverIterations(int iterations)
{
	solverIterations = std::max(1, iterations);
}

int World::GetSolverIterations() const
{
	return solverIterations;
}

void World::SetSleepingEnabled(bool enabled)
{
	sleepingEnabled = enabled;
	if (!sleepingEnabled)
	{
		WakeAllBodies();
	}
}

bool World::IsSleepingEnabled() const
{
	return sleepingEnabled;
}

void World::SetSleepThresholds(float linearVelocity, float angularVelocity, float time)
{
	sleepLinearVelocityThreshold = std::max(0.0f, linearVelocity);
	sleepAngularVelocityThreshold = std::max(0.0f, angularVelocity);
	sleepTimeThreshold = std::max(0.0f, time);
}

void World::SetParallelNarrowPhaseEnabled(bool enabled)
{
	parallelNarrowPhaseEnabled = enabled;
}

bool World::IsParallelNarrowPhaseEnabled() const
{
	return parallelNarrowPhaseEnabled;
}

void World::SetParallelNarrowPhaseMinPairs(SizeT minPairs)
{
	parallelNarrowPhaseMinPairs = std::max<SizeT>(1u, minPairs);
}

SizeT World::GetParallelNarrowPhaseMinPairs() const
{
	return parallelNarrowPhaseMinPairs;
}

void World::SetParallelSolverEnabled(bool enabled)
{
	parallelSolverEnabled = enabled;
}

bool World::IsParallelSolverEnabled() const
{
	return parallelSolverEnabled;
}

void World::SetParallelSolverMinConstraints(SizeT minConstraints)
{
	parallelSolverMinConstraints = std::max<SizeT>(1u, minConstraints);
}

SizeT World::GetParallelSolverMinConstraints() const
{
	return parallelSolverMinConstraints;
}

void World::WakeBody(Body& body)
{
	body.sleeping = false;
	body.sleepTime = 0.0f;
}

void World::WakeAllBodies()
{
	for (Body* body : bodies)
	{
		if (body)
		{
			WakeBody(*body);
		}
	}
}

void World::AddForce(const FVector2D& force)
{
	forces.push_back(force);
	WakeAllBodies();
}

void World::AddTorque(float torque)
{
	torques.push_back(torque);
	WakeAllBodies();
}

void World::Update(float dt)
{
	const auto totalStart = Clock::now();

	// Create a vector of penetration constraints that will be solved frame per frame
	std::vector<PenetrationConstraint> penetrations;
	lastContacts.clear();
	lastStats = WorldStats{};
	lastStats.bodyCount = bodies.size();
	lastStats.solverIterations = solverIterations;

	const auto forceStart = Clock::now();

	// Loop all bodies of the world applying forces
	for (auto& body : bodies)
	{
		if (body->sleeping &&
			(body->velocity.MagnitudeSquared() > sleepLinearVelocityThreshold * sleepLinearVelocityThreshold ||
			 std::abs(body->angularVelocity) > sleepAngularVelocityThreshold))
		{
			WakeBody(*body);
		}
		if (sleepingEnabled && body->sleeping)
		{
			continue;
		}

		// Apply the weight force to all bodies
		FVector2D weight = FVector2D(0.0, body->mass * G * AE::Physics::PIXELS_PER_METER);
		body->AddForce(weight);

		// Apply forces to all bodies
		for (auto force : forces)
		{
			body->AddForce(force);
		}

		// Apply torque to all bodies
		for (auto torque : torques)
		{
			body->AddTorque(torque);
		}
	}

	// Integrate all the forces
	for (auto& body : bodies)
	{
		if (sleepingEnabled && body->sleeping)
		{
			continue;
		}
		body->IntegrateForces(dt);
	}
	auto phaseEnd = Clock::now();
	lastStats.forcePhaseMs = ElapsedMilliseconds(forceStart, phaseEnd);

	const auto broadPhaseStart = phaseEnd;
	const auto potentialPairs = BuildPotentialPairs();
	phaseEnd = Clock::now();
	lastStats.broadPhaseMs = ElapsedMilliseconds(broadPhaseStart, phaseEnd);

	const auto narrowPhaseStart = phaseEnd;
	const auto narrowPhasePairs = BuildNarrowPhasePairs(potentialPairs);
	DetectContacts(narrowPhasePairs, lastContacts);

	penetrations.reserve(lastContacts.size());
	for (const Contact& contact : lastContacts)
	{
		if (!contact.a->isSensor && !contact.b->isSensor)
		{
			// Create a new penetration constraint
			PenetrationConstraint penetration(contact.a, contact.b, contact.start, contact.end, contact.normal);
			penetrations.push_back(penetration);
		}
	}

	lastStats.contactCount = lastContacts.size();
	lastStats.penetrationConstraintCount = penetrations.size();
	phaseEnd = Clock::now();
	lastStats.narrowPhaseMs = ElapsedMilliseconds(narrowPhaseStart, phaseEnd);

	const auto solverStart = phaseEnd;
	auto solverIslands = BuildSolverIslands(penetrations);
	UpdateSolverIslandStats(solverIslands);
	SolveConstraints(dt, penetrations, solverIslands);
	phaseEnd = Clock::now();
	lastStats.solverPhaseMs = ElapsedMilliseconds(solverStart, phaseEnd);

	const auto velocityStart = phaseEnd;
	// Integrate all the velocities
	for (auto& body : bodies)
	{
		if (sleepingEnabled && body->sleeping)
		{
			continue;
		}
		body->IntegrateVelocities(dt);
	}
	phaseEnd = Clock::now();
	lastStats.velocityPhaseMs = ElapsedMilliseconds(velocityStart, phaseEnd);

	const auto sleepingStart = phaseEnd;
	UpdateSleepingBodies(dt);
	phaseEnd = Clock::now();
	lastStats.sleepingPhaseMs = ElapsedMilliseconds(sleepingStart, phaseEnd);
	lastStats.totalStepMs = ElapsedMilliseconds(totalStart, phaseEnd);
}

void World::CheckCollisions()
{
	const auto totalStart = Clock::now();

	lastContacts.clear();
	lastStats = WorldStats{};
	lastStats.bodyCount = bodies.size();
	lastStats.solverIterations = solverIterations;

	const auto broadPhaseStart = Clock::now();
	const auto potentialPairs = BuildPotentialPairs();
	auto phaseEnd = Clock::now();
	lastStats.broadPhaseMs = ElapsedMilliseconds(broadPhaseStart, phaseEnd);

	const auto narrowPhaseStart = phaseEnd;
	const auto narrowPhasePairs = BuildNarrowPhasePairs(potentialPairs);
	DetectContacts(narrowPhasePairs, lastContacts);

	lastStats.contactCount = lastContacts.size();
	phaseEnd = Clock::now();
	lastStats.narrowPhaseMs = ElapsedMilliseconds(narrowPhaseStart, phaseEnd);
	lastStats.totalStepMs = ElapsedMilliseconds(totalStart, phaseEnd);
}

std::vector<std::pair<SizeT, SizeT>> World::BuildNarrowPhasePairs(
	const std::vector<std::pair<SizeT, SizeT>>& potentialPairs)
{
	std::vector<std::pair<SizeT, SizeT>> narrowPhasePairs;
	narrowPhasePairs.reserve(potentialPairs.size());

	for (const auto& pair : potentialPairs)
	{
		Body* a = bodies[pair.first];
		Body* b = bodies[pair.second];

		if (ShouldSkipCollisionPair(*a, *b))
		{
			continue;
		}

		if (!a->CanCollideWith(*b))
		{
			++lastStats.maskFilteredPairs;
			continue;
		}

		narrowPhasePairs.push_back(pair);
	}

	lastStats.narrowPhaseTests = narrowPhasePairs.size();
	return narrowPhasePairs;
}

void World::DetectContacts(
	const std::vector<std::pair<SizeT, SizeT>>& narrowPhasePairs,
	std::vector<Contact>& contactsOut)
{
	contactsOut.clear();
	if (narrowPhasePairs.empty())
	{
		return;
	}

	if (!parallelNarrowPhaseEnabled || narrowPhasePairs.size() < parallelNarrowPhaseMinPairs)
	{
		DetectContactsSequential(narrowPhasePairs, contactsOut);
		return;
	}

	DetectContactsParallel(narrowPhasePairs, contactsOut);
}

void World::DetectContactsSequential(
	const std::vector<std::pair<SizeT, SizeT>>& narrowPhasePairs,
	std::vector<Contact>& contactsOut)
{
	std::vector<Contact> contacts;
	contacts.reserve(2u);

	for (const auto& pair : narrowPhasePairs)
	{
		contacts.clear();
		if (CollisionDetection::IsColliding(bodies[pair.first], bodies[pair.second], contacts))
		{
			contactsOut.insert(contactsOut.end(), contacts.begin(), contacts.end());
		}
	}
}

void World::DetectContactsParallel(
	const std::vector<std::pair<SizeT, SizeT>>& narrowPhasePairs,
	std::vector<Contact>& contactsOut)
{
	AE::Threading::ThreadPool& threadPool = AE::Threading::ThreadPool::Get();
	const SizeT workerCount = threadPool.WorkerCount();
	const SizeT jobCount = std::min(workerCount, narrowPhasePairs.size());
	if (jobCount < 2u)
	{
		DetectContactsSequential(narrowPhasePairs, contactsOut);
		return;
	}

	lastStats.parallelNarrowPhaseUsed = true;
	lastStats.parallelNarrowPhaseJobs = jobCount;

	std::vector<std::vector<Contact>> jobContacts(jobCount);

	threadPool.ParallelFor(jobCount, 1u, [&](SizeT beginJob, SizeT endJob)
						   {
        std::vector<Contact> scratchContacts;
        scratchContacts.reserve(2u);

        for (SizeT jobIndex = beginJob; jobIndex < endJob; ++jobIndex)
        {
            const SizeT beginPair = jobIndex * narrowPhasePairs.size() / jobCount;
            const SizeT endPair = (jobIndex + 1u) * narrowPhasePairs.size() / jobCount;
            std::vector<Contact>& localContacts = jobContacts[jobIndex];
            localContacts.reserve((endPair - beginPair) * 2u);

            for (SizeT pairIndex = beginPair; pairIndex < endPair; ++pairIndex)
            {
                const auto& pair = narrowPhasePairs[pairIndex];
                scratchContacts.clear();
                if (CollisionDetection::IsColliding(bodies[pair.first], bodies[pair.second], scratchContacts))
                {
                    localContacts.insert(localContacts.end(), scratchContacts.begin(), scratchContacts.end());
                }
            }
        } });

	SizeT contactCount = 0u;
	for (const auto& localContacts : jobContacts)
	{
		contactCount += localContacts.size();
	}

	contactsOut.reserve(contactCount);
	for (const auto& localContacts : jobContacts)
	{
		contactsOut.insert(contactsOut.end(), localContacts.begin(), localContacts.end());
	}
}

std::vector<World::SolverIsland> World::BuildSolverIslands(std::vector<PenetrationConstraint>& penetrations)
{
	std::vector<SolverIsland> islands;
	if (constraints.empty() && penetrations.empty())
	{
		return islands;
	}

	constexpr SizeT InvalidIndex = std::numeric_limits<SizeT>::max();

	std::unordered_map<Body*, SizeT> dynamicBodyIndices;
	dynamicBodyIndices.reserve(bodies.size());

	for (Body* body : bodies)
	{
		if (body && !body->IsStatic())
		{
			dynamicBodyIndices.emplace(body, dynamicBodyIndices.size());
		}
	}

	std::vector<SizeT> parents(dynamicBodyIndices.size());
	std::vector<SizeT> ranks(dynamicBodyIndices.size(), 0u);
	for (SizeT index = 0; index < parents.size(); ++index)
	{
		parents[index] = index;
	}

	auto findRoot = [&parents](SizeT index)
	{
		while (parents[index] != index)
		{
			parents[index] = parents[parents[index]];
			index = parents[index];
		}
		return index;
	};

	const auto dynamicIndexFor = [&dynamicBodyIndices, InvalidIndex](Body* body)
	{
		const auto found = dynamicBodyIndices.find(body);
		return found == dynamicBodyIndices.end() ? InvalidIndex : found->second;
	};

	auto uniteBodies = [&](Body* first, Body* second)
	{
		const SizeT firstIndex = dynamicIndexFor(first);
		const SizeT secondIndex = dynamicIndexFor(second);
		if (firstIndex == InvalidIndex || secondIndex == InvalidIndex)
		{
			return;
		}

		SizeT firstRoot = findRoot(firstIndex);
		SizeT secondRoot = findRoot(secondIndex);
		if (firstRoot == secondRoot)
		{
			return;
		}

		if (ranks[firstRoot] < ranks[secondRoot])
		{
			std::swap(firstRoot, secondRoot);
		}

		parents[secondRoot] = firstRoot;
		if (ranks[firstRoot] == ranks[secondRoot])
		{
			++ranks[firstRoot];
		}
	};

	for (Constraint* constraint : constraints)
	{
		if (constraint)
		{
			uniteBodies(constraint->a, constraint->b);
		}
	}
	for (const PenetrationConstraint& penetration : penetrations)
	{
		uniteBodies(penetration.a, penetration.b);
	}

	std::unordered_map<SizeT, SizeT> islandIndexByRoot;
	islandIndexByRoot.reserve(dynamicBodyIndices.size());
	SizeT staticOnlyIslandIndex = InvalidIndex;

	auto islandForBodies = [&](Body* first, Body* second) -> SolverIsland&
	{
		const SizeT firstIndex = dynamicIndexFor(first);
		const SizeT secondIndex = dynamicIndexFor(second);
		const SizeT islandBodyIndex = firstIndex != InvalidIndex ? firstIndex : secondIndex;
		if (islandBodyIndex == InvalidIndex)
		{
			if (staticOnlyIslandIndex == InvalidIndex)
			{
				staticOnlyIslandIndex = islands.size();
				islands.emplace_back();
			}
			return islands[staticOnlyIslandIndex];
		}

		const SizeT root = findRoot(islandBodyIndex);
		const auto inserted = islandIndexByRoot.emplace(root, islands.size());
		if (inserted.second)
		{
			islands.emplace_back();
		}
		return islands[inserted.first->second];
	};

	for (Constraint* constraint : constraints)
	{
		if (constraint)
		{
			islandForBodies(constraint->a, constraint->b).constraints.push_back(constraint);
		}
	}
	for (PenetrationConstraint& penetration : penetrations)
	{
		islandForBodies(penetration.a, penetration.b).penetrations.push_back(&penetration);
	}

	for (const auto& bodyEntry : dynamicBodyIndices)
	{
		const SizeT root = findRoot(bodyEntry.second);
		const auto islandIndex = islandIndexByRoot.find(root);
		if (islandIndex != islandIndexByRoot.end())
		{
			++islands[islandIndex->second].bodyCount;
		}
	}

	return islands;
}

void World::UpdateSolverIslandStats(const std::vector<SolverIsland>& islands)
{
	lastStats.solverConstraintCount = constraints.size() + lastStats.penetrationConstraintCount;
	lastStats.solverIslandCount = islands.size();

	for (const SolverIsland& island : islands)
	{
		lastStats.largestSolverIslandBodyCount =
			std::max(lastStats.largestSolverIslandBodyCount, island.bodyCount);
		lastStats.largestSolverIslandConstraintCount =
			std::max(lastStats.largestSolverIslandConstraintCount, island.ConstraintCount());
	}
}

void World::SolveConstraints(
	float dt,
	std::vector<PenetrationConstraint>& penetrations,
	std::vector<SolverIsland>& islands)
{
	if (!parallelSolverEnabled ||
		islands.size() < 2u ||
		lastStats.solverConstraintCount < parallelSolverMinConstraints)
	{
		SolveConstraintsSequential(dt, penetrations);
		return;
	}

	AE::Threading::ThreadPool& threadPool = AE::Threading::ThreadPool::Get();
	const SizeT jobCount = std::min(threadPool.WorkerCount(), islands.size());
	if (jobCount < 2u)
	{
		SolveConstraintsSequential(dt, penetrations);
		return;
	}

	lastStats.parallelSolverUsed = true;
	lastStats.parallelSolverJobs = jobCount;
	SolveConstraintsParallel(dt, islands);
}

void World::SolveConstraintsSequential(float dt, std::vector<PenetrationConstraint>& penetrations)
{
	for (Constraint* constraint : constraints)
	{
		constraint->PreSolve(dt);
	}
	for (PenetrationConstraint& constraint : penetrations)
	{
		constraint.PreSolve(dt);
	}
	for (int i = AE::Physics::ZERO; i < solverIterations; i++)
	{
		for (Constraint* constraint : constraints)
		{
			constraint->Solve();
		}
		for (PenetrationConstraint& constraint : penetrations)
		{
			constraint.Solve();
		}
	}
	for (Constraint* constraint : constraints)
	{
		constraint->PostSolve();
	}
	for (PenetrationConstraint& constraint : penetrations)
	{
		constraint.PostSolve();
	}
}

void World::SolveConstraintsParallel(float dt, std::vector<SolverIsland>& islands)
{
	AE::Threading::ThreadPool::Get().ParallelFor(
		islands.size(),
		1u,
		[&](SizeT beginIsland, SizeT endIsland)
		{
			for (SizeT islandIndex = beginIsland; islandIndex < endIsland; ++islandIndex)
			{
				SolveIsland(dt, islands[islandIndex]);
			}
		});
}

void World::SolveIsland(float dt, SolverIsland& island)
{
	for (Constraint* constraint : island.constraints)
	{
		constraint->PreSolve(dt);
	}
	for (PenetrationConstraint* constraint : island.penetrations)
	{
		constraint->PreSolve(dt);
	}
	for (int i = AE::Physics::ZERO; i < solverIterations; i++)
	{
		for (Constraint* constraint : island.constraints)
		{
			constraint->Solve();
		}
		for (PenetrationConstraint* constraint : island.penetrations)
		{
			constraint->Solve();
		}
	}
	for (Constraint* constraint : island.constraints)
	{
		constraint->PostSolve();
	}
	for (PenetrationConstraint* constraint : island.penetrations)
	{
		constraint->PostSolve();
	}
}

bool World::ShouldSkipCollisionPair(Body& first, Body& second)
{
	if (first.IsStatic() && second.IsStatic())
	{
		++lastStats.staticPairFilteredPairs;
		return true;
	}

	if (!sleepingEnabled)
	{
		return false;
	}

	if (first.sleeping && second.sleeping)
	{
		++lastStats.sleepingPairFilteredPairs;
		return true;
	}

	const auto staticBodyIsMoving = [](const Body& body)
	{
		return body.IsStatic() &&
			   (body.velocity.MagnitudeSquared() > 0.01f || std::abs(body.angularVelocity) > 0.001f);
	};

	if (first.sleeping && second.IsStatic())
	{
		if (staticBodyIsMoving(second))
		{
			WakeBody(first);
			return false;
		}

		++lastStats.sleepingPairFilteredPairs;
		return true;
	}

	if (second.sleeping && first.IsStatic())
	{
		if (staticBodyIsMoving(first))
		{
			WakeBody(second);
			return false;
		}

		++lastStats.sleepingPairFilteredPairs;
		return true;
	}

	if (first.sleeping && !second.IsStatic())
	{
		WakeBody(first);
	}
	if (second.sleeping && !first.IsStatic())
	{
		WakeBody(second);
	}

	return false;
}

void World::UpdateSleepingBodies(float dt)
{
	lastStats.sleepingBodyCount = 0;

	if (!sleepingEnabled)
	{
		return;
	}

	const float linearThresholdSq = sleepLinearVelocityThreshold * sleepLinearVelocityThreshold;
	for (Body* body : bodies)
	{
		if (!body || body->IsStatic() || !body->canSleep || body->isSensor)
		{
			if (body)
			{
				body->sleeping = false;
				body->sleepTime = 0.0f;
			}
			continue;
		}

		const bool isSlow = body->velocity.MagnitudeSquared() <= linearThresholdSq &&
							std::abs(body->angularVelocity) <= sleepAngularVelocityThreshold;
		if (isSlow)
		{
			body->sleepTime += dt;
			if (body->sleepTime >= sleepTimeThreshold)
			{
				body->sleeping = true;
				body->velocity = FVector2D::Zero;
				body->acceleration = FVector2D::Zero;
				body->angularVelocity = 0.0f;
				body->angularAcceleration = 0.0f;
				body->ClearForces();
				body->ClearTorque();
			}
		}
		else
		{
			WakeBody(*body);
		}

		if (body->sleeping)
		{
			++lastStats.sleepingBodyCount;
		}
	}
}

std::vector<std::pair<SizeT, SizeT>> World::BuildPotentialPairs()
{
	lastStats.bodyCount = bodies.size();
	lastStats.bruteForcePairs = BruteForcePairCount(bodies.size());

	std::vector<std::pair<SizeT, SizeT>> pairs;
	if (bodies.size() < 2)
	{
		return pairs;
	}

	if (!broadPhaseEnabled || bodies.size() < 12)
	{
		pairs.reserve(lastStats.bruteForcePairs);
		for (SizeT i = 0; i < bodies.size(); ++i)
		{
			for (SizeT j = i + 1u; j < bodies.size(); ++j)
			{
				pairs.emplace_back(i, j);
			}
		}
		lastStats.broadPhasePairs = pairs.size();
		return pairs;
	}

	std::vector<AABB> bounds;
	bounds.reserve(bodies.size());
	for (const Body* body : bodies)
	{
		bounds.push_back(ComputeAABB(*body));
	}

	std::unordered_map<CellCoord, std::vector<SizeT>, CellCoordHash> grid;
	grid.reserve(bodies.size() * 2u);

	for (SizeT index = 0; index < bounds.size(); ++index)
	{
		const AABB& aabb = bounds[index];
		const int minCellX = CellFor(aabb.minX, broadPhaseCellSize);
		const int maxCellX = CellFor(aabb.maxX, broadPhaseCellSize);
		const int minCellY = CellFor(aabb.minY, broadPhaseCellSize);
		const int maxCellY = CellFor(aabb.maxY, broadPhaseCellSize);

		for (int y = minCellY; y <= maxCellY; ++y)
		{
			for (int x = minCellX; x <= maxCellX; ++x)
			{
				grid[CellCoord{x, y}].push_back(index);
			}
		}
	}

	const SizeT reserveCount = std::min(lastStats.bruteForcePairs, bodies.size() * 16u);
	pairs.reserve(reserveCount);

	for (const auto& entry : grid)
	{
		const std::vector<SizeT>& cellBodies = entry.second;
		for (SizeT i = 0; i < cellBodies.size(); ++i)
		{
			for (SizeT j = i + 1u; j < cellBodies.size(); ++j)
			{
				const SizeT first = std::min(cellBodies[i], cellBodies[j]);
				const SizeT second = std::max(cellBodies[i], cellBodies[j]);
				if (Overlaps(bounds[first], bounds[second]))
				{
					pairs.emplace_back(first, second);
				}
			}
		}
	}

	std::sort(pairs.begin(), pairs.end());
	pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
	lastStats.broadPhasePairs = pairs.size();

	return pairs;
}

} // namespace AE::Physics
