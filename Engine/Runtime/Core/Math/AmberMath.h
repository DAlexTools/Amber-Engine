#ifndef AMBER_MATH_H
#define AMBER_MATH_H

#include <cmath>	
#include <memory.h>
#include <limits>
#include <type_traits>


namespace AE::Math
{

static constexpr float Pi = 3.14159265358979323846264f;
static constexpr float TwoPi = Pi * 2.0f;
static constexpr float PiOver2 = Pi / 2.0f;
static constexpr float Infinity = std::numeric_limits<float>::infinity();
static constexpr float NegativeInfinity = -std::numeric_limits<float>::infinity();

/**
 * 
 */
inline float ToRadians(float Degrees)
{
    return Degrees * ( Pi / 180.0f );
}

/**
 * 
 */
inline float ToDegrees(float Radians)
{ 
    return Radians * ( 180.0f / Pi );
}

/**
 * 
 */
inline bool NearZero(float InValue, float Epsilon = 0.001f)
{
    if (std::fabs(InValue) <= Epsilon)
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
inline float Abs(float InValue)
{
    return std::fabs(InValue);
}

/**
 * 
 */
inline float Cos(float Angle)
{
    return std::cos(Angle);
}

/**
 * 
 */
inline float Sin(float Angle)
{
    return std::sin(Angle);
}

/**
 * 
 */
inline float Tan(float Angle)
{
    return std::tan(Angle);
}

/**
 * 
 */
inline float Acos(float InValue)
{
    return std::acos(InValue);
}

/**
 * 
 */
inline float Atan2(float Y, float X)
{
    return std::atan2(Y, X);
}

/**
 * 
 */
inline float Cot(float Angle)
{
    return 1.0f / Tan(Angle);
}

inline float Lerp(float A, float B, float F)
{
    return A + F * (B - A);
}

/**
 * 
 */
inline float Sqrt(float InValue)
{
    return std::sqrt(InValue);
}

/**
 * 
 */
inline float Fmod(float Numer, float Denom)
{
    return std::fmod(Numer, Denom);
}

/**
 * 
 */
template <typename T>
constexpr const T& Max(const T& A, const T& B)
{
    return (A < B ? B : A);
}


/**
 * 
 */
template <typename T>
constexpr const T& Min(const T& A, const T& B)
{
    return (A < B ? A : B);
}

template <typename T>
constexpr const T& Clamp(const T& InValue, const T& Lower, const T& Upper)
{
    return Min(Upper, Max(Lower, InValue));
}


/**
 * Check if inV is a power of
 */
template <typename T>
constexpr bool IsPowerOfTwo(T InValue)
{
    return (InValue& (InValue - 1)) == 0;
}

/**
 * Get the sign of a value 
 */
template <typename T> 
constexpr T Sign(T InValue)
{
    return InValue < 0 ? T(-1) : T(1);
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
