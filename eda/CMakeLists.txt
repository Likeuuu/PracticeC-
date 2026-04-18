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
    enable_testing()

    add_executable(mnf_lexer_test tests/lexer_test.cpp)
    target_link_libraries(mnf_lexer_test PRIVATE mnf)
    add_test(NAME mnf_lexer_test COMMAND mnf_lexer_test)

    add_executable(mnf_parser_test tests/parser_test.cpp)
    target_link_libraries(mnf_parser_test PRIVATE mnf)
    add_test(NAME mnf_parser_test COMMAND mnf_parser_test)

    add_executable(mnf_semantic_checker_test tests/semantic_checker_test.cpp)
    target_link_libraries(mnf_semantic_checker_test PRIVATE mnf)
    add_test(NAME mnf_semantic_checker_test COMMAND mnf_semantic_checker_test)

    add_executable(mnf_semantic_checker_repeat_ports tests/semantic_checker_repeat_ports.cpp)
    target_link_libraries(mnf_semantic_checker_repeat_ports PRIVATE mnf)
    add_test(NAME mnf_semantic_checker_repeat_ports COMMAND mnf_semantic_checker_repeat_ports)

    add_executable(mnf_semantic_checker_port_consistency_test tests/semantic_checker_port_consistency_test.cpp)
    target_link_libraries(mnf_semantic_checker_port_consistency_test PRIVATE mnf)
    add_test(NAME mnf_semantic_checker_port_consistency_test COMMAND mnf_semantic_checker_port_consistency_test)

    add_executable(mnf_elaborator_test tests/elaborator_test.cpp)
    target_link_libraries(mnf_elaborator_test PRIVATE mnf)
    add_test(NAME mnf_elaborator_test COMMAND mnf_elaborator_test)

    add_executable(mnf_elaboration_cycle_test tests/elaboration_cycle_test.cpp)
    target_link_libraries(mnf_elaboration_cycle_test PRIVATE mnf)
    add_test(NAME mnf_elaboration_cycle_test COMMAND mnf_elaboration_cycle_test)
endif()
