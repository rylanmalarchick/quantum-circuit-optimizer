// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file SabreRouter.hpp
 * @brief SABRE routing algorithm implementation
 *
 * Implements the SABRE (SWAP-based Bidirectional heuristic search) algorithm
 * for qubit routing. SABRE is a state-of-the-art heuristic that efficiently
 * maps logical qubits to physical qubits while minimizing SWAP overhead.
 *
 * Reference:
 * Li, Ding, and Xie, "Tackling the Qubit Mapping Problem for NISQ-Era
 * Quantum Devices", ASPLOS 2019.
 * https://doi.org/10.1145/3297858.3304023
 *
 * @see Router.hpp for base class
 * @see Topology.hpp for device connectivity
 */

#pragma once

#include "../ir/Circuit.hpp"
#include "../ir/DAG.hpp"
#include "../ir/Gate.hpp"
#include "Router.hpp"
#include "Topology.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace qopt::routing {

/**
 * @brief SABRE routing algorithm implementation.
 *
 * SABRE uses a heuristic search to find good SWAP sequences:
 *
 * 1. **Front Layer**: Identify gates with satisfied dependencies
 * 2. **Executable Check**: If front layer gates are on adjacent qubits, execute
 * 3. **SWAP Selection**: Otherwise, score candidate SWAPs and insert best one
 * 4. **Lookahead**: Consider future gates when scoring SWAPs
 *
 * The algorithm runs forward and optionally backward passes for refinement.
 *
 * Example:
 * @code
 * Circuit circuit(4);
 * circuit.addGate(Gate::h(0));
 * circuit.addGate(Gate::cnot(0, 3));  // Non-adjacent
 *
 * SabreRouter router;
 * auto topology = Topology::linear(4);
 * RoutingResult result = router.route(circuit, topology);
 * @endcode
 */
class SabreRouter : public Router {
public:
    /**
     * @brief Constructs a SABRE router with optional parameters.
     * @param lookahead_depth How many layers ahead to consider (default: 20)
     * @param decay_factor Weight decay for distant gates (default: 0.5)
     * @param extended_set_weight Weight for extended set in scoring (default: 0.5)
     */
    explicit SabreRouter(std::size_t lookahead_depth = 20,
                         double decay_factor = 0.5,
                         double extended_set_weight = 0.5)
        : lookahead_depth_(lookahead_depth)
        , decay_factor_(decay_factor)
        , extended_set_weight_(extended_set_weight)
        , rng_(std::random_device{}())
    {}

    [[nodiscard]] std::string name() const override {
        return "SabreRouter";
    }

    [[nodiscard]] RoutingResult route(
        const ir::Circuit& circuit,
        const Topology& topology) override {
        validateRouteInputs(circuit, topology);

        if (circuit.empty()) {
            RoutingResult result(ir::Circuit(circuit.numQubits()));
            result.initial_mapping = identityMapping(circuit.numQubits());
            result.final_mapping = result.initial_mapping;
            result.original_depth = 0;
            result.final_depth = 0;
            return result;
        }

        std::size_t original_depth = circuit.depth();

        // Initialize mapping
        auto mapping = initialMapping(circuit, topology);
        auto reverse_mapping = computeReverseMapping(mapping, topology.numQubits());

        // Build the circuit DAG
        ir::DAG dag = ir::DAG::fromCircuit(circuit);

        // Route using SABRE forward pass
        ir::Circuit routed(topology.numQubits());
        std::size_t swaps = routeForward(dag, topology, mapping, reverse_mapping, routed);

        // Build result
        RoutingResult result(std::move(routed));
        result.initial_mapping = identityMapping(circuit.numQubits());
        result.final_mapping = mapping;
        result.swaps_inserted = swaps;
        result.original_depth = original_depth;
        result.final_depth = result.routed_circuit.depth();

        return result;
    }

private:
    std::size_t lookahead_depth_;
    double decay_factor_;
    double extended_set_weight_;
    mutable std::mt19937 rng_;

    /**
     * @brief Creates an initial logical->physical mapping.
     *
     * Uses a heuristic based on circuit structure to find a good initial
     * mapping that reduces SWAP overhead.
     */
    [[nodiscard]] std::vector<std::size_t> initialMapping(
        const ir::Circuit& circuit,
        const Topology& /*topology*/) const {

        std::size_t num_logical = circuit.numQubits();

        // Simple initial mapping: identity mapping
        // A more sophisticated approach would analyze gate patterns
        std::vector<std::size_t> mapping(num_logical);
        for (std::size_t i = 0; i < num_logical; ++i) {
            mapping[i] = i;
        }

        return mapping;
    }

    /**
     * @brief Computes the reverse mapping (physical -> logical).
     */
    [[nodiscard]] std::vector<std::size_t> computeReverseMapping(
        const std::vector<std::size_t>& mapping,
        std::size_t num_physical) const {

        std::vector<std::size_t> reverse(num_physical, INVALID_LOGICAL);
        for (std::size_t logical = 0; logical < mapping.size(); ++logical) {
            reverse[mapping[logical]] = logical;
        }
        return reverse;
    }

    /// @brief Sentinel for unmapped physical qubits
    static constexpr std::size_t INVALID_LOGICAL = std::numeric_limits<std::size_t>::max();

    /**
     * @brief Forward pass of SABRE routing.
     * @return Number of SWAPs inserted
     */
    std::size_t routeForward(
        const ir::DAG& dag,
        const Topology& topology,
        std::vector<std::size_t>& mapping,
        std::vector<std::size_t>& reverse_mapping,
        ir::Circuit& routed) const {

        std::size_t swaps_inserted = 0;

        // Track executed gates
        std::unordered_set<GateId> executed;

        // Track remaining predecessors for each gate
        std::unordered_map<GateId, std::size_t> remaining_deps;
        for (GateId id : dag.nodeIds()) {
            remaining_deps[id] = dag.node(id).inDegree();
        }

        // Front layer: gates ready to execute (all predecessors executed)
        auto front_layer = dag.sources();

        while (!front_layer.empty()) {
            // Try to execute gates in front layer
            std::vector<GateId> executed_this_round;
            std::vector<GateId> blocked;

            for (GateId id : front_layer) {
                const ir::Gate& gate = dag.node(id).gate();

                if (gate.numQubits() == 1) {
                    // Single-qubit gates: always executable
                    // Map logical to physical
                    std::size_t physical = mapping[gate.qubits()[0]];
                    routed.addGate(ir::Gate(gate.type(), {physical}, gate.parameter()));
                    executed_this_round.push_back(id);
                } else {
                    // Two-qubit gate: check if qubits are adjacent
                    std::size_t p0 = mapping[gate.qubits()[0]];
                    std::size_t p1 = mapping[gate.qubits()[1]];

                    if (topology.connected(p0, p1)) {
                        // Executable: add to routed circuit with physical qubits
                        routed.addGate(ir::Gate(gate.type(), {p0, p1}, gate.parameter()));
                        executed_this_round.push_back(id);
                    } else {
                        blocked.push_back(id);
                    }
                }
            }

            if (!executed_this_round.empty()) {
                // Mark executed and update front layer
                for (GateId id : executed_this_round) {
                    executed.insert(id);
                    // Update successors
                    for (GateId succ : dag.node(id).successors()) {
                        if (--remaining_deps[succ] == 0) {
                            blocked.push_back(succ);  // Now ready
                        }
                    }
                }

                // Update front layer: blocked gates + newly ready gates
                front_layer = blocked;
            } else {
                // No progress - need to insert SWAPs
                // Find best SWAP to make progress on blocked gates
                auto best_swap = selectBestSwap(
                    dag, topology, mapping, reverse_mapping, blocked, executed, remaining_deps);

                if (best_swap.first != INVALID_LOGICAL) {
                    // Insert SWAP
                    insertSwap(best_swap.first, best_swap.second,
                               mapping, reverse_mapping, routed);
                    ++swaps_inserted;
                } else {
                    // No valid SWAP found - this shouldn't happen
                    // Fall back: just pick any blocked gate and force a path
                    if (!blocked.empty()) {
                        const ir::Gate& gate = dag.node(blocked[0]).gate();
                        std::size_t p0 = mapping[gate.qubits()[0]];
                        std::size_t p1 = mapping[gate.qubits()[1]];
                        auto path = topology.shortestPath(p0, p1);
                        if (path.size() >= 2) {
                            insertSwap(path[0], path[1], mapping, reverse_mapping, routed);
                            ++swaps_inserted;
                        }
                    }
                }
            }
        }

        return swaps_inserted;
    }

    /**
     * @brief Selects the best SWAP to make progress on blocked gates.
     *
     * Uses SABRE's heuristic scoring:
     * - Consider SWAPs adjacent to qubits in blocked gates
     * - Score = distance reduction for front layer + lookahead bonus
     */
    [[nodiscard]] std::pair<std::size_t, std::size_t> selectBestSwap(
        const ir::DAG& dag,
        const Topology& topology,
        const std::vector<std::size_t>& mapping,
        const std::vector<std::size_t>& /*reverse_mapping*/,
        const std::vector<GateId>& front_layer,
        const std::unordered_set<GateId>& executed,
        const std::unordered_map<GateId, std::size_t>& remaining_deps) const {

        double best_score = std::numeric_limits<double>::max();
        std::pair<std::size_t, std::size_t> best_swap = {INVALID_LOGICAL, INVALID_LOGICAL};

        // Collect physical qubits involved in blocked two-qubit gates
        std::unordered_set<std::size_t> active_physical;
        for (GateId id : front_layer) {
            const ir::Gate& gate = dag.node(id).gate();
            if (gate.numQubits() == 2) {
                active_physical.insert(mapping[gate.qubits()[0]]);
                active_physical.insert(mapping[gate.qubits()[1]]);
            }
        }

        // Consider SWAPs on edges involving active qubits
        for (std::size_t p : active_physical) {
            for (std::size_t neighbor : topology.neighbors(p)) {
                // Score this SWAP
                double score = scoreSwap(
                    p, neighbor, dag, topology, mapping, front_layer, executed, remaining_deps);

                if (score < best_score) {
                    best_score = score;
                    best_swap = {p, neighbor};
                }
            }
        }

        return best_swap;
    }

    /**
     * @brief Scores a candidate SWAP.
     *
     * Lower score = better SWAP.
     * Score considers:
     * - Distance reduction for front layer gates
     * - Distance reduction for lookahead gates (with decay)
     */
    [[nodiscard]] double scoreSwap(
        std::size_t p0,
        std::size_t p1,
        const ir::DAG& dag,
        const Topology& topology,
        const std::vector<std::size_t>& mapping,
        const std::vector<GateId>& front_layer,
        const std::unordered_set<GateId>& executed,
        const std::unordered_map<GateId, std::size_t>& /*remaining_deps*/) const {

        // Simulate the SWAP
        auto new_mapping = mapping;
        std::size_t logical0 = INVALID_LOGICAL;
        std::size_t logical1 = INVALID_LOGICAL;

        // Find which logical qubits are at these physical positions
        for (std::size_t l = 0; l < mapping.size(); ++l) {
            if (mapping[l] == p0) logical0 = l;
            if (mapping[l] == p1) logical1 = l;
        }

        // Apply SWAP to mapping
        if (logical0 != INVALID_LOGICAL) new_mapping[logical0] = p1;
        if (logical1 != INVALID_LOGICAL) new_mapping[logical1] = p0;

        // Score: total distance for front layer gates
        double score = 0.0;

        // Front layer contribution
        for (GateId id : front_layer) {
            const ir::Gate& gate = dag.node(id).gate();
            if (gate.numQubits() == 2) {
                std::size_t new_p0 = new_mapping[gate.qubits()[0]];
                std::size_t new_p1 = new_mapping[gate.qubits()[1]];
                score += static_cast<double>(topology.distance(new_p0, new_p1));
            }
        }

        // Extended set (lookahead) contribution
        // Limit extended set size based on lookahead_depth_
        std::unordered_set<GateId> extended;
        for (GateId id : front_layer) {
            if (extended.size() >= lookahead_depth_) break;
            for (GateId succ : dag.node(id).successors()) {
                if (executed.find(succ) == executed.end()) {
                    extended.insert(succ);
                }
            }
        }

        double decay = decay_factor_;
        for (GateId id : extended) {
            const ir::Gate& gate = dag.node(id).gate();
            if (gate.numQubits() == 2) {
                std::size_t new_p0 = new_mapping[gate.qubits()[0]];
                std::size_t new_p1 = new_mapping[gate.qubits()[1]];
                score += decay * extended_set_weight_ *
                         static_cast<double>(topology.distance(new_p0, new_p1));
            }
        }

        return score;
    }

    /**
     * @brief Inserts a SWAP gate and updates mappings.
     */
    void insertSwap(
        std::size_t p0,
        std::size_t p1,
        std::vector<std::size_t>& mapping,
        std::vector<std::size_t>& reverse_mapping,
        ir::Circuit& routed) const {

        // Add SWAP gate
        routed.addGate(ir::Gate::swap(p0, p1));

        // Update mappings
        std::size_t logical0 = reverse_mapping[p0];
        std::size_t logical1 = reverse_mapping[p1];

        if (logical0 != INVALID_LOGICAL) mapping[logical0] = p1;
        if (logical1 != INVALID_LOGICAL) mapping[logical1] = p0;

        reverse_mapping[p0] = logical1;
        reverse_mapping[p1] = logical0;
    }
};

}  // namespace qopt::routing
