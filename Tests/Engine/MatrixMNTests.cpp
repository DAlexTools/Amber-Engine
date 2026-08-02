#include "Core/Math/MatrixMN.h"

#include <gtest/gtest.h>
#include <utility>

TEST(FMatrixMNTests, CopiesRowsDeeply)
{
	AE::Math::FMatrixMN<float> Matrix(2, 2);
	Matrix.Rows[0][0] = 1.0f;
	Matrix.Rows[0][1] = 2.0f;
	Matrix.Rows[1][0] = 3.0f;
	Matrix.Rows[1][1] = 4.0f;

	const AE::Math::FMatrixMN<float> Copy = Matrix;
	Matrix.Rows[0][0] = 10.0f;

	EXPECT_FLOAT_EQ(Copy.Rows[0][0], 1.0f);
	EXPECT_FLOAT_EQ(Copy.Rows[1][1], 4.0f);
}

TEST(FMatrixMNTests, MovesRowsAndLeavesSourceValid)
{
	AE::Math::FMatrixMN<float> Source(2, 2);
	Source.Rows[0][0] = 1.0f;
	Source.Rows[1][1] = 4.0f;

	AE::Math::FMatrixMN<float> Moved(std::move(Source));

	EXPECT_TRUE(Source.IsValid());
	EXPECT_EQ(Source.M, 0);
	EXPECT_EQ(Source.N, 0);
	EXPECT_TRUE(Source.Rows.empty());
	EXPECT_FLOAT_EQ(Moved.Rows[0][0], 1.0f);
	EXPECT_FLOAT_EQ(Moved.Rows[1][1], 4.0f);

	AE::Math::FMatrixMN<float> Assigned;
	Assigned = std::move(Moved);

	EXPECT_TRUE(Moved.IsValid());
	EXPECT_EQ(Moved.M, 0);
	EXPECT_EQ(Moved.N, 0);
	EXPECT_TRUE(Moved.Rows.empty());
	EXPECT_FLOAT_EQ(Assigned.Rows[0][0], 1.0f);
	EXPECT_FLOAT_EQ(Assigned.Rows[1][1], 4.0f);
}

TEST(FMatrixMNTests, ReportsShapeAndValidity)
{
	const AE::Math::FMatrixMN<float> Matrix(2, 3);

	EXPECT_TRUE(Matrix.IsValid());
	EXPECT_FALSE(Matrix.IsSquare());
	EXPECT_EQ(Matrix.GetRows(), 2);
	EXPECT_EQ(Matrix.GetColumns(), 3);
}

TEST(FMatrixMNTests, TransposesRowsAndColumns)
{
	AE::Math::FMatrixMN<float> Matrix(2, 3);
	Matrix.Rows[0][0] = 1.0f;
	Matrix.Rows[0][1] = 2.0f;
	Matrix.Rows[0][2] = 3.0f;
	Matrix.Rows[1][0] = 4.0f;
	Matrix.Rows[1][1] = 5.0f;
	Matrix.Rows[1][2] = 6.0f;

	const AE::Math::FMatrixMN<float> Transposed = Matrix.Transpose();

	EXPECT_EQ(Transposed.M, 3);
	EXPECT_EQ(Transposed.N, 2);
	EXPECT_FLOAT_EQ(Transposed.Rows[0][0], 1.0f);
	EXPECT_FLOAT_EQ(Transposed.Rows[0][1], 4.0f);
	EXPECT_FLOAT_EQ(Transposed.Rows[2][0], 3.0f);
	EXPECT_FLOAT_EQ(Transposed.Rows[2][1], 6.0f);
}

TEST(FMatrixMNTests, MultipliesMatrices)
{
	AE::Math::FMatrixMN<float> A(2, 3);
	A.Rows[0][0] = 1.0f;
	A.Rows[0][1] = 2.0f;
	A.Rows[0][2] = 3.0f;
	A.Rows[1][0] = 4.0f;
	A.Rows[1][1] = 5.0f;
	A.Rows[1][2] = 6.0f;

	AE::Math::FMatrixMN<float> B(3, 2);
	B.Rows[0][0] = 7.0f;
	B.Rows[0][1] = 8.0f;
	B.Rows[1][0] = 9.0f;
	B.Rows[1][1] = 10.0f;
	B.Rows[2][0] = 11.0f;
	B.Rows[2][1] = 12.0f;

	const AE::Math::FMatrixMN<float> Result = A * B;

	EXPECT_EQ(Result.M, 2);
	EXPECT_EQ(Result.N, 2);
	EXPECT_FLOAT_EQ(Result.Rows[0][0], 58.0f);
	EXPECT_FLOAT_EQ(Result.Rows[0][1], 64.0f);
	EXPECT_FLOAT_EQ(Result.Rows[1][0], 139.0f);
	EXPECT_FLOAT_EQ(Result.Rows[1][1], 154.0f);
}

TEST(FMatrixMNTests, BuildsIdentityAndComputesTrace)
{
	const AE::Math::FMatrixMN<float> Identity = AE::Math::FMatrixMN<float>::Identity(3);

	EXPECT_TRUE(Identity.IsSquare());
	EXPECT_FLOAT_EQ(Identity.Rows[0][0], 1.0f);
	EXPECT_FLOAT_EQ(Identity.Rows[1][1], 1.0f);
	EXPECT_FLOAT_EQ(Identity.Rows[2][2], 1.0f);
	EXPECT_FLOAT_EQ(Identity.Rows[0][1], 0.0f);
	EXPECT_FLOAT_EQ(Identity.Trace(), 3.0f);
}

TEST(FMatrixMNTests, ComputesDeterminant)
{
	AE::Math::FMatrixMN<float> Matrix(3, 3);
	Matrix.Rows[0][0] = 6.0f;
	Matrix.Rows[0][1] = 1.0f;
	Matrix.Rows[0][2] = 1.0f;
	Matrix.Rows[1][0] = 4.0f;
	Matrix.Rows[1][1] = -2.0f;
	Matrix.Rows[1][2] = 5.0f;
	Matrix.Rows[2][0] = 2.0f;
	Matrix.Rows[2][1] = 8.0f;
	Matrix.Rows[2][2] = 7.0f;

	EXPECT_NEAR(Matrix.Determinant(), -306.0f, 0.0001f);
}

TEST(FMatrixMNTests, ComputesInverse)
{
	AE::Math::FMatrixMN<float> Matrix(2, 2);
	Matrix.Rows[0][0] = 4.0f;
	Matrix.Rows[0][1] = 7.0f;
	Matrix.Rows[1][0] = 2.0f;
	Matrix.Rows[1][1] = 6.0f;

	const AE::Math::FMatrixMN<float> Inverse = Matrix.Inverse();
	const AE::Math::FMatrixMN<float> Product = Matrix * Inverse;

	EXPECT_EQ(Inverse.M, 2);
	EXPECT_EQ(Inverse.N, 2);
	EXPECT_NEAR(Inverse.Rows[0][0], 0.6f, 0.0001f);
	EXPECT_NEAR(Inverse.Rows[0][1], -0.7f, 0.0001f);
	EXPECT_NEAR(Inverse.Rows[1][0], -0.2f, 0.0001f);
	EXPECT_NEAR(Inverse.Rows[1][1], 0.4f, 0.0001f);
	EXPECT_NEAR(Product.Rows[0][0], 1.0f, 0.0001f);
	EXPECT_NEAR(Product.Rows[0][1], 0.0f, 0.0001f);
	EXPECT_NEAR(Product.Rows[1][0], 0.0f, 0.0001f);
	EXPECT_NEAR(Product.Rows[1][1], 1.0f, 0.0001f);
}

TEST(FMatrixMNTests, ReturnsEmptyInverseForSingularMatrix)
{
	AE::Math::FMatrixMN<float> Matrix(2, 2);
	Matrix.Rows[0][0] = 1.0f;
	Matrix.Rows[0][1] = 2.0f;
	Matrix.Rows[1][0] = 2.0f;
	Matrix.Rows[1][1] = 4.0f;

	const AE::Math::FMatrixMN<float> Inverse = Matrix.Inverse();

	EXPECT_EQ(Inverse.M, 0);
	EXPECT_EQ(Inverse.N, 0);
	EXPECT_TRUE(Inverse.Rows.empty());
	EXPECT_FLOAT_EQ(Matrix.Determinant(), 0.0f);
}

TEST(FMatrixMNTests, MultipliesDoubleMatrixByDoubleVector)
{
	AE::Math::FMatrixMN<double> Matrix(1, 2);
	Matrix.Rows[0][0] = 2.5;
	Matrix.Rows[0][1] = 4.0;

	AE::Math::TVectorN<double> Vector(2);
	Vector[0] = 2.0;
	Vector[1] = 3.0;

	const AE::Math::TVectorN<double> Result = Matrix * Vector;

	EXPECT_EQ(Result.N, 1);
	EXPECT_DOUBLE_EQ(Result[0], 17.0);
}

TEST(FMatrixMNTests, SolvesSmallSystemWithGaussSeidel)
{
	AE::Math::FMatrixMN<float> A(2, 2);
	A.Rows[0][0] = 4.0f;
	A.Rows[0][1] = 1.0f;
	A.Rows[1][0] = 1.0f;
	A.Rows[1][1] = 3.0f;

	AE::Math::FVectorN B(2);
	B[0] = 9.0f;
	B[1] = 7.0f;

	const AE::Math::FVectorN X = AE::Math::FMatrixMN<float>::SolveGaussSeidel(A, B);

	EXPECT_NEAR(X[0], 1.818f, 0.05f);
	EXPECT_NEAR(X[1], 1.727f, 0.05f);
}
