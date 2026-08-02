#ifndef VECTORN_H
#define VECTORN_H

#include <algorithm>
#include <cassert>
#include <type_traits>
#include <utility>


namespace AE::Math
{
/**
 * @brief Dynamically sized mathematical vector.
 *
 * Represents an N-dimensional vector whose elements are stored in a dynamically
 * allocated array. The vector provides basic arithmetic operations, scalar
 * multiplication, dot product computation, and copy/move semantics.
 *
 * @tparam T Floating-point element type.
 */
template <typename T>
struct TVectorN
{
	static_assert(std::is_floating_point_v<T>, "TVectorN<T> requires a floating point T");

	/** Number of vector components. */
	int N;

	/** Dynamically allocated array containing vector components. */
	T* data;

	/**
	 * @brief Constructs an empty vector.
	 *
	 * Creates a vector with zero dimensions and no allocated storage.
	 */
	TVectorN()
		: N(0)
		, data(nullptr)
	{
	}

	/**
	 * @brief Constructs a vector with the specified dimension.
	 *
	 * Allocates storage for the requested number of components and initializes
	 * them to zero.
	 *
	 * @param InN Number of vector components.
	 */
	explicit TVectorN(int InN)
		: N(InN)
		, data(InN > 0 ? new T[InN]{} : nullptr)
	{
	}

	/**
	 * @brief Copy constructor.
	 *
	 * Creates a deep copy of another vector.
	 *
	 * @param V Vector to copy.
	 */
	TVectorN(const TVectorN& V)
		: TVectorN(V.N)
	{
		if (N > 0)
		{
			std::copy(V.data, V.data + N, data);
		}
	}

	/**
	 * @brief Move constructor.
	 *
	 * Transfers ownership of the underlying storage from another vector.
	 *
	 * @param V Vector to move.
	 */
	TVectorN(TVectorN&& V) noexcept
		: N(V.N)
		, data(V.data)
	{
		V.N = 0;
		V.data = nullptr;
	}

	/**
	 * @brief Destroys the vector.
	 *
	 * Releases the dynamically allocated component storage.
	 */
	~TVectorN()
	{
		delete[] data;
	}

	/**
	 * @brief Sets all vector components to zero.
	 */
	void Zero()
	{
		std::fill(data, data + N, T(0));
	}

	/**
	 * @brief Computes the dot product with another vector.
	 *
	 * @param V Vector to multiply with.
	 *
	 * @return Dot product of the two vectors.
	 */
	[[nodiscard]] T DotProduct(const TVectorN& V) const
	{
		assert(N == V.N);

		T Sum = T(0);
		for (int Index = 0; Index < N; ++Index)
		{
			Sum += data[Index] * V.data[Index];
		}
		return Sum;
	}

	TVectorN& operator=(const TVectorN& V)
	{
		if (this != &V)
		{
			TVectorN Copy(V);
			Swap(Copy);
		}
		return *this;
	}

	TVectorN& operator=(TVectorN&& V) noexcept
	{
		if (this != &V)
		{
			delete[] data;
			N = V.N;
			data = V.data;
			V.N = 0;
			V.data = nullptr;
		}
		return *this;
	}

	[[nodiscard]] TVectorN operator+(const TVectorN& V) const
	{
		TVectorN Result = *this;
		Result += V;
		return Result;
	}

	[[nodiscard]] TVectorN operator-(const TVectorN& V) const
	{
		TVectorN Result = *this;
		Result -= V;
		return Result;
	}

	[[nodiscard]] TVectorN operator*(T Scalar) const
	{
		TVectorN Result = *this;
		Result *= Scalar;
		return Result;
	}

	TVectorN& operator+=(const TVectorN& V)
	{
		for (int Index = 0; Index < N; ++Index)
		{
			data[Index] += V.data[Index];
		}
		return *this;
	}

	TVectorN& operator-=(const TVectorN& V)
	{
		for (int Index = 0; Index < N; ++Index)
		{
			data[Index] -= V.data[Index];
		}
		return *this;
	}

	TVectorN& operator*=(T Scalar)
	{
		for (int Index = 0; Index < N; ++Index)
		{
			data[Index] *= Scalar;
		}
		return *this;
	}

	[[nodiscard]] T operator[](int Index) const
	{
		return data[Index];
	}

	[[nodiscard]] T& operator[](int Index)
	{
		return data[Index];
	}

private:
	/**
	 * @brief Exchanges the contents of two vectors.
	 *
	 * Swaps both the vector dimension and the underlying storage pointer.
	 *
	 * @param Other Vector to swap with.
	 */
	void Swap(TVectorN& Other) noexcept
	{
		std::swap(N, Other.N);
		std::swap(data, Other.data);
	}
};
/** Floating-point N-dimensional vector type. */
using FVectorN = TVectorN<float>;
} // namespace AE::Math

using AE::Math::FVectorN;
using AE::Math::TVectorN;


#endif
