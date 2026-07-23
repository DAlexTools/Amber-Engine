

#include "MatMN.h"

namespace AE::Physics
{

MatMN::MatMN()
: M(0), N(0), rows(nullptr)
 {
    // 
 }

/**
 * 
 */
MatMN::MatMN(int M, int N)
: M(M), N(N)
{
    rows = new VectorN[M];
    
    for (int i = 0; i < M; i++)
        rows[i] = VectorN(N);
}

/**
 * 
 */
MatMN::MatMN(const MatMN& m)
{
    *this = m;
}

/**
 * 
 */
MatMN::~MatMN()
{
    delete[] rows;
}

/**
 * 
 */
void MatMN::Zero()
{
    for (int i = 0; i < M; i++)
        rows[i].Zero();
}

/**
 * 
 */
MatMN MatMN::Transpose() const
{
    MatMN result(N, M);

    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            result.rows[j][i] = rows[i][j];

    return result;
}

/**
 * 
 */
const MatMN& MatMN::operator=(const MatMN& m)
{
    M = m.M;
    N = m.N;
    rows = new VectorN[M];

    for (int i = 0; i < M; i++)
        rows[i] = m.rows[i];

    return *this;
}

/**
 * 
 */
VectorN MatMN::operator*(const VectorN& v) const
{
    if (v.N != N) return v;
    VectorN result(M);
    for (int i = 0; i < M; i++)
        result[i] = v.Dot(rows[i]);
    return result;
}

/**
 * 
 */
MatMN MatMN::operator*(const MatMN& m) const
{
    if (m.M != N && m.N != M) return m;

    MatMN tranposed = m.Transpose();
    MatMN result(M, m.N);

    for (int i = 0; i < M; i++)
        for (int j = 0; j < m.N; j++)
            result.rows[i][j] = rows[i].Dot(tranposed.rows[j]);

    return result;
}

/**
 * 
 */
VectorN MatMN::SolveGaussSeidel(const MatMN& A, const VectorN& b)
{
    const int N = b.N;
    VectorN X(N);
    X.Zero();

    for (int iterations = 0; iterations < N; iterations++)
    {
        for (int i = 0; i < N; i++)
        {
            float dx = (b[i] / A.rows[i][i]) - (A.rows[i].Dot(X) / A.rows[i][i]);

            if (dx == dx)
            {
                X[i] += dx;
            }
        }
    }
    return X;
}

}
