#ifndef PHYSICSDEBUGRENDERSYSTEM_H
#define PHYSICSDEBUGRENDERSYSTEM_H

#include <cmath>
#include <cstddef>

#include <SDL2/SDL.h>

#include "../EntityComponentSystem/ECS.h"
#include "PhysicsWorldSystem.h"

class PhysicsDebugRenderSystem : public System
{
public:
    void Update(SDL_Renderer* renderer, SDL_Rect& camera, PhysicsWorldSystem& physicsWorldSystem)
    {
        for (auto body : physicsWorldSystem.GetWorld().GetBodies())
        {
            if (!body || !body->shape)
            {
                continue;
            }

            SetBodyDrawColor(renderer, *body);
            switch (body->shape->GetType())
            {
                case AE::Physics::CIRCLE:
                    DrawCircle(renderer, camera, *body, *static_cast<AE::Physics::CircleShape*>(body->shape));
                    break;
                case AE::Physics::BOX:
                case AE::Physics::POLYGON:
                    DrawPolygon(renderer, camera, *static_cast<AE::Physics::PolygonShape*>(body->shape));
                    break;
            }
        }

        DrawContacts(renderer, camera, physicsWorldSystem);
    }

private:
    void SetBodyDrawColor(SDL_Renderer* renderer, const AE::Physics::Body& body)
    {
        if (body.isSensor)
        {
            SDL_SetRenderDrawColor(renderer, 255, 80, 180, 255);
            return;
        }

        if (body.IsStatic())
        {
            SDL_SetRenderDrawColor(renderer, 80, 190, 255, 255);
            return;
        }

        SDL_SetRenderDrawColor(renderer, 80, 255, 140, 255);
    }

    void DrawContacts(SDL_Renderer* renderer, const SDL_Rect& camera, PhysicsWorldSystem& physicsWorldSystem)
    {
        for (const auto& contact : physicsWorldSystem.GetWorld().GetContacts())
        {
            const int startX = static_cast<int>(contact.start.X - camera.x);
            const int startY = static_cast<int>(contact.start.Y - camera.y);
            const int endX = static_cast<int>(contact.end.X - camera.x);
            const int endY = static_cast<int>(contact.end.Y - camera.y);

            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            SDL_RenderDrawLine(renderer, startX - 3, startY, startX + 3, startY);
            SDL_RenderDrawLine(renderer, startX, startY - 3, startX, startY + 3);
            SDL_RenderDrawLine(renderer, endX - 3, endY, endX + 3, endY);
            SDL_RenderDrawLine(renderer, endX, endY - 3, endX, endY + 3);

            const int normalEndX = startX + static_cast<int>(contact.normal.X * 18.0f);
            const int normalEndY = startY + static_cast<int>(contact.normal.Y * 18.0f);
            SDL_SetRenderDrawColor(renderer, 0, 255, 120, 255);
            SDL_RenderDrawLine(renderer, startX, startY, normalEndX, normalEndY);
        }

        SDL_SetRenderDrawColor(renderer, 0, 170, 255, 255);
    }

    void DrawPolygon(SDL_Renderer* renderer, const SDL_Rect& camera, const AE::Physics::PolygonShape& shape)
    {
        const auto& vertices = shape.worldVertices;
        if (vertices.size() < 2)
        {
            return;
        }

        for (std::size_t index = 0; index < vertices.size(); index++)
        {
            const auto& start = vertices[index];
            const auto& end = vertices[(index + 1) % vertices.size()];
            SDL_RenderDrawLine(
                renderer,
                static_cast<int>(start.X - camera.x),
                static_cast<int>(start.Y - camera.y),
                static_cast<int>(end.X - camera.x),
                static_cast<int>(end.Y - camera.y));
        }
    }

    void DrawCircle(
        SDL_Renderer* renderer,
        const SDL_Rect& camera,
        const AE::Physics::Body& body,
        const AE::Physics::CircleShape& shape)
    {
        const int centerX = static_cast<int>(body.position.X - camera.x);
        const int centerY = static_cast<int>(body.position.Y - camera.y);
        const int radius = static_cast<int>(shape.radius);

        int previousX = centerX + radius;
        int previousY = centerY;
        for (int degree = 1; degree <= 360; degree++)
        {
            const float radians = static_cast<float>(degree) * 0.017453292519943295f;
            const int currentX = centerX + static_cast<int>(std::cos(radians) * radius);
            const int currentY = centerY + static_cast<int>(std::sin(radians) * radius);

            SDL_RenderDrawLine(renderer, previousX, previousY, currentX, currentY);
            previousX = currentX;
            previousY = currentY;
        }
    }
};

#endif
