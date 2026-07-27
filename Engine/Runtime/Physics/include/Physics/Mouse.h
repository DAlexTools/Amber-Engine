#pragma once


#include "Core/Math/Vector2D.h"

namespace AE::Physics
{

class Mouse
{
private:
    bool            leftButtonDown = false;
    bool            rightButtonDown = false;
    float           maxCursorSize = 150;
    float           minCursorSize = 10;
    float           cursorSize = 20;
    FVector2D        pos;
    FVector2D        prevPos;

public:
    Mouse()     = default;
    ~Mouse()    = default;

    const FVector2D& GetPosition() const { return pos; }
    const FVector2D& GetPreviousPosition() const { return prevPos; }
    void            UpdatePosition(int x, int y);

    bool            GetLeftButtonDown() const { return leftButtonDown; }
    void            SetLeftMouseButton(bool state) { this->leftButtonDown = state; }

    bool            GetRightMouseButton() const { return rightButtonDown; }
    void            SetRightMouseButton(bool state) { this->rightButtonDown = state; }

    void            IncreaseCursorSize(float increment);
    float           GetCursorSize() const { return cursorSize; }
};

}

using AE::Physics::Mouse;
