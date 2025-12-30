// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file basic_usage.cpp
 * @brief Basic usage example for the Quantum Circuit Optimizer
 *
 * Demonstrates:
 * - Creating circuits programmatically
 * - Parsing OpenQASM
 * - Optimizing circuits
 * - Routing to hardware topology
 */

#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"
#include "parser/Parser.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

#include <iostream>

using namespace qopt;

int main() {
    std::cout << "=== Quantum Circuit Optimizer - Basic Usage ===\n\n";

    // =========================================================================
    // 1. Creating a circuit programmatically
    // =========================================================================
    std::cout << "1. Creating a Bell state circuit programmatically:\n";
    
    ir::Circuit bell(2);
    bell.addGate(ir::Gate::h(0));
    bell.addGate(ir::Gate::cnot(0, 1));
    
    std::cout << "   Gates: " << bell.numGates() << "\n";
    std::cout << "   Depth: " << bell.depth() << "\n\n";

    // =========================================================================
    // 2. Parsing OpenQASM
    // =========================================================================
    std::cout << "2. Parsing an OpenQASM circuit:\n";
    
    const char* qasm_source = R"(
        OPENQASM 3.0;
        qubit[3] q;
        
        // Create GHZ state with some redundant gates
        h q[0];
        h q[0];     // This H cancels with the previous one
        h q[0];     // This H is the effective one
        cx q[0], q[1];
        cx q[1], q[2];
        
        // Some rotations that can be merged
        rz(pi/4) q[0];
        rz(pi/4) q[0];
        rz(pi/2) q[0];
    )";
    
    auto circuit = parser::parseQASM(qasm_source);
    std::cout << "   Parsed gates: " << circuit->numGates() << "\n";
    std::cout << "   Qubits: " << circuit->numQubits() << "\n\n";

    // =========================================================================
    // 3. Optimizing the circuit
    // =========================================================================
    std::cout << "3. Optimizing the circuit:\n";
    std::cout << "   Before optimization: " << circuit->numGates() << " gates\n";
    
    passes::PassManager pm;
    pm.addPass(std::make_unique<passes::CancellationPass>());
    pm.addPass(std::make_unique<passes::RotationMergePass>());
    pm.addPass(std::make_unique<passes::IdentityEliminationPass>());
    pm.run(*circuit);
    
    std::cout << "   After optimization: " << circuit->numGates() << " gates\n";
    
    const auto& stats = pm.statistics();
    std::cout << "   Reduction: " << stats.reductionPercent() << "%\n\n";

    // =========================================================================
    // 4. Routing to hardware topology
    // =========================================================================
    std::cout << "4. Routing to a linear topology:\n";
    
    // Create a linear topology: 0 -- 1 -- 2
    auto topology = routing::Topology::linear(3);
    std::cout << "   Topology: Linear with " << topology.numQubits() << " qubits\n";
    std::cout << "   0-1 connected: " << (topology.connected(0, 1) ? "yes" : "no") << "\n";
    std::cout << "   0-2 connected: " << (topology.connected(0, 2) ? "yes" : "no") << "\n";
    
    routing::SabreRouter router;
    auto result = router.route(*circuit, topology);
    
    std::cout << "   Routed gates: " << result.routed_circuit.numGates() << "\n";
    std::cout << "   SWAPs inserted: " << result.swaps_inserted << "\n";
    
    // Show final mapping
    std::cout << "   Final mapping:\n";
    for (std::size_t i = 0; i < result.final_mapping.size(); ++i) {
        std::cout << "     Logical q[" << i << "] -> Physical q[" 
                  << result.final_mapping[i] << "]\n";
    }
    
    std::cout << "\n=== Done! ===\n";
    
    return 0;
}
