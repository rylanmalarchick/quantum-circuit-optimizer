// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_gate.cpp
 * @brief Unit tests for the Gate class
 */

#include "ir/Gate.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace qopt::ir {
namespace {

// =============================================================================
// Factory Method Tests
// =============================================================================

TEST(GateFactoryTest, HadamardCreatesCorrectGate) {
    auto h = Gate::h(0);
    EXPECT_EQ(h.type(), GateType::H);
    EXPECT_EQ(h.numQubits(), 1);
    EXPECT_EQ(h.qubits()[0], 0);
    EXPECT_FALSE(h.isParameterized());
}

TEST(GateFactoryTest, PauliGatesCreateCorrectly) {
    auto x = Gate::x(1);
    auto y = Gate::y(2);
    auto z = Gate::z(3);

    EXPECT_EQ(x.type(), GateType::X);
    EXPECT_EQ(x.qubits()[0], 1);

    EXPECT_EQ(y.type(), GateType::Y);
    EXPECT_EQ(y.qubits()[0], 2);

    EXPECT_EQ(z.type(), GateType::Z);
    EXPECT_EQ(z.qubits()[0], 3);
}

TEST(GateFactoryTest, CNOTCreatesCorrectGate) {
    auto cx = Gate::cnot(0, 1);
    EXPECT_EQ(cx.type(), GateType::CNOT);
    EXPECT_EQ(cx.numQubits(), 2);
    EXPECT_EQ(cx.qubits()[0], 0);  // control
    EXPECT_EQ(cx.qubits()[1], 1);  // target
}

TEST(GateFactoryTest, CNOTThrowsOnSameQubit) {
    EXPECT_THROW(Gate::cnot(0, 0), std::invalid_argument);
}

TEST(GateFactoryTest, CZThrowsOnSameQubit) {
    EXPECT_THROW(Gate::cz(1, 1), std::invalid_argument);
}

TEST(GateFactoryTest, SWAPThrowsOnSameQubit) {
    EXPECT_THROW(Gate::swap(2, 2), std::invalid_argument);
}

TEST(GateFactoryTest, RzCreatesParameterizedGate) {
    double angle = constants::PI / 4;
    auto rz = Gate::rz(0, angle);

    EXPECT_EQ(rz.type(), GateType::Rz);
    EXPECT_TRUE(rz.isParameterized());
    ASSERT_TRUE(rz.parameter().has_value());
    EXPECT_DOUBLE_EQ(rz.parameter().value(), angle);
}

TEST(GateFactoryTest, AllRotationGatesAcceptAngles) {
    auto rx = Gate::rx(0, 1.0);
    auto ry = Gate::ry(0, 2.0);
    auto rz = Gate::rz(0, 3.0);

    EXPECT_TRUE(rx.isParameterized());
    EXPECT_TRUE(ry.isParameterized());
    EXPECT_TRUE(rz.isParameterized());

    EXPECT_DOUBLE_EQ(rx.parameter().value(), 1.0);
    EXPECT_DOUBLE_EQ(ry.parameter().value(), 2.0);
    EXPECT_DOUBLE_EQ(rz.parameter().value(), 3.0);
}

TEST(GateFactoryTest, SAndTGatesCreateCorrectly) {
    auto s = Gate::s(0);
    auto sdg = Gate::sdg(0);
    auto t = Gate::t(0);
    auto tdg = Gate::tdg(0);

    EXPECT_EQ(s.type(), GateType::S);
    EXPECT_EQ(sdg.type(), GateType::Sdg);
    EXPECT_EQ(t.type(), GateType::T);
    EXPECT_EQ(tdg.type(), GateType::Tdg);
}

// =============================================================================
// Accessor Tests
// =============================================================================

TEST(GateAccessorTest, MaxQubitReturnsCorrectValue) {
    auto h = Gate::h(5);
    EXPECT_EQ(h.maxQubit(), 5);

    auto cx = Gate::cnot(2, 7);
    EXPECT_EQ(cx.maxQubit(), 7);

    auto swap = Gate::swap(10, 3);
    EXPECT_EQ(swap.maxQubit(), 10);
}

TEST(GateAccessorTest, IdManagement) {
    auto h = Gate::h(0);
    EXPECT_EQ(h.id(), INVALID_GATE_ID);

    h.setId(42);
    EXPECT_EQ(h.id(), 42);
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST(GateEqualityTest, SameGatesAreEqual) {
    auto h1 = Gate::h(0);
    auto h2 = Gate::h(0);
    EXPECT_EQ(h1, h2);
}

TEST(GateEqualityTest, DifferentQubitsNotEqual) {
    auto h1 = Gate::h(0);
    auto h2 = Gate::h(1);
    EXPECT_NE(h1, h2);
}

TEST(GateEqualityTest, DifferentTypesNotEqual) {
    auto h = Gate::h(0);
    auto x = Gate::x(0);
    EXPECT_NE(h, x);
}

TEST(GateEqualityTest, DifferentParametersNotEqual) {
    auto rz1 = Gate::rz(0, 1.0);
    auto rz2 = Gate::rz(0, 2.0);
    EXPECT_NE(rz1, rz2);
}

TEST(GateEqualityTest, EqualityIgnoresId) {
    auto h1 = Gate::h(0);
    auto h2 = Gate::h(0);
    h1.setId(1);
    h2.setId(2);
    EXPECT_EQ(h1, h2);  // IDs differ but gates are equal
}

// =============================================================================
// Validation Tests
// =============================================================================

TEST(GateValidationTest, RotationGateRequiresParameter) {
    // Direct construction without parameter should throw
    EXPECT_THROW(
        Gate(GateType::Rz, {0}, std::nullopt),
        std::invalid_argument
    );
}

TEST(GateValidationTest, CNOTRequiresTwoQubits) {
    EXPECT_THROW(
        Gate(GateType::CNOT, {0}),
        std::invalid_argument
    );
}

TEST(GateValidationTest, SingleQubitGateRejectsMultipleQubits) {
    EXPECT_THROW(
        Gate(GateType::H, {0, 1}),
        std::invalid_argument
    );
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST(GateUtilityTest, GateTypeNameReturnsCorrectStrings) {
    EXPECT_EQ(gateTypeName(GateType::H), "H");
    EXPECT_EQ(gateTypeName(GateType::CNOT), "CNOT");
    EXPECT_EQ(gateTypeName(GateType::Rz), "Rz");
    EXPECT_EQ(gateTypeName(GateType::SWAP), "SWAP");
}

TEST(GateUtilityTest, NumQubitsForReturnsCorrectCounts) {
    EXPECT_EQ(numQubitsFor(GateType::H), 1);
    EXPECT_EQ(numQubitsFor(GateType::X), 1);
    EXPECT_EQ(numQubitsFor(GateType::Rz), 1);
    EXPECT_EQ(numQubitsFor(GateType::CNOT), 2);
    EXPECT_EQ(numQubitsFor(GateType::CZ), 2);
    EXPECT_EQ(numQubitsFor(GateType::SWAP), 2);
}

TEST(GateUtilityTest, IsParameterizedCorrect) {
    EXPECT_FALSE(ir::isParameterized(GateType::H));
    EXPECT_FALSE(ir::isParameterized(GateType::CNOT));
    EXPECT_TRUE(ir::isParameterized(GateType::Rx));
    EXPECT_TRUE(ir::isParameterized(GateType::Ry));
    EXPECT_TRUE(ir::isParameterized(GateType::Rz));
}

TEST(GateUtilityTest, IsHermitianCorrect) {
    EXPECT_TRUE(isHermitian(GateType::H));
    EXPECT_TRUE(isHermitian(GateType::X));
    EXPECT_TRUE(isHermitian(GateType::Y));
    EXPECT_TRUE(isHermitian(GateType::Z));
    EXPECT_TRUE(isHermitian(GateType::CNOT));
    EXPECT_TRUE(isHermitian(GateType::SWAP));

    EXPECT_FALSE(isHermitian(GateType::S));
    EXPECT_FALSE(isHermitian(GateType::T));
    EXPECT_FALSE(isHermitian(GateType::Rz));
}

// =============================================================================
// ToString Tests
// =============================================================================

TEST(GateToStringTest, SingleQubitGateFormat) {
    auto h = Gate::h(0);
    EXPECT_EQ(h.toString(), "H q[0]");

    auto x = Gate::x(3);
    EXPECT_EQ(x.toString(), "X q[3]");
}

TEST(GateToStringTest, TwoQubitGateFormat) {
    auto cx = Gate::cnot(0, 1);
    EXPECT_EQ(cx.toString(), "CNOT q[0], q[1]");
}

TEST(GateToStringTest, ParameterizedGateFormat) {
    auto rz = Gate::rz(0, 1.5);
    std::string str = rz.toString();
    EXPECT_TRUE(str.find("Rz(") != std::string::npos);
    EXPECT_TRUE(str.find("q[0]") != std::string::npos);
}

}  // namespace
}  // namespace qopt::ir
