# Agent Coding Context

**Purpose:** Paste this into AI agent context for vibe coding sessions.  
**Author:** Rylan Malarchick  
**Version:** 1.0  
**Last Updated:** December 2025

---

## Related Documents

| Document | Purpose |
|----------|---------|
| [agent-coding-context.md](agent-coding-context.md) | Quick reference for AI agents |
| [research-code-principles.md](research-code-principles.md) | Philosophy and principles |
| [style-guide-reference.md](style-guide-reference.md) | Detailed style conventions |
| [repo-standards.md](repo-standards.md) | Git workflow and repository hygiene |

---

## Quick Reference: 5 Core Principles

### 1. Correctness First
Physical accuracy and mathematical correctness are non-negotiable. Research code must be *right* before it's fast, flexible, or elegant. Validate against primary sources (papers, textbooks). Unit test against ground truth. Check physical constraints (unitarity, normalization, bounds). When results disagree with expectations, assume code is wrong—not physics.

### 2. Specification Before Code
You can't write code until you define what "correct" means. Tests *are* your specification—they define the contract. Write test cases before or alongside implementation. Never write feature code without corresponding test infrastructure. Define edge cases as part of specification, not afterthoughts.

### 3. Fail Fast with Clarity
Detect errors at boundaries and report them immediately with context. Silent failures are the enemy of correctness. Validate all inputs at function boundaries. Throw exceptions with informative messages (what went wrong, what was expected, where it happened). Never silently ignore errors.

### 4. Simplicity by Design
Each component does one thing well. Unix philosophy for research code. Functions ≤60 lines (C++), ≤50 lines (Python). Single Responsibility Principle. Minimal coupling. No premature abstraction. Prefer composition over inheritance.

### 5. Infrastructure Enables Speed
Tests, CI/CD, and tooling are not overhead—they make you faster. Project skeleton exists before feature code. Zero-warning policy from day one. Automated testing in CI. Test coverage tracking. Reproducibility tooling (pinned dependencies, documented seeds).

---

## Instructions for AI Agents

### You MUST:

1. **Write or describe test cases BEFORE implementing features**
   - Define expected behavior for happy path
   - Enumerate edge cases (empty input, single element, max size, invalid input)
   - Tests are specification, not afterthought

2. **Validate all inputs at function boundaries**
   - Check types, ranges, shapes
   - Fail immediately with descriptive error messages
   - Include: what went wrong, what was expected, where it happened

3. **Cite sources for physics equations and algorithms**
   - Reference paper/textbook for any non-trivial equation
   - Include DOI or arXiv link when available
   - If implementing from a paper, cite it in docstring/comment

4. **Check physical constraints**
   - Unitarity: U†U = I
   - Normalization: ⟨ψ|ψ⟩ = 1, trace(ρ) = 1
   - Probabilities: non-negative, sum to 1
   - Hermiticity: H = H†

5. **Keep functions short and focused**
   - C++: ≤60 lines per function
   - Python: ≤50 lines per function
   - Each function does ONE thing
   - If you can't describe it in one sentence, split it

6. **Use RAII for resource management (C++)**
   - No raw new/delete
   - Use smart pointers (unique_ptr, shared_ptr)
   - Wrap CUDA memory in RAII classes

7. **Ensure reproducibility**
   - Set random seeds explicitly
   - Document seed values
   - Use deterministic algorithms where possible
   - Pin dependencies with exact versions

8. **Follow naming conventions**
   - C++: PascalCase classes, camelCase methods, trailing underscore for private members
   - Python: PascalCase classes, snake_case functions/methods, UPPER_SNAKE constants

### You MUST NOT:

1. **Skip tests "to save time"**
   - Tests are not optional
   - No "I'll add tests later"
   - If there's no test, it doesn't work

2. **Assume inputs are valid**
   - Always validate at boundaries
   - Never trust caller to provide correct data
   - Check for NaN, infinity, negative where inappropriate

3. **Implement algorithms without referencing source**
   - No "here's the gradient calculation" without citation
   - Physics equations need sources
   - If you can't cite it, you can't verify it

4. **Write "God functions" that do multiple things**
   - Break into smaller functions
   - Each function has single responsibility
   - If it's > 60 lines, it's too long

5. **Add dependencies without justification**
   - Can we implement without the dependency?
   - If not, pin the version
   - Document why it's needed

6. **Use magic numbers**
   - No hardcoded tolerances without explanation
   - Define constants with descriptive names
   - Document where values come from

7. **Ignore warnings**
   - Zero warnings policy
   - Warnings often indicate real bugs
   - Fix them, don't suppress them

8. **Skip error handling**
   - Every function that can fail needs explicit error handling
   - No silent failures
   - No returning garbage on error

---

## Common Agent Failure Modes

Learn from these examples. When you catch yourself making these mistakes, stop and correct.

### When writing code without tests:
```
❌ Agent: "I've implemented the rotation merge function."
✅ You: "Show me the tests first. What are the edge cases?"
```

### When skipping input validation:
```
❌ Agent: "Function assumes input is valid."
✅ You: "Add validation. What if angle is NaN? If qubit index is -1?"
```

### When writing complex functions:
```
❌ Agent: "This function optimizes the circuit in one pass."
✅ You: "Break into smaller functions. Each should do one thing."
```

### When adding dependencies without justification:
```
❌ Agent: "I added library X for feature Y."
✅ You: "Can we implement Y without X? If not, pin X version."
```

### When implementing without citing source:
```
❌ Agent: "Here's the GRAPE gradient calculation."
✅ You: "Cite the paper. How do we verify correctness?"
```

### When not validating physics:
```
❌ Agent: "Here's the fidelity calculation."
✅ You: "Does this enforce unitarity? What if the matrix isn't square?"
```

### When using magic numbers:
```
❌ Agent: "Using tolerance 1e-10 for comparison."
✅ You: "Where does 1e-10 come from? Document or make it a constant."
```

### When skipping reproducibility:
```
❌ Agent: "Running the optimization..."
✅ You: "Set the seed. Document it. Make this deterministic."
```

### When writing monolithic functions:
```
❌ Agent: "Here's the complete optimization loop (150 lines)."
✅ You: "Break into: setup, iterate, evaluate, finalize. Each < 50 lines."
```

### When ignoring edge cases:
```
❌ Agent: "This handles the normal case."
✅ You: "What about empty input? Single element? Maximum size? Zero values?"
```

### When not checking return values:
```
❌ Agent: "Called cudaMalloc and proceeded."
✅ You: "Check the return value. Use CUDA_CHECK macro."
```

### When writing optimistic code:
```
❌ Agent: "The file should exist at this path."
✅ You: "Check if it exists. Throw descriptive error if not."
```

---

## Code Review Checklist

Use this checklist before considering any code "done."

### Correctness
- [ ] Code matches specification/requirements
- [ ] Physical quantities validated against known values
- [ ] References primary sources (papers, textbooks)
- [ ] Numerical stability considered (no subtracting nearly-equal floats)
- [ ] Physical constraints enforced (unitarity, normalization, bounds)

### Testing
- [ ] Tests written before or alongside code
- [ ] Happy path tested
- [ ] Edge cases tested (empty, single, max, zero)
- [ ] Invalid inputs tested (wrong type, out of range, NaN)
- [ ] Tests are deterministic (fixed seeds)
- [ ] Tests run in CI

### Error Handling
- [ ] Inputs validated at function boundaries
- [ ] Errors fail fast with descriptive messages
- [ ] Error messages include: what, expected, where
- [ ] Return values checked (especially for C/CUDA APIs)
- [ ] No silent failures

### Simplicity
- [ ] Each function does one thing
- [ ] Functions are short (≤60 lines C++, ≤50 lines Python)
- [ ] Classes have single responsibility
- [ ] No premature abstraction
- [ ] No unnecessary dependencies

### Style
- [ ] Naming follows conventions
- [ ] Code is formatted (clang-format, black)
- [ ] No magic numbers (constants defined and documented)
- [ ] Comments explain "why" not "what"
- [ ] Docstrings on all public APIs

### Infrastructure
- [ ] CI passes (tests, linting, type checking)
- [ ] Zero warnings
- [ ] Documentation updated
- [ ] Dependencies pinned
- [ ] Seeds documented for reproducibility

---

## Self-Check Before Completing

Before marking any task complete, verify:

- [ ] All tests pass?
- [ ] Zero compiler warnings?
- [ ] Every public function has docstring?
- [ ] Every non-trivial algorithm has citation?
- [ ] Edge cases tested (empty, single, max, invalid)?
- [ ] Physical constraints validated (if applicable)?
- [ ] Dependencies pinned (if added)?
- [ ] Documentation updated?

---

## Algorithms Requiring Citation

Always cite the source for these algorithms:

| Algorithm | Citation Example |
|-----------|------------------|
| Topological sort | Kahn, A.B. (1962). Comm. ACM 5(11):558-562 |
| Graph traversal (BFS/DFS) | Standard algorithm, note variant if non-standard |
| Matrix decomposition (QR, SVD, LU) | Cite numerical library or Golub & Van Loan |
| Optimization (GRAPE, BFGS, Adam) | Cite original paper with DOI/arXiv |
| Quantum algorithms | Cite Nielsen & Chuang or original paper |
| Any equation from a paper | Include DOI or arXiv link |

Format in code:
```cpp
/**
 * @brief Brief description.
 *
 * @references
 * [1] Author, "Title", Journal, Year. doi:XXX
 */
```

---

## New File Checklist

Every new source file must have:

1. **SPDX header**: `// SPDX-License-Identifier: MIT`
2. **Copyright**: `// Copyright (c) 2025 Rylan Malarchick`
3. **Doxygen block**: `@file`, `@brief`, `@author`, `@date`
4. **References**: `@references` if implementing algorithms
5. **Include guard**: `#pragma once` (C++) or nothing needed (Python)
6. **Include order**: own header → project → third-party → stdlib

---

## Task Management

When working on complex tasks:

1. **Break down**: Split into trackable items before starting
2. **Status tracking**: Mark items `in_progress` while working
3. **Immediate completion**: Mark `complete` as soon as done
4. **No batching**: Never batch multiple completions
5. **Verification**: Run self-check before marking complete

Example workflow:
```
- [ ] pending: Implement DAG class
- [~] in_progress: Add topological sort  ← currently working
- [x] complete: Create DAGNode wrapper
- [x] complete: Add unit tests
```

---

## Language-Specific Quick Reference

### C++

```cpp
// Naming
namespace qopt::ir { }              // lowercase namespaces
class CircuitOptimizer { };         // PascalCase classes
void addGate();                     // camelCase methods
size_t numQubits_;                  // trailing underscore for private
constexpr size_t MAX_QUBITS = 30;   // constexpr constants
cuDoubleComplex* d_state_;          // d_ prefix for device pointers

// Required attributes
[[nodiscard]] size_t numQubits() const noexcept;
constexpr double PI = 3.14159265358979323846;

// Error handling
if (qubit >= numQubits_) {
    throw std::out_of_range(
        "Qubit index " + std::to_string(qubit) + 
        " out of range [0, " + std::to_string(numQubits_ - 1) + "]"
    );
}

// CUDA error checking
#define CUDA_CHECK(call) do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
        throw std::runtime_error( \
            std::string("CUDA error: ") + cudaGetErrorString(err)); \
    } \
} while(0)

// RAII memory
template<typename T>
class CudaMemory {
public:
    explicit CudaMemory(size_t count);
    ~CudaMemory() noexcept;
    CudaMemory(CudaMemory&&) noexcept;
    CudaMemory& operator=(CudaMemory&&) noexcept;
    CudaMemory(const CudaMemory&) = delete;
    T* get() noexcept { return ptr_; }
private:
    T* ptr_ = nullptr;
};
```

### Python

```python
# Naming
from dataclasses import dataclass
from typing import Optional, List

MAX_ITERATIONS = 1000              # UPPER_SNAKE constants
DEFAULT_TOLERANCE = 1e-10

class GRAPEOptimizer:              # PascalCase classes
    def __init__(self, drift: DriftHamiltonian) -> None:
        self._drift = drift        # leading underscore for private
    
    def optimize_unitary(          # snake_case methods
        self,
        target: np.ndarray,
        max_iterations: int = 1000,
    ) -> GRAPEResult:
        """Optimize pulses to implement target unitary.
        
        Args:
            target: Target unitary matrix, shape (d, d).
            max_iterations: Maximum optimization iterations.
        
        Returns:
            GRAPEResult with final fidelity and optimized pulses.
        
        Raises:
            ValueError: If target is not unitary.
        """
        if target.shape[0] != target.shape[1]:
            raise ValueError(f"Target must be square, got {target.shape}")
        
        if not np.allclose(target @ target.conj().T, np.eye(target.shape[0])):
            raise ValueError("Target must be unitary (U @ U† = I)")
        
        # ... implementation

@dataclass(frozen=True)
class GRAPEResult:
    final_fidelity: float
    optimized_pulses: np.ndarray
    converged: bool
```

---

## Physical Validation Reference

### Quantum States
```python
# Normalization check
assert np.isclose(np.linalg.norm(state), 1.0), "State not normalized"

# Density matrix checks
assert np.allclose(rho, rho.conj().T), "Density matrix not Hermitian"
assert np.isclose(np.trace(rho), 1.0), "Density matrix trace != 1"
assert np.all(np.linalg.eigvalsh(rho) >= -1e-10), "Density matrix not positive semidefinite"
```

### Unitaries
```python
# Unitarity check
identity = np.eye(U.shape[0])
assert np.allclose(U @ U.conj().T, identity), "U @ U† != I"
assert np.allclose(U.conj().T @ U, identity), "U† @ U != I"
```

### Probabilities
```python
# Probability constraints
assert np.all(probs >= 0), "Negative probabilities"
assert np.isclose(np.sum(probs), 1.0), "Probabilities don't sum to 1"
```

### Fidelity
```python
# State fidelity: F = |⟨ψ|φ⟩|²
fidelity = np.abs(np.vdot(psi, phi))**2
assert 0 <= fidelity <= 1, f"Invalid fidelity: {fidelity}"

# Gate fidelity (average): F = |Tr(U†V)|² / d²
d = U.shape[0]
fidelity = np.abs(np.trace(U.conj().T @ V))**2 / d**2
```

---

## Project Context Template

Fill this in for each coding session:

```markdown
## Session Context

**Project:** [project-name]
**Language:** [C++17 / Python 3.10 / CUDA]
**Key Files:** 
- [path/to/main/file.cpp]
- [path/to/test/file.cpp]

**Build System:** [CMake / pyproject.toml]
**Test Framework:** [GoogleTest / pytest]
**CI Status:** [passing / failing]

**Current Task:** [What we're implementing]

**Relevant Standards:**
- Apply research-code-principles.md
- Follow style-guide-reference.md for detailed conventions
- Check repo-standards.md for git workflow
```

---

## Project Examples

These standards are applied in production across all projects:

| Project | Language | Tests | Coverage | Key Practices |
|---------|----------|-------|----------|---------------|
| **QubitPulseOpt** | Python | 800+ | 74% | CI/CD, type hints, GRAPE validation |
| **cuda-quantum-simulator** | CUDA/C++ | 9 suites | - | RAII via CudaMemory<T>, zero cudaFree |
| **quantum-circuit-optimizer** | C++17 | - | - | DAG traversal, bounded recursion |
| **QuantumVQE** | Python | - | - | PBS scripts, deterministic seeds |
| **CloudMLPublic** | Python | - | - | Frozen dataclasses, reproducibility |

---

## Quick Commands

### Python
```bash
# Run tests
pytest -v

# Run with coverage
pytest --cov=src/ --cov-report=term

# Type checking
mypy src/

# Linting
ruff check .

# Format
black .
isort .
```

### C++
```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure

# Format
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### Git
```bash
# Commit format
git commit -m "feat(ir): add Gate factory methods"
git commit -m "fix(passes): handle zero-angle rotations"
git commit -m "test(dag): add cycle detection tests"
git commit -m "docs: update API reference"
```

---

**Remember:** Research code that can't be reproduced isn't research. Code that isn't tested doesn't work. Code without sources can't be verified. Apply these principles consistently.
