// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file optimization_demo.cpp
 * @brief Demonstrates the optimization passes in detail
 *
 * Shows how each optimization pass works and its effect on circuits.
 */

#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"
#include "ir/DAG.hpp"
#include "passes/PassManager.hpp"
#include "passes/Pass.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/CommutationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"

#include <cmath>
#include <iostream>
#include <memory>

using namespace qopt;

void printCircuit(const ir::Circuit& circuit, const std::string& label) {
    std::cout << label << " (" << circuit.numGates() << " gates, depth " 
              << circuit.depth() << "):\n";
    for (const auto& gate : circuit) {
        std::cout << "  " << gate.toString() << "\n";
    }
    std::cout << "\n";
}

void runSinglePass(ir::Circuit& circuit, std::unique_ptr<passes::Pass> pass) {
    std::string name = pass->name();
    std::size_t before = circuit.numGates();
    
    passes::PassManager pm;
    pm.addPass(std::move(pass));
    pm.run(circuit);
    
    std::size_t after = circuit.numGates();
    std::cout << "  " << name << ": " << before << " -> " << after 
              << " gates (removed " << (before - after) << ")\n";
}

int main() {
    std::cout << "=== Optimization Pass Demo ===\n\n";

    // =========================================================================
    // 1. CancellationPass Demo
    // =========================================================================
    std::cout << "1. CancellationPass - Removes adjacent inverse pairs\n";
    std::cout << std::string(50, '-') << "\n";
    
    {
        ir::Circuit circuit(2);
        circuit.addGate(ir::Gate::h(0));
        circuit.addGate(ir::Gate::h(0));      // H*H = I
        circuit.addGate(ir::Gate::x(1));
        circuit.addGate(ir::Gate::cnot(0, 1));
        circuit.addGate(ir::Gate::cnot(0, 1)); // CNOT*CNOT = I
        circuit.addGate(ir::Gate::x(1));       // X*X = I (with the earlier X)
        
        printCircuit(circuit, "Before");
        
        passes::PassManager pm;
        pm.addPass(std::make_unique<passes::CancellationPass>());
        pm.run(circuit);
        
        printCircuit(circuit, "After CancellationPass");
    }

    // =========================================================================
    // 2. RotationMergePass Demo
    // =========================================================================
    std::cout << "2. RotationMergePass - Merges adjacent rotations\n";
    std::cout << std::string(50, '-') << "\n";
    
    {
        ir::Circuit circuit(1);
        circuit.addGate(ir::Gate::rz(0, M_PI / 4));   // pi/4
        circuit.addGate(ir::Gate::rz(0, M_PI / 4));   // + pi/4 = pi/2
        circuit.addGate(ir::Gate::rz(0, M_PI / 2));   // + pi/2 = pi
        circuit.addGate(ir::Gate::h(0));
        circuit.addGate(ir::Gate::rx(0, M_PI / 8));
        circuit.addGate(ir::Gate::rx(0, M_PI / 8));   // -> Rx(pi/4)
        
        printCircuit(circuit, "Before");
        
        passes::PassManager pm;
        pm.addPass(std::make_unique<passes::RotationMergePass>());
        pm.run(circuit);
        
        printCircuit(circuit, "After RotationMergePass");
    }

    // =========================================================================
    // 3. IdentityEliminationPass Demo
    // =========================================================================
    std::cout << "3. IdentityEliminationPass - Removes identity rotations\n";
    std::cout << std::string(50, '-') << "\n";
    
    {
        ir::Circuit circuit(2);
        circuit.addGate(ir::Gate::h(0));
        circuit.addGate(ir::Gate::rz(0, 0.0));        // Identity: Rz(0)
        circuit.addGate(ir::Gate::cnot(0, 1));
        circuit.addGate(ir::Gate::rx(1, 0.0));        // Identity: Rx(0)
        circuit.addGate(ir::Gate::ry(0, 2 * M_PI));   // Identity: Ry(2pi)
        circuit.addGate(ir::Gate::z(1));
        
        printCircuit(circuit, "Before");
        
        passes::PassManager pm;
        pm.addPass(std::make_unique<passes::IdentityEliminationPass>());
        pm.run(circuit);
        
        printCircuit(circuit, "After IdentityEliminationPass");
    }

    // =========================================================================
    // 4. CommutationPass Demo
    // =========================================================================
    std::cout << "4. CommutationPass - Reorders commuting gates\n";
    std::cout << std::string(50, '-') << "\n";
    
    {
        ir::Circuit circuit(1);
        // Rz gates commute with each other
        circuit.addGate(ir::Gate::rz(0, M_PI / 4));
        circuit.addGate(ir::Gate::h(0));              // Blocks direct merge
        circuit.addGate(ir::Gate::rz(0, M_PI / 4));
        
        printCircuit(circuit, "Before");
        
        // First apply commutation to expose optimization opportunities
        passes::PassManager pm1;
        pm1.addPass(std::make_unique<passes::CommutationPass>());
        pm1.run(circuit);
        
        printCircuit(circuit, "After CommutationPass");
        
        // Now merge the rotations
        passes::PassManager pm2;
        pm2.addPass(std::make_unique<passes::RotationMergePass>());
        pm2.run(circuit);
        
        printCircuit(circuit, "After RotationMergePass");
    }

    // =========================================================================
    // 5. Full Pipeline Demo
    // =========================================================================
    std::cout << "5. Full Optimization Pipeline\n";
    std::cout << std::string(50, '-') << "\n";
    
    {
        ir::Circuit circuit(3);
        
        // Create a circuit with various optimization opportunities
        circuit.addGate(ir::Gate::h(0));
        circuit.addGate(ir::Gate::h(0));           // Cancels
        circuit.addGate(ir::Gate::h(0));           // Remains
        
        circuit.addGate(ir::Gate::rz(1, M_PI/4));
        circuit.addGate(ir::Gate::rz(1, M_PI/4));  // Merge -> pi/2
        circuit.addGate(ir::Gate::rz(1, -M_PI/2)); // Merge -> 0 -> eliminate
        
        circuit.addGate(ir::Gate::cnot(0, 1));
        circuit.addGate(ir::Gate::cnot(0, 1));     // Cancels
        
        circuit.addGate(ir::Gate::x(2));
        circuit.addGate(ir::Gate::x(2));           // Cancels
        circuit.addGate(ir::Gate::h(2));
        
        printCircuit(circuit, "Original Circuit");
        
        std::cout << "Running passes:\n";
        
        ir::Circuit c1 = circuit.clone();
        runSinglePass(c1, std::make_unique<passes::CommutationPass>());
        
        ir::Circuit c2 = circuit.clone();
        runSinglePass(c2, std::make_unique<passes::CancellationPass>());
        
        ir::Circuit c3 = circuit.clone();
        runSinglePass(c3, std::make_unique<passes::RotationMergePass>());
        
        ir::Circuit c4 = circuit.clone();
        runSinglePass(c4, std::make_unique<passes::IdentityEliminationPass>());
        
        std::cout << "\nFull pipeline:\n";
        
        passes::PassManager pm;
        pm.addPass(std::make_unique<passes::CommutationPass>());
        pm.addPass(std::make_unique<passes::CancellationPass>());
        pm.addPass(std::make_unique<passes::RotationMergePass>());
        pm.addPass(std::make_unique<passes::IdentityEliminationPass>());
        pm.run(circuit);
        
        std::cout << pm.statistics().toString() << "\n";
        
        printCircuit(circuit, "Final Optimized Circuit");
    }

    std::cout << "=== Done! ===\n";
    
    return 0;
}
