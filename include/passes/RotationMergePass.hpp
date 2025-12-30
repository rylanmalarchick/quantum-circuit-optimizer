// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file RotationMergePass.hpp
 * @brief Optimization pass to merge adjacent rotation gates
 *
 * Merges adjacent rotation gates of the same type into a single gate:
 * - Rz(a) · Rz(b) = Rz(a + b)
 * - Rx(a) · Rx(b) = Rx(a + b)
 * - Ry(a) · Ry(b) = Ry(a + b)
 *
 * Angles are normalized to [-π, π] after merging.
 *
 * @see Pass.hpp for the base pass interface
 * @see IdentityEliminationPass.hpp for removing Rz(0) etc.
 */

#pragma once

#include "Pass.hpp"
#include "../ir/DAG.hpp"
#include "../ir/Gate.hpp"
#include "../ir/Types.hpp"

#include <cmath>
#include <unordered_set>
#include <vector>

namespace qopt::passes {

/**
 * @brief Optimization pass that merges adjacent rotation gates.
 *
 * When two rotation gates of the same type (Rx, Ry, or Rz) are adjacent
 * on the same qubit, they are merged into a single rotation with the
 * sum of their angles.
 *
 * Example:
 * @code
 * // Before: Rz(π/4) q[0]; Rz(π/4) q[0];
 * // After:  Rz(π/2) q[0];
 *
 * RotationMergePass pass;
 * pass.run(dag);
 * @endcode
 */
class RotationMergePass : public Pass {
public:
    RotationMergePass() = default;

    [[nodiscard]] std::string name() const override {
        return "RotationMergePass";
    }

    void run(ir::DAG& dag) override {
        resetStatistics();

        // Track nodes to remove
        std::unordered_set<GateId> to_remove;

        // Keep merging until no more changes
        bool changed = true;
        while (changed) {
            changed = false;
            std::vector<GateId> order = dag.topologicalOrder();

            for (GateId id : order) {
                if (to_remove.count(id) > 0) continue;
                if (!dag.hasNode(id)) continue;

                ir::DAGNode& node = dag.node(id);
                const ir::Gate& gate = node.gate();

                // Only process rotation gates
                if (!isRotationGate(gate.type())) continue;

                // Look for adjacent rotation of same type
                for (GateId succ_id : node.successors()) {
                    if (to_remove.count(succ_id) > 0) continue;
                    if (!dag.hasNode(succ_id)) continue;

                    const ir::Gate& succ_gate = dag.node(succ_id).gate();

                    // Check if mergeable
                    if (canMerge(dag, id, succ_id)) {
                        // Merge: update first gate's angle, remove second
                        Angle new_angle = normalizeAngle(
                            gate.parameter().value() +
                            succ_gate.parameter().value());

                        // Create new gate with merged angle
                        ir::Gate merged(gate.type(), gate.qubits(), new_angle);
                        node.gate() = merged;

                        // Mark successor for removal
                        to_remove.insert(succ_id);
                        gates_removed_ += 1;  // One removed (other updated)
                        changed = true;
                        break;
                    }
                }
            }

            // Remove marked nodes
            for (GateId id : to_remove) {
                if (dag.hasNode(id)) {
                    dag.removeNode(id);
                }
            }
            to_remove.clear();
        }
    }

private:
    /**
     * @brief Checks if a gate type is a rotation gate.
     * @param type The gate type
     * @return true for Rx, Ry, Rz
     */
    [[nodiscard]] static bool isRotationGate(ir::GateType type) noexcept {
        return type == ir::GateType::Rx ||
               type == ir::GateType::Ry ||
               type == ir::GateType::Rz;
    }

    /**
     * @brief Checks if two rotation gates can be merged.
     *
     * Gates can be merged if:
     * 1. They are the same rotation type (Rx, Ry, or Rz)
     * 2. They operate on the same qubit
     * 3. They are adjacent (direct edge, same qubit)
     *
     * @param dag The DAG
     * @param id1 First gate ID
     * @param id2 Second gate ID
     * @return true if mergeable
     */
    [[nodiscard]] static bool canMerge(
            const ir::DAG& dag,
            GateId id1,
            GateId id2) {
        const ir::Gate& g1 = dag.node(id1).gate();
        const ir::Gate& g2 = dag.node(id2).gate();

        // Same rotation type
        if (g1.type() != g2.type()) return false;
        if (!isRotationGate(g1.type())) return false;

        // Same qubit
        if (g1.qubits() != g2.qubits()) return false;

        // Direct edge (adjacent)
        if (!dag.hasEdge(id1, id2)) return false;

        return true;
    }

    /**
     * @brief Normalizes an angle to the range [-π, π].
     * @param angle The angle in radians
     * @return Normalized angle
     */
    [[nodiscard]] static Angle normalizeAngle(Angle angle) noexcept {
        constexpr Angle TWO_PI = 2.0 * constants::PI;

        // Reduce to [-2π, 2π]
        angle = std::fmod(angle, TWO_PI);

        // Normalize to [-π, π]
        if (angle > constants::PI) {
            angle -= TWO_PI;
        } else if (angle < -constants::PI) {
            angle += TWO_PI;
        }

        return angle;
    }
};

}  // namespace qopt::passes
