// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Token.hpp
 * @brief Token types and Token class for OpenQASM 3.0 lexer
 * @author Rylan Malarchick
 * @date 2025
 *
 * This file defines the token types recognized by the OpenQASM 3.0 lexer
 * and the Token class that carries token information including source location.
 *
 * @see Lexer.hpp for the tokenizer implementation
 * @see Parser.hpp for the parser that consumes tokens
 *
 * @references
 * - OpenQASM 3.0 Specification: https://openqasm.com/
 */
#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>

namespace qopt::parser {

/**
 * @brief Token types recognized by the OpenQASM 3.0 lexer.
 *
 * Organized by category for readability.
 */
enum class TokenType {
    // Special tokens
    EndOfFile,   ///< End of input
    Error,       ///< Lexical error

    // Literals
    Integer,     ///< Integer literal (e.g., 42)
    Float,       ///< Floating-point literal (e.g., 3.14)
    String,      ///< String literal (e.g., "stdgates.inc")

    // Identifiers and keywords
    Identifier,  ///< User-defined name (e.g., q, mygate)

    // OpenQASM keywords
    OpenQASM,    ///< OPENQASM keyword
    Include,     ///< include keyword
    Qubit,       ///< qubit keyword
    Bit,         ///< bit keyword
    Measure,     ///< measure keyword

    // Gate names (treated as keywords for simplicity)
    // Single-qubit gates
    GateH,       ///< h (Hadamard)
    GateX,       ///< x (Pauli-X)
    GateY,       ///< y (Pauli-Y)
    GateZ,       ///< z (Pauli-Z)
    GateS,       ///< s (S gate)
    GateT,       ///< t (T gate)
    GateSdg,     ///< sdg (S-dagger)
    GateTdg,     ///< tdg (T-dagger)
    GateRx,      ///< rx (X rotation)
    GateRy,      ///< ry (Y rotation)
    GateRz,      ///< rz (Z rotation)

    // Two-qubit gates
    GateCX,      ///< cx (CNOT)
    GateCZ,      ///< cz (controlled-Z)
    GateSwap,    ///< swap

    // Mathematical constants
    Pi,          ///< pi constant

    // Operators and punctuation
    Semicolon,   ///< ;
    Comma,       ///< ,
    LeftParen,   ///< (
    RightParen,  ///< )
    LeftBracket, ///< [
    RightBracket,///< ]
    LeftBrace,   ///< {
    RightBrace,  ///< }
    Equals,      ///< =
    Arrow,       ///< -> (for measurement)

    // Arithmetic operators
    Plus,        ///< +
    Minus,       ///< -
    Star,        ///< *
    Slash,       ///< /
};

/**
 * @brief Get string representation of a token type.
 * @param type The token type
 * @return Human-readable name of the token type
 */
[[nodiscard]] constexpr std::string_view tokenTypeName(TokenType type) noexcept {
    switch (type) {
        case TokenType::EndOfFile:    return "EndOfFile";
        case TokenType::Error:        return "Error";
        case TokenType::Integer:      return "Integer";
        case TokenType::Float:        return "Float";
        case TokenType::String:       return "String";
        case TokenType::Identifier:   return "Identifier";
        case TokenType::OpenQASM:     return "OPENQASM";
        case TokenType::Include:      return "include";
        case TokenType::Qubit:        return "qubit";
        case TokenType::Bit:          return "bit";
        case TokenType::Measure:      return "measure";
        case TokenType::GateH:        return "h";
        case TokenType::GateX:        return "x";
        case TokenType::GateY:        return "y";
        case TokenType::GateZ:        return "z";
        case TokenType::GateS:        return "s";
        case TokenType::GateT:        return "t";
        case TokenType::GateSdg:      return "sdg";
        case TokenType::GateTdg:      return "tdg";
        case TokenType::GateRx:       return "rx";
        case TokenType::GateRy:       return "ry";
        case TokenType::GateRz:       return "rz";
        case TokenType::GateCX:       return "cx";
        case TokenType::GateCZ:       return "cz";
        case TokenType::GateSwap:     return "swap";
        case TokenType::Pi:           return "pi";
        case TokenType::Semicolon:    return ";";
        case TokenType::Comma:        return ",";
        case TokenType::LeftParen:    return "(";
        case TokenType::RightParen:   return ")";
        case TokenType::LeftBracket:  return "[";
        case TokenType::RightBracket: return "]";
        case TokenType::LeftBrace:    return "{";
        case TokenType::RightBrace:   return "}";
        case TokenType::Equals:       return "=";
        case TokenType::Arrow:        return "->";
        case TokenType::Plus:         return "+";
        case TokenType::Minus:        return "-";
        case TokenType::Star:         return "*";
        case TokenType::Slash:        return "/";
    }
    return "Unknown";
}

/**
 * @brief Source location within the input.
 *
 * Used for error reporting with line and column information.
 */
struct SourceLocation {
    size_t line = 1;    ///< 1-based line number
    size_t column = 1;  ///< 1-based column number
    size_t offset = 0;  ///< 0-based byte offset from start

    /**
     * @brief Check equality of two source locations.
     */
    [[nodiscard]] bool operator==(const SourceLocation& other) const noexcept {
        return line == other.line && column == other.column && offset == other.offset;
    }

    /**
     * @brief Check inequality of two source locations.
     */
    [[nodiscard]] bool operator!=(const SourceLocation& other) const noexcept {
        return !(*this == other);
    }
};

/**
 * @brief A token from the OpenQASM 3.0 lexer.
 *
 * Contains the token type, lexeme (text), and source location.
 * Tokens are lightweight and copyable.
 */
class Token {
public:
    /**
     * @brief Construct a token.
     * @param type Token type
     * @param lexeme The text of the token
     * @param location Source location
     */
    Token(TokenType type, std::string lexeme, SourceLocation location) noexcept
        : type_(type)
        , lexeme_(std::move(lexeme))
        , location_(location) {}

    /**
     * @brief Default constructor creates an EOF token.
     */
    Token() noexcept
        : type_(TokenType::EndOfFile)
        , lexeme_("")
        , location_{} {}

    // Accessors
    [[nodiscard]] TokenType type() const noexcept { return type_; }
    [[nodiscard]] const std::string& lexeme() const noexcept { return lexeme_; }
    [[nodiscard]] SourceLocation location() const noexcept { return location_; }
    [[nodiscard]] size_t line() const noexcept { return location_.line; }
    [[nodiscard]] size_t column() const noexcept { return location_.column; }

    /**
     * @brief Check if this token is of a specific type.
     */
    [[nodiscard]] bool is(TokenType t) const noexcept { return type_ == t; }

    /**
     * @brief Check if this token is one of multiple types.
     */
    template<typename... Types>
    [[nodiscard]] bool isOneOf(Types... types) const noexcept {
        return (is(types) || ...);
    }

    /**
     * @brief Check if this is an error token.
     */
    [[nodiscard]] bool isError() const noexcept { return type_ == TokenType::Error; }

    /**
     * @brief Check if this is the end of file.
     */
    [[nodiscard]] bool isEof() const noexcept { return type_ == TokenType::EndOfFile; }

    /**
     * @brief Check if this is a gate keyword.
     */
    [[nodiscard]] bool isGate() const noexcept {
        return isOneOf(
            TokenType::GateH, TokenType::GateX, TokenType::GateY, TokenType::GateZ,
            TokenType::GateS, TokenType::GateT, TokenType::GateSdg, TokenType::GateTdg,
            TokenType::GateRx, TokenType::GateRy, TokenType::GateRz,
            TokenType::GateCX, TokenType::GateCZ, TokenType::GateSwap
        );
    }

    /**
     * @brief Check if this is a parameterized gate.
     */
    [[nodiscard]] bool isParameterizedGate() const noexcept {
        return isOneOf(TokenType::GateRx, TokenType::GateRy, TokenType::GateRz);
    }

    /**
     * @brief Check if this is a two-qubit gate.
     */
    [[nodiscard]] bool isTwoQubitGate() const noexcept {
        return isOneOf(TokenType::GateCX, TokenType::GateCZ, TokenType::GateSwap);
    }

    /**
     * @brief Equality comparison.
     */
    [[nodiscard]] bool operator==(const Token& other) const noexcept {
        return type_ == other.type_ && lexeme_ == other.lexeme_;
    }

    /**
     * @brief Inequality comparison.
     */
    [[nodiscard]] bool operator!=(const Token& other) const noexcept {
        return !(*this == other);
    }

private:
    TokenType type_;
    std::string lexeme_;
    SourceLocation location_;
};

/**
 * @brief Stream output for Token (for debugging).
 */
inline std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "Token(" << tokenTypeName(token.type())
       << ", \"" << token.lexeme() << "\""
       << ", " << token.line() << ":" << token.column() << ")";
    return os;
}

/**
 * @brief Stream output for SourceLocation.
 */
inline std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
    os << loc.line << ":" << loc.column;
    return os;
}

}  // namespace qopt::parser
