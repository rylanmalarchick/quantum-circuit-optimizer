# Tutorial: Working with Circuits {#tutorial-circuits}

This tutorial covers the basics of creating and manipulating quantum circuits.

## Creating Gates

Gates are created using factory methods on the `Gate` class:

```cpp
#include "ir/Gate.hpp"

using namespace qopt;

// Single-qubit gates
auto h = Gate::h(0);           // Hadamard on qubit 0
auto x = Gate::x(1);           // Pauli-X on qubit 1
auto y = Gate::y(2);           // Pauli-Y on qubit 2
auto z = Gate::z(0);           // Pauli-Z on qubit 0

// Rotation gates (parameterized)
auto rx = Gate::rx(0, M_PI/2); // Rx(π/2) on qubit 0
auto ry = Gate::ry(1, M_PI/4); // Ry(π/4) on qubit 1
auto rz = Gate::rz(2, M_PI);   // Rz(π) on qubit 2

// Two-qubit gates
auto cx = Gate::cnot(0, 1);    // CNOT: control=0, target=1
auto cz = Gate::cz(1, 2);      // CZ on qubits 1 and 2
auto swap = Gate::swap(0, 2);  // SWAP qubits 0 and 2
```

## Inspecting Gates

Gates provide methods to query their properties:

```cpp
Gate h = Gate::h(0);

// Type information
h.type();                      // GateType::H
h.name();                      // "H"
h.numQubits();                 // 1

// Qubit access
h.qubits();                    // {0}
h.qubit();                     // 0 (first/only qubit)

// Parameterized gates
Gate rz = Gate::rz(0, M_PI/4);
rz.isParameterized();          // true
rz.angle();                    // 0.785398... (π/4)
rz.hasAngle();                 // true

// Two-qubit gates
Gate cx = Gate::cnot(0, 1);
cx.control();                  // 0
cx.target();                   // 1
```

## Gate Inverses

Get the inverse (adjoint) of a gate:

```cpp
auto h = Gate::h(0);
auto h_inv = h.inverse();      // H† = H (self-inverse)

auto rz = Gate::rz(0, M_PI/4);
auto rz_inv = rz.inverse();    // Rz(-π/4)

auto cx = Gate::cnot(0, 1);
auto cx_inv = cx.inverse();    // CNOT† = CNOT (self-inverse)
```

## Creating Circuits

A Circuit is a container of gates with a fixed number of qubits:

```cpp
#include "ir/Circuit.hpp"

using namespace qopt;

// Create a 3-qubit circuit
Circuit circuit(3);

// Add gates
circuit.addGate(Gate::h(0));
circuit.addGate(Gate::cnot(0, 1));
circuit.addGate(Gate::cnot(1, 2));

// Query circuit properties
circuit.numQubits();           // 3
circuit.size();                // 3 (number of gates)
circuit.empty();               // false
circuit.depth();               // 3 (circuit depth)
```

## Circuit Metrics

```cpp
Circuit circuit(4);
circuit.addGate(Gate::h(0));
circuit.addGate(Gate::h(1));
circuit.addGate(Gate::cnot(0, 1));
circuit.addGate(Gate::cnot(2, 3));
circuit.addGate(Gate::rz(0, M_PI/4));

// Count gates by type
circuit.countGateType(GateType::H);      // 2
circuit.countGateType(GateType::CNOT);   // 2
circuit.countGateType(GateType::RZ);     // 1

// Count categories
circuit.singleQubitGateCount();          // 3 (2 H + 1 RZ)
circuit.twoQubitGateCount();             // 2 (2 CNOT)

// Depth (longest path through circuit)
circuit.depth();                          // 3
```

## Iterating Over Gates

```cpp
Circuit circuit(2);
circuit.addGate(Gate::h(0));
circuit.addGate(Gate::cnot(0, 1));

// Range-based for loop
for (const Gate& gate : circuit) {
    std::cout << gate.name() << " on qubits ";
    for (size_t q : gate.qubits()) {
        std::cout << q << " ";
    }
    std::cout << "\n";
}

// Indexed access
for (size_t i = 0; i < circuit.size(); ++i) {
    const Gate& gate = circuit[i];
    // ...
}

// Iterator access
for (auto it = circuit.begin(); it != circuit.end(); ++it) {
    const Gate& gate = *it;
    // ...
}
```

## Building Common Circuits

### Bell State

```cpp
Circuit bellState() {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    return c;
}
```

### GHZ State

```cpp
Circuit ghzState(size_t n) {
    Circuit c(n);
    c.addGate(Gate::h(0));
    for (size_t i = 1; i < n; ++i) {
        c.addGate(Gate::cnot(0, i));
    }
    return c;
}
```

### Quantum Fourier Transform

```cpp
Circuit qft(size_t n) {
    Circuit c(n);
    for (size_t i = 0; i < n; ++i) {
        c.addGate(Gate::h(i));
        for (size_t j = i + 1; j < n; ++j) {
            double angle = M_PI / std::pow(2, j - i);
            // Controlled rotation (using decomposition)
            c.addGate(Gate::cnot(j, i));
            c.addGate(Gate::rz(i, -angle / 2));
            c.addGate(Gate::cnot(j, i));
            c.addGate(Gate::rz(i, angle / 2));
        }
    }
    return c;
}
```

## Clearing and Modifying Circuits

```cpp
Circuit circuit(3);
circuit.addGate(Gate::h(0));
circuit.addGate(Gate::h(1));
circuit.addGate(Gate::h(2));

// Clear all gates
circuit.clear();
circuit.size();   // 0
circuit.empty();  // true

// Number of qubits is preserved
circuit.numQubits();  // 3
```

## Error Handling

Invalid operations throw descriptive exceptions:

```cpp
try {
    Circuit circuit(3);
    circuit.addGate(Gate::h(5));  // Qubit 5 doesn't exist!
} catch (const std::out_of_range& e) {
    std::cerr << "Error: " << e.what() << "\n";
    // "Qubit index 5 exceeds circuit size 3"
}

try {
    Gate::cnot(0, 0);  // Control and target are same!
} catch (const std::invalid_argument& e) {
    std::cerr << "Error: " << e.what() << "\n";
    // "Control and target qubits must differ"
}
```

## Next Steps

- @ref tutorial-parsing "Tutorial: Parsing OpenQASM" - Load circuits from files
- @ref tutorial-optimization "Tutorial: Optimization Passes" - Reduce gate count
- @ref tutorial-routing "Tutorial: Qubit Routing" - Map to hardware
