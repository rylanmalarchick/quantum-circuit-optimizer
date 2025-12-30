# Research Code Principles

**Author:** Rylan Malarchick  
**Version:** 2.1  
**Last Updated:** December 2025  
**Scope:** Quantum computing, HPC, and ML research software

---

## Related Documents

| Document | Purpose |
|----------|---------|
| [agent-coding-context.md](agent-coding-context.md) | Quick reference for AI agents |
| [research-code-principles.md](research-code-principles.md) | Philosophy and principles |
| [style-guide-reference.md](style-guide-reference.md) | Detailed style conventions |
| [repo-standards.md](repo-standards.md) | Git workflow and repository hygiene |

---

## Preamble: Why This Matters

Research code has a reputation problem. Scientists write code that "works" for a single experiment, then move on. No tests. No validation. No thought about whether results are reproducible. This is not acceptable.

**Your code is your experiment.** If your code is wrong, your science is wrong. If your code can't be reproduced, your results can't be trusted. If your code breaks when someone changes one line, you never understood what it did in the first place.

This document exists because:

1. **Physical accuracy matters**: Quantum gates must match their mathematical definitions. ML metrics must be calculated correctly. Numerical stability matters when you're simulating physics.

2. **Reproducibility is non-negotiable**: Research that can't be reproduced isn't research. Seeds must be set. Floating-point behavior must be understood. Results must be deterministic.

3. **Future you will need to debug this**: Code written once is read dozens of times. By you. By collaborators. By reviewers. By the poor PhD student who inherits your project.

4. **AI agents need guidance**: You're pairing with AI to write code. Without principles, agents optimize for "code that runs" not "code that's correct." They skip tests. They miss edge cases. They build on sand.

This document is not a style guide. It's a **philosophy** for building research software that meets the same standards as production code, because research code *is* production code—it produces results that matter.

---

## Core Principles

These five principles are affirmative: they tell you what good code *is*, not what bad code *isn't*. They apply across C++, Python, CUDA, and any language you use. They're the foundation; the rest of this document shows how they manifest in practice.

### 1. Correctness First

**Physical accuracy and mathematical correctness are non-negotiable.**

Research code must be *right* before it's fast, flexible, or elegant. A quantum gate that achieves 99.9% fidelity is worthless if the gate definition is wrong. An ML model with high accuracy is meaningless if the metric is miscalculated.

**What this means:**
- Validate against primary sources: papers, textbooks, known analytical results
- Unit test against ground truth (e.g., Pauli X gate should satisfy X² = I)
- Check physical constraints (unitaries must satisfy U†U = I, probabilities sum to 1)
- Prefer numerical stability over performance (use stable matrix decompositions, avoid subtracting nearly-equal numbers)
- When results disagree with expectations, assume your code is wrong—not physics

**Why agents get this wrong:**  
Agents optimize for "code compiles and runs" not "code produces correct results." They'll happily implement a rotation gate with the wrong sign, or calculate fidelity without normalization, because the code *works*—it just doesn't work *correctly*.

**How to recognize you're on track:**  
Every non-trivial calculation has a test comparing against known values. Every physical quantity has bounds checking. Every equation references the paper/textbook it came from.

### 2. Specification Before Code

**You can't write code until you define what "correct" means.**

Tests are not something you "add later." Tests *are* your specification. They define the contract: given these inputs, produce these outputs. Writing tests first forces you to think about:
- What should happen in the happy path?
- What should happen at boundaries (empty input, single element, maximum size)?
- What should happen when inputs are invalid (NaN, negative probabilities, non-unitary matrices)?

**What this means:**
- Write test cases before implementation (true TDD for production code)
- Or write tests alongside implementation (acceptable for research code)
- Never write feature code without corresponding test infrastructure in place
- Define edge cases as part of specification, not as afterthoughts
- Use test parameterization to exhaustively cover cases

**Why agents get this wrong:**  
Agents default to feature-first development: "write function, then add tests." This leads to code that handles the case the agent thought about, but misses the 12 edge cases the agent didn't consider. Tests written after implementation are validation theater—they test what the code *does*, not what it *should do*.

**How to recognize you're on track:**  
Test files exist before feature code. Edge cases are enumerated in test names. Parameterized tests cover boundaries. When you discover a bug, you write a test that fails, *then* fix the code.

### 3. Fail Fast with Clarity

**Detect errors at boundaries and report them immediately with context.**

Silent failures are the enemy of correctness. If a function receives invalid input, it must fail *now* with a clear message—not propagate garbage through the system and fail cryptically 10 function calls later.

**What this means:**
- Validate all inputs at function boundaries (preconditions)
- Check physical constraints immediately (unitarity, normalization, bounds)
- Throw exceptions with informative messages that include:
  - What went wrong ("Qubit index 5 out of range")
  - What was expected ("[0, 4]")
  - Where it happened (file, line, function)
- Use assertions to check invariants (conditions that should *never* be false)
- Never silently ignore errors or return success when operation failed

**Why agents get this wrong:**  
Agents write optimistic code that assumes inputs are valid. They skip validation because "the caller should ensure..." This creates code that works perfectly in the happy path and explodes mysteriously when given real-world inputs.

**How to recognize you're on track:**  
Every public function checks its inputs. Every function that can fail has explicit error handling. Error messages are specific enough to debug without looking at code. Assertions check invariants within complex logic.

### 4. Simplicity by Design

**Each component does one thing well. Unix philosophy for research code.**

Complexity is the enemy of correctness. A function that does 5 things is impossible to test exhaustively. A class with 10 responsibilities is impossible to understand. Simple code has fewer bugs. Simple code is easier to test. Simple code is easier to verify.

**What this means:**
- Single Responsibility Principle: each class/function has one reason to change
- Functions are short (≤60 lines for C++, ≤50 lines for Python)
- Deep modules with simple interfaces (complex implementation, simple API)
- Minimal coupling: components depend on interfaces, not implementations
- No premature abstraction: solve the concrete problem first
- Prefer composition over inheritance
- Unix philosophy: write components that do one thing and work together

**Why agents get this wrong:**  
Agents tend toward two extremes: (1) God objects that do everything, or (2) over-abstraction with interfaces for everything. Both are bad. The first creates untestable messes. The second creates architectural astronautics.

**How to recognize you're on track:**  
You can explain what a function does in one sentence. Each class has a clear purpose. Tests are straightforward because each component does one thing. Refactoring is easy because components are loosely coupled.

### 5. Infrastructure Enables Speed

**Tests, CI/CD, and tooling are not overhead—they make you faster.**

The agent's instinct is: "skip tests, build fast, add infrastructure later." This is backwards. Infrastructure-first development means:
- You catch bugs immediately (in seconds via unit tests, not days via manual testing)
- You refactor fearlessly (tests tell you if you broke something)
- You onboard collaborators easily (CI enforces standards, docs are generated)
- You reproduce results reliably (deterministic tests, fixed seeds, documented dependencies)

**What this means:**
- Project skeleton exists before feature code (CMakeLists.txt, pytest config, CI/CD)
- Zero-warning policy from day one (`-Werror`, mypy strict mode, ruff)
- Automated testing in CI (tests run on every commit)
- Test coverage tracking (know what's tested and what isn't)
- Generated documentation (Doxygen, Sphinx)
- Reproducibility tooling (requirements.txt pinned, random seeds documented)

**Why agents get this wrong:**  
Agents see infrastructure as slowing them down. "Let me just write the function first, we'll add tests later." But "later" never comes. Or it comes after the function is written, so tests only validate what the code *does* (which might be wrong), not what it *should do*.

**How to recognize you're on track:**  
CI fails immediately if tests break or warnings appear. Coverage reports show what's tested. Dependencies are pinned. Builds are reproducible. Adding a feature involves: (1) write test, (2) implement, (3) CI passes.

---

## Research-Grade Code Requirements

These are non-negotiable standards for research code. They're not "nice to have"—they're the difference between science and wishful thinking.

### Physical Accuracy

**Ground truth validation is mandatory.**

For every physical quantity you compute, you must validate against:

1. **Analytical results** (when available)
   - Single-qubit gates: verify against closed-form matrices
   - Simple circuits: verify against hand calculation
   - Known limits: e.g., infinite temperature → maximally mixed state

2. **Primary sources** (papers, textbooks)
   - Every equation must cite its source
   - Every algorithm must reference the paper that defined it
   - Gate definitions must match Nielsen & Chuang or equivalent

3. **Established tools** (Qiskit, QuTiP, etc.)
   - Cross-validate against mature implementations
   - Not to copy behavior, but to verify correctness
   - When your results differ, investigate—don't assume you're right

**Physical constraints must be enforced:**
- Quantum states: normalization (⟨ψ|ψ⟩ = 1)
- Unitaries: orthogonality (U†U = I, det(U) = 1)
- Probabilities: non-negative and sum to 1
- Density matrices: Hermitian, positive semidefinite, trace 1
- Measurement outcomes: orthogonal projectors sum to identity

**Numerical stability matters:**
- Avoid subtracting nearly-equal floating-point numbers
- Use stable algorithms (QR decomposition, not Gram-Schmidt)
- Check condition numbers of matrices before inversion
- Prefer built-in linear algebra (LAPACK, cuBLAS) over manual implementation
- Validate results don't accumulate error (compare T-step evolution vs. single-step with T×dt)

### Edge Cases as Specification

**Edge cases are not bugs to fix later—they're part of the definition.**

When you define a function, you must specify:

**Boundary conditions:**
- Empty input (empty circuit, zero qubits, empty dataset)
- Single element (one gate, one qubit, one sample)
- Maximum size (MAX_QUBITS, memory limits, array bounds)
- Zero values (zero angle rotations, zero learning rate)

**Degenerate cases:**
- Identity operations (do they compose correctly?)
- Commuting operations (can they be reordered?)
- Repeated operations (does X⁴ = I?)
- Inverse operations (does U·U† = I?)

**Invalid inputs:**
- Out of range (qubit index > num_qubits, negative probabilities)
- Wrong shape (mismatched matrix dimensions)
- Wrong type (complex instead of real, non-unitary matrix)
- Special values (NaN, infinity, subnormal numbers)

### Reproducibility

**Research that can't be reproduced isn't research.**

Every result must be reproducible:

**Determinism:**
- Set random seeds explicitly (document seed values)
- Disable non-deterministic GPU operations if necessary
- Use deterministic algorithms (sort before iteration over sets/dicts)
- Avoid race conditions (even if "unlikely" in single-threaded code)

**Environment documentation:**
- Pin all dependencies (requirements.txt with exact versions)
- Document system requirements (CUDA version, CPU/GPU specs)
- Provide Docker containers for complex environments
- Document non-Python dependencies (BLAS, LAPACK, compilers)

**Data provenance:**
- Document data sources (URLs, dates accessed, versions)
- Include data preprocessing steps (scaling, normalization, splits)
- Store processed data with code (or document how to regenerate)
- Use deterministic train/test splits (fixed seeds)

**Floating-point behavior:**
- Understand that 0.1 + 0.2 ≠ 0.3 in floating-point
- Use np.allclose() for floating-point comparison (with documented tolerance)
- Be aware of summation order (parallel reductions may differ from sequential)
- Document precision requirements (float32 vs float64)

---

## Standards by Language

The principles above apply everywhere. Here's how they manifest in specific languages.

### C++ Standards

**Philosophy: RAII, fail fast, zero-cost abstractions.**

#### Memory Management

**RAII is mandatory.** Every resource (memory, files, GPU memory, network sockets) is managed through RAII:

```cpp
// Good: RAII wrapper for CUDA memory
template <typename T>
class CudaMemory {
public:
    explicit CudaMemory(size_t count) : count_(count) {
        CUDA_CHECK(cudaMalloc(&ptr_, count * sizeof(T)));
    }
    
    ~CudaMemory() noexcept {
        if (ptr_) cudaFree(ptr_);
    }
    
    CudaMemory(const CudaMemory&) = delete;
    CudaMemory& operator=(const CudaMemory&) = delete;
    
    CudaMemory(CudaMemory&& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
        other.ptr_ = nullptr;
    }
    
    T* get() noexcept { return ptr_; }
    
private:
    T* ptr_ = nullptr;
    size_t count_ = 0;
};
```

#### Error Handling

**Fail fast with descriptive errors:**

```cpp
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

void Circuit::addGate(const Gate& gate) {
    if (gate.qubit() >= numQubits_) {
        throw std::out_of_range(
            "Qubit index " + std::to_string(gate.qubit()) + 
            " out of range [0, " + std::to_string(numQubits_ - 1) + "]"
        );
    }
    gates_.push_back(gate);
}
```

#### Modern C++ Features

```cpp
[[nodiscard]] std::optional<Gate> Gate::merge(const Gate& other) const;
constexpr size_t stateSize(int numQubits) { return 1ULL << numQubits; }
[[nodiscard]] size_t numQubits() const noexcept { return numQubits_; }

std::optional<double> Gate::parameter() const noexcept {
    if (isParameterized()) return parameter_;
    return std::nullopt;
}

for (const auto& gate : gates_) { /* ... */ }
auto [fidelity, iterations] = optimizer.run(target);
```

#### Zero-Warning Policy

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast
    )
    
    if(DEFINED ENV{CI})
        add_compile_options(-Werror)
    endif()
endif()
```

#### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Namespaces | lowercase | `qopt`, `qsim::ir` |
| Classes | PascalCase | `Circuit`, `PassManager` |
| Methods | camelCase | `addGate()`, `isParameterized()` |
| Private members | trailing `_` | `numQubits_`, `gates_` |
| Constants | `constexpr` in namespace | `constants::MAX_QUBITS` |
| Device pointers | `d_` prefix | `d_state_`, `d_amplitudes_` |

### Python Standards

**Philosophy: Explicit, typed, tested.**

#### Type Hints

**Type hints are required for all public APIs:**

```python
from typing import Optional, List
import numpy as np
from dataclasses import dataclass

PulseArray = np.ndarray
UnitaryMatrix = np.ndarray

@dataclass
class GRAPEResult:
    final_fidelity: float
    optimized_pulses: PulseArray
    fidelity_history: List[float]
    converged: bool

class GRAPEOptimizer:
    def __init__(
        self,
        drift: DriftHamiltonian,
        controls: List[ControlHamiltonian],
        n_timeslices: int = 100,
        total_time: float = 10.0,
    ) -> None:
        self._drift = drift
        self._controls = controls
    
    def optimize_unitary(
        self,
        target: UnitaryMatrix,
        max_iterations: int = 1000,
        initial_pulses: Optional[PulseArray] = None,
    ) -> GRAPEResult:
        if target.shape[0] != target.shape[1]:
            raise ValueError(f"Target must be square, got shape {target.shape}")
        
        if not np.allclose(target @ target.conj().T, np.eye(target.shape[0])):
            raise ValueError("Target must be unitary (U @ U† = I)")
```

#### Testing with pytest

```toml
[tool.pytest.ini_options]
minversion = "7.0"
testpaths = ["tests"]
addopts = ["-v", "--strict-markers", "-ra"]
markers = [
    "slow: marks tests as slow",
    "deterministic: marks tests that must be deterministic",
    "requires_gpu: marks tests requiring GPU",
]
```

```python
import pytest
import numpy as np

class TestGRAPEOptimizer:
    @pytest.fixture
    def optimizer(self):
        return GRAPEOptimizer(drift, controls, n_timeslices=50)
    
    @pytest.mark.deterministic
    def test_x_gate_high_fidelity(self, optimizer):
        target = np.array([[0, 1], [1, 0]], dtype=complex)
        result = optimizer.optimize_unitary(target, max_iterations=500)
        
        assert result.converged
        assert result.final_fidelity > 0.999
    
    def test_non_unitary_raises_error(self, optimizer):
        non_unitary = np.array([[1, 1], [0, 1]], dtype=complex)
        
        with pytest.raises(ValueError, match="must be unitary"):
            optimizer.optimize_unitary(non_unitary)
```

#### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Modules | snake_case | `grape.py`, `pulse_exporter.py` |
| Classes | PascalCase | `GRAPEOptimizer`, `DriftHamiltonian` |
| Functions | snake_case | `optimize_unitary()`, `compute_fidelity()` |
| Private methods | leading `_` | `_compute_propagators()` |
| Constants | UPPER_SNAKE | `MAX_ITERATIONS`, `DEFAULT_SEED` |

### Cross-Language Requirements

#### Project Structure

```
project-name/
├── CMakeLists.txt / pyproject.toml
├── README.md
├── LICENSE
├── .gitignore
├── .github/workflows/ci.yml
├── include/ or src/
├── tests/
│   ├── conftest.py
│   ├── test_*.py or test_*.cpp
├── docs/
└── examples/
```

#### CI/CD Pipeline

```yaml
name: CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup
        run: |
          pip install -r requirements.txt
          pip install pytest pytest-cov ruff mypy
      
      - name: Lint
        run: |
          ruff check .
          mypy src/
      
      - name: Test
        run: pytest --cov=src/ --cov-report=term
      
      - name: Build docs
        run: sphinx-build docs/ docs/_build/
```

**CI must fail if:**
- Any test fails
- Coverage drops below threshold (70% research, 90% production)
- Linting warnings exist
- Type checking fails
- Documentation build fails

#### Testing Requirements

| Project Type | Coverage Target |
|--------------|-----------------|
| Production code | 90% |
| Research code | 70% |
| Experimental | 50% |

---

## Bounded Complexity (Pragmatic Power of 10)

Adapted from NASA JPL's Power of 10 rules. We adopt the philosophy while pragmatically relaxing rules that conflict with research needs.

### Applied Across Projects

These rules are applied in production across all research projects:

| Project | Language | Tests | Coverage | Key Practices |
|---------|----------|-------|----------|---------------|
| **QubitPulseOpt** | Python | 800+ | 74% | CI/CD pipeline, type hints, GRAPE validation |
| **cuda-quantum-simulator** | CUDA/C++ | 9 suites | - | RAII via CudaMemory<T>, zero manual cudaFree |
| **quantum-circuit-optimizer** | C++17 | - | - | DAG traversal with bounded recursion |
| **QuantumVQE** | Python | - | - | PBS job scripts, deterministic seeds, pinned deps |
| **CloudMLPublic** | Python | - | - | Frozen dataclasses, reproducibility tooling |

### Adopted Rules

| Rule | Description |
|------|-------------|
| **Simple control flow** | No `goto`, `setjmp`, `longjmp` |
| **Bounded loops** | All loops must have provable upper bound |
| **Small functions** | Functions ≤ 60 lines (C++), ≤ 50 lines (Python) |
| **Minimal nesting** | Max 3-4 levels of indentation |
| **Limited preprocessor** | Only `#include`, `#pragma once`, simple `#define` |
| **Zero warnings** | Compile with all warnings enabled |
| **Input validation** | Check all function inputs |
| **Smallest scope** | Declare variables at smallest scope |

### Pragmatically Relaxed

| Rule | Why Not Adopted |
|------|-----------------|
| No recursion | DAG traversal needs recursion |
| No dynamic allocation after init | Circuits grow dynamically |
| No function pointers | Needed for callbacks, optimization |
| 2 assertions per function | Validation at boundaries sufficient |

---

## Quick Reference

### Starting a New Project

**Bootstrap checklist:**

- [ ] Create repository with LICENSE, README, .gitignore
- [ ] Set up build system (CMakeLists.txt or pyproject.toml)
- [ ] Configure CI/CD (.github/workflows/ci.yml)
- [ ] Set up testing framework (GoogleTest or pytest)
- [ ] Create test directory with example test
- [ ] Configure linting (clang-format, ruff)
- [ ] Configure type checking (mypy)
- [ ] Enable compiler warnings (`-Wall -Wextra -Werror`)
- [ ] Write stub README
- [ ] Pin dependencies (requirements.txt)

### Code Review Checklist

**Correctness:**
- [ ] Code matches specification
- [ ] Physical quantities validated
- [ ] References primary sources
- [ ] Numerical stability considered

**Testing:**
- [ ] Tests written before or alongside code
- [ ] Happy path tested
- [ ] Edge cases tested
- [ ] Invalid inputs tested
- [ ] Tests are deterministic

**Error Handling:**
- [ ] Inputs validated at boundaries
- [ ] Errors fail fast with descriptive messages
- [ ] Physical constraints enforced
- [ ] Return values checked

**Simplicity:**
- [ ] Each function does one thing
- [ ] Functions are short
- [ ] Classes have single responsibility
- [ ] No premature abstraction

**Infrastructure:**
- [ ] CI passes
- [ ] Zero warnings
- [ ] Documentation updated
- [ ] Dependencies pinned

### Common Agent Failures

**When agent writes code without tests:**
```
❌ Agent: "I've implemented the rotation merge function."
✅ You: "Show me the tests first. What are the edge cases?"
```

**When agent skips input validation:**
```
❌ Agent: "Function assumes input is valid."
✅ You: "Add validation. What if angle is NaN? If qubit index is -1?"
```

**When agent writes complex function:**
```
❌ Agent: "This function optimizes the circuit in one pass."
✅ You: "Break into smaller functions. Each should do one thing."
```

**When agent adds dependency without justification:**
```
❌ Agent: "I added library X for feature Y."
✅ You: "Can we implement Y without X? If not, pin X version."
```

**When agent implements without citing source:**
```
❌ Agent: "Here's the GRAPE gradient calculation."
✅ You: "Cite the paper. How do we verify correctness?"
```

**When agent doesn't validate physics:**
```
❌ Agent: "Here's the fidelity calculation."
✅ You: "Does this check unitarity? What if the matrix isn't square?"
```

**When agent uses magic numbers:**
```
❌ Agent: "Using tolerance 1e-10 for comparison."
✅ You: "Where does 1e-10 come from? Define as constant with source."
```

**When agent skips reproducibility:**
```
❌ Agent: "Running the optimization..."
✅ You: "Set the seed. Document it. Make this deterministic."
```

**When agent writes monolithic functions:**
```
❌ Agent: "Here's the complete optimization loop (150 lines)."
✅ You: "Break into: setup, iterate, evaluate, finalize. Each < 50 lines."
```

**When agent ignores edge cases:**
```
❌ Agent: "This handles the normal case."
✅ You: "What about empty input? Single element? Maximum size? Zero values?"
```

---

## Appendices

### Appendix A: C++ Header Template

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file FileName.hpp
 * @brief One-line description
 * @author Rylan Malarchick
 * @date 2025
 *
 * Extended description:
 * - What this file implements
 * - How it fits into architecture
 * - Key algorithms or data structures
 */
#pragma once

#include <vector>
#include <optional>

namespace project {

/**
 * @brief Brief class description.
 *
 * Detailed description with usage examples.
 *
 * Thread safety: Not thread-safe.
 *
 * Example:
 * @code
 * MyClass obj(42);
 * obj.doSomething();
 * @endcode
 */
class MyClass {
public:
    /**
     * @brief Constructor description.
     * @param value Description of parameter
     * @throws std::invalid_argument if value < 0
     */
    explicit MyClass(int value);
    
    /**
     * @brief Method description.
     * @return Description of return value
     * @throws std::runtime_error if operation fails
     */
    [[nodiscard]] std::optional<int> compute() const;

private:
    int value_;
};

}
```

### Appendix B: Python Module Template

```python
"""
Module name - brief description.

Extended module description explaining purpose, key classes/functions,
and usage patterns.

Example:
    >>> from project import MyClass
    >>> obj = MyClass(42)
    >>> result = obj.compute()

Author: Rylan Malarchick
Date: 2025
"""

from typing import Optional
import numpy as np


class MyClass:
    """Brief class description.
    
    Detailed description with usage information.
    
    Attributes:
        value: Description of attribute.
    """
    
    def __init__(self, value: int) -> None:
        """Initialize MyClass.
        
        Args:
            value: Description of parameter.
            
        Raises:
            ValueError: If value < 0.
        """
        if value < 0:
            raise ValueError(f"value must be non-negative, got {value}")
        self.value = value
    
    def compute(self) -> Optional[int]:
        """Brief method description.
        
        Returns:
            Description of return value, or None if ...
            
        Raises:
            RuntimeError: If operation fails.
        """
        return self.value


MAX_ITERATIONS = 1000
DEFAULT_TOLERANCE = 1e-10
```

### Appendix C: CMake Template

```cmake
cmake_minimum_required(VERSION 3.18)
project(project-name LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast
    )
    
    if(DEFINED ENV{CI})
        add_compile_options(-Werror)
    endif()
endif()

add_library(project_lib
    src/core.cpp
    src/utils.cpp
)
target_include_directories(project_lib PUBLIC include)

add_executable(project src/main.cpp)
target_link_libraries(project PRIVATE project_lib)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests
    tests/test_core.cpp
    tests/test_utils.cpp
)
target_link_libraries(tests PRIVATE project_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)
```

### Appendix D: pytest Configuration

```toml
[build-system]
requires = ["setuptools>=61.0"]
build-backend = "setuptools.build_meta"

[project]
name = "project-name"
version = "1.0.0"
description = "Brief description"
authors = [{name = "Rylan Malarchick"}]
requires-python = ">=3.9"
dependencies = [
    "numpy>=1.21",
    "scipy>=1.7",
]

[project.optional-dependencies]
dev = [
    "pytest>=7.0",
    "pytest-cov>=4.0",
    "ruff>=0.1",
    "mypy>=1.0",
]

[tool.pytest.ini_options]
minversion = "7.0"
testpaths = ["tests"]
addopts = ["-v", "--strict-markers", "-ra"]
markers = [
    "slow: marks tests as slow",
    "deterministic: marks tests requiring fixed seed",
    "integration: marks integration tests",
    "requires_gpu: marks tests requiring GPU",
]

[tool.ruff]
line-length = 100
target-version = "py39"

[tool.ruff.lint]
select = ["E", "F", "W", "I", "N", "UP"]

[tool.mypy]
python_version = "3.9"
warn_return_any = true
warn_unused_ignores = true
no_implicit_optional = true
check_untyped_defs = true
```

### Appendix E: One-Page Cheat Sheet

**Before Writing Code:**
- [ ] Define what "correct" means (tests are specification)
- [ ] Identify edge cases (empty, single, max, zero, invalid)
- [ ] Find primary sources (papers, textbooks) for algorithms
- [ ] Set up test infrastructure if not present
- [ ] Document random seeds for reproducibility

**While Writing Code:**
- [ ] Validate inputs at function boundaries
- [ ] Keep functions short (≤60 lines C++, ≤50 lines Python)
- [ ] Cite sources for physics/algorithms in comments
- [ ] Check physical constraints (unitarity, normalization, bounds)
- [ ] Use RAII for resources (no raw new/delete, cudaMalloc/Free)
- [ ] Define constants (no magic numbers)
- [ ] Write tests alongside implementation

**After Writing Code:**
- [ ] All tests pass
- [ ] Zero warnings (compile with `-Wall -Wextra -Werror`)
- [ ] Type checking passes (mypy strict mode)
- [ ] Linting passes (ruff, clang-tidy)
- [ ] Coverage meets threshold (70% research, 90% production)
- [ ] Documentation updated
- [ ] Dependencies pinned

**Red Flags to Watch For:**
- Function > 60 lines → Split it
- No tests → Write them first
- Magic number → Define constant with source
- "Assumes input is valid" → Add validation
- No citation → Find the source
- Random without seed → Make deterministic

### Appendix F: Git Workflow

#### Branch Naming

| Branch Type | Pattern | Example |
|-------------|---------|---------|
| Main | `main` | `main` |
| Feature | `feat/<short-description>` | `feat/rotation-merge-pass` |
| Bug fix | `fix/<short-description>` | `fix/dag-cycle-detection` |
| Refactor | `refactor/<short-description>` | `refactor/raii-memory` |
| Documentation | `docs/<short-description>` | `docs/api-reference` |
| Experiment | `exp/<short-description>` | `exp/sabre-routing` |

#### Commit Message Format (Conventional Commits)

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

**Examples:**

```
feat(ir): add Gate factory methods for common gates

Add static factory methods (x, h, cnot, rz) to Gate class.
This provides a cleaner API than direct construction.

Closes #42
```

```
fix(passes): handle zero-angle rotations in RotationMerge

Previously, merging Rz(θ) with Rz(-θ) could produce Rz(0),
which should be eliminated. Now correctly removes identity gates.
```

```
refactor(simulator): replace raw pointers with CudaMemory<T>
```

```
test(dag): add tests for topological sort with cycles
```

---

## Conclusion

Research code doesn't have to be bad code. The five principles—**Correctness First**, **Specification Before Code**, **Fail Fast**, **Simplicity by Design**, and **Infrastructure Enables Speed**—apply universally across quantum computing, HPC, ML, and any research domain.

**For AI agents:** These principles prevent common failure modes: writing features without tests, skipping edge cases, missing input validation, and building without infrastructure.

**For researchers:** Research code following these principles is faster to debug, easier to extend, and more trustworthy. Infrastructure isn't overhead—it's what lets you move fast and trust your results.

**For the community:** This document is opinionated but adaptable. The core principles apply everywhere; specific standards can be adjusted for your stack.

---

**Version History:**
- v2.1 (Dec 2025): Added project examples, expanded agent failures, cheat sheet (Appendix E), git workflow (Appendix F)
- v2.0 (Dec 2025): Complete rewrite focusing on principles-first approach
- v1.0 (Dec 2024): Initial standards-based version

**License:** MIT

**Author:** Rylan Malarchick (rylan1012@gmail.com)
