// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Lexer.hpp
 * @brief Tokenizer for OpenQASM 3.0 source code
 * @author Rylan Malarchick
 * @date 2025
 *
 * Hand-written lexer that tokenizes OpenQASM 3.0 source code into a stream
 * of tokens. Supports line/column tracking for error reporting.
 *
 * Supported constructs:
 * - Version declaration: OPENQASM 3.0;
 * - Include statements: include "stdgates.inc";
 * - Register declarations: qubit[n] q; bit[n] c;
 * - Gate applications: h q[0]; cx q[0], q[1]; rz(pi/4) q[0];
 * - Measurement: c[0] = measure q[0];
 * - Comments: // single-line, multi-line with slash-star
 *
 * @see Token.hpp for token types
 * @see Parser.hpp for the parser that consumes tokens
 *
 * @references
 * - OpenQASM 3.0 Specification: https://openqasm.com/
 */
#pragma once

#include "Token.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace qopt::parser {

/**
 * @brief Tokenizer for OpenQASM 3.0 source code.
 *
 * The Lexer converts source text into a stream of tokens. It tracks
 * line and column numbers for error reporting.
 *
 * Usage:
 * @code
 * Lexer lexer("OPENQASM 3.0; qubit[2] q;");
 * while (true) {
 *     Token tok = lexer.nextToken();
 *     if (tok.isEof()) break;
 *     std::cout << tok << std::endl;
 * }
 * @endcode
 *
 * Thread safety: Not thread-safe. Each thread should have its own Lexer.
 */
class Lexer {
public:
    /**
     * @brief Construct a lexer for the given source code.
     * @param source The OpenQASM 3.0 source code to tokenize
     */
    explicit Lexer(std::string_view source) noexcept
        : source_(source)
        , current_(0)
        , line_(1)
        , column_(1)
        , lineStart_(0) {}

    /**
     * @brief Get the next token from the source.
     * @return The next token, or EOF token if at end
     *
     * Advances the lexer position past the returned token.
     * On lexical error, returns an Error token with the error message
     * in the lexeme.
     */
    [[nodiscard]] Token nextToken() noexcept {
        skipWhitespaceAndComments();

        if (isAtEnd()) {
            return Token(TokenType::EndOfFile, "", currentLocation());
        }

        tokenStart_ = current_;
        tokenStartLocation_ = currentLocation();

        char c = advance();

        // Single-character tokens
        switch (c) {
            case ';': return makeToken(TokenType::Semicolon);
            case ',': return makeToken(TokenType::Comma);
            case '(': return makeToken(TokenType::LeftParen);
            case ')': return makeToken(TokenType::RightParen);
            case '[': return makeToken(TokenType::LeftBracket);
            case ']': return makeToken(TokenType::RightBracket);
            case '{': return makeToken(TokenType::LeftBrace);
            case '}': return makeToken(TokenType::RightBrace);
            case '=': return makeToken(TokenType::Equals);
            case '+': return makeToken(TokenType::Plus);
            case '*': return makeToken(TokenType::Star);
            case '/': return makeToken(TokenType::Slash);
            case '-':
                if (match('>')) {
                    return makeToken(TokenType::Arrow);
                }
                return makeToken(TokenType::Minus);
            case '"': return scanString();
            default:
                break;
        }

        // Numbers
        if (std::isdigit(static_cast<unsigned char>(c))) {
            return scanNumber();
        }

        // Identifiers and keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            return scanIdentifier();
        }

        return errorToken("Unexpected character");
    }

    /**
     * @brief Peek at the next token without consuming it.
     * @return The next token
     *
     * Does not advance the lexer position.
     */
    [[nodiscard]] Token peekToken() noexcept {
        size_t savedCurrent = current_;
        size_t savedLine = line_;
        size_t savedColumn = column_;
        size_t savedLineStart = lineStart_;

        Token tok = nextToken();

        current_ = savedCurrent;
        line_ = savedLine;
        column_ = savedColumn;
        lineStart_ = savedLineStart;

        return tok;
    }

    /**
     * @brief Tokenize the entire source into a vector.
     * @return Vector of all tokens including final EOF
     */
    [[nodiscard]] std::vector<Token> tokenizeAll() noexcept {
        std::vector<Token> tokens;
        while (true) {
            Token tok = nextToken();
            tokens.push_back(tok);
            if (tok.isEof() || tok.isError()) {
                break;
            }
        }
        return tokens;
    }

    /**
     * @brief Get the current source position.
     */
    [[nodiscard]] SourceLocation currentLocation() const noexcept {
        return SourceLocation{line_, column_, current_};
    }

    /**
     * @brief Check if we've reached the end of input.
     */
    [[nodiscard]] bool isAtEnd() const noexcept {
        return current_ >= source_.size();
    }

private:
    std::string_view source_;
    size_t current_;
    size_t line_;
    size_t column_;
    size_t lineStart_;
    size_t tokenStart_ = 0;
    SourceLocation tokenStartLocation_{};

    // Keyword lookup table
    static const std::unordered_map<std::string_view, TokenType>& keywords() {
        static const std::unordered_map<std::string_view, TokenType> kw = {
            {"OPENQASM", TokenType::OpenQASM},
            {"include", TokenType::Include},
            {"qubit", TokenType::Qubit},
            {"bit", TokenType::Bit},
            {"measure", TokenType::Measure},
            {"h", TokenType::GateH},
            {"x", TokenType::GateX},
            {"y", TokenType::GateY},
            {"z", TokenType::GateZ},
            {"s", TokenType::GateS},
            {"t", TokenType::GateT},
            {"sdg", TokenType::GateSdg},
            {"tdg", TokenType::GateTdg},
            {"rx", TokenType::GateRx},
            {"ry", TokenType::GateRy},
            {"rz", TokenType::GateRz},
            {"cx", TokenType::GateCX},
            {"cnot", TokenType::GateCX},  // Alias
            {"cz", TokenType::GateCZ},
            {"swap", TokenType::GateSwap},
            {"pi", TokenType::Pi},
        };
        return kw;
    }

    /**
     * @brief Consume and return the current character.
     */
    char advance() noexcept {
        char c = source_[current_++];
        if (c == '\n') {
            line_++;
            lineStart_ = current_;
            column_ = 1;
        } else {
            column_++;
        }
        return c;
    }

    /**
     * @brief Peek at the current character without consuming.
     */
    [[nodiscard]] char peek() const noexcept {
        if (isAtEnd()) return '\0';
        return source_[current_];
    }

    /**
     * @brief Peek at the next character without consuming.
     */
    [[nodiscard]] char peekNext() const noexcept {
        if (current_ + 1 >= source_.size()) return '\0';
        return source_[current_ + 1];
    }

    /**
     * @brief Consume if the current character matches expected.
     */
    bool match(char expected) noexcept {
        if (isAtEnd() || source_[current_] != expected) return false;
        advance();
        return true;
    }

    /**
     * @brief Skip whitespace and comments.
     */
    void skipWhitespaceAndComments() noexcept {
        while (!isAtEnd()) {
            char c = peek();
            switch (c) {
                case ' ':
                case '\t':
                case '\r':
                case '\n':
                    advance();
                    break;
                case '/':
                    if (peekNext() == '/') {
                        // Single-line comment
                        while (!isAtEnd() && peek() != '\n') {
                            advance();
                        }
                    } else if (peekNext() == '*') {
                        // Multi-line comment
                        advance(); // consume /
                        advance(); // consume *
                        while (!isAtEnd()) {
                            if (peek() == '*' && peekNext() == '/') {
                                advance(); // consume *
                                advance(); // consume /
                                break;
                            }
                            advance();
                        }
                    } else {
                        return;
                    }
                    break;
                default:
                    return;
            }
        }
    }

    /**
     * @brief Create a token from the current span.
     */
    [[nodiscard]] Token makeToken(TokenType type) const noexcept {
        std::string lexeme(source_.substr(tokenStart_, current_ - tokenStart_));
        return Token(type, std::move(lexeme), tokenStartLocation_);
    }

    /**
     * @brief Create an error token.
     */
    [[nodiscard]] Token errorToken(const char* message) const noexcept {
        return Token(TokenType::Error, message, tokenStartLocation_);
    }

    /**
     * @brief Scan a string literal.
     */
    [[nodiscard]] Token scanString() noexcept {
        while (!isAtEnd() && peek() != '"') {
            if (peek() == '\n') {
                return errorToken("Unterminated string (newline in string)");
            }
            advance();
        }

        if (isAtEnd()) {
            return errorToken("Unterminated string");
        }

        advance(); // Closing "

        // Extract content without quotes
        std::string value(source_.substr(tokenStart_ + 1, current_ - tokenStart_ - 2));
        return Token(TokenType::String, std::move(value), tokenStartLocation_);
    }

    /**
     * @brief Scan a number literal (integer or float).
     */
    [[nodiscard]] Token scanNumber() noexcept {
        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }

        // Check for decimal part
        bool isFloat = false;
        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
            isFloat = true;
            advance(); // consume .
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        // Check for exponent
        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            advance(); // consume e/E
            if (peek() == '+' || peek() == '-') {
                advance();
            }
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                return errorToken("Invalid number: expected digit after exponent");
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) {
                advance();
            }
        }

        return makeToken(isFloat ? TokenType::Float : TokenType::Integer);
    }

    /**
     * @brief Scan an identifier or keyword.
     */
    [[nodiscard]] Token scanIdentifier() noexcept {
        while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
            advance();
        }

        std::string_view text = source_.substr(tokenStart_, current_ - tokenStart_);
        
        // Check if it's a keyword
        auto it = keywords().find(text);
        TokenType type = (it != keywords().end()) ? it->second : TokenType::Identifier;

        return Token(type, std::string(text), tokenStartLocation_);
    }
};

}  // namespace qopt::parser
