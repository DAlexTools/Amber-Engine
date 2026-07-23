#ifndef RENDERCOLLIDERSYSTEM_H
#define RENDERCOLLIDERSYSTEM_H

#include "../EntityComponentSystem/ECS.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/TransformComponent.h"


// TODO in future add Circle Collision Component 
enum EColliderType
{
    ECT_Box,
    ECT_Circle
};

/**
 * RenderColliderSystem is responsible for rendering the bounding boxes (colliders)
 * of entities with the required components: TransformComponent and BoxCollisionComponent.
 * It is typically used for debugging purposes to visualize collision areas.
 */
class RenderColliderSystem : public System
{
public:
    /**
     * Constructs a RenderColliderSystem and specifies required components.
     * Entities must have both TransformComponent and BoxCollisionComponent
     * to be processed by this system.
     */
    RenderColliderSystem()
    {
        RequireComponent<TransformComponent>();
        RequireComponent<BoxCollisionComponent>();
    }

    /**
     * Renders the bounding boxes (colliders) of all entities managed by this system.
     * For each entity, the collider rectangle is calculated based on its position,
     * offset, and dimensions, and then rendered as a red rectangle on the screen.
     *
     * @param renderer Pointer to SDL_Renderer used for drawing the bounding boxes.
     */
    void Update(SDL_Renderer* renderer, SDL_Rect& camera)
    {
        for (auto entity : GetSystemEntity())
        {
            const auto transform = entity.GetComponent<TransformComponent>();
            const auto collider = entity.GetComponent<BoxCollisionComponent>();

            // Calculate collider rectangle
            const SDL_Rect colliderRect = 
            {
                static_cast<int>(transform.position.x + collider.offset.x - camera.x),
                static_cast<int>(transform.position.y + collider.offset.y - camera.y),
                static_cast<int>(collider.width),
                static_cast<int>(collider.height)
            };

            // Set draw color to red and render the rectangle
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &colliderRect);
        }
    }


//TODO in future add Circle Collision Component
private:
    void DrawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius)
    {
        for (int w = 0; w < radius * 2; w++)
        {
            for (int h = 0; h < radius * 2; h++)
            {
                int dx = radius - w;                // horizontal offset
                int dy = radius - h;                // vertical offset
                if ((dx * dx + dy * dy) <= (radius * radius))
                {
                    SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
                }
            }
        }
    }
};

#endif