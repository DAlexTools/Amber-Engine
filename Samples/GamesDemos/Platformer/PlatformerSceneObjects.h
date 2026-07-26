#ifndef PLATFORMER_SCENE_OBJECTS_H
#define PLATFORMER_SCENE_OBJECTS_H

#include "Scene/Object.h"

#include <string>

namespace AE::Scene
{
class ObjectFactory;
}

namespace PlatformerScene
{

struct PlayerSpawnComponent
{
};

struct GoalComponent
{
};

struct CoinComponent
{
};

struct SolidPlatformComponent
{
};

class PlayerSpawnObject final : public AE::Scene::Object
{
public:
    explicit PlayerSpawnObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class GoalObject final : public AE::Scene::Object
{
public:
    explicit GoalObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class CoinObject final : public AE::Scene::Object
{
public:
    explicit CoinObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class SolidPlatformObject final : public AE::Scene::Object
{
public:
    explicit SolidPlatformObject(AE::Scene::ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

void RegisterPlatformerSceneObjects(AE::Scene::ObjectFactory& factory);
bool IsPlatformerGameplayClass(const std::string& className);

}

#endif
