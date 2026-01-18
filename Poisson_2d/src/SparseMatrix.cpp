#include "SparseMatrix.h"

SparseMatrix::SparseMatrix(int nrows_, int ncols_) : nrows(nrows_), ncols(ncols_) {
    colIndex.resize(nrows);
    values.resize(nrows);
}

void SparseMatrix::addValue(int i, int j, double val) {
    colIndex[i].push_back(j);
    values[i].push_back(val);
}

std::vector<double> SparseMatrix::multiply(const std::vector<double>& x) const {
    std::vector<double> result(nrows, 0.0);
    for(int i=0;i<nrows;i++){
        for(size_t k=0;k<colIndex[i].size();k++){
            int j = colIndex[i][k];
            result[i] += values[i][k]*x[j];
        }
    }
    return result;
}
