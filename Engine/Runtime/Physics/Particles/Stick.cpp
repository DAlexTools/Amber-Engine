
#include "Physics/Particles/Stick.h"
#include "Physics/Particles/Point.h"
#include "Physics/Core/Macro.h"
#include "Logging/Logger.h"
#include <iostream>

namespace AE::Physics
{

Stick::Stick(Point& p0, Point& p1, float lenght, uint32 color, uint32 selectedColor)
	: point0(p0)
	, point1(p1)
	, Length(lenght)
	, color(color)
	, colorWhenSelected(selectedColor)
{
	AE::Logger::Log("Stick contruction called", "Physics");
}

Stick::Stick(Point& p0, Point& p1, float lenght)
	: point0(p0)
	, point1(p1)
	, Length(lenght)
{
	color = TRANSPARENT_WHITE_COLOR;
	colorWhenSelected = GREEN_COLOR;
	static int count = 0;
	AE::Logger::Log("Stick contruction called. ( position x - " + std::to_string(p0.GetPosition().X) + ". position y " + std::to_string(p0.GetPosition().Y) + " count - " + std::to_string(count++) + " ). ", "Physics");
}

bool Stick::IsActive() const
{
	return bIsActive;
}

uint32 Stick::GetRenderColor() const
{
	return bIsSelected ? colorWhenSelected : color;
}

FVector2D Stick::GetPoint0Position() const
{
	return point0.GetPosition();
}

FVector2D Stick::GetPoint1Position() const
{
	return point1.GetPosition();
}

void Stick::Update()
{
	if (!bIsActive)
		return;

	FVector2D p0Pos = point0.GetPosition();
	FVector2D p1Pos = point1.GetPosition();

	FVector2D diff = p0Pos - p1Pos;
	float dist = sqrtf(diff.X * diff.X + diff.Y * diff.Y);
	float diffFactor = (Length - dist) / dist;
	FVector2D offset = diff * diffFactor * 0.5f;

	point0.SetPosition(p0Pos.X + offset.X, p0Pos.Y + offset.Y);
	point1.SetPosition(p1Pos.X - offset.X, p1Pos.Y - offset.Y);
}

void Stick::Break()
{
	bIsActive = false;
}

void Stick::SetIsSelected(bool value)
{
	bIsSelected = value;
}

} // namespace AE::Physics
