#include "Grid.h"
#include "SparseMatrix.h"
#include "Solver.h"
#include <iostream>

int main(){
    int nx = 10, ny = 10;
    Grid grid(nx, ny);
    grid.initialize();

    int N = nx*ny;
    SparseMatrix A(N, N);
    std::vector<double> b(N, 1.0);
    std::vector<double> x(N, 0.0);

    // 简单 5 点离散 Laplace
    for(int j=0;j<ny;j++){
        for(int i=0;i<nx;i++){
            int idx = j*nx + i;
            if(grid.nodes[idx].isBoundary){
                A.addValue(idx, idx, 1.0);
                b[idx] = grid.nodes[idx].value;
            } else {
                A.addValue(idx, idx, -4.0);
                A.addValue(idx, idx-1, 1.0);
                A.addValue(idx, idx+1, 1.0);
                A.addValue(idx, idx-nx, 1.0);
                A.addValue(idx, idx+nx, 1.0);
            }
        }
    }

    JacobiSolver solver(10000, 1e-6, nx, ny);
    solver.solve(A, b, x);

    for(int j=0;j<ny;j++){
        for(int i=0;i<nx;i++){
            int idx = j*nx + i;
            std::cout << x[idx] << " ";
        }
        std::cout << "\n";
    }
}
