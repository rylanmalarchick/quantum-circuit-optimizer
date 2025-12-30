# Quantum Circuit Optimizer

A production-quality quantum circuit compiler written in C++17.

## Overview

The Quantum Circuit Optimizer provides a complete toolchain for:

1. **Parsing** OpenQASM 3.0 circuit descriptions
2. **Representing** circuits as a DAG-based intermediate representation
3. **Optimizing** circuits through multiple transformation passes
4. **Routing** qubits to hardware topologies with SWAP insertion

## Quick Start

```cpp
#include "parser/Parser.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

int main() {
    // Parse an OpenQASM 3.0 circuit
    auto circuit = qopt::parseQASM(R"(
        OPENQASM 3.0;
        qubit[3] q;
        h q[0];
        cx q[0], q[1];
        cx q[1], q[2];
    )");
    
    // Optimize the circuit
    qopt::PassManager pm;
    pm.addPass(std::make_unique<qopt::CancellationPass>());
    pm.addPass(std::make_unique<qopt::RotationMergePass>());
    pm.run(circuit);
    
    // Route to hardware topology
    auto topology = qopt::Topology::linear(3);
    qopt::SabreRouter router;
    auto result = router.route(circuit, topology);
    
    std::cout << "Original gates: " << circuit.size() << "\n";
    std::cout << "Routed gates: " << result.routed_circuit.size() << "\n";
    std::cout << "SWAPs inserted: " << result.swaps_inserted << "\n";
    
    return 0;
}
```

## Features

### Intermediate Representation

- **Gate**: Immutable gate representation with factory methods
- **Circuit**: Linear sequence container with depth calculation
- **DAG**: Directed acyclic graph for optimization

### Parser

- Full OpenQASM 3.0 subset support
- Qubit/bit register declarations
- Standard gate set (H, X, Y, Z, CX, CZ, RX, RY, RZ, etc.)
- Measurement operations
- Detailed error reporting with line/column information

### Optimization Passes

| Pass | Description |
|------|-------------|
| CancellationPass | Removes adjacent inverse gate pairs (HH, XX, etc.) |
| CommutationPass | Reorders commuting gates to enable further optimization |
| RotationMergePass | Combines adjacent rotations: Rz(a)Rz(b) → Rz(a+b) |
| IdentityEliminationPass | Removes identity rotations: Rz(0) |

### Qubit Routing

- **Topology**: Graph-based hardware connectivity representation
- **Factory topologies**: Linear, ring, grid, heavy-hex
- **SABRE Router**: State-of-the-art routing algorithm with SWAP insertion
- **Configurable**: Lookahead depth, decay factor, extended set weight

## Documentation

- @subpage building "Building from Source"
- @subpage architecture "Architecture Overview"
- @ref tutorial/01-circuits.md "Tutorial: Working with Circuits"
- @ref tutorial/02-parsing.md "Tutorial: Parsing OpenQASM"
- @ref tutorial/03-optimization.md "Tutorial: Optimization Passes"
- @ref tutorial/04-routing.md "Tutorial: Qubit Routing"

## API Reference

### Core IR

- @ref qopt::Gate "Gate" - Quantum gate representation
- @ref qopt::Circuit "Circuit" - Circuit container
- @ref qopt::DAG "DAG" - DAG intermediate representation
- @ref qopt::DAGNode "DAGNode" - DAG node wrapper

### Parser

- @ref qopt::Parser "Parser" - OpenQASM 3.0 parser
- @ref qopt::Lexer "Lexer" - Tokenizer
- @ref qopt::QASMError "QASMError" - Error reporting

### Optimization

- @ref qopt::Pass "Pass" - Base optimization pass
- @ref qopt::PassManager "PassManager" - Pass pipeline manager
- @ref qopt::CancellationPass "CancellationPass"
- @ref qopt::CommutationPass "CommutationPass"
- @ref qopt::RotationMergePass "RotationMergePass"
- @ref qopt::IdentityEliminationPass "IdentityEliminationPass"

### Routing

- @ref qopt::Topology "Topology" - Hardware topology
- @ref qopt::Router "Router" - Base router class
- @ref qopt::SabreRouter "SabreRouter" - SABRE algorithm implementation
- @ref qopt::RoutingResult "RoutingResult" - Routing result container

## Requirements

- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.18+
- GoogleTest (fetched automatically)

## License

MIT License. See LICENSE file for details.

## References

1. Nielsen & Chuang, "Quantum Computation and Quantum Information" (2010)
2. Li et al., "Tackling the Qubit Mapping Problem for NISQ-Era Quantum Devices" (ASPLOS 2019)
3. OpenQASM 3.0 Specification: https://openqasm.com/
