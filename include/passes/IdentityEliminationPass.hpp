// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file IdentityEliminationPass.hpp
 * @brief Optimization pass to remove identity (no-op) gates
 *
 * Removes rotation gates with zero (or near-zero) angles:
 * - Rz(0) = I
 * - Rx(0) = I
 * - Ry(0) = I
 *
 * Also handles angles that are multiples of 2π.
 *
 * @see Pass.hpp for the base pass interface
 * @see RotationMergePass.hpp which may produce Rz(0) gates
 */

#pragma once

#include "Pass.hpp"
#include "../ir/DAG.hpp"
#include "../ir/Gate.hpp"
#include "../ir/Types.hpp"

#include <cmath>
#include <vector>

namespace qopt::passes {

/**
 * @brief Optimization pass that removes identity rotation gates.
 *
 * Rotation gates with angles that are effectively zero (within tolerance)
 * or multiples of 2π are removed since they have no effect.
 *
 * Example:
 * @code
 * // Before: Rz(0) q[0]; H q[0];
 * // After:  H q[0];
 *
 * IdentityEliminationPass pass;
 * pass.run(dag);
 * @endcode
 */
class IdentityEliminationPass : public Pass {
public:
    /**
     * @brief Constructs the pass with specified tolerance.
     * @param tolerance Angles smaller than this are considered zero
     */
    explicit IdentityEliminationPass(
            double tolerance = constants::TOLERANCE)
        : tolerance_(tolerance)
    {}

    [[nodiscard]] std::string name() const override {
        return "IdentityEliminationPass";
    }

    void run(ir::DAG& dag) override {
        resetStatistics();

        // Collect gates to remove
        std::vector<GateId> to_remove;

        for (GateId id : dag.topologicalOrder()) {
            const ir::Gate& gate = dag.node(id).gate();

            if (isIdentityGate(gate)) {
                to_remove.push_back(id);
            }
        }

        // Remove identity gates
        for (GateId id : to_remove) {
            if (dag.hasNode(id)) {
                dag.removeNode(id);
                gates_removed_++;
            }
        }
    }

private:
    double tolerance_;

    /**
     * @brief Checks if a gate is effectively an identity operation.
     *
     * A rotation gate is identity if its angle is a multiple of 2π
     * (within tolerance).
     *
     * @param gate The gate to check
     * @return true if the gate is equivalent to identity
     */
    [[nodiscard]] bool isIdentityGate(const ir::Gate& gate) const noexcept {
        // Only rotation gates can be identity based on parameter
        if (!ir::isParameterized(gate.type())) {
            return false;
        }

        // Check if angle is effectively zero or 2πn
        Angle angle = gate.parameter().value_or(0.0);
        return isEffectivelyZero(angle);
    }

    /**
     * @brief Checks if an angle is effectively zero (mod 2π).
     * @param angle The angle in radians
     * @return true if angle ≈ 0 (mod 2π)
     */
    [[nodiscard]] bool isEffectivelyZero(Angle angle) const noexcept {
        constexpr Angle TWO_PI = 2.0 * constants::PI;

        // Reduce modulo 2π
        Angle reduced = std::fmod(std::abs(angle), TWO_PI);

        // Check if close to 0 or 2π
        return reduced < tolerance_ || (TWO_PI - reduced) < tolerance_;
    }
};

}  // namespace qopt::passes
