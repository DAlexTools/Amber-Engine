#ifndef VectorN_H
#define VectorN_H


namespace AE::Physics
{

struct VectorN
{
    int N;
    float* data;

    VectorN();
    VectorN(int N);
    VectorN(const VectorN& v);
    ~VectorN();

    void Zero();                                    // v1.Zero()
    float Dot(const VectorN& v) const;              // v1.Dot(v2)

    VectorN& operator=(const VectorN& v);           // v1 = v2
    VectorN operator+(const VectorN& v) const;      // v1 + v2
    VectorN operator-(const VectorN& v) const;      // v1 - v2
    VectorN operator*(const float n) const;         // v1 * n
    const VectorN& operator+=(const VectorN& v);    // v1 += v2
    const VectorN& operator-=(const VectorN& v);    // v1 -= v2
    const VectorN& operator*=(const float n);       // v1 *= n
    float operator[](const int index) const;        // v1[index]
    float& operator[](const int index);             // v1[index]
};

}

using AE::Physics::VectorN;

#endif
