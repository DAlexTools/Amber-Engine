#ifndef BOXCOLLISIONCOMPONENT_H
#define BOXCOLLISIONCOMPONENT_H

#include <glm/glm.hpp>

struct BoxCollisionComponent
{
    int width;
    int height;
    glm::vec2 offset;

    BoxCollisionComponent(int width = 0, int height = 0, glm::vec2 offset = glm::vec2(0))
    {
        this->width = width;
        this->height = height;
        this->offset = offset;
    }
};


#endif