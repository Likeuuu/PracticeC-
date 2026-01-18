#include "Grid.h"

Grid::Grid(int nx_, int ny_) : nx(nx_), ny(ny_) {
    dx = 1.0 / (nx-1);
    dy = 1.0 / (ny-1);
    nodes.resize(nx*ny);
}

void Grid::initialize() {
    for(int j=0;j<ny;j++){
        for(int i=0;i<nx;i++){
            int idx = j*nx + i;
            nodes[idx].x = i*dx;
            nodes[idx].y = j*dy;

            // Dirichlet boundary condition
            if(i==0 || i==nx-1 || j==0 || j==ny-1){
                nodes[idx].isBoundary = true;
                nodes[idx].value = 0.0;
            } else {
                nodes[idx].isBoundary = false;
                nodes[idx].value = 0.0;
            }
        }
    }
}
