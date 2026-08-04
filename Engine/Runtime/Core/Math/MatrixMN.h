#ifndef MATRIXMN_H
#define MATRIXMN_H

#include "VectorN.h"

namespace AE::Math
{

/**
 * @brief Dynamically sized MxN matrix with a floating point scalar type.
 *
 * `FMatrixMN` stores a rectangular matrix whose row count and column count are
 * chosen at runtime. The matrix is represented as an array of row vectors, where
 * each row is a `TVectorN<T>` with exactly `N` scalar values.
 *
 * This type is intended for small and medium runtime math problems where the
 * dimensions are not known at compile time. Current physics constraints use it
 * for Jacobians, inverse mass matrices, and small linear systems.
 *
 * Matrix storage is row-major at the API level:
 *
 * @code
 * Matrix.Rows[RowIndex][ColumnIndex]
 * @endcode
 *
 * `M` and `N` describe dimensions only. They are kept as `int` because they are
 * counts, while `T` controls the scalar type stored inside the matrix.
 *
 * @tparam T Floating point scalar type, usually `float` or `double`.
 */
template <typename T>
struct FMatrixMN
{
	static_assert(std::is_floating_point_v<T>, "FMatrixMN<T> requires a floating point T");

	/** @brief Number of matrix rows. */
	int M;

	/** @brief Number of matrix columns. */
	int N;

	/** @brief Row-major matrix storage. Each row contains exactly `N` values. */
	std::vector<TVectorN<T>> Rows;

	/**
	 * @brief Creates an empty matrix.
	 *
	 * The resulting matrix has zero rows, zero columns, and no row storage.
	 */
	FMatrixMN()
		: M(0)
		, N(0)
		, Rows()
	{
	}

	/**
	 * @brief Creates a matrix with explicit runtime dimensions.
	 *
	 * This allocates `InM` row vectors and initializes each row with `InN`
	 * zero-initialized scalar values.
	 *
	 * Negative dimensions are programmer errors and trigger assertions. In
	 * non-assert builds the matrix falls back to an empty state.
	 *
	 * @param InM Number of rows.
	 * @param InN Number of columns.
	 */
	FMatrixMN(int InM, int InN)
		: M(InM)
		, N(InN)
		, Rows()
	{
		assert(InM >= 0);
		assert(InN >= 0);

		if (InM < 0 || InN < 0)
		{
			M = 0;
			N = 0;
			return;
		}

		Rows.reserve(M);
		for (int RowIndex = 0; RowIndex < M; ++RowIndex)
		{
			Rows.emplace_back(N);
		}
	}

	/**
	 * @brief Creates a deep copy of another matrix.
	 *
	 * The row vector storage is copied by value, so the new matrix owns
	 * independent rows. Changing `Other` after this constructor does not change
	 * the copied matrix.
	 *
	 * @param Other Matrix to copy.
	 */
	FMatrixMN(const FMatrixMN& Other) = default;

	/**
	 * @brief Moves another matrix into this matrix.
	 *
	 * Row storage is transferred from `Other`. After the move, `Other` is reset
	 * to an empty `0 x 0` matrix so it remains valid for `IsValid()`, assignment,
	 * and destruction.
	 *
	 * @param Other Matrix to move from.
	 */
	FMatrixMN(FMatrixMN&& Other) noexcept
		: M(Other.M)
		, N(Other.N)
		, Rows(std::move(Other.Rows))
	{
		Other.M = 0;
		Other.N = 0;
		Other.Rows.clear();
	}

	/**
	 * @brief Destroys the matrix and releases row storage.
	 *
	 * Storage cleanup is handled by `std::vector` and `TVectorN<T>`.
	 */
	~FMatrixMN() = default;

	/**
	 * @brief Replaces this matrix with a deep copy of another matrix.
	 *
	 * Existing rows are replaced with copied rows from `Other`.
	 *
	 * @param Other Matrix to copy.
	 * @return Reference to this matrix.
	 */
	FMatrixMN& operator=(const FMatrixMN& Other) = default;

	/**
	 * @brief Replaces this matrix by moving another matrix into it.
	 *
	 * Existing rows are released, then row storage is transferred from `Other`.
	 * After the move, `Other` is reset to an empty valid matrix.
	 *
	 * @param Other Matrix to move from.
	 * @return Reference to this matrix.
	 */
	FMatrixMN& operator=(FMatrixMN&& Other) noexcept
	{
		if (this != &Other)
		{
			M = Other.M;
			N = Other.N;
			Rows = std::move(Other.Rows);

			Other.M = 0;
			Other.N = 0;
			Other.Rows.clear();
		}

		return *this;
	}

	/**
	 * @brief Returns true when the matrix has the same number of rows and columns.
	 *
	 * Square matrices are required for operations such as trace, determinant,
	 * inverse, and solving square linear systems.
	 */
	[[nodiscard]] bool IsSquare() const
	{
		return IsValid() && M == N;
	}

	/**
	 * @brief Returns true when dimensions and row storage agree.
	 *
	 * A valid matrix has non-negative dimensions, exactly `M` row vectors, and
	 * every row vector has exactly `N` scalar components.
	 */
	[[nodiscard]] bool IsValid() const
	{
		if (M < 0 || N < 0)
		{
			return false;
		}

		if (Rows.size() != static_cast<std::size_t>(M))
		{
			return false;
		}

		for (const TVectorN<T>& Row : Rows)
		{
			if (Row.N != N)
			{
				return false;
			}

			if (N > 0 && Row.data == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	/** @brief Returns the number of matrix rows. */
	[[nodiscard]] int GetRows() const
	{
		return M;
	}

	/** @brief Returns the number of matrix columns. */
	[[nodiscard]] int GetColumns() const
	{
		return N;
	}

	/**
	 * @brief Returns the sum of diagonal elements.
	 *
	 * The matrix must be square. For a valid empty `0 x 0` matrix the trace is
	 * zero because there are no diagonal elements.
	 *
	 * @return Sum of `Rows[i][i]` for every diagonal index.
	 */
	[[nodiscard]] T Trace() const
	{
		if (!IsSquare())
		{
			assert(IsSquare());
			return T(0);
		}

		T Sum = T(0);
		for (int Index = 0; Index < M; ++Index)
		{
			Sum += Rows[Index][Index];
		}

		return Sum;
	}

	/**
	 * @brief Creates an identity matrix of the requested size.
	 *
	 * The returned matrix has `Size` rows and `Size` columns. Diagonal elements
	 * are set to one and all other elements remain zero.
	 *
	 * Negative sizes are programmer errors and trigger an assertion. In
	 * non-assert builds an empty matrix is returned.
	 *
	 * @param Size Number of rows and columns.
	 * @return Identity matrix with dimensions `Size x Size`.
	 */
	[[nodiscard]] static FMatrixMN Identity(int Size)
	{
		assert(Size >= 0);
		if (Size < 0)
		{
			return FMatrixMN();
		}

		FMatrixMN Result(Size, Size);
		for (int Index = 0; Index < Size; ++Index)
		{
			Result.Rows[Index][Index] = T(1);
		}

		return Result;
	}

	/**
	 * @brief Sets every matrix element to zero.
	 *
	 * The matrix dimensions are preserved. Only values inside existing rows are
	 * changed.
	 */
	void Zero()
	{
		for (TVectorN<T>& Row : Rows)
		{
			Row.Zero();
		}
	}

	/**
	 * @brief Returns a transposed copy of this matrix.
	 *
	 * The result has `N` rows and `M` columns. Element `(i, j)` in this matrix is
	 * copied to element `(j, i)` in the returned matrix.
	 *
	 * @return Matrix whose rows and columns are swapped.
	 */
	[[nodiscard]] FMatrixMN Transpose() const
	{
		FMatrixMN Result(N, M);

		for (int RowIndex = 0; RowIndex < M; ++RowIndex)
		{
			for (int ColumnIndex = 0; ColumnIndex < N; ++ColumnIndex)
			{
				Result.Rows[ColumnIndex][RowIndex] = Rows[RowIndex][ColumnIndex];
			}
		}

		return Result;
	}

	/**
	 * @brief Multiplies this matrix by a column vector.
	 *
	 * Computes `Result = A * V`, where `A` is this matrix and `V` is
	 * `InVector`. The input vector must have exactly `N` components because each
	 * row dot product uses all matrix columns.
	 *
	 * If dimensions do not match, an assertion is triggered and an empty vector
	 * is returned in non-assert builds.
	 *
	 * @param InVector Column vector on the right side of the multiplication.
	 * @return Vector with `M` components, one value per matrix row.
	 */
	[[nodiscard]] TVectorN<T> operator*(const TVectorN<T>& InVector) const
	{
		if (InVector.N != N)
		{
			assert(InVector.N == N);
			return TVectorN<T>();
		}

		TVectorN<T> Result(M);
		for (int RowIndex = 0; RowIndex < M; ++RowIndex)
		{
			Result[RowIndex] = Rows[RowIndex].DotProduct(InVector);
		}

		return Result;
	}

	/**
	 * @brief Multiplies this matrix by another matrix.
	 *
	 * Computes the standard matrix product `Result = A * B`, where `A` is this
	 * matrix and `B` is `InMatrix`. The left matrix column count must match the
	 * right matrix row count: `N == InMatrix.M`.
	 *
	 * The returned matrix has `M` rows and `InMatrix.N` columns. Internally the
	 * right matrix is transposed so each output cell can be computed as one row
	 * dot product.
	 *
	 * If dimensions do not match, an assertion is triggered and an empty matrix
	 * is returned in non-assert builds.
	 *
	 * @param InMatrix Matrix on the right side of the multiplication.
	 * @return Product matrix with dimensions `M x InMatrix.N`.
	 */
	[[nodiscard]] FMatrixMN operator*(const FMatrixMN& InMatrix) const
	{
		if (N != InMatrix.M)
		{
			assert(N == InMatrix.M);
			return FMatrixMN();
		}

		const FMatrixMN Transposed = InMatrix.Transpose();
		FMatrixMN Result(M, InMatrix.N);

		for (int RowIndex = 0; RowIndex < M; ++RowIndex)
		{
			for (int ColumnIndex = 0; ColumnIndex < InMatrix.N; ++ColumnIndex)
			{
				Result.Rows[RowIndex][ColumnIndex] = Rows[RowIndex].DotProduct(Transposed.Rows[ColumnIndex]);
			}
		}

		return Result;
	}

	/**
	 * @brief Returns the determinant of this matrix.
	 *
	 * The matrix must be square. The determinant is computed with Gaussian
	 * elimination and partial pivoting. Row swaps flip the determinant sign, and
	 * singular matrices return zero.
	 *
	 * A valid empty `0 x 0` matrix returns one, matching the determinant of an
	 * empty identity matrix.
	 *
	 * @return Determinant value.
	 */
	[[nodiscard]] T Determinant() const
	{
		if (!IsSquare())
		{
			assert(IsSquare());
			return T(0);
		}

		if (M == 0)
		{
			return T(1);
		}

		FMatrixMN Working = *this;
		T Result = T(1);
		T Sign = T(1);
		const T PivotTolerance = std::numeric_limits<T>::epsilon() * T(100);

		for (int ColumnIndex = 0; ColumnIndex < N; ++ColumnIndex)
		{
			int PivotRow = ColumnIndex;
			T PivotAbs = std::fabs(Working.Rows[PivotRow][ColumnIndex]);

			for (int RowIndex = ColumnIndex + 1; RowIndex < M; ++RowIndex)
			{
				const T CandidateAbs = std::fabs(Working.Rows[RowIndex][ColumnIndex]);
				if (CandidateAbs > PivotAbs)
				{
					PivotAbs = CandidateAbs;
					PivotRow = RowIndex;
				}
			}

			if (PivotAbs <= PivotTolerance)
			{
				return T(0);
			}

			if (PivotRow != ColumnIndex)
			{
				std::swap(Working.Rows[PivotRow], Working.Rows[ColumnIndex]);
				Sign = -Sign;
			}

			const T Pivot = Working.Rows[ColumnIndex][ColumnIndex];
			Result *= Pivot;

			for (int RowIndex = ColumnIndex + 1; RowIndex < M; ++RowIndex)
			{
				const T Factor = Working.Rows[RowIndex][ColumnIndex] / Pivot;
				Working.Rows[RowIndex][ColumnIndex] = T(0);

				for (int InnerColumnIndex = ColumnIndex + 1; InnerColumnIndex < N; ++InnerColumnIndex)
				{
					Working.Rows[RowIndex][InnerColumnIndex] -= Factor * Working.Rows[ColumnIndex][InnerColumnIndex];
				}
			}
		}

		return Result * Sign;
	}

	/**
	 * @brief Returns the inverse of this matrix.
	 *
	 * The matrix must be square and non-singular. The inverse is computed with
	 * Gauss-Jordan elimination and partial pivoting. If the matrix is singular,
	 * an empty matrix is returned.
	 *
	 * @return Inverse matrix with the same dimensions, or an empty matrix when
	 * the inverse cannot be computed.
	 */
	[[nodiscard]] FMatrixMN Inverse() const
	{
		if (!IsSquare())
		{
			assert(IsSquare());
			return FMatrixMN();
		}

		if (M == 0)
		{
			return FMatrixMN();
		}

		FMatrixMN Working = *this;
		FMatrixMN Result = Identity(M);
		const T PivotTolerance = std::numeric_limits<T>::epsilon() * T(100);

		for (int ColumnIndex = 0; ColumnIndex < N; ++ColumnIndex)
		{
			int PivotRow = ColumnIndex;
			T PivotAbs = std::fabs(Working.Rows[PivotRow][ColumnIndex]);

			for (int RowIndex = ColumnIndex + 1; RowIndex < M; ++RowIndex)
			{
				const T CandidateAbs = std::fabs(Working.Rows[RowIndex][ColumnIndex]);
				if (CandidateAbs > PivotAbs)
				{
					PivotAbs = CandidateAbs;
					PivotRow = RowIndex;
				}
			}

			if (PivotAbs <= PivotTolerance)
			{
				return FMatrixMN();
			}

			if (PivotRow != ColumnIndex)
			{
				std::swap(Working.Rows[PivotRow], Working.Rows[ColumnIndex]);
				std::swap(Result.Rows[PivotRow], Result.Rows[ColumnIndex]);
			}

			const T Pivot = Working.Rows[ColumnIndex][ColumnIndex];
			for (int InnerColumnIndex = 0; InnerColumnIndex < N; ++InnerColumnIndex)
			{
				Working.Rows[ColumnIndex][InnerColumnIndex] /= Pivot;
				Result.Rows[ColumnIndex][InnerColumnIndex] /= Pivot;
			}

			for (int RowIndex = 0; RowIndex < M; ++RowIndex)
			{
				if (RowIndex == ColumnIndex)
				{
					continue;
				}

				const T Factor = Working.Rows[RowIndex][ColumnIndex];
				if (std::fabs(Factor) <= PivotTolerance)
				{
					Working.Rows[RowIndex][ColumnIndex] = T(0);
					continue;
				}

				for (int InnerColumnIndex = 0; InnerColumnIndex < N; ++InnerColumnIndex)
				{
					Working.Rows[RowIndex][InnerColumnIndex] -= Factor * Working.Rows[ColumnIndex][InnerColumnIndex];
					Result.Rows[RowIndex][InnerColumnIndex] -= Factor * Result.Rows[ColumnIndex][InnerColumnIndex];
				}
			}
		}

		return Result;
	}

	/**
	 * @brief Solves a square linear system with the Gauss-Seidel method.
	 *
	 * Solves `A * X = B` iteratively, starting from a zero vector. This method is
	 * useful for small systems that appear in constraint solving, especially when
	 * the coefficient matrix is diagonally dominant.
	 *
	 * `A` must be a square matrix with the same dimension as `B`. Diagonal values
	 * must be non-zero because each row update divides by the diagonal entry.
	 *
	 * The iteration count is currently equal to the system size. This keeps the
	 * solver cheap for runtime physics but means the returned result is an
	 * approximation, not a guaranteed exact solution.
	 *
	 * If dimensions do not match, an assertion is triggered and an empty vector is
	 * returned in non-assert builds. If a diagonal entry is zero, that row update
	 * is skipped in non-assert builds.
	 *
	 * @param A Square coefficient matrix.
	 * @param B Right-hand side vector.
	 * @return Approximate solution vector `X`.
	 */
	[[nodiscard]] static TVectorN<T> SolveGaussSeidel(const FMatrixMN& A, const TVectorN<T>& B)
	{
		const int Size = B.N;
		if (A.M != Size || A.N != Size)
		{
			assert(A.M == Size);
			assert(A.N == Size);
			return TVectorN<T>();
		}

		TVectorN<T> X(Size);
		X.Zero();

		for (int Iteration = 0; Iteration < Size; ++Iteration)
		{
			for (int RowIndex = 0; RowIndex < Size; ++RowIndex)
			{
				const T Diagonal = A.Rows[RowIndex][RowIndex];
				if (Diagonal == T(0))
				{
					assert(Diagonal != T(0));
					continue;
				}

				const T Delta = (B[RowIndex] - A.Rows[RowIndex].DotProduct(X)) / Diagonal;
				if (std::isfinite(Delta))
				{
					X[RowIndex] += Delta;
				}
			}
		}

		return X;
	}
};

} // namespace AE::Math

using AE::Math::FMatrixMN;

#endif
