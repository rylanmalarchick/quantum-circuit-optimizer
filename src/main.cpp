// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file main.cpp
 * @brief Quantum Circuit Optimizer CLI
 *
 * Command-line interface for the quantum circuit optimizer. Supports
 * OpenQASM 3.0 input, multiple optimization passes, topology-aware
 * routing, and JSON/QASM output.
 *
 * Usage:
 *   qopt --input circuit.qasm [OPTIONS]
 *
 * Options:
 *   --input <file>       Input OpenQASM file (required)
 *   --output <file>      Output file (default: stdout)
 *   --passes <list>      Comma-separated pass names (default: all)
 *   --topology <name>    Target topology for routing
 *   --route              Enable SABRE routing after optimization
 *   --output-format      Output format: json or qasm (default: json)
 *   --help               Show usage information
 *
 * Pass names: cancel, commute, rotate, identity
 * Topology names: linear-N, ring-N, grid-RxC, heavy-hex-D, iqm-garnet
 */

#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"
#include "parser/Parser.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/CommutationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"
#include "routing/Topology.hpp"
#include "routing/SabreRouter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// =============================================================================
// JSON Utilities (minimal implementation, no external dependencies)
// =============================================================================

/**
 * @brief Escapes a string for JSON output.
 */
std::string jsonEscape(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Control character - escape as unicode
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

/**
 * @brief Simple JSON builder for output formatting.
 */
class JsonBuilder {
public:
    void beginObject() { comma(); newline(); buffer_ << "{"; needComma_ = false; indent_++; }
    void endObject() { indent_--; newline(); buffer_ << "}"; needComma_ = true; }
    void beginArray() { buffer_ << "["; needComma_ = false; indent_++; }
    void endArray() { indent_--; newline(); buffer_ << "]"; needComma_ = true; }

    void key(const std::string& k) {
        comma();
        newline();
        buffer_ << "\"" << jsonEscape(k) << "\": ";
        needComma_ = false;
    }

    void value(const std::string& v) {
        comma();
        buffer_ << "\"" << jsonEscape(v) << "\"";
        needComma_ = true;
    }

    void value(std::size_t v) {
        comma();
        buffer_ << v;
        needComma_ = true;
    }

    void value(std::ptrdiff_t v) {
        comma();
        buffer_ << v;
        needComma_ = true;
    }

    void value(double v) {
        comma();
        buffer_ << std::fixed << std::setprecision(6) << v;
        needComma_ = true;
    }

    void nullValue() {
        comma();
        buffer_ << "null";
        needComma_ = true;
    }

    std::string str() const { return buffer_.str(); }

private:
    std::ostringstream buffer_;
    int indent_ = 0;
    bool needComma_ = false;

    void comma() {
        if (needComma_) buffer_ << ",";
    }

    void newline() {
        buffer_ << "\n";
        for (int i = 0; i < indent_; ++i) buffer_ << "  ";
    }
};

// =============================================================================
// OpenQASM 3.0 Emitter
// =============================================================================

/**
 * @brief Converts a gate type to its OpenQASM 3.0 name.
 */
std::string gateToQasmName(qopt::ir::GateType type) {
    switch (type) {
        case qopt::ir::GateType::H:    return "h";
        case qopt::ir::GateType::X:    return "x";
        case qopt::ir::GateType::Y:    return "y";
        case qopt::ir::GateType::Z:    return "z";
        case qopt::ir::GateType::S:    return "s";
        case qopt::ir::GateType::Sdg:  return "sdg";
        case qopt::ir::GateType::T:    return "t";
        case qopt::ir::GateType::Tdg:  return "tdg";
        case qopt::ir::GateType::Rx:   return "rx";
        case qopt::ir::GateType::Ry:   return "ry";
        case qopt::ir::GateType::Rz:   return "rz";
        case qopt::ir::GateType::CNOT: return "cx";
        case qopt::ir::GateType::CZ:   return "cz";
        case qopt::ir::GateType::SWAP: return "swap";
    }
    return "unknown";
}

/**
 * @brief Emits a Circuit as OpenQASM 3.0 source code.
 */
std::string emitQASM(const qopt::ir::Circuit& circuit) {
    std::ostringstream oss;

    // Header
    oss << "OPENQASM 3.0;\n";
    oss << "include \"stdgates.inc\";\n\n";

    // Qubit declaration
    oss << "qubit[" << circuit.numQubits() << "] q;\n\n";

    // Gates
    for (const auto& gate : circuit) {
        std::string name = gateToQasmName(gate.type());

        // Handle parameterized gates
        if (gate.isParameterized()) {
            oss << name << "(" << std::fixed << std::setprecision(10)
                << gate.parameter().value() << ") ";
        } else {
            oss << name << " ";
        }

        // Qubit operands
        const auto& qubits = gate.qubits();
        for (std::size_t i = 0; i < qubits.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "q[" << qubits[i] << "]";
        }
        oss << ";\n";
    }

    return oss.str();
}

// =============================================================================
// IQM Garnet Topology
// =============================================================================

/**
 * @brief Creates the IQM Garnet 20-qubit topology.
 *
 * The IQM Garnet is a 20-qubit superconducting quantum processor with
 * a grid-like connectivity pattern. Reference: arXiv:2408.12433
 *
 * @return Topology with 20 qubits and 30 edges
 */
qopt::routing::Topology iqmGarnet() {
    qopt::routing::Topology topology(20);

    // 30 edges defining the IQM Garnet coupling map
    const std::vector<std::pair<std::size_t, std::size_t>> edges = {
        {0, 1}, {0, 3}, {1, 4}, {2, 3}, {2, 7},
        {3, 4}, {3, 8}, {4, 5}, {4, 9}, {5, 6},
        {5, 10}, {6, 11}, {7, 8}, {7, 12}, {8, 9},
        {8, 13}, {9, 10}, {9, 14}, {10, 11}, {10, 15},
        {11, 16}, {12, 13}, {13, 14}, {13, 17}, {14, 15},
        {14, 18}, {15, 16}, {15, 19}, {17, 18}, {18, 19},
    };

    for (const auto& [q1, q2] : edges) {
        topology.addEdge(q1, q2);
    }

    return topology;
}

// =============================================================================
// Topology Parsing
// =============================================================================

/**
 * @brief Parses a topology specification string.
 *
 * Formats:
 *   - linear-N: Linear chain of N qubits
 *   - ring-N: Ring of N qubits
 *   - grid-RxC: R rows by C columns grid
 *   - heavy-hex-D: Heavy-hex with distance D
 *   - iqm-garnet: IQM Garnet 20-qubit topology
 *
 * @param spec Topology specification string
 * @return Optional topology (nullopt on parse error)
 */
std::optional<qopt::routing::Topology> parseTopology(const std::string& spec) {
    using qopt::routing::Topology;

    if (spec == "iqm-garnet") {
        return iqmGarnet();
    }

    // Parse "linear-N"
    if (spec.rfind("linear-", 0) == 0) {
        try {
            std::size_t n = std::stoull(spec.substr(7));
            if (n == 0) return std::nullopt;
            return Topology::linear(n);
        } catch (...) {
            return std::nullopt;
        }
    }

    // Parse "ring-N"
    if (spec.rfind("ring-", 0) == 0) {
        try {
            std::size_t n = std::stoull(spec.substr(5));
            if (n < 2) return std::nullopt;
            return Topology::ring(n);
        } catch (...) {
            return std::nullopt;
        }
    }

    // Parse "grid-RxC"
    if (spec.rfind("grid-", 0) == 0) {
        std::string dims = spec.substr(5);
        auto xpos = dims.find('x');
        if (xpos == std::string::npos) return std::nullopt;
        try {
            std::size_t rows = std::stoull(dims.substr(0, xpos));
            std::size_t cols = std::stoull(dims.substr(xpos + 1));
            if (rows == 0 || cols == 0) return std::nullopt;
            return Topology::grid(rows, cols);
        } catch (...) {
            return std::nullopt;
        }
    }

    // Parse "heavy-hex-D"
    if (spec.rfind("heavy-hex-", 0) == 0) {
        try {
            std::size_t d = std::stoull(spec.substr(10));
            if (d == 0) return std::nullopt;
            return Topology::heavyHex(d);
        } catch (...) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

// =============================================================================
// CLI Argument Parsing
// =============================================================================

struct CLIOptions {
    std::string input_file;
    std::string output_file;  // Empty means stdout
    std::vector<std::string> passes;
    std::string topology_spec;
    bool route = false;
    std::string output_format = "json";
    bool show_help = false;
    bool has_error = false;
    std::string error_message;
};

void printUsage(const char* prog_name) {
    std::cerr << "Quantum Circuit Optimizer (qopt)\n";
    std::cerr << "Usage: " << prog_name << " --input <file> [OPTIONS]\n\n";
    std::cerr << "Options:\n";
    std::cerr << "  --input <file>       Input OpenQASM 3.0 file (required)\n";
    std::cerr << "  --output <file>      Output file (default: stdout)\n";
    std::cerr << "  --passes <list>      Comma-separated optimization passes\n";
    std::cerr << "                       Available: cancel, commute, rotate, identity\n";
    std::cerr << "                       Default: all passes in order\n";
    std::cerr << "  --topology <spec>    Target topology for routing\n";
    std::cerr << "                       Formats: linear-N, ring-N, grid-RxC,\n";
    std::cerr << "                                heavy-hex-D, iqm-garnet\n";
    std::cerr << "  --route              Enable SABRE routing after optimization\n";
    std::cerr << "  --output-format <f>  Output format: json or qasm (default: json)\n";
    std::cerr << "  --help               Show this help message\n\n";
    std::cerr << "Examples:\n";
    std::cerr << "  " << prog_name << " --input circuit.qasm --output result.json\n";
    std::cerr << "  " << prog_name << " --input circuit.qasm --passes cancel,rotate --route --topology iqm-garnet\n";
    std::cerr << "  " << prog_name << " --input circuit.qasm --output-format qasm\n";
}

std::vector<std::string> splitPasses(const std::string& passes_str) {
    std::vector<std::string> result;
    std::istringstream iss(passes_str);
    std::string pass;
    while (std::getline(iss, pass, ',')) {
        // Trim whitespace
        auto start = pass.find_first_not_of(" \t");
        auto end = pass.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(pass.substr(start, end - start + 1));
        }
    }
    return result;
}

CLIOptions parseArgs(int argc, char* argv[]) {
    CLIOptions opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            opts.show_help = true;
            return opts;
        }

        if (arg == "--input" || arg == "-i") {
            if (i + 1 >= argc) {
                opts.has_error = true;
                opts.error_message = "--input requires a file path";
                return opts;
            }
            opts.input_file = argv[++i];
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 >= argc) {
                opts.has_error = true;
                opts.error_message = "--output requires a file path";
                return opts;
            }
            opts.output_file = argv[++i];
        } else if (arg == "--passes" || arg == "-p") {
            if (i + 1 >= argc) {
                opts.has_error = true;
                opts.error_message = "--passes requires a comma-separated list";
                return opts;
            }
            opts.passes = splitPasses(argv[++i]);
        } else if (arg == "--topology" || arg == "-t") {
            if (i + 1 >= argc) {
                opts.has_error = true;
                opts.error_message = "--topology requires a specification";
                return opts;
            }
            opts.topology_spec = argv[++i];
        } else if (arg == "--route" || arg == "-r") {
            opts.route = true;
        } else if (arg == "--output-format" || arg == "-f") {
            if (i + 1 >= argc) {
                opts.has_error = true;
                opts.error_message = "--output-format requires 'json' or 'qasm'";
                return opts;
            }
            opts.output_format = argv[++i];
            if (opts.output_format != "json" && opts.output_format != "qasm") {
                opts.has_error = true;
                opts.error_message = "Invalid output format: " + opts.output_format;
                return opts;
            }
        } else {
            opts.has_error = true;
            opts.error_message = "Unknown option: " + arg;
            return opts;
        }
    }

    // Validate required options
    if (opts.input_file.empty()) {
        opts.has_error = true;
        opts.error_message = "Missing required --input option";
    }

    // Validate routing requires topology
    if (opts.route && opts.topology_spec.empty()) {
        opts.has_error = true;
        opts.error_message = "--route requires --topology";
    }

    return opts;
}

// =============================================================================
// Pass Creation
// =============================================================================

std::unique_ptr<qopt::passes::Pass> createPass(const std::string& name) {
    if (name == "cancel" || name == "cancellation") {
        return std::make_unique<qopt::passes::CancellationPass>();
    }
    if (name == "commute" || name == "commutation") {
        return std::make_unique<qopt::passes::CommutationPass>();
    }
    if (name == "rotate" || name == "rotation") {
        return std::make_unique<qopt::passes::RotationMergePass>();
    }
    if (name == "identity") {
        return std::make_unique<qopt::passes::IdentityEliminationPass>();
    }
    return nullptr;
}

std::vector<std::unique_ptr<qopt::passes::Pass>> createDefaultPasses() {
    std::vector<std::unique_ptr<qopt::passes::Pass>> passes;
    passes.push_back(std::make_unique<qopt::passes::CancellationPass>());
    passes.push_back(std::make_unique<qopt::passes::CommutationPass>());
    passes.push_back(std::make_unique<qopt::passes::RotationMergePass>());
    passes.push_back(std::make_unique<qopt::passes::IdentityEliminationPass>());
    return passes;
}

// =============================================================================
// Extended Statistics Tracking
// =============================================================================

/**
 * @brief Extended per-pass statistics including depth information.
 */
struct ExtendedPassStats {
    std::string name;
    std::size_t gates_removed;
    std::size_t gates_added;
    std::size_t output_gates;
    std::size_t output_depth;
};

/**
 * @brief Runs passes one at a time to capture intermediate statistics.
 */
std::vector<ExtendedPassStats> runPassesWithStats(
    qopt::ir::Circuit& circuit,
    std::vector<std::unique_ptr<qopt::passes::Pass>>& passes)
{
    std::vector<ExtendedPassStats> stats;

    for (auto& pass : passes) {
        qopt::passes::PassManager pm;
        pm.addPass(std::move(pass));
        pm.run(circuit);

        const auto& pm_stats = pm.statistics();
        ExtendedPassStats es;

        if (!pm_stats.per_pass.empty()) {
            es.name = std::get<0>(pm_stats.per_pass[0]);
            es.gates_removed = std::get<1>(pm_stats.per_pass[0]);
            es.gates_added = std::get<2>(pm_stats.per_pass[0]);
        } else {
            es.name = "Unknown";
            es.gates_removed = 0;
            es.gates_added = 0;
        }

        es.output_gates = circuit.numGates();
        es.output_depth = circuit.depth();

        stats.push_back(es);
    }

    return stats;
}

// =============================================================================
// Main Entry Point
// =============================================================================

}  // anonymous namespace

int main(int argc, char* argv[]) {
    // Parse command-line arguments
    CLIOptions opts = parseArgs(argc, argv);

    if (opts.show_help) {
        printUsage(argv[0]);
        return 0;
    }

    if (opts.has_error) {
        std::cerr << "Error: " << opts.error_message << "\n";
        std::cerr << "Use --help for usage information.\n";
        return 1;
    }

    // Read input file
    std::ifstream input_stream(opts.input_file);
    if (!input_stream) {
        std::cerr << "Error: Cannot open input file: " << opts.input_file << "\n";
        return 1;
    }

    std::ostringstream source_buffer;
    source_buffer << input_stream.rdbuf();
    std::string source = source_buffer.str();
    input_stream.close();

    // Parse OpenQASM
    std::unique_ptr<qopt::ir::Circuit> circuit;
    try {
        circuit = qopt::parser::parseQASM(source);
    } catch (const qopt::parser::QASMParseException& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing QASM: " << e.what() << "\n";
        return 1;
    }

    if (!circuit) {
        std::cerr << "Error: Failed to parse circuit\n";
        return 1;
    }

    // Record input statistics
    std::size_t input_gates = circuit->numGates();
    std::size_t input_depth = circuit->depth();
    std::size_t input_qubits = circuit->numQubits();
    std::size_t input_two_qubit_gates = circuit->countTwoQubitGates();

    // Create optimization passes
    std::vector<std::unique_ptr<qopt::passes::Pass>> passes;
    if (opts.passes.empty()) {
        passes = createDefaultPasses();
    } else {
        for (const auto& pass_name : opts.passes) {
            auto pass = createPass(pass_name);
            if (!pass) {
                std::cerr << "Error: Unknown pass: " << pass_name << "\n";
                std::cerr << "Available passes: cancel, commute, rotate, identity\n";
                return 1;
            }
            passes.push_back(std::move(pass));
        }
    }

    // Run optimization passes with per-pass stats
    auto pass_stats = runPassesWithStats(*circuit, passes);

    // Record post-optimization statistics
    std::size_t post_opt_gates = circuit->numGates();
    std::size_t post_opt_depth = circuit->depth();
    std::size_t post_opt_qubits = circuit->numQubits();
    std::size_t post_opt_two_qubit_gates = circuit->countTwoQubitGates();

    // Routing (optional)
    std::optional<qopt::routing::RoutingResult> routing_result;
    std::string topology_name;

    if (opts.route) {
        auto topology_opt = parseTopology(opts.topology_spec);
        if (!topology_opt) {
            std::cerr << "Error: Invalid topology specification: " << opts.topology_spec << "\n";
            return 1;
        }

        auto& topology = *topology_opt;
        topology_name = opts.topology_spec;

        // Check that circuit fits on topology
        if (circuit->numQubits() > topology.numQubits()) {
            std::cerr << "Error: Circuit has " << circuit->numQubits()
                      << " qubits but topology only has " << topology.numQubits() << "\n";
            return 1;
        }

        qopt::routing::SabreRouter router;
        routing_result = router.route(*circuit, topology);

        // Replace circuit with routed version
        circuit = std::make_unique<qopt::ir::Circuit>(
            std::move(routing_result->routed_circuit));
    }

    // Generate output QASM
    std::string output_qasm = emitQASM(*circuit);

    // Build output
    std::string output;
    if (opts.output_format == "json") {
        JsonBuilder json;
        json.beginObject();

        // Input statistics
        json.key("input");
        json.beginObject();
        json.key("gates"); json.value(input_gates);
        json.key("depth"); json.value(input_depth);
        json.key("qubits"); json.value(input_qubits);
        json.key("two_qubit_gates"); json.value(input_two_qubit_gates);
        json.endObject();

        // Per-pass statistics
        json.key("passes");
        json.beginArray();
        for (const auto& ps : pass_stats) {
            json.beginObject();
            json.key("name"); json.value(ps.name);
            json.key("gates_removed"); json.value(ps.gates_removed);
            json.key("gates_added"); json.value(ps.gates_added);
            json.key("output_gates"); json.value(ps.output_gates);
            json.key("output_depth"); json.value(ps.output_depth);
            json.endObject();
        }
        json.endArray();

        // Post-optimization statistics
        json.key("post_optimization");
        json.beginObject();
        json.key("gates"); json.value(post_opt_gates);
        json.key("depth"); json.value(post_opt_depth);
        json.key("qubits"); json.value(post_opt_qubits);
        json.key("two_qubit_gates"); json.value(post_opt_two_qubit_gates);
        json.endObject();

        // Routing statistics (if applicable)
        json.key("routing");
        if (routing_result) {
            json.beginObject();
            json.key("topology"); json.value(topology_name);
            json.key("swaps_inserted"); json.value(routing_result->swaps_inserted);
            json.key("final_gates"); json.value(circuit->numGates());
            json.key("final_depth"); json.value(circuit->depth());
            json.endObject();
        } else {
            json.nullValue();
        }

        // Output QASM
        json.key("output_qasm"); json.value(output_qasm);

        json.endObject();
        output = json.str() + "\n";
    } else {
        // QASM output format
        output = output_qasm;
    }

    // Write output
    if (opts.output_file.empty()) {
        std::cout << output;
    } else {
        std::ofstream output_stream(opts.output_file);
        if (!output_stream) {
            std::cerr << "Error: Cannot open output file: " << opts.output_file << "\n";
            return 1;
        }
        output_stream << output;
        output_stream.close();

        // Print summary to stderr when writing to file
        std::cerr << "Optimization complete:\n";
        std::cerr << "  Input: " << input_gates << " gates, depth " << input_depth << "\n";
        std::cerr << "  Output: " << circuit->numGates() << " gates, depth "
                  << circuit->depth() << "\n";
        if (routing_result) {
            std::cerr << "  Routing: " << routing_result->swaps_inserted << " SWAPs inserted\n";
        }
        std::cerr << "  Written to: " << opts.output_file << "\n";
    }

    return 0;
}
