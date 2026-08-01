#ifndef VECTORN_H
#define VECTORN_H

#include <algorithm>
#include <type_traits>

namespace AE::Math
{

template <typename T>
struct TVectorN
{
	static_assert(std::is_floating_point_v<T>, "TVectorN<T> requires a floating point T");

	int N;
	T* data;

	TVectorN()
		: N(0)
		, data(nullptr)
	{
	}

	explicit TVectorN(int InN)
		: N(InN)
		, data(InN > 0 ? new T[InN]{} : nullptr)
	{
	}

	TVectorN(const TVectorN& V)
		: TVectorN(V.N)
	{
		std::copy(V.data, V.data + N, data);
	}

	TVectorN(TVectorN&& V) noexcept
		: N(V.N)
		, data(V.data)
	{
		V.N = 0;
		V.data = nullptr;
	}

	~TVectorN()
	{
		delete[] data;
	}

	void Zero()
	{
		std::fill(data, data + N, T(0));
	}

	[[nodiscard]] T Dot(const TVectorN& V) const
	{
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
	void Swap(TVectorN& Other) noexcept
	{
		std::swap(N, Other.N);
		std::swap(data, Other.data);
	}
};

using FVectorN = TVectorN<float>;
using VectorN = FVectorN;

} // namespace AE::Math

namespace AE::Physics
{
using AE::Math::FVectorN;
using AE::Math::TVectorN;
using AE::Math::VectorN;
} // namespace AE::Physics

using AE::Math::FVectorN;
using AE::Math::TVectorN;
using AE::Math::VectorN;

#endif
