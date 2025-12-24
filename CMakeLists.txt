## Standards

cmake_minimum_required(VERSION 3.10)

project(QuantumCircuitOptimizer)

set(CMAKE_CXX_STANDARD 17)

## Add your CMake configuration here

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(quantum_circuit_optimizer src/main.cpp)
