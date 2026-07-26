#ifndef AMBER_RUNTIME_SCENE_SCENE_OBJECT_COMPONENTS_H
#define AMBER_RUNTIME_SCENE_SCENE_OBJECT_COMPONENTS_H

#include <string>

namespace AE::Scene
{

struct SceneObjectComponent
{
    std::string name;
    std::string className;
    std::string assetId;
    bool visible = true;

    SceneObjectComponent(
        std::string objectName = {},
        std::string objectClassName = "Object",
        std::string objectAssetId = {},
        bool objectVisible = true);
};

struct SceneSpriteComponent
{
    std::string assetId;
    float width = 0.0f;
    float height = 0.0f;

    SceneSpriteComponent(std::string spriteAssetId = {}, float spriteWidth = 0.0f, float spriteHeight = 0.0f);
};

}

#endif
