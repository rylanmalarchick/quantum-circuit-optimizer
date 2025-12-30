// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file routing_demo.cpp
 * @brief Demonstrates qubit routing to various hardware topologies
 *
 * Shows how to route circuits to different hardware connectivity patterns.
 */

#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

#include <iostream>
#include <string>

using namespace qopt;

void printTopology(const routing::Topology& t, const std::string& name) {
    std::cout << name << " (" << t.numQubits() << " qubits):\n";
    std::cout << "  Connections:\n";
    
    for (std::size_t i = 0; i < t.numQubits(); ++i) {
        auto neighbors = t.neighbors(i);
        if (!neighbors.empty()) {
            std::cout << "    " << i << " -- ";
            for (std::size_t j = 0; j < neighbors.size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << neighbors[j];
            }
            std::cout << "\n";
        }
    }
    std::cout << "\n";
}

void runRouting(const ir::Circuit& circuit, const routing::Topology& topology,
                const std::string& topology_name) {
    std::cout << "Routing to " << topology_name << ":\n";
    std::cout << "  Original: " << circuit.numGates() << " gates, depth " 
              << circuit.depth() << "\n";
    
    routing::SabreRouter router;
    auto result = router.route(circuit, topology);
    
    std::cout << "  Routed:   " << result.routed_circuit.numGates() << " gates, depth "
              << result.routed_circuit.depth() << "\n";
    std::cout << "  SWAPs inserted: " << result.swaps_inserted << "\n";
    
    // Calculate overhead
    double overhead = 0.0;
    if (circuit.numGates() > 0) {
        overhead = 100.0 * (static_cast<double>(result.routed_circuit.numGates()) / 
                           static_cast<double>(circuit.numGates()) - 1.0);
    }
    std::cout << "  Gate overhead: " << overhead << "%\n\n";
}

int main() {
    std::cout << "=== Qubit Routing Demo ===\n\n";

    // =========================================================================
    // 1. Create Test Circuits
    // =========================================================================
    std::cout << "1. Test Circuits\n";
    std::cout << std::string(50, '-') << "\n";
    
    // Simple circuit with non-adjacent 2Q gates
    ir::Circuit simple(4);
    simple.addGate(ir::Gate::h(0));
    simple.addGate(ir::Gate::h(3));
    simple.addGate(ir::Gate::cnot(0, 3));  // Far apart
    simple.addGate(ir::Gate::cnot(1, 2));  // Adjacent in most topologies
    
    std::cout << "Simple circuit: " << simple.numGates() << " gates\n";
    std::cout << "  Contains CNOT(0,3) which spans non-adjacent qubits\n\n";
    
    // GHZ-like circuit
    ir::Circuit ghz(5);
    ghz.addGate(ir::Gate::h(0));
    for (std::size_t i = 0; i < 4; ++i) {
        ghz.addGate(ir::Gate::cnot(0, i + 1));  // Star pattern
    }
    
    std::cout << "GHZ-like circuit: " << ghz.numGates() << " gates\n";
    std::cout << "  CNOT star pattern from q[0] to all others\n\n";
    
    // =========================================================================
    // 2. Different Topologies
    // =========================================================================
    std::cout << "2. Available Topologies\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto linear = routing::Topology::linear(4);
    printTopology(linear, "Linear(4)");
    
    auto ring = routing::Topology::ring(5);
    printTopology(ring, "Ring(5)");
    
    auto grid = routing::Topology::grid(2, 3);
    printTopology(grid, "Grid(2x3)");
    
    // =========================================================================
    // 3. Routing Simple Circuit
    // =========================================================================
    std::cout << "3. Routing Simple Circuit to Different Topologies\n";
    std::cout << std::string(50, '-') << "\n";
    
    runRouting(simple, routing::Topology::linear(4), "Linear(4)");
    runRouting(simple, routing::Topology::ring(4), "Ring(4)");
    runRouting(simple, routing::Topology::grid(2, 2), "Grid(2x2)");
    
    // =========================================================================
    // 4. Routing GHZ Circuit
    // =========================================================================
    std::cout << "4. Routing GHZ Circuit to Different Topologies\n";
    std::cout << std::string(50, '-') << "\n";
    
    runRouting(ghz, routing::Topology::linear(5), "Linear(5)");
    runRouting(ghz, routing::Topology::ring(5), "Ring(5)");
    runRouting(ghz, routing::Topology::grid(2, 3), "Grid(2x3)");
    
    // =========================================================================
    // 5. Custom Topology
    // =========================================================================
    std::cout << "5. Custom Topology Example\n";
    std::cout << std::string(50, '-') << "\n";
    
    // Create a star topology (hub and spoke)
    routing::Topology star(5);
    for (std::size_t i = 1; i < 5; ++i) {
        star.addEdge(0, i);  // Connect all to center
    }
    
    printTopology(star, "Star(5)");
    
    // The GHZ circuit should route perfectly to star topology
    std::cout << "GHZ circuit on star topology (optimal for this pattern):\n";
    runRouting(ghz, star, "Star(5)");
    
    // =========================================================================
    // 6. Distance and Path Queries
    // =========================================================================
    std::cout << "6. Topology Queries\n";
    std::cout << std::string(50, '-') << "\n";
    
    auto topo = routing::Topology::grid(3, 3);
    // Grid layout:
    // 0 - 1 - 2
    // |   |   |
    // 3 - 4 - 5
    // |   |   |
    // 6 - 7 - 8
    
    std::cout << "Grid(3x3) distance queries:\n";
    std::cout << "  Distance(0, 4) = " << topo.distance(0, 4) << "\n";
    std::cout << "  Distance(0, 8) = " << topo.distance(0, 8) << "\n";
    std::cout << "  Distance(2, 6) = " << topo.distance(2, 6) << "\n";
    
    std::cout << "\nShortest path from 0 to 8:\n  ";
    auto path = topo.shortestPath(0, 8);
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i > 0) std::cout << " -> ";
        std::cout << path[i];
    }
    std::cout << "\n\n";

    std::cout << "=== Done! ===\n";
    
    return 0;
}
