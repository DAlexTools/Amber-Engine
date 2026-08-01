#include "ECS.h"
#include <algorithm>
#include "../Logging/Logger.h"

/**
 * @brief Static member to track the next unique ID for components.
 *
 * Used to assign unique IDs to each component type dynamically.
 */
int IComponent::NextID = 0;

/**
 * @brief Retrieves the unique ID of the entity.
 *
 * @return int The unique identifier of the entity.
 */
int Entity::GetID() const
{
	return ID;
}

/**
 * @brief Marks the entity as "killed" and removes it from the registry.
 *
 * This method logs the action and informs the registry to handle the removal process.
 */
void Entity::Kill()
{
	registry->KillEntity(*this);
	AE::Logger::Log("Kill entity - " + GetName() + " id: " + std::to_string(GetID()));
}

void Entity::Tag(const std::string& tag)
{
	registry->TagEntity(*this, tag);
}

bool Entity::HasTag(const std::string& tag) const
{
	return registry->EntityHasTag(*this, tag);
}

void Entity::Group(const std::string& group)
{
	registry->GroupEntity(*this, group);
}

bool Entity::BelongsToGroup(const std::string& group) const
{
	return registry->EntityBelongsToGroup(*this, group);
}

/**
 * @brief Adds an entity to the system's list of tracked entities.
 *
 * @param entity The entity to be added to the system.
 */
void System::AddEntityToSystem(Entity entity)
{
	entities.push_back(entity);
}

/**
 * @brief Removes an entity from the system's list of tracked entities.
 *
 * Searches for the specified entity and removes it if found.
 *
 * @param entity The entity to be removed.
 */
void System::RemoveEntityFromSystem(Entity entity)
{
	entities.erase(std::remove_if(entities.begin(), entities.end(), [&entity](Entity other)
								  { return entity == other; }),
				   entities.end());
}

/**
 * @brief Retrieves all entities currently tracked by the system.
 *
 * @return std::vector<Entity> A list of entities managed by the system.
 */
std::vector<Entity> System::GetSystemEntity() const
{
	return entities;
}

/**
 * @brief Retrieves the system's component signature.
 *
 * The signature defines which components an entity must have to be managed by the system.
 *
 * @return const Signature& The system's component signature.
 */
const Signature& System::GetComponentSignature() const
{
	return componentSignature;
}

/**
 * @brief Updates the registry, handling additions and removals of entities from systems.
 *
 * Processes entities marked for addition or removal, updating the active systems and
 * recycling entity IDs as necessary.
 */
void Registry::Update()
{
	// Add the entities that are waiting to be created to the active Systems
	for (auto entity : entitiesToBeAdded)
	{
		AddEntityToSystems(entity);
	}

	entitiesToBeAdded.clear();

	// Process entities that are waiting to be killed from the active Systems
	for (auto entity : entitiesToBeKilled)
	{
		RemoveEntityFromSystems(entity);
		entityComponentSignatures[entity.GetID()].reset();

		for (auto pool : componentPools)
		{
			if (pool)
			{
				pool->RemoveEntityFromPool(entity.GetID());
			}
		}

		// make the entity id available to be reused
		freeIDs.push_back(entity.GetID());

		RemoveEntityTag(entity);
		RemoveEntityGroup(entity);
	}

	entitiesToBeKilled.clear();
}

/**
 * @brief Creates a new entity in the registry.
 *
 * Allocates a unique ID for the new entity, either by reusing a recycled ID or by incrementing the counter.
 * The entity is marked for addition and associated with the registry.
 *
 * @return Entity The newly created entity.
 */
Entity Registry::CreateEntity()
{
	int entityId;

	if (freeIDs.empty())
	{
		entityId = numEntities++;

		if (entityId >= static_cast<int>(entityComponentSignatures.size()))
		{
			entityComponentSignatures.resize(entityId + 1);
		}
	}
	else
	{
		entityId = freeIDs.front();
		freeIDs.pop_front();
	}

	Entity entity(entityId);
	entity.registry = this;
	entitiesToBeAdded.insert(entity);

	AE::Logger::Log("Entity created with id = " + std::to_string(entityId));

	return entity;
}

/**
 * @brief Marks an entity for removal from the registry.
 *
 * The entity will be processed for removal during the next update.
 *
 * @param entity The entity to be killed.
 */
void Registry::KillEntity(Entity entity)
{
	entitiesToBeKilled.insert(entity);
}

const std::set<Entity>& Registry::GetEntitiesToBeKilled() const
{
	return entitiesToBeKilled;
}

void Registry::TagEntity(Entity entity, const std::string& tag)
{
	entityPerTag.emplace(tag, entity);
	tagPerEntity.emplace(entity.GetID(), tag);
}

bool Registry::EntityHasTag(Entity entity, const std::string& tag) const
{
	if (tagPerEntity.find(entity.GetID()) == tagPerEntity.end())
	{
		return false;
	}
	return entityPerTag.find(tag)->second == entity;
}

Entity Registry::GetEntityByTag(const std::string& tag) const
{
	return entityPerTag.at(tag);
}

void Registry::RemoveEntityTag(Entity entity)
{
	auto taggedEntity = tagPerEntity.find(entity.GetID());
	if (taggedEntity != tagPerEntity.end())
	{
		auto tag = taggedEntity->second;
		entityPerTag.erase(tag);
		tagPerEntity.erase(taggedEntity);
	}
}

void Registry::GroupEntity(Entity entity, const std::string& group)
{
	entitiesPerGroup.emplace(group, std::set<Entity>());
	entitiesPerGroup[group].emplace(entity);
	groupPerEntity.emplace(entity.GetID(), group);
}

bool Registry::EntityBelongsToGroup(Entity entity, const std::string& group) const
{
	if (entitiesPerGroup.find(group) == entitiesPerGroup.end())
		return false;

	auto groupEntities = entitiesPerGroup.at(group);
	return groupEntities.find(entity.GetID()) != groupEntities.end();
}

std::vector<Entity> Registry::GetEntitiesByGroup(const std::string& group) const
{
	auto& setOfEntities = entitiesPerGroup.at(group);
	return std::vector<Entity>(setOfEntities.begin(), setOfEntities.end());
}

void Registry::RemoveEntityGroup(Entity entity)
{
	auto groupedEntity = groupPerEntity.find(entity.GetID());
	if (groupedEntity != groupPerEntity.end())
	{
		auto group = entitiesPerGroup.find(groupedEntity->second);
		if (group != entitiesPerGroup.end())
		{
			auto entityInGroup = group->second.find(entity);
			if (entityInGroup != group->second.end())
			{
				group->second.erase(entityInGroup);
			}
		}
		groupPerEntity.erase(groupedEntity);
	}
}

/**
 * @brief Adds an entity to all systems that are interested in its components.
 *
 * Iterates over the systems and checks if the entity's component signature matches the system's signature.
 * If a match is found, the entity is added to the system.
 *
 * @param entity The entity to be added to the relevant systems.
 */
void Registry::AddEntityToSystems(Entity entity)
{
	const auto entityID = entity.GetID();
	const auto& entityComponentSignature = entityComponentSignatures[entityID];

	for (auto& system : systems)
	{
		const auto& systemComponentSignature = system.second->GetComponentSignature();
		bool IsInterested = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;

		if (IsInterested)
		{
			system.second->AddEntityToSystem(entity);
		}
	}
}

/**
 * @brief Removes an entity from all systems.
 *
 * Iterates through the systems and removes the specified entity if it is being managed by the system.
 *
 * @param entity The entity to be removed from all systems.
 */
void Registry::RemoveEntityFromSystems(Entity entity)
{
	for (auto system : systems)
	{
		system.second->RemoveEntityFromSystem(entity);
	}
}
