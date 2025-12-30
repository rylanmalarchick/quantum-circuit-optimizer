// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Gate.hpp
 * @brief Quantum gate representation and factory methods
 *
 * Provides the Gate class representing single-qubit and multi-qubit quantum
 * gates, along with factory methods for common gates and utility functions
 * for gate properties.
 *
 * @see Circuit.hpp for circuit-level operations
 * @see Types.hpp for common type definitions
 */

#pragma once

#include "Types.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace qopt::ir {

/**
 * @brief Enumeration of supported quantum gate types.
 *
 * Single-qubit gates: H, X, Y, Z, S, Sdg, T, Tdg, Rx, Ry, Rz
 * Two-qubit gates: CNOT, CZ, SWAP
 */
enum class GateType {
    // Single-qubit Clifford gates
    H,      ///< Hadamard gate
    X,      ///< Pauli-X (NOT) gate
    Y,      ///< Pauli-Y gate
    Z,      ///< Pauli-Z gate
    S,      ///< S gate (sqrt(Z))
    Sdg,    ///< S-dagger gate
    T,      ///< T gate (sqrt(S))
    Tdg,    ///< T-dagger gate

    // Single-qubit rotation gates (parameterized)
    Rx,     ///< Rotation around X-axis
    Ry,     ///< Rotation around Y-axis
    Rz,     ///< Rotation around Z-axis

    // Two-qubit gates
    CNOT,   ///< Controlled-NOT (CX) gate
    CZ,     ///< Controlled-Z gate
    SWAP    ///< SWAP gate
};

/**
 * @brief Returns the name of a gate type as a string.
 * @param type The gate type
 * @return String representation of the gate type
 */
[[nodiscard]] constexpr std::string_view gateTypeName(GateType type) noexcept {
    switch (type) {
        case GateType::H:    return "H";
        case GateType::X:    return "X";
        case GateType::Y:    return "Y";
        case GateType::Z:    return "Z";
        case GateType::S:    return "S";
        case GateType::Sdg:  return "Sdg";
        case GateType::T:    return "T";
        case GateType::Tdg:  return "Tdg";
        case GateType::Rx:   return "Rx";
        case GateType::Ry:   return "Ry";
        case GateType::Rz:   return "Rz";
        case GateType::CNOT: return "CNOT";
        case GateType::CZ:   return "CZ";
        case GateType::SWAP: return "SWAP";
    }
    return "Unknown";
}

/**
 * @brief Returns the number of qubits a gate type acts on.
 * @param type The gate type
 * @return Number of qubits (1 or 2)
 */
[[nodiscard]] constexpr std::size_t numQubitsFor(GateType type) noexcept {
    switch (type) {
        case GateType::CNOT:
        case GateType::CZ:
        case GateType::SWAP:
            return 2;
        default:
            return 1;
    }
}

/**
 * @brief Returns whether a gate type is parameterized.
 * @param type The gate type
 * @return true if the gate requires a rotation angle parameter
 */
[[nodiscard]] constexpr bool isParameterized(GateType type) noexcept {
    switch (type) {
        case GateType::Rx:
        case GateType::Ry:
        case GateType::Rz:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Returns whether a gate type is Hermitian (self-inverse).
 * @param type The gate type
 * @return true if the gate equals its own inverse
 */
[[nodiscard]] constexpr bool isHermitian(GateType type) noexcept {
    switch (type) {
        case GateType::H:
        case GateType::X:
        case GateType::Y:
        case GateType::Z:
        case GateType::CNOT:
        case GateType::CZ:
        case GateType::SWAP:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Represents a quantum gate operation.
 *
 * A Gate consists of a type, target qubit(s), optional rotation parameter,
 * and a unique identifier. Gates are value types and can be copied/moved.
 *
 * Example:
 * @code
 * auto h = Gate::h(0);           // Hadamard on qubit 0
 * auto cx = Gate::cnot(0, 1);    // CNOT with control=0, target=1
 * auto rz = Gate::rz(0, PI/4);   // Rz(π/4) on qubit 0
 * @endcode
 */
class Gate {
public:
    /**
     * @brief Constructs a gate with the given properties.
     * @param type The gate type
     * @param qubits Target qubit indices
     * @param parameter Optional rotation angle (for Rx, Ry, Rz)
     * @param id Unique gate identifier (default: INVALID_GATE_ID)
     * @throws std::invalid_argument if qubit count doesn't match gate type
     */
    Gate(GateType type,
         std::vector<QubitIndex> qubits,
         std::optional<Angle> parameter = std::nullopt,
         GateId id = INVALID_GATE_ID)
        : type_(type)
        , qubits_(std::move(qubits))
        , parameter_(parameter)
        , id_(id)
    {
        validate();
    }

    // Default special members
    ~Gate() noexcept = default;
    Gate(const Gate&) = default;
    Gate& operator=(const Gate&) = default;
    Gate(Gate&&) noexcept = default;
    Gate& operator=(Gate&&) noexcept = default;

    // -------------------------------------------------------------------------
    // Factory Methods
    // -------------------------------------------------------------------------

    /// @brief Creates a Hadamard gate on the specified qubit.
    [[nodiscard]] static Gate h(QubitIndex qubit) {
        return Gate(GateType::H, {qubit});
    }

    /// @brief Creates a Pauli-X gate on the specified qubit.
    [[nodiscard]] static Gate x(QubitIndex qubit) {
        return Gate(GateType::X, {qubit});
    }

    /// @brief Creates a Pauli-Y gate on the specified qubit.
    [[nodiscard]] static Gate y(QubitIndex qubit) {
        return Gate(GateType::Y, {qubit});
    }

    /// @brief Creates a Pauli-Z gate on the specified qubit.
    [[nodiscard]] static Gate z(QubitIndex qubit) {
        return Gate(GateType::Z, {qubit});
    }

    /// @brief Creates an S gate on the specified qubit.
    [[nodiscard]] static Gate s(QubitIndex qubit) {
        return Gate(GateType::S, {qubit});
    }

    /// @brief Creates an S-dagger gate on the specified qubit.
    [[nodiscard]] static Gate sdg(QubitIndex qubit) {
        return Gate(GateType::Sdg, {qubit});
    }

    /// @brief Creates a T gate on the specified qubit.
    [[nodiscard]] static Gate t(QubitIndex qubit) {
        return Gate(GateType::T, {qubit});
    }

    /// @brief Creates a T-dagger gate on the specified qubit.
    [[nodiscard]] static Gate tdg(QubitIndex qubit) {
        return Gate(GateType::Tdg, {qubit});
    }

    /// @brief Creates an Rx rotation gate.
    [[nodiscard]] static Gate rx(QubitIndex qubit, Angle angle) {
        return Gate(GateType::Rx, {qubit}, angle);
    }

    /// @brief Creates an Ry rotation gate.
    [[nodiscard]] static Gate ry(QubitIndex qubit, Angle angle) {
        return Gate(GateType::Ry, {qubit}, angle);
    }

    /// @brief Creates an Rz rotation gate.
    [[nodiscard]] static Gate rz(QubitIndex qubit, Angle angle) {
        return Gate(GateType::Rz, {qubit}, angle);
    }

    /// @brief Creates a CNOT gate with specified control and target qubits.
    /// @throws std::invalid_argument if control == target
    [[nodiscard]] static Gate cnot(QubitIndex control, QubitIndex target) {
        if (control == target) {
            throw std::invalid_argument(
                "CNOT control and target must be different qubits");
        }
        return Gate(GateType::CNOT, {control, target});
    }

    /// @brief Creates a CZ gate with specified control and target qubits.
    /// @throws std::invalid_argument if control == target
    [[nodiscard]] static Gate cz(QubitIndex control, QubitIndex target) {
        if (control == target) {
            throw std::invalid_argument(
                "CZ control and target must be different qubits");
        }
        return Gate(GateType::CZ, {control, target});
    }

    /// @brief Creates a SWAP gate between two qubits.
    /// @throws std::invalid_argument if qubit1 == qubit2
    [[nodiscard]] static Gate swap(QubitIndex qubit1, QubitIndex qubit2) {
        if (qubit1 == qubit2) {
            throw std::invalid_argument(
                "SWAP requires two different qubits");
        }
        return Gate(GateType::SWAP, {qubit1, qubit2});
    }

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /// @brief Returns the gate type.
    [[nodiscard]] GateType type() const noexcept { return type_; }

    /// @brief Returns the target qubit indices.
    [[nodiscard]] const std::vector<QubitIndex>& qubits() const noexcept {
        return qubits_;
    }

    /// @brief Returns the rotation parameter if present.
    [[nodiscard]] std::optional<Angle> parameter() const noexcept {
        return parameter_;
    }

    /// @brief Returns the unique gate identifier.
    [[nodiscard]] GateId id() const noexcept { return id_; }

    /// @brief Sets the gate identifier.
    void setId(GateId id) noexcept { id_ = id; }

    /// @brief Returns the number of qubits this gate acts on.
    [[nodiscard]] std::size_t numQubits() const noexcept {
        return qubits_.size();
    }

    /// @brief Returns whether this gate is parameterized.
    [[nodiscard]] bool isParameterized() const noexcept {
        return parameter_.has_value();
    }

    /// @brief Returns the maximum qubit index referenced by this gate.
    [[nodiscard]] QubitIndex maxQubit() const noexcept {
        QubitIndex max = 0;
        for (auto q : qubits_) {
            if (q > max) max = q;
        }
        return max;
    }

    // -------------------------------------------------------------------------
    // Operators
    // -------------------------------------------------------------------------

    /// @brief Equality comparison (ignores id).
    [[nodiscard]] bool operator==(const Gate& other) const noexcept {
        return type_ == other.type_ &&
               qubits_ == other.qubits_ &&
               parameter_ == other.parameter_;
    }

    /// @brief Inequality comparison.
    [[nodiscard]] bool operator!=(const Gate& other) const noexcept {
        return !(*this == other);
    }

    /// @brief Returns a string representation of the gate.
    [[nodiscard]] std::string toString() const {
        std::string result{gateTypeName(type_)};
        if (parameter_.has_value()) {
            result += "(" + std::to_string(parameter_.value()) + ")";
        }
        result += " ";
        for (std::size_t i = 0; i < qubits_.size(); ++i) {
            if (i > 0) result += ", ";
            result += "q[" + std::to_string(qubits_[i]) + "]";
        }
        return result;
    }

private:
    GateType type_;
    std::vector<QubitIndex> qubits_;
    std::optional<Angle> parameter_;
    GateId id_;

    /// @brief Validates gate construction parameters.
    void validate() const {
        const std::size_t expected = numQubitsFor(type_);
        if (qubits_.size() != expected) {
            throw std::invalid_argument(
                "Gate " + std::string(gateTypeName(type_)) +
                " requires " + std::to_string(expected) +
                " qubit(s), got " + std::to_string(qubits_.size()));
        }

        if (ir::isParameterized(type_) && !parameter_.has_value()) {
            throw std::invalid_argument(
                "Gate " + std::string(gateTypeName(type_)) +
                " requires a rotation parameter");
        }
    }
};

}  // namespace qopt::ir
