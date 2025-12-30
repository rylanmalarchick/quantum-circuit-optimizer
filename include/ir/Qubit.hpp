// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Qubit.hpp
 * @brief Qubit type definitions and utilities
 *
 * Provides type aliases and utilities for working with qubit indices.
 * Currently a thin wrapper; may be expanded in future for qubit metadata.
 *
 * @see Types.hpp for QubitIndex definition
 * @see Circuit.hpp for qubit register management
 */

#pragma once

#include "Types.hpp"

namespace qopt::ir {

/**
 * @brief Type alias for qubit indices within a circuit.
 *
 * Qubits are identified by their index in the circuit's qubit register.
 * Indices are zero-based and contiguous.
 */
using QubitId = QubitIndex;

/**
 * @brief Validates that a qubit index is within bounds.
 * @param qubit The qubit index to validate
 * @param num_qubits Total number of qubits in the circuit
 * @return true if the qubit index is valid
 */
[[nodiscard]] constexpr bool isValidQubit(QubitIndex qubit,
                                           std::size_t num_qubits) noexcept {
    return qubit < num_qubits;
}

}  // namespace qopt::ir
