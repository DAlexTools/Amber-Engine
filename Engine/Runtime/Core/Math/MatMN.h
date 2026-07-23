#ifndef MATMN_H
#define MATMN_H

#include "VecN.h"

namespace AE::Physics
{

/**
 * 
 */
struct MatMN
{
    int             M;                                  // rows
    int             N;                                  // cols
    VectorN*        rows;                               // the rows of the matrix with N columns inside

    MatMN();
    MatMN(int M, int N);
    MatMN(const MatMN& m);
    ~MatMN();

    void            Zero();
    MatMN           Transpose() const;

    const MatMN&    operator=(const MatMN& m);          // m1 = m2
    VectorN         operator*(const VectorN& v) const;  // m1 * v
    MatMN           operator*(const MatMN& m) const;    // m1 * m2

    static VectorN  SolveGaussSeidel(const MatMN& A, const VectorN& B);
};

}

using AE::Physics::MatMN;

#endif 
