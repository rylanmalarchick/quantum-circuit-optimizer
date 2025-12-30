// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file QASMError.hpp
 * @brief Error types for OpenQASM 3.0 parsing with source location
 * @author Rylan Malarchick
 * @date 2025
 *
 * Provides structured error reporting for lexing and parsing errors,
 * including source location (line, column) for user-friendly messages.
 *
 * @see Lexer.hpp for lexical errors
 * @see Parser.hpp for parse errors
 */
#pragma once

#include "Token.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace qopt::parser {

/**
 * @brief Category of QASM error.
 */
enum class QASMErrorKind {
    Lexical,     ///< Tokenization error (invalid character, unterminated string)
    Syntax,      ///< Parse error (unexpected token, missing semicolon)
    Semantic,    ///< Semantic error (undeclared variable, type mismatch)
};

/**
 * @brief Get string representation of error kind.
 */
[[nodiscard]] constexpr std::string_view errorKindName(QASMErrorKind kind) noexcept {
    switch (kind) {
        case QASMErrorKind::Lexical:  return "lexical error";
        case QASMErrorKind::Syntax:   return "syntax error";
        case QASMErrorKind::Semantic: return "semantic error";
    }
    return "error";
}

/**
 * @brief A single error from QASM lexing or parsing.
 *
 * Contains the error message, source location, and error category.
 * Designed for accumulating multiple errors before reporting.
 */
class QASMError {
public:
    /**
     * @brief Construct an error with message and location.
     * @param kind Error category
     * @param message Description of the error
     * @param location Source location where error occurred
     */
    QASMError(QASMErrorKind kind, std::string message, SourceLocation location)
        : kind_(kind)
        , message_(std::move(message))
        , location_(location) {}

    /**
     * @brief Construct an error at a token's location.
     * @param kind Error category
     * @param message Description of the error
     * @param token Token where error occurred
     */
    QASMError(QASMErrorKind kind, std::string message, const Token& token)
        : kind_(kind)
        , message_(std::move(message))
        , location_(token.location()) {}

    // Accessors
    [[nodiscard]] QASMErrorKind kind() const noexcept { return kind_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] SourceLocation location() const noexcept { return location_; }
    [[nodiscard]] size_t line() const noexcept { return location_.line; }
    [[nodiscard]] size_t column() const noexcept { return location_.column; }

    /**
     * @brief Format the error as a string.
     * @return Formatted error message with location
     *
     * Format: "line:column: error_kind: message"
     */
    [[nodiscard]] std::string format() const {
        return std::to_string(location_.line) + ":" +
               std::to_string(location_.column) + ": " +
               std::string(errorKindName(kind_)) + ": " +
               message_;
    }

private:
    QASMErrorKind kind_;
    std::string message_;
    SourceLocation location_;
};

/**
 * @brief Stream output for QASMError.
 */
inline std::ostream& operator<<(std::ostream& os, const QASMError& error) {
    os << error.format();
    return os;
}

/**
 * @brief Exception thrown when parsing fails.
 *
 * Contains all accumulated errors from the parse attempt.
 * Inherits from std::runtime_error for compatibility with
 * standard exception handling.
 */
class QASMParseException : public std::runtime_error {
public:
    /**
     * @brief Construct with a single error.
     */
    explicit QASMParseException(QASMError error)
        : std::runtime_error(error.format())
        , errors_{std::move(error)} {}

    /**
     * @brief Construct with multiple errors.
     */
    explicit QASMParseException(std::vector<QASMError> errors)
        : std::runtime_error(formatErrors(errors))
        , errors_(std::move(errors)) {}

    /**
     * @brief Get all parse errors.
     */
    [[nodiscard]] const std::vector<QASMError>& errors() const noexcept {
        return errors_;
    }

    /**
     * @brief Get the number of errors.
     */
    [[nodiscard]] size_t numErrors() const noexcept {
        return errors_.size();
    }

private:
    std::vector<QASMError> errors_;

    static std::string formatErrors(const std::vector<QASMError>& errors) {
        if (errors.empty()) {
            return "parse error";
        }
        if (errors.size() == 1) {
            return errors[0].format();
        }
        std::string result = std::to_string(errors.size()) + " errors:\n";
        for (const auto& err : errors) {
            result += "  " + err.format() + "\n";
        }
        return result;
    }
};

/**
 * @brief Helper to create a lexical error.
 */
[[nodiscard]] inline QASMError lexicalError(
    const std::string& message,
    SourceLocation location) {
    return QASMError(QASMErrorKind::Lexical, message, location);
}

/**
 * @brief Helper to create a syntax error.
 */
[[nodiscard]] inline QASMError syntaxError(
    const std::string& message,
    const Token& token) {
    return QASMError(QASMErrorKind::Syntax, message, token);
}

/**
 * @brief Helper to create a syntax error with custom location.
 */
[[nodiscard]] inline QASMError syntaxError(
    const std::string& message,
    SourceLocation location) {
    return QASMError(QASMErrorKind::Syntax, message, location);
}

/**
 * @brief Helper to create a semantic error.
 */
[[nodiscard]] inline QASMError semanticError(
    const std::string& message,
    const Token& token) {
    return QASMError(QASMErrorKind::Semantic, message, token);
}

}  // namespace qopt::parser
