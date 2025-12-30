// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Router.hpp
 * @brief Abstract base class for qubit routing algorithms
 *
 * Provides the Router interface and RoutingResult container for mapping
 * logical qubits to physical qubits on a constrained topology.
 *
 * Routing is necessary because most quantum circuits assume all-to-all
 * qubit connectivity, but physical devices have limited connectivity.
 * The router inserts SWAP gates to move qubit states so that two-qubit
 * gates can be executed on adjacent physical qubits.
 *
 * @see Topology.hpp for device connectivity
 * @see SabreRouter.hpp for SABRE implementation
 */

#pragma once

#include "../ir/Circuit.hpp"
#include "Topology.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace qopt::routing {

/**
 * @brief Result container for qubit routing.
 *
 * Contains the routed circuit with SWAP gates inserted, the initial
 * and final qubit mappings, and routing statistics.
 */
struct RoutingResult {
    /// @brief The routed circuit with SWAPs inserted
    ir::Circuit routed_circuit;

    /// @brief Initial mapping: initial_mapping[logical] = physical
    std::vector<std::size_t> initial_mapping;

    /// @brief Final mapping: final_mapping[logical] = physical
    std::vector<std::size_t> final_mapping;

    /// @brief Number of SWAP gates inserted
    std::size_t swaps_inserted = 0;

    /// @brief Original circuit depth before routing
    std::size_t original_depth = 0;

    /// @brief Final circuit depth after routing
    std::size_t final_depth = 0;

    /// @brief Depth overhead from routing (final_depth - original_depth)
    [[nodiscard]] std::size_t depthOverhead() const noexcept {
        return final_depth > original_depth ? final_depth - original_depth : 0;
    }

    /// @brief Gate count overhead (3 * swaps_inserted for CNOT decomposition)
    [[nodiscard]] std::size_t gateOverhead() const noexcept {
        // Each SWAP = 3 CNOTs
        return swaps_inserted * 3;
    }

    /**
     * @brief Returns a string representation of the routing result.
     * @return Multi-line string with routing statistics
     */
    [[nodiscard]] std::string toString() const {
        std::string result = "RoutingResult:\n";
        result += "  SWAPs inserted: " + std::to_string(swaps_inserted) + "\n";
        result += "  Original depth: " + std::to_string(original_depth) + "\n";
        result += "  Final depth: " + std::to_string(final_depth) + "\n";
        result += "  Depth overhead: " + std::to_string(depthOverhead()) + "\n";
        result += "  Gate overhead: " + std::to_string(gateOverhead()) + "\n";
        result += "  Initial mapping: [";
        for (std::size_t i = 0; i < initial_mapping.size(); ++i) {
            if (i > 0) result += ", ";
            result += std::to_string(i) + "->" + std::to_string(initial_mapping[i]);
        }
        result += "]\n";
        result += "  Final mapping: [";
        for (std::size_t i = 0; i < final_mapping.size(); ++i) {
            if (i > 0) result += ", ";
            result += std::to_string(i) + "->" + std::to_string(final_mapping[i]);
        }
        result += "]";
        return result;
    }

    /**
     * @brief Constructs a RoutingResult with the given circuit.
     * @param circuit The routed circuit
     */
    explicit RoutingResult(ir::Circuit circuit)
        : routed_circuit(std::move(circuit))
    {}

    // Move semantics
    RoutingResult(RoutingResult&&) noexcept = default;
    RoutingResult& operator=(RoutingResult&&) noexcept = default;

    // Delete copy (circuits are move-only)
    RoutingResult(const RoutingResult&) = delete;
    RoutingResult& operator=(const RoutingResult&) = delete;
};

/**
 * @brief Abstract base class for qubit routing algorithms.
 *
 * A Router transforms a logical circuit (assuming all-to-all connectivity)
 * into a physical circuit that respects the device topology's connectivity
 * constraints.
 *
 * Derived classes implement specific routing algorithms:
 * - SabreRouter: SABRE heuristic routing (Li et al., 2019)
 * - TrivialRouter: Identity mapping (for testing)
 *
 * Example:
 * @code
 * Circuit logical(4);
 * logical.addGate(Gate::h(0));
 * logical.addGate(Gate::cnot(0, 3));  // Non-adjacent on linear topology
 *
 * SabreRouter router;
 * auto topology = Topology::linear(4);
 * RoutingResult result = router.route(logical, topology);
 *
 * // result.routed_circuit has SWAPs to make CNOT executable
 * @endcode
 */
class Router {
public:
    virtual ~Router() noexcept = default;

    // Non-copyable, movable
    Router(const Router&) = delete;
    Router& operator=(const Router&) = delete;
    Router(Router&&) noexcept = default;
    Router& operator=(Router&&) noexcept = default;

    /**
     * @brief Returns the router name for logging and debugging.
     * @return Router algorithm name
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /**
     * @brief Routes a logical circuit to a physical topology.
     *
     * Maps logical qubits to physical qubits and inserts SWAP gates
     * as needed so that all two-qubit gates operate on adjacent
     * physical qubits.
     *
     * @param circuit The logical circuit to route
     * @param topology The physical device topology
     * @return RoutingResult containing the routed circuit and statistics
     * @throws std::invalid_argument if circuit has more qubits than topology
     */
    [[nodiscard]] virtual RoutingResult route(
        const ir::Circuit& circuit,
        const Topology& topology) = 0;

protected:
    Router() = default;  // Only constructible via derived classes

    /**
     * @brief Validates that a circuit can be routed on a topology.
     * @param circuit The circuit to validate
     * @param topology The target topology
     * @throws std::invalid_argument if circuit is incompatible
     */
    static void validateRouteInputs(const ir::Circuit& circuit,
                                     const Topology& topology) {
        if (circuit.numQubits() > topology.numQubits()) {
            throw std::invalid_argument(
                "Circuit has " + std::to_string(circuit.numQubits()) +
                " qubits but topology only has " +
                std::to_string(topology.numQubits()) + " qubits");
        }
    }

    /**
     * @brief Creates an identity mapping (logical[i] -> physical[i]).
     * @param num_qubits Number of qubits
     * @return Identity mapping vector
     */
    [[nodiscard]] static std::vector<std::size_t> identityMapping(
        std::size_t num_qubits) {
        std::vector<std::size_t> mapping(num_qubits);
        for (std::size_t i = 0; i < num_qubits; ++i) {
            mapping[i] = i;
        }
        return mapping;
    }
};

/**
 * @brief Trivial router that uses identity mapping.
 *
 * This router performs no routing - it assumes the circuit already
 * respects topology constraints or that all-to-all connectivity exists.
 * Useful for testing or as a baseline.
 */
class TrivialRouter : public Router {
public:
    TrivialRouter() = default;

    [[nodiscard]] std::string name() const override {
        return "TrivialRouter";
    }

    [[nodiscard]] RoutingResult route(
        const ir::Circuit& circuit,
        const Topology& topology) override {
        validateRouteInputs(circuit, topology);

        RoutingResult result(circuit.clone());
        result.initial_mapping = identityMapping(circuit.numQubits());
        result.final_mapping = result.initial_mapping;
        result.swaps_inserted = 0;
        result.original_depth = circuit.depth();
        result.final_depth = result.routed_circuit.depth();

        return result;
    }
};

}  // namespace qopt::routing
