// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file DAG.hpp
 * @brief Directed Acyclic Graph representation for quantum circuits
 *
 * Provides the DAGNode and DAG classes for representing quantum circuits
 * as dependency graphs. This representation enables efficient optimization
 * passes by making gate dependencies explicit.
 *
 * The DAG structure represents:
 * - Nodes: quantum gates
 * - Edges: dependencies between gates (qubit wire connections)
 *
 * @see Circuit.hpp for linear circuit representation
 * @see Gate.hpp for gate representation
 */

#pragma once

#include "Circuit.hpp"
#include "Gate.hpp"
#include "Types.hpp"

#include <algorithm>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace qopt::ir {

// Forward declaration
class DAG;

/**
 * @brief A node in the circuit DAG, wrapping a Gate with dependency tracking.
 *
 * Each DAGNode contains a gate and maintains lists of predecessor and
 * successor nodes. Predecessors are gates that must execute before this gate;
 * successors are gates that depend on this gate's output.
 *
 * DAGNodes are owned by the DAG and should not be created directly.
 */
class DAGNode {
public:
    /**
     * @brief Constructs a DAGNode wrapping the given gate.
     * @param gate The gate this node represents
     */
    explicit DAGNode(Gate gate)
        : gate_(std::move(gate))
    {}

    // Move-only (nodes are managed by DAG)
    DAGNode(DAGNode&&) noexcept = default;
    DAGNode& operator=(DAGNode&&) noexcept = default;
    DAGNode(const DAGNode&) = delete;
    DAGNode& operator=(const DAGNode&) = delete;
    ~DAGNode() noexcept = default;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /// @brief Returns the gate this node represents.
    [[nodiscard]] const Gate& gate() const noexcept { return gate_; }

    /// @brief Returns a mutable reference to the gate.
    [[nodiscard]] Gate& gate() noexcept { return gate_; }

    /// @brief Returns the gate ID (convenience accessor).
    [[nodiscard]] GateId id() const noexcept { return gate_.id(); }

    /// @brief Returns predecessor node IDs (gates that must execute before).
    [[nodiscard]] const std::vector<GateId>& predecessors() const noexcept {
        return predecessors_;
    }

    /// @brief Returns successor node IDs (gates that depend on this).
    [[nodiscard]] const std::vector<GateId>& successors() const noexcept {
        return successors_;
    }

    /// @brief Returns the number of predecessors.
    [[nodiscard]] std::size_t inDegree() const noexcept {
        return predecessors_.size();
    }

    /// @brief Returns the number of successors.
    [[nodiscard]] std::size_t outDegree() const noexcept {
        return successors_.size();
    }

    /// @brief Returns true if this node has no predecessors (input node).
    [[nodiscard]] bool isSource() const noexcept {
        return predecessors_.empty();
    }

    /// @brief Returns true if this node has no successors (output node).
    [[nodiscard]] bool isSink() const noexcept {
        return successors_.empty();
    }

private:
    friend class DAG;  // DAG manages edges

    Gate gate_;
    std::vector<GateId> predecessors_;
    std::vector<GateId> successors_;

    /// @brief Adds a predecessor to this node.
    void addPredecessor(GateId pred_id) {
        predecessors_.push_back(pred_id);
    }

    /// @brief Adds a successor to this node.
    void addSuccessor(GateId succ_id) {
        successors_.push_back(succ_id);
    }

    /// @brief Removes a predecessor from this node.
    void removePredecessor(GateId pred_id) {
        auto it = std::find(predecessors_.begin(), predecessors_.end(), pred_id);
        if (it != predecessors_.end()) {
            predecessors_.erase(it);
        }
    }

    /// @brief Removes a successor from this node.
    void removeSuccessor(GateId succ_id) {
        auto it = std::find(successors_.begin(), successors_.end(), succ_id);
        if (it != successors_.end()) {
            successors_.erase(it);
        }
    }
};

/**
 * @brief Directed Acyclic Graph representation of a quantum circuit.
 *
 * The DAG represents gate dependencies explicitly, enabling efficient
 * pattern matching and optimization. Nodes are gates, edges represent
 * qubit wire dependencies.
 *
 * Key features:
 * - Construct from Circuit (fromCircuit)
 * - Convert back to Circuit (toCircuit)
 * - Topological traversal
 * - Node addition/removal
 * - Dependency queries
 *
 * Example:
 * @code
 * Circuit circuit(2);
 * circuit.addGate(Gate::h(0));
 * circuit.addGate(Gate::cnot(0, 1));
 *
 * DAG dag = DAG::fromCircuit(circuit);
 * auto sorted = dag.topologicalOrder();
 * Circuit recovered = dag.toCircuit();
 * @endcode
 */
class DAG {
public:
    /**
     * @brief Constructs an empty DAG with the specified number of qubits.
     * @param num_qubits Number of qubits in the circuit
     * @throws std::invalid_argument if num_qubits is 0 or exceeds MAX_QUBITS
     */
    explicit DAG(std::size_t num_qubits)
        : num_qubits_(num_qubits)
        , next_gate_id_(0)
    {
        if (num_qubits == 0) {
            throw std::invalid_argument("DAG must have at least 1 qubit");
        }
        if (num_qubits > constants::MAX_QUBITS) {
            throw std::invalid_argument(
                "DAG exceeds maximum qubit count of " +
                std::to_string(constants::MAX_QUBITS));
        }
        // Initialize last gate on each qubit to INVALID
        last_gate_on_qubit_.resize(num_qubits, INVALID_GATE_ID);
    }

    // Move semantics
    DAG(DAG&&) noexcept = default;
    DAG& operator=(DAG&&) noexcept = default;

    // Delete copy (use clone() if needed)
    DAG(const DAG&) = delete;
    DAG& operator=(const DAG&) = delete;

    ~DAG() noexcept = default;

    // -------------------------------------------------------------------------
    // Factory Methods
    // -------------------------------------------------------------------------

    /**
     * @brief Constructs a DAG from a Circuit.
     *
     * Analyzes gate dependencies based on qubit usage and builds the
     * dependency graph. Gates are connected if they share a qubit.
     *
     * @param circuit The circuit to convert
     * @return DAG representation of the circuit
     */
    [[nodiscard]] static DAG fromCircuit(const Circuit& circuit) {
        DAG dag(circuit.numQubits());

        for (const auto& gate : circuit) {
            dag.addGate(gate);
        }

        return dag;
    }

    // -------------------------------------------------------------------------
    // Node Management
    // -------------------------------------------------------------------------

    /**
     * @brief Adds a gate to the DAG, automatically computing dependencies.
     *
     * The gate is connected to the last gate on each qubit it touches.
     *
     * @param gate The gate to add
     * @return The assigned gate ID
     * @throws std::out_of_range if gate references qubit beyond DAG size
     */
    GateId addGate(Gate gate) {
        validateGateQubits(gate);

        // Assign ID
        GateId id = next_gate_id_++;
        gate.setId(id);

        // Create node
        auto node = std::make_unique<DAGNode>(std::move(gate));

        // Connect to predecessors (last gate on each qubit this gate touches)
        for (auto q : node->gate().qubits()) {
            GateId pred_id = last_gate_on_qubit_[q];
            if (pred_id != INVALID_GATE_ID) {
                // Add edge: predecessor -> this node
                node->addPredecessor(pred_id);
                nodes_.at(pred_id)->addSuccessor(id);
            }
            // Update last gate on this qubit
            last_gate_on_qubit_[q] = id;
        }

        nodes_[id] = std::move(node);
        return id;
    }

    /**
     * @brief Returns the node with the given ID.
     * @param id The gate ID
     * @return Const reference to the node
     * @throws std::out_of_range if ID not found
     */
    [[nodiscard]] const DAGNode& node(GateId id) const {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) {
            throw std::out_of_range(
                "Node with ID " + std::to_string(id) + " not found");
        }
        return *it->second;
    }

    /**
     * @brief Returns a mutable reference to the node with the given ID.
     * @param id The gate ID
     * @return Mutable reference to the node
     * @throws std::out_of_range if ID not found
     */
    [[nodiscard]] DAGNode& node(GateId id) {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) {
            throw std::out_of_range(
                "Node with ID " + std::to_string(id) + " not found");
        }
        return *it->second;
    }

    /**
     * @brief Checks if a node with the given ID exists.
     * @param id The gate ID
     * @return true if node exists
     */
    [[nodiscard]] bool hasNode(GateId id) const noexcept {
        return nodes_.find(id) != nodes_.end();
    }

    /**
     * @brief Removes a node from the DAG.
     *
     * Reconnects predecessors directly to successors to maintain
     * dependency relationships.
     *
     * @param id The gate ID to remove
     * @throws std::out_of_range if ID not found
     */
    void removeNode(GateId id) {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) {
            throw std::out_of_range(
                "Cannot remove node with ID " + std::to_string(id) + ": not found");
        }

        DAGNode& target = *it->second;

        // Reconnect: each predecessor connects to each successor
        for (GateId pred_id : target.predecessors()) {
            nodes_.at(pred_id)->removeSuccessor(id);
            for (GateId succ_id : target.successors()) {
                // Avoid duplicates
                auto& pred_succs = nodes_.at(pred_id)->successors_;
                if (std::find(pred_succs.begin(), pred_succs.end(), succ_id) == pred_succs.end()) {
                    nodes_.at(pred_id)->addSuccessor(succ_id);
                }
            }
        }

        for (GateId succ_id : target.successors()) {
            nodes_.at(succ_id)->removePredecessor(id);
            for (GateId pred_id : target.predecessors()) {
                // Avoid duplicates
                auto& succ_preds = nodes_.at(succ_id)->predecessors_;
                if (std::find(succ_preds.begin(), succ_preds.end(), pred_id) == succ_preds.end()) {
                    nodes_.at(succ_id)->addPredecessor(pred_id);
                }
            }
        }

        // Update last_gate_on_qubit_ if this was the last gate on any qubit
        for (auto q : target.gate().qubits()) {
            if (last_gate_on_qubit_[q] == id) {
                // Find new last gate on this qubit (search predecessors)
                GateId new_last = INVALID_GATE_ID;
                for (GateId pred_id : target.predecessors()) {
                    const auto& pred_qubits = nodes_.at(pred_id)->gate().qubits();
                    if (std::find(pred_qubits.begin(), pred_qubits.end(), q) != pred_qubits.end()) {
                        new_last = pred_id;
                        break;
                    }
                }
                last_gate_on_qubit_[q] = new_last;
            }
        }

        nodes_.erase(it);
    }

    // -------------------------------------------------------------------------
    // DAG Properties
    // -------------------------------------------------------------------------

    /// @brief Returns the number of qubits.
    [[nodiscard]] std::size_t numQubits() const noexcept { return num_qubits_; }

    /// @brief Returns the number of nodes (gates).
    [[nodiscard]] std::size_t numNodes() const noexcept { return nodes_.size(); }

    /// @brief Returns true if the DAG has no nodes.
    [[nodiscard]] bool empty() const noexcept { return nodes_.empty(); }

    /**
     * @brief Returns all node IDs in the DAG.
     * @return Vector of gate IDs
     */
    [[nodiscard]] std::vector<GateId> nodeIds() const {
        std::vector<GateId> ids;
        ids.reserve(nodes_.size());
        for (const auto& [id, _] : nodes_) {
            ids.push_back(id);
        }
        return ids;
    }

    /**
     * @brief Returns IDs of source nodes (no predecessors).
     * @return Vector of source node IDs
     */
    [[nodiscard]] std::vector<GateId> sources() const {
        std::vector<GateId> result;
        for (const auto& [id, node] : nodes_) {
            if (node->isSource()) {
                result.push_back(id);
            }
        }
        return result;
    }

    /**
     * @brief Returns IDs of sink nodes (no successors).
     * @return Vector of sink node IDs
     */
    [[nodiscard]] std::vector<GateId> sinks() const {
        std::vector<GateId> result;
        for (const auto& [id, node] : nodes_) {
            if (node->isSink()) {
                result.push_back(id);
            }
        }
        return result;
    }

    // -------------------------------------------------------------------------
    // Traversal
    // -------------------------------------------------------------------------

    /**
     * @brief Returns nodes in topological order (Kahn's algorithm).
     *
     * A valid topological order ensures that for every edge (u, v),
     * node u appears before node v in the ordering.
     *
     * @return Vector of gate IDs in topological order
     * @throws std::logic_error if the graph contains a cycle (should not happen)
     */
    [[nodiscard]] std::vector<GateId> topologicalOrder() const {
        if (nodes_.empty()) {
            return {};
        }

        std::vector<GateId> result;
        result.reserve(nodes_.size());

        // Compute in-degrees
        std::unordered_map<GateId, std::size_t> in_degree;
        for (const auto& [id, node] : nodes_) {
            in_degree[id] = node->inDegree();
        }

        // Initialize queue with sources
        std::queue<GateId> ready;
        for (const auto& [id, degree] : in_degree) {
            if (degree == 0) {
                ready.push(id);
            }
        }

        while (!ready.empty()) {
            GateId current = ready.front();
            ready.pop();
            result.push_back(current);

            // Decrease in-degree of successors
            for (GateId succ_id : nodes_.at(current)->successors()) {
                if (--in_degree[succ_id] == 0) {
                    ready.push(succ_id);
                }
            }
        }

        if (result.size() != nodes_.size()) {
            throw std::logic_error(
                "DAG contains a cycle (internal error: should not happen)");
        }

        return result;
    }

    /**
     * @brief Returns nodes grouped by layers (parallel execution levels).
     *
     * Gates in the same layer can execute in parallel. Layer 0 contains
     * source nodes, layer 1 contains nodes whose predecessors are all
     * in layer 0, etc.
     *
     * @return Vector of layers, each containing gate IDs
     */
    [[nodiscard]] std::vector<std::vector<GateId>> layers() const {
        if (nodes_.empty()) {
            return {};
        }

        std::vector<std::vector<GateId>> result;
        std::unordered_map<GateId, std::size_t> in_degree;
        std::unordered_set<GateId> processed;

        // Initialize in-degrees
        for (const auto& [id, node] : nodes_) {
            in_degree[id] = node->inDegree();
        }

        // Process layer by layer
        while (processed.size() < nodes_.size()) {
            std::vector<GateId> layer;

            // Find all nodes with zero remaining in-degree
            for (const auto& [id, degree] : in_degree) {
                if (degree == 0 && processed.find(id) == processed.end()) {
                    layer.push_back(id);
                }
            }

            if (layer.empty()) {
                throw std::logic_error("DAG contains a cycle (internal error)");
            }

            // Mark as processed and update in-degrees
            for (GateId id : layer) {
                processed.insert(id);
                for (GateId succ_id : nodes_.at(id)->successors()) {
                    if (in_degree[succ_id] > 0) {
                        --in_degree[succ_id];
                    }
                }
            }

            result.push_back(std::move(layer));
        }

        return result;
    }

    /**
     * @brief Calculates the critical path length (DAG depth).
     * @return Maximum path length from any source to any sink
     */
    [[nodiscard]] std::size_t depth() const {
        return layers().size();
    }

    // -------------------------------------------------------------------------
    // Conversion
    // -------------------------------------------------------------------------

    /**
     * @brief Converts the DAG back to a Circuit.
     *
     * Gates are emitted in topological order, preserving dependencies.
     *
     * @return Circuit representation of the DAG
     */
    [[nodiscard]] Circuit toCircuit() const {
        Circuit circuit(num_qubits_);

        for (GateId id : topologicalOrder()) {
            // Create a copy of the gate (Circuit will assign new IDs)
            const Gate& g = nodes_.at(id)->gate();
            circuit.addGate(Gate(g.type(), g.qubits(), g.parameter()));
        }

        return circuit;
    }

    // -------------------------------------------------------------------------
    // Edge Queries
    // -------------------------------------------------------------------------

    /**
     * @brief Checks if there is a direct edge from one node to another.
     * @param from_id Source node ID
     * @param to_id Target node ID
     * @return true if edge exists
     */
    [[nodiscard]] bool hasEdge(GateId from_id, GateId to_id) const {
        if (!hasNode(from_id) || !hasNode(to_id)) {
            return false;
        }
        const auto& succs = nodes_.at(from_id)->successors();
        return std::find(succs.begin(), succs.end(), to_id) != succs.end();
    }

    /**
     * @brief Returns all edges in the DAG.
     * @return Vector of (from, to) pairs
     */
    [[nodiscard]] std::vector<std::pair<GateId, GateId>> edges() const {
        std::vector<std::pair<GateId, GateId>> result;
        for (const auto& [id, node] : nodes_) {
            for (GateId succ_id : node->successors()) {
                result.emplace_back(id, succ_id);
            }
        }
        return result;
    }

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /**
     * @brief Returns a string representation of the DAG.
     * @return Multi-line string showing nodes and edges
     */
    [[nodiscard]] std::string toString() const {
        std::string result = "DAG(" + std::to_string(num_qubits_) +
                             " qubits, " + std::to_string(nodes_.size()) +
                             " nodes, depth " + std::to_string(depth()) + "):\n";

        for (GateId id : topologicalOrder()) {
            const DAGNode& n = *nodes_.at(id);
            result += "  [" + std::to_string(id) + "] " +
                      n.gate().toString();

            if (!n.predecessors().empty()) {
                result += " <- {";
                for (std::size_t i = 0; i < n.predecessors().size(); ++i) {
                    if (i > 0) result += ", ";
                    result += std::to_string(n.predecessors()[i]);
                }
                result += "}";
            }
            result += "\n";
        }

        return result;
    }

    /**
     * @brief Clears all nodes from the DAG.
     */
    void clear() {
        nodes_.clear();
        std::fill(last_gate_on_qubit_.begin(), last_gate_on_qubit_.end(),
                  INVALID_GATE_ID);
        next_gate_id_ = 0;
    }

private:
    std::size_t num_qubits_;
    GateId next_gate_id_;
    std::unordered_map<GateId, std::unique_ptr<DAGNode>> nodes_;
    std::vector<GateId> last_gate_on_qubit_;  // Track last gate on each qubit

    /**
     * @brief Validates that a gate's qubits are within DAG bounds.
     * @param g The gate to validate
     * @throws std::out_of_range if any qubit index is invalid
     */
    void validateGateQubits(const Gate& g) const {
        for (auto q : g.qubits()) {
            if (q >= num_qubits_) {
                throw std::out_of_range(
                    "Gate " + std::string(gateTypeName(g.type())) +
                    " references qubit " + std::to_string(q) +
                    " but DAG only has " + std::to_string(num_qubits_) +
                    " qubits");
            }
        }
    }
};

/**
 * @brief Stream output operator for DAG.
 * @param os Output stream
 * @param dag The DAG to output
 * @return Reference to the output stream
 */
inline std::ostream& operator<<(std::ostream& os, const DAG& dag) {
    os << dag.toString();
    return os;
}

}  // namespace qopt::ir
