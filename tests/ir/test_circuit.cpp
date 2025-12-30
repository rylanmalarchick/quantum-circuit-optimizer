// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_circuit.cpp
 * @brief Unit tests for the Circuit class
 */

#include "ir/Circuit.hpp"

#include <gtest/gtest.h>

namespace qopt::ir {
namespace {

// =============================================================================
// Construction Tests
// =============================================================================

TEST(CircuitConstructionTest, ConstructsWithValidQubitCount) {
    Circuit c(5);
    EXPECT_EQ(c.numQubits(), 5);
    EXPECT_EQ(c.numGates(), 0);
    EXPECT_TRUE(c.empty());
}

TEST(CircuitConstructionTest, ThrowsOnZeroQubits) {
    EXPECT_THROW(Circuit(0), std::invalid_argument);
}

TEST(CircuitConstructionTest, ThrowsOnExcessiveQubits) {
    EXPECT_THROW(Circuit(constants::MAX_QUBITS + 1), std::invalid_argument);
}

TEST(CircuitConstructionTest, AcceptsMaxQubits) {
    Circuit c(constants::MAX_QUBITS);
    EXPECT_EQ(c.numQubits(), constants::MAX_QUBITS);
}

// =============================================================================
// Gate Management Tests
// =============================================================================

TEST(CircuitGateTest, AddGateIncreasesCount) {
    Circuit c(2);
    EXPECT_EQ(c.numGates(), 0);

    c.addGate(Gate::h(0));
    EXPECT_EQ(c.numGates(), 1);

    c.addGate(Gate::cnot(0, 1));
    EXPECT_EQ(c.numGates(), 2);
}

TEST(CircuitGateTest, AddGateAssignsSequentialIds) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::x(1));
    c.addGate(Gate::cnot(0, 1));

    EXPECT_EQ(c.gate(0).id(), 0);
    EXPECT_EQ(c.gate(1).id(), 1);
    EXPECT_EQ(c.gate(2).id(), 2);
}

TEST(CircuitGateTest, GateAccessorReturnsCorrectGate) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::x(1));

    EXPECT_EQ(c.gate(0).type(), GateType::H);
    EXPECT_EQ(c.gate(1).type(), GateType::X);
}

TEST(CircuitGateTest, GateAccessorThrowsOnOutOfRange) {
    Circuit c(2);
    c.addGate(Gate::h(0));

    EXPECT_THROW((void)c.gate(1), std::out_of_range);
    EXPECT_THROW((void)c.gate(100), std::out_of_range);
}

TEST(CircuitGateTest, AddGateThrowsOnInvalidQubit) {
    Circuit c(2);  // qubits 0 and 1 only

    EXPECT_THROW(c.addGate(Gate::h(2)), std::out_of_range);
    EXPECT_THROW(c.addGate(Gate::cnot(0, 5)), std::out_of_range);
}

TEST(CircuitGateTest, GatesAccessorReturnsAllGates) {
    Circuit c(3);
    c.addGate(Gate::h(0));
    c.addGate(Gate::h(1));
    c.addGate(Gate::h(2));

    const auto& gates = c.gates();
    EXPECT_EQ(gates.size(), 3);
}

TEST(CircuitGateTest, ClearRemovesAllGates) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    EXPECT_EQ(c.numGates(), 2);

    c.clear();
    EXPECT_EQ(c.numGates(), 0);
    EXPECT_TRUE(c.empty());
}

// =============================================================================
// Depth Calculation Tests
// =============================================================================

TEST(CircuitDepthTest, EmptyCircuitHasDepthZero) {
    Circuit c(2);
    EXPECT_EQ(c.depth(), 0);
}

TEST(CircuitDepthTest, SingleGateHasDepthOne) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    EXPECT_EQ(c.depth(), 1);
}

TEST(CircuitDepthTest, ParallelGatesHaveDepthOne) {
    Circuit c(3);
    c.addGate(Gate::h(0));
    c.addGate(Gate::h(1));
    c.addGate(Gate::h(2));
    // All H gates are on different qubits, depth = 1
    EXPECT_EQ(c.depth(), 1);
}

TEST(CircuitDepthTest, SequentialGatesOnSameQubit) {
    Circuit c(1);
    c.addGate(Gate::h(0));
    c.addGate(Gate::x(0));
    c.addGate(Gate::z(0));
    // All on qubit 0, depth = 3
    EXPECT_EQ(c.depth(), 3);
}

TEST(CircuitDepthTest, BellCircuitDepth) {
    Circuit c(2);
    c.addGate(Gate::h(0));        // depth 1
    c.addGate(Gate::cnot(0, 1));  // depth 2 (both qubits)
    EXPECT_EQ(c.depth(), 2);
}

TEST(CircuitDepthTest, GHZCircuitDepth) {
    Circuit c(3);
    c.addGate(Gate::h(0));        // depth 1
    c.addGate(Gate::cnot(0, 1));  // depth 2
    c.addGate(Gate::cnot(1, 2));  // depth 3
    EXPECT_EQ(c.depth(), 3);
}

TEST(CircuitDepthTest, ComplexCircuitDepth) {
    Circuit c(3);
    // Layer 1: H on all qubits (parallel)
    c.addGate(Gate::h(0));
    c.addGate(Gate::h(1));
    c.addGate(Gate::h(2));
    // Layer 2: CNOT 0-1
    c.addGate(Gate::cnot(0, 1));
    // Layer 3: CNOT 1-2
    c.addGate(Gate::cnot(1, 2));
    // Layer 4: X on qubit 0 (parallel with layer 3)
    c.addGate(Gate::x(0));

    // Qubit 0: H(1), CNOT(2), X(3) = 3
    // Qubit 1: H(1), CNOT(2), CNOT(3) = 3
    // Qubit 2: H(1), CNOT(3) = 3
    // But X on q0 happens after CNOT(0,1), so depth analysis:
    // Actually: H's at depth 1, CNOT(0,1) at depth 2 (needs both q0, q1 done)
    // CNOT(1,2) at depth 3 (needs q1 from CNOT done, q2 from H done)
    // X(0) at depth 3 (needs q0 from CNOT done)
    EXPECT_EQ(c.depth(), 3);
}

// =============================================================================
// Gate Counting Tests
// =============================================================================

TEST(CircuitCountTest, CountGatesOfType) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::h(1));
    c.addGate(Gate::cnot(0, 1));
    c.addGate(Gate::x(0));

    EXPECT_EQ(c.countGates(GateType::H), 2);
    EXPECT_EQ(c.countGates(GateType::CNOT), 1);
    EXPECT_EQ(c.countGates(GateType::X), 1);
    EXPECT_EQ(c.countGates(GateType::Z), 0);
}

TEST(CircuitCountTest, CountTwoQubitGates) {
    Circuit c(3);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    c.addGate(Gate::cz(1, 2));
    c.addGate(Gate::swap(0, 2));
    c.addGate(Gate::x(1));

    EXPECT_EQ(c.countTwoQubitGates(), 3);
}

// =============================================================================
// Iteration Tests
// =============================================================================

TEST(CircuitIterationTest, RangeForWorks) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));

    std::size_t count = 0;
    for (const auto& gate : c) {
        (void)gate;  // suppress unused warning
        ++count;
    }
    EXPECT_EQ(count, 2);
}

TEST(CircuitIterationTest, IteratorOrderMatchesAddOrder) {
    Circuit c(3);
    c.addGate(Gate::h(0));
    c.addGate(Gate::x(1));
    c.addGate(Gate::z(2));

    auto it = c.begin();
    EXPECT_EQ(it->type(), GateType::H);
    ++it;
    EXPECT_EQ(it->type(), GateType::X);
    ++it;
    EXPECT_EQ(it->type(), GateType::Z);
    ++it;
    EXPECT_EQ(it, c.end());
}

// =============================================================================
// Clone Tests
// =============================================================================

TEST(CircuitCloneTest, CloneCreatesIndependentCopy) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));

    Circuit copy = c.clone();

    EXPECT_EQ(copy.numQubits(), c.numQubits());
    EXPECT_EQ(copy.numGates(), c.numGates());

    // Modify original, copy should be unchanged
    c.addGate(Gate::x(0));
    EXPECT_EQ(c.numGates(), 3);
    EXPECT_EQ(copy.numGates(), 2);
}

// =============================================================================
// ToString Tests
// =============================================================================

TEST(CircuitToStringTest, FormatsCorrectly) {
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));

    std::string str = c.toString();
    EXPECT_TRUE(str.find("2 qubits") != std::string::npos);
    EXPECT_TRUE(str.find("2 gates") != std::string::npos);
    EXPECT_TRUE(str.find("H q[0]") != std::string::npos);
    EXPECT_TRUE(str.find("CNOT") != std::string::npos);
}

}  // namespace
}  // namespace qopt::ir
