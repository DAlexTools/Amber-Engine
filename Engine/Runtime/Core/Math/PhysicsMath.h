#ifndef PHYSICS_MATH_H
#define PHYSICS_MATH_H

#include <cmath>	
#include <memory.h>
#include <limits>
#include <type_traits>


namespace Math
{

static constexpr float Pi = 3.1415926535f;
static constexpr float TwoPi = Pi * 2.0f;
static constexpr float PiOver2 = Pi / 2.0f;
static constexpr float Infinity = std::numeric_limits<float>::infinity();
static constexpr float NegInfinity = -std::numeric_limits<float>::infinity();

/**
 * 
 */
inline float ToRadians(float degrees)
{
    return degrees * ( Pi / 180.0f );
}

/**
 * 
 */
inline float ToDegrees(float radians)
{ 
    return radians * ( 180.0f / Pi );
}

/**
 * 
 */
inline bool NearZero(float val, float epsilon = 0.001f)
{
    if (std::fabs(val) <= epsilon)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * 
 */
inline float Abs(float value)
{
    return std::fabs(value);
}

/**
 * 
 */
inline float Cos(float angle)
{
    return std::cos(angle);
}

/**
 * 
 */
inline float Sin(float angle)
{
    return std::sin(angle);
}

/**
 * 
 */
inline float Tan(float angle)
{
    return std::tan(angle);
}

/**
 * 
 */
inline float Acos(float value)
{
    return std::acos(value);
}

/**
 * 
 */
inline float Atan2(float y, float x)
{
    return std::atan2(y, x);
}

/**
 * 
 */
inline float Cot(float angle)
{
    return 1.0f / Tan(angle);
}

inline float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

/**
 * 
 */
inline float Sqrt(float value)
{
    return std::sqrt(value);
}

/**
 * 
 */
inline float Fmod(float numer, float denom)
{
    return std::fmod(numer, denom);
}

/**
 * 
 */
template <typename T>
constexpr const T& Max(const T& a, const T& b)
{
    return (a < b ? b : a);
}


/**
 * 
 */
template <typename T>
constexpr const T& Min(const T& a, const T& b)
{
    return (a < b ? a : b);
}

template <typename T>
constexpr const T& Clamp(const T& val, const T& lower, const T& upper)
{
    return Min(upper, Max(lower, val));
}


/**
 * Check if inV is a power of
 */
template <typename T>
constexpr bool IsPowerOfTwo(T inV)
{
    return (inV & (inV - 1)) == 0;
}

/**
 * Get the sign of a value 
 */
template <typename T> 
constexpr T Sign(T inV)
{
    return inV < 0 ? T(-1) : T(1);
}

/**
 * Simple implementation power of 2 of a value, or the itself if the value is already a power of 2
 */
template <class  To, class From>
To BitCast(const From& InValue)
{
    static_assert(std::is_trivially_constructible_v<To>);
    static_assert(sizeof(From) == sizeof(To));

    union FromTo
    {
        To      mTo;
        From    mFrom;
    };

    FromTo convert;
    convert.mFrom = InValue;
    
    return convert.mTo;
}
}

#endif
