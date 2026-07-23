#ifndef ECS_H
#define ECS_H

#include <bitset>
#include <vector>
#include <set>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <deque>

#include "../Logging/Logger.h"

class Registry;



/**
 * Defines the maximum number of components that can be registered.
 */
constexpr unsigned int MAX_COMPONENTS = 64;

/**
 * Represents a signature, which is a bitset indicating the presence or absence of components.
 * Each bit corresponds to a specific component type.
 */
using Signature = std::bitset<MAX_COMPONENTS>;

/**
 * Base interface for all components. Contains a static integer that is used to generate unique IDs
 * for each component type.
 */
struct IComponent
{
protected:
    /** 
     * The next available ID for a component. Each new component type will increment this value.
     */
    static int NextID;
};

/**
 * Template class for creating specific component types.
 *
 * @tparam T The component type being created.
 */
template<class T>
class Component : public IComponent
{
public:
   /**
    * Retrieves a unique ID for the component type.
    * 
    * @return A unique ID for the component type.
    */
    static int GetID() 
    {
        static auto ID = NextID++;
        return ID;
    }
};


/**
 * Represents an entity with a unique identifier.
 */
class Entity
{
private:
    /** 
     * Unique identifier for the entity.
     */
    int ID;

    /**
     * Unique name for the entity 
     */
    std::string Name;
public:
    /**
     * Constructs an Entity with a specified ID.
     *
     * @param id The unique ID to assign to this entity.
     */
    Entity(int id) : ID(id) {}

    Entity(int id ,std::string name) : ID(id), Name(name) {}

    /**
     * Default copy constructor.
     *
     * @param other The entity to copy from.
     */
    Entity(const Entity& other) = default;

    /**
     * Retrieves the unique ID of this entity.
     *
     * @return The ID of this entity.
     */
    int GetID() const;

    /**
     * 
     */
    std::string GetName() const { return Name; }

    /**
     * 
     */
    void SetName(const std::string name) { Name = name; }

    
    /**
     * Kill entity function 
     */
    void Kill();


    void Tag(const std::string& tag);
    bool HasTag(const std::string& tag) const;
    void Group(const std::string& group);
    bool BelongsToGroup(const std::string& group) const;

    /**
     * Default assignment operator.
     *
     * @param other The entity to assign from.
     * @return A reference to this entity.
     */
    Entity& operator=(const Entity& other) = default;

    /**
     * Equality operator to compare two entities by their IDs.
     *
     * @param other The entity to compare with.
     * @return True if the entities have the same ID, false otherwise.
     */
    bool operator==(const Entity& other) const { return ID == other.ID; }

    /**
     * Inequality operator to compare two entities by their IDs.
     *
     * @param other The entity to compare with.
     * @return True if the entities have different IDs, false otherwise.
     */
    bool operator!=(const Entity& other) const { return ID != other.ID; }

    /**
     * Less-than operator to compare two entities by their IDs.
     *
     * @param other The entity to compare with.
     * @return True if this entity's ID is less than the other's ID, false otherwise.
     */
    bool operator<(const Entity& other) const { return ID < other.ID; }

    /**
     * Greater-than operator to compare two entities by their IDs.
     *
     * @param other The entity to compare with.
     * @return True if this entity's ID is greater than the other's ID, false otherwise.
     */
    bool operator>(const Entity& other) const { return ID > other.ID; }


    template<typename TComponent, typename ...TArgs>
    void AddComponent(TArgs&& ...args);

    template<typename TComponent>
    void RemoveComponent();

    template<typename TComponent>
    bool HasComponent() const;

    template<typename TComponent>
    TComponent& GetComponent() const;

    // Hold a pointer to the entity's owner registry
    Registry* registry;
};


class System
{
private:
    Signature componentSignature;
    std::vector<Entity> entities;

public:
    System() = default;
    virtual ~System() = default;

    void AddEntityToSystem(Entity entity);
    void RemoveEntityFromSystem(Entity entity);
    std::vector<Entity> GetSystemEntity() const;
    const Signature& GetComponentSignature() const;

    template<typename TComponent> 
    void RequireComponent();
};

 

/**
 * Interface representing a generic pool. Serves as a base class for typed pools.
 */
class IPool
{
public:
    virtual ~IPool() = default;
    virtual void RemoveEntityFromPool(int entityId) = 0;

};

/**
 * A pool class for managing a collection of objects of a specific type.
 *
 * @tparam T The type of objects that the pool will manage.
 */
template<class T>
class Pool : public IPool
{
private:
    /** 
     * A vector to store the objects in the pool.
     */
    std::vector<T> data;
    int size;

    /** Helper maps to keep track of entity ids per index, so the vector is always packed */
    std::unordered_map<int, int> entityIdToIndex;
    std::unordered_map<int, int> indexToEntityId;   

public:
    /**
     * Constructs a Pool with a specified initial size.
     *
     * @param size The initial size of the pool. Defaults to 100.
     */
    Pool(int capacity = 100)
    {
        size = 0;

        data.resize(capacity);
    }

    /**
     * Virtual destructor for the Pool class.
     */
    virtual ~Pool() = default;

    /**
     * Checks if the pool is empty.
     *
     * @return True if the pool is empty, false otherwise.
     */
    bool IsEmpty() const
    {
        return size == 0;
    }

    /**
     * Gets the current size of the pool.
     *
     * @return The number of objects in the pool.
     */
    int GetSize() const
    {
        return size;
    }

    /**
     * Resizes the pool to hold a specified number of objects.
     *
     * @param size The new size of the pool.
     */
    void Resize(int n)
    {
        data.resize(n); 
    }

    /**
     * Clears all objects from the pool.
     */
    void Clear()
    {
        data.clear();
        size = 0;
    }

    /**
     * Adds an object to the pool.
     *
     * @param object The object to add to the pool.
     */
    void Add(T object)
    {
        data.push_back(object);
    }

    /**
     * Sets the object at a specified index in the pool.
     *
     * @param index The index at which to set the object.
     * @param object The object to set at the specified index.
     */
    void Set(int entityId, T object)
    {
        if (entityIdToIndex.find(entityId) != entityIdToIndex.end())
        {
            int index = entityIdToIndex[entityId];
            data[index] = object;
        }
        else
        {
            int index = size;
            entityIdToIndex.emplace(entityId, index);
            indexToEntityId.emplace(index, entityId);
            if (index >= static_cast<int>(data.capacity()))
            {
                data.resize(size * 2);
            }
            data[index] = object;
            size++;
        }
    }

    void Remove(int entityId)
    {
        int indexOfRemove = entityIdToIndex[entityId];
        int indexOfLast = size - 1;
        data[indexOfRemove] = data[indexOfLast];

        int entityIdOfLastElement = indexToEntityId[indexOfLast];
        entityIdToIndex[entityIdOfLastElement] = indexOfRemove;
        indexToEntityId[indexOfRemove] = entityIdOfLastElement;

        entityIdToIndex.erase(entityId);
        indexToEntityId.erase(indexOfLast);

        size--;
    }

    /**
     * Gets the object at a specified index in the pool.
     *
     * @param index The index of the object to retrieve.
     * @return A reference to the object at the specified index.
     */
    T& Get(int entityId) 
    {
        int index = entityIdToIndex[entityId];
        return static_cast<T&>(data[index]);
    }

    virtual void RemoveEntityFromPool(int entityId) override
    {
        if (entityIdToIndex.find(entityId) != entityIdToIndex.end())
        {
            Remove(entityId);
        }
    }


    /**
     * Provides array-style access to the pool.
     *
     * @param index The index of the object to access.
     * @return A reference to the object at the specified index.
     */
    T& operator[](unsigned int index)
    {
        return data[index];
    }
};

/**
 * The Registry class manages entities, components, and systems within an ECS (Entity-Component-System) architecture.
 */
class Registry
{
private:
    /** 
     * Counter to keep track of the total number of entities.
     */
    int numEntities = 0;

    /** 
     * Stores component pools, where each pool contains components of a specific type for all entities.
     */
    std::vector<std::shared_ptr<IPool>> componentPools;

    /** 
     * Stores a signature for each entity, representing the components associated with the entity.
     */
    std::vector<Signature> entityComponentSignatures;

    /** 
     * Maps component types to their corresponding systems.
     */
    std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

    /** 
     * Stores entities that need to be added to the registry.
     */
    std::set<Entity> entitiesToBeAdded;

    /** 
     * Stores entities that are scheduled to be removed (killed) from the registry.
     */
    std::set<Entity> entitiesToBeKilled;


    std::unordered_map<std::string, Entity> entityPerTag;
    std::unordered_map<int, std::string> tagPerEntity;

    std::unordered_map<std::string, std::set<Entity>> entitiesPerGroup;
    std::unordered_map<int, std::string> groupPerEntity;

    std::deque<int> freeIDs;

public:
    /**
     * Default constructor for the Registry.
     */
    Registry() { AE::Logger::Log("Registry constructor called"); }

    /**
     * Destructor for Registry
     */
    ~Registry() { AE::Logger::Log("Registry destructor called "); }

    /**
     * Updates the registry, processing any pending entity additions or removals.
     */
    void Update();

    /**
     * Creates a new entity and registers it within the registry.
     *
     * @return The newly created entity.
     */
    Entity CreateEntity();

    /**
     * Kill Entity
     * 
     * @param entity - entity to be kill 
     */
    void KillEntity(Entity entity);
    const std::set<Entity>& GetEntitiesToBeKilled() const;


    void TagEntity(Entity entity, const std::string& tag);
    bool EntityHasTag(Entity entity, const std::string& tag) const;
    Entity GetEntityByTag(const std::string& tag) const;
    void RemoveEntityTag(Entity entity);

    void GroupEntity(Entity entity, const std::string& group);
    bool EntityBelongsToGroup(Entity entity, const std::string& group) const;
    std::vector<Entity> GetEntitiesByGroup(const std::string& group) const;
    void RemoveEntityGroup(Entity entity);

    /** COMPONENT MANAGER */
    /**
     * Adds a component of type `TComponent` to the specified entity.
     *
     * @tparam TComponent The type of component to add.
     * @tparam TArgs The types of arguments to pass to the component's constructor.
     * @param entity The entity to add the component to.
     * @param args Arguments to construct the component.
     */
    template<typename TComponent,typename ...TArgs>
    void AddComponent(Entity entity, TArgs&& ...args);

    /**
     * Removes a component of type `TComponent` from the specified entity.
     *
     * @tparam TComponent The type of component to remove.
     * @param entity The entity to remove the component from.
     */
    template<typename TComponent>
    void RemoveComponent(Entity entity);

    /**
     * Checks if the specified entity has a component of type `TComponent`.
     *
     * @tparam TComponent The type of component to check.
     * @param entity The entity to check for the component.
     * @return True if the entity has the component, false otherwise.
     */
    template<typename TComponent>
    bool HasComponent(Entity entity) const;

    /**
     * Retrieves a specific component of type TComponent attached to the specified entity.
     *
     * @tparam TComponent The type of the component to retrieve.
     * @param entity The entity from which the component is retrieved.
     * @return Reference to the component of type TComponent associated with the entity.
     */
    template<typename TComponent>
    TComponent& GetComponent(Entity entity) const;

    /** SYSTEM MANAGER */


    /**
     * Adds a new system of type TSystem to the registry with optional initialization arguments.
     *
     * @tparam TSystem The type of the system to add.
     * @tparam TArgs Parameter pack for forwarding arguments to the system's constructor.
     * @param args Arguments to initialize the system.
     */    
    template<typename TSystem, typename ...TArgs>
    void AddSystem(TArgs&& ...args);

    /**
     * Removes a system of type TSystem from the registry.
     *
     * @tparam TSystem The type of the system to remove.
     */
    template<typename TSystem>
    void RemoveSystem();

    /**
     * Checks if a system of type TSystem exists in the registry.
     *
     * @tparam TSystem The type of the system to check.
     * @return True if the system exists; otherwise, false.
     */
    template<typename TSystem>
    bool HasSystem() const;

    /**
     * Retrieves the system of type TSystem from the registry.
     *
     * @tparam TSystem The type of the system to retrieve.
     * @return Reference to the system of type TSystem.
     */
    template<typename TSystem>
    TSystem& GetSystem() const;

    /**
     * Adds an entity to all relevant systems based on its components.
     *
     * @param entity The entity to add to systems.
     */
    void AddEntityToSystems(Entity entity);


    /**
     * Remove an entity to all relevant systems 
     */
    void RemoveEntityFromSystems(Entity entity);




};

/**
 * Sets a requirement for a component type in a system by updating the system's component signature.
 *
 * @tparam TComponent The type of component required by the system.
 */
template <typename TComponent>
inline void System::RequireComponent()
{
    const auto componentID = Component<TComponent>::GetID();    
    componentSignature.set(componentID);
}


/**
 * Adds a component of type `TComponent` to the specified entity, initializing it with given arguments.
 * Resizes component pools and the pool for this component type if necessary.
 *
 * @tparam TComponent The type of component to add.
 * @tparam TArgs The types of arguments to pass to the component's constructor.
 * @param entity The entity to which the component will be added.
 * @param args Arguments used to construct the component.
 */
template <typename TComponent, typename... TArgs>
inline void Registry::AddComponent(Entity entity, TArgs &&...args)
{
    const auto componentID = Component<TComponent>::GetID();
    const auto entityID = entity.GetID();

    if (componentID >= static_cast<int>(componentPools.size()))
    {
        componentPools.resize(componentID + 1, nullptr);
    }

    if (!componentPools[componentID])
    {
        std::shared_ptr<Pool<TComponent>> newComponentPool = std::make_shared<Pool<TComponent>>();
        componentPools[componentID] = newComponentPool;
    }

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentID]);

    TComponent newComponent(std::forward<TArgs>(args)...);

    componentPool->Set(entityID, newComponent);
    entityComponentSignatures[entityID].set(componentID);

    AE::Logger::Log("Component id = " + std::to_string(componentID) + " was added to entity id " + std::to_string(entityID));
}

/**
 * Removes a component of type `TComponent` from the specified entity.
 *
 * @tparam TComponent The type of component to remove.
 * @param entity The entity from which the component will be removed.
 */
template <typename TComponent>
inline void Registry::RemoveComponent(Entity entity)
{
    const auto componentID = Component<TComponent>::GetID();
    const auto entityID = entity.GetID();

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentID]);
    componentPool->Remove(entityID);

    entityComponentSignatures[entityID].set(componentID, false);
    AE::Logger::Log("Component id = " + std::to_string(componentID) + " was remove from entity id " + std::to_string(entityID));
}

/**
 * Checks if the specified entity has a component of type `TComponent`.
 *
 * @tparam TComponent The type of component to check.
 * @param entity The entity to check for the component.
 * @return True if the entity has the component, false otherwise.
 */
template <typename TComponent>
inline bool Registry::HasComponent(Entity entity) const
{
    const auto componentID = Component<TComponent>::GetID();
    const auto entityID = entity.GetID();

    return entityComponentSignatures[entityID].test(componentID);
}

template <typename TComponent>
inline TComponent &Registry::GetComponent(Entity entity) const
{
    const auto componentID = Component<TComponent>::GetID();
    const auto entityID = entity.GetID();
    auto componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentID]);

    return componentPool->Get(entityID);
}

/**
 * Template method to add a system to the registry with optional initialization arguments.
 *
 * @tparam TSystem The type of the system to add.
 * @tparam TArgs Parameter pack for forwarding arguments to the system's constructor.
 * @param args Arguments to initialize the system.
 */
template <typename TSystem,typename ...TArgs>
inline void Registry::AddSystem(TArgs &&...args)
{
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

/**
 * Removes a system of type TSystem from the registry.
 *
 * @tparam TSystem The type of the system to remove.
 */
template <typename TSystem>
inline void Registry::RemoveSystem()
{
    auto system = systems.find(std::type_index(typeid(TSystem)));
    systems.erase(system);
}

/**
 * Checks if a system of type TSystem exists in the registry.
 *
 * @tparam TSystem The type of the system to check.
 * @return True if the system exists; otherwise, false.
 */
template <typename TSystem>
inline bool Registry::HasSystem() const
{
    return systems.find(std::type_index(typeid(TSystem))) != systems.end();
}

/**
 * Retrieves the system of type TSystem from the registry.
 *
 * @tparam TSystem The type of the system to retrieve.
 * @return Reference to the system of type TSystem.
 * @throws std::bad_cast if the system type is invalid.
 */
template <typename TSystem>
inline TSystem &Registry::GetSystem() const
{
    auto system = systems.find(std::type_index(typeid(TSystem)));
    return *(std::static_pointer_cast<TSystem>(system->second));
}

/**
 * Adds a component of type TComponent to the specified entity.
 *
 * @tparam TComponent The type of the component to add.
 * @tparam TArgs Parameter pack for forwarding arguments to the component's constructor.
 * @param args Arguments to initialize the component.
 */
template <typename TComponent, typename ...TArgs>
void Entity::AddComponent(TArgs&& ...args) 
{
    registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

/**
 * Removes a component of type TComponent from the specified entity.
 *
 * @tparam TComponent The type of the component to remove.
 */
template <typename TComponent>
void Entity::RemoveComponent() 
{
    registry->RemoveComponent<TComponent>(*this);
}

/**
 * Checks if an entity has a component of type TComponent.
 *
 * @tparam TComponent The type of the component to check.
 * @return True if the component exists; otherwise, false.
 */
template <typename TComponent>
bool Entity::HasComponent() const 
{
    return registry->HasComponent<TComponent>(*this);
}

/**
 * Retrieves a component of type TComponent from the specified entity.
 *
 * @tparam TComponent The type of the component to retrieve.
 * @return Reference to the component of type TComponent associated with the entity.
 */
template <typename TComponent>
TComponent& Entity::GetComponent() const 
{
    return registry->GetComponent<TComponent>(*this);
}


#endif
