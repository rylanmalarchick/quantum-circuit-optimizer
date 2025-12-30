# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Rylan Malarchick

cmake_minimum_required(VERSION 3.18)
project(quantum-circuit-optimizer LANGUAGES CXX)

# ------------------------------------------------------------------------------
# C++ Standard
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd/IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Compiler Warnings
# ------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Woverloaded-virtual
    )
    # -Wnull-dereference causes false positives in GCC 13 with STL iterators
endif()

# Option to treat warnings as errors (enabled in CI)
option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# ------------------------------------------------------------------------------
# Library Target: qopt_ir
# ------------------------------------------------------------------------------
add_library(qopt_ir INTERFACE)
target_include_directories(qopt_ir INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ------------------------------------------------------------------------------
# Main Executable
# ------------------------------------------------------------------------------
add_executable(quantum_circuit_optimizer src/main.cpp)
target_link_libraries(quantum_circuit_optimizer PRIVATE qopt_ir)

# Apply warnings-as-errors only to our targets (not GoogleTest)
if(WARNINGS_AS_ERRORS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(quantum_circuit_optimizer PRIVATE -Werror)
    elseif(MSVC)
        target_compile_options(quantum_circuit_optimizer PRIVATE /WX)
    endif()
endif()

# ------------------------------------------------------------------------------
# GoogleTest
# ------------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# Prevent GoogleTest from overriding compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# ------------------------------------------------------------------------------
# Test Executables
# ------------------------------------------------------------------------------
add_executable(test_gate tests/ir/test_gate.cpp)
target_link_libraries(test_gate PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_gate)

add_executable(test_circuit tests/ir/test_circuit.cpp)
target_link_libraries(test_circuit PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_circuit)

add_executable(test_dag tests/ir/test_dag.cpp)
target_link_libraries(test_dag PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_dag)

add_executable(test_lexer tests/parser/test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_lexer)

add_executable(test_parser tests/parser/test_parser.cpp)
target_link_libraries(test_parser PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_parser)

add_executable(test_passes tests/passes/test_passes.cpp)
target_link_libraries(test_passes PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_passes)

add_executable(test_routing tests/routing/test_routing.cpp)
target_link_libraries(test_routing PRIVATE qopt_ir GTest::gtest_main)
gtest_discover_tests(test_routing)

# Apply warnings-as-errors to test targets
if(WARNINGS_AS_ERRORS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_gate PRIVATE -Werror)
        target_compile_options(test_circuit PRIVATE -Werror)
        target_compile_options(test_dag PRIVATE -Werror)
        target_compile_options(test_lexer PRIVATE -Werror)
        target_compile_options(test_parser PRIVATE -Werror)
        target_compile_options(test_passes PRIVATE -Werror)
        target_compile_options(test_routing PRIVATE -Werror)
    elseif(MSVC)
        target_compile_options(test_gate PRIVATE /WX)
        target_compile_options(test_circuit PRIVATE /WX)
        target_compile_options(test_dag PRIVATE /WX)
        target_compile_options(test_lexer PRIVATE /WX)
        target_compile_options(test_parser PRIVATE /WX)
        target_compile_options(test_passes PRIVATE /WX)
        target_compile_options(test_routing PRIVATE /WX)
    endif()
endif()

# ------------------------------------------------------------------------------
# Example Programs
# ------------------------------------------------------------------------------
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    add_executable(basic_usage examples/basic_usage.cpp)
    target_link_libraries(basic_usage PRIVATE qopt_ir)

    add_executable(optimization_demo examples/optimization_demo.cpp)
    target_link_libraries(optimization_demo PRIVATE qopt_ir)

    add_executable(routing_demo examples/routing_demo.cpp)
    target_link_libraries(routing_demo PRIVATE qopt_ir)

    if(WARNINGS_AS_ERRORS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(basic_usage PRIVATE -Werror)
            target_compile_options(optimization_demo PRIVATE -Werror)
            target_compile_options(routing_demo PRIVATE -Werror)
        elseif(MSVC)
            target_compile_options(basic_usage PRIVATE /WX)
            target_compile_options(optimization_demo PRIVATE /WX)
            target_compile_options(routing_demo PRIVATE /WX)
        endif()
    endif()
endif()

# ------------------------------------------------------------------------------
# Benchmark Programs
# ------------------------------------------------------------------------------
option(BUILD_BENCHMARKS "Build benchmark programs" OFF)

if(BUILD_BENCHMARKS)
    add_executable(benchmark_circuits benchmarks/benchmark_circuits.cpp)
    target_link_libraries(benchmark_circuits PRIVATE qopt_ir)

    if(WARNINGS_AS_ERRORS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(benchmark_circuits PRIVATE -Werror)
        elseif(MSVC)
            target_compile_options(benchmark_circuits PRIVATE /WX)
        endif()
    endif()
endif()

# ------------------------------------------------------------------------------
# Documentation (Doxygen)
# ------------------------------------------------------------------------------
option(BUILD_DOCS "Build Doxygen documentation" OFF)

if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
