// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file CancellationPass.hpp
 * @brief Optimization pass to cancel adjacent inverse gate pairs
 *
 * Identifies and removes pairs of adjacent gates that cancel to identity:
 * - Hermitian gates: H·H = I, X·X = I, Y·Y = I, Z·Z = I
 * - Adjoint pairs: S·S† = I, T·T† = I
 * - Two-qubit: CNOT·CNOT = I, CZ·CZ = I, SWAP·SWAP = I (same qubits)
 *
 * @see Pass.hpp for the base pass interface
 * @see DAG.hpp for the circuit representation
 */

#pragma once

#include "Pass.hpp"
#include "../ir/DAG.hpp"
#include "../ir/Gate.hpp"

#include <unordered_set>
#include <vector>

namespace qopt::passes {

/**
 * @brief Optimization pass that cancels adjacent inverse gate pairs.
 *
 * Traverses the DAG looking for pairs of adjacent gates that multiply
 * to the identity. When found, both gates are removed from the circuit.
 *
 * Adjacent gates are defined as: gate B is a successor of gate A, and
 * they operate on exactly the same qubits with no intervening gates
 * on those qubits.
 *
 * Example:
 * @code
 * // Before: H q[0]; H q[0];   (two Hadamards on same qubit)
 * // After:  (empty)           (cancelled)
 *
 * CancellationPass pass;
 * pass.run(dag);
 * @endcode
 */
class CancellationPass : public Pass {
public:
    CancellationPass() = default;

    [[nodiscard]] std::string name() const override {
        return "CancellationPass";
    }

    void run(ir::DAG& dag) override {
        resetStatistics();

        // Track nodes to remove (we can't remove during iteration)
        std::unordered_set<GateId> to_remove;

        // Get topological order for consistent traversal
        std::vector<GateId> order = dag.topologicalOrder();

        for (GateId id : order) {
            // Skip if already marked for removal
            if (to_remove.count(id) > 0) continue;
            if (!dag.hasNode(id)) continue;

            const ir::DAGNode& node = dag.node(id);
            const ir::Gate& gate = node.gate();

            // Check each successor for potential cancellation
            for (GateId succ_id : node.successors()) {
                if (to_remove.count(succ_id) > 0) continue;
                if (!dag.hasNode(succ_id)) continue;

                const ir::Gate& succ_gate = dag.node(succ_id).gate();

                // Both gates must be adjacent on every shared wire, not merely
                // connected by a single edge: a gate intervening on one wire of
                // a two-qubit pair would otherwise be skipped over.
                if (dag.areWireAdjacent(id, succ_id) &&
                    areCancellingPair(gate, succ_gate)) {
                    // Mark both for removal
                    to_remove.insert(id);
                    to_remove.insert(succ_id);
                    gates_removed_ += 2;
                    break;  // Each gate can only cancel once
                }
            }
        }

        // Remove marked nodes (in reverse topological order to maintain validity)
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            if (to_remove.count(*it) > 0 && dag.hasNode(*it)) {
                dag.removeNode(*it);
            }
        }
    }

private:
    /**
     * @brief Checks if two gates cancel to identity.
     *
     * Cancellation rules:
     * - Hermitian gates cancel with themselves: H·H, X·X, Y·Y, Z·Z
     * - Adjoint pairs: S·Sdg, Sdg·S, T·Tdg, Tdg·T
     * - Two-qubit Hermitian: CNOT·CNOT, CZ·CZ, SWAP·SWAP
     *
     * @param g1 First gate
     * @param g2 Second gate
     * @return true if g1·g2 = I
     */
    [[nodiscard]] static bool areCancellingPair(
            const ir::Gate& g1,
            const ir::Gate& g2) {
        using ir::GateType;

        // Hermitian gates cancel with themselves
        if (ir::isHermitian(g1.type())) {
            return g1.type() == g2.type();
        }

        // Adjoint pairs
        switch (g1.type()) {
            case GateType::S:
                return g2.type() == GateType::Sdg;
            case GateType::Sdg:
                return g2.type() == GateType::S;
            case GateType::T:
                return g2.type() == GateType::Tdg;
            case GateType::Tdg:
                return g2.type() == GateType::T;
            default:
                break;
        }

        // Rotation gates: Rx(a)·Rx(-a) = I, etc.
        // This is handled by RotationMergePass instead
        // (merge to Rx(0), then IdentityEliminationPass removes it)

        return false;
    }
};

}  // namespace qopt::passes
