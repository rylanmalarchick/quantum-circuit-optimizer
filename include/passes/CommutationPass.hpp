// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file CommutationPass.hpp
 * @brief Optimization pass to reorder commuting gates
 *
 * Moves gates past each other when they commute to enable further
 * optimizations (e.g., bringing inverse pairs together for cancellation).
 *
 * Commutation rules implemented:
 * - Diagonal gates commute: Z, S, Sdg, T, Tdg, Rz, CZ
 * - Z commutes with CNOT control
 * - X commutes with CNOT target
 * - Same-axis rotations commute: [Rz(a), Rz(b)] = 0
 *
 * @see Pass.hpp for the base pass interface
 * @see CancellationPass.hpp which benefits from commutation
 */

#pragma once

#include "Pass.hpp"
#include "../ir/DAG.hpp"
#include "../ir/Gate.hpp"
#include "../ir/Types.hpp"

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace qopt::passes {

/**
 * @brief Optimization pass that reorders commuting gates.
 *
 * Attempts to move gates through each other when they commute,
 * with the goal of bringing cancellable pairs together.
 *
 * This pass is a "setup" pass - it doesn't reduce gate count directly
 * but enables CancellationPass and RotationMergePass to find more
 * opportunities.
 *
 * Example:
 * @code
 * // Before: Z q[0]; CNOT q[0], q[1]; Z q[0];
 * // After:  Z q[0]; Z q[0]; CNOT q[0], q[1];  (Z commutes through the control;
 * //                                            the two Z gates then cancel)
 *
 * CommutationPass pass;
 * pass.run(dag);
 * @endcode
 */
class CommutationPass : public Pass {
public:
    CommutationPass() = default;

    [[nodiscard]] std::string name() const override {
        return "CommutationPass";
    }

    void run(ir::DAG& dag) override {
        resetStatistics();

        // Greedily move commuting gates together so a later cancellation or
        // merge pass can act on them. Each accepted swap reorders one adjacent
        // commuting pair and never changes gate count. Work proceeds in full
        // sweeps; the sweep count is bounded by the gate count so the pass
        // always terminates (JPL Rule 2: every loop has a fixed bound).
        const std::size_t max_sweeps = dag.numNodes() + 1;
        for (std::size_t sweep = 0; sweep < max_sweeps; ++sweep) {
            if (!sweepOnce(dag)) break;
        }
    }

private:
    /**
     * @brief One reordering sweep over the DAG in topological order.
     *
     * For each gate, performs at most one beneficial swap with an adjacent,
     * commuting successor (one whose reorder places a gate next to a cancel or
     * merge partner).
     *
     * @return true if any swap was made during the sweep
     */
    bool sweepOnce(ir::DAG& dag) {
        bool changed = false;
        for (GateId a : dag.topologicalOrder()) {
            if (!dag.hasNode(a)) continue;
            // Copy the successor list: swapAdjacent rewires edges mid-iteration.
            const std::vector<GateId> succs = dag.node(a).successors();
            for (GateId b : succs) {
                if (dag.canSwapAdjacent(a, b) && shouldSwap(dag, a, b)) {
                    dag.swapAdjacent(a, b);
                    changed = true;
                    break;  // a has moved; continue the sweep from the next gate
                }
            }
        }
        return changed;
    }

    /**
     * @brief Checks if a gate is diagonal (commutes with other diagonals).
     *
     * Diagonal gates are those that are diagonal in the computational basis:
     * Z, S, Sdg, T, Tdg, Rz, CZ
     *
     * @param type The gate type
     * @return true if diagonal
     */
    [[nodiscard]] static bool isDiagonal(ir::GateType type) noexcept {
        switch (type) {
            case ir::GateType::Z:
            case ir::GateType::S:
            case ir::GateType::Sdg:
            case ir::GateType::T:
            case ir::GateType::Tdg:
            case ir::GateType::Rz:
            case ir::GateType::CZ:
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Checks if two gates commute.
     *
     * @param g1 First gate
     * @param g2 Second gate
     * @return true if [g1, g2] = 0
     */
    [[nodiscard]] static bool commute(
            const ir::Gate& g1,
            const ir::Gate& g2) noexcept {
        // Gates on disjoint qubits always commute
        if (!qubitsOverlap(g1, g2)) {
            return true;
        }

        // Identical gates commute
        if (g1.type() == g2.type() && g1.qubits() == g2.qubits()) {
            return true;
        }

        // Diagonal gates commute with each other
        if (isDiagonal(g1.type()) && isDiagonal(g2.type())) {
            return true;
        }

        // Z commutes with CNOT control
        if (isZLike(g1.type()) && g2.type() == ir::GateType::CNOT) {
            // Z on qubit q commutes with CNOT if q is the control
            if (g1.qubits()[0] == g2.qubits()[0]) {  // control is qubits[0]
                return true;
            }
        }
        if (isZLike(g2.type()) && g1.type() == ir::GateType::CNOT) {
            if (g2.qubits()[0] == g1.qubits()[0]) {
                return true;
            }
        }

        // X commutes with CNOT target
        if (g1.type() == ir::GateType::X && g2.type() == ir::GateType::CNOT) {
            if (g1.qubits()[0] == g2.qubits()[1]) {  // target is qubits[1]
                return true;
            }
        }
        if (g2.type() == ir::GateType::X && g1.type() == ir::GateType::CNOT) {
            if (g2.qubits()[0] == g1.qubits()[1]) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Checks if gate is Z-like (diagonal single-qubit).
     */
    [[nodiscard]] static bool isZLike(ir::GateType type) noexcept {
        switch (type) {
            case ir::GateType::Z:
            case ir::GateType::S:
            case ir::GateType::Sdg:
            case ir::GateType::T:
            case ir::GateType::Tdg:
            case ir::GateType::Rz:
                return true;
            default:
                return false;
        }
    }

    /**
     * @brief Checks if two gates share any qubits.
     */
    [[nodiscard]] static bool qubitsOverlap(
            const ir::Gate& g1,
            const ir::Gate& g2) noexcept {
        for (auto q1 : g1.qubits()) {
            for (auto q2 : g2.qubits()) {
                if (q1 == q2) return true;
            }
        }
        return false;
    }

    /**
     * @brief Determines whether reordering the adjacent pair (a, b) helps.
     *
     * @p a is the earlier gate and @p b its immediate successor. Moving @p b
     * ahead of @p a places it next to @p a's predecessor on each shared wire;
     * that is worthwhile only if the predecessor can then cancel or merge with
     * @p b. Correctness does not depend on this heuristic: the later pass
     * re-checks adjacency before acting, so a wasted swap is harmless.
     *
     * @param dag The DAG
     * @param a Earlier gate ID
     * @param b Immediate-successor gate ID
     * @return true if the swap is worth performing
     */
    [[nodiscard]] static bool shouldSwap(const ir::DAG& dag, GateId a, GateId b) {
        const ir::Gate& ga = dag.node(a).gate();
        const ir::Gate& gb = dag.node(b).gate();

        if (!commute(ga, gb)) {
            return false;
        }

        for (QubitIndex q : ga.qubits()) {
            const GateId pa = dag.wirePredecessor(a, q);
            if (pa == INVALID_GATE_ID) continue;
            const ir::Gate& gpa = dag.node(pa).gate();
            if (couldCancel(gpa, gb) || couldMerge(gpa, gb)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Checks if two gates could cancel.
     */
    [[nodiscard]] static bool couldCancel(
            const ir::Gate& g1,
            const ir::Gate& g2) noexcept {
        if (g1.qubits() != g2.qubits()) return false;

        // Hermitian gates
        if (ir::isHermitian(g1.type()) && g1.type() == g2.type()) {
            return true;
        }

        // Adjoint pairs
        using ir::GateType;
        if ((g1.type() == GateType::S && g2.type() == GateType::Sdg) ||
            (g1.type() == GateType::Sdg && g2.type() == GateType::S) ||
            (g1.type() == GateType::T && g2.type() == GateType::Tdg) ||
            (g1.type() == GateType::Tdg && g2.type() == GateType::T)) {
            return true;
        }

        return false;
    }

    /**
     * @brief Checks if two gates could merge.
     */
    [[nodiscard]] static bool couldMerge(
            const ir::Gate& g1,
            const ir::Gate& g2) noexcept {
        if (g1.qubits() != g2.qubits()) return false;

        // Same rotation type
        if (g1.type() == g2.type() && ir::isParameterized(g1.type())) {
            return true;
        }

        return false;
    }

};

}  // namespace qopt::passes
