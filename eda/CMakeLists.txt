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
    src/codegen/code_generator.cpp
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


find_path(LIBCLANG_INCLUDE_DIR
    NAMES clang-c/Index.h
    PATHS /usr/lib/llvm-18/include /usr/include
)

find_library(LIBCLANG_LIBRARY
    NAMES clang libclang
    PATHS /usr/lib/llvm-18/lib /usr/lib /lib/x86_64-linux-gnu
)

if(LIBCLANG_INCLUDE_DIR AND LIBCLANG_LIBRARY)
    add_executable(clang_ast_demo
        tools/clang_ast_demo/main.cpp
    )

    target_include_directories(clang_ast_demo PRIVATE ${LIBCLANG_INCLUDE_DIR})
    target_link_libraries(clang_ast_demo PRIVATE ${LIBCLANG_LIBRARY})
    target_compile_features(clang_ast_demo PRIVATE cxx_std_17)

    add_executable(verify_codegen
        tools/verify_codegen/main.cpp
    )

    target_include_directories(verify_codegen PRIVATE ${LIBCLANG_INCLUDE_DIR})
    target_link_libraries(verify_codegen PRIVATE ${LIBCLANG_LIBRARY})
    target_compile_features(verify_codegen PRIVATE cxx_std_17)
endif()

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
        tests/codegen/code_generator_test.cpp
    )

    target_link_libraries(mnf_tests PRIVATE mnf GTest::gtest_main)

    gtest_discover_tests(mnf_tests)

    if(TARGET verify_codegen)
        add_test(
            NAME VerifyCodegenAstHierarchy
            COMMAND
                /bin/sh -c
                "$<TARGET_FILE:mnf_cli> ${CMAKE_CURRENT_SOURCE_DIR}/examples/codegen_hierarchy.nl top --format=cpp --input=in1=1 --input=in2=1 --print-nets > ${CMAKE_CURRENT_BINARY_DIR}/verify_codegen_hierarchy.cpp && $<TARGET_FILE:verify_codegen> ${CMAKE_CURRENT_BINARY_DIR}/verify_codegen_hierarchy.cpp top 7"
        )
    endif()
endif()
