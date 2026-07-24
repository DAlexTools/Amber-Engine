#ifndef CAMERAMOVEMENTSYSTEM_H
#define CAMERAMOVEMENTSYSTEM_H

#include "../EntityComponentSystem/ECS.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/TransformComponent.h"
#include "../Classes/Engine.h"

class CameraMovementSystem : public System
{
public:
    CameraMovementSystem()
    {
        RequireComponent<CameraFollowComponent>();
        RequireComponent<TransformComponent>();
    }

    void Update(SDL_Rect& camera)
    {
        for (auto entity : GetSystemEntity())
        {
            auto transform = entity.GetComponent<TransformComponent>();

            if (transform.position.x + (camera.w / 2) < AE::Engine::MapWidth)
            {
                camera.x = transform.position.x - (AE::Engine::WindowWidth / 2);
            }

            if (transform.position.y + (camera.h / 2) < AE::Engine::MapHeight)
            {
                camera.y = transform.position.y - (AE::Engine::WindowHeight / 2);
            }
            
            camera.x = camera.x < 0 ? 0 : camera.x;
            camera.y = camera.y  < 0 ? 0 : camera.y;
            camera.x = camera.x > camera.w ? camera.w : camera.x;
            camera.y = camera.y > camera.h ? camera.h : camera.y;
            
        }
    }
};


#endif
