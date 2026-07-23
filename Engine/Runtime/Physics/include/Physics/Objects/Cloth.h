#pragma once

#include <vector>
#include "../Point.h"
#include "Stick.h"

namespace AE::Physics
{

class Mouse;

class Cloth
{
private:
    Vector2D gravity = {0.0f, 981.0f}; // 981.0f standart
    float drag = 0.01f;
    float elasticity = 10000.0f;

    std::vector<Point*> points;
    std::vector<Stick*> sticks;

public:
    Cloth() = default;
    Cloth(int width, int height, int spacing, int startX, int startY);
    ~Cloth();

    void Update(Mouse* mouse, float deltaTime, int windowWidth, int windowHeight);
    const std::vector<Stick*>& GetSticks() const { return sticks; }
};

}

using AE::Physics::Cloth;
