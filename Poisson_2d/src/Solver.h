#pragma once
#include "SparseMatrix.h"
#include <vector>

class JacobiSolver {
public:
    int maxIter;
    double tol;
    int nx, ny;  // 新增


    JacobiSolver(int maxIter_, double tol_, int nx_, int ny_);
    void solve(const SparseMatrix& A, const std::vector<double>& b, std::vector<double>& x);
    void writeCSV(const std::vector<double>& x, int iter);
};
