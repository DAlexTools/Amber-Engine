#ifndef CIRCLECOLLISIONCOMPONENT_H
#define CIRCLECOLLISIONCOMPONENT_H

#include <glm/glm.hpp>

struct CircleCollisionComponent
{
    int width;
    int height;
    int radius;
    glm::vec2 offset;

    CircleCollisionComponent(int width = 0, int height, int radius, glm::vec2 offset = glm::vec2(0))
    {
        this->width = width;
        this->height = height;
        this->radius = radius;
        this->offset = offset;
    }
};

#endif