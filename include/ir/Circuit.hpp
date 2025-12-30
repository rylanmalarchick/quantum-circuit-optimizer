// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Circuit.hpp
 * @brief Quantum circuit container and operations
 *
 * Provides the Circuit class for building and manipulating quantum circuits.
 * Circuits consist of a qubit register and a sequence of gates. The class
 * supports iteration, gate management, and circuit metrics.
 *
 * @see Gate.hpp for gate representation
 * @see DAG.hpp for dependency graph representation (Sprint 1B)
 */

#pragma once

#include "Gate.hpp"
#include "Qubit.hpp"
#include "Types.hpp"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace qopt::ir {

/**
 * @brief A quantum circuit consisting of qubits and gates.
 *
 * Circuits are containers for quantum gates applied to a fixed-size qubit
 * register. Gates are stored in application order and can be iterated.
 *
 * Example:
 * @code
 * Circuit circuit(2);  // 2-qubit circuit
 * circuit.addGate(Gate::h(0));
 * circuit.addGate(Gate::cnot(0, 1));
 *
 * for (const auto& gate : circuit) {
 *     std::cout << gate.toString() << "\n";
 * }
 * @endcode
 */
class Circuit {
public:
    using iterator = std::vector<Gate>::iterator;
    using const_iterator = std::vector<Gate>::const_iterator;

    /**
     * @brief Constructs an empty circuit with the specified number of qubits.
     * @param num_qubits Number of qubits in the circuit register
     * @throws std::invalid_argument if num_qubits is 0 or exceeds MAX_QUBITS
     */
    explicit Circuit(std::size_t num_qubits)
        : num_qubits_(num_qubits)
        , next_gate_id_(0)
    {
        if (num_qubits == 0) {
            throw std::invalid_argument("Circuit must have at least 1 qubit");
        }
        if (num_qubits > constants::MAX_QUBITS) {
            throw std::invalid_argument(
                "Circuit exceeds maximum qubit count of " +
                std::to_string(constants::MAX_QUBITS));
        }
    }

    // Move semantics (circuits can be large)
    Circuit(Circuit&&) noexcept = default;
    Circuit& operator=(Circuit&&) noexcept = default;

    // Delete copy (use clone() if needed)
    Circuit(const Circuit&) = delete;
    Circuit& operator=(const Circuit&) = delete;

    ~Circuit() noexcept = default;

    // -------------------------------------------------------------------------
    // Gate Management
    // -------------------------------------------------------------------------

    /**
     * @brief Adds a gate to the circuit.
     * @param gate The gate to add
     * @throws std::out_of_range if gate references qubit beyond circuit size
     */
    void addGate(Gate gate) {
        validateGateQubits(gate);
        gate.setId(next_gate_id_++);
        gates_.push_back(std::move(gate));
    }

    /**
     * @brief Returns the gate at the specified index.
     * @param index Gate index
     * @return Reference to the gate
     * @throws std::out_of_range if index >= numGates()
     */
    [[nodiscard]] const Gate& gate(std::size_t index) const {
        if (index >= gates_.size()) {
            throw std::out_of_range(
                "Gate index " + std::to_string(index) +
                " out of range [0, " + std::to_string(gates_.size()) + ")");
        }
        return gates_[index];
    }

    /**
     * @brief Returns a mutable reference to the gate at the specified index.
     * @param index Gate index
     * @return Mutable reference to the gate
     * @throws std::out_of_range if index >= numGates()
     */
    [[nodiscard]] Gate& gate(std::size_t index) {
        if (index >= gates_.size()) {
            throw std::out_of_range(
                "Gate index " + std::to_string(index) +
                " out of range [0, " + std::to_string(gates_.size()) + ")");
        }
        return gates_[index];
    }

    /**
     * @brief Returns all gates in the circuit.
     * @return Const reference to the gate vector
     */
    [[nodiscard]] const std::vector<Gate>& gates() const noexcept {
        return gates_;
    }

    /**
     * @brief Removes all gates from the circuit.
     */
    void clear() noexcept {
        gates_.clear();
        next_gate_id_ = 0;
    }

    // -------------------------------------------------------------------------
    // Circuit Properties
    // -------------------------------------------------------------------------

    /// @brief Returns the number of qubits in the circuit.
    [[nodiscard]] std::size_t numQubits() const noexcept { return num_qubits_; }

    /// @brief Returns the number of gates in the circuit.
    [[nodiscard]] std::size_t numGates() const noexcept { return gates_.size(); }

    /// @brief Returns true if the circuit has no gates.
    [[nodiscard]] bool empty() const noexcept { return gates_.empty(); }

    /**
     * @brief Calculates the circuit depth.
     *
     * Depth is the maximum number of gates on any single qubit path,
     * representing the critical path length.
     *
     * @return Circuit depth (0 for empty circuits)
     */
    [[nodiscard]] std::size_t depth() const noexcept {
        if (gates_.empty()) {
            return 0;
        }

        // Track depth at each qubit
        std::vector<std::size_t> qubit_depths(num_qubits_, 0);

        for (const auto& g : gates_) {
            // Find max depth among qubits this gate touches
            std::size_t max_depth = 0;
            for (auto q : g.qubits()) {
                max_depth = std::max(max_depth, qubit_depths[q]);
            }

            // Update all touched qubits to new depth
            for (auto q : g.qubits()) {
                qubit_depths[q] = max_depth + 1;
            }
        }

        return *std::max_element(qubit_depths.begin(), qubit_depths.end());
    }

    /**
     * @brief Counts gates of a specific type.
     * @param type The gate type to count
     * @return Number of gates of the specified type
     */
    [[nodiscard]] std::size_t countGates(GateType type) const noexcept {
        return static_cast<std::size_t>(
            std::count_if(gates_.begin(), gates_.end(),
                          [type](const Gate& g) { return g.type() == type; }));
    }

    /**
     * @brief Counts two-qubit gates in the circuit.
     * @return Number of two-qubit gates
     */
    [[nodiscard]] std::size_t countTwoQubitGates() const noexcept {
        return static_cast<std::size_t>(
            std::count_if(gates_.begin(), gates_.end(),
                          [](const Gate& g) { return g.numQubits() == 2; }));
    }

    // -------------------------------------------------------------------------
    // Iteration
    // -------------------------------------------------------------------------

    [[nodiscard]] iterator begin() noexcept { return gates_.begin(); }
    [[nodiscard]] iterator end() noexcept { return gates_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return gates_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return gates_.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return gates_.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return gates_.cend(); }

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /**
     * @brief Creates a deep copy of the circuit.
     * @return New circuit with copied gates
     */
    [[nodiscard]] Circuit clone() const {
        Circuit copy(num_qubits_);
        copy.gates_ = gates_;
        copy.next_gate_id_ = next_gate_id_;
        return copy;
    }

    /**
     * @brief Returns a string representation of the circuit.
     * @return Multi-line string showing circuit structure
     */
    [[nodiscard]] std::string toString() const {
        std::string result = "Circuit(" + std::to_string(num_qubits_) +
                             " qubits, " + std::to_string(gates_.size()) +
                             " gates, depth " + std::to_string(depth()) + "):\n";
        for (const auto& g : gates_) {
            result += "  " + g.toString() + "\n";
        }
        return result;
    }

private:
    std::size_t num_qubits_;
    std::vector<Gate> gates_;
    GateId next_gate_id_;

    /**
     * @brief Validates that a gate's qubits are within circuit bounds.
     * @param g The gate to validate
     * @throws std::out_of_range if any qubit index is invalid
     */
    void validateGateQubits(const Gate& g) const {
        for (auto q : g.qubits()) {
            if (q >= num_qubits_) {
                throw std::out_of_range(
                    "Gate " + std::string(gateTypeName(g.type())) +
                    " references qubit " + std::to_string(q) +
                    " but circuit only has " + std::to_string(num_qubits_) +
                    " qubits");
            }
        }
    }
};

/**
 * @brief Stream output operator for Circuit.
 * @param os Output stream
 * @param circuit The circuit to output
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const Circuit& circuit) {
    os << circuit.toString();
    return os;
}

}  // namespace qopt::ir
