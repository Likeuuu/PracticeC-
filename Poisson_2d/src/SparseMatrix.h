#pragma once
#include <vector>

class SparseMatrix {
public:
    int nrows, ncols;
    std::vector<std::vector<int>> colIndex;
    std::vector<std::vector<double>> values;

    SparseMatrix(int nrows_, int ncols_);
    void addValue(int i, int j, double val);
    std::vector<double> multiply(const std::vector<double>& x) const;
};
