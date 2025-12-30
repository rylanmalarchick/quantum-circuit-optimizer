// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Parser.hpp
 * @brief Recursive descent parser for OpenQASM 3.0
 * @author Rylan Malarchick
 * @date 2025
 *
 * Parses OpenQASM 3.0 source code and produces an IR Circuit.
 * Supports a subset of OpenQASM 3.0 suitable for circuit optimization:
 *
 * - Version declaration: OPENQASM 3.0;
 * - Include statements: include "stdgates.inc";
 * - Register declarations: qubit[n] q; bit[n] c;
 * - Gate applications: h q[0]; cx q[0], q[1]; rz(pi/4) q[0];
 * - Measurement: c[0] = measure q[0];
 *
 * @see Lexer.hpp for tokenization
 * @see QASMError.hpp for error handling
 * @see ir/Circuit.hpp for the output representation
 *
 * @references
 * - OpenQASM 3.0 Specification: https://openqasm.com/
 */
#pragma once

#include "Lexer.hpp"
#include "QASMError.hpp"
#include "Token.hpp"
#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"

#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace qopt::parser {

/**
 * @brief Result of parsing an OpenQASM 3.0 program.
 *
 * Contains the parsed circuit and any non-fatal warnings or notes.
 */
struct ParseResult {
    std::unique_ptr<ir::Circuit> circuit;  ///< The parsed circuit (null if parse failed)
    std::vector<QASMError> warnings;        ///< Non-fatal warnings
    
    /**
     * @brief Check if parsing succeeded.
     */
    [[nodiscard]] bool success() const noexcept {
        return circuit != nullptr;
    }
};

/**
 * @brief Recursive descent parser for OpenQASM 3.0.
 *
 * Parses a subset of OpenQASM 3.0 into the IR Circuit representation.
 * Accumulates errors and can report multiple issues before failing.
 *
 * Usage:
 * @code
 * Parser parser("OPENQASM 3.0; qubit[2] q; h q[0]; cx q[0], q[1];");
 * auto result = parser.parse();
 * if (result.success()) {
 *     // Use result.circuit
 * }
 * @endcode
 *
 * Thread safety: Not thread-safe. Each thread should have its own Parser.
 */
class Parser {
public:
    /**
     * @brief Construct a parser for the given source code.
     * @param source The OpenQASM 3.0 source code to parse
     */
    explicit Parser(std::string_view source)
        : lexer_(source)
        , current_(lexer_.nextToken())
        , hadError_(false)
        , panicMode_(false) {}

    /**
     * @brief Parse the source code into a circuit.
     * @return ParseResult containing the circuit or nullptr on failure
     * @throws QASMParseException if parsing fails with unrecoverable errors
     *
     * After calling parse(), use hasErrors() to check for issues.
     */
    [[nodiscard]] ParseResult parse() {
        ParseResult result;
        
        // Parse version declaration (required)
        parseVersionDeclaration();
        
        // Parse statements until EOF
        while (!check(TokenType::EndOfFile) && !hadError_) {
            parseStatement();
        }
        
        if (hadError_) {
            throw QASMParseException(errors_);
        }
        
        // Build the circuit from parsed data
        result.circuit = buildCircuit();
        result.warnings = std::move(warnings_);
        
        return result;
    }

    /**
     * @brief Check if any errors occurred during parsing.
     */
    [[nodiscard]] bool hasErrors() const noexcept {
        return hadError_;
    }

    /**
     * @brief Get all accumulated errors.
     */
    [[nodiscard]] const std::vector<QASMError>& errors() const noexcept {
        return errors_;
    }

private:
    Lexer lexer_;
    Token current_;
    Token previous_;
    bool hadError_;
    bool panicMode_;
    
    std::vector<QASMError> errors_;
    std::vector<QASMError> warnings_;
    
    // Parsed declarations
    struct RegisterInfo {
        std::string name;
        size_t size;
        bool isQubit;  // true for qubit, false for bit
    };
    std::vector<RegisterInfo> registers_;
    std::unordered_map<std::string, size_t> registerIndex_;  // name -> index in registers_
    
    // Parsed gates (before building circuit)
    struct ParsedGate {
        ir::GateType type;
        std::vector<std::pair<std::string, size_t>> qubits;  // (register, index) pairs
        std::optional<double> parameter;
    };
    std::vector<ParsedGate> gates_;
    
    // Parsed measurements
    struct ParsedMeasurement {
        std::pair<std::string, size_t> bitTarget;   // (register, index)
        std::pair<std::string, size_t> qubitSource; // (register, index)
    };
    std::vector<ParsedMeasurement> measurements_;

    // =========================================================================
    // Token Management
    // =========================================================================

    /**
     * @brief Advance to the next token.
     */
    Token advance() {
        previous_ = current_;
        
        while (true) {
            current_ = lexer_.nextToken();
            
            if (!current_.isError()) {
                break;
            }
            
            // Report lexical error and continue
            errorAtCurrent(current_.lexeme());
        }
        
        return previous_;
    }

    /**
     * @brief Check if current token is of the given type.
     */
    [[nodiscard]] bool check(TokenType type) const noexcept {
        return current_.type() == type;
    }

    /**
     * @brief Consume the current token if it matches the expected type.
     */
    bool match(TokenType type) {
        if (!check(type)) return false;
        advance();
        return true;
    }

    /**
     * @brief Consume a token of the expected type or report an error.
     */
    void consume(TokenType type, const std::string& message) {
        if (check(type)) {
            advance();
            return;
        }
        
        errorAtCurrent(message);
    }

    // =========================================================================
    // Error Handling
    // =========================================================================

    /**
     * @brief Report an error at the current token.
     */
    void errorAtCurrent(const std::string& message) {
        errorAt(current_, message);
    }

    /**
     * @brief Report an error at the previous token.
     */
    void errorAtPrevious(const std::string& message) {
        errorAt(previous_, message);
    }

    /**
     * @brief Report an error at a specific token.
     */
    void errorAt(const Token& token, const std::string& message) {
        if (panicMode_) return;  // Suppress cascade errors
        panicMode_ = true;
        hadError_ = true;
        
        std::string fullMessage = message;
        if (!token.isEof() && !token.isError()) {
            fullMessage += " (got '" + token.lexeme() + "')";
        }
        
        errors_.push_back(syntaxError(fullMessage, token));
    }

    /**
     * @brief Add a warning (non-fatal).
     */
    void warn(const Token& token, const std::string& message) {
        warnings_.push_back(QASMError(QASMErrorKind::Syntax, message, token));
    }

    /**
     * @brief Synchronize after an error by skipping to a statement boundary.
     */
    void synchronize() {
        panicMode_ = false;
        
        while (!check(TokenType::EndOfFile)) {
            // Statement just ended
            if (previous_.type() == TokenType::Semicolon) return;
            
            // Statement about to start
            switch (current_.type()) {
                case TokenType::Qubit:
                case TokenType::Bit:
                case TokenType::Include:
                case TokenType::Measure:
                case TokenType::GateH:
                case TokenType::GateX:
                case TokenType::GateY:
                case TokenType::GateZ:
                case TokenType::GateS:
                case TokenType::GateT:
                case TokenType::GateSdg:
                case TokenType::GateTdg:
                case TokenType::GateRx:
                case TokenType::GateRy:
                case TokenType::GateRz:
                case TokenType::GateCX:
                case TokenType::GateCZ:
                case TokenType::GateSwap:
                    return;
                default:
                    break;
            }
            
            advance();
        }
    }

    // =========================================================================
    // Parsing Rules
    // =========================================================================

    /**
     * @brief Parse: OPENQASM 3.0;
     */
    void parseVersionDeclaration() {
        consume(TokenType::OpenQASM, "Expected 'OPENQASM' version declaration");
        if (hadError_) return;
        
        // Expect version number (e.g., "3.0" or "3")
        if (!check(TokenType::Float) && !check(TokenType::Integer)) {
            errorAtCurrent("Expected version number after 'OPENQASM'");
            return;
        }
        
        Token versionToken = current_;
        advance();
        
        // Check version is 3.x
        double version = std::stod(versionToken.lexeme());
        if (version < 3.0 || version >= 4.0) {
            warn(versionToken, "Only OpenQASM 3.x is fully supported");
        }
        
        consume(TokenType::Semicolon, "Expected ';' after version declaration");
    }

    /**
     * @brief Parse a single statement.
     */
    void parseStatement() {
        if (match(TokenType::Include)) {
            parseInclude();
        } else if (match(TokenType::Qubit)) {
            parseQubitDeclaration();
        } else if (match(TokenType::Bit)) {
            parseBitDeclaration();
        } else if (current_.isGate()) {
            parseGateApplication();
        } else if (check(TokenType::Identifier)) {
            // Could be: c[0] = measure q[0]; or c = measure q;
            parseMeasurementOrAssignment();
        } else if (match(TokenType::Measure)) {
            // Standalone measure (result discarded)
            parseStandaloneMeasure();
        } else {
            errorAtCurrent("Expected statement");
            synchronize();
        }
    }

    /**
     * @brief Parse: include "filename";
     */
    void parseInclude() {
        if (!check(TokenType::String)) {
            errorAtCurrent("Expected filename string after 'include'");
            synchronize();
            return;
        }
        
        Token filename = current_;
        advance();
        
        // We don't actually process includes, just acknowledge them
        // stdgates.inc defines standard gates which we have built-in
        if (filename.lexeme() != "stdgates.inc") {
            warn(filename, "Include file ignored (only stdgates.inc is supported)");
        }
        
        consume(TokenType::Semicolon, "Expected ';' after include statement");
    }

    /**
     * @brief Parse: qubit[n] name; or qubit name;
     */
    void parseQubitDeclaration() {
        size_t size = 1;
        
        if (match(TokenType::LeftBracket)) {
            size = parseIntegerLiteral("qubit array size");
            consume(TokenType::RightBracket, "Expected ']' after qubit size");
        }
        
        if (!check(TokenType::Identifier)) {
            errorAtCurrent("Expected register name after 'qubit'");
            synchronize();
            return;
        }
        
        std::string name = current_.lexeme();
        advance();
        
        // Check for duplicate
        if (registerIndex_.count(name) > 0) {
            errorAtPrevious("Register '" + name + "' already declared");
            synchronize();
            return;
        }
        
        registerIndex_[name] = registers_.size();
        registers_.push_back({name, size, true});
        
        consume(TokenType::Semicolon, "Expected ';' after qubit declaration");
    }

    /**
     * @brief Parse: bit[n] name; or bit name;
     */
    void parseBitDeclaration() {
        size_t size = 1;
        
        if (match(TokenType::LeftBracket)) {
            size = parseIntegerLiteral("bit array size");
            consume(TokenType::RightBracket, "Expected ']' after bit size");
        }
        
        if (!check(TokenType::Identifier)) {
            errorAtCurrent("Expected register name after 'bit'");
            synchronize();
            return;
        }
        
        std::string name = current_.lexeme();
        advance();
        
        // Check for duplicate
        if (registerIndex_.count(name) > 0) {
            errorAtPrevious("Register '" + name + "' already declared");
            synchronize();
            return;
        }
        
        registerIndex_[name] = registers_.size();
        registers_.push_back({name, size, false});
        
        consume(TokenType::Semicolon, "Expected ';' after bit declaration");
    }

    /**
     * @brief Parse gate application: h q[0]; cx q[0], q[1]; rz(pi/4) q[0];
     */
    void parseGateApplication() {
        Token gateToken = current_;
        ir::GateType gateType = tokenToGateType(gateToken.type());
        advance();
        
        std::optional<double> parameter;
        
        // Parse optional parameter for rotation gates
        if (gateToken.isParameterizedGate()) {
            consume(TokenType::LeftParen, "Expected '(' for gate parameter");
            parameter = parseExpression();
            consume(TokenType::RightParen, "Expected ')' after gate parameter");
        }
        
        // Parse qubit operands
        std::vector<std::pair<std::string, size_t>> qubits;
        qubits.push_back(parseQubitOperand());
        
        // Two-qubit gates need a second operand
        if (gateToken.isTwoQubitGate()) {
            consume(TokenType::Comma, "Expected ',' between qubit operands");
            qubits.push_back(parseQubitOperand());
        }
        
        consume(TokenType::Semicolon, "Expected ';' after gate application");
        
        if (!hadError_) {
            gates_.push_back({gateType, std::move(qubits), parameter});
        }
    }

    /**
     * @brief Parse a qubit operand: q[0] or q
     */
    std::pair<std::string, size_t> parseQubitOperand() {
        if (!check(TokenType::Identifier)) {
            errorAtCurrent("Expected qubit register name");
            return {"", 0};
        }
        
        std::string regName = current_.lexeme();
        advance();
        
        size_t index = 0;
        if (match(TokenType::LeftBracket)) {
            index = parseIntegerLiteral("qubit index");
            consume(TokenType::RightBracket, "Expected ']' after qubit index");
        }
        
        return {regName, index};
    }

    /**
     * @brief Parse measurement assignment: c[0] = measure q[0]; or c = measure q;
     */
    void parseMeasurementOrAssignment() {
        std::string targetReg = current_.lexeme();
        advance();
        
        size_t targetIndex = 0;
        if (match(TokenType::LeftBracket)) {
            targetIndex = parseIntegerLiteral("bit index");
            consume(TokenType::RightBracket, "Expected ']' after bit index");
        }
        
        consume(TokenType::Equals, "Expected '=' in measurement assignment");
        consume(TokenType::Measure, "Expected 'measure' after '='");
        
        if (!check(TokenType::Identifier)) {
            errorAtCurrent("Expected qubit register name after 'measure'");
            synchronize();
            return;
        }
        
        std::string sourceReg = current_.lexeme();
        advance();
        
        size_t sourceIndex = 0;
        if (match(TokenType::LeftBracket)) {
            sourceIndex = parseIntegerLiteral("qubit index");
            consume(TokenType::RightBracket, "Expected ']' after qubit index");
        }
        
        consume(TokenType::Semicolon, "Expected ';' after measurement");
        
        if (!hadError_) {
            measurements_.push_back({{targetReg, targetIndex}, {sourceReg, sourceIndex}});
        }
    }

    /**
     * @brief Parse standalone measure: measure q[0];
     */
    void parseStandaloneMeasure() {
        if (!check(TokenType::Identifier)) {
            errorAtCurrent("Expected qubit register name after 'measure'");
            synchronize();
            return;
        }
        
        // We parse but don't store standalone measurements
        // (result is discarded in our model)
        advance();  // register name
        
        if (match(TokenType::LeftBracket)) {
            parseIntegerLiteral("qubit index");
            consume(TokenType::RightBracket, "Expected ']' after qubit index");
        }
        
        consume(TokenType::Semicolon, "Expected ';' after measurement");
        
        warn(previous_, "Standalone measure discards result (use 'c = measure q')");
    }

    /**
     * @brief Parse an integer literal.
     */
    size_t parseIntegerLiteral(const std::string& context) {
        if (!check(TokenType::Integer)) {
            errorAtCurrent("Expected integer for " + context);
            return 0;
        }
        
        size_t value = static_cast<size_t>(std::stoul(current_.lexeme()));
        advance();
        return value;
    }

    /**
     * @brief Parse an arithmetic expression for gate parameters.
     *
     * Supports: pi, numbers, +, -, *, /
     * Uses simple precedence climbing.
     */
    double parseExpression() {
        return parseAdditive();
    }

    double parseAdditive() {
        double left = parseMultiplicative();
        
        while (check(TokenType::Plus) || check(TokenType::Minus)) {
            TokenType op = current_.type();
            advance();
            double right = parseMultiplicative();
            
            if (op == TokenType::Plus) {
                left = left + right;
            } else {
                left = left - right;
            }
        }
        
        return left;
    }

    double parseMultiplicative() {
        double left = parseUnary();
        
        while (check(TokenType::Star) || check(TokenType::Slash)) {
            TokenType op = current_.type();
            advance();
            double right = parseUnary();
            
            if (op == TokenType::Star) {
                left = left * right;
            } else {
                if (right == 0.0) {
                    errorAtPrevious("Division by zero in gate parameter");
                    return 0.0;
                }
                left = left / right;
            }
        }
        
        return left;
    }

    double parseUnary() {
        if (match(TokenType::Minus)) {
            return -parseUnary();
        }
        if (match(TokenType::Plus)) {
            return parseUnary();
        }
        return parsePrimary();
    }

    double parsePrimary() {
        if (match(TokenType::Pi)) {
            return M_PI;
        }
        
        if (check(TokenType::Integer)) {
            double value = std::stod(current_.lexeme());
            advance();
            return value;
        }
        
        if (check(TokenType::Float)) {
            double value = std::stod(current_.lexeme());
            advance();
            return value;
        }
        
        if (match(TokenType::LeftParen)) {
            double value = parseExpression();
            consume(TokenType::RightParen, "Expected ')' after expression");
            return value;
        }
        
        errorAtCurrent("Expected number or 'pi' in expression");
        return 0.0;
    }

    // =========================================================================
    // Helper Functions
    // =========================================================================

    /**
     * @brief Convert a token type to an IR gate type.
     */
    [[nodiscard]] ir::GateType tokenToGateType(TokenType type) const noexcept {
        switch (type) {
            case TokenType::GateH:    return ir::GateType::H;
            case TokenType::GateX:    return ir::GateType::X;
            case TokenType::GateY:    return ir::GateType::Y;
            case TokenType::GateZ:    return ir::GateType::Z;
            case TokenType::GateS:    return ir::GateType::S;
            case TokenType::GateT:    return ir::GateType::T;
            case TokenType::GateSdg:  return ir::GateType::Sdg;
            case TokenType::GateTdg:  return ir::GateType::Tdg;
            case TokenType::GateRx:   return ir::GateType::Rx;
            case TokenType::GateRy:   return ir::GateType::Ry;
            case TokenType::GateRz:   return ir::GateType::Rz;
            case TokenType::GateCX:   return ir::GateType::CNOT;
            case TokenType::GateCZ:   return ir::GateType::CZ;
            case TokenType::GateSwap: return ir::GateType::SWAP;
            default:                  return ir::GateType::H;  // Should never happen
        }
    }

    /**
     * @brief Build the IR Circuit from parsed data.
     */
    [[nodiscard]] std::unique_ptr<ir::Circuit> buildCircuit() {
        // Calculate total qubit count
        size_t totalQubits = 0;
        std::unordered_map<std::string, size_t> qubitOffset;
        
        for (const auto& reg : registers_) {
            if (reg.isQubit) {
                qubitOffset[reg.name] = totalQubits;
                totalQubits += reg.size;
            }
        }
        
        if (totalQubits == 0) {
            // No qubits declared - create minimal circuit
            totalQubits = 1;
            warn(Token(), "No qubit declarations found, defaulting to 1 qubit");
        }
        
        auto circuit = std::make_unique<ir::Circuit>(totalQubits);
        
        // Add gates
        for (const auto& pg : gates_) {
            std::vector<QubitIndex> qubitIndices;
            
            for (const auto& [regName, idx] : pg.qubits) {
                auto it = qubitOffset.find(regName);
                if (it == qubitOffset.end()) {
                    // Error: undeclared register - should have been caught earlier
                    continue;
                }
                qubitIndices.push_back(static_cast<QubitIndex>(it->second + idx));
            }
            
            try {
                ir::Gate gate = createGate(pg.type, qubitIndices, pg.parameter);
                circuit->addGate(std::move(gate));
            } catch (const std::exception& e) {
                // Gate creation failed - add warning but continue
                warnings_.push_back(QASMError(
                    QASMErrorKind::Semantic,
                    std::string("Gate creation failed: ") + e.what(),
                    SourceLocation{}));
            }
        }
        
        return circuit;
    }

    /**
     * @brief Create an IR Gate from parsed data.
     */
    [[nodiscard]] ir::Gate createGate(
        ir::GateType type,
        const std::vector<QubitIndex>& qubits,
        std::optional<double> param) const {
        
        switch (type) {
            case ir::GateType::H:    return ir::Gate::h(qubits[0]);
            case ir::GateType::X:    return ir::Gate::x(qubits[0]);
            case ir::GateType::Y:    return ir::Gate::y(qubits[0]);
            case ir::GateType::Z:    return ir::Gate::z(qubits[0]);
            case ir::GateType::S:    return ir::Gate::s(qubits[0]);
            case ir::GateType::Sdg:  return ir::Gate::sdg(qubits[0]);
            case ir::GateType::T:    return ir::Gate::t(qubits[0]);
            case ir::GateType::Tdg:  return ir::Gate::tdg(qubits[0]);
            case ir::GateType::Rx:   return ir::Gate::rx(qubits[0], param.value_or(0.0));
            case ir::GateType::Ry:   return ir::Gate::ry(qubits[0], param.value_or(0.0));
            case ir::GateType::Rz:   return ir::Gate::rz(qubits[0], param.value_or(0.0));
            case ir::GateType::CNOT: return ir::Gate::cnot(qubits[0], qubits[1]);
            case ir::GateType::CZ:   return ir::Gate::cz(qubits[0], qubits[1]);
            case ir::GateType::SWAP: return ir::Gate::swap(qubits[0], qubits[1]);
        }
        
        // Should never reach here
        return ir::Gate::h(0);
    }
};

/**
 * @brief Convenience function to parse OpenQASM source.
 * @param source The OpenQASM 3.0 source code
 * @return The parsed circuit
 * @throws QASMParseException if parsing fails
 */
[[nodiscard]] inline std::unique_ptr<ir::Circuit> parseQASM(std::string_view source) {
    Parser parser(source);
    auto result = parser.parse();
    return std::move(result.circuit);
}

}  // namespace qopt::parser
