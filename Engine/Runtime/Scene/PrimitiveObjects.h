#ifndef AMBER_RUNTIME_SCENE_PRIMITIVE_OBJECTS_H
#define AMBER_RUNTIME_SCENE_PRIMITIVE_OBJECTS_H

#include "Scene/Object.h"

namespace AE::Scene
{

class BoxObject : public Object
{
public:
    explicit BoxObject(ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

class CircleObject : public Object
{
public:
    explicit CircleObject(ObjectData data);

    const char* GetClassName() const override;
    void ConfigureEntity(Registry& ownerRegistry) override;
};

}

#endif
