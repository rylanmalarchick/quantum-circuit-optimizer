# Architecture Overview {#architecture}

This document describes the architecture and design decisions of the Quantum Circuit Optimizer.

## Project Structure

```
quantum-circuit-optimizer/
├── include/
│   ├── ir/                    # Intermediate Representation
│   │   ├── Types.hpp          # Type aliases, constants
│   │   ├── Gate.hpp           # Gate representation
│   │   ├── Qubit.hpp          # Qubit utilities
│   │   ├── Circuit.hpp        # Circuit container
│   │   └── DAG.hpp            # DAG IR
│   ├── parser/                # OpenQASM 3.0 Parser
│   │   ├── Token.hpp          # Token types
│   │   ├── Lexer.hpp          # Tokenizer
│   │   ├── Parser.hpp         # Recursive descent parser
│   │   └── QASMError.hpp      # Error reporting
│   ├── passes/                # Optimization Passes
│   │   ├── Pass.hpp           # Base class
│   │   ├── PassManager.hpp    # Pass pipeline
│   │   ├── CancellationPass.hpp
│   │   ├── CommutationPass.hpp
│   │   ├── RotationMergePass.hpp
│   │   └── IdentityEliminationPass.hpp
│   └── routing/               # Qubit Routing
│       ├── Topology.hpp       # Device topology
│       ├── Router.hpp         # Base router class
│       └── SabreRouter.hpp    # SABRE algorithm
├── tests/                     # Test suite
├── examples/                  # Example programs
├── benchmarks/                # Performance benchmarks
└── docs/                      # Documentation
```

## Design Principles

### 1. Header-Only Library

The entire library is implemented in header files for:
- Easy integration (just include headers)
- Better inlining opportunities
- Simpler build configuration

### 2. Immutable Gates

Gates are immutable value types created via factory methods:

```cpp
auto h = Gate::h(0);           // Hadamard on qubit 0
auto cx = Gate::cnot(0, 1);    // CNOT with control 0, target 1
auto rz = Gate::rz(0, M_PI/4); // Rz(π/4) on qubit 0
```

Benefits:
- Thread-safe by construction
- Easy to reason about
- Efficient copying (small size)

### 3. Validation at Boundaries

All public methods validate their inputs and throw descriptive exceptions:

```cpp
auto gate = Gate::cnot(0, 0);  // throws: "Control and target qubits must differ"
circuit.addGate(Gate::h(100)); // throws: "Qubit index 100 exceeds circuit size 5"
```

Internal methods assume valid inputs for performance.

### 4. RAII and Smart Pointers

Memory is managed via RAII with smart pointers:

```cpp
// DAG owns nodes via unique_ptr
std::vector<std::unique_ptr<DAGNode>> nodes_;

// Edges are non-owning raw pointers
std::vector<DAGNode*> predecessors_;
std::vector<DAGNode*> successors_;
```

No raw `new`/`delete` in user code.

### 5. Algorithm Citations

Important algorithms include academic citations:

```cpp
/**
 * @brief Perform topological sort using Kahn's algorithm.
 * 
 * Reference: Kahn, A.B., "Topological sorting of large networks",
 * Communications of the ACM 5(11):558-562 (1962)
 */
std::vector<DAGNode*> topologicalOrder() const;
```

## Component Details

### Intermediate Representation

The IR has two levels:

1. **Circuit**: Linear sequence of gates, efficient for I/O
2. **DAG**: Graph representation, efficient for analysis and transformation

```
                    ┌─────────┐
    OpenQASM ──────>│ Parser  │──────> Circuit
                    └─────────┘           │
                                          │ fromCircuit()
                                          v
                                    ┌─────────┐
                                    │   DAG   │<──── Optimization passes
                                    └─────────┘
                                          │
                                          │ toCircuit()
                                          v
                                       Circuit ──────> Output
```

### Parser Architecture

The parser uses a two-phase approach:

1. **Lexer**: Converts source text to tokens
2. **Parser**: Recursive descent parser consuming tokens

```
Source → Lexer → Token Stream → Parser → Circuit
                      ↓
                 Error recovery
```

Error recovery allows reporting multiple errors per parse.

### Optimization Pass Pipeline

Passes follow a simple interface:

```cpp
class Pass {
public:
    virtual std::string name() const = 0;
    virtual void run(DAG& dag) = 0;
};
```

The PassManager executes passes in sequence:

```cpp
PassManager pm;
pm.addPass(std::make_unique<CommutationPass>());
pm.addPass(std::make_unique<CancellationPass>());
pm.addPass(std::make_unique<RotationMergePass>());
pm.run(circuit);  // Converts to DAG internally
```

### Routing Architecture

Routing maps logical qubits to physical topology:

```
Logical Circuit ──> Router ──> Physical Circuit
                      ↑
                  Topology
```

The SABRE algorithm:
1. Assigns initial logical-to-physical mapping
2. Processes gates in topological order
3. Inserts SWAPs when two-qubit gates span non-adjacent qubits
4. Uses lookahead heuristics to minimize SWAP count

## Thread Safety

The library is **not thread-safe** by design. Rationale:
- Quantum circuits are typically processed single-threaded
- Adding locks would reduce performance
- Users can add their own synchronization if needed

## Performance Considerations

### Time Complexity

| Operation | Complexity |
|-----------|------------|
| Circuit::addGate | O(1) amortized |
| DAG::fromCircuit | O(n) |
| DAG::toCircuit | O(n log n) |
| DAG::topologicalOrder | O(n + e) |
| CancellationPass | O(n) |
| SabreRouter | O(n^2) typical |

Where n = number of gates, e = number of edges.

### Space Complexity

| Structure | Space |
|-----------|-------|
| Gate | ~48 bytes |
| Circuit | O(n) |
| DAG | O(n + e) |
| Topology | O(q^2) |

Where q = number of qubits.

## Extensibility

### Adding New Gate Types

1. Add enum value to `GateType` in `Gate.hpp`
2. Add factory method to `Gate` class
3. Update `inverse()`, `isParameterized()`, etc.
4. Update parser's gate recognition

### Adding New Optimization Passes

1. Create header in `include/passes/`
2. Inherit from `Pass`
3. Implement `name()` and `run(DAG&)`
4. Add tests in `tests/passes/`

### Adding New Topologies

Use the `Topology` class's `addEdge()` method or create factory functions:

```cpp
Topology Topology::myCustomTopology(size_t n) {
    Topology t(n);
    for (size_t i = 0; i < n; ++i) {
        t.addEdge(i, (i + 1) % n);  // Ring
        t.addEdge(i, (i + 2) % n);  // Skip connections
    }
    return t;
}
```

## Testing Strategy

### Test Levels

1. **Unit tests**: Individual functions/classes
2. **Integration tests**: Component interactions
3. **Round-trip tests**: Parse → transform → emit → parse

### Test Organization

```
tests/
├── ir/
│   ├── test_gate.cpp      # Gate unit tests
│   ├── test_circuit.cpp   # Circuit unit tests
│   └── test_dag.cpp       # DAG unit tests
├── parser/
│   ├── test_lexer.cpp     # Lexer unit tests
│   └── test_parser.cpp    # Parser unit tests
├── passes/
│   └── test_passes.cpp    # Optimization pass tests
└── routing/
    └── test_routing.cpp   # Routing tests
```

### Coverage Goals

Target: 85%+ line coverage for all components.
