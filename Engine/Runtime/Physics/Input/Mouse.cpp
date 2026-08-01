

#include "Physics/Input/Mouse.h"

namespace AE::Physics
{

void Mouse::IncreaseCursorSize(float increment)
{
	if (cursorSize + increment > maxCursorSize || cursorSize + increment < minCursorSize)
		return;

	cursorSize += increment;
}

void Mouse::UpdatePosition(int x, int y)
{
	prevPos.X = pos.X;
	prevPos.Y = pos.Y;
	pos.X = x;
	pos.Y = y;
}

} // namespace AE::Physics
