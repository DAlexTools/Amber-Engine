#ifndef Vector2D_H
#define Vector2D_H

#include <iostream>

/**
 * Vector2D 
 */
namespace AE::Physics
{

struct Vector2D
{
public:    
    Vector2D();
    Vector2D(float x, float y);
    ~Vector2D() = default;

    const float         GetX() const { return x; }          // get x coord 
    const float         GetY() const { return y; }          // get y coord
    void                Add(const Vector2D& v);                    // v1.Add(v2)
    void                Sub(const Vector2D& v);                    // v1.Sub(v2)
    void                Scale(const float n);                      // v1.Scale(n)
    Vector2D            Rotate(const float angle) const;       // v1.Rotate(angle)

    float               Magnitude() const;                        // v1.Magnitude()
    float               MagnitudeSquared() const;                 // v1.MagnitudeSquared()

    Vector2D&           Normalize();                          // v1.Normalize()
    Vector2D            UnitVector() const;                    // v1.UnitVector()
    Vector2D            Normal() const;                        // n = v1.Normalize ()

    float               DotProduct(const Vector2D& v) const;      // v1.Dot(v2)
    float               CrossProduct(const Vector2D& v) const;    // v1.Cross(v2)
    float               Length() const;
    float               LengthSq() const;
    static Vector2D     ZeroVector();            // ZeroVector return (Vector2D(0.0, 0.0)

#pragma region Operator_
    /* Override operators function */
    Vector2D&   operator=(const Vector2D& v);         // v1 = v2
    bool        operator==(const Vector2D& v) const;       // v1 == v2
    bool        operator!=(const Vector2D& v) const;       // v1 != v2

    Vector2D    operator+(const Vector2D& v) const;    // v1 + v2
    Vector2D    operator-(const Vector2D& v) const;    // v1 - v2
    Vector2D    operator*(const float n) const;        // v1 * n
    Vector2D    operator/(const float n) const;        // v1 / n
    Vector2D    operator-();                           // -v1

    Vector2D&   operator+=(const Vector2D& v);        // v1 += v2
    Vector2D&   operator-=(const Vector2D& v);        // v1 -= v2
    Vector2D&   operator*=(const float n);            // v1 *= n
    Vector2D&   operator/=(const float n);            // v1 /= n
    
#pragma endregion Operator_

#pragma region StaticConstants
    static const Vector2D Zero;
    static const Vector2D UnitX;
    static const Vector2D UnitY;
    static const Vector2D NegUnitX;
    static const Vector2D NegUnitY;
#pragma endregion StaticConstants
    float x;
    float y;
};

}

using AE::Physics::Vector2D;

#endif
