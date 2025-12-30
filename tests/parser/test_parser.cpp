// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_parser.cpp
 * @brief Unit tests for the OpenQASM 3.0 Parser
 * @author Rylan Malarchick
 * @date 2025
 *
 * Tests cover:
 * - Version declaration parsing
 * - Include statement handling
 * - Qubit/bit register declarations
 * - Single-qubit gate applications
 * - Two-qubit gate applications
 * - Parameterized gate applications with arithmetic
 * - Measurement statements
 * - Error handling and recovery
 * - Full program parsing
 */

#include "parser/Parser.hpp"
#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <string>

namespace qopt::parser {
namespace {

// =============================================================================
// Test Fixtures and Helpers
// =============================================================================

class ParserTest : public ::testing::Test {
protected:
    /**
     * @brief Parse source and return the circuit.
     */
    std::unique_ptr<ir::Circuit> parse(std::string_view source) {
        return parseQASM(source);
    }

    /**
     * @brief Parse source and expect it to throw.
     */
    void expectParseError(std::string_view source) {
        EXPECT_THROW({ [[maybe_unused]] auto c = parseQASM(source); }, QASMParseException)
            << "Source: " << source;
    }

    /**
     * @brief Parse source and check the number of gates.
     */
    void expectGateCount(std::string_view source, size_t expected) {
        auto circuit = parse(source);
        ASSERT_NE(circuit, nullptr);
        EXPECT_EQ(circuit->numGates(), expected) << "Source: " << source;
    }

    /**
     * @brief Parse source and check qubit count.
     */
    void expectQubitCount(std::string_view source, size_t expected) {
        auto circuit = parse(source);
        ASSERT_NE(circuit, nullptr);
        EXPECT_EQ(circuit->numQubits(), expected) << "Source: " << source;
    }

    static constexpr double TOLERANCE = 1e-10;
};

// =============================================================================
// Version Declaration Tests
// =============================================================================

TEST_F(ParserTest, VersionDeclaration30) {
    auto circuit = parse("OPENQASM 3.0; qubit q;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, VersionDeclaration3) {
    auto circuit = parse("OPENQASM 3; qubit q;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, MissingVersionDeclaration) {
    expectParseError("qubit q;");
}

TEST_F(ParserTest, MissingVersionNumber) {
    expectParseError("OPENQASM ; qubit q;");
}

TEST_F(ParserTest, MissingVersionSemicolon) {
    expectParseError("OPENQASM 3.0 qubit q;");
}

// =============================================================================
// Include Statement Tests
// =============================================================================

TEST_F(ParserTest, IncludeStdgates) {
    auto circuit = parse("OPENQASM 3.0; include \"stdgates.inc\"; qubit q;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, IncludeOtherFile) {
    // Should parse but generate a warning (which we don't check here)
    auto circuit = parse("OPENQASM 3.0; include \"mylib.qasm\"; qubit q;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, IncludeMissingString) {
    expectParseError("OPENQASM 3.0; include; qubit q;");
}

// =============================================================================
// Qubit Declaration Tests
// =============================================================================

TEST_F(ParserTest, SingleQubit) {
    expectQubitCount("OPENQASM 3.0; qubit q;", 1);
}

TEST_F(ParserTest, QubitArray) {
    expectQubitCount("OPENQASM 3.0; qubit[4] q;", 4);
}

TEST_F(ParserTest, MultipleQubitRegisters) {
    expectQubitCount("OPENQASM 3.0; qubit[2] q; qubit[3] r;", 5);
}

TEST_F(ParserTest, DuplicateQubitRegister) {
    expectParseError("OPENQASM 3.0; qubit[2] q; qubit[2] q;");
}

TEST_F(ParserTest, QubitMissingName) {
    expectParseError("OPENQASM 3.0; qubit[2];");
}

TEST_F(ParserTest, QubitMissingSemicolon) {
    expectParseError("OPENQASM 3.0; qubit q");
}

// =============================================================================
// Bit Declaration Tests
// =============================================================================

TEST_F(ParserTest, SingleBit) {
    auto circuit = parse("OPENQASM 3.0; qubit q; bit c;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, BitArray) {
    auto circuit = parse("OPENQASM 3.0; qubit[2] q; bit[2] c;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, DuplicateBitRegister) {
    expectParseError("OPENQASM 3.0; qubit q; bit c; bit c;");
}

// =============================================================================
// Single-Qubit Gate Tests
// =============================================================================

TEST_F(ParserTest, HadamardGate) {
    auto circuit = parse("OPENQASM 3.0; qubit q; h q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::H);
}

TEST_F(ParserTest, PauliGates) {
    auto circuit = parse("OPENQASM 3.0; qubit q; x q[0]; y q[0]; z q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 3u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::X);
    EXPECT_EQ(circuit->gate(1).type(), ir::GateType::Y);
    EXPECT_EQ(circuit->gate(2).type(), ir::GateType::Z);
}

TEST_F(ParserTest, SGate) {
    auto circuit = parse("OPENQASM 3.0; qubit q; s q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::S);
}

TEST_F(ParserTest, TGate) {
    auto circuit = parse("OPENQASM 3.0; qubit q; t q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::T);
}

TEST_F(ParserTest, SdgGate) {
    auto circuit = parse("OPENQASM 3.0; qubit q; sdg q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::Sdg);
}

TEST_F(ParserTest, TdgGate) {
    auto circuit = parse("OPENQASM 3.0; qubit q; tdg q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::Tdg);
}

TEST_F(ParserTest, GateWithoutIndex) {
    // Gate on single-qubit register without index
    auto circuit = parse("OPENQASM 3.0; qubit q; h q;");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::H);
}

// =============================================================================
// Two-Qubit Gate Tests
// =============================================================================

TEST_F(ParserTest, CNOTGate) {
    auto circuit = parse("OPENQASM 3.0; qubit[2] q; cx q[0], q[1];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::CNOT);
    EXPECT_EQ(circuit->gate(0).qubits()[0], 0u);
    EXPECT_EQ(circuit->gate(0).qubits()[1], 1u);
}

TEST_F(ParserTest, CNOTAlias) {
    auto circuit = parse("OPENQASM 3.0; qubit[2] q; cnot q[0], q[1];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::CNOT);
}

TEST_F(ParserTest, CZGate) {
    auto circuit = parse("OPENQASM 3.0; qubit[2] q; cz q[0], q[1];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::CZ);
}

TEST_F(ParserTest, SWAPGate) {
    auto circuit = parse("OPENQASM 3.0; qubit[2] q; swap q[0], q[1];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::SWAP);
}

TEST_F(ParserTest, TwoQubitGateMissingComma) {
    expectParseError("OPENQASM 3.0; qubit[2] q; cx q[0] q[1];");
}

TEST_F(ParserTest, TwoQubitGateMissingSecondOperand) {
    expectParseError("OPENQASM 3.0; qubit[2] q; cx q[0];");
}

// =============================================================================
// Parameterized Gate Tests
// =============================================================================

TEST_F(ParserTest, RzWithPi) {
    auto circuit = parse("OPENQASM 3.0; qubit q; rz(pi) q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::Rz);
    ASSERT_TRUE(circuit->gate(0).parameter().has_value());
    EXPECT_NEAR(circuit->gate(0).parameter().value(), M_PI, TOLERANCE);
}

TEST_F(ParserTest, RzWithPiOver4) {
    auto circuit = parse("OPENQASM 3.0; qubit q; rz(pi/4) q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    ASSERT_TRUE(circuit->gate(0).parameter().has_value());
    EXPECT_NEAR(circuit->gate(0).parameter().value(), M_PI / 4.0, TOLERANCE);
}

TEST_F(ParserTest, RxWithPiOver2) {
    auto circuit = parse("OPENQASM 3.0; qubit q; rx(pi/2) q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::Rx);
    ASSERT_TRUE(circuit->gate(0).parameter().has_value());
    EXPECT_NEAR(circuit->gate(0).parameter().value(), M_PI / 2.0, TOLERANCE);
}

TEST_F(ParserTest, RyWithNumber) {
    auto circuit = parse("OPENQASM 3.0; qubit q; ry(1.5) q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::Ry);
    ASSERT_TRUE(circuit->gate(0).parameter().has_value());
    EXPECT_NEAR(circuit->gate(0).parameter().value(), 1.5, TOLERANCE);
}

TEST_F(ParserTest, RzWithArithmetic) {
    auto circuit = parse("OPENQASM 3.0; qubit q; rz(2*pi - pi/2) q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    ASSERT_TRUE(circuit->gate(0).parameter().has_value());
    EXPECT_NEAR(circuit->gate(0).parameter().value(), 2.0 * M_PI - M_PI / 2.0, TOLERANCE);
}

TEST_F(ParserTest, RzWithNegative) {
    auto circuit = parse("OPENQASM 3.0; qubit q; rz(-pi/4) q[0];");
    ASSERT_NE(circuit, nullptr);
    ASSERT_EQ(circuit->numGates(), 1u);
    ASSERT_TRUE(circuit->gate(0).parameter().has_value());
    EXPECT_NEAR(circuit->gate(0).parameter().value(), -M_PI / 4.0, TOLERANCE);
}

TEST_F(ParserTest, ParameterizedGateMissingParen) {
    expectParseError("OPENQASM 3.0; qubit q; rz pi q[0];");
}

// =============================================================================
// Measurement Tests
// =============================================================================

TEST_F(ParserTest, MeasurementAssignment) {
    auto circuit = parse("OPENQASM 3.0; qubit q; bit c; c[0] = measure q[0];");
    EXPECT_NE(circuit, nullptr);
    // Measurement doesn't add gates in our model
}

TEST_F(ParserTest, MeasurementWithoutIndex) {
    auto circuit = parse("OPENQASM 3.0; qubit q; bit c; c = measure q;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, StandaloneMeasure) {
    auto circuit = parse("OPENQASM 3.0; qubit q; measure q[0];");
    EXPECT_NE(circuit, nullptr);
    // Should generate a warning (result discarded)
}

// =============================================================================
// Error Recovery Tests
// =============================================================================

TEST_F(ParserTest, ErrorAtToken) {
    try {
        parse("OPENQASM 3.0; @invalid qubit q;");
        FAIL() << "Expected QASMParseException";
    } catch (const QASMParseException& e) {
        EXPECT_GE(e.numErrors(), 1u);
    }
}

// =============================================================================
// Full Program Tests
// =============================================================================

TEST_F(ParserTest, BellState) {
    const char* source = R"(
OPENQASM 3.0;
include "stdgates.inc";

qubit[2] q;
bit[2] c;

h q[0];
cx q[0], q[1];
c = measure q;
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numQubits(), 2u);
    EXPECT_EQ(circuit->numGates(), 2u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::H);
    EXPECT_EQ(circuit->gate(1).type(), ir::GateType::CNOT);
}

TEST_F(ParserTest, GHZState) {
    const char* source = R"(
OPENQASM 3.0;
qubit[3] q;

h q[0];
cx q[0], q[1];
cx q[1], q[2];
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numQubits(), 3u);
    EXPECT_EQ(circuit->numGates(), 3u);
}

TEST_F(ParserTest, RotationSequence) {
    const char* source = R"(
OPENQASM 3.0;
qubit q;

rz(pi/4) q[0];
rx(pi/2) q[0];
rz(pi/4) q[0];
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numGates(), 3u);
    EXPECT_EQ(circuit->gate(0).type(), ir::GateType::Rz);
    EXPECT_EQ(circuit->gate(1).type(), ir::GateType::Rx);
    EXPECT_EQ(circuit->gate(2).type(), ir::GateType::Rz);
}

TEST_F(ParserTest, MultipleRegistersAddressing) {
    const char* source = R"(
OPENQASM 3.0;
qubit[2] a;
qubit[2] b;

h a[0];
h b[0];
cx a[0], b[0];
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numQubits(), 4u);  // 2 + 2
    EXPECT_EQ(circuit->numGates(), 3u);
    
    // a[0] maps to qubit 0, b[0] maps to qubit 2
    EXPECT_EQ(circuit->gate(0).qubits()[0], 0u);  // h a[0]
    EXPECT_EQ(circuit->gate(1).qubits()[0], 2u);  // h b[0]
    EXPECT_EQ(circuit->gate(2).qubits()[0], 0u);  // cx a[0], b[0]
    EXPECT_EQ(circuit->gate(2).qubits()[1], 2u);
}

TEST_F(ParserTest, WithComments) {
    const char* source = R"(
// This is a comment
OPENQASM 3.0;
/* Multi-line
   comment */
qubit q;
h q[0]; // Apply Hadamard
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numGates(), 1u);
}

TEST_F(ParserTest, ComplexProgram) {
    const char* source = R"(
OPENQASM 3.0;
include "stdgates.inc";

// Quantum teleportation circuit (partial)
qubit[3] q;
bit[2] c;

// Create Bell pair
h q[1];
cx q[1], q[2];

// Prepare state to teleport
rz(pi/4) q[0];

// Entangle with sender
cx q[0], q[1];
h q[0];

// Measure sender qubits
c[0] = measure q[0];
c[1] = measure q[1];
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numQubits(), 3u);
    EXPECT_EQ(circuit->numGates(), 5u);  // h, cx, rz, cx, h
}

TEST_F(ParserTest, CircuitDepthCalculation) {
    const char* source = R"(
OPENQASM 3.0;
qubit[2] q;

h q[0];
h q[1];
cx q[0], q[1];
)";
    auto circuit = parse(source);
    ASSERT_NE(circuit, nullptr);
    // Depth: h q[0] and h q[1] are parallel (depth 1), then cx (depth 2)
    EXPECT_EQ(circuit->depth(), 2u);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(ParserTest, MinimalProgram) {
    auto circuit = parse("OPENQASM 3.0; qubit q;");
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numQubits(), 1u);
    EXPECT_EQ(circuit->numGates(), 0u);
}

TEST_F(ParserTest, EmptyAfterVersion) {
    // No declarations - should still produce a circuit (with default 1 qubit)
    auto circuit = parse("OPENQASM 3.0;");
    EXPECT_NE(circuit, nullptr);
}

TEST_F(ParserTest, ZeroIndexedQubits) {
    auto circuit = parse("OPENQASM 3.0; qubit[5] q; h q[0]; x q[4];");
    ASSERT_NE(circuit, nullptr);
    EXPECT_EQ(circuit->numGates(), 2u);
    EXPECT_EQ(circuit->gate(0).qubits()[0], 0u);
    EXPECT_EQ(circuit->gate(1).qubits()[0], 4u);
}

}  // namespace
}  // namespace qopt::parser
