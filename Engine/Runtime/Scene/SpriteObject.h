#ifndef AMBER_RUNTIME_SCENE_SPRITE_OBJECT_H
#define AMBER_RUNTIME_SCENE_SPRITE_OBJECT_H

#include "Scene/Object.h"

namespace AE::Scene
{

class SpriteObject : public Object
{
public:
    explicit SpriteObject(ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

}

#endif
