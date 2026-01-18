#include "Solver.h"
#include <cmath>
#include <iostream>

#include <fstream>
#include <iomanip>


JacobiSolver::JacobiSolver(int maxIter_, double tol_, int nx_, int ny_) : maxIter(maxIter_), tol(tol_), nx(nx_), ny(ny_) {}


void JacobiSolver::writeCSV(const std::vector<double>& x, int iter){
    std::ofstream file("iter_" + std::to_string(iter) + ".csv");
    for(int j=0;j<ny;j++){
        for(int i=0;i<nx;i++){
            int idx = j*nx + i;
            file << std::setprecision(6) << x[idx];
            if(i != nx-1) file << ",";
        }
        file << "\n";
    }
    file.close();
}


void JacobiSolver::solve(const SparseMatrix& A, const std::vector<double>& b, std::vector<double>& x){
    int n = b.size();
    std::vector<double> x_new(n,0.0);

    for(int iter=0; iter<maxIter; iter++){
        for(int i=0;i<n;i++){
            double sum = 0.0;
            double diag = 0.0;
            for(size_t k=0;k<A.colIndex[i].size();k++){
                int j = A.colIndex[i][k];
                if(i==j) diag = A.values[i][k];
                else sum += A.values[i][k]*x[j];
            }
            x_new[i] = (b[i] - sum)/diag;
        }

        // 计算误差
        double err = 0.0;
        for(int i=0;i<n;i++) err += std::abs(x_new[i]-x[i]);
        if(err < tol) {
            std::cout << "Converged at iteration " << iter << "\n";
            break;
        }

        x = x_new;

        writeCSV(x, iter);
    }
}
