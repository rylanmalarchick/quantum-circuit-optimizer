# Tutorial: Optimization Passes {#tutorial-optimization}

This tutorial covers using optimization passes to reduce circuit gate count.

## Overview

The optimizer works on the DAG intermediate representation:

```
Circuit → DAG → Optimization Passes → DAG → Circuit
```

The PassManager handles the conversion automatically.

## Using PassManager

```cpp
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"

using namespace qopt;

// Create a circuit with redundant gates
Circuit circuit(2);
circuit.addGate(Gate::h(0));
circuit.addGate(Gate::h(0));     // H·H = I (can be cancelled)
circuit.addGate(Gate::rz(1, M_PI/4));
circuit.addGate(Gate::rz(1, M_PI/4)); // Can be merged

std::cout << "Before: " << circuit.size() << " gates\n";  // 4

// Create and run pass manager
PassManager pm;
pm.addPass(std::make_unique<CancellationPass>());
pm.addPass(std::make_unique<RotationMergePass>());
pm.addPass(std::make_unique<IdentityEliminationPass>());

pm.run(circuit);

std::cout << "After: " << circuit.size() << " gates\n";   // 1 (just Rz(π/2))
```

## Available Passes

### CancellationPass

Removes adjacent pairs of inverse gates:

```cpp
#include "passes/CancellationPass.hpp"

// Before: H H X X CNOT CNOT
// After:  (empty - all gates cancel)

Circuit c(2);
c.addGate(Gate::h(0));
c.addGate(Gate::h(0));     // H·H = I
c.addGate(Gate::x(1));
c.addGate(Gate::x(1));     // X·X = I
c.addGate(Gate::cnot(0, 1));
c.addGate(Gate::cnot(0, 1)); // CNOT·CNOT = I

PassManager pm;
pm.addPass(std::make_unique<CancellationPass>());
pm.run(c);

std::cout << c.size() << " gates\n";  // 0
```

**Cancelled pairs:**
- H·H = I
- X·X = I
- Y·Y = I
- Z·Z = I
- CNOT·CNOT = I
- CZ·CZ = I
- SWAP·SWAP = I

### RotationMergePass

Merges adjacent rotation gates on the same qubit:

```cpp
#include "passes/RotationMergePass.hpp"

// Before: Rz(π/4) Rz(π/4) Rz(π/2)
// After:  Rz(π)

Circuit c(1);
c.addGate(Gate::rz(0, M_PI/4));
c.addGate(Gate::rz(0, M_PI/4));
c.addGate(Gate::rz(0, M_PI/2));

PassManager pm;
pm.addPass(std::make_unique<RotationMergePass>());
pm.run(c);

std::cout << c.size() << " gates\n";  // 1
// The single gate is Rz(π)
```

**Merged rotations:**
- Rz(a)·Rz(b) = Rz(a+b)
- Rx(a)·Rx(b) = Rx(a+b)
- Ry(a)·Ry(b) = Ry(a+b)

### IdentityEliminationPass

Removes rotation gates with zero angle:

```cpp
#include "passes/IdentityEliminationPass.hpp"

// Before: H Rz(0) X Ry(0)
// After:  H X

Circuit c(1);
c.addGate(Gate::h(0));
c.addGate(Gate::rz(0, 0.0));    // Identity
c.addGate(Gate::x(0));
c.addGate(Gate::ry(0, 0.0));    // Identity

PassManager pm;
pm.addPass(std::make_unique<IdentityEliminationPass>());
pm.run(c);

std::cout << c.size() << " gates\n";  // 2
```

### CommutationPass

Reorders commuting gates to enable further optimization:

```cpp
#include "passes/CommutationPass.hpp"

// Before: Rz(π/4) H Rz(π/4)
// After:  H Rz(π/4) Rz(π/4)
// (Now rotation merge can combine the Rz gates)

Circuit c(1);
c.addGate(Gate::rz(0, M_PI/4));
c.addGate(Gate::h(0));
c.addGate(Gate::rz(0, M_PI/4));

PassManager pm;
pm.addPass(std::make_unique<CommutationPass>());
pm.addPass(std::make_unique<RotationMergePass>());
pm.run(c);
```

**Commutation rules used:**
- Rz gates commute with each other
- Diagonal gates commute with Z-basis measurements
- Control qubits of CNOT commute with Z
- Target qubits of CNOT commute with X

## Recommended Pass Order

For best results, run passes in this order:

```cpp
PassManager pm;

// 1. Commutation first to expose optimization opportunities
pm.addPass(std::make_unique<CommutationPass>());

// 2. Cancellation to remove inverse pairs
pm.addPass(std::make_unique<CancellationPass>());

// 3. Merge rotations
pm.addPass(std::make_unique<RotationMergePass>());

// 4. Remove identity rotations created by merging
pm.addPass(std::make_unique<IdentityEliminationPass>());

pm.run(circuit);
```

## Multiple Iterations

Sometimes running passes multiple times finds more optimizations:

```cpp
PassManager pm;
pm.addPass(std::make_unique<CommutationPass>());
pm.addPass(std::make_unique<CancellationPass>());
pm.addPass(std::make_unique<RotationMergePass>());
pm.addPass(std::make_unique<IdentityEliminationPass>());

size_t previousSize;
do {
    previousSize = circuit.size();
    pm.run(circuit);
} while (circuit.size() < previousSize);
```

## Working with DAG Directly

For custom analysis, work with the DAG:

```cpp
#include "ir/DAG.hpp"
#include "passes/CancellationPass.hpp"

// Convert to DAG
DAG dag = DAG::fromCircuit(circuit);

// Run pass on DAG
CancellationPass pass;
pass.run(dag);

// Analyze the DAG
for (DAGNode* node : dag.topologicalOrder()) {
    std::cout << node->gate().name() << " on qubits: ";
    for (size_t q : node->gate().qubits()) {
        std::cout << q << " ";
    }
    std::cout << "\n";
}

// Convert back to circuit
Circuit optimized = dag.toCircuit();
```

## Custom Passes

Create your own optimization pass:

```cpp
#include "passes/Pass.hpp"

class MyCustomPass : public Pass {
public:
    std::string name() const override {
        return "MyCustomPass";
    }
    
    void run(DAG& dag) override {
        // Get nodes in topological order
        auto nodes = dag.topologicalOrder();
        
        for (DAGNode* node : nodes) {
            // Your optimization logic here
            if (shouldRemove(node)) {
                dag.removeNode(node);
            }
        }
    }
    
private:
    bool shouldRemove(DAGNode* node) {
        // Custom logic
        return false;
    }
};
```

## Measuring Optimization

```cpp
Circuit original = /* ... */;
Circuit optimized = original;  // Copy

PassManager pm;
pm.addPass(std::make_unique<CancellationPass>());
pm.addPass(std::make_unique<RotationMergePass>());
pm.run(optimized);

std::cout << "Original gates: " << original.size() << "\n";
std::cout << "Optimized gates: " << optimized.size() << "\n";
std::cout << "Reduction: " 
          << (1.0 - double(optimized.size()) / original.size()) * 100 
          << "%\n";

std::cout << "Original depth: " << original.depth() << "\n";
std::cout << "Optimized depth: " << optimized.depth() << "\n";
```

## Real-World Example

Optimizing a parsed QASM circuit:

```cpp
#include "parser/Parser.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/CommutationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"

std::string source = R"(
    OPENQASM 3.0;
    qubit[3] q;
    
    // Some redundant operations
    h q[0];
    h q[0];           // Cancels with previous H
    
    rz(pi/4) q[1];
    rz(pi/4) q[1];    // Merges to Rz(π/2)
    
    cx q[0], q[1];
    cx q[0], q[1];    // Cancels with previous CX
    
    rz(0) q[2];       // Identity, will be removed
)";

Circuit circuit = parseQASM(source);
std::cout << "Before optimization: " << circuit.size() << " gates\n";

PassManager pm;
pm.addPass(std::make_unique<CommutationPass>());
pm.addPass(std::make_unique<CancellationPass>());
pm.addPass(std::make_unique<RotationMergePass>());
pm.addPass(std::make_unique<IdentityEliminationPass>());
pm.run(circuit);

std::cout << "After optimization: " << circuit.size() << " gates\n";
// Expected: 1 gate (just Rz(π/2))
```

## Next Steps

- @ref tutorial-circuits "Tutorial: Working with Circuits" - Create circuits programmatically
- @ref tutorial-parsing "Tutorial: Parsing OpenQASM" - Load circuits from files
- @ref tutorial-routing "Tutorial: Qubit Routing" - Map to hardware topology
