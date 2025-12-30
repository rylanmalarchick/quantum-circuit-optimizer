# Quantum Circuit Optimizer: Scope of Work

**Project:** quantum-circuit-optimizer  
**Author:** Rylan Malarchick  
**Created:** December 2025  
**Status:** Sprint 5 Complete (v1.0.0)

---

## Executive Summary

Build a production-quality quantum circuit compiler in C++17 that:
1. Parses OpenQASM 3.0 circuit descriptions
2. Represents circuits as a DAG-based intermediate representation
3. Optimizes circuits through multiple transformation passes
4. Performs topology-aware qubit routing with SWAP insertion

The project follows research-code-principles with comprehensive testing, CI/CD, and documentation.

---

## Sprint Overview

| Sprint | Focus | Duration | Status |
|--------|-------|----------|--------|
| **1A** | Foundation (Gate, Circuit, Build, CI) | 3-5 sessions | Complete |
| **1B** | DAG IR for Optimization | 2-3 sessions | Complete |
| **2** | OpenQASM 3.0 Parser | 3-4 sessions | Complete |
| **3** | Optimization Passes | 4-5 sessions | Complete |
| **4** | Qubit Routing | 4-5 sessions | Complete |
| **5** | Polish & Documentation | 2-3 sessions | Complete |

---

## Sprint 1A: Foundation

**Goal:** Working build, basic IR, first tests passing, CI/CD running  
**Exit Criteria:** `cmake --build build && ctest` passes with >=10 unit tests

### Deliverables

| Component | Description | Tests |
|-----------|-------------|-------|
| `Types.hpp` | Common type aliases (QubitIndex, Angle), constants (MAX_QUBITS) | - |
| `Gate.hpp` | Gate representation with factory methods, validation, operators | 26 |
| `Qubit.hpp` | Qubit utilities and type definitions | - |
| `Circuit.hpp` | Circuit container with iteration, depth calculation, metrics | 24 |
| `CMakeLists.txt` | Build system with warnings, GoogleTest, CTest | - |
| `ci.yml` | GitHub Actions CI pipeline | - |

### Key Design Decisions

- **Factory methods**: `Gate::h(qubit)`, `Gate::cnot(control, target)`, `Gate::rz(qubit, angle)`
- **Validation at boundaries**: All public methods validate inputs, throw descriptive exceptions
- **RAII**: Smart pointers for ownership, no raw new/delete
- **Zero warnings**: `-Wall -Wextra -Wpedantic -Werror` in CI

### Acceptance Criteria

- [x] `cmake -B build` succeeds
- [x] `cmake --build build` succeeds with zero warnings
- [x] `ctest` runs 50 tests, all pass
- [x] CI pipeline passes on GitHub
- [x] All source files have SPDX headers

---

## Sprint 1B: DAG IR

**Goal:** DAG-based intermediate representation for circuit optimization  
**Exit Criteria:** DAG construction, topological traversal, and node manipulation work with >=20 tests

### Deliverables

| Component | Description | Tests |
|-----------|-------------|-------|
| `DAG.hpp` | DAG class with node management, topological sort, traversal | 54 |
| `DAGNode` | Node wrapper containing Gate, predecessor/successor edges | - |

### Key Design Decisions

- **Ownership model**: DAG owns nodes via `std::unique_ptr<DAGNode>`, edges are raw pointers (non-owning)
- **Topological sort**: Kahn's algorithm with citation (Kahn 1962)
- **Traversal**: Both forward (topological) and reverse iteration support
- **Thread safety**: Not thread-safe (documented)

### API Surface

```cpp
// DAGNode - wrapper around Gate with graph edges
class DAGNode {
public:
    explicit DAGNode(Gate gate);
    
    [[nodiscard]] const Gate& gate() const noexcept;
    [[nodiscard]] const std::vector<DAGNode*>& predecessors() const noexcept;
    [[nodiscard]] const std::vector<DAGNode*>& successors() const noexcept;
    
    void addPredecessor(DAGNode* node);
    void addSuccessor(DAGNode* node);
    void removePredecessor(DAGNode* node);
    void removeSuccessor(DAGNode* node);
};

// DAG - directed acyclic graph of quantum gates
class DAG {
public:
    explicit DAG(size_t numQubits);
    
    // Node management
    DAGNode* addNode(Gate gate);
    void removeNode(DAGNode* node);
    
    // Graph queries
    [[nodiscard]] size_t numNodes() const noexcept;
    [[nodiscard]] size_t numQubits() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    
    // Traversal
    [[nodiscard]] std::vector<DAGNode*> topologicalOrder() const;
    [[nodiscard]] std::vector<DAGNode*> nodesOnQubit(size_t qubit) const;
    
    // Conversion
    [[nodiscard]] Circuit toCircuit() const;
    static DAG fromCircuit(const Circuit& circuit);
};
```

### Acceptance Criteria

- [x] DAG construction from Circuit works correctly
- [x] Topological sort produces valid ordering
- [x] Node removal maintains graph consistency
- [x] 54 tests pass with zero warnings
- [x] Kahn's algorithm properly cited in source

---

## Sprint 2: OpenQASM 3.0 Parser

**Goal:** Parse OpenQASM 3.0 files into Circuit/DAG representation  
**Exit Criteria:** Parser handles core QASM constructs with >=30 tests

### Deliverables

| Component | Description | Tests |
|-----------|-------------|-------|
| `Lexer.hpp` | Tokenizer for OpenQASM 3.0 | 58 |
| `Parser.hpp` | Recursive descent parser producing Circuit | 51 |
| `Token.hpp` | Token types and Token class | - |
| `QASMError.hpp` | Error types with line/column information | - |

### Supported Constructs

```qasm
// Version declaration
OPENQASM 3.0;

// Quantum register declaration
qubit[4] q;

// Classical register declaration (optional)
bit[4] c;

// Gate applications
h q[0];
cx q[0], q[1];
rz(pi/4) q[2];

// Measurement
c[0] = measure q[0];
```

### Key Design Decisions

- **Recursive descent**: Simple, readable, sufficient for QASM subset
- **Error recovery**: Continue parsing after errors to report multiple issues
- **Line/column tracking**: All errors include source location
- **Extensibility**: Parser architecture allows adding constructs incrementally

### API Surface

```cpp
class Parser {
public:
    explicit Parser(std::string_view source);
    
    [[nodiscard]] Circuit parse();
    [[nodiscard]] DAG parseToDAG();
    
    [[nodiscard]] bool hasErrors() const noexcept;
    [[nodiscard]] const std::vector<QASMError>& errors() const noexcept;
};

// Convenience function
[[nodiscard]] Circuit parseQASM(std::string_view source);
[[nodiscard]] Circuit parseQASMFile(const std::filesystem::path& path);
```

### Acceptance Criteria

- [x] Lexer tokenizes all QASM 3.0 tokens correctly
- [x] Parser handles qubit declarations
- [x] Parser handles standard gates (h, x, y, z, cx, cz, rx, ry, rz)
- [x] Parser reports errors with line/column
- [x] Round-trip: parse -> Circuit -> emit matches input semantics
- [x] >=30 tests pass (109 new tests, 213 total)

---

## Sprint 3: Optimization Passes

**Goal:** Implement circuit optimization passes using DAG IR  
**Exit Criteria:** Passes demonstrably reduce circuit metrics with >=40 tests

### Deliverables

| Component | Description | Tests |
|-----------|-------------|-------|
| `Pass.hpp` | Base class for optimization passes | - |
| `PassManager.hpp` | Pipeline for running multiple passes | ~10 |
| `CancellationPass.hpp` | Cancel adjacent inverse gates (XX, HH, etc.) | ~10 |
| `CommutationPass.hpp` | Reorder commuting gates for optimization | ~10 |
| `RotationMergePass.hpp` | Merge adjacent rotation gates (Rz(a)Rz(b) = Rz(a+b)) | ~10 |
| `IdentityEliminationPass.hpp` | Remove identity gates (Rz(0), etc.) | ~5 |

### Pass Architecture

```cpp
// Base class for all passes
class Pass {
public:
    virtual ~Pass() = default;
    
    [[nodiscard]] virtual std::string name() const = 0;
    virtual void run(DAG& dag) = 0;
    
    // Statistics
    [[nodiscard]] size_t gatesRemoved() const noexcept;
    [[nodiscard]] size_t gatesAdded() const noexcept;
};

// Pipeline manager
class PassManager {
public:
    void addPass(std::unique_ptr<Pass> pass);
    
    void run(DAG& dag);
    void run(Circuit& circuit);  // Converts to DAG internally
    
    [[nodiscard]] PassStatistics statistics() const;
};
```

### Optimization Strategies

| Pass | Strategy | Expected Reduction |
|------|----------|-------------------|
| **Cancellation** | Find adjacent inverse pairs on same qubits | 5-15% |
| **Commutation** | Move gates through commuting neighbors to enable cancellation | 2-5% |
| **Rotation Merge** | Combine Rz(a)Rz(b) -> Rz(a+b) | 5-10% |
| **Identity Elimination** | Remove Rz(0), etc. | 1-3% |

### Commutation Rules

```
[H, Z] = 0  (up to global phase)
[CNOT, Z_control] = 0
[CNOT, X_target] = 0
[Rz(a), Rz(b)] = 0
[CZ, Z_either] = 0
```

### Acceptance Criteria

- [x] PassManager runs passes in sequence
- [x] CancellationPass removes inverse pairs
- [x] RotationMergePass combines rotations correctly
- [x] Passes preserve circuit semantics (validated by simulation)
- [x] >=40 tests pass (64 new tests, 277 total)

---

## Sprint 4: Qubit Routing

**Goal:** Map logical qubits to physical topology with SWAP insertion  
**Exit Criteria:** Router produces valid physical circuits for various topologies with >=30 tests

### Deliverables

| Component | Description | Tests |
|-----------|-------------|-------|
| `Topology.hpp` | Graph representation of qubit connectivity | ~10 |
| `Router.hpp` | Base class for routing algorithms | - |
| `SabreRouter.hpp` | SABRE routing algorithm implementation | ~15 |
| `RoutingResult.hpp` | Result container with mapping and statistics | - |
| `topologies/` | Predefined topologies (linear, grid, heavy-hex) | ~5 |

### Topology API

```cpp
class Topology {
public:
    explicit Topology(size_t numQubits);
    
    void addEdge(size_t q1, size_t q2);
    [[nodiscard]] bool connected(size_t q1, size_t q2) const;
    [[nodiscard]] size_t distance(size_t q1, size_t q2) const;
    [[nodiscard]] std::vector<size_t> neighbors(size_t qubit) const;
    
    // Predefined topologies
    static Topology linear(size_t n);
    static Topology grid(size_t rows, size_t cols);
    static Topology heavyHex(size_t distance);
};
```

### Router API

```cpp
struct RoutingResult {
    Circuit routed_circuit;
    std::vector<size_t> initial_mapping;  // logical -> physical
    size_t swaps_inserted;
    size_t depth_overhead;
};

class Router {
public:
    virtual ~Router() = default;
    
    [[nodiscard]] virtual RoutingResult route(
        const Circuit& circuit,
        const Topology& topology
    ) = 0;
};

// SABRE implementation
class SabreRouter : public Router {
public:
    SabreRouter(size_t lookahead = 20, size_t decay = 0.001);
    
    [[nodiscard]] RoutingResult route(
        const Circuit& circuit,
        const Topology& topology
    ) override;
};
```

### SABRE Algorithm

Reference: Li et al., "Tackling the Qubit Mapping Problem for NISQ-Era Quantum Devices" (2019)

1. **Initial mapping**: Heuristic based on gate frequency
2. **Forward pass**: Process gates, insert SWAPs when needed
3. **Backward pass**: Refine mapping by processing in reverse
4. **Lookahead**: Consider future gates when choosing SWAPs

### Acceptance Criteria

- [x] Topology correctly represents device connectivity
- [x] SabreRouter produces valid physical circuits
- [x] All two-qubit gates in output are on connected qubits
- [x] Router works on linear, grid, and heavy-hex topologies
- [x] SWAP overhead is reasonable (< 3x gate increase for typical circuits)
- [x] >=30 tests pass (63 new tests, 340 total)

---

## Sprint 5: Polish & Documentation

**Goal:** Production-ready release with comprehensive documentation  
**Exit Criteria:** All documentation complete, benchmarks run, release tagged

### Deliverables

| Component | Description |
|-----------|-------------|
| API Documentation | Doxygen-generated docs for all public APIs |
| User Guide | Tutorial-style guide with examples |
| Benchmarks | Performance benchmarks with standard circuits |
| Examples | Example programs demonstrating usage |
| Release | Tagged v1.0.0 release with changelog |

### Documentation Structure

```
docs/
├── index.md                 # Overview and quick start
├── building.md              # Build instructions
├── tutorial/
│   ├── 01-circuits.md       # Working with circuits
│   ├── 02-parsing.md        # Parsing OpenQASM
│   ├── 03-optimization.md   # Optimization passes
│   └── 04-routing.md        # Qubit routing
├── api/                     # Generated Doxygen docs
├── architecture.md          # Design decisions
└── benchmarks.md            # Performance results
```

### Benchmark Suite

| Circuit | Qubits | Gates | Purpose |
|---------|--------|-------|---------|
| QFT | 4-20 | O(n^2) | Scalability |
| Random | 10, 20, 50 | 100-1000 | Typical workload |
| Adder | 8-32 | O(n) | Practical circuit |
| QAOA | 10-20 | O(n^2) | Variational |

### Acceptance Criteria

- [x] All public APIs have Doxygen documentation
- [x] User guide covers all major features
- [x] Examples compile and run
- [x] Benchmarks produce reproducible results
- [x] CHANGELOG.md documents all changes
- [x] README.md updated with final status
- [ ] GitHub release tagged v1.0.0

---

## Architecture Overview

```
quantum-circuit-optimizer/
├── include/
│   ├── ir/
│   │   ├── Types.hpp           # Type aliases, constants
│   │   ├── Gate.hpp            # Gate representation
│   │   ├── Qubit.hpp           # Qubit utilities
│   │   ├── Circuit.hpp         # Circuit container
│   │   └── DAG.hpp             # DAG IR
│   ├── parser/
│   │   ├── Lexer.hpp           # Tokenizer
│   │   ├── Parser.hpp          # QASM parser
│   │   └── QASMError.hpp       # Error types
│   ├── passes/
│   │   ├── Pass.hpp            # Base class
│   │   ├── PassManager.hpp     # Pipeline
│   │   ├── CancellationPass.hpp
│   │   ├── CommutationPass.hpp
│   │   ├── RotationMergePass.hpp
│   │   └── IdentityEliminationPass.hpp
│   └── routing/
│       ├── Topology.hpp        # Device topology
│       ├── Router.hpp          # Base class
│       └── SabreRouter.hpp     # SABRE implementation
├── src/
│   ├── ir/
│   ├── parser/
│   ├── passes/
│   ├── routing/
│   └── main.cpp
├── tests/
│   ├── ir/
│   ├── parser/
│   ├── passes/
│   └── routing/
├── benchmarks/
├── examples/
├── docs/
├── CMakeLists.txt
├── README.md
├── SCOPE_OF_WORK.md            # This file
├── CHANGELOG.md
└── LICENSE
```

---

## Test Coverage Goals

| Sprint | Component | Target Tests | Coverage |
|--------|-----------|--------------|----------|
| 1A | Gate, Circuit | 50 | 90% |
| 1B | DAG | 54 | 90% |
| 2 | Parser | 109 | 85% |
| 3 | Passes | 64 | 85% |
| 4 | Routing | 63 | 80% |
| **Total** | - | **340** | **85%** |

---

## Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| CMake | >= 3.18 | Build system |
| C++ Compiler | C++17 | GCC 9+, Clang 10+ |
| GoogleTest | v1.14.0 | Testing framework |

**No external runtime dependencies.** All functionality is self-contained.

---

## Risk Register

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| QASM 3.0 complexity | Medium | High | Support subset, add features incrementally |
| SABRE performance | Low | Medium | Profile early, optimize critical paths |
| Scope creep | Medium | Medium | Stick to sprint boundaries, defer features |
| Test coverage gaps | Low | High | Enforce coverage in CI |

---

## Success Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Test coverage | >= 85% | gcov/lcov |
| Build time | < 30s | CI timing |
| Optimization reduction | >= 15% | Benchmark suite |
| Routing overhead | < 3x SWAP increase | Benchmark suite |
| Documentation | 100% public API | Doxygen warnings = 0 |

---

## References

1. Nielsen & Chuang, "Quantum Computation and Quantum Information" (2010)
2. Kahn, A.B., "Topological sorting of large networks", Comm. ACM 5(11):558-562 (1962)
3. Li et al., "Tackling the Qubit Mapping Problem for NISQ-Era Quantum Devices" (2019)
4. OpenQASM 3.0 Specification: https://openqasm.com/

---

## Changelog

| Date | Sprint | Changes |
|------|--------|---------|
| 2025-12 | 1A | Foundation complete: Gate, Circuit, 50 tests |
| 2025-12 | 1B | DAG IR complete: DAGNode, DAG, 54 tests, 104 total |
| 2025-12 | 2 | Parser complete: Token, Lexer, Parser, QASMError, 109 tests, 213 total |
| 2025-12 | 3 | Optimization passes complete: Pass, PassManager, 4 passes, 64 tests, 277 total |
| 2025-12 | 4 | Routing complete: Topology, Router, SabreRouter, 63 tests, 340 total |
| 2025-12 | 5 | Documentation complete: Doxygen, tutorials, examples, benchmarks, CHANGELOG |

---

**License:** MIT  
**Author:** Rylan Malarchick (rylan1012@gmail.com)
