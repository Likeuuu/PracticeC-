cmake_minimum_required(VERSION 3.20)

project(MiniNetlistFrontend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MNF_BUILD_TESTS "Build tests" ON)

add_library(mnf
    src/lexer/lexer.cpp
    src/parser/ast.cpp
    src/parser/parser.cpp
    src/semantic/symbol_table.cpp
    src/semantic/semantic_checker.cpp
    src/elaboration/ir.cpp
    src/elaboration/elaborator.cpp
    src/output/json_writer.cpp
    src/output/text_writer.cpp
    src/sim/combinational_evaluator.cpp
    src/app/cli.cpp
)

target_include_directories(mnf
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(mnf PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(mnf PRIVATE /W4 /permissive-)
else()
    target_compile_options(mnf PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(mnf_cli
    src/main.cpp
)

target_link_libraries(mnf_cli PRIVATE mnf)

if(MNF_BUILD_TESTS)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    include(GoogleTest)

    add_executable(mnf_tests
        tests/lexer_test.cpp
        tests/parser_test.cpp
        tests/semantic_checker_test.cpp
        tests/semantic_checker_repeat_ports.cpp
        tests/semantic_checker_port_consistency_test.cpp
        tests/semantic_checker_wire_port_conflict_test.cpp
        tests/semantic_checker_repeat_wires_test.cpp
        tests/elaborator_test.cpp
        tests/elaboration_cycle_test.cpp
        tests/sim/combinational_evaluator_test.cpp
    )

    target_link_libraries(mnf_tests PRIVATE mnf GTest::gtest_main)

    gtest_discover_tests(mnf_tests)
endif()
