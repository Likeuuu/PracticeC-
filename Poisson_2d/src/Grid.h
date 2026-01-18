#pragma once
#include <vector>

struct Node {
    double x, y;
    double value;
    bool isBoundary;
};

class Grid {
public:
    int nx, ny;
    double dx, dy;
    std::vector<Node> nodes;

    Grid(int nx_, int ny_);
    void initialize();
};
