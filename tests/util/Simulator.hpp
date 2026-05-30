// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Simulator.hpp
 * @brief Tiny dense statevector simulator for test-time semantic verification.
 *
 * Builds the full unitary of a Circuit (small qubit counts only) and compares
 * two circuits for equivalence up to global phase. This is the oracle that
 * pins what every optimization pass must preserve: the unitary, not the gate
 * count. Intended for use from test executables, not the library.
 */

#pragma once

#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"
#include "ir/Types.hpp"

#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace qopt::test {

using Complex = std::complex<double>;
using StateVector = std::vector<Complex>;
using Matrix = std::vector<StateVector>;  // row-major, square

/// @brief Applies a single-qubit 2x2 matrix to one qubit of a statevector.
inline void applySingle(StateVector& sv, std::size_t qubit, const Complex m[2][2]) {
    const std::size_t bit = std::size_t{1} << qubit;
    for (std::size_t i = 0; i < sv.size(); ++i) {
        if ((i & bit) == 0) {
            const std::size_t j = i | bit;
            const Complex a = sv[i];
            const Complex b = sv[j];
            sv[i] = m[0][0] * a + m[0][1] * b;
            sv[j] = m[1][0] * a + m[1][1] * b;
        }
    }
}

/// @brief Applies a gate to a statevector using textbook gate matrices.
inline void applyGate(StateVector& sv, const ir::Gate& g) {
    using ir::GateType;
    const auto& q = g.qubits();
    const double theta = g.parameter().value_or(0.0);
    const double c = std::cos(theta / 2.0);
    const double s = std::sin(theta / 2.0);
    const Complex I(0.0, 1.0);
    const double r2 = 1.0 / std::sqrt(2.0);

    switch (g.type()) {
        case GateType::H:   { Complex m[2][2] = {{r2, r2}, {r2, -r2}}; applySingle(sv, q[0], m); return; }
        case GateType::X:   { Complex m[2][2] = {{0, 1}, {1, 0}};       applySingle(sv, q[0], m); return; }
        case GateType::Y:   { Complex m[2][2] = {{0, -I}, {I, 0}};      applySingle(sv, q[0], m); return; }
        case GateType::Z:   { Complex m[2][2] = {{1, 0}, {0, -1}};      applySingle(sv, q[0], m); return; }
        case GateType::S:   { Complex m[2][2] = {{1, 0}, {0, I}};       applySingle(sv, q[0], m); return; }
        case GateType::Sdg: { Complex m[2][2] = {{1, 0}, {0, -I}};      applySingle(sv, q[0], m); return; }
        case GateType::T:   { Complex m[2][2] = {{1, 0}, {0, std::exp(I * (constants::PI / 4.0))}}; applySingle(sv, q[0], m); return; }
        case GateType::Tdg: { Complex m[2][2] = {{1, 0}, {0, std::exp(-I * (constants::PI / 4.0))}}; applySingle(sv, q[0], m); return; }
        case GateType::Rx:  { Complex m[2][2] = {{c, -I * s}, {-I * s, c}}; applySingle(sv, q[0], m); return; }
        case GateType::Ry:  { Complex m[2][2] = {{c, -s}, {s, c}};          applySingle(sv, q[0], m); return; }
        case GateType::Rz:  { Complex m[2][2] = {{std::exp(-I * (theta / 2.0)), 0}, {0, std::exp(I * (theta / 2.0))}}; applySingle(sv, q[0], m); return; }
        case GateType::CNOT: {
            const std::size_t cb = std::size_t{1} << q[0];
            const std::size_t tb = std::size_t{1} << q[1];
            for (std::size_t i = 0; i < sv.size(); ++i)
                if ((i & cb) && ((i & tb) == 0)) std::swap(sv[i], sv[i | tb]);
            return;
        }
        case GateType::CZ: {
            const std::size_t cb = std::size_t{1} << q[0];
            const std::size_t tb = std::size_t{1} << q[1];
            for (std::size_t i = 0; i < sv.size(); ++i)
                if ((i & cb) && (i & tb)) sv[i] = -sv[i];
            return;
        }
        case GateType::SWAP: {
            const std::size_t ab = std::size_t{1} << q[0];
            const std::size_t bb = std::size_t{1} << q[1];
            for (std::size_t i = 0; i < sv.size(); ++i) {
                const bool a = (i & ab) != 0;
                const bool b = (i & bb) != 0;
                if (a && !b) std::swap(sv[i], sv[(i & ~ab) | bb]);
            }
            return;
        }
    }
}

/// @brief Builds the 2^n x 2^n unitary of a circuit (column j = circuit applied to |j>).
inline Matrix buildUnitary(const ir::Circuit& circuit) {
    const std::size_t n = circuit.numQubits();
    const std::size_t dim = std::size_t{1} << n;
    Matrix u(dim, StateVector(dim, Complex(0.0, 0.0)));
    for (std::size_t col = 0; col < dim; ++col) {
        StateVector sv(dim, Complex(0.0, 0.0));
        sv[col] = 1.0;
        for (const auto& g : circuit) applyGate(sv, g);
        for (std::size_t row = 0; row < dim; ++row) u[row][col] = sv[row];
    }
    return u;
}

/// @brief Largest entrywise deviation between a and b after removing global phase.
inline double unitaryDiffUpToPhase(const Matrix& a, const Matrix& b) {
    const std::size_t dim = a.size();
    // Find the largest-magnitude entry of a to anchor the global phase.
    double best = 0.0;
    std::size_t br = 0, bc = 0;
    for (std::size_t i = 0; i < dim; ++i)
        for (std::size_t j = 0; j < dim; ++j)
            if (std::abs(a[i][j]) > best) { best = std::abs(a[i][j]); br = i; bc = j; }
    if (best < 1e-12) return 0.0;  // a is ~zero; degenerate
    const Complex phase = b[br][bc] / a[br][bc];
    double maxdiff = std::abs(std::abs(phase) - 1.0);
    for (std::size_t i = 0; i < dim; ++i)
        for (std::size_t j = 0; j < dim; ++j)
            maxdiff = std::max(maxdiff, std::abs(b[i][j] - phase * a[i][j]));
    return maxdiff;
}

/// @brief True if two circuits implement the same unitary up to global phase.
inline bool circuitsEquivalent(const ir::Circuit& a, const ir::Circuit& b, double tol = 1e-9) {
    if (a.numQubits() != b.numQubits()) return false;
    return unitaryDiffUpToPhase(buildUnitary(a), buildUnitary(b)) < tol;
}

/**
 * @brief Builds the logical unitary realised by a routed circuit.
 *
 * Places logical qubit q's input bit at physical initial_mapping[q], applies
 * the routed (physical) circuit including its SWAPs, and reads logical qubit q
 * back from physical final_mapping[q]. Unused physical qubits start and (for a
 * correct routing) end in |0>.
 */
inline Matrix buildRoutedLogicalUnitary(
        const ir::Circuit& routed, std::size_t n_logical,
        const std::vector<std::size_t>& initial_mapping,
        const std::vector<std::size_t>& final_mapping) {
    const std::size_t m = routed.numQubits();
    const std::size_t mdim = std::size_t{1} << m;
    const std::size_t ndim = std::size_t{1} << n_logical;
    Matrix u(ndim, StateVector(ndim, Complex(0.0, 0.0)));
    for (std::size_t x = 0; x < ndim; ++x) {
        StateVector sv(mdim, Complex(0.0, 0.0));
        std::size_t in_idx = 0;
        for (std::size_t q = 0; q < n_logical; ++q)
            if ((x >> q) & 1u) in_idx |= (std::size_t{1} << initial_mapping[q]);
        sv[in_idx] = 1.0;
        for (const auto& g : routed) applyGate(sv, g);
        for (std::size_t y = 0; y < ndim; ++y) {
            std::size_t out_idx = 0;
            for (std::size_t q = 0; q < n_logical; ++q)
                if ((y >> q) & 1u) out_idx |= (std::size_t{1} << final_mapping[q]);
            u[y][x] = sv[out_idx];
        }
    }
    return u;
}

/// @brief True if a routed circuit implements the original's unitary (up to phase).
inline bool routedPreservesUnitary(
        const ir::Circuit& original, const ir::Circuit& routed,
        const std::vector<std::size_t>& initial_mapping,
        const std::vector<std::size_t>& final_mapping, double tol = 1e-9) {
    const Matrix a = buildUnitary(original);
    const Matrix b = buildRoutedLogicalUnitary(
        routed, original.numQubits(), initial_mapping, final_mapping);
    return unitaryDiffUpToPhase(a, b) < tol;
}

}  // namespace qopt::test
