#include "Core/BuildConfig.h"

#if C_UNIT_TEST

#include <gtest/gtest.h>

#include <cmath>

#include "Core/Math/Vector2D.h"

namespace
{
constexpr float Pi = 3.14159265358979323846f;
}

TEST(FVector2DTests, AddsSubtractsAndScales)
{
	const AE::Math::FVector2D a(3.0f, 4.0f);
	const AE::Math::FVector2D b(-1.0f, 2.0f);

	const AE::Math::FVector2D sum = a + b;
	EXPECT_FLOAT_EQ(sum.X, 2.0f);
	EXPECT_FLOAT_EQ(sum.Y, 6.0f);

	const AE::Math::FVector2D diff = a - b;
	EXPECT_FLOAT_EQ(diff.X, 4.0f);
	EXPECT_FLOAT_EQ(diff.Y, 2.0f);

	const AE::Math::FVector2D scaled = a * 2.0f;
	EXPECT_FLOAT_EQ(scaled.X, 6.0f);
	EXPECT_FLOAT_EQ(scaled.Y, 8.0f);
}

TEST(FVector2DTests, ComputesLengthDotAndCrossProducts)
{
	const AE::Math::FVector2D vector(3.0f, 4.0f);
	const AE::Math::FVector2D other(2.0f, -1.0f);

	EXPECT_FLOAT_EQ(vector.Length(), 5.0f);
	EXPECT_FLOAT_EQ(vector.LengthSq(), 25.0f);
	EXPECT_FLOAT_EQ(vector.DotProduct(other), 2.0f);
	EXPECT_FLOAT_EQ(vector.CrossProduct(other), -11.0f);
}

TEST(FVector2DTests, NormalizesWithoutMutatingOriginalWhenRequestingUnitVector)
{
	const AE::Math::FVector2D vector(3.0f, 4.0f);

	const AE::Math::FVector2D unit = vector.UnitVector();

	EXPECT_FLOAT_EQ(vector.X, 3.0f);
	EXPECT_FLOAT_EQ(vector.Y, 4.0f);
	EXPECT_NEAR(unit.X, 0.6f, 0.0001f);
	EXPECT_NEAR(unit.Y, 0.8f, 0.0001f);
	EXPECT_NEAR(unit.Length(), 1.0f, 0.0001f);
}

TEST(FVector2DTests, RotatesUsingRadians)
{
	const AE::Math::FVector2D right(1.0f, 0.0f);

	const AE::Math::FVector2D up = right.Rotate(Pi * 0.5f);

	EXPECT_NEAR(up.X, 0.0f, 0.0001f);
	EXPECT_NEAR(up.Y, 1.0f, 0.0001f);
}

#endif
