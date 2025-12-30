// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Pass.hpp
 * @brief Base class for optimization passes
 *
 * Provides the abstract Pass base class that all optimization passes inherit
 * from. Passes transform DAG representations of quantum circuits to reduce
 * gate count, circuit depth, or other metrics.
 *
 * @see PassManager.hpp for running passes in sequence
 * @see DAG.hpp for the circuit representation passes operate on
 */

#pragma once

#include "../ir/DAG.hpp"

#include <cstddef>
#include <string>

namespace qopt::passes {

/**
 * @brief Abstract base class for optimization passes.
 *
 * All optimization passes inherit from this class and implement the run()
 * method to transform a DAG. Passes track statistics about their effect
 * on the circuit (gates removed/added).
 *
 * Example implementation:
 * @code
 * class MyPass : public Pass {
 * public:
 *     std::string name() const override { return "MyPass"; }
 *     void run(ir::DAG& dag) override {
 *         // Transform the DAG
 *         // Update gates_removed_, gates_added_
 *     }
 * };
 * @endcode
 */
class Pass {
public:
    /// Virtual destructor for proper cleanup of derived classes.
    virtual ~Pass() = default;

    // Non-copyable, non-movable (passes are used via unique_ptr)
    Pass(const Pass&) = delete;
    Pass& operator=(const Pass&) = delete;
    Pass(Pass&&) = delete;
    Pass& operator=(Pass&&) = delete;

    // -------------------------------------------------------------------------
    // Abstract Interface
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the name of this pass.
     * @return Human-readable pass name for logging/debugging
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Runs the optimization pass on the given DAG.
     *
     * Transforms the DAG in place. Statistics (gates removed/added) should
     * be updated during execution.
     *
     * @param dag The DAG to transform (modified in place)
     */
    virtual void run(ir::DAG& dag) = 0;

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    /**
     * @brief Returns the number of gates removed by this pass.
     * @return Count of removed gates from the last run() call
     */
    [[nodiscard]] std::size_t gatesRemoved() const noexcept {
        return gates_removed_;
    }

    /**
     * @brief Returns the number of gates added by this pass.
     * @return Count of added gates from the last run() call
     */
    [[nodiscard]] std::size_t gatesAdded() const noexcept {
        return gates_added_;
    }

    /**
     * @brief Returns the net change in gate count.
     * @return gatesAdded() - gatesRemoved() (negative means reduction)
     */
    [[nodiscard]] std::ptrdiff_t netChange() const noexcept {
        return static_cast<std::ptrdiff_t>(gates_added_) -
               static_cast<std::ptrdiff_t>(gates_removed_);
    }

    /**
     * @brief Resets statistics counters to zero.
     *
     * Called automatically at the start of run() by derived classes.
     */
    void resetStatistics() noexcept {
        gates_removed_ = 0;
        gates_added_ = 0;
    }

protected:
    /// Default constructor (only callable by derived classes).
    Pass() = default;

    /// Number of gates removed during the last run.
    std::size_t gates_removed_ = 0;

    /// Number of gates added during the last run.
    std::size_t gates_added_ = 0;
};

}  // namespace qopt::passes
