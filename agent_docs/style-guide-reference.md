# Style Guide Reference

> **Note:** This is the exhaustive style reference. For philosophy and principles, 
> see [research-code-principles.md](research-code-principles.md). For agent coding 
> sessions, see [agent-coding-context.md](agent-coding-context.md). For repository 
> hygiene, see [repo-standards.md](repo-standards.md).

**Author:** Rylan Malarchick  
**Version:** 1.1  
**Last Updated:** December 2025  
**Scope:** All quantum computing and research software projects

---

## Related Documents

| Document | Purpose |
|----------|---------|
| [agent-coding-context.md](agent-coding-context.md) | Quick reference for AI agents |
| [research-code-principles.md](research-code-principles.md) | Philosophy and principles |
| [style-guide-reference.md](style-guide-reference.md) | Detailed style conventions |
| [repo-standards.md](repo-standards.md) | Git workflow and repository hygiene |

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [General Principles](#2-general-principles)
3. [C++ Standards](#3-c-standards)
4. [Python Standards](#4-python-standards)
5. [Cross-Language Concerns](#5-cross-language-concerns)
6. [Repository Standards](#6-repository-standards)
7. [Security](#7-security)
8. [CI/CD Requirements](#8-cicd-requirements)
9. [Project-Specific Notes](#9-project-specific-notes)
10. [Appendix A: Quick Reference Card](#appendix-a-quick-reference-card)
11. [Appendix B: File Templates](#appendix-b-file-templates)

---

## 1. Introduction

### 1.1 Purpose

This document defines coding standards for all research software projects. The goals are:

1. **Readability**: Code is read far more often than written. Optimize for the reader.
2. **Maintainability**: Code should be easy to modify, extend, and debug.
3. **Reproducibility**: Research code must produce consistent, verifiable results.
4. **Professionalism**: Research code that meets industry standards demonstrates engineering maturity.

### 1.2 Scope

These standards apply to:

- **CUDA Quantum Simulator** (`quantum/compilers/cuda-quantum-simulator/`)
- **Quantum Circuit Optimizer** (`quantum/compilers/quantum-circuit-optimizer/`)
- **QubitPulseOpt** (`quantum/Controls/QubitPulseOpt/`)
- **QuantumVQE** (`quantum/HPC/QuantumVQE/`)
- **CloudML** (`NASA/cloudML/`)
- All future quantum computing and research projects

### 1.3 Philosophy

> "Any fool can write code that a computer can understand. Good programmers write code that humans can understand." — Martin Fowler

Key principles:

- **Explicit over implicit**: Make intentions clear in code
- **Fail fast**: Detect and report errors as early as possible
- **DRY (Don't Repeat Yourself)**: Extract common patterns into reusable components
- **YAGNI (You Aren't Gonna Need It)**: Don't add functionality until needed
- **Single Responsibility**: Each class/function does one thing well

### 1.4 How to Use This Document

- **For new projects**: Follow all standards from the start
- **For existing projects**: Adopt standards incrementally; new code follows standards
- **For code review**: Use this as a checklist
- **For LLM assistance**: Reference this document when asking for code modifications

---

## 2. General Principles

### 2.1 RAII (Resource Acquisition Is Initialization)

All resources (memory, file handles, network connections, GPU memory) must be managed through RAII:

- **Acquire** resources in constructors
- **Release** resources in destructors
- **Never** use raw `new`/`delete` or `malloc`/`free`
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) for heap allocations

**Rationale**: Prevents resource leaks, ensures exception safety, eliminates manual cleanup.

### 2.2 Fail Fast with Descriptive Errors

Validate inputs at function boundaries and fail immediately with clear error messages:

```cpp
// Good: Early validation with descriptive message
void Circuit::addGate(const Gate& gate) {
    if (gate.qubit() >= numQubits_) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(gate.qubit()) + 
            " out of range [0, " + std::to_string(numQubits_ - 1) + "]"
        );
    }
    gates_.push_back(gate);
}

// Bad: Silent failure or cryptic error later
void Circuit::addGate(const Gate& gate) {
    gates_.push_back(gate);  // May cause segfault later
}
```

```python
# Good: Early validation
def optimize_pulse(self, target_unitary: np.ndarray) -> GRAPEResult:
    if target_unitary.shape[0] != target_unitary.shape[1]:
        raise ValueError(
            f"Target unitary must be square, got shape {target_unitary.shape}"
        )
    if not np.allclose(target_unitary @ target_unitary.conj().T, np.eye(target_unitary.shape[0])):
        raise ValueError("Target unitary must be unitary (U @ U† = I)")
    # ... proceed with optimization
```

### 2.3 Zero Warnings Policy

All code must compile/run with zero warnings:

- **C++**: Compile with `-Wall -Wextra -Wpedantic -Werror`
- **Python**: Pass `ruff`, `mypy`, `pylint` with zero warnings
- **CI/CD**: Builds must fail on any warning

**Rationale**: Warnings often indicate real bugs. A codebase with warnings trains developers to ignore them, hiding real issues.

### 2.4 Test-Driven Development

Write tests before or alongside implementation:

- **Unit tests**: Test individual functions/classes in isolation
- **Integration tests**: Test component interactions
- **Regression tests**: Prevent fixed bugs from recurring

Coverage targets:

| Project Type | Minimum Coverage |
|--------------|------------------|
| Production code | 90% |
| Research code | 70% |
| Experimental/exploratory | 50% |

### 2.5 Documentation as Code

Documentation is not optional:

- **Every public function/class** must have a docstring/comment
- **Every file** must have a header explaining its purpose
- **Every non-obvious algorithm** must reference its source (paper, textbook)
- **Update docs when code changes**: Stale docs are worse than no docs

### 2.6 Bounded Complexity (Power of 10 Pragmatic Subset)

Adapted from NASA JPL's Power of 10 rules for safety-critical code:

| Rule | Description | Enforcement |
|------|-------------|-------------|
| **Bounded loops** | All loops must have a provable upper bound | Define `MAX_*` constants |
| **Small functions** | Functions ≤ 60 lines | Code review |
| **Minimal nesting** | Max 3-4 levels of indentation | Refactor into helper functions |
| **Limited preprocessor** | Only `#include`, `#pragma once`, simple `#define` | No token pasting, no variadic macros |
| **Zero warnings** | Compile with all warnings enabled | `-Werror` in CMake |
| **Explicit control flow** | No `goto`, `setjmp`, `longjmp` | Code review |

**Not adopted** (too restrictive for research code):

- No recursion (DAG traversal benefits from recursion)
- No dynamic allocation after init (circuits grow dynamically)
- No function pointers (needed for callbacks, pass infrastructure)
- 2 assertions per function (validation at boundaries is sufficient)

---

## 3. C++ Standards

### 3.1 Language Version

- **Standard**: C++17 minimum (C++20 where supported)
- **Compiler**: GCC 9+ or Clang 10+
- **CUDA**: CUDA 11.0+ with `--std=c++17`

### 3.2 File Organization

#### 3.2.1 File Extensions

| Extension | Usage |
|-----------|-------|
| `.hpp` | C++ headers (no CUDA dependencies) |
| `.cuh` | CUDA headers (contain `__global__`, `__device__`, CUDA types) |
| `.cpp` | C++ source files |
| `.cu` | CUDA source files |

#### 3.2.2 Directory Structure

```
project-name/
├── CMakeLists.txt
├── LICENSE                    # MIT license
├── README.md
├── .gitignore
├── include/
│   └── project/              # Namespace matches directory
│       ├── Types.hpp
│       ├── ir/
│       │   ├── Gate.hpp
│       │   ├── Circuit.hpp
│       │   └── DAG.hpp
│       └── passes/
│           ├── Pass.hpp
│           └── CancellationPass.hpp
├── src/
│   ├── ir/
│   │   ├── Gate.cpp
│   │   └── Circuit.cpp
│   ├── passes/
│   │   └── CancellationPass.cpp
│   └── main.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_gate.cpp
│   └── test_circuit.cpp
├── benchmarks/
│   └── bench_optimization.cpp
└── docs/
    └── architecture.md
```

#### 3.2.3 Include Guards

Use `#pragma once` (modern, widely supported, less error-prone than `#ifndef`):

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Gate.hpp
 * @brief Quantum gate representation
 */
#pragma once

#include <vector>
// ... rest of file
```

#### 3.2.4 Include Order

Organize includes in this order, with blank lines between groups:

```cpp
#include "Gate.hpp"           // 1. Own header (in .cpp files only)

#include "Circuit.hpp"        // 2. Project headers (alphabetized)
#include "Types.hpp"

#include <cuda_runtime.h>     // 3. Third-party headers (alphabetized)
#include <cuComplex.h>

#include <algorithm>          // 4. Standard library (alphabetized)
#include <optional>
#include <vector>
```

**Rationale**: Own header first catches missing includes in that header. Alphabetization makes it easy to find and avoid duplicates.

### 3.3 Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| **Namespaces** | lowercase, short | `qopt`, `qsim`, `qopt::ir` |
| **Classes/Structs** | PascalCase | `Gate`, `Circuit`, `PassManager` |
| **Enum classes** | PascalCase | `enum class GateType { X, Y, Z }` |
| **Enum values** | PascalCase | `GateType::CNOT`, `GateType::Hadamard` |
| **Methods** | camelCase | `addGate()`, `getQubits()`, `isParameterized()` |
| **Free functions** | camelCase | `toString(GateType)`, `createBellCircuit()` |
| **Private members** | trailing underscore | `numQubits_`, `gates_`, `parameter_` |
| **Local variables** | camelCase | `gateCount`, `targetQubit` |
| **Constants** | constexpr in namespace | `constants::MAX_QUBITS` |
| **Macros** | SCREAMING_SNAKE_CASE | `CUDA_CHECK(call)` |
| **Template params** | PascalCase, single letter or descriptive | `T`, `ValueType` |
| **Device pointers** | `d_` prefix | `d_state_`, `d_rngStates_` |
| **CUDA kernels** | camelCase with prefixes | `applyHadamard`, `dmApplyNoise` |

#### 3.3.1 Naming Examples

```cpp
namespace qopt::ir {

// Class: PascalCase
class Circuit {
public:
    // Method: camelCase
    void addGate(const Gate& gate);
    
    // Getter: camelCase, no "get" prefix for simple accessors
    [[nodiscard]] size_t numQubits() const noexcept { return numQubits_; }
    [[nodiscard]] size_t numGates() const noexcept { return gates_.size(); }
    
    // Query methods: is/has/can prefix
    [[nodiscard]] bool isEmpty() const noexcept { return gates_.empty(); }
    [[nodiscard]] bool hasParameterizedGates() const;
    
private:
    // Private members: trailing underscore
    size_t numQubits_;
    std::vector<Gate> gates_;
};

// Enum class: PascalCase for both enum and values
enum class GateType {
    X, Y, Z,           // Pauli gates
    H,                 // Hadamard
    S, T, Sdag, Tdag,  // Phase gates
    CNOT, CZ,          // Two-qubit gates (acronyms uppercase)
    Rx, Ry, Rz         // Rotations
};

// Free function: camelCase
[[nodiscard]] std::string toString(GateType type);

// Constants: in namespace, constexpr
namespace constants {
    constexpr size_t MAX_QUBITS = 30;
    constexpr size_t MAX_GATES = 1'000'000;
    constexpr double EPSILON = 1e-10;
}

}  // namespace qopt::ir
```

### 3.4 License and Documentation Headers

#### 3.4.1 File Header Template

Every source file must begin with:

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file FileName.hpp
 * @brief One-line description of the file's purpose
 * @author Rylan Malarchick
 * @date 2025
 *
 * Extended description explaining:
 * - What this file implements
 * - How it fits into the larger architecture
 * - Key algorithms or data structures
 *
 * @see RelatedFile.hpp for cross-references
 *
 * @references
 * - Nielsen & Chuang, "Quantum Computation and Quantum Information" (2010)
 * - Relevant paper with DOI or arXiv link
 */
#pragma once
```

#### 3.4.2 Class Documentation

```cpp
/**
 * @brief Represents a quantum circuit as a sequence of gates.
 *
 * Circuit provides a builder-style API for constructing quantum circuits
 * and supports conversion to DAG representation for optimization.
 *
 * Thread safety: Not thread-safe. Use external synchronization if needed.
 *
 * Example:
 * @code
 * Circuit circuit(4);
 * circuit.h(0).cnot(0, 1).rz(1, M_PI/4);
 * auto dag = circuit.toDAG();
 * @endcode
 *
 * @see DAG for optimization-friendly representation
 * @see Gate for individual gate operations
 */
class Circuit {
    // ...
};
```

#### 3.4.3 Method Documentation

```cpp
/**
 * @brief Adds a rotation gate around the Z axis.
 *
 * Applies Rz(θ) = exp(-i θ/2 Z) = [[e^{-iθ/2}, 0], [0, e^{iθ/2}]]
 *
 * @param qubit Target qubit index (0-indexed)
 * @param theta Rotation angle in radians
 * @return Reference to this circuit (for method chaining)
 *
 * @throws std::out_of_range if qubit >= numQubits()
 * @throws std::invalid_argument if theta is NaN or infinite
 *
 * @note Angles are not automatically normalized to [0, 2π)
 *
 * @see rx(), ry() for other rotation axes
 */
Circuit& rz(size_t qubit, double theta);
```

### 3.5 Memory Management

#### 3.5.1 Smart Pointers

| Pointer Type | Use Case |
|--------------|----------|
| `std::unique_ptr<T>` | Sole ownership, most common |
| `std::shared_ptr<T>` | Shared ownership (rare, has overhead) |
| `std::weak_ptr<T>` | Non-owning reference to shared_ptr |
| Raw pointer (`T*`) | Non-owning reference, must not outlive owner |

```cpp
// Good: unique_ptr for ownership
class PassManager {
public:
    void addPass(std::unique_ptr<Pass> pass) {
        passes_.push_back(std::move(pass));
    }
    
private:
    std::vector<std::unique_ptr<Pass>> passes_;
};

// Good: Raw pointer for non-owning reference
class DAGNode {
public:
    void addPredecessor(DAGNode* pred) {  // Does not own pred
        predecessors_.push_back(pred);
    }
    
private:
    std::vector<DAGNode*> predecessors_;  // Non-owning
};
```

#### 3.5.2 RAII Wrapper for CUDA Memory

```cpp
/**
 * @brief RAII wrapper for CUDA device memory.
 *
 * Automatically allocates on construction and frees on destruction.
 * Move-only (no copying).
 */
template <typename T>
class CudaMemory {
public:
    explicit CudaMemory(size_t count);
    ~CudaMemory() noexcept;
    
    // Disable copy
    CudaMemory(const CudaMemory&) = delete;
    CudaMemory& operator=(const CudaMemory&) = delete;
    
    // Enable move
    CudaMemory(CudaMemory&& other) noexcept;
    CudaMemory& operator=(CudaMemory&& other) noexcept;
    
    [[nodiscard]] T* get() noexcept { return ptr_; }
    [[nodiscard]] const T* get() const noexcept { return ptr_; }
    
    void copyFromHost(const T* src, size_t count);
    void copyToHost(T* dst, size_t count) const;

private:
    T* ptr_ = nullptr;
    size_t count_ = 0;
};
```

#### 3.5.3 Rule of Five

If a class manages resources, implement all five special member functions:

```cpp
class StateVector {
public:
    explicit StateVector(int numQubits);
    ~StateVector() noexcept;
    
    // Copy (often deleted for resource-managing classes)
    StateVector(const StateVector&) = delete;
    StateVector& operator=(const StateVector&) = delete;
    
    // Move
    StateVector(StateVector&& other) noexcept;
    StateVector& operator=(StateVector&& other) noexcept;
    
private:
    cuDoubleComplex* d_state_ = nullptr;  // GPU memory
    size_t size_ = 0;
};
```

### 3.6 Error Handling

#### 3.6.1 Exception Types

| Exception | Use Case |
|-----------|----------|
| `std::invalid_argument` | Invalid parameter value |
| `std::out_of_range` | Index out of bounds |
| `std::runtime_error` | General runtime error |
| `std::logic_error` | Programming error (should not happen) |

#### 3.6.2 CUDA Error Checking

Define these macros in a common header:

```cpp
// In Constants.hpp or CudaUtils.hpp

#define CUDA_CHECK(call)                                                    \
    do {                                                                    \
        cudaError_t err = call;                                            \
        if (err != cudaSuccess) {                                          \
            throw std::runtime_error(                                      \
                std::string("CUDA error in ") + __FILE__ + ":" +          \
                std::to_string(__LINE__) + ": " + cudaGetErrorString(err) \
            );                                                             \
        }                                                                  \
    } while (0)

#define CUDA_CHECK_LAST_ERROR()                                            \
    do {                                                                   \
        cudaError_t err = cudaGetLastError();                             \
        if (err != cudaSuccess) {                                         \
            throw std::runtime_error(                                     \
                std::string("CUDA kernel error in ") + __FILE__ + ":" +  \
                std::to_string(__LINE__) + ": " + cudaGetErrorString(err)\
            );                                                            \
        }                                                                 \
    } while (0)
```

Usage:

```cpp
void StateVector::allocate() {
    CUDA_CHECK(cudaMalloc(&d_state_, size_ * sizeof(cuDoubleComplex)));
    CUDA_CHECK(cudaMemset(d_state_, 0, size_ * sizeof(cuDoubleComplex)));
}
```

#### 3.6.3 Destructor Exception Safety

Destructors must be `noexcept`. Log errors instead of throwing:

```cpp
StateVector::~StateVector() noexcept {
    if (d_state_) {
        cudaError_t err = cudaFree(d_state_);
        if (err != cudaSuccess) {
            // Log but do not throw
            std::cerr << "Warning: cudaFree failed: " 
                      << cudaGetErrorString(err) << std::endl;
        }
    }
}
```

### 3.7 Modern C++ Features

#### 3.7.1 Required Features

| Feature | Usage |
|---------|-------|
| `[[nodiscard]]` | All functions returning values that must be used |
| `constexpr` | Compile-time constants and functions |
| `noexcept` | Functions that cannot throw (destructors, move ops, simple getters) |
| `auto` | Iterator types, complex template return types |
| Range-based for | Preferred over index-based loops |
| `std::optional` | Return values that may not exist |
| `std::string_view` | Non-owning string parameters |
| Structured bindings | Unpacking tuples/pairs |
| `emplace_back` | In-place construction in containers |

#### 3.7.2 Examples

```cpp
// [[nodiscard]] - Compiler warns if return value ignored
[[nodiscard]] std::optional<Gate> Gate::merge(const Gate& other) const;

// constexpr - Compile-time evaluation
constexpr double PI = 3.14159265358979323846;
constexpr size_t stateSize(int numQubits) { return 1ULL << numQubits; }

// noexcept - Cannot throw
[[nodiscard]] size_t numQubits() const noexcept { return numQubits_; }

// auto for iterators
for (auto it = gates_.begin(); it != gates_.end(); ++it) { ... }

// Range-based for (preferred)
for (const auto& gate : gates_) { ... }

// std::optional for nullable returns
std::optional<double> Gate::parameter() const noexcept {
    if (isParameterized()) {
        return parameter_;
    }
    return std::nullopt;
}

// Structured bindings
auto [fidelity, iterations] = optimizer.run(target);

// emplace_back
gates_.emplace_back(GateType::Rz, qubit, theta);
```

### 3.8 Build System (CMake)

#### 3.8.1 Minimum CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.18)
project(quantum-circuit-optimizer LANGUAGES CXX)

# Standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Use -std=c++17, not -std=gnu++17

# Generate compile_commands.json for IDE/LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Strict warnings (Power of 10 Rule #10)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow                    # Warn on variable shadowing
        -Wconversion                # Warn on implicit conversions
        -Wsign-conversion           # Warn on signed/unsigned conversion
        -Wnon-virtual-dtor          # Warn on classes with virtual functions but non-virtual dtor
        -Wold-style-cast            # Warn on C-style casts
        -Wcast-align                # Warn on potential alignment issues
        -Wunused                    # Warn on unused variables/functions
        -Woverloaded-virtual        # Warn on hidden virtual functions
        -Wformat=2                  # Warn on printf format issues
    )
    
    # Treat warnings as errors in CI
    if(DEFINED ENV{CI})
        add_compile_options(-Werror)
    endif()
endif()

# Library
add_library(qopt_lib
    src/ir/Gate.cpp
    src/ir/Circuit.cpp
    src/ir/DAG.cpp
    src/passes/CancellationPass.cpp
    src/passes/RotationMergePass.cpp
)

target_include_directories(qopt_lib PUBLIC include)

# Executable
add_executable(qopt src/main.cpp)
target_link_libraries(qopt PRIVATE qopt_lib)

# Testing with GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
    tests/test_gate.cpp
    tests/test_circuit.cpp
    tests/test_dag.cpp
    tests/test_cancellation.cpp
)

target_link_libraries(tests PRIVATE qopt_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)
```

#### 3.8.2 CUDA CMake Configuration

For CUDA projects, add:

```cmake
project(cuda-quantum-simulator LANGUAGES CXX CUDA)

# CUDA standards
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Target architecture (update for your GPU)
set(CMAKE_CUDA_ARCHITECTURES 89)  # RTX 4070

# CUDA-specific warnings
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --expt-relaxed-constexpr")
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -Wall -Xcompiler -Wextra")

# Link CUDA libraries
target_link_libraries(mylib cudart curand)
```

### 3.9 Testing with GoogleTest

#### 3.9.1 Test File Structure

```cpp
// tests/test_gate.cpp

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

#include "qopt/ir/Gate.hpp"

#include <gtest/gtest.h>

namespace qopt::ir {
namespace {

// Test fixture for shared setup
class GateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup
    }
    
    // Helper: compare gates with tolerance
    void expectGatesEqual(const Gate& a, const Gate& b) {
        EXPECT_EQ(a.type(), b.type());
        EXPECT_EQ(a.qubits(), b.qubits());
        // Compare parameters with floating-point tolerance
        if (a.parameter() && b.parameter()) {
            EXPECT_NEAR(*a.parameter(), *b.parameter(), 1e-10);
        } else {
            EXPECT_EQ(a.parameter().has_value(), b.parameter().has_value());
        }
    }
};

// Test naming: TestFixture_MethodOrBehavior_ExpectedResult
TEST_F(GateTest, FactoryX_CreatesValidXGate) {
    auto x = Gate::x(0);
    
    EXPECT_EQ(x.type(), GateType::X);
    ASSERT_EQ(x.qubits().size(), 1);
    EXPECT_EQ(x.qubits()[0], 0);
    EXPECT_FALSE(x.parameter().has_value());
}

TEST_F(GateTest, FactoryRz_StoresAngleParameter) {
    auto rz = Gate::rz(2, M_PI / 4);
    
    EXPECT_EQ(rz.type(), GateType::Rz);
    ASSERT_TRUE(rz.parameter().has_value());
    EXPECT_NEAR(*rz.parameter(), M_PI / 4, 1e-10);
}

TEST_F(GateTest, IsInverse_XXEqualsIdentity) {
    auto x1 = Gate::x(0);
    auto x2 = Gate::x(0);
    
    EXPECT_TRUE(x1.isInverse(x2));
}

TEST_F(GateTest, IsInverse_DifferentQubitsNotInverse) {
    auto x0 = Gate::x(0);
    auto x1 = Gate::x(1);
    
    EXPECT_FALSE(x0.isInverse(x1));
}

TEST_F(GateTest, Merge_TwoRzGatesCombine) {
    auto rz1 = Gate::rz(0, M_PI / 4);
    auto rz2 = Gate::rz(0, M_PI / 4);
    
    auto merged = rz1.merge(rz2);
    
    ASSERT_TRUE(merged.has_value());
    EXPECT_EQ(merged->type(), GateType::Rz);
    EXPECT_NEAR(*merged->parameter(), M_PI / 2, 1e-10);
}

}  // namespace
}  // namespace qopt::ir
```

#### 3.9.2 Test Categories

Organize tests by type:

| Test Type | Purpose | Example |
|-----------|---------|---------|
| Unit tests | Test single function/class | `test_gate.cpp` |
| Integration tests | Test component interactions | `test_optimizer_pipeline.cpp` |
| Regression tests | Verify bug fixes stay fixed | `test_issue_42.cpp` |
| Benchmark tests | Performance measurements | `bench_optimization.cpp` |
| Boundary tests | Edge cases, limits | `test_boundary.cpp` |

---

## 4. Python Standards

### 4.1 Language Version

- **Minimum**: Python 3.9
- **Recommended**: Python 3.10+ (for improved type hints)
- **Type checking**: mypy with moderate strictness

### 4.2 File Organization

#### 4.2.1 Package Structure

```
project/
├── pyproject.toml           # Project configuration (PEP 517/518)
├── setup.cfg                # Legacy compatibility (optional)
├── requirements.txt         # Pinned dependencies
├── README.md
├── LICENSE
├── .gitignore
├── src/
│   └── package_name/
│       ├── __init__.py      # Public API exports
│       ├── config.py        # Configuration (dataclasses)
│       ├── module1.py
│       ├── module2.py
│       └── subpackage/
│           ├── __init__.py
│           └── submodule.py
├── tests/
│   ├── conftest.py          # pytest fixtures
│   ├── test_module1.py
│   └── test_module2.py
├── scripts/                 # Utility scripts
├── notebooks/               # Jupyter notebooks
├── docs/                    # Documentation
└── experiments/             # Research experiments (separate from production)
```

#### 4.2.2 __init__.py Exports

Explicitly define the public API:

```python
# src/qubitpulseopt/__init__.py
"""
QubitPulseOpt - Quantum pulse optimization library.

This package provides tools for optimizing control pulses for quantum gates
using GRAPE (Gradient Ascent Pulse Engineering) and Krotov algorithms.
"""

from .optimization.grape import GRAPEOptimizer, GRAPEResult
from .optimization.gates import UniversalGates, GateResult
from .optimization.compilation import GateCompiler, CompiledCircuit
from .hamiltonian.drift import DriftHamiltonian
from .hamiltonian.control import ControlHamiltonian
from .io.export import PulseExporter

__all__ = [
    "GRAPEOptimizer",
    "GRAPEResult",
    "UniversalGates",
    "GateResult",
    "GateCompiler",
    "CompiledCircuit",
    "DriftHamiltonian",
    "ControlHamiltonian",
    "PulseExporter",
]

__version__ = "1.0.0"
```

### 4.3 Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| **Modules** | snake_case | `grape.py`, `pulse_exporter.py` |
| **Packages** | snake_case | `qubitpulseopt`, `cbh_retrieval` |
| **Classes** | PascalCase | `GRAPEOptimizer`, `DriftHamiltonian` |
| **Exceptions** | PascalCase + Error suffix | `ConfigError`, `OptimizationError` |
| **Functions** | snake_case | `optimize_unitary()`, `load_pulse()` |
| **Methods** | snake_case | `compute_fidelity()`, `add_control()` |
| **Private methods** | leading underscore | `_compute_propagators()` |
| **Constants** | UPPER_SNAKE_CASE | `MAX_ITERATIONS`, `DEFAULT_SEED` |
| **Variables** | snake_case | `target_unitary`, `pulse_amplitudes` |
| **Type aliases** | PascalCase | `PulseArray = np.ndarray` |

#### 4.3.1 Naming Examples

```python
# Module: snake_case
# grape.py

# Constants: UPPER_SNAKE_CASE
MAX_ITERATIONS = 10000
DEFAULT_TOLERANCE = 1e-10
DETERMINISTIC_SEED = 42

# Class: PascalCase
class GRAPEOptimizer:
    """GRAPE (Gradient Ascent Pulse Engineering) optimizer."""
    
    # Method: snake_case
    def optimize_unitary(
        self,
        target_unitary: np.ndarray,
        initial_pulses: Optional[np.ndarray] = None,
    ) -> GRAPEResult:
        """Optimize pulses to implement target unitary."""
        # Variable: snake_case
        current_fidelity = 0.0
        fidelity_history = []
        
        # Private method call
        propagators = self._compute_propagators(initial_pulses)
        
        return GRAPEResult(...)
    
    # Private method: leading underscore
    def _compute_propagators(self, pulses: np.ndarray) -> List[np.ndarray]:
        """Compute time-evolution propagators for given pulses."""
        ...

# Exception: PascalCase + Error
class OptimizationError(Exception):
    """Raised when optimization fails to converge."""
    pass
```

### 4.4 Documentation

#### 4.4.1 Docstring Styles

Two styles are acceptable; be consistent within a project:

**NumPy Style** (used in QubitPulseOpt):

```python
def optimize_unitary(
    self,
    target_unitary: np.ndarray,
    max_iterations: int = 1000,
    tolerance: float = 1e-10,
) -> GRAPEResult:
    """
    Optimize control pulses to implement target unitary.
    
    Uses GRAPE algorithm with L-BFGS-B optimizer to find pulse
    amplitudes that maximize gate fidelity.
    
    Parameters
    ----------
    target_unitary : np.ndarray
        Target unitary matrix, shape (d, d) where d = 2^n_qubits.
        Must be unitary (U @ U† = I).
    max_iterations : int, optional
        Maximum optimization iterations. Default: 1000.
    tolerance : float, optional
        Convergence tolerance for fidelity. Default: 1e-10.
    
    Returns
    -------
    GRAPEResult
        Optimization result containing final fidelity, optimized
        pulses, and convergence information.
    
    Raises
    ------
    ValueError
        If target_unitary is not square or not unitary.
    OptimizationError
        If optimization fails to converge.
    
    See Also
    --------
    optimize_gate : Optimize a named gate (X, Y, H, etc.)
    GRAPEResult : Result container class
    
    Notes
    -----
    The GRAPE algorithm [1]_ computes gradients analytically using
    the chain rule through the time-evolution operator.
    
    References
    ----------
    .. [1] Khaneja et al., "Optimal control of coupled spin dynamics",
           J. Magn. Reson. 172, 296 (2005). doi:10.1016/j.jmr.2004.11.004
    
    Examples
    --------
    >>> optimizer = GRAPEOptimizer(drift, controls)
    >>> result = optimizer.optimize_unitary(target_x_gate)
    >>> print(f"Fidelity: {result.final_fidelity:.6f}")
    Fidelity: 0.999998
    """
```

**Google Style** (used in CloudML):

```python
def train_production_model(
    self,
    X: np.ndarray,
    y: np.ndarray,
    feature_names: List[str],
) -> Tuple[GradientBoostingRegressor, StandardScaler, Dict, Dict]:
    """Train production GBDT model on full dataset.
    
    Trains a Gradient Boosting Decision Tree regressor with standardized
    features and returns the model along with diagnostics.
    
    Args:
        X: Feature matrix of shape (n_samples, n_features).
        y: Target values of shape (n_samples,).
        feature_names: List of feature names matching X columns.
    
    Returns:
        A tuple containing:
            - model: Trained GradientBoostingRegressor.
            - scaler: Fitted StandardScaler for feature normalization.
            - metrics: Dict with training metrics (R², MAE, RMSE).
            - feature_importance: Dict mapping feature names to importance.
    
    Raises:
        ValueError: If X and y have incompatible shapes.
        ValueError: If feature_names length doesn't match X columns.
    
    Example:
        >>> trainer = ProductionModelTrainer()
        >>> X, y, names = trainer.load_data(data_path)
        >>> model, scaler, metrics, importance = trainer.train_production_model(X, y, names)
        >>> print(f"R² = {metrics['r2']:.3f}")
        R² = 0.744
    """
```

#### 4.4.2 Module Docstrings

```python
"""
Sprint 6 - Production Model Training.

This module provides the production GBDT model training pipeline for
cloud base height retrieval. It implements the final model selected
through hyperparameter optimization and validation studies.

Key components:
    - ProductionModelTrainer: Main training class
    - GBDT_CONFIG: Canonical hyperparameters (frozen dataclass)
    - train_production_model(): Convenience function

Usage:
    >>> from cbh_retrieval import train_production_model
    >>> model, scaler = train_production_model()

See Also:
    - offline_validation_tabular.py: Cross-validation framework
    - config.py: Configuration management
    - MODEL_CARD.md: Full model documentation

Author: Rylan Malarchick
Date: 2025
"""
```

### 4.5 Type Hints

#### 4.5.1 Required Type Hints

Type hints are **required** for:

- All public function/method signatures
- Class attributes
- Return types (use `-> None` for procedures)

```python
from typing import Dict, List, Optional, Tuple, Union
from dataclasses import dataclass
import numpy as np

# Type aliases for clarity
PulseArray = np.ndarray  # Shape: (n_controls, n_timeslices)
UnitaryMatrix = np.ndarray  # Shape: (d, d), complex

@dataclass
class GRAPEResult:
    """Result container for GRAPE optimization."""
    
    final_fidelity: float
    optimized_pulses: PulseArray
    fidelity_history: List[float]
    n_iterations: int
    converged: bool
    message: str


class GRAPEOptimizer:
    """GRAPE optimizer for quantum gate synthesis."""
    
    def __init__(
        self,
        drift: DriftHamiltonian,
        controls: List[ControlHamiltonian],
        n_timeslices: int = 100,
        total_time: float = 10.0,
    ) -> None:
        self._drift = drift
        self._controls = controls
        self._n_timeslices = n_timeslices
        self._total_time = total_time
    
    def optimize_unitary(
        self,
        target: UnitaryMatrix,
        max_iterations: int = 1000,
        tolerance: float = 1e-10,
        initial_pulses: Optional[PulseArray] = None,
    ) -> GRAPEResult:
        ...
    
    @property
    def n_controls(self) -> int:
        return len(self._controls)
```

#### 4.5.2 mypy Configuration

In `pyproject.toml`:

```toml
[tool.mypy]
python_version = "3.10"
warn_return_any = true
warn_unused_ignores = true
no_implicit_optional = true
check_untyped_defs = true
disallow_untyped_defs = false  # Too strict for research code

[[tool.mypy.overrides]]
module = "tests.*"
disallow_untyped_defs = false
```

### 4.6 Configuration

#### 4.6.1 Frozen Dataclasses

Use frozen dataclasses for immutable configuration:

```python
from dataclasses import dataclass, field
from typing import Dict, Any

@dataclass(frozen=True)
class GBDTConfig:
    """Canonical GBDT hyperparameters.
    
    These parameters were selected through hyperparameter optimization
    on the validation set. Do not modify without re-running validation.
    """
    
    n_estimators: int = 200
    max_depth: int = 8
    learning_rate: float = 0.05
    min_samples_split: int = 10
    min_samples_leaf: int = 5
    subsample: float = 0.8
    random_state: int = 42
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary for sklearn initialization."""
        return {
            "n_estimators": self.n_estimators,
            "max_depth": self.max_depth,
            "learning_rate": self.learning_rate,
            "min_samples_split": self.min_samples_split,
            "min_samples_leaf": self.min_samples_leaf,
            "subsample": self.subsample,
            "random_state": self.random_state,
        }


# Singleton instance
GBDT_CONFIG = GBDTConfig()
```

#### 4.6.2 YAML Configuration

For runtime configuration, use YAML with validation:

```python
# config.py
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict
import yaml

@dataclass
class Config:
    """Runtime configuration loaded from YAML."""
    
    data: Dict[str, Any]
    
    @classmethod
    def from_yaml(cls, path: Path) -> "Config":
        """Load configuration from YAML file."""
        if not path.exists():
            raise FileNotFoundError(f"Config file not found: {path}")
        
        with open(path) as f:
            data = yaml.safe_load(f)
        
        return cls(data=data)
    
    def __getattr__(self, key: str) -> Any:
        """Allow dot notation access to config values."""
        if key in self.data:
            value = self.data[key]
            if isinstance(value, dict):
                return Config(data=value)
            return value
        raise AttributeError(f"Config has no attribute '{key}'")
```

### 4.7 Testing with pytest

#### 4.7.1 pytest Configuration

In `pyproject.toml`:

```toml
[tool.pytest.ini_options]
minversion = "7.0"
testpaths = ["tests"]
python_files = ["test_*.py"]
python_classes = ["Test*"]
python_functions = ["test_*"]
addopts = [
    "-v",
    "--strict-markers",
    "--strict-config",
    "-ra",
    "--tb=short",
    "--color=yes",
]
markers = [
    "slow: marks tests as slow (deselect with '-m \"not slow\"')",
    "integration: marks integration tests",
    "unit: marks unit tests",
    "stochastic: marks tests with inherent randomness",
    "deterministic: marks tests that should be deterministic with fixed seed",
    "requires_gpu: marks tests requiring GPU",
    "requires_data: marks tests requiring data files",
]
filterwarnings = [
    "error",
    "ignore::DeprecationWarning",
]
```

#### 4.7.2 Test Structure

```python
# tests/test_grape.py
"""Unit tests for GRAPE optimizer."""

import numpy as np
import pytest

from qubitpulseopt import GRAPEOptimizer, GRAPEResult
from qubitpulseopt.optimization.gates import UniversalGates


class TestGRAPEOptimizer:
    """Tests for GRAPEOptimizer class."""
    
    @pytest.fixture
    def optimizer(self, single_qubit_drift, single_qubit_controls):
        """Create optimizer with standard single-qubit setup."""
        return GRAPEOptimizer(
            drift=single_qubit_drift,
            controls=single_qubit_controls,
            n_timeslices=50,
            total_time=10.0,
        )
    
    @pytest.fixture
    def target_x_gate(self):
        """Pauli X gate as optimization target."""
        return np.array([[0, 1], [1, 0]], dtype=complex)
    
    @pytest.mark.deterministic
    def test_optimize_x_gate_converges(self, optimizer, target_x_gate):
        """GRAPE should achieve high fidelity for X gate."""
        result = optimizer.optimize_unitary(
            target_x_gate,
            max_iterations=500,
            tolerance=1e-6,
        )
        
        assert result.converged
        assert result.final_fidelity > 0.999
    
    @pytest.mark.deterministic
    def test_fidelity_history_monotonic(self, optimizer, target_x_gate):
        """Fidelity should generally increase during optimization."""
        result = optimizer.optimize_unitary(target_x_gate)
        
        # Allow small dips due to numerical precision
        for i in range(1, len(result.fidelity_history)):
            assert result.fidelity_history[i] >= result.fidelity_history[i-1] - 1e-10
    
    def test_invalid_target_raises_error(self, optimizer):
        """Non-unitary target should raise ValueError."""
        non_unitary = np.array([[1, 1], [0, 1]], dtype=complex)
        
        with pytest.raises(ValueError, match="must be unitary"):
            optimizer.optimize_unitary(non_unitary)
    
    @pytest.mark.slow
    @pytest.mark.stochastic
    def test_random_unitaries_high_fidelity(self, optimizer):
        """GRAPE should achieve high fidelity for random unitaries."""
        rng = np.random.default_rng(42)
        
        for _ in range(10):
            # Generate random unitary via QR decomposition
            random_matrix = rng.standard_normal((2, 2)) + 1j * rng.standard_normal((2, 2))
            target, _ = np.linalg.qr(random_matrix)
            
            result = optimizer.optimize_unitary(target, max_iterations=1000)
            assert result.final_fidelity > 0.99
```

#### 4.7.3 Fixtures

Define shared fixtures in `conftest.py`:

```python
# tests/conftest.py
"""Shared pytest fixtures."""

import numpy as np
import pytest
import qutip as qt


@pytest.fixture
def deterministic_seed():
    """Fixed seed for reproducible tests."""
    return 42


@pytest.fixture(autouse=True)
def reset_random_state(deterministic_seed):
    """Reset random state before each test."""
    np.random.seed(deterministic_seed)


@pytest.fixture
def single_qubit_drift():
    """Standard single-qubit drift Hamiltonian: H0 = (ω/2) σz."""
    omega = 2 * np.pi * 5.0  # 5 GHz
    return DriftHamiltonian(omega / 2 * qt.sigmaz())


@pytest.fixture
def single_qubit_controls():
    """Standard single-qubit control Hamiltonians: Hx = σx, Hy = σy."""
    return [
        ControlHamiltonian(qt.sigmax(), "x"),
        ControlHamiltonian(qt.sigmay(), "y"),
    ]
```

### 4.8 Tooling

#### 4.8.1 Required Tools

| Tool | Purpose | Configuration |
|------|---------|---------------|
| **black** | Code formatting | `line-length = 100` |
| **isort** | Import sorting | `profile = "black"` |
| **ruff** | Fast linting | See config below |
| **mypy** | Type checking | See config above |
| **pytest** | Testing | See config above |
| **coverage** | Test coverage | `branch = true` |

#### 4.8.2 pyproject.toml Configuration

```toml
[project]
name = "qubitpulseopt"
version = "1.0.0"
description = "Quantum pulse optimization library"
requires-python = ">=3.9"
dependencies = [
    "numpy>=1.24",
    "scipy>=1.10",
    "qutip>=4.8",
    "matplotlib>=3.7",
]

[project.optional-dependencies]
dev = [
    "pytest>=7.0",
    "pytest-cov>=4.0",
    "black>=23.0",
    "isort>=5.12",
    "ruff>=0.1.0",
    "mypy>=1.0",
    "pre-commit>=3.0",
]

[tool.black]
line-length = 100
target-version = ["py310"]

[tool.isort]
profile = "black"
line_length = 100
known_first_party = ["qubitpulseopt"]

[tool.ruff]
line-length = 100
target-version = "py310"

[tool.ruff.lint]
select = [
    "E",      # pycodestyle errors
    "W",      # pycodestyle warnings
    "F",      # Pyflakes
    "I",      # isort
    "N",      # pep8-naming
    "D",      # pydocstyle
    "UP",     # pyupgrade
    "ANN",    # flake8-annotations
    "S",      # flake8-bandit (security)
    "B",      # flake8-bugbear
    "C4",     # flake8-comprehensions
    "DTZ",    # flake8-datetimez
    "ISC",    # flake8-implicit-str-concat
    "PIE",    # flake8-pie
    "PT",     # flake8-pytest-style
    "RSE",    # flake8-raise
    "RET",    # flake8-return
    "SIM",    # flake8-simplify
    "ARG",    # flake8-unused-arguments
    "ERA",    # eradicate (commented code)
    "PL",     # Pylint
    "RUF",    # Ruff-specific
]
ignore = [
    "D100",   # Missing docstring in public module (too strict)
    "D104",   # Missing docstring in public package
    "ANN101", # Missing type annotation for self
    "ANN102", # Missing type annotation for cls
]

[tool.ruff.lint.pydocstyle]
convention = "numpy"  # or "google"
```

#### 4.8.3 Pre-commit Configuration

```yaml
# .pre-commit-config.yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.5.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-json
      - id: check-added-large-files
        args: ['--maxkb=1000']
      - id: check-merge-conflict
      - id: detect-private-key

  - repo: https://github.com/psf/black
    rev: 23.12.1
    hooks:
      - id: black

  - repo: https://github.com/pycqa/isort
    rev: 5.13.2
    hooks:
      - id: isort

  - repo: https://github.com/astral-sh/ruff-pre-commit
    rev: v0.1.9
    hooks:
      - id: ruff
        args: [--fix, --exit-non-zero-on-fix]

  - repo: https://github.com/pre-commit/mirrors-mypy
    rev: v1.8.0
    hooks:
      - id: mypy
        additional_dependencies: [numpy, types-PyYAML]
```

---

## 5. Cross-Language Concerns

### 5.1 C++/Python Bindings (pybind11)

When creating Python bindings for C++ code, follow these conventions:

#### 5.1.1 Naming Translation

| C++ (camelCase) | Python (snake_case) |
|-----------------|---------------------|
| `addGate()` | `add_gate()` |
| `numQubits()` | `num_qubits` (property) |
| `getOptimizedPulses()` | `get_optimized_pulses()` |
| `GateType::CNOT` | `GateType.CNOT` |
| `CircuitOptimizer` | `CircuitOptimizer` (class names stay PascalCase) |

#### 5.1.2 pybind11 Binding Example

```cpp
// src/bindings/python_bindings.cpp

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "qopt/ir/Gate.hpp"
#include "qopt/ir/Circuit.hpp"
#include "qopt/passes/PassManager.hpp"

namespace py = pybind11;

PYBIND11_MODULE(qopt, m) {
    m.doc() = "Quantum circuit optimizer";
    
    // Enum: Keep PascalCase values
    py::enum_<qopt::ir::GateType>(m, "GateType")
        .value("X", qopt::ir::GateType::X)
        .value("Y", qopt::ir::GateType::Y)
        .value("Z", qopt::ir::GateType::Z)
        .value("H", qopt::ir::GateType::H)
        .value("CNOT", qopt::ir::GateType::CNOT)
        .value("Rz", qopt::ir::GateType::Rz)
        .export_values();
    
    // Gate class
    py::class_<qopt::ir::Gate>(m, "Gate")
        // Factory methods → snake_case
        .def_static("x", &qopt::ir::Gate::x, py::arg("qubit"),
            "Create an X (Pauli-X) gate")
        .def_static("h", &qopt::ir::Gate::h, py::arg("qubit"),
            "Create an H (Hadamard) gate")
        .def_static("cnot", &qopt::ir::Gate::cnot,
            py::arg("control"), py::arg("target"),
            "Create a CNOT (controlled-X) gate")
        .def_static("rz", &qopt::ir::Gate::rz,
            py::arg("qubit"), py::arg("angle"),
            "Create an Rz (Z-rotation) gate")
        // Properties → snake_case
        .def_property_readonly("type", &qopt::ir::Gate::type)
        .def_property_readonly("qubits", [](const qopt::ir::Gate& g) {
            auto span = g.qubits();
            return std::vector<size_t>(span.begin(), span.end());
        })
        .def_property_readonly("parameter", [](const qopt::ir::Gate& g) {
            return g.parameter();  // Returns std::optional → Python None or value
        })
        // Methods → snake_case
        .def("is_inverse", &qopt::ir::Gate::isInverse, py::arg("other"))
        .def("merge", &qopt::ir::Gate::merge, py::arg("other"));
    
    // Circuit class
    py::class_<qopt::ir::Circuit>(m, "Circuit")
        .def(py::init<size_t>(), py::arg("num_qubits"))
        // Methods → snake_case (even though C++ uses camelCase)
        .def("add_gate", &qopt::ir::Circuit::addGate, py::arg("gate"))
        .def("to_dag", &qopt::ir::Circuit::toDAG)
        // Properties
        .def_property_readonly("num_qubits", &qopt::ir::Circuit::numQubits)
        .def_property_readonly("num_gates", &qopt::ir::Circuit::numGates)
        .def("__len__", &qopt::ir::Circuit::numGates)
        .def("__repr__", [](const qopt::ir::Circuit& c) {
            return "<Circuit with " + std::to_string(c.numQubits()) + 
                   " qubits, " + std::to_string(c.numGates()) + " gates>";
        });
    
    // PassManager
    py::class_<qopt::passes::PassManager>(m, "PassManager")
        .def(py::init<>())
        .def("add_pass", &qopt::passes::PassManager::addPass)
        .def("run", &qopt::passes::PassManager::run, py::arg("circuit"));
}
```

#### 5.1.3 Python Wrapper Layer

For complex integrations, add a Python wrapper:

```python
# python/qopt/integration.py
"""Integration with QubitPulseOpt for cross-layer analysis."""

from typing import List, Tuple
import numpy as np

from . import _qopt  # C++ bindings
from qubitpulseopt import GRAPEOptimizer, GRAPEResult


class CrossLayerAnalyzer:
    """Analyze circuit optimization impact on pulse fidelity."""
    
    def __init__(
        self,
        optimizer: GRAPEOptimizer,
        pass_names: List[str] = None,
    ) -> None:
        """Initialize analyzer.
        
        Args:
            optimizer: QubitPulseOpt GRAPE optimizer for fidelity computation.
            pass_names: List of optimization passes to apply.
                Default: ["cancellation", "rotation_merge"]
        """
        self._optimizer = optimizer
        self._pass_manager = _qopt.PassManager()
        
        pass_names = pass_names or ["cancellation", "rotation_merge"]
        for name in pass_names:
            self._pass_manager.add_pass(self._create_pass(name))
    
    def analyze_circuit(
        self,
        circuit: "_qopt.Circuit",
    ) -> Tuple[float, float, dict]:
        """Analyze fidelity before and after optimization.
        
        Args:
            circuit: Circuit to analyze.
        
        Returns:
            Tuple of (fidelity_before, fidelity_after, metrics).
        """
        # Compute fidelity before optimization
        gates_before = self._extract_gates(circuit)
        fidelity_before = self._compute_fidelity(gates_before)
        
        # Apply optimization
        optimized = circuit.copy()
        stats = self._pass_manager.run(optimized)
        
        # Compute fidelity after optimization
        gates_after = self._extract_gates(optimized)
        fidelity_after = self._compute_fidelity(gates_after)
        
        metrics = {
            "gates_before": len(gates_before),
            "gates_after": len(gates_after),
            "gate_reduction": 1 - len(gates_after) / len(gates_before),
            "fidelity_before": fidelity_before,
            "fidelity_after": fidelity_after,
            "fidelity_change": fidelity_after - fidelity_before,
            "optimization_time_ns": stats.total_time,
        }
        
        return fidelity_before, fidelity_after, metrics
```

### 5.2 Data Exchange Formats

#### 5.2.1 JSON Schema for Pulses

Use a consistent JSON schema for pulse data exchange:

```json
{
    "schema_version": "1.0",
    "metadata": {
        "created_at": "2025-01-15T10:30:00Z",
        "created_by": "QubitPulseOpt v1.0.0",
        "description": "Optimized X gate pulse"
    },
    "pulse": {
        "n_controls": 2,
        "n_timeslices": 100,
        "total_time": 10.0,
        "dt": 0.1,
        "amplitudes": [
            [0.1, 0.2, 0.3, ...],
            [0.0, 0.1, 0.2, ...]
        ],
        "control_names": ["x", "y"]
    },
    "result": {
        "target_gate": "X",
        "final_fidelity": 0.999998,
        "n_iterations": 342,
        "converged": true
    }
}
```

#### 5.2.2 HDF5 for Large Datasets

For large numerical data (benchmarks, training data):

```python
import h5py
import numpy as np

def save_benchmark_results(
    filepath: str,
    circuits: List[str],
    gate_counts: np.ndarray,
    fidelities: np.ndarray,
    metadata: dict,
) -> None:
    """Save benchmark results to HDF5.
    
    Structure:
        /metadata/          - Group with string attributes
        /circuits           - Dataset of circuit names
        /gate_counts        - Dataset (n_circuits, 2) [before, after]
        /fidelities         - Dataset (n_circuits, 2) [before, after]
    """
    with h5py.File(filepath, "w") as f:
        # Metadata as attributes
        meta = f.create_group("metadata")
        for key, value in metadata.items():
            meta.attrs[key] = value
        
        # Data
        f.create_dataset("circuits", data=np.array(circuits, dtype="S"))
        f.create_dataset("gate_counts", data=gate_counts)
        f.create_dataset("fidelities", data=fidelities)
```

#### 5.2.3 NumPy Binary (.npz) for Intermediate Data

```python
# Saving
np.savez_compressed(
    "optimized_pulses.npz",
    amplitudes=amplitudes,
    times=times,
    fidelity_history=fidelity_history,
)

# Loading
data = np.load("optimized_pulses.npz")
amplitudes = data["amplitudes"]
times = data["times"]
```

### 5.3 Error Handling Across Languages

#### 5.3.1 C++ Exceptions → Python Exceptions

pybind11 automatically translates standard C++ exceptions:

| C++ Exception | Python Exception |
|---------------|------------------|
| `std::runtime_error` | `RuntimeError` |
| `std::invalid_argument` | `ValueError` |
| `std::out_of_range` | `IndexError` |
| `std::bad_alloc` | `MemoryError` |

For custom exceptions:

```cpp
// C++ side
class OptimizationError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// In bindings
static py::exception<qopt::OptimizationError> exc(m, "OptimizationError");
py::register_exception_translator([](std::exception_ptr p) {
    try {
        if (p) std::rethrow_exception(p);
    } catch (const qopt::OptimizationError& e) {
        exc(e.what());
    }
});
```

---

## 6. Repository Standards

### 6.1 Git Workflow

#### 6.1.1 Branch Naming

| Branch Type | Pattern | Example |
|-------------|---------|---------|
| Main | `main` | `main` |
| Feature | `feat/<short-description>` | `feat/rotation-merge-pass` |
| Bug fix | `fix/<short-description>` | `fix/dag-cycle-detection` |
| Refactor | `refactor/<short-description>` | `refactor/raii-memory` |
| Documentation | `docs/<short-description>` | `docs/api-reference` |
| Experiment | `exp/<short-description>` | `exp/sabre-routing` |

#### 6.1.2 Commit Message Format (Conventional Commits)

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

**Types:**

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `style` | Formatting, no code change |
| `refactor` | Code change that neither fixes bug nor adds feature |
| `perf` | Performance improvement |
| `test` | Adding or correcting tests |
| `build` | Build system or dependencies |
| `ci` | CI/CD configuration |
| `chore` | Other changes (e.g., .gitignore) |

**Scope:** Component affected (e.g., `ir`, `passes`, `routing`, `grape`, `io`)

**Examples:**

```
feat(ir): add Gate factory methods for common gates

Add static factory methods (x, h, cnot, rz) to Gate class.
This provides a cleaner API than direct construction.

Closes #42

---

fix(passes): handle zero-angle rotations in RotationMerge

Previously, merging Rz(θ) with Rz(-θ) could produce Rz(0),
which should be eliminated. Now correctly removes identity gates.

---

refactor(simulator): replace raw pointers with CudaMemory<T>

Convert NoisySimulator and BatchedSimulator to use RAII
memory management. Eliminates manual cudaFree calls.

---

docs: add SPDX license headers to all source files

---

test(dag): add tests for topological sort with cycles
```

#### 6.1.3 .gitignore Template

```gitignore
# Build artifacts
build/
cmake-build-*/
*.o
*.a
*.so
*.dylib

# IDE/Editor
.vscode/
.idea/
*.swp
*.swo
*~
.cache/
compile_commands.json

# Python
__pycache__/
*.py[cod]
*$py.class
*.egg-info/
dist/
.eggs/
*.egg
.mypy_cache/
.pytest_cache/
.ruff_cache/
.coverage
htmlcov/
venv/
.venv/

# Jupyter
.ipynb_checkpoints/
*.ipynb_checkpoints

# Data (keep small test fixtures, ignore large data)
*.hdf5
*.h5
!tests/fixtures/*.hdf5
*.npz
!tests/fixtures/*.npz

# Secrets (NEVER commit these)
.env
.env.*
*.pem
*.key
credentials.json
secrets.yaml

# OS
.DS_Store
Thumbs.db

# Logs
*.log
logs/

# Temporary
tmp/
temp/
*.tmp
```

### 6.2 README Structure

Every project should have a README with these sections:

```markdown
# Project Name

One-line description of what the project does.

## Overview

2-3 paragraph description including:
- What problem this solves
- Key features
- Target audience

## Installation

### Prerequisites
- Required software/libraries
- Version requirements

### Quick Start
```bash
# Clone, build, run example
```

## Usage

### Basic Example
```python
# Minimal working example
```

### API Reference
Link to detailed docs or inline documentation.

## Project Structure

```
project/
├── src/          # Source code
├── tests/        # Test suite
└── docs/         # Documentation
```

## Development

### Building
```bash
# Build commands
```

### Testing
```bash
# Test commands
```

### Code Style
Link to coding standards.

## Citation

If you use this work, please cite:
```bibtex
@software{...}
```

## License

MIT License - see LICENSE file.

## Contact

- Author: Name <email>
- Issues: GitHub Issues link
```

### 6.3 LICENSE File

Use MIT License for all projects:

```
MIT License

Copyright (c) 2025 Rylan Malarchick

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 7. Security

### 7.1 Secrets Management

#### 7.1.1 Never Commit Secrets

The following should **never** be committed to version control:

| File Pattern | Description |
|--------------|-------------|
| `.env` | Environment variables |
| `.env.*` | Environment variants (.env.local, .env.prod) |
| `*.pem`, `*.key` | Private keys |
| `credentials.json` | API credentials |
| `secrets.yaml` | Secret configuration |
| `*_secret*` | Any file with "secret" in name |
| `*.token` | API tokens |
| `id_rsa*` | SSH keys |

#### 7.1.2 .gitignore Enforcement

Ensure `.gitignore` includes:

```gitignore
# Secrets - NEVER commit
.env
.env.*
*.pem
*.key
credentials.json
secrets.yaml
*_secret*
*.token
id_rsa*
config/local.yaml
config/production.yaml
```

#### 7.1.3 Pre-commit Hook for Secrets

Add to `.pre-commit-config.yaml`:

```yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.5.0
    hooks:
      - id: detect-private-key
      - id: detect-aws-credentials
      
  - repo: https://github.com/Yelp/detect-secrets
    rev: v1.4.0
    hooks:
      - id: detect-secrets
        args: ['--baseline', '.secrets.baseline']
```

#### 7.1.4 Environment Variables

Use environment variables for secrets:

```python
# Good: Load from environment
import os

API_KEY = os.environ.get("QUANTUM_API_KEY")
if not API_KEY:
    raise EnvironmentError("QUANTUM_API_KEY environment variable not set")

# Bad: Hardcoded secret
API_KEY = "sk-abc123..."  # NEVER DO THIS
```

```cpp
// Good: Load from environment
const char* apiKey = std::getenv("QUANTUM_API_KEY");
if (!apiKey) {
    throw std::runtime_error("QUANTUM_API_KEY environment variable not set");
}

// Bad: Hardcoded
const char* apiKey = "sk-abc123...";  // NEVER DO THIS
```

#### 7.1.5 Example .env.example

Provide a template without actual values:

```bash
# .env.example - Copy to .env and fill in values

# IQM Quantum Computer Access
IQM_SERVER_URL=https://your-iqm-instance.iqm.fi
IQM_CLIENT_ID=your-client-id
IQM_CLIENT_SECRET=  # Get from IQM console

# NASA Data Access
NASA_API_KEY=  # Get from api.nasa.gov

# Development
DEBUG=false
LOG_LEVEL=INFO
```

### 7.2 Dependency Security

#### 7.2.1 Pin Dependencies

Always pin exact versions in production:

```
# requirements.txt - Production
numpy==1.24.3
scipy==1.10.1
qutip==4.7.3
```

```
# requirements-dev.txt - Development (can use ranges)
pytest>=7.0,<8.0
black>=23.0
```

#### 7.2.2 Security Scanning

Add to CI/CD:

```yaml
# .github/workflows/security.yml
name: Security Scan

on: [push, pull_request]

jobs:
  security:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.10"
      
      - name: Install dependencies
        run: |
          pip install safety pip-audit bandit
      
      - name: Check for vulnerable dependencies (safety)
        run: safety check -r requirements.txt
      
      - name: Check for vulnerable dependencies (pip-audit)
        run: pip-audit -r requirements.txt
      
      - name: Static security analysis (bandit)
        run: bandit -r src/ -ll
```

#### 7.2.3 Dependabot Configuration

```yaml
# .github/dependabot.yml
version: 2
updates:
  - package-ecosystem: "pip"
    directory: "/"
    schedule:
      interval: "weekly"
    open-pull-requests-limit: 5
    
  - package-ecosystem: "github-actions"
    directory: "/"
    schedule:
      interval: "weekly"
```

### 7.3 Input Validation

#### 7.3.1 Validate All External Input

```python
def load_pulse_file(filepath: str) -> dict:
    """Load pulse data from JSON file.
    
    Args:
        filepath: Path to JSON file.
    
    Returns:
        Validated pulse data dictionary.
    
    Raises:
        ValueError: If file contains invalid data.
        FileNotFoundError: If file doesn't exist.
    """
    # Validate path
    path = Path(filepath)
    if not path.exists():
        raise FileNotFoundError(f"Pulse file not found: {filepath}")
    if not path.suffix == ".json":
        raise ValueError(f"Expected .json file, got: {path.suffix}")
    
    # Load and validate structure
    with open(path) as f:
        data = json.load(f)
    
    # Validate required fields
    required = ["schema_version", "pulse", "result"]
    missing = [k for k in required if k not in data]
    if missing:
        raise ValueError(f"Missing required fields: {missing}")
    
    # Validate schema version
    if data["schema_version"] != "1.0":
        raise ValueError(
            f"Unsupported schema version: {data['schema_version']}. "
            f"Expected: 1.0"
        )
    
    # Validate numerical data
    amplitudes = np.array(data["pulse"]["amplitudes"])
    if not np.all(np.isfinite(amplitudes)):
        raise ValueError("Pulse amplitudes contain NaN or Inf values")
    
    return data
```

#### 7.3.2 Sanitize Paths

```python
from pathlib import Path

def safe_join(base_dir: Path, user_path: str) -> Path:
    """Safely join paths, preventing directory traversal.
    
    Args:
        base_dir: Base directory (trusted).
        user_path: User-provided path component (untrusted).
    
    Returns:
        Safe joined path.
    
    Raises:
        ValueError: If path would escape base_dir.
    """
    # Resolve to absolute path
    base = base_dir.resolve()
    full = (base / user_path).resolve()
    
    # Check that result is under base
    if not str(full).startswith(str(base)):
        raise ValueError(f"Path traversal detected: {user_path}")
    
    return full
```

---

## 8. CI/CD Requirements

### 8.1 Required Checks

All pull requests must pass these checks before merge:

| Check | Description | Blocking |
|-------|-------------|----------|
| **Build** | Code compiles without errors | Yes |
| **Warnings** | Zero compiler/linter warnings | Yes |
| **Tests** | All tests pass | Yes |
| **Coverage** | Coverage doesn't decrease | Yes |
| **Lint** | Passes all linters | Yes |
| **Type check** | mypy passes (Python) | Yes |
| **Security** | No known vulnerabilities | Yes |
| **Format** | Code is properly formatted | Yes |

### 8.2 GitHub Actions Workflow Templates

#### 8.2.1 C++ Project Workflow

```yaml
# .github/workflows/cpp.yml
name: C++ CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    
    strategy:
      matrix:
        compiler: [gcc-11, clang-14]
        build_type: [Debug, Release]
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build
      
      - name: Configure
        run: |
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
            -DCMAKE_CXX_COMPILER=${{ matrix.compiler }}
      
      - name: Build
        run: cmake --build build
      
      - name: Test
        run: ctest --test-dir build --output-on-failure
      
      - name: Check for warnings
        run: |
          cmake --build build 2>&1 | tee build.log
          if grep -q "warning:" build.log; then
            echo "::error::Build produced warnings"
            exit 1
          fi

  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Run clang-format
        run: |
          find include src -name "*.hpp" -o -name "*.cpp" -o -name "*.cuh" -o -name "*.cu" | \
            xargs clang-format --dry-run --Werror
      
      - name: Run clang-tidy
        run: |
          cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
          clang-tidy -p build include/**/*.hpp src/**/*.cpp
```

#### 8.2.2 Python Project Workflow

```yaml
# .github/workflows/python.yml
name: Python CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
        python-version: ["3.9", "3.10", "3.11", "3.12"]
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python ${{ matrix.python-version }}
        uses: actions/setup-python@v5
        with:
          python-version: ${{ matrix.python-version }}
      
      - name: Install dependencies
        run: |
          python -m pip install --upgrade pip
          pip install -e ".[dev]"
      
      - name: Run tests with coverage
        run: |
          pytest --cov=src --cov-report=xml --cov-fail-under=70
      
      - name: Upload coverage
        uses: codecov/codecov-action@v3
        with:
          files: coverage.xml

  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.10"
      
      - name: Install linters
        run: pip install black isort ruff mypy
      
      - name: Check formatting (black)
        run: black --check .
      
      - name: Check imports (isort)
        run: isort --check .
      
      - name: Lint (ruff)
        run: ruff check .
      
      - name: Type check (mypy)
        run: mypy src/

  security:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.10"
      
      - name: Security scan
        run: |
          pip install safety bandit
          safety check -r requirements.txt
          bandit -r src/ -ll
```

#### 8.2.3 CUDA Project Workflow

```yaml
# .github/workflows/cuda.yml
name: CUDA CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  build:
    runs-on: ubuntu-latest
    container: nvidia/cuda:11.8.0-devel-ubuntu22.04
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Install dependencies
        run: |
          apt-get update
          apt-get install -y cmake ninja-build git
      
      - name: Configure
        run: |
          cmake -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_CUDA_ARCHITECTURES=70
      
      - name: Build
        run: cmake --build build
      
      - name: Run CPU tests (no GPU in CI)
        run: |
          ctest --test-dir build --output-on-failure \
            -E "gpu|cuda"  # Exclude GPU-requiring tests
```

### 8.3 Branch Protection Rules

Configure on GitHub:

| Rule | Setting |
|------|---------|
| Require pull request reviews | 0 (solo project) or 1+ (team) |
| Require status checks | All CI jobs |
| Require branches to be up to date | Yes |
| Require signed commits | Optional |
| Include administrators | Yes |
| Allow force pushes | No |
| Allow deletions | No |

### 8.4 Build Badge

Add to README:

```markdown
![CI](https://github.com/rylanmalarchick/project-name/workflows/CI/badge.svg)
![Coverage](https://codecov.io/gh/rylanmalarchick/project-name/branch/main/graph/badge.svg)
![License](https://img.shields.io/github/license/rylanmalarchick/project-name)
```

---

## 9. Project-Specific Notes

### 9.1 CUDA Quantum Simulator

**Repository:** `quantum/compilers/cuda-quantum-simulator/`

#### 9.1.1 CUDA-Specific Conventions

| Convention | Description |
|------------|-------------|
| File extensions | `.cuh` for CUDA headers, `.cu` for CUDA source |
| Device pointers | Prefix with `d_` (e.g., `d_state_`, `d_rngStates_`) |
| Kernel naming | `apply*` for gate kernels, `dm*` for density matrix ops |
| Optimized variants | Suffix with `_opt`, `_shared`, `_coalesced` |
| Block size | `constexpr int BLOCK_SIZE = 256;` |
| Error checking | Always use `CUDA_CHECK()` and `CUDA_CHECK_LAST_ERROR()` |

#### 9.1.2 Memory Management

Use the `CudaMemory<T>` RAII wrapper:

```cpp
// Instead of:
cuDoubleComplex* d_state;
cudaMalloc(&d_state, size * sizeof(cuDoubleComplex));
// ... use d_state ...
cudaFree(d_state);  // Easy to forget!

// Use:
CudaMemory<cuDoubleComplex> d_state(size);
// Automatically freed when out of scope
```

#### 9.1.3 Namespace

All code in `qsim` namespace with sub-namespaces:

```cpp
namespace qsim {
namespace constants { ... }
namespace cuda_config { ... }
class StateVector { ... };
class Circuit { ... };
}
```

### 9.2 Quantum Circuit Optimizer

**Repository:** `quantum/compilers/quantum-circuit-optimizer/`

#### 9.2.1 Namespace Structure

```cpp
namespace qopt {
namespace ir { class Gate, Circuit, DAG; }
namespace passes { class Pass, PassManager, CancellationPass; }
namespace routing { class CouplingMap, GreedyRouter; }
namespace analysis { struct Metrics; }
}
```

#### 9.2.2 Gate Factory Pattern

Use static factory methods instead of constructors:

```cpp
// Preferred: Factory methods
auto x = Gate::x(0);
auto cnot = Gate::cnot(0, 1);
auto rz = Gate::rz(0, M_PI / 4);

// Avoid: Direct construction
Gate g(GateType::X, {0}, std::nullopt);  // Less readable
```

#### 9.2.3 Pass Interface

All optimization passes inherit from `Pass`:

```cpp
class Pass {
public:
    virtual ~Pass() noexcept = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual bool run(ir::Circuit& circuit) = 0;
};
```

### 9.3 QubitPulseOpt

**Repository:** `quantum/Controls/QubitPulseOpt/`

#### 9.3.1 Power of 10 Compliance

This project adapts NASA JPL Power of 10 rules for Python:

| Rule | Implementation |
|------|----------------|
| Bounded loops | `MAX_ITERATIONS`, `MAX_TIMESLICES` constants |
| Function length | ≤60 lines, helper methods for decomposition |
| Assertion density | Validation at public API boundaries |
| Minimal scope | Local variables, explicit data flow |
| Return value checks | All inputs/outputs validated |

Constants defined in `src/constants.py`:

```python
MAX_ITERATIONS = 10000
MAX_TIMESLICES = 100000
MAX_CONTROL_HAMILTONIANS = 100
MAX_PARAMS = 10000
```

#### 9.3.2 Docstring Style

Uses **NumPy-style docstrings** (configured in `.flake8`):

```python
def optimize_unitary(self, target: np.ndarray) -> GRAPEResult:
    """
    Optimize pulses for target unitary.
    
    Parameters
    ----------
    target : np.ndarray
        Target unitary matrix.
    
    Returns
    -------
    GRAPEResult
        Optimization result.
    """
```

#### 9.3.3 Dataclass Pattern

All result containers use `@dataclass`:

```python
@dataclass
class GRAPEResult:
    final_fidelity: float
    optimized_pulses: np.ndarray
    fidelity_history: List[float]
    n_iterations: int
    converged: bool
    message: str
```

### 9.4 QuantumVQE

**Repository:** `quantum/HPC/QuantumVQE/`

#### 9.4.1 MPI/CUDA Considerations

- Use `MPI_Comm_rank` to identify process
- Ensure CUDA operations are GPU-aware MPI compatible
- Log with rank prefix: `[Rank 0] Message`

#### 9.4.2 Benchmark Data Format

Results stored in JSON with consistent schema:

```json
{
    "experiment": "vqe_benchmark",
    "timestamp": "2025-01-15T10:30:00Z",
    "parameters": {
        "n_qubits": 12,
        "n_layers": 4,
        "optimizer": "COBYLA"
    },
    "results": {
        "final_energy": -1.234,
        "n_iterations": 150,
        "wall_time_seconds": 45.2
    },
    "hardware": {
        "n_gpus": 4,
        "gpu_model": "A100",
        "n_mpi_ranks": 4
    }
}
```

### 9.5 CloudML

**Repository:** `NASA/cloudML/`

#### 9.5.1 Production vs Research Code

| Directory | Purpose | Standards |
|-----------|---------|-----------|
| `src/cbh_retrieval/` | Production code | Full standards, 90%+ coverage |
| `experiments/` | Research experiments | Relaxed, documented |
| `archive/` | Historical code | Preserved for reproducibility |

#### 9.5.2 Docstring Style

Uses **Google-style docstrings**:

```python
def train_model(X: np.ndarray, y: np.ndarray) -> Model:
    """Train production GBDT model.
    
    Args:
        X: Feature matrix.
        y: Target values.
    
    Returns:
        Trained model.
    """
```

#### 9.5.3 Data Provenance

All data files must have provenance metadata:

```python
metadata = {
    "source": "NASA ER-2 WHYMSIE 2024 campaign",
    "processing_date": "2025-01-15",
    "processing_script": "scripts/preprocess_cpl.py",
    "git_commit": "abc123",
    "contact": "rylan1012@gmail.com",
}
```

---

## Appendix A: Quick Reference Card

### A.1 C++ Quick Reference

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        C++ CODING STANDARDS                              │
├─────────────────────────────────────────────────────────────────────────┤
│ NAMING                                                                   │
│   namespace qopt::ir        │ lowercase, short                          │
│   class Circuit             │ PascalCase                                │
│   void addGate()            │ camelCase                                 │
│   size_t numQubits_         │ trailing underscore for private members   │
│   constexpr MAX_QUBITS      │ UPPER_CASE in namespace                   │
│   enum class GateType       │ PascalCase                                │
│   cuDoubleComplex* d_state_ │ d_ prefix for device pointers             │
├─────────────────────────────────────────────────────────────────────────┤
│ FILE HEADER                                                              │
│   // SPDX-License-Identifier: MIT                                       │
│   // Copyright (c) 2025 Rylan Malarchick                                │
│   /**                                                                    │
│    * @file Name.hpp                                                      │
│    * @brief One-line description                                         │
│    */                                                                    │
│   #pragma once                                                           │
├─────────────────────────────────────────────────────────────────────────┤
│ INCLUDE ORDER                                                            │
│   1. Own header (in .cpp)                                               │
│   2. Project headers (alphabetized)                                     │
│   3. Third-party headers (alphabetized)                                 │
│   4. Standard library (alphabetized)                                    │
├─────────────────────────────────────────────────────────────────────────┤
│ MODERN C++                                                               │
│   [[nodiscard]]  │ Functions returning values that must be used         │
│   constexpr      │ Compile-time constants                               │
│   noexcept       │ Destructors, move ops, simple getters                │
│   std::optional  │ Nullable returns                                     │
│   emplace_back   │ In-place container construction                      │
├─────────────────────────────────────────────────────────────────────────┤
│ MEMORY                                                                   │
│   std::unique_ptr<T>  │ Sole ownership (most common)                    │
│   CudaMemory<T>       │ RAII for GPU memory                             │
│   Rule of 5           │ Delete copy, enable move for resources          │
├─────────────────────────────────────────────────────────────────────────┤
│ CMAKE                                                                    │
│   -Wall -Wextra -Wpedantic -Werror                                      │
│   set(CMAKE_CXX_STANDARD 17)                                            │
│   FetchContent for GoogleTest                                           │
└─────────────────────────────────────────────────────────────────────────┘
```

### A.2 Python Quick Reference

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       PYTHON CODING STANDARDS                            │
├─────────────────────────────────────────────────────────────────────────┤
│ NAMING                                                                   │
│   module_name.py            │ snake_case                                │
│   class ClassName           │ PascalCase                                │
│   def function_name()       │ snake_case                                │
│   def _private_method()     │ leading underscore                        │
│   CONSTANT_VALUE            │ UPPER_SNAKE_CASE                          │
├─────────────────────────────────────────────────────────────────────────┤
│ DOCSTRINGS (NumPy or Google style)                                      │
│   """                                                                    │
│   Brief description.                                                     │
│                                                                          │
│   Parameters                 │ Args:                                    │
│   ----------                 │     param: Description.                  │
│   param : type               │                                          │
│       Description.           │ Returns:                                 │
│                              │     Description.                         │
│   Returns                    │                                          │
│   -------                    │ Raises:                                  │
│   type                       │     ExceptionType: When raised.          │
│       Description.           │                                          │
│   """                                                                    │
├─────────────────────────────────────────────────────────────────────────┤
│ TYPE HINTS                                                               │
│   def func(x: int, y: str = "default") -> List[int]:                    │
│   from typing import Dict, List, Optional, Tuple                        │
├─────────────────────────────────────────────────────────────────────────┤
│ TOOLING                                                                  │
│   black          │ Formatting (line-length = 100)                       │
│   isort          │ Import sorting (profile = "black")                   │
│   ruff           │ Linting                                              │
│   mypy           │ Type checking                                        │
│   pytest         │ Testing                                              │
├─────────────────────────────────────────────────────────────────────────┤
│ TESTING                                                                  │
│   @pytest.mark.slow          │ Mark slow tests                          │
│   @pytest.mark.stochastic    │ Tests with randomness                    │
│   @pytest.fixture            │ Shared test setup                        │
│   pytest --cov=src           │ Coverage reporting                       │
└─────────────────────────────────────────────────────────────────────────┘
```

### A.3 Git Quick Reference

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         GIT CONVENTIONS                                  │
├─────────────────────────────────────────────────────────────────────────┤
│ COMMIT FORMAT (Conventional Commits)                                    │
│   <type>(<scope>): <description>                                        │
│                                                                          │
│   Types: feat, fix, docs, style, refactor, perf, test, build, ci, chore│
│                                                                          │
│   Examples:                                                              │
│     feat(ir): add Gate factory methods                                  │
│     fix(passes): handle zero-angle rotations                            │
│     docs: add SPDX headers to all files                                 │
│     refactor(simulator): use CudaMemory<T> RAII                         │
├─────────────────────────────────────────────────────────────────────────┤
│ BRANCH NAMING                                                            │
│   feat/<description>     │ New feature                                  │
│   fix/<description>      │ Bug fix                                      │
│   refactor/<description> │ Code refactoring                             │
│   docs/<description>     │ Documentation                                │
│   exp/<description>      │ Experiment                                   │
├─────────────────────────────────────────────────────────────────────────┤
│ NEVER COMMIT                                                             │
│   .env, *.pem, *.key, credentials.json, secrets.yaml, *_secret*        │
└─────────────────────────────────────────────────────────────────────────┘
```

### A.4 Security Checklist

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       SECURITY CHECKLIST                                 │
├─────────────────────────────────────────────────────────────────────────┤
│ BEFORE EVERY COMMIT                                                      │
│   [ ] No secrets in code (API keys, passwords, tokens)                  │
│   [ ] No hardcoded credentials                                          │
│   [ ] .env files in .gitignore                                          │
│   [ ] Private keys not committed                                         │
├─────────────────────────────────────────────────────────────────────────┤
│ BEFORE EVERY PR                                                          │
│   [ ] Dependencies pinned to specific versions                          │
│   [ ] No known vulnerabilities (safety/pip-audit)                       │
│   [ ] Input validation on external data                                 │
│   [ ] Path traversal checks on file operations                          │
├─────────────────────────────────────────────────────────────────────────┤
│ CI/CD REQUIREMENTS                                                       │
│   [ ] All tests pass                                                     │
│   [ ] Zero warnings                                                      │
│   [ ] Coverage doesn't decrease                                          │
│   [ ] Security scans pass                                                │
│   [ ] Linting passes                                                     │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Appendix B: File Templates

### B.1 C++ Header Template

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file ClassName.hpp
 * @brief One-line description of this file
 * @author Rylan Malarchick
 * @date 2025
 *
 * Extended description explaining what this file implements,
 * how it fits into the architecture, and any key algorithms.
 *
 * @see RelatedClass.hpp
 *
 * @references
 * - Reference 1 with citation
 * - Reference 2 with DOI
 */
#pragma once

#include "ProjectHeader.hpp"

#include <optional>
#include <string>
#include <vector>

namespace project::subnamespace {

/**
 * @brief Brief class description.
 *
 * Extended description of the class purpose, usage patterns,
 * and any important invariants.
 *
 * Thread safety: [Thread-safe / Not thread-safe / Conditionally thread-safe]
 *
 * Example:
 * @code
 * ClassName obj(args);
 * obj.doSomething();
 * @endcode
 */
class ClassName {
public:
    // ========================================================================
    // Construction / Destruction
    // ========================================================================
    
    /**
     * @brief Construct a ClassName.
     * @param param1 Description of parameter
     * @throws std::invalid_argument if param1 is invalid
     */
    explicit ClassName(int param1);
    
    ~ClassName() noexcept = default;
    
    // Disable copy (if managing resources)
    ClassName(const ClassName&) = delete;
    ClassName& operator=(const ClassName&) = delete;
    
    // Enable move
    ClassName(ClassName&&) noexcept = default;
    ClassName& operator=(ClassName&&) noexcept = default;
    
    // ========================================================================
    // Public Interface
    // ========================================================================
    
    /**
     * @brief Brief description of method.
     * @param arg Description
     * @return Description of return value
     * @throws ExceptionType when condition
     */
    [[nodiscard]] ReturnType methodName(ArgType arg) const;
    
    // ========================================================================
    // Accessors
    // ========================================================================
    
    [[nodiscard]] int value() const noexcept { return value_; }

private:
    // ========================================================================
    // Private Implementation
    // ========================================================================
    
    void helperMethod();
    
    // ========================================================================
    // Data Members
    // ========================================================================
    
    int value_;
    std::vector<int> data_;
};

// ============================================================================
// Non-member Functions
// ============================================================================

/**
 * @brief Convert to string representation.
 */
[[nodiscard]] std::string toString(const ClassName& obj);

}  // namespace project::subnamespace
```

### B.2 C++ Source Template

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file ClassName.cpp
 * @brief Implementation of ClassName
 */

#include "project/subnamespace/ClassName.hpp"

#include "project/OtherHeader.hpp"

#include <algorithm>
#include <stdexcept>

namespace project::subnamespace {

// ============================================================================
// Construction
// ============================================================================

ClassName::ClassName(int param1)
    : value_(param1)
    , data_()
{
    if (param1 < 0) {
        throw std::invalid_argument(
            "param1 must be non-negative, got: " + std::to_string(param1)
        );
    }
}

// ============================================================================
// Public Interface
// ============================================================================

ReturnType ClassName::methodName(ArgType arg) const {
    // Implementation
}

// ============================================================================
// Private Implementation
// ============================================================================

void ClassName::helperMethod() {
    // Implementation
}

// ============================================================================
// Non-member Functions
// ============================================================================

std::string toString(const ClassName& obj) {
    return "ClassName(" + std::to_string(obj.value()) + ")";
}

}  // namespace project::subnamespace
```

### B.3 C++ Test Template

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_classname.cpp
 * @brief Unit tests for ClassName
 */

#include "project/subnamespace/ClassName.hpp"

#include <gtest/gtest.h>

namespace project::subnamespace {
namespace {

// ============================================================================
// Test Fixture
// ============================================================================

class ClassNameTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup
    }
    
    void TearDown() override {
        // Common cleanup
    }
    
    // Helper methods
    void expectValid(const ClassName& obj) {
        EXPECT_GE(obj.value(), 0);
    }
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(ClassNameTest, Constructor_ValidParam_CreatesObject) {
    ClassName obj(42);
    EXPECT_EQ(obj.value(), 42);
}

TEST_F(ClassNameTest, Constructor_NegativeParam_Throws) {
    EXPECT_THROW(ClassName(-1), std::invalid_argument);
}

// ============================================================================
// Method Tests
// ============================================================================

TEST_F(ClassNameTest, MethodName_ValidInput_ReturnsExpected) {
    ClassName obj(10);
    auto result = obj.methodName(arg);
    EXPECT_EQ(result, expected);
}

TEST_F(ClassNameTest, MethodName_EdgeCase_HandlesCorrectly) {
    // Test edge cases
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(ClassNameTest, MethodName_InvalidInput_Throws) {
    ClassName obj(10);
    EXPECT_THROW(obj.methodName(invalidArg), std::invalid_argument);
}

}  // namespace
}  // namespace project::subnamespace
```

### B.4 Python Module Template

```python
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Rylan Malarchick

"""
Module brief description.

Extended description of the module's purpose, what it provides,
and how it fits into the larger package.

Key components:
    - ClassName: Brief description
    - function_name: Brief description

Example:
    >>> from package import ClassName
    >>> obj = ClassName(param)
    >>> result = obj.method()

See Also:
    - related_module: Description
    - other_module: Description

Author: Rylan Malarchick
Date: 2025
"""

from __future__ import annotations

import logging
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING, Any, Dict, List, Optional, Tuple

import numpy as np

if TYPE_CHECKING:
    from .other_module import OtherClass

__all__ = ["ClassName", "function_name", "CONSTANT"]

logger = logging.getLogger(__name__)

# =============================================================================
# Constants
# =============================================================================

CONSTANT: int = 42
"""Brief description of constant."""

MAX_ITERATIONS: int = 10000
"""Maximum number of iterations for optimization."""


# =============================================================================
# Data Classes
# =============================================================================

@dataclass
class ResultClass:
    """Container for results.
    
    Attributes:
        value: Description of value.
        data: Description of data.
        metadata: Additional metadata.
    """
    
    value: float
    data: np.ndarray
    metadata: Dict[str, Any] = field(default_factory=dict)


# =============================================================================
# Main Classes
# =============================================================================

class ClassName:
    """Brief class description.
    
    Extended description of the class purpose, invariants,
    and usage patterns.
    
    Attributes:
        param: Description of param.
    
    Example:
        >>> obj = ClassName(param=10)
        >>> result = obj.method(arg)
        >>> print(result.value)
        42.0
    """
    
    def __init__(self, param: int) -> None:
        """Initialize ClassName.
        
        Args:
            param: Description of parameter.
        
        Raises:
            ValueError: If param is negative.
        """
        if param < 0:
            raise ValueError(f"param must be non-negative, got: {param}")
        
        self._param = param
        self._data: List[float] = []
    
    @property
    def param(self) -> int:
        """The param value."""
        return self._param
    
    def method(self, arg: float) -> ResultClass:
        """Brief method description.
        
        Extended description if needed.
        
        Args:
            arg: Description of argument.
        
        Returns:
            ResultClass containing the computed result.
        
        Raises:
            ValueError: If arg is not finite.
        
        Example:
            >>> obj = ClassName(10)
            >>> result = obj.method(3.14)
        """
        if not np.isfinite(arg):
            raise ValueError(f"arg must be finite, got: {arg}")
        
        # Implementation
        value = self._compute(arg)
        
        return ResultClass(
            value=value,
            data=np.array(self._data),
        )
    
    def _compute(self, arg: float) -> float:
        """Private helper method.
        
        Args:
            arg: Input value.
        
        Returns:
            Computed result.
        """
        return arg * self._param


# =============================================================================
# Functions
# =============================================================================

def function_name(
    param1: np.ndarray,
    param2: Optional[str] = None,
    *,
    keyword_only: bool = False,
) -> Tuple[float, np.ndarray]:
    """Brief function description.
    
    Extended description if needed.
    
    Args:
        param1: Description of param1.
        param2: Description of param2. Defaults to None.
        keyword_only: Description. Defaults to False.
    
    Returns:
        A tuple containing:
            - value: Description of first element.
            - array: Description of second element.
    
    Raises:
        ValueError: If param1 is empty.
    
    Example:
        >>> result = function_name(np.array([1, 2, 3]))
        >>> print(result[0])
        2.0
    """
    if param1.size == 0:
        raise ValueError("param1 must not be empty")
    
    value = float(np.mean(param1))
    array = param1 * 2
    
    return value, array
```

### B.5 Python Test Template

```python
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Rylan Malarchick

"""Unit tests for module_name."""

from __future__ import annotations

import numpy as np
import pytest

from package.module_name import ClassName, ResultClass, function_name


# =============================================================================
# Fixtures
# =============================================================================

@pytest.fixture
def sample_instance() -> ClassName:
    """Create a sample ClassName instance for testing."""
    return ClassName(param=10)


@pytest.fixture
def sample_array() -> np.ndarray:
    """Create a sample array for testing."""
    return np.array([1.0, 2.0, 3.0, 4.0, 5.0])


# =============================================================================
# ClassName Tests
# =============================================================================

class TestClassName:
    """Tests for ClassName."""
    
    # -------------------------------------------------------------------------
    # Construction Tests
    # -------------------------------------------------------------------------
    
    def test_init_valid_param_creates_instance(self) -> None:
        """ClassName should initialize with valid parameter."""
        obj = ClassName(param=42)
        assert obj.param == 42
    
    def test_init_negative_param_raises_value_error(self) -> None:
        """ClassName should raise ValueError for negative param."""
        with pytest.raises(ValueError, match="must be non-negative"):
            ClassName(param=-1)
    
    # -------------------------------------------------------------------------
    # Method Tests
    # -------------------------------------------------------------------------
    
    def test_method_valid_input_returns_result(
        self,
        sample_instance: ClassName,
    ) -> None:
        """method should return ResultClass for valid input."""
        result = sample_instance.method(arg=3.14)
        
        assert isinstance(result, ResultClass)
        assert result.value == pytest.approx(31.4)
    
    def test_method_nan_input_raises_value_error(
        self,
        sample_instance: ClassName,
    ) -> None:
        """method should raise ValueError for NaN input."""
        with pytest.raises(ValueError, match="must be finite"):
            sample_instance.method(arg=float("nan"))
    
    @pytest.mark.parametrize(
        "arg,expected",
        [
            (0.0, 0.0),
            (1.0, 10.0),
            (-1.0, -10.0),
        ],
    )
    def test_method_various_inputs(
        self,
        sample_instance: ClassName,
        arg: float,
        expected: float,
    ) -> None:
        """method should handle various input values."""
        result = sample_instance.method(arg=arg)
        assert result.value == pytest.approx(expected)


# =============================================================================
# function_name Tests
# =============================================================================

class TestFunctionName:
    """Tests for function_name."""
    
    def test_valid_input_returns_tuple(self, sample_array: np.ndarray) -> None:
        """function_name should return tuple for valid input."""
        value, array = function_name(sample_array)
        
        assert value == pytest.approx(3.0)
        np.testing.assert_array_equal(array, sample_array * 2)
    
    def test_empty_array_raises_value_error(self) -> None:
        """function_name should raise ValueError for empty array."""
        with pytest.raises(ValueError, match="must not be empty"):
            function_name(np.array([]))
    
    @pytest.mark.slow
    def test_large_array_performance(self) -> None:
        """function_name should handle large arrays efficiently."""
        large_array = np.random.randn(1_000_000)
        value, array = function_name(large_array)
        
        assert np.isfinite(value)
        assert array.shape == large_array.shape
```

### B.6 CMakeLists.txt Template

```cmake
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Rylan Malarchick

cmake_minimum_required(VERSION 3.18)

project(project-name
    VERSION 1.0.0
    DESCRIPTION "Brief project description"
    LANGUAGES CXX
)

# =============================================================================
# Options
# =============================================================================

option(BUILD_TESTS "Build test suite" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(ENABLE_CUDA "Enable CUDA support" OFF)

# =============================================================================
# Standards
# =============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Compiler Warnings
# =============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast
        -Wcast-align -Wunused -Woverloaded-virtual
        -Wformat=2
    )
    
    # Treat warnings as errors in CI
    if(DEFINED ENV{CI})
        add_compile_options(-Werror)
    endif()
endif()

# =============================================================================
# Library
# =============================================================================

add_library(${PROJECT_NAME}_lib
    src/module1.cpp
    src/module2.cpp
)

target_include_directories(${PROJECT_NAME}_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Alias for use in subdirectories
add_library(${PROJECT_NAME}::lib ALIAS ${PROJECT_NAME}_lib)

# =============================================================================
# Executable
# =============================================================================

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}::lib)

# =============================================================================
# Testing
# =============================================================================

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Prevent overriding parent project's compiler/linker settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    
    add_executable(${PROJECT_NAME}_tests
        tests/test_module1.cpp
        tests/test_module2.cpp
    )
    
    target_link_libraries(${PROJECT_NAME}_tests
        PRIVATE
            ${PROJECT_NAME}::lib
            GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
endif()

# =============================================================================
# Installation
# =============================================================================

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}_lib ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
```

### B.7 pyproject.toml Template

```toml
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Rylan Malarchick

[build-system]
requires = ["setuptools>=61.0", "wheel"]
build-backend = "setuptools.build_meta"

[project]
name = "package-name"
version = "1.0.0"
description = "Brief package description"
readme = "README.md"
license = {text = "MIT"}
authors = [
    {name = "Rylan Malarchick", email = "rylan1012@gmail.com"},
]
requires-python = ">=3.9"
classifiers = [
    "Development Status :: 4 - Beta",
    "Intended Audience :: Science/Research",
    "License :: OSI Approved :: MIT License",
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
    "Programming Language :: Python :: 3.12",
    "Topic :: Scientific/Engineering :: Physics",
]
keywords = ["quantum", "computing", "optimization"]
dependencies = [
    "numpy>=1.24",
    "scipy>=1.10",
]

[project.optional-dependencies]
dev = [
    "pytest>=7.0",
    "pytest-cov>=4.0",
    "black>=23.0",
    "isort>=5.12",
    "ruff>=0.1.0",
    "mypy>=1.0",
    "pre-commit>=3.0",
]
docs = [
    "sphinx>=6.0",
    "sphinx-rtd-theme>=1.0",
]

[project.urls]
Homepage = "https://github.com/rylanmalarchick/package-name"
Documentation = "https://package-name.readthedocs.io"
Repository = "https://github.com/rylanmalarchick/package-name"
Issues = "https://github.com/rylanmalarchick/package-name/issues"

[tool.setuptools.packages.find]
where = ["src"]

# =============================================================================
# Black
# =============================================================================

[tool.black]
line-length = 100
target-version = ["py310"]

# =============================================================================
# isort
# =============================================================================

[tool.isort]
profile = "black"
line_length = 100
known_first_party = ["package_name"]

# =============================================================================
# Ruff
# =============================================================================

[tool.ruff]
line-length = 100
target-version = "py310"

[tool.ruff.lint]
select = [
    "E", "W",      # pycodestyle
    "F",           # Pyflakes
    "I",           # isort
    "N",           # pep8-naming
    "D",           # pydocstyle
    "UP",          # pyupgrade
    "ANN",         # flake8-annotations
    "S",           # flake8-bandit
    "B",           # flake8-bugbear
    "C4",          # flake8-comprehensions
    "PT",          # flake8-pytest-style
    "RUF",         # Ruff-specific
]
ignore = [
    "D100",        # Missing docstring in public module
    "D104",        # Missing docstring in public package
    "ANN101",      # Missing type annotation for self
    "ANN102",      # Missing type annotation for cls
]

[tool.ruff.lint.pydocstyle]
convention = "numpy"

# =============================================================================
# mypy
# =============================================================================

[tool.mypy]
python_version = "3.10"
warn_return_any = true
warn_unused_ignores = true
no_implicit_optional = true
check_untyped_defs = true

[[tool.mypy.overrides]]
module = "tests.*"
disallow_untyped_defs = false

# =============================================================================
# pytest
# =============================================================================

[tool.pytest.ini_options]
minversion = "7.0"
testpaths = ["tests"]
python_files = ["test_*.py"]
addopts = [
    "-v",
    "--strict-markers",
    "-ra",
    "--tb=short",
]
markers = [
    "slow: marks tests as slow",
    "integration: marks integration tests",
    "stochastic: marks tests with inherent randomness",
]

# =============================================================================
# coverage
# =============================================================================

[tool.coverage.run]
source = ["src"]
branch = true

[tool.coverage.report]
precision = 2
show_missing = true
exclude_lines = [
    "pragma: no cover",
    "if TYPE_CHECKING:",
    "@abstractmethod",
]
```

### B.8 GitHub Actions Workflow Template

```yaml
# .github/workflows/ci.yml
# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Rylan Malarchick

name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  # ===========================================================================
  # Test Job
  # ===========================================================================
  test:
    name: Test (Python ${{ matrix.python-version }}, ${{ matrix.os }})
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest]
        python-version: ["3.9", "3.10", "3.11", "3.12"]
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python ${{ matrix.python-version }}
        uses: actions/setup-python@v5
        with:
          python-version: ${{ matrix.python-version }}
          cache: "pip"
      
      - name: Install dependencies
        run: |
          python -m pip install --upgrade pip
          pip install -e ".[dev]"
      
      - name: Run tests
        run: pytest --cov=src --cov-report=xml --cov-fail-under=70
      
      - name: Upload coverage
        if: matrix.os == 'ubuntu-latest' && matrix.python-version == '3.10'
        uses: codecov/codecov-action@v3
        with:
          files: coverage.xml

  # ===========================================================================
  # Lint Job
  # ===========================================================================
  lint:
    name: Lint
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.10"
          cache: "pip"
      
      - name: Install linters
        run: pip install black isort ruff mypy
      
      - name: Check formatting (black)
        run: black --check .
      
      - name: Check imports (isort)
        run: isort --check .
      
      - name: Lint (ruff)
        run: ruff check .
      
      - name: Type check (mypy)
        run: mypy src/

  # ===========================================================================
  # Security Job
  # ===========================================================================
  security:
    name: Security
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.10"
      
      - name: Install security tools
        run: pip install safety bandit pip-audit
      
      - name: Check dependencies (safety)
        run: safety check -r requirements.txt || true
      
      - name: Check dependencies (pip-audit)
        run: pip-audit -r requirements.txt || true
      
      - name: Static analysis (bandit)
        run: bandit -r src/ -ll
```
