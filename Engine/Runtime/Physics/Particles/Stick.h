#pragma once

#include "Core/Platform/PlatformTypes.h"
#include "Core/Math/Vector2D.h"

namespace AE::Physics
{

class Point;
class Stick
{
public:
	Stick(Point& p0, Point& p1, float lenght, uint32 color, uint32 selectedColor);
	Stick(Point& p0, Point& p1, float lenght);
	~Stick() = default;

	void SetIsSelected(bool value);
	bool IsActive() const;
	uint32 GetRenderColor() const;
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

	uint32 color = 0xFF00FF00;
	uint32 colorWhenSelected = 0xFFFF0000;
};

} // namespace AE::Physics

using AE::Physics::Stick;
