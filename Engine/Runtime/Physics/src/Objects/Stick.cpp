
#include "Objects/Stick.h"
#include "Point.h"
#include "Utils/Macro.h"
#include "Logging/Logger.h"
#include <iostream>

namespace AE::Physics
{

Stick::Stick(Point &p0, Point &p1, float lenght, uint32_t color, uint32_t selectedColor)
:point0(p0), point1(p1),Length(lenght), color(color), colorWhenSelected(selectedColor)
{
    AE::Logger::Log("Stick contruction called", "Physics");
}

Stick::Stick(Point& p0, Point& p1, float lenght) : point0(p0), point1(p1), Length(lenght)
{
    color = TRANSPARENT_WHITE_COLOR;
    colorWhenSelected = GREEN_COLOR;
    static int count = 0;
    AE::Logger::Log("Stick contruction called. ( position x - "  + std::to_string(p0.GetPosition().GetX())
                                            + ". position y " + std::to_string(p0.GetPosition().GetY()) 
                                            + " count - " + std::to_string(count++) + " ). ", "Physics");
}

bool Stick::IsActive() const
{
    return bIsActive;
}

uint32_t Stick::GetRenderColor() const
{
    return bIsSelected ? colorWhenSelected : color;
}

Vector2D Stick::GetPoint0Position() const
{
    return point0.GetPosition();
}

Vector2D Stick::GetPoint1Position() const
{
    return point1.GetPosition();
}

void Stick::Update()
{
    if (!bIsActive) return;

    Vector2D p0Pos = point0.GetPosition();
    Vector2D p1Pos = point1.GetPosition();

    Vector2D diff = p0Pos - p1Pos;
    float dist = sqrtf(diff.x * diff.x + diff.y * diff.y);
    float diffFactor = (Length - dist) / dist;
    Vector2D offset = diff * diffFactor * 0.5f;

    point0.SetPosition(p0Pos.x + offset.x, p0Pos.y + offset.y);
    point1.SetPosition(p1Pos.x - offset.x, p1Pos.y - offset.y);
}

void Stick::Break()
{
    bIsActive = false;
}

void Stick::SetIsSelected(bool value)
{
    bIsSelected = value;
}

}
