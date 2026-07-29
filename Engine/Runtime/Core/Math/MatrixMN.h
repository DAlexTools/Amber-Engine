#ifndef MATRIXMN_H
#define MATRIXMN_H

#include "VectorN.h"

namespace AE::Math
{

/**
 * 
 */
struct MatrixMN
{
    int             M;                                  // rows
    int             N;                                  // cols
    FVectorN*       rows;                               // the rows of the matrix with N columns inside

    MatrixMN();
    MatrixMN(int M, int N);
    MatrixMN(const MatrixMN& m);
    ~MatrixMN();

    void            Zero();
    MatrixMN        Transpose() const;

    const MatrixMN& operator=(const MatrixMN& m);       // m1 = m2
    FVectorN        operator*(const FVectorN& v) const; // m1 * v
    MatrixMN        operator*(const MatrixMN& m) const; // m1 * m2

    static FVectorN SolveGaussSeidel(const MatrixMN& A, const FVectorN& B);
};

}

using AE::Math::MatrixMN;

#endif 
