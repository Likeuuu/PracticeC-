cmake_minimum_required(VERSION 3.10)
project(PoissonSolver)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

add_executable(PoissonSolver
    src/main.cpp
    src/Grid.cpp
    src/SparseMatrix.cpp
    src/Solver.cpp
)
