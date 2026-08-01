#pragma once

#include "Core/Math/Vector2D.h"
#include "Physics/Input/Mouse.h"

namespace AE::Physics
{

class Stick;
/**
 *
 */
class Point
{
private:
	Stick* sticks[2] = {nullptr};

	FVector2D pos;
	FVector2D prevPos;
	FVector2D initPos;
	bool isPinned = false;

	bool isSelected = false;

	void KeepInsideView(int windowWidth, int windowHeight);

public:
	Point() = default;
	Point(float x, float y);
	~Point() = default;

	void AddStick(Stick* stick, int index);

	const FVector2D& GetPosition() const
	{
		return pos;
	}
	void SetPosition(float x, float y);

	void Pin();

	void Update(float deltaTime, float drag, const FVector2D& acceleration, float elasticity, Mouse* mouse, int windowWidth, int windowHeight);
};

} // namespace AE::Physics

using AE::Physics::Point;
