#ifndef GATE_HPP
#define GATE_HPP

#include <vector>
#include <cstddef>

enum class GateType {
    H,      // Hadamard gate
    X,      // Pauli-X
    Y,      // Pauli-Y
    Z,      // Pauli-Z
    CNOT,   // Controlled-NOT (2 qubit)
    Rz      // Rotation around Z (parametrized)
};

class Gate {
    public:
        GateType type;
        std::vector<size_t> qubits; // which qubit(s) this gate acts on
        std::vector<double> params; // parameters like rotation angle(s)
        size_t id;                  // unique identifier for the gate
};

#endif
