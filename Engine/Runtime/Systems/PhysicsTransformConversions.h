#ifndef PHYSICSTRANSFORMCONVERSIONS_H
#define PHYSICSTRANSFORMCONVERSIONS_H

namespace EnginePhysics
{
    constexpr float DegreesToRadians(float degrees)
    {
        return degrees * 0.017453292519943295f;
    }

    constexpr float RadiansToDegrees(float radians)
    {
        return radians * 57.29577951308232f;
    }
}

#endif
