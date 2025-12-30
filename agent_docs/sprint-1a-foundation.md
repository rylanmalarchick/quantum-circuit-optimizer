# Sprint 1A: Foundation

**Goal:** Working build, basic IR, first tests passing, CI/CD running  
**Duration:** 3-5 coding sessions  
**Exit Criteria:** `cmake --build build && ctest` passes with ≥10 unit tests

---

## Current State

| Component | Status | Issues |
|-----------|--------|--------|
| CMakeLists.txt | Broken | No warnings, no GoogleTest, minimal config |
| Gate.hpp | Partial | No constructors, no validation, no operators |
| Circuit.hpp | Partial | No iteration, missing methods main.cpp expects |
| Qubit.hpp | Empty | 1-line stub |
| DAG.hpp | Empty | 1-line stub (deferred to Sprint 1B) |
| main.cpp | Broken | Uses non-existent constructors/methods |
| Tests | None | Empty directories |
| CI/CD | None | No GitHub Actions |

---

## Tasks

### Task 1: CMakeLists.txt Overhaul
**Priority:** Critical (blocks everything)

Rewrite CMakeLists.txt with:
- [x] cmake_minimum_required(VERSION 3.18)
- [x] C++17 standard required
- [x] Compiler warnings (-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
- [x] WARNINGS_AS_ERRORS option for CI
- [x] GoogleTest via FetchContent
- [x] Library target (qopt_ir) for testability
- [x] compile_commands.json export
- [x] CTest integration

**Acceptance:** `cmake -B build && cmake --build build` completes with zero warnings

---

### Task 2: Types.hpp (New File)
**Priority:** High

Create `include/ir/Types.hpp` with:
- [x] SPDX header + copyright
- [x] Common type aliases (QubitIndex, GateId, Angle)
- [x] Constants (MAX_QUBITS, TOLERANCE)
- [x] Namespace structure (qopt::ir)

**Acceptance:** Types compile and are used by other headers

---

### Task 3: Gate.hpp Rewrite
**Priority:** High

Complete rewrite with:
- [x] SPDX header + Doxygen file-level docs
- [x] Namespace qopt::ir
- [x] GateType enum with all common gates (H, X, Y, Z, S, T, CNOT, CZ, Rx, Ry, Rz, SWAP)
- [x] Gate class with:
  - Proper constructor(s)
  - Factory methods: Gate::h(qubit), Gate::cnot(control, target), Gate::rz(qubit, angle)
  - Accessors: type(), qubits(), parameter(), numQubits(), isParameterized()
  - Validation: throws on invalid qubit indices
  - [[nodiscard]] attributes
  - operator== for comparison
- [x] Non-member functions: gateTypeName(GateType), isHermitian(GateType), numQubitsFor(GateType)

**Acceptance:** Unit tests for all Gate operations pass

---

### Task 4: Qubit.hpp Implementation
**Priority:** Medium

Create `include/ir/Qubit.hpp` with:
- [x] SPDX header + Doxygen
- [x] Namespace qopt::ir
- [x] QubitId type (size_t alias)
- [x] Qubit class (simple wrapper, may just be typedef for now)

**Acceptance:** Compiles and integrates with Gate/Circuit

---

### Task 5: Circuit.hpp Rewrite
**Priority:** High

Complete rewrite with:
- [x] SPDX header + Doxygen
- [x] Namespace qopt::ir
- [x] Circuit class with:
  - Constructor: Circuit(size_t num_qubits)
  - Gate management: addGate(Gate), gates(), gate(index), numGates()
  - Qubit info: numQubits()
  - Metrics: depth() (longest path through circuit)
  - Iteration: begin(), end() for range-for
  - Validation: throws if gate references invalid qubit
  - Move semantics (delete copy for large circuits)
- [x] operator<< for printing

**Acceptance:** Unit tests for Circuit operations pass, can iterate with range-for

---

### Task 6: main.cpp Fix
**Priority:** Medium

Rewrite to use actual API:
- [x] Use factory methods: Gate::h(), Gate::cnot()
- [x] Use proper addGate() calls
- [x] Print circuit with operator<<
- [x] Demonstrate basic operations

**Acceptance:** main.cpp compiles and runs, produces sensible output

---

### Task 7: Unit Tests - Gate
**Priority:** High

Create `tests/ir/test_gate.cpp` with:
- [x] Test fixture setup
- [x] Factory method tests (h, x, cnot, rz create correct gates)
- [x] Accessor tests (type, qubits, parameter)
- [x] Validation tests (invalid qubit throws)
- [x] Equality tests
- [x] Parameterized gate tests (Rz with angle)
- [x] Edge cases (single qubit, two qubit, parametric)

**Acceptance:** ≥6 tests, all pass

---

### Task 8: Unit Tests - Circuit
**Priority:** High

Create `tests/ir/test_circuit.cpp` with:
- [x] Construction tests (empty circuit, with qubits)
- [x] addGate tests (single, multiple)
- [x] Validation tests (gate with invalid qubit throws)
- [x] Iteration tests (range-for works)
- [x] Depth calculation tests
- [x] numGates/numQubits tests

**Acceptance:** ≥6 tests, all pass

---

### Task 9: GitHub Actions CI
**Priority:** Medium

Create `.github/workflows/ci.yml` with:
- [x] Trigger on push/PR to main
- [x] Ubuntu latest
- [x] CMake configure with WARNINGS_AS_ERRORS=ON
- [x] Build
- [x] Run tests with ctest
- [x] Upload test results

**Acceptance:** CI badge shows green on push

---

### Task 10: README Update
**Priority:** Low

Update README.md with:
- [x] Project description
- [x] Build instructions
- [x] Test instructions
- [x] Current status
- [x] Project structure

**Acceptance:** README accurately reflects project state

---

## File Changes Summary

### New Files
```
include/ir/Types.hpp           # Type aliases and constants
tests/ir/test_gate.cpp         # Gate unit tests
tests/ir/test_circuit.cpp      # Circuit unit tests
.github/workflows/ci.yml       # CI pipeline
docs/sprint-1a-foundation.md   # This file
```

### Modified Files
```
CMakeLists.txt                 # Complete rewrite
include/ir/Gate.hpp            # Complete rewrite
include/ir/Qubit.hpp           # Implement (currently empty)
include/ir/Circuit.hpp         # Complete rewrite
src/main.cpp                   # Fix to use actual API
README.md                      # Update with build/test instructions
```

### Unchanged (Deferred)
```
include/ir/DAG.hpp             # Sprint 1B
include/parser/Parser.hpp      # Sprint 2
include/passes/Pass.hpp        # Sprint 3
include/routing/Router.hpp     # Sprint 4
```

---

## Implementation Order

```
1. CMakeLists.txt      ← Must be first (enables compilation)
2. Types.hpp           ← Dependencies for other headers
3. Gate.hpp            ← Core type, needed by Circuit
4. Qubit.hpp           ← Simple, quick win
5. Circuit.hpp         ← Uses Gate, enables main.cpp
6. main.cpp            ← Validates API works
7. test_gate.cpp       ← Validates Gate implementation
8. test_circuit.cpp    ← Validates Circuit implementation
9. ci.yml              ← Automation
10. README.md          ← Documentation
```

---

## Code Standards Checklist

For each file, verify:

- [ ] SPDX-License-Identifier: MIT header
- [ ] Copyright line
- [ ] File-level Doxygen documentation
- [ ] Namespace qopt::ir (or qopt::*)
- [ ] Include guards using #pragma once
- [ ] Includes ordered: corresponding header, project headers, std headers
- [ ] [[nodiscard]] on query functions
- [ ] noexcept on destructors and simple accessors
- [ ] const correctness
- [ ] Input validation with descriptive exceptions
- [ ] No raw new/delete

---

## Validation Commands

```bash
# Configure (from project root)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DWARNINGS_AS_ERRORS=ON

# Build
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure

# Run main
./build/quantum_circuit_optimizer

# Check for warnings (should be zero)
cmake --build build 2>&1 | grep -i warning
```

---

## Exit Criteria Checklist

- [ ] `cmake -B build` succeeds
- [ ] `cmake --build build` succeeds with zero warnings
- [ ] `ctest` runs ≥10 tests, all pass
- [ ] `./quantum_circuit_optimizer` produces sensible output
- [ ] CI pipeline passes on GitHub
- [ ] All source files have SPDX headers
- [ ] README has build/test instructions

---

## Notes

- DAG.hpp is deferred to Sprint 1B (not needed for basic IR)
- Parser, Passes, Routing are deferred to later sprints
- Focus is on getting a solid, tested foundation
- Apply coding standards from docs/ consistently
