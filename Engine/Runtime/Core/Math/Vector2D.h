#ifndef VECTOR2D_H
#define VECTOR2D_H

#include <cmath>
#include <limits>
#include <type_traits>

namespace AE::Math
{

/**
 * @brief Two dimensional vector with a floating point scalar type.
 *
 * `TVector2` stores two coordinates and provides common vector operations used
 * by runtime math, physics, gameplay, and editor code. Coordinates are exposed
 * as both `X/Y` and `x/y` while the lowercase names are kept for compatibility
 * with older code and data imported from libraries that use lowercase fields.
 *
 * @tparam T Floating point scalar type, usually `float` or `double`.
 */
template <typename T>
struct TVector2
{
	static_assert(std::is_floating_point_v<T>, "TVector2<T> requires a floating point T");

	/** @brief Coordinate storage with compatibility names and array access. */
	union
	{
		struct
		{
			/** @brief Horizontal coordinate. */
			T X;

			/** @brief Vertical coordinate. */
			T Y;
		};
		struct
		{
			/** @brief Lowercase compatibility alias for `X`. */
			T x;

			/** @brief Lowercase compatibility alias for `Y`. */
			T y;
		};

		/** @brief Array-style coordinate access: `XY[0]` is X, `XY[1]` is Y. */
		T XY[2];
	};

	/** @brief Vector with both coordinates equal to zero. */
	static const TVector2 Zero;

	/** @brief Vector with both coordinates equal to one. */
	static const TVector2 One;

	/** @brief Vector with both coordinates equal to one half. */
	static const TVector2 Half;

	/** @brief Vector with both coordinates set to positive infinity. */
	static const TVector2 Infinity;

	/** @brief Vector with both coordinates set to negative infinity. */
	static const TVector2 NegativeInfinity;

	/** @brief Unit-length diagonal vector where X and Y point equally. */
	static const TVector2 Unit45Deg;

	/** @brief Unit vector pointing along positive X. */
	static const TVector2 UnitX;

	/** @brief Unit vector pointing along positive Y. */
	static const TVector2 UnitY;

	/** @brief Unit vector pointing along negative X. */
	static const TVector2 NegUnitX;

	/** @brief Unit vector pointing along negative Y. */
	static const TVector2 NegUnitY;

	/** @brief Creates a zero vector. */
	constexpr TVector2()
		: X(T(0))
		, Y(T(0))
	{
	}

	/**
	 * @brief Creates a vector from explicit coordinates.
	 *
	 * @param InX Horizontal coordinate.
	 * @param InY Vertical coordinate.
	 */
	constexpr TVector2(T InX, T InY)
		: X(InX)
		, Y(InY)
	{
	}

	/**
	 * @brief Creates a vector where both coordinates use the same value.
	 *
	 * @param Value Value copied into both X and Y.
	 */
	explicit constexpr TVector2(T Value)
		: X(Value)
		, Y(Value)
	{
	}

	/**
	 * @brief Adds another vector into this vector.
	 *
	 * This performs `X += Value.X` and `Y += Value.Y`.
	 */
	void Add(const TVector2& Value)
	{
		X += Value.X;
		Y += Value.Y;
	}

	/**
	 * @brief Subtracts another vector from this vector.
	 *
	 * This performs `X -= Value.X` and `Y -= Value.Y`.
	 */
	void Sub(const TVector2& Value)
	{
		X -= Value.X;
		Y -= Value.Y;
	}

	/**
	 * @brief Multiplies both coordinates by the same scalar.
	 *
	 * This performs `X *= Scalar` and `Y *= Scalar`.
	 */
	void Scale(T Scalar)
	{
		X *= Scalar;
		Y *= Scalar;
	}

	/**
	 * @brief Returns this vector rotated around the origin by an angle in radians.
	 *
	 * The rotated X coordinate is built from the old X projected by cosine minus
	 * the old Y projected by sine. The rotated Y coordinate is built from the old
	 * X projected by sine plus the old Y projected by cosine.
	 *
	 * @param Angle Rotation angle in radians.
	 * @return Rotated copy of this vector.
	 */
	[[nodiscard]] TVector2 Rotate(T Angle) const
	{
		const T CosValue = std::cos(Angle);
		const T SinValue = std::sin(Angle);
		return TVector2(
			X * CosValue - Y * SinValue,
			X * SinValue + Y * CosValue);
	}

	/**
	 * @brief Returns this vector rotated around the origin by an angle in radians.
	 *
	 * Compatibility wrapper for code that uses the `Rotated` name.
	 */
	[[nodiscard]] TVector2 Rotated(T Angle) const
	{
		return Rotate(Angle);
	}

	/**
	 * @brief Returns vector length.
	 *
	 * Same value as `Length()`.
	 */
	[[nodiscard]] T Magnitude() const
	{
		return Length();
	}

	/**
	 * @brief Returns squared vector length.
	 *
	 * Same value as `LengthSq()`. Use this when only comparing distances because
	 * it avoids the square root used by `Length()`.
	 */
	[[nodiscard]] constexpr T MagnitudeSquared() const
	{
		return LengthSq();
	}

	/**
	 * @brief Changes this vector to point in the same direction with length one.
	 *
	 * The vector is divided by its current length. Very small vectors are left
	 * unchanged to avoid division by zero or unstable values.
	 *
	 * @return This vector after normalization.
	 */
	TVector2& Normalize()
	{
		const T Size = Length();
		if (Size > T(0.000001))
		{
			X /= Size;
			Y /= Size;
		}
		return *this;
	}

	/**
	 * @brief Returns a normalized copy of this vector.
	 *
	 * The original vector is not changed.
	 */
	[[nodiscard]] TVector2 UnitVector() const
	{
		TVector2 Result = *this;
		Result.Normalize();
		return Result;
	}

	/**
	 * @brief Returns a normalized copy of this vector.
	 *
	 * Compatibility wrapper for code that uses the `GetNormalized` name.
	 */
	[[nodiscard]] TVector2 GetNormalized() const
	{
		return UnitVector();
	}

	/**
	 * @brief Returns the counter-clockwise perpendicular vector.
	 *
	 * This swaps the coordinates and negates the old Y, so `(X, Y)` becomes
	 * `(-Y, X)`.
	 */
	[[nodiscard]] constexpr TVector2 GetPerpendicular() const
	{
		return TVector2(-Y, X);
	}

	/**
	 * @brief Returns the normalized clockwise perpendicular vector.
	 *
	 * This is the normal direction expected by the current polygon collision
	 * code. It converts `(X, Y)` into `(Y, -X)` and then normalizes the result.
	 */
	[[nodiscard]] TVector2 Normal() const
	{
		return TVector2(Y, -X).UnitVector();
	}

	/**
	 * @brief Returns the dot product with another vector.
	 *
	 * The result is computed as `X * V.X + Y * V.Y`. It is useful for measuring
	 * how much two vectors point in the same direction.
	 */
	[[nodiscard]] constexpr T DotProduct(const TVector2& V) const
	{
		return X * V.X + Y * V.Y;
	}

	/**
	 * @brief Returns the 2D cross product magnitude with another vector.
	 *
	 * The result is computed as `X * V.Y - Y * V.X`. In 2D this is a scalar that
	 * tells which side the other vector is on and how strong that turn is.
	 */
	[[nodiscard]] constexpr T CrossProduct(const TVector2& V) const
	{
		return X * V.Y - Y * V.X;
	}

	/**
	 * @brief Returns vector length.
	 *
	 * The length is calculated by adding the squared coordinates and taking the
	 * square root.
	 */
	[[nodiscard]] T Length() const
	{
		return std::sqrt(LengthSq());
	}

	/**
	 * @brief Returns squared vector length.
	 *
	 * This is computed as `X * X + Y * Y`.
	 */
	[[nodiscard]] constexpr T LengthSq() const
	{
		return X * X + Y * Y;
	}

	/** @brief Returns squared vector length. */
	[[nodiscard]] constexpr T LengthSquared() const
	{
		return LengthSq();
	}

	/**
	 * @brief Returns true when both coordinates are close enough to zero.
	 *
	 * @param Tolerance Maximum absolute value allowed for each coordinate.
	 */
	[[nodiscard]] bool IsZero(T Tolerance = T(0.000001)) const
	{
		return std::fabs(X) <= Tolerance && std::fabs(Y) <= Tolerance;
	}

	/** @brief Returns true when both coordinates are smaller than `Other`. */
	[[nodiscard]] constexpr bool AllLessThan(const TVector2& Other) const
	{
		return X < Other.X && Y < Other.Y;
	}

	/** @brief Returns true when both coordinates are greater than `Other`. */
	[[nodiscard]] constexpr bool AllGreaterThan(const TVector2& Other) const
	{
		return X > Other.X && Y > Other.Y;
	}

	/** @brief Returns true when both coordinates are smaller than or equal to `Other`. */
	[[nodiscard]] constexpr bool AllLessOrEqual(const TVector2& Other) const
	{
		return X <= Other.X && Y <= Other.Y;
	}

	/** @brief Returns true when both coordinates are greater than or equal to `Other`. */
	[[nodiscard]] constexpr bool AllGreaterOrEqual(const TVector2& Other) const
	{
		return X >= Other.X && Y >= Other.Y;
	}

	/** @brief Returns a zero vector. */
	static constexpr TVector2 ZeroVector()
	{
		return TVector2(T(0), T(0));
	}

	/**
	 * @brief Returns distance between two points.
	 *
	 * The function subtracts `A` from `B` and returns the resulting vector length.
	 */
	static T Distance(const TVector2& A, const TVector2& B)
	{
		return (B - A).Length();
	}

	/**
	 * @brief Returns squared distance between two points.
	 *
	 * The function subtracts `A` from `B` and returns the squared length. Use this
	 * when comparing distances because it avoids a square root.
	 */
	static constexpr T DistanceSquared(const TVector2& A, const TVector2& B)
	{
		return (B - A).LengthSq();
	}

	/** @brief Copies coordinates from another vector. */
	constexpr TVector2& operator=(const TVector2& Value) = default;

	/** @brief Returns true when both coordinates are exactly equal. */
	[[nodiscard]] constexpr bool operator==(const TVector2& V) const
	{
		return X == V.X && Y == V.Y;
	}

	/** @brief Returns true when at least one coordinate is different. */
	[[nodiscard]] constexpr bool operator!=(const TVector2& V) const
	{
		return !(*this == V);
	}

	/** @brief Returns a vector where coordinates are added one by one. */
	[[nodiscard]] constexpr TVector2 operator+(const TVector2& V) const
	{
		return TVector2(X + V.X, Y + V.Y);
	}

	/** @brief Returns a vector where coordinates are subtracted one by one. */
	[[nodiscard]] constexpr TVector2 operator-(const TVector2& V) const
	{
		return TVector2(X - V.X, Y - V.Y);
	}

	/** @brief Returns a vector with both coordinates multiplied by `Scalar`. */
	[[nodiscard]] constexpr TVector2 operator*(T Scalar) const
	{
		return TVector2(X * Scalar, Y * Scalar);
	}

	/** @brief Returns a vector with both coordinates divided by `Scalar`. */
	[[nodiscard]] constexpr TVector2 operator/(T Scalar) const
	{
		const T InvScalar = T(1) / Scalar;
		return TVector2(X * InvScalar, Y * InvScalar);
	}

	/** @brief Returns a vector pointing in the opposite direction. */
	[[nodiscard]] constexpr TVector2 operator-() const
	{
		return TVector2(-X, -Y);
	}

	/** @brief Adds another vector into this vector coordinate by coordinate. */
	constexpr TVector2& operator+=(const TVector2& V)
	{
		X += V.X;
		Y += V.Y;
		return *this;
	}

	/** @brief Subtracts another vector from this vector coordinate by coordinate. */
	constexpr TVector2& operator-=(const TVector2& V)
	{
		X -= V.X;
		Y -= V.Y;
		return *this;
	}

	/** @brief Multiplies both coordinates by `Scalar`. */
	constexpr TVector2& operator*=(T Scalar)
	{
		X *= Scalar;
		Y *= Scalar;
		return *this;
	}

	/** @brief Divides both coordinates by `Scalar`. */
	constexpr TVector2& operator/=(T Scalar)
	{
		X /= Scalar;
		Y /= Scalar;
		return *this;
	}

	/** @brief Allows scalar multiplication with scalar on the left side. */
	friend constexpr TVector2 operator*(T Scalar, const TVector2& V)
	{
		return V * Scalar;
	}
};

template <typename T>
inline const TVector2<T> TVector2<T>::Zero(T(0), T(0));

template <typename T>
inline const TVector2<T> TVector2<T>::One(T(1), T(1));

template <typename T>
inline const TVector2<T> TVector2<T>::Half(T(0.5), T(0.5));

template <typename T>
inline const TVector2<T> TVector2<T>::Infinity(
	std::numeric_limits<T>::infinity(),
	std::numeric_limits<T>::infinity());

template <typename T>
inline const TVector2<T> TVector2<T>::NegativeInfinity(
	-std::numeric_limits<T>::infinity(),
	-std::numeric_limits<T>::infinity());

template <typename T>
inline const TVector2<T> TVector2<T>::Unit45Deg(
	T(0.7071067811865475244),
	T(0.7071067811865475244));

template <typename T>
inline const TVector2<T> TVector2<T>::UnitX(T(1), T(0));

template <typename T>
inline const TVector2<T> TVector2<T>::UnitY(T(0), T(1));

template <typename T>
inline const TVector2<T> TVector2<T>::NegUnitX(T(-1), T(0));

template <typename T>
inline const TVector2<T> TVector2<T>::NegUnitY(T(0), T(-1));

using FVector2D = TVector2<float>;
using Vector2D = FVector2D;

} // namespace AE::Math

namespace AE::Physics
{
using AE::Math::FVector2D;
using AE::Math::TVector2;
using AE::Math::Vector2D;
} // namespace AE::Physics

using FVector2D = AE::Math::FVector2D;
using Vector2D = AE::Math::Vector2D;

#endif
