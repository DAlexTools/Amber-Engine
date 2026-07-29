#pragma once

#include <stdint.h>
#include "Core/Math/Vector2D.h"

namespace AE::Physics
{

class Point;
class Stick
{
public:
    Stick(Point& p0, Point& p1, float lenght, uint32_t color, uint32_t selectedColor);
    Stick(Point& p0, Point& p1, float lenght);
    ~Stick() = default;

    void SetIsSelected(bool value);
    bool IsActive() const;
    uint32_t GetRenderColor() const;
    FVector2D GetPoint0Position() const;
    FVector2D GetPoint1Position() const;

    void Update();
    void Break();

private:
    Point& point0;
    Point& point1;

    float Length;

    bool bIsActive = true;
    bool bIsSelected = false;

    uint32_t color = 0xFF00FF00;
    uint32_t colorWhenSelected = 0xFFFF0000;
};

}

using AE::Physics::Stick;
