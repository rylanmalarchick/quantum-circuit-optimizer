// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file PassManager.hpp
 * @brief Pipeline manager for running optimization passes
 *
 * Provides the PassManager class for building and executing optimization
 * pipelines. Passes are run in sequence, and statistics are aggregated
 * across all passes.
 *
 * @see Pass.hpp for the base pass interface
 * @see DAG.hpp for the circuit representation
 */

#pragma once

#include "Pass.hpp"
#include "../ir/Circuit.hpp"
#include "../ir/DAG.hpp"

#include <memory>
#include <string>
#include <vector>

namespace qopt::passes {

/**
 * @brief Statistics from running an optimization pipeline.
 *
 * Aggregates statistics across all passes in a pipeline, including
 * per-pass breakdowns.
 */
struct PassStatistics {
    /// Total gates removed across all passes.
    std::size_t total_gates_removed = 0;

    /// Total gates added across all passes.
    std::size_t total_gates_added = 0;

    /// Initial gate count before optimization.
    std::size_t initial_gate_count = 0;

    /// Final gate count after optimization.
    std::size_t final_gate_count = 0;

    /// Per-pass statistics: (pass_name, gates_removed, gates_added).
    std::vector<std::tuple<std::string, std::size_t, std::size_t>> per_pass;

    /**
     * @brief Returns the net change in gate count.
     * @return Negative means reduction (good)
     */
    [[nodiscard]] std::ptrdiff_t netChange() const noexcept {
        return static_cast<std::ptrdiff_t>(total_gates_added) -
               static_cast<std::ptrdiff_t>(total_gates_removed);
    }

    /**
     * @brief Returns the percentage reduction in gates.
     * @return Percentage (0-100), 0 if initial count was 0
     */
    [[nodiscard]] double reductionPercent() const noexcept {
        if (initial_gate_count == 0) return 0.0;
        return 100.0 * static_cast<double>(initial_gate_count - final_gate_count) /
               static_cast<double>(initial_gate_count);
    }

    /**
     * @brief Returns a string summary of the statistics.
     * @return Multi-line summary
     */
    [[nodiscard]] std::string toString() const {
        std::string result = "PassManager Statistics:\n";
        result += "  Initial gates: " + std::to_string(initial_gate_count) + "\n";
        result += "  Final gates:   " + std::to_string(final_gate_count) + "\n";
        result += "  Reduction:     " + std::to_string(reductionPercent()) + "%\n";
        result += "  Per-pass:\n";
        for (const auto& [name, removed, added] : per_pass) {
            result += "    " + name + ": -" + std::to_string(removed) +
                      " / +" + std::to_string(added) + "\n";
        }
        return result;
    }
};

/**
 * @brief Manages a pipeline of optimization passes.
 *
 * Passes are added with addPass() and executed in order with run().
 * The same PassManager can be reused on multiple circuits.
 *
 * Example:
 * @code
 * PassManager pm;
 * pm.addPass(std::make_unique<CancellationPass>());
 * pm.addPass(std::make_unique<RotationMergePass>());
 *
 * DAG dag = DAG::fromCircuit(circuit);
 * pm.run(dag);
 *
 * auto stats = pm.statistics();
 * std::cout << stats.toString();
 * @endcode
 */
class PassManager {
public:
    /// Default constructor.
    PassManager() = default;

    // Non-copyable (owns unique_ptrs)
    PassManager(const PassManager&) = delete;
    PassManager& operator=(const PassManager&) = delete;

    // Movable
    PassManager(PassManager&&) noexcept = default;
    PassManager& operator=(PassManager&&) noexcept = default;

    ~PassManager() = default;

    // -------------------------------------------------------------------------
    // Pass Management
    // -------------------------------------------------------------------------

    /**
     * @brief Adds a pass to the pipeline.
     *
     * Passes are executed in the order they are added.
     *
     * @param pass The pass to add (ownership transferred)
     */
    void addPass(std::unique_ptr<Pass> pass) {
        passes_.push_back(std::move(pass));
    }

    /**
     * @brief Returns the number of passes in the pipeline.
     * @return Pass count
     */
    [[nodiscard]] std::size_t numPasses() const noexcept {
        return passes_.size();
    }

    /**
     * @brief Returns whether the pipeline is empty.
     * @return true if no passes have been added
     */
    [[nodiscard]] bool empty() const noexcept {
        return passes_.empty();
    }

    /**
     * @brief Clears all passes from the pipeline.
     */
    void clear() {
        passes_.clear();
        statistics_ = PassStatistics{};
    }

    // -------------------------------------------------------------------------
    // Execution
    // -------------------------------------------------------------------------

    /**
     * @brief Runs all passes on the given DAG.
     *
     * Passes are executed in order. Statistics are accumulated and can
     * be retrieved with statistics().
     *
     * @param dag The DAG to transform (modified in place)
     */
    void run(ir::DAG& dag) {
        statistics_ = PassStatistics{};
        statistics_.initial_gate_count = dag.numNodes();

        for (auto& pass : passes_) {
            pass->resetStatistics();
            pass->run(dag);

            statistics_.total_gates_removed += pass->gatesRemoved();
            statistics_.total_gates_added += pass->gatesAdded();
            statistics_.per_pass.emplace_back(
                pass->name(),
                pass->gatesRemoved(),
                pass->gatesAdded());
        }

        statistics_.final_gate_count = dag.numNodes();
    }

    /**
     * @brief Convenience method to run passes on a Circuit.
     *
     * Converts the circuit to a DAG, runs all passes, and converts back.
     * The original circuit is replaced with the optimized version.
     *
     * @param circuit The circuit to optimize (modified in place)
     */
    void run(ir::Circuit& circuit) {
        ir::DAG dag = ir::DAG::fromCircuit(circuit);
        run(dag);
        circuit = dag.toCircuit();
    }

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    /**
     * @brief Returns statistics from the last run.
     * @return PassStatistics from the most recent run() call
     */
    [[nodiscard]] const PassStatistics& statistics() const noexcept {
        return statistics_;
    }

private:
    std::vector<std::unique_ptr<Pass>> passes_;
    PassStatistics statistics_;
};

}  // namespace qopt::passes
