// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file main.cpp
 * @brief Quantum Circuit Optimizer demonstration
 *
 * Demonstrates basic circuit construction and manipulation using the
 * qopt::ir library.
 */

#include "ir/Circuit.hpp"

#include <iostream>

int main() {
    using namespace qopt::ir;

    std::cout << "=== Quantum Circuit Optimizer ===\n\n";

    // Create a 2-qubit Bell state circuit
    std::cout << "Building Bell state circuit...\n";
    Circuit bell(2);
    bell.addGate(Gate::h(0));
    bell.addGate(Gate::cnot(0, 1));

    std::cout << bell << "\n";

    // Create a 3-qubit GHZ state circuit
    std::cout << "Building GHZ state circuit...\n";
    Circuit ghz(3);
    ghz.addGate(Gate::h(0));
    ghz.addGate(Gate::cnot(0, 1));
    ghz.addGate(Gate::cnot(1, 2));

    std::cout << ghz << "\n";

    // Demonstrate rotation gates
    std::cout << "Building rotation circuit...\n";
    Circuit rotations(2);
    rotations.addGate(Gate::h(0));
    rotations.addGate(Gate::rz(0, qopt::constants::PI / 4));
    rotations.addGate(Gate::rx(1, qopt::constants::PI / 2));
    rotations.addGate(Gate::cnot(0, 1));
    rotations.addGate(Gate::ry(1, qopt::constants::PI));

    std::cout << rotations << "\n";

    // Show circuit statistics
    std::cout << "=== Circuit Statistics ===\n";
    std::cout << "Bell circuit:\n";
    std::cout << "  Qubits: " << bell.numQubits() << "\n";
    std::cout << "  Gates: " << bell.numGates() << "\n";
    std::cout << "  Depth: " << bell.depth() << "\n";
    std::cout << "  2-qubit gates: " << bell.countTwoQubitGates() << "\n";

    std::cout << "\nGHZ circuit:\n";
    std::cout << "  Qubits: " << ghz.numQubits() << "\n";
    std::cout << "  Gates: " << ghz.numGates() << "\n";
    std::cout << "  Depth: " << ghz.depth() << "\n";
    std::cout << "  2-qubit gates: " << ghz.countTwoQubitGates() << "\n";

    std::cout << "\nRotation circuit:\n";
    std::cout << "  Qubits: " << rotations.numQubits() << "\n";
    std::cout << "  Gates: " << rotations.numGates() << "\n";
    std::cout << "  Depth: " << rotations.depth() << "\n";
    std::cout << "  2-qubit gates: " << rotations.countTwoQubitGates() << "\n";

    // Demonstrate iteration
    std::cout << "\n=== Iterating over GHZ gates ===\n";
    for (const auto& gate : ghz) {
        std::cout << "  " << gate.toString() << "\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
