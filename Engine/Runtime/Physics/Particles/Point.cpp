#include "Physics/Particles/Point.h"
#include "Physics/Particles/Stick.h"

namespace AE::Physics
{

Point::Point(float x, float y)
{
	pos = prevPos = initPos = FVector2D(x, y);
}

void Point::KeepInsideView(int windowWidth, int windowHeight)
{
	if (pos.X > windowWidth)
	{
		pos.X = windowWidth;
		prevPos.X = pos.X;
	}
	else if (pos.X < 0)
	{
		pos.X = 0;
		prevPos.X = pos.X;
	}

	if (pos.Y > windowHeight)
	{
		pos.Y = windowHeight;
		prevPos.Y = pos.Y;
	}
	else if (pos.Y < 0)
	{
		pos.Y = 0;
		prevPos.Y = pos.Y;
	}
}

void Point::AddStick(Stick* stick, int index)
{
	sticks[index] = stick;
}

void Point::SetPosition(float x, float y)
{
	pos.X = x;
	pos.Y = y;
}

void Point::Pin()
{
	isPinned = true;
}

void Point::Update(float deltaTime, float drag, const FVector2D& acceleration, float elasticity, Mouse* mouse, int windowWidth, int windowHeight)
{
	FVector2D cursorToPosDir = pos - mouse->GetPosition();
	float cursorToPosDist = cursorToPosDir.X * cursorToPosDir.X + cursorToPosDir.Y * cursorToPosDir.Y;
	float cursorSize = mouse->GetCursorSize();
	isSelected = cursorToPosDist < cursorSize * cursorSize;

	for (Stick* stick : sticks)
	{
		if (stick != nullptr)
			stick->SetIsSelected(isSelected);
	}

	if (mouse->GetLeftButtonDown() && isSelected)
	{
		FVector2D difference = mouse->GetPosition() - mouse->GetPreviousPosition();

		if (difference.X > elasticity)
			difference.X = elasticity;
		if (difference.Y > elasticity)
			difference.Y = elasticity;
		if (difference.X < -elasticity)
			difference.X = -elasticity;
		if (difference.Y < -elasticity)
			difference.Y = -elasticity;

		prevPos = pos - difference;
	}

	if (mouse->GetRightMouseButton() && isSelected)
	{
		for (Stick* stick : sticks)
		{
			if (stick != nullptr)
				stick->Break();
		}
	}

	if (isPinned)
	{
		pos = initPos;
		return;
	}

	FVector2D newPos = pos + (pos - prevPos) * (1.0f - drag) + acceleration * (1.0f - drag) * deltaTime * deltaTime;
	prevPos = pos;
	pos = newPos;

	KeepInsideView(windowWidth, windowHeight);
}

} // namespace AE::Physics
