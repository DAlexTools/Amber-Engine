#ifndef PHYSICSCONTACTRESPONSESYSTEM_H
#define PHYSICSCONTACTRESPONSESYSTEM_H

#include <cstdint>
#include <string>
#include <vector>

#include "../Components/PhysicsBodyComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../EntityComponentSystem/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Logging/Logger.h"
#include "Core/Math/Vector2D.h"
#include "PhysicsCollisionLayers.h"

enum class PhysicsContactAction
{
    Bounce
};

enum class PhysicsContactResponder
{
    First,
    Second,
    Both
};

struct PhysicsContactPolicy
{
    std::uint32_t firstLayerMask = EnginePhysicsCollision::All;
    std::uint32_t secondLayerMask = EnginePhysicsCollision::All;
    PhysicsContactAction action = PhysicsContactAction::Bounce;
    PhysicsContactResponder responder = PhysicsContactResponder::First;
    bool bidirectional = true;
    std::string name = "contact_policy";
};

class PhysicsContactResponseSystem : public System
{
public:
    void ClearPolicies()
    {
        policies.clear();
    }

    void AddPolicy(const PhysicsContactPolicy& policy)
    {
        policies.push_back(policy);
    }

    const std::vector<PhysicsContactPolicy>& GetPolicies() const
    {
        return policies;
    }

    void SubscribeToEvents(std::unique_ptr<EventBus>& eventBus)
    {
        eventBus->SubscribeToEvent<CollisionEvent>(this, &PhysicsContactResponseSystem::OnCollision);
    }

    void OnCollision(CollisionEvent& event)
    {
        if (!event.fromPhysics)
        {
            return;
        }

        for (const auto& policy : policies)
        {
            if (TryApplyPolicy(policy, event.a, event.b))
            {
                return;
            }
        }
    }

private:
    std::vector<PhysicsContactPolicy> policies;

    bool TryApplyPolicy(const PhysicsContactPolicy& policy, Entity a, Entity b)
    {
        if (MatchesPolicy(policy, a, b))
        {
            ApplyPolicy(policy, a, b);
            return true;
        }

        if (policy.bidirectional && MatchesPolicy(policy, b, a))
        {
            ApplyPolicy(policy, b, a);
            return true;
        }

        return false;
    }

    bool MatchesPolicy(const PhysicsContactPolicy& policy, Entity first, Entity second) const
    {
        return LayerMatches(first, policy.firstLayerMask) &&
            LayerMatches(second, policy.secondLayerMask);
    }

    bool LayerMatches(Entity entity, std::uint32_t layerMask) const
    {
        if (!entity.HasComponent<PhysicsBodyComponent>())
        {
            return false;
        }

        const auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();
        return physicsBody.body && (physicsBody.body->collisionCategory & layerMask) != 0;
    }

    void ApplyPolicy(const PhysicsContactPolicy& policy, Entity first, Entity second)
    {
        switch (policy.action)
        {
            case PhysicsContactAction::Bounce:
                ApplyBouncePolicy(policy, first, second);
                break;
        }
    }

    void ApplyBouncePolicy(const PhysicsContactPolicy& policy, Entity first, Entity second)
    {
        if (policy.responder == PhysicsContactResponder::First || policy.responder == PhysicsContactResponder::Both)
        {
            ApplyBounce(first, policy.name);
        }

        if (policy.responder == PhysicsContactResponder::Second || policy.responder == PhysicsContactResponder::Both)
        {
            ApplyBounce(second, policy.name);
        }
    }

    void ApplyBounce(Entity entity, const std::string& policyName)
    {
        if (!entity.HasComponent<RigidBodyComponent>())
        {
            return;
        }

        auto& rigidBody = entity.GetComponent<RigidBodyComponent>();
        const bool shouldFlipSprite = rigidBody.velocity.x != 0 || rigidBody.velocity.y != 0;

        if (rigidBody.velocity.x != 0)
        {
            rigidBody.velocity.x *= -1;
        }

        if (rigidBody.velocity.y != 0)
        {
            rigidBody.velocity.y *= -1;
        }

        if (shouldFlipSprite && entity.HasComponent<SpriteComponent>())
        {
            auto& sprite = entity.GetComponent<SpriteComponent>();
            sprite.flip = (sprite.flip == SDL_FLIP_NONE) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        }

        if (entity.HasComponent<PhysicsBodyComponent>())
        {
            auto& physicsBody = entity.GetComponent<PhysicsBodyComponent>();
            if (physicsBody.body)
            {
                physicsBody.body->velocity = AE::Math::FVector2D(
                    rigidBody.velocity.x,
                    rigidBody.velocity.y);
            }
        }

        AE::Logger::Log("Applied physics contact policy: " + policyName + " for entity id " + std::to_string(entity.GetID()));
    }
};

#endif

