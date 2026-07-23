#ifndef RENDERGUISYSTEM_H
#define RENDERGUISYSTEM_H

#include "../EntityComponentSystem/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include <imgui/imgui.h>
#include <imgui/imgui_sdl.h>



/**
 * Render GUI system 
 * add new entity in game  
 */
class RenderGUISystem : public System
{
    bool* open = NULL;
public:
    RenderGUISystem() = default;
    
    void Update(const std::unique_ptr<Registry>& registry, bool beginFrame = true)
    {
        if (beginFrame)
        {
            ImGui::NewFrame();
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Game"))
            {
                
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();    

        ImGui::SetNextWindowSize(ImVec2(500, 600));
        if (ImGui::Begin("Spawn Enemies"))
        {
            static int enemyXPos = 0;
            static int enemyYPos = 0;
            static int scaleX = 0;
            static int scaleY = 0;
            static int velX = 0;
            static int velY = 0;
            static int health = 100;
            static float rotation = 0.0f;

            static float projAngle = 0.0f;
            static float projSpeed = 100.0f;

            static int projRepeat = 10;
            static int projDuration = 10;
            static int damage = 10;
            const char* sprites [] = 
            {
                 "tank-texture", 
                 "truck-texture", 
                 "su27-texture", 
                 "f22-texture", 
                 "fw190-texture",  
                 };
                 
            static int selectedSpriteIndex = 0;

            if (ImGui::CollapsingHeader("Sprite Settings ", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Combo("texture_id", &selectedSpriteIndex, sprites, IM_ARRAYSIZE(sprites));
            }
            ImGui::Spacing();
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Transform Enemy Settings ", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputInt("Enemy x position", &enemyXPos);
                ImGui::InputInt("Enemy y position", &enemyYPos);
                ImGui::SliderInt("Enemy scale x", &scaleX, 1, 5);
                ImGui::SliderInt("Enemy scale y", &scaleY, 1, 5);
                ImGui::SliderAngle("Rotation (&deg)", &rotation, 0, 360);
            }
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Rigid Body Settings ", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputInt("Velocity x", &velX);
                ImGui::InputInt("Velocity y", &velY);
            }
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Projectile Enemy Settings ", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderAngle("Angle (&deg)", &projAngle, 0, 360);
                ImGui::SliderFloat("Speed (px/sec)", &projSpeed, 10, 1000);
                ImGui::InputInt("Repeat (sec)", &projRepeat);
                ImGui::InputInt("duration (sec)", &projDuration);
                ImGui::InputInt("Projectile Damage", &damage, 10, 100);
            }
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Enemy Health", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::InputInt("Enemy Health Value", &health, 1, 1000);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            
            if (ImGui::Button("Create new entity"))
            {
                Entity entity = registry->CreateEntity();

                entity.Group("enemies");
                entity.AddComponent<TransformComponent>(glm::vec2(enemyXPos, enemyYPos), glm::vec2(scaleX, scaleY), glm::degrees(rotation));
                entity.AddComponent<RigidBodyComponent>(glm::vec2(velY, velX));
                entity.AddComponent<SpriteComponent>(sprites[selectedSpriteIndex], 32, 32, 2);
                entity.AddComponent<BoxCollisionComponent>(32 * scaleX, 32 * scaleY);
                double projVelX = cos(projAngle) * projSpeed;
                double projVelY = sin(projAngle) * projSpeed;
                entity.AddComponent<ProjectileEmitterComponent>(glm::vec2(projVelX, projVelY), projRepeat * 1000, projDuration * 1000, damage, false);
                entity.AddComponent<HealthComponent>(health);

                enemyXPos = enemyYPos = 0;
                scaleX = scaleY = 1;
                projRepeat = projDuration = 10;
                projSpeed = 100;
                health = 100;
                damage = 10;
                
            }  
        }
        ImGui::End();

        // Overlay(open);
        ImGui::Render();
        ImGuiSDL::Render(ImGui::GetDrawData());

    }


    static void Overlay(bool* is_open)
    {
        const float DISTANCE = 10.0f;
        static int corner = 0;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (corner != -1)
        {
            window_flags |= ImGuiWindowFlags_NoMove;
            ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
            ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        }
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin("Example: Simple overlay", is_open, window_flags))
        {
            ImGui::Text("Simple overlay\n" "in the corner of the screen.\n" "(right-click to change position)");
            ImGui::Separator();
            if (ImGui::IsMousePosValid())
                ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
            else
                ImGui::Text("Mouse Position: <invalid>");
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                if (is_open && ImGui::MenuItem("Close")) *is_open = false;
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
};

#endif
