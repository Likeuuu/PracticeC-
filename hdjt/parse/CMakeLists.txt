cmake_minimum_required(VERSION 3.10)
project(simulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(simulator
    main.cpp
    parser.cpp
)
