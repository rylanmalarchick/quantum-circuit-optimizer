# Tutorial: Qubit Routing {#tutorial-routing}

This tutorial covers mapping logical circuits to hardware topologies.

## Overview

Real quantum hardware has limited qubit connectivity. Two-qubit gates can only operate on physically connected qubits. The router maps logical qubits to physical qubits and inserts SWAP gates when needed.

```
Logical Circuit  ──→  Router  ──→  Physical Circuit
                        ↑
                    Topology
```

## Creating Topologies

### Predefined Topologies

```cpp
#include "routing/Topology.hpp"

using namespace qopt;

// Linear chain: 0—1—2—3—4
auto linear = Topology::linear(5);

// Ring: 0—1—2—3—4—0
auto ring = Topology::ring(5);

// Grid: 
// 0—1—2
// |   |   |
// 3—4—5
// |   |   |
// 6—7—8
auto grid = Topology::grid(3, 3);

// Heavy-hex (IBM-style):
auto heavyHex = Topology::heavyHex(3);
```

### Custom Topologies

```cpp
// Create a custom 5-qubit topology
Topology custom(5);
custom.addEdge(0, 1);
custom.addEdge(1, 2);
custom.addEdge(2, 3);
custom.addEdge(3, 4);
custom.addEdge(0, 4);  // Extra connection
custom.addEdge(1, 3);  // Extra connection
```

## Querying Topologies

```cpp
Topology t = Topology::linear(5);

// Basic properties
t.numQubits();           // 5

// Connectivity queries
t.isConnected(0, 1);     // true (adjacent)
t.isConnected(0, 2);     // false (not adjacent)
t.isConnected(0, 4);     // false

// Neighbors
auto n = t.neighbors(2); // {1, 3}

// Distance (shortest path length)
t.distance(0, 4);        // 4

// Shortest path
auto path = t.shortestPath(0, 4);  // {0, 1, 2, 3, 4}
```

## Basic Routing

```cpp
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

using namespace qopt;

// Create a circuit that needs routing
Circuit circuit(4);
circuit.addGate(Gate::h(0));
circuit.addGate(Gate::cnot(0, 3));  // Qubits 0 and 3 are not adjacent!

// Create topology (linear: 0—1—2—3)
auto topology = Topology::linear(4);

// Route the circuit
SabreRouter router;
RoutingResult result = router.route(circuit, topology);

// Examine results
std::cout << "Original gates: " << circuit.size() << "\n";
std::cout << "Routed gates: " << result.routed_circuit.size() << "\n";
std::cout << "SWAPs inserted: " << result.swaps_inserted << "\n";
```

## Understanding RoutingResult

```cpp
struct RoutingResult {
    Circuit routed_circuit;              // Physical circuit with SWAPs
    std::vector<size_t> initial_mapping; // logical → physical mapping
    std::vector<size_t> final_mapping;   // mapping after routing
    size_t swaps_inserted;               // number of SWAP gates added
    size_t depth_overhead;               // additional depth from SWAPs
};

RoutingResult result = router.route(circuit, topology);

// Initial mapping: logical qubit i → physical qubit initial_mapping[i]
for (size_t i = 0; i < result.initial_mapping.size(); ++i) {
    std::cout << "Logical " << i << " → Physical " 
              << result.initial_mapping[i] << "\n";
}

// The routed circuit operates on physical qubits
for (const Gate& gate : result.routed_circuit) {
    std::cout << gate.name() << " on physical qubits: ";
    for (size_t q : gate.qubits()) {
        std::cout << q << " ";
    }
    std::cout << "\n";
}
```

## SabreRouter Options

```cpp
#include "routing/SabreRouter.hpp"

// Configure SABRE parameters
SabreRouter router;
router.setLookaheadDepth(20);      // How far ahead to look (default: 20)
router.setDecayFactor(0.001);      // Decay for extended set (default: 0.001)
router.setExtendedSetWeight(0.5); // Weight of lookahead (default: 0.5)

RoutingResult result = router.route(circuit, topology);
```

**Parameters:**
- **lookahead_depth**: Number of future gates to consider when choosing SWAPs
- **decay_factor**: How quickly distant gates' importance decays
- **extended_set_weight**: Balance between immediate and future gates

## Complete Workflow Example

```cpp
#include "parser/Parser.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

using namespace qopt;

int main() {
    // 1. Parse circuit
    Circuit circuit = parseQASM(R"(
        OPENQASM 3.0;
        qubit[5] q;
        h q[0];
        cx q[0], q[4];
        cx q[1], q[3];
        h q[2];
        cx q[2], q[4];
    )");
    
    std::cout << "=== Parsed Circuit ===\n";
    std::cout << "Gates: " << circuit.size() << "\n";
    std::cout << "Depth: " << circuit.depth() << "\n\n";
    
    // 2. Optimize before routing (optional but recommended)
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.addPass(std::make_unique<RotationMergePass>());
    pm.run(circuit);
    
    std::cout << "=== After Optimization ===\n";
    std::cout << "Gates: " << circuit.size() << "\n\n";
    
    // 3. Define target topology (IBM-like grid)
    auto topology = Topology::grid(2, 3);
    // 0—1—2
    // |   |   |
    // 3—4—5
    
    std::cout << "=== Topology ===\n";
    std::cout << "Qubits: " << topology.numQubits() << "\n";
    std::cout << "0-1 connected: " << topology.isConnected(0, 1) << "\n";
    std::cout << "0-4 connected: " << topology.isConnected(0, 4) << "\n\n";
    
    // 4. Route circuit
    SabreRouter router;
    RoutingResult result = router.route(circuit, topology);
    
    std::cout << "=== Routing Result ===\n";
    std::cout << "Original gates: " << circuit.size() << "\n";
    std::cout << "Routed gates: " << result.routed_circuit.size() << "\n";
    std::cout << "SWAPs inserted: " << result.swaps_inserted << "\n";
    std::cout << "Depth overhead: " << result.depth_overhead << "\n\n";
    
    // 5. Show initial mapping
    std::cout << "=== Qubit Mapping ===\n";
    for (size_t i = 0; i < result.initial_mapping.size(); ++i) {
        std::cout << "Logical q[" << i << "] → Physical q[" 
                  << result.initial_mapping[i] << "]\n";
    }
    
    return 0;
}
```

## Optimizing After Routing

SWAP gates might introduce cancellation opportunities:

```cpp
// Route first
SabreRouter router;
RoutingResult result = router.route(circuit, topology);

// Optimize the routed circuit
PassManager pm;
pm.addPass(std::make_unique<CancellationPass>());
pm.run(result.routed_circuit);

std::cout << "After post-routing optimization: " 
          << result.routed_circuit.size() << " gates\n";
```

## Different Topology Patterns

### Linear (1D Chain)

Best for: Small circuits, sequential algorithms

```cpp
// 0—1—2—3—4
auto linear = Topology::linear(5);
```

### Ring

Best for: Periodic boundary conditions

```cpp
// 0—1—2—3—4—0 (wraps around)
auto ring = Topology::ring(5);
```

### Grid (2D Lattice)

Best for: Surface code, 2D algorithms

```cpp
// 0—1—2
// |   |   |
// 3—4—5
// |   |   |
// 6—7—8
auto grid = Topology::grid(3, 3);
```

### Heavy-Hex (IBM)

Best for: IBM quantum processors

```cpp
auto heavyHex = Topology::heavyHex(3);
```

## Measuring Routing Quality

```cpp
RoutingResult result = router.route(circuit, topology);

// Gate overhead
double gateOverhead = double(result.routed_circuit.size()) / circuit.size();
std::cout << "Gate overhead: " << gateOverhead << "x\n";

// SWAP rate
double swapRate = double(result.swaps_inserted) / circuit.twoQubitGateCount();
std::cout << "SWAPs per 2Q gate: " << swapRate << "\n";

// Depth overhead
double depthOverhead = double(result.routed_circuit.depth()) / circuit.depth();
std::cout << "Depth overhead: " << depthOverhead << "x\n";
```

## Trivial Router (Testing)

For testing, use the trivial router that assumes identity mapping:

```cpp
#include "routing/Router.hpp"

TrivialRouter trivial;
RoutingResult result = trivial.route(circuit, topology);

// Only works if circuit is already compatible with topology
// Throws if any 2Q gate is on non-adjacent qubits
```

## Best Practices

1. **Optimize before routing**: Fewer gates = fewer routing constraints
2. **Choose appropriate topology**: Match your target hardware
3. **Tune SABRE parameters**: More lookahead = better results but slower
4. **Optimize after routing**: SWAP insertion may create cancellation opportunities
5. **Measure overhead**: Track gate count and depth increases

## Next Steps

- @ref tutorial-circuits "Tutorial: Working with Circuits"
- @ref tutorial-parsing "Tutorial: Parsing OpenQASM"
- @ref tutorial-optimization "Tutorial: Optimization Passes"
