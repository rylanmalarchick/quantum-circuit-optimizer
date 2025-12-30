// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file benchmark_circuits.cpp
 * @brief Benchmark suite for quantum circuit optimization and routing
 *
 * Benchmarks the optimizer with standard circuit patterns:
 * - QFT (Quantum Fourier Transform)
 * - Random circuits
 * - Ripple-carry adder
 * - QAOA-style circuits
 */

#include "ir/Circuit.hpp"
#include "ir/DAG.hpp"
#include "ir/Gate.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/CommutationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"
#include "routing/SabreRouter.hpp"
#include "routing/Topology.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace qopt;

// ============================================================================
// Circuit Generators
// ============================================================================

/**
 * @brief Generates a Quantum Fourier Transform circuit.
 *
 * QFT on n qubits requires O(n^2) gates:
 * - n Hadamard gates
 * - n(n-1)/2 controlled rotation gates
 */
ir::Circuit generateQFT(std::size_t n) {
    ir::Circuit circuit(n);

    for (std::size_t i = 0; i < n; ++i) {
        circuit.addGate(ir::Gate::h(i));

        for (std::size_t j = i + 1; j < n; ++j) {
            double angle = M_PI / std::pow(2.0, static_cast<double>(j - i));
            // Controlled rotation decomposed as: CNOT + Rz + CNOT + Rz
            circuit.addGate(ir::Gate::cnot(j, i));
            circuit.addGate(ir::Gate::rz(i, -angle / 2));
            circuit.addGate(ir::Gate::cnot(j, i));
            circuit.addGate(ir::Gate::rz(i, angle / 2));
        }
    }

    return circuit;
}

/**
 * @brief Generates a random circuit with mixed gate types.
 */
ir::Circuit generateRandom(std::size_t n_qubits, std::size_t n_gates, unsigned seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<std::size_t> qubit_dist(0, n_qubits - 1);
    std::uniform_int_distribution<int> gate_dist(0, 5);
    std::uniform_real_distribution<double> angle_dist(0.0, 2 * M_PI);

    ir::Circuit circuit(n_qubits);

    for (std::size_t i = 0; i < n_gates; ++i) {
        int gate_type = gate_dist(rng);
        std::size_t q0 = qubit_dist(rng);

        switch (gate_type) {
            case 0:
                circuit.addGate(ir::Gate::h(q0));
                break;
            case 1:
                circuit.addGate(ir::Gate::x(q0));
                break;
            case 2:
                circuit.addGate(ir::Gate::rz(q0, angle_dist(rng)));
                break;
            case 3:
            case 4:
            case 5: {
                // Two-qubit gate
                std::size_t q1 = qubit_dist(rng);
                if (q1 == q0) {
                    q1 = (q0 + 1) % n_qubits;
                }
                if (gate_type == 3) {
                    circuit.addGate(ir::Gate::cnot(q0, q1));
                } else if (gate_type == 4) {
                    circuit.addGate(ir::Gate::cz(q0, q1));
                } else {
                    circuit.addGate(ir::Gate::swap(q0, q1));
                }
                break;
            }
        }
    }

    return circuit;
}

/**
 * @brief Generates a ripple-carry adder circuit.
 *
 * Adds two n-bit numbers stored in quantum registers.
 * Uses 2n+1 qubits and O(n) gates.
 */
ir::Circuit generateAdder(std::size_t n_bits) {
    // 2n+1 qubits: n for A, n for B (result), 1 carry
    std::size_t n_qubits = 2 * n_bits + 1;
    ir::Circuit circuit(n_qubits);

    // Simplified adder structure using Toffoli-like decomposition
    for (std::size_t i = 0; i < n_bits; ++i) {
        std::size_t a = i;
        std::size_t b = n_bits + i;
        std::size_t carry = 2 * n_bits;

        // Carry propagation (simplified)
        circuit.addGate(ir::Gate::cnot(a, b));
        circuit.addGate(ir::Gate::cnot(carry, b));

        if (i < n_bits - 1) {
            // Generate carry
            circuit.addGate(ir::Gate::cnot(a, carry));
            circuit.addGate(ir::Gate::h(carry));
            circuit.addGate(ir::Gate::cnot(b, carry));
            circuit.addGate(ir::Gate::h(carry));
        }
    }

    return circuit;
}

/**
 * @brief Generates a QAOA-style circuit.
 *
 * Alternating layers of:
 * - Problem Hamiltonian (ZZ interactions)
 * - Mixer Hamiltonian (X rotations)
 */
ir::Circuit generateQAOA(std::size_t n_qubits, std::size_t p_layers) {
    ir::Circuit circuit(n_qubits);

    // Initial state: |+>^n
    for (std::size_t i = 0; i < n_qubits; ++i) {
        circuit.addGate(ir::Gate::h(i));
    }

    for (std::size_t layer = 0; layer < p_layers; ++layer) {
        double gamma = M_PI / (4.0 * static_cast<double>(layer + 1));
        double beta = M_PI / (2.0 * static_cast<double>(layer + 1));

        // Problem Hamiltonian: ZZ on all edges (ring graph)
        for (std::size_t i = 0; i < n_qubits; ++i) {
            std::size_t j = (i + 1) % n_qubits;
            // ZZ(gamma) = CNOT Rz CNOT
            circuit.addGate(ir::Gate::cnot(i, j));
            circuit.addGate(ir::Gate::rz(j, gamma));
            circuit.addGate(ir::Gate::cnot(i, j));
        }

        // Mixer: X rotations
        for (std::size_t i = 0; i < n_qubits; ++i) {
            circuit.addGate(ir::Gate::rx(i, beta));
        }
    }

    return circuit;
}

// ============================================================================
// Benchmarking Infrastructure
// ============================================================================

struct BenchmarkResult {
    std::string name;
    std::size_t n_qubits;
    std::size_t original_gates;
    std::size_t optimized_gates;
    std::size_t routed_gates;
    std::size_t swaps_inserted;
    double optimization_time_ms;
    double routing_time_ms;
    double optimization_reduction_pct;
    double routing_overhead_pct;
};

BenchmarkResult runBenchmark(
    const std::string& name,
    ir::Circuit circuit,
    const routing::Topology& topology) {

    BenchmarkResult result;
    result.name = name;
    result.n_qubits = circuit.numQubits();
    result.original_gates = circuit.numGates();

    // Optimization
    auto opt_start = std::chrono::high_resolution_clock::now();

    passes::PassManager pm;
    pm.addPass(std::make_unique<passes::CommutationPass>());
    pm.addPass(std::make_unique<passes::CancellationPass>());
    pm.addPass(std::make_unique<passes::RotationMergePass>());
    pm.addPass(std::make_unique<passes::IdentityEliminationPass>());
    pm.run(circuit);

    auto opt_end = std::chrono::high_resolution_clock::now();
    result.optimization_time_ms = std::chrono::duration<double, std::milli>(opt_end - opt_start).count();
    result.optimized_gates = circuit.numGates();

    // Routing
    auto route_start = std::chrono::high_resolution_clock::now();

    routing::SabreRouter router;
    auto routing_result = router.route(circuit, topology);

    auto route_end = std::chrono::high_resolution_clock::now();
    result.routing_time_ms = std::chrono::duration<double, std::milli>(route_end - route_start).count();

    result.routed_gates = routing_result.routed_circuit.numGates();
    result.swaps_inserted = routing_result.swaps_inserted;

    // Calculate percentages
    if (result.original_gates > 0) {
        result.optimization_reduction_pct = 100.0 *
            (1.0 - static_cast<double>(result.optimized_gates) / static_cast<double>(result.original_gates));
    } else {
        result.optimization_reduction_pct = 0.0;
    }

    if (result.optimized_gates > 0) {
        result.routing_overhead_pct = 100.0 *
            (static_cast<double>(result.routed_gates) / static_cast<double>(result.optimized_gates) - 1.0);
    } else {
        result.routing_overhead_pct = 0.0;
    }

    return result;
}

void printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << "================================================================================\n";
    std::cout << "                      QUANTUM CIRCUIT OPTIMIZER BENCHMARKS                      \n";
    std::cout << "================================================================================\n\n";

    std::cout << std::left << std::setw(20) << "Circuit"
              << std::right << std::setw(8) << "Qubits"
              << std::setw(10) << "Original"
              << std::setw(10) << "Optimized"
              << std::setw(10) << "Routed"
              << std::setw(8) << "SWAPs"
              << std::setw(10) << "Opt %"
              << std::setw(12) << "Route OH%"
              << "\n";

    std::cout << std::string(88, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(20) << r.name
                  << std::right << std::setw(8) << r.n_qubits
                  << std::setw(10) << r.original_gates
                  << std::setw(10) << r.optimized_gates
                  << std::setw(10) << r.routed_gates
                  << std::setw(8) << r.swaps_inserted
                  << std::setw(9) << std::fixed << std::setprecision(1) << r.optimization_reduction_pct << "%"
                  << std::setw(11) << std::fixed << std::setprecision(1) << r.routing_overhead_pct << "%"
                  << "\n";
    }

    std::cout << "\n";
    std::cout << "Timing:\n";
    std::cout << std::string(50, '-') << "\n";

    double total_opt_time = 0;
    double total_route_time = 0;

    for (const auto& r : results) {
        std::cout << std::left << std::setw(20) << r.name
                  << "  Opt: " << std::setw(8) << std::fixed << std::setprecision(2) << r.optimization_time_ms << " ms"
                  << "  Route: " << std::setw(8) << std::fixed << std::setprecision(2) << r.routing_time_ms << " ms"
                  << "\n";
        total_opt_time += r.optimization_time_ms;
        total_route_time += r.routing_time_ms;
    }

    std::cout << std::string(50, '-') << "\n";
    std::cout << std::left << std::setw(20) << "TOTAL"
              << "  Opt: " << std::setw(8) << std::fixed << std::setprecision(2) << total_opt_time << " ms"
              << "  Route: " << std::setw(8) << std::fixed << std::setprecision(2) << total_route_time << " ms"
              << "\n\n";
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "Generating benchmark circuits...\n";

    std::vector<BenchmarkResult> results;

    // QFT benchmarks
    for (std::size_t n : {4UL, 8UL, 12UL, 16UL}) {
        auto circuit = generateQFT(n);
        auto topology = routing::Topology::grid((n + 3) / 4, 4);
        results.push_back(runBenchmark("QFT-" + std::to_string(n), std::move(circuit), topology));
    }

    // Random circuit benchmarks
    for (auto [n, g] : std::vector<std::pair<std::size_t, std::size_t>>{{10, 100}, {20, 500}, {50, 1000}}) {
        auto circuit = generateRandom(n, g);
        auto topology = routing::Topology::grid((n + 4) / 5, 5);
        results.push_back(runBenchmark("Random-" + std::to_string(n) + "x" + std::to_string(g),
                                        std::move(circuit), topology));
    }

    // Adder benchmarks
    for (std::size_t n : {4UL, 8UL, 16UL}) {
        auto circuit = generateAdder(n);
        auto topology = routing::Topology::linear(2 * n + 1);
        results.push_back(runBenchmark("Adder-" + std::to_string(n), std::move(circuit), topology));
    }

    // QAOA benchmarks
    for (auto [n, p] : std::vector<std::pair<std::size_t, std::size_t>>{{10, 2}, {10, 4}, {20, 2}}) {
        auto circuit = generateQAOA(n, p);
        auto topology = routing::Topology::ring(n);
        results.push_back(runBenchmark("QAOA-" + std::to_string(n) + "-p" + std::to_string(p),
                                        std::move(circuit), topology));
    }

    printResults(results);

    return 0;
}
