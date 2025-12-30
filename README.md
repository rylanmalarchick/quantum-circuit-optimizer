# Quantum Circuit Optimizer

A production-quality quantum circuit compiler in C++17 that parses OpenQASM 3.0, optimizes circuits through multiple passes, and performs topology-aware qubit routing.

[![Build Status](https://github.com/username/quantum-circuit-optimizer/workflows/CI/badge.svg)](https://github.com/username/quantum-circuit-optimizer/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)

## Overview

This project implements a complete quantum circuit compilation pipeline:

- **Circuit IR**: DAG-based intermediate representation for quantum circuits
- **Parser**: OpenQASM 3.0 parser with error recovery
- **Optimization Passes**: Gate cancellation, commutation, rotation merging
- **Qubit Routing**: SABRE algorithm for topology-aware mapping

## Features

| Feature | Description |
|---------|-------------|
| **OpenQASM 3.0 Parser** | Parse quantum circuits from standard format |
| **DAG Representation** | Efficient dependency graph for optimization |
| **4 Optimization Passes** | Reduce gate count by 10-30% |
| **SABRE Routing** | State-of-the-art qubit mapping algorithm |
| **Multiple Topologies** | Linear, ring, grid, heavy-hex support |
| **340 Unit Tests** | Comprehensive test coverage |

## Quick Start

```bash
# Clone and build
git clone https://github.com/username/quantum-circuit-optimizer.git
cd quantum-circuit-optimizer
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run tests
ctest --test-dir build

# Run examples
./build/examples/basic_usage
```

## Usage

### Basic Example

```cpp
#include "parser/Parser.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

using namespace qopt;

int main() {
    // Parse an OpenQASM circuit
    auto circuit = parser::parseQASM(R"(
        OPENQASM 3.0;
        qubit[4] q;
        h q[0];
        cx q[0], q[1];
        cx q[1], q[2];
        cx q[2], q[3];
    )");
    
    // Optimize
    passes::PassManager pm;
    pm.addPass(std::make_unique<passes::CancellationPass>());
    pm.run(*circuit);
    
    // Route to hardware topology
    auto topology = routing::Topology::grid(2, 2);
    routing::SabreRouter router;
    auto result = router.route(*circuit, topology);
    
    std::cout << "Routed gates: " << result.routed_circuit.numGates() << "\n";
    std::cout << "SWAPs inserted: " << result.swaps_inserted << "\n";
    
    return 0;
}
```

### Supported Gates

| Gate | Factory Method | Description |
|------|----------------|-------------|
| H | `Gate::h(qubit)` | Hadamard |
| X, Y, Z | `Gate::x(qubit)`, etc. | Pauli gates |
| S, Sdg | `Gate::s(qubit)`, `Gate::sdg(qubit)` | S and S-dagger |
| T, Tdg | `Gate::t(qubit)`, `Gate::tdg(qubit)` | T and T-dagger |
| Rx, Ry, Rz | `Gate::rx(qubit, angle)`, etc. | Rotation gates |
| CNOT | `Gate::cnot(control, target)` | Controlled-NOT |
| CZ | `Gate::cz(control, target)` | Controlled-Z |
| SWAP | `Gate::swap(q1, q2)` | SWAP gate |

### Optimization Passes

| Pass | Description | Typical Reduction |
|------|-------------|------------------|
| CancellationPass | Remove inverse pairs (HH, XX, CNOT·CNOT) | 5-15% |
| CommutationPass | Reorder gates to expose optimizations | 2-5% |
| RotationMergePass | Combine rotations: Rz(a)Rz(b) → Rz(a+b) | 5-10% |
| IdentityEliminationPass | Remove identity gates: Rz(0) | 1-3% |

### Hardware Topologies

```cpp
// Predefined topologies
auto linear = Topology::linear(5);     // 0—1—2—3—4
auto ring = Topology::ring(5);         // 0—1—2—3—4—0
auto grid = Topology::grid(3, 3);      // 3x3 lattice
auto heavy = Topology::heavyHex(3);    // IBM-style heavy-hex

// Custom topology
Topology custom(4);
custom.addEdge(0, 1);
custom.addEdge(1, 2);
custom.addEdge(2, 3);
custom.addEdge(0, 3);  // Square
```

## Project Structure

```
quantum-circuit-optimizer/
├── include/
│   ├── ir/                    # Intermediate Representation
│   │   ├── Gate.hpp           # Gate representation
│   │   ├── Circuit.hpp        # Circuit container
│   │   └── DAG.hpp            # DAG for optimization
│   ├── parser/                # OpenQASM 3.0 Parser
│   │   ├── Lexer.hpp          # Tokenizer
│   │   ├── Parser.hpp         # Recursive descent parser
│   │   └── QASMError.hpp      # Error handling
│   ├── passes/                # Optimization Passes
│   │   ├── Pass.hpp           # Base class
│   │   ├── PassManager.hpp    # Pass pipeline
│   │   └── *Pass.hpp          # Individual passes
│   └── routing/               # Qubit Routing
│       ├── Topology.hpp       # Device topology
│       └── SabreRouter.hpp    # SABRE algorithm
├── tests/                     # 340 unit tests
├── examples/                  # Example programs
├── benchmarks/                # Performance benchmarks
└── docs/                      # Documentation
```

## Building

### Prerequisites

- CMake 3.18+
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Git (for fetching GoogleTest)

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug | Build type (Debug, Release) |
| `BUILD_TESTING` | ON | Build test suite |
| `BUILD_EXAMPLES` | ON | Build example programs |
| `BUILD_BENCHMARKS` | OFF | Build benchmarks |
| `BUILD_DOCS` | OFF | Build Doxygen documentation |

### Building Documentation

```bash
# Requires Doxygen and Graphviz
cmake -B build -DBUILD_DOCS=ON
cmake --build build --target docs
# Open docs/api/html/index.html
```

## Test Results

```
340 tests passed, 0 tests failed

Test Categories:
  Gate tests:          26
  Circuit tests:       24
  DAG tests:           54
  Lexer tests:         58
  Parser tests:        51
  Optimization tests:  64
  Routing tests:       63
```

## Benchmarks

Run the benchmark suite:

```bash
cmake -B build -DBUILD_BENCHMARKS=ON
cmake --build build
./build/benchmarks/benchmark_circuits
```

Sample results:

| Circuit | Qubits | Original | Optimized | Routed | SWAPs | Opt % |
|---------|--------|----------|-----------|--------|-------|-------|
| QFT-8 | 8 | 148 | 140 | 158 | 6 | 5.4% |
| Random-20x500 | 20 | 500 | 465 | 512 | 15 | 7.0% |
| QAOA-10-p2 | 10 | 52 | 48 | 52 | 2 | 7.7% |

## Documentation

- [Building from Source](docs/building.md)
- [Architecture Overview](docs/architecture.md)
- **Tutorials:**
  - [Working with Circuits](docs/tutorial/01-circuits.md)
  - [Parsing OpenQASM](docs/tutorial/02-parsing.md)
  - [Optimization Passes](docs/tutorial/03-optimization.md)
  - [Qubit Routing](docs/tutorial/04-routing.md)

## Roadmap

- [x] **Sprint 1A**: Foundation (Gate, Circuit, build system, CI)
- [x] **Sprint 1B**: DAG IR for optimization
- [x] **Sprint 2**: OpenQASM 3.0 parser
- [x] **Sprint 3**: Optimization passes
- [x] **Sprint 4**: Qubit routing (SABRE)
- [x] **Sprint 5**: Documentation and polish

### Future Plans

- Gate definitions and custom gates
- Classical control flow
- Additional routing algorithms
- Noise-aware optimization
- Python bindings

## References

1. Li et al., "Tackling the Qubit Mapping Problem for NISQ-Era Quantum Devices", ASPLOS 2019
2. OpenQASM 3.0 Specification: https://openqasm.com/
3. Kahn, A.B., "Topological sorting of large networks", Comm. ACM 5(11), 1962

## License

MIT License - see [LICENSE](LICENSE) for details.

## Author

Rylan Malarchick (rylan1012@gmail.com)

## Contributing

Contributions are welcome! Please read the documentation and ensure all tests pass before submitting a PR.

```bash
# Run tests before committing
cmake --build build && ctest --test-dir build --output-on-failure
```
