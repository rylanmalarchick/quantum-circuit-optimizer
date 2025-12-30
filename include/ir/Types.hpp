// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Types.hpp
 * @brief Common type aliases and constants for the quantum circuit optimizer
 *
 * Provides foundational types used throughout the qopt library including
 * qubit indices, gate identifiers, and physical constants.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace qopt {

/// @brief Type alias for qubit indices
using QubitIndex = std::size_t;

/// @brief Type alias for unique gate identifiers
using GateId = std::size_t;

/// @brief Type alias for rotation angles (in radians)
using Angle = double;

/// @brief Sentinel value for invalid/unassigned gate IDs
inline constexpr GateId INVALID_GATE_ID = std::numeric_limits<GateId>::max();

/// @brief Sentinel value for invalid qubit indices
inline constexpr QubitIndex INVALID_QUBIT = std::numeric_limits<QubitIndex>::max();

namespace constants {

/// @brief Maximum number of qubits supported (practical limit for simulation)
inline constexpr std::size_t MAX_QUBITS = 30;

/// @brief Default tolerance for floating-point comparisons
inline constexpr double TOLERANCE = 1e-10;

/// @brief Pi constant for rotation gates
inline constexpr double PI = 3.14159265358979323846;

/// @brief Pi/2 for common rotations
inline constexpr double PI_2 = PI / 2.0;

/// @brief Pi/4 for T gate
inline constexpr double PI_4 = PI / 4.0;

}  // namespace constants

}  // namespace qopt
