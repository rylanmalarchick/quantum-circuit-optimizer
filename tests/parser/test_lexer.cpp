// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_lexer.cpp
 * @brief Unit tests for the OpenQASM 3.0 Lexer
 * @author Rylan Malarchick
 * @date 2025
 *
 * Tests cover:
 * - Single-character tokens (punctuation, operators)
 * - Keywords (OPENQASM, qubit, bit, gate names)
 * - Numbers (integers, floats, scientific notation)
 * - Strings
 * - Identifiers
 * - Comments (single-line, multi-line)
 * - Error cases
 * - Line/column tracking
 */

#include "parser/Lexer.hpp"
#include "parser/Token.hpp"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace qopt::parser {
namespace {

// =============================================================================
// Test Fixtures and Helpers
// =============================================================================

class LexerTest : public ::testing::Test {
protected:
    /**
     * @brief Tokenize source and return all tokens.
     */
    std::vector<Token> tokenize(std::string_view source) {
        Lexer lexer(source);
        return lexer.tokenizeAll();
    }

    /**
     * @brief Get the first token from source.
     */
    Token firstToken(std::string_view source) {
        Lexer lexer(source);
        return lexer.nextToken();
    }

    /**
     * @brief Assert token type and lexeme match.
     */
    void expectToken(const Token& tok, TokenType type, std::string_view lexeme) {
        EXPECT_EQ(tok.type(), type) << "Token: " << tok;
        EXPECT_EQ(tok.lexeme(), lexeme) << "Token: " << tok;
    }

    /**
     * @brief Assert token type, lexeme, and location match.
     */
    void expectTokenAt(const Token& tok, TokenType type, std::string_view lexeme,
                       size_t line, size_t column) {
        EXPECT_EQ(tok.type(), type) << "Token: " << tok;
        EXPECT_EQ(tok.lexeme(), lexeme) << "Token: " << tok;
        EXPECT_EQ(tok.line(), line) << "Token: " << tok;
        EXPECT_EQ(tok.column(), column) << "Token: " << tok;
    }
};

// =============================================================================
// Single-Character Token Tests
// =============================================================================

TEST_F(LexerTest, Semicolon) {
    Token tok = firstToken(";");
    expectToken(tok, TokenType::Semicolon, ";");
}

TEST_F(LexerTest, Comma) {
    Token tok = firstToken(",");
    expectToken(tok, TokenType::Comma, ",");
}

TEST_F(LexerTest, Parentheses) {
    auto tokens = tokenize("()");
    ASSERT_GE(tokens.size(), 3u);
    expectToken(tokens[0], TokenType::LeftParen, "(");
    expectToken(tokens[1], TokenType::RightParen, ")");
}

TEST_F(LexerTest, Brackets) {
    auto tokens = tokenize("[]");
    ASSERT_GE(tokens.size(), 3u);
    expectToken(tokens[0], TokenType::LeftBracket, "[");
    expectToken(tokens[1], TokenType::RightBracket, "]");
}

TEST_F(LexerTest, Braces) {
    auto tokens = tokenize("{}");
    ASSERT_GE(tokens.size(), 3u);
    expectToken(tokens[0], TokenType::LeftBrace, "{");
    expectToken(tokens[1], TokenType::RightBrace, "}");
}

TEST_F(LexerTest, Equals) {
    Token tok = firstToken("=");
    expectToken(tok, TokenType::Equals, "=");
}

TEST_F(LexerTest, ArithmeticOperators) {
    auto tokens = tokenize("+ - * /");
    ASSERT_GE(tokens.size(), 5u);
    expectToken(tokens[0], TokenType::Plus, "+");
    expectToken(tokens[1], TokenType::Minus, "-");
    expectToken(tokens[2], TokenType::Star, "*");
    expectToken(tokens[3], TokenType::Slash, "/");
}

TEST_F(LexerTest, Arrow) {
    Token tok = firstToken("->");
    expectToken(tok, TokenType::Arrow, "->");
}

TEST_F(LexerTest, MinusNotArrow) {
    auto tokens = tokenize("- >");
    ASSERT_GE(tokens.size(), 2u);
    expectToken(tokens[0], TokenType::Minus, "-");
    // '>' is not a valid token, should be error
}

// =============================================================================
// Keyword Tests
// =============================================================================

TEST_F(LexerTest, OpenQASMKeyword) {
    Token tok = firstToken("OPENQASM");
    expectToken(tok, TokenType::OpenQASM, "OPENQASM");
}

TEST_F(LexerTest, IncludeKeyword) {
    Token tok = firstToken("include");
    expectToken(tok, TokenType::Include, "include");
}

TEST_F(LexerTest, QubitKeyword) {
    Token tok = firstToken("qubit");
    expectToken(tok, TokenType::Qubit, "qubit");
}

TEST_F(LexerTest, BitKeyword) {
    Token tok = firstToken("bit");
    expectToken(tok, TokenType::Bit, "bit");
}

TEST_F(LexerTest, MeasureKeyword) {
    Token tok = firstToken("measure");
    expectToken(tok, TokenType::Measure, "measure");
}

TEST_F(LexerTest, PiKeyword) {
    Token tok = firstToken("pi");
    expectToken(tok, TokenType::Pi, "pi");
}

// =============================================================================
// Gate Keyword Tests
// =============================================================================

TEST_F(LexerTest, SingleQubitGates) {
    auto tokens = tokenize("h x y z s t sdg tdg");
    ASSERT_GE(tokens.size(), 9u);
    expectToken(tokens[0], TokenType::GateH, "h");
    expectToken(tokens[1], TokenType::GateX, "x");
    expectToken(tokens[2], TokenType::GateY, "y");
    expectToken(tokens[3], TokenType::GateZ, "z");
    expectToken(tokens[4], TokenType::GateS, "s");
    expectToken(tokens[5], TokenType::GateT, "t");
    expectToken(tokens[6], TokenType::GateSdg, "sdg");
    expectToken(tokens[7], TokenType::GateTdg, "tdg");
}

TEST_F(LexerTest, ParameterizedGates) {
    auto tokens = tokenize("rx ry rz");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::GateRx, "rx");
    expectToken(tokens[1], TokenType::GateRy, "ry");
    expectToken(tokens[2], TokenType::GateRz, "rz");
}

TEST_F(LexerTest, TwoQubitGates) {
    auto tokens = tokenize("cx cz swap cnot");
    ASSERT_GE(tokens.size(), 5u);
    expectToken(tokens[0], TokenType::GateCX, "cx");
    expectToken(tokens[1], TokenType::GateCZ, "cz");
    expectToken(tokens[2], TokenType::GateSwap, "swap");
    expectToken(tokens[3], TokenType::GateCX, "cnot");  // cnot is alias for cx
}

TEST_F(LexerTest, GateTokenHelpers) {
    Token h = firstToken("h");
    EXPECT_TRUE(h.isGate());
    EXPECT_FALSE(h.isParameterizedGate());
    EXPECT_FALSE(h.isTwoQubitGate());

    Token rx = firstToken("rx");
    EXPECT_TRUE(rx.isGate());
    EXPECT_TRUE(rx.isParameterizedGate());
    EXPECT_FALSE(rx.isTwoQubitGate());

    Token cx = firstToken("cx");
    EXPECT_TRUE(cx.isGate());
    EXPECT_FALSE(cx.isParameterizedGate());
    EXPECT_TRUE(cx.isTwoQubitGate());
}

// =============================================================================
// Number Tests
// =============================================================================

TEST_F(LexerTest, Integers) {
    auto tokens = tokenize("0 1 42 123456");
    ASSERT_GE(tokens.size(), 5u);
    expectToken(tokens[0], TokenType::Integer, "0");
    expectToken(tokens[1], TokenType::Integer, "1");
    expectToken(tokens[2], TokenType::Integer, "42");
    expectToken(tokens[3], TokenType::Integer, "123456");
}

TEST_F(LexerTest, Floats) {
    auto tokens = tokenize("3.14 0.5 1.0 123.456");
    ASSERT_GE(tokens.size(), 5u);
    expectToken(tokens[0], TokenType::Float, "3.14");
    expectToken(tokens[1], TokenType::Float, "0.5");
    expectToken(tokens[2], TokenType::Float, "1.0");
    expectToken(tokens[3], TokenType::Float, "123.456");
}

TEST_F(LexerTest, ScientificNotation) {
    auto tokens = tokenize("1e10 1E10 1e+10 1e-10 1.5e10");
    ASSERT_GE(tokens.size(), 6u);
    expectToken(tokens[0], TokenType::Float, "1e10");
    expectToken(tokens[1], TokenType::Float, "1E10");
    expectToken(tokens[2], TokenType::Float, "1e+10");
    expectToken(tokens[3], TokenType::Float, "1e-10");
    expectToken(tokens[4], TokenType::Float, "1.5e10");
}

TEST_F(LexerTest, InvalidExponent) {
    Token tok = firstToken("1e");
    EXPECT_TRUE(tok.isError());
    EXPECT_NE(tok.lexeme().find("exponent"), std::string::npos);
}

// =============================================================================
// String Tests
// =============================================================================

TEST_F(LexerTest, SimpleString) {
    Token tok = firstToken("\"stdgates.inc\"");
    expectToken(tok, TokenType::String, "stdgates.inc");
}

TEST_F(LexerTest, EmptyString) {
    Token tok = firstToken("\"\"");
    expectToken(tok, TokenType::String, "");
}

TEST_F(LexerTest, StringWithSpaces) {
    Token tok = firstToken("\"hello world\"");
    expectToken(tok, TokenType::String, "hello world");
}

TEST_F(LexerTest, UnterminatedString) {
    Token tok = firstToken("\"unterminated");
    EXPECT_TRUE(tok.isError());
    EXPECT_NE(tok.lexeme().find("Unterminated"), std::string::npos);
}

TEST_F(LexerTest, StringWithNewline) {
    Token tok = firstToken("\"hello\nworld\"");
    EXPECT_TRUE(tok.isError());
    EXPECT_NE(tok.lexeme().find("newline"), std::string::npos);
}

// =============================================================================
// Identifier Tests
// =============================================================================

TEST_F(LexerTest, SimpleIdentifiers) {
    auto tokens = tokenize("q c myvar");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::Identifier, "q");
    expectToken(tokens[1], TokenType::Identifier, "c");
    expectToken(tokens[2], TokenType::Identifier, "myvar");
}

TEST_F(LexerTest, IdentifiersWithUnderscores) {
    auto tokens = tokenize("my_var _private __internal__");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::Identifier, "my_var");
    expectToken(tokens[1], TokenType::Identifier, "_private");
    expectToken(tokens[2], TokenType::Identifier, "__internal__");
}

TEST_F(LexerTest, IdentifiersWithNumbers) {
    auto tokens = tokenize("q0 qubit1 var123");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::Identifier, "q0");
    expectToken(tokens[1], TokenType::Identifier, "qubit1");
    expectToken(tokens[2], TokenType::Identifier, "var123");
}

TEST_F(LexerTest, KeywordPrefixIdentifier) {
    // "qubit1" should be an identifier, not "qubit" + "1"
    Token tok = firstToken("qubit1");
    expectToken(tok, TokenType::Identifier, "qubit1");
}

// =============================================================================
// Comment Tests
// =============================================================================

TEST_F(LexerTest, SingleLineComment) {
    auto tokens = tokenize("x // this is a comment\ny");
    ASSERT_GE(tokens.size(), 3u);
    expectToken(tokens[0], TokenType::GateX, "x");
    expectToken(tokens[1], TokenType::GateY, "y");
}

TEST_F(LexerTest, SingleLineCommentAtEnd) {
    auto tokens = tokenize("x // comment");
    ASSERT_GE(tokens.size(), 2u);
    expectToken(tokens[0], TokenType::GateX, "x");
    EXPECT_TRUE(tokens[1].isEof());
}

TEST_F(LexerTest, MultiLineComment) {
    auto tokens = tokenize("x /* multi\nline\ncomment */ y");
    ASSERT_GE(tokens.size(), 3u);
    expectToken(tokens[0], TokenType::GateX, "x");
    expectToken(tokens[1], TokenType::GateY, "y");
}

TEST_F(LexerTest, MultiLineCommentInline) {
    auto tokens = tokenize("x /* inline */ y");
    ASSERT_GE(tokens.size(), 3u);
    expectToken(tokens[0], TokenType::GateX, "x");
    expectToken(tokens[1], TokenType::GateY, "y");
}

TEST_F(LexerTest, SlashNotComment) {
    // Single slash should be division, not start of comment
    Token tok = firstToken("/");
    expectToken(tok, TokenType::Slash, "/");
}

// =============================================================================
// Whitespace Tests
// =============================================================================

TEST_F(LexerTest, WhitespaceHandling) {
    auto tokens = tokenize("  x   y  \t z  ");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::GateX, "x");
    expectToken(tokens[1], TokenType::GateY, "y");
    expectToken(tokens[2], TokenType::GateZ, "z");
}

TEST_F(LexerTest, NewlineHandling) {
    auto tokens = tokenize("x\ny\nz");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::GateX, "x");
    expectToken(tokens[1], TokenType::GateY, "y");
    expectToken(tokens[2], TokenType::GateZ, "z");
}

TEST_F(LexerTest, EmptyInput) {
    auto tokens = tokenize("");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_TRUE(tokens[0].isEof());
}

TEST_F(LexerTest, OnlyWhitespace) {
    auto tokens = tokenize("   \n\t  \r\n  ");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_TRUE(tokens[0].isEof());
}

// =============================================================================
// Error Tests
// =============================================================================

TEST_F(LexerTest, UnexpectedCharacter) {
    Token tok = firstToken("@");
    EXPECT_TRUE(tok.isError());
    EXPECT_NE(tok.lexeme().find("Unexpected"), std::string::npos);
}

TEST_F(LexerTest, ErrorRecoveryOnNextToken) {
    // After an error, lexer should continue to next token
    Lexer lexer("@ x");
    Token err = lexer.nextToken();
    EXPECT_TRUE(err.isError());

    Token x = lexer.nextToken();
    expectToken(x, TokenType::GateX, "x");
}

// =============================================================================
// Line/Column Tracking Tests
// =============================================================================

TEST_F(LexerTest, LineColumnTracking) {
    auto tokens = tokenize("x\ny\nz");
    ASSERT_GE(tokens.size(), 4u);
    expectTokenAt(tokens[0], TokenType::GateX, "x", 1, 1);
    expectTokenAt(tokens[1], TokenType::GateY, "y", 2, 1);
    expectTokenAt(tokens[2], TokenType::GateZ, "z", 3, 1);
}

TEST_F(LexerTest, ColumnTrackingWithSpaces) {
    auto tokens = tokenize("   x   y");
    ASSERT_GE(tokens.size(), 3u);
    expectTokenAt(tokens[0], TokenType::GateX, "x", 1, 4);
    expectTokenAt(tokens[1], TokenType::GateY, "y", 1, 8);
}

TEST_F(LexerTest, LineTrackingAfterMultiLineComment) {
    auto tokens = tokenize("x\n/*\nmulti\nline\n*/\ny");
    ASSERT_GE(tokens.size(), 3u);
    expectTokenAt(tokens[0], TokenType::GateX, "x", 1, 1);
    expectTokenAt(tokens[1], TokenType::GateY, "y", 6, 1);
}

// =============================================================================
// Peek Tests
// =============================================================================

TEST_F(LexerTest, PeekDoesNotConsume) {
    Lexer lexer("x y z");
    Token peeked = lexer.peekToken();
    expectToken(peeked, TokenType::GateX, "x");

    // Peeking again should return the same token
    Token peeked2 = lexer.peekToken();
    expectToken(peeked2, TokenType::GateX, "x");

    // Now consume it
    Token consumed = lexer.nextToken();
    expectToken(consumed, TokenType::GateX, "x");

    // Next token should be y
    Token next = lexer.nextToken();
    expectToken(next, TokenType::GateY, "y");
}

// =============================================================================
// Integration Tests (Full Statements)
// =============================================================================

TEST_F(LexerTest, VersionDeclaration) {
    auto tokens = tokenize("OPENQASM 3.0;");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::OpenQASM, "OPENQASM");
    expectToken(tokens[1], TokenType::Float, "3.0");
    expectToken(tokens[2], TokenType::Semicolon, ";");
}

TEST_F(LexerTest, IncludeStatement) {
    auto tokens = tokenize("include \"stdgates.inc\";");
    ASSERT_GE(tokens.size(), 4u);
    expectToken(tokens[0], TokenType::Include, "include");
    expectToken(tokens[1], TokenType::String, "stdgates.inc");
    expectToken(tokens[2], TokenType::Semicolon, ";");
}

TEST_F(LexerTest, QubitDeclaration) {
    auto tokens = tokenize("qubit[2] q;");
    ASSERT_GE(tokens.size(), 6u);
    expectToken(tokens[0], TokenType::Qubit, "qubit");
    expectToken(tokens[1], TokenType::LeftBracket, "[");
    expectToken(tokens[2], TokenType::Integer, "2");
    expectToken(tokens[3], TokenType::RightBracket, "]");
    expectToken(tokens[4], TokenType::Identifier, "q");
    expectToken(tokens[5], TokenType::Semicolon, ";");
}

TEST_F(LexerTest, GateApplication) {
    auto tokens = tokenize("h q[0];");
    ASSERT_GE(tokens.size(), 6u);
    expectToken(tokens[0], TokenType::GateH, "h");
    expectToken(tokens[1], TokenType::Identifier, "q");
    expectToken(tokens[2], TokenType::LeftBracket, "[");
    expectToken(tokens[3], TokenType::Integer, "0");
    expectToken(tokens[4], TokenType::RightBracket, "]");
    expectToken(tokens[5], TokenType::Semicolon, ";");
}

TEST_F(LexerTest, ParameterizedGateApplication) {
    auto tokens = tokenize("rz(pi/4) q[0];");
    ASSERT_GE(tokens.size(), 11u);
    expectToken(tokens[0], TokenType::GateRz, "rz");
    expectToken(tokens[1], TokenType::LeftParen, "(");
    expectToken(tokens[2], TokenType::Pi, "pi");
    expectToken(tokens[3], TokenType::Slash, "/");
    expectToken(tokens[4], TokenType::Integer, "4");
    expectToken(tokens[5], TokenType::RightParen, ")");
    expectToken(tokens[6], TokenType::Identifier, "q");
    expectToken(tokens[7], TokenType::LeftBracket, "[");
    expectToken(tokens[8], TokenType::Integer, "0");
    expectToken(tokens[9], TokenType::RightBracket, "]");
    expectToken(tokens[10], TokenType::Semicolon, ";");
}

TEST_F(LexerTest, TwoQubitGateApplication) {
    auto tokens = tokenize("cx q[0], q[1];");
    ASSERT_GE(tokens.size(), 11u);
    expectToken(tokens[0], TokenType::GateCX, "cx");
    expectToken(tokens[1], TokenType::Identifier, "q");
    expectToken(tokens[2], TokenType::LeftBracket, "[");
    expectToken(tokens[3], TokenType::Integer, "0");
    expectToken(tokens[4], TokenType::RightBracket, "]");
    expectToken(tokens[5], TokenType::Comma, ",");
}

TEST_F(LexerTest, Measurement) {
    auto tokens = tokenize("c[0] = measure q[0];");
    ASSERT_GE(tokens.size(), 11u);
    expectToken(tokens[0], TokenType::Identifier, "c");
    expectToken(tokens[1], TokenType::LeftBracket, "[");
    expectToken(tokens[2], TokenType::Integer, "0");
    expectToken(tokens[3], TokenType::RightBracket, "]");
    expectToken(tokens[4], TokenType::Equals, "=");
    expectToken(tokens[5], TokenType::Measure, "measure");
}

TEST_F(LexerTest, FullProgram) {
    const char* source = R"(
// Simple Bell state
OPENQASM 3.0;
include "stdgates.inc";

qubit[2] q;
bit[2] c;

h q[0];
cx q[0], q[1];
c = measure q;
)";
    auto tokens = tokenize(source);
    
    // Should have many tokens and end with EOF
    EXPECT_GT(tokens.size(), 20u);
    EXPECT_TRUE(tokens.back().isEof());
    
    // First meaningful token should be OPENQASM (after comment)
    EXPECT_EQ(tokens[0].type(), TokenType::OpenQASM);
}

// =============================================================================
// Token Helper Tests
// =============================================================================

TEST_F(LexerTest, TokenIsOneOf) {
    Token tok = firstToken("h");
    EXPECT_TRUE(tok.isOneOf(TokenType::GateH, TokenType::GateX));
    EXPECT_FALSE(tok.isOneOf(TokenType::GateY, TokenType::GateZ));
}

TEST_F(LexerTest, TokenEquality) {
    Token a = firstToken("h");
    Token b = firstToken("h");
    EXPECT_EQ(a, b);
    
    Token c = firstToken("x");
    EXPECT_NE(a, c);
}

TEST_F(LexerTest, DefaultTokenIsEof) {
    Token tok;
    EXPECT_TRUE(tok.isEof());
    EXPECT_EQ(tok.lexeme(), "");
}

}  // namespace
}  // namespace qopt::parser
