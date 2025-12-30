# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-12-30

### Added

#### Sprint 1A: Foundation
- **Gate representation** (`include/ir/Gate.hpp`)
  - Immutable gate class with factory methods: `Gate::h()`, `Gate::cnot()`, `Gate::rz()`, etc.
  - Support for single-qubit gates: H, X, Y, Z, S, Sdg, T, Tdg
  - Support for rotation gates: Rx, Ry, Rz with angle parameters
  - Support for two-qubit gates: CNOT, CZ, SWAP
  - Gate validation, equality operators, and string representation

- **Circuit container** (`include/ir/Circuit.hpp`)
  - Linear sequence of gates with qubit register management
  - Gate addition, iteration, and indexing
  - Circuit metrics: depth calculation, gate counting by type
  - Deep copy via `clone()` method

- **Type definitions** (`include/ir/Types.hpp`)
  - Common type aliases: `QubitIndex`, `GateId`, `Angle`
  - Constants: `MAX_QUBITS`, `INVALID_GATE_ID`

- **Build system** (`CMakeLists.txt`)
  - CMake 3.18+ with C++17 requirement
  - GoogleTest integration via FetchContent
  - Strict warning flags: `-Wall -Wextra -Wpedantic -Werror`

- **CI/CD** (`.github/workflows/ci.yml`)
  - GitHub Actions pipeline for Linux builds
  - Automatic test execution

#### Sprint 1B: DAG IR
- **DAG representation** (`include/ir/DAG.hpp`)
  - DAGNode class wrapping gates with predecessor/successor tracking
  - DAG class for circuit dependency graph
  - Topological sort via Kahn's algorithm (with citation)
  - Layer extraction for parallel execution analysis
  - Conversion: `DAG::fromCircuit()` and `dag.toCircuit()`
  - Node management: add, remove, query

#### Sprint 2: OpenQASM 3.0 Parser
- **Tokenizer** (`include/parser/Lexer.hpp`)
  - Full tokenization of OpenQASM 3.0 subset
  - Line and column tracking for error reporting
  - Support for comments (single-line and multi-line)

- **Token types** (`include/parser/Token.hpp`)
  - Complete token type enumeration
  - Token classification helpers: `isGate()`, `isParameterizedGate()`, etc.

- **Parser** (`include/parser/Parser.hpp`)
  - Recursive descent parser for OpenQASM 3.0
  - Version declaration parsing
  - Qubit and bit register declarations
  - Gate application parsing with parameter expressions
  - Measurement statement parsing
  - Error recovery with multiple error reporting
  - Convenience function: `parseQASM()`

- **Error handling** (`include/parser/QASMError.hpp`)
  - Structured error type with source location
  - Error kinds: Syntax, Semantic, Internal
  - Exception type for parse failures

#### Sprint 3: Optimization Passes
- **Pass infrastructure** (`include/passes/Pass.hpp`)
  - Abstract base class for optimization passes
  - Statistics tracking: gates removed/added

- **Pass manager** (`include/passes/PassManager.hpp`)
  - Pipeline for sequential pass execution
  - Aggregated statistics with per-pass breakdown
  - Convenience method for Circuit optimization

- **CancellationPass** (`include/passes/CancellationPass.hpp`)
  - Removes adjacent inverse gate pairs
  - Handles: H-H, X-X, Y-Y, Z-Z, CNOT-CNOT, CZ-CZ, SWAP-SWAP

- **CommutationPass** (`include/passes/CommutationPass.hpp`)
  - Reorders commuting gates to expose optimization opportunities
  - Implements standard commutation rules

- **RotationMergePass** (`include/passes/RotationMergePass.hpp`)
  - Merges adjacent rotation gates: Rz(a)Rz(b) → Rz(a+b)
  - Handles Rx, Ry, Rz on same qubit

- **IdentityEliminationPass** (`include/passes/IdentityEliminationPass.hpp`)
  - Removes identity rotations: Rz(0), Rx(2π), etc.
  - Configurable angle tolerance

#### Sprint 4: Qubit Routing
- **Topology representation** (`include/routing/Topology.hpp`)
  - Graph-based hardware connectivity model
  - Factory methods: `linear()`, `ring()`, `grid()`, `heavyHex()`
  - Distance computation with BFS and caching
  - Shortest path queries
  - Neighbor listing

- **Router interface** (`include/routing/Router.hpp`)
  - Abstract Router base class
  - RoutingResult container with statistics
  - TrivialRouter for testing (identity mapping)

- **SABRE router** (`include/routing/SabreRouter.hpp`)
  - State-of-the-art SABRE algorithm implementation
  - Reference: Li et al., ASPLOS 2019
  - Heuristic SWAP selection with lookahead
  - Configurable parameters: lookahead depth, decay factor, extended set weight

#### Sprint 5: Polish & Documentation
- **Doxygen documentation** (`docs/Doxyfile`)
  - Configuration for API documentation generation
  - Class diagrams enabled
  - Markdown support

- **User documentation**
  - `docs/index.md`: Overview and quick start
  - `docs/building.md`: Build instructions
  - `docs/architecture.md`: Design documentation

- **Tutorials**
  - `docs/tutorial/01-circuits.md`: Working with circuits
  - `docs/tutorial/02-parsing.md`: Parsing OpenQASM
  - `docs/tutorial/03-optimization.md`: Optimization passes
  - `docs/tutorial/04-routing.md`: Qubit routing

- **Examples**
  - `examples/basic_usage.cpp`: End-to-end workflow
  - `examples/optimization_demo.cpp`: Detailed pass demonstration
  - `examples/routing_demo.cpp`: Topology and routing examples

- **Benchmarks**
  - `benchmarks/benchmark_circuits.cpp`: Performance benchmark suite
  - QFT, random circuits, adder, QAOA circuit generators

### Test Coverage
- **Total: 340 tests**
  - Gate tests: 26
  - Circuit tests: 24
  - DAG tests: 54
  - Lexer tests: 58
  - Parser tests: 51
  - Optimization pass tests: 64
  - Routing tests: 63

### Dependencies
- CMake >= 3.18
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- GoogleTest v1.14.0 (fetched automatically)

## [Unreleased]

### Planned
- Gate definitions (`gate mygate(a) q { ... }`)
- Classical control (`if (c == 1) x q[0];`)
- Loop constructs (`for`, `while`)
- Additional routing algorithms (A*, simulated annealing)
- Gate synthesis and decomposition
- Noise-aware optimization

---

**License:** MIT  
**Author:** Rylan Malarchick (rylan1012@gmail.com)
