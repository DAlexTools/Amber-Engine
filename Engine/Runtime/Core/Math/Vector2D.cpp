#include "Vector2D.h"
#include <math.h>
#include "PhysicsMath.h"

namespace AE::Physics
{

const float k_pi = 3.14159265358979323846264f;

const FVector2D FVector2D::Zero       (0.0f, 0.0f);
const FVector2D FVector2D::UnitX      (1.0f, 0.0f);
const FVector2D FVector2D::UnitY      (0.0f, 1.0f);
const FVector2D FVector2D::NegUnitX   (-1.0f, 0.0f);
const FVector2D FVector2D::NegUnitY   (0.0f, -1.0f);


FVector2D::FVector2D() : X(0.0), Y(0.0) 
{

}

FVector2D::FVector2D(float InX, float InY) : X(InX), Y(InY) 
{

}

FVector2D FVector2D::ZeroVector()
{
    return FVector2D(0.0, 0.0);
}

void FVector2D::Add(const FVector2D& V)
{
    X += V.X;
    Y += V.Y;
}

void FVector2D::Sub(const FVector2D& V)
{
    X -= V.X;
    Y -= V.Y;
}

void FVector2D::Scale(const float N)
{
    X *= N;
    Y *= N;
}

FVector2D FVector2D::Rotate(const float Angle) const
{
    FVector2D Result;
    Result.X = Y * Math::Cos(Angle) - Y * Math::Sin(Angle);
    Result.Y = X * Math::Sin(Angle) + Y * Math::Cos(Angle);

    return Result;
}

float FVector2D::Magnitude() const
{
    return Math::Sqrt(X * X + Y * Y);
}

float FVector2D::MagnitudeSquared() const
{
    return (X * X + Y * Y);
}

FVector2D& FVector2D::Normalize()
{
    float Length = Magnitude();

    if (Length != 0.0)
    {
        X /= Length;
        Y /= Length;
    }

    return *this;
}

FVector2D FVector2D::UnitVector() const
{
    FVector2D Result = FVector2D(0, 0);
    float Length = Magnitude();

    if (Length != 0.0)
    {
        Result.X = X / Length;
        Result.Y = Y / Length;
    }

    return Result;
}

FVector2D FVector2D::Normal() const
{
    return FVector2D(Y, -X).Normalize();
}

float FVector2D::DotProduct(const FVector2D& V) const
{
    return (X * V.X) + (Y * V.Y);
}

float FVector2D::CrossProduct(const FVector2D& V) const
{
    return (X * V.Y) - (Y * V.X);
}

float FVector2D::Length() const
{
    return sqrtf(X * X + Y * Y);
}

float FVector2D::LengthSq() const
{
    return (X * X + Y * Y);
}

FVector2D& FVector2D::operator=(const FVector2D& V)
{
    X = V.X;
    Y = V.Y;
    return *this;
}

bool FVector2D::operator==(const FVector2D& V) const
{
    return X == V.X && Y == V.Y;
}

bool FVector2D::operator!=(const FVector2D& V) const
{
    return !(*this == V);
}

FVector2D FVector2D::operator+(const FVector2D& V) const
{
    FVector2D Result;
    Result.X = X + V.X;
    Result.Y = Y + V.Y;

    return Result;
}


FVector2D FVector2D::operator-(const FVector2D& V) const
{
    FVector2D Result;
    Result.X = X - V.X;
    Result.Y = Y - V.Y;

    return Result;
}

FVector2D FVector2D::operator*(const float N) const
{
    FVector2D Result;
    Result.X = X * N;
    Result.Y = Y * N;

    return Result;
}

FVector2D FVector2D::operator/(const float N) const
{
    FVector2D Result;
    Result.X = X / N;
    Result.Y = Y / N;

    return Result;
}

FVector2D& FVector2D::operator+=(const FVector2D& V)
{
    X += V.X;
    Y += V.Y;

    return *this;
}

FVector2D& FVector2D::operator-=(const FVector2D& V)
{
    X -= V.X;
    Y -= V.Y;

    return *this;
}

FVector2D& FVector2D::operator*=(const float N)
{
    X *= N;
    Y *= N;

    return *this;
}

FVector2D& FVector2D::operator/=(const float N)
{
    X /= N;
    Y /= N;

    return *this;
}

FVector2D FVector2D::operator-()
{
    FVector2D Result;
    Result.X = X * -1;
    Result.Y = Y * -1;

    return Result;
}

}
