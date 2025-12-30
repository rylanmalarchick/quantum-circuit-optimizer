// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file Topology.hpp
 * @brief Hardware topology representation for qubit routing
 *
 * Provides the Topology class representing the physical qubit connectivity
 * of a quantum device. The topology defines which pairs of physical qubits
 * can directly execute two-qubit gates.
 *
 * Factory methods provide common topologies:
 * - Linear: qubits connected in a line
 * - Grid: 2D rectangular grid
 * - Heavy-Hex: IBM's heavy-hexagon topology
 *
 * @see Router.hpp for routing algorithms
 * @see SabreRouter.hpp for SABRE implementation
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace qopt::routing {

/**
 * @brief Represents the physical qubit connectivity of a quantum device.
 *
 * The topology is modeled as an undirected graph where:
 * - Nodes represent physical qubits
 * - Edges represent direct two-qubit gate connectivity
 *
 * Distances between qubits are computed using BFS and cached for efficiency.
 *
 * Example:
 * @code
 * // 5-qubit linear chain: 0-1-2-3-4
 * auto topology = Topology::linear(5);
 *
 * // Check connectivity
 * assert(topology.connected(0, 1) == true);
 * assert(topology.connected(0, 2) == false);
 *
 * // Get distance
 * assert(topology.distance(0, 4) == 4);
 * @endcode
 */
class Topology {
public:
    /// @brief Edge type: pair of connected qubit indices
    using Edge = std::pair<std::size_t, std::size_t>;

    /**
     * @brief Constructs an empty topology with the specified number of qubits.
     * @param num_qubits Number of physical qubits
     * @throws std::invalid_argument if num_qubits is 0
     */
    explicit Topology(std::size_t num_qubits)
        : num_qubits_(num_qubits)
        , adjacency_(num_qubits)
        , distance_computed_(false)
    {
        if (num_qubits == 0) {
            throw std::invalid_argument("Topology must have at least 1 qubit");
        }
    }

    // Default special members
    ~Topology() noexcept = default;
    Topology(Topology&&) noexcept = default;
    Topology& operator=(Topology&&) noexcept = default;
    Topology(const Topology&) = default;
    Topology& operator=(const Topology&) = default;

    // -------------------------------------------------------------------------
    // Edge Management
    // -------------------------------------------------------------------------

    /**
     * @brief Adds a bidirectional edge between two qubits.
     * @param q1 First qubit index
     * @param q2 Second qubit index
     * @throws std::out_of_range if either qubit index is invalid
     * @throws std::invalid_argument if q1 == q2
     */
    void addEdge(std::size_t q1, std::size_t q2) {
        validateQubit(q1);
        validateQubit(q2);
        if (q1 == q2) {
            throw std::invalid_argument("Cannot add self-loop edge");
        }

        // Avoid duplicate edges
        if (!connected(q1, q2)) {
            adjacency_[q1].push_back(q2);
            adjacency_[q2].push_back(q1);
            edges_.emplace_back(std::min(q1, q2), std::max(q1, q2));
            distance_computed_ = false;  // Invalidate cache
        }
    }

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Returns the number of physical qubits.
    [[nodiscard]] std::size_t numQubits() const noexcept { return num_qubits_; }

    /// @brief Returns the number of edges.
    [[nodiscard]] std::size_t numEdges() const noexcept { return edges_.size(); }

    /// @brief Returns all edges in the topology.
    [[nodiscard]] const std::vector<Edge>& edges() const noexcept { return edges_; }

    /**
     * @brief Checks if two qubits are directly connected.
     * @param q1 First qubit index
     * @param q2 Second qubit index
     * @return true if q1 and q2 can execute a two-qubit gate directly
     */
    [[nodiscard]] bool connected(std::size_t q1, std::size_t q2) const {
        if (q1 >= num_qubits_ || q2 >= num_qubits_) {
            return false;
        }
        if (q1 == q2) {
            return true;  // Qubit is connected to itself
        }
        const auto& neighbors = adjacency_[q1];
        return std::find(neighbors.begin(), neighbors.end(), q2) != neighbors.end();
    }

    /**
     * @brief Returns the neighbors of a qubit (directly connected qubits).
     * @param qubit The qubit index
     * @return Vector of connected qubit indices
     * @throws std::out_of_range if qubit index is invalid
     */
    [[nodiscard]] const std::vector<std::size_t>& neighbors(std::size_t qubit) const {
        validateQubit(qubit);
        return adjacency_[qubit];
    }

    /**
     * @brief Returns the shortest path distance between two qubits.
     *
     * Distance is the minimum number of SWAP operations needed to bring
     * the qubits adjacent. Returns 0 if qubits are the same, 1 if directly
     * connected.
     *
     * @param q1 First qubit index
     * @param q2 Second qubit index
     * @return Number of edges in shortest path, or INFINITE if disconnected
     */
    [[nodiscard]] std::size_t distance(std::size_t q1, std::size_t q2) const {
        validateQubit(q1);
        validateQubit(q2);
        if (q1 == q2) {
            return 0;
        }
        ensureDistanceComputed();
        return distance_cache_[q1][q2];
    }

    /**
     * @brief Returns the shortest path between two qubits.
     * @param from Source qubit
     * @param to Destination qubit
     * @return Vector of qubit indices forming the path (includes from and to)
     * @throws std::out_of_range if either qubit is invalid
     * @throws std::runtime_error if qubits are disconnected
     */
    [[nodiscard]] std::vector<std::size_t> shortestPath(std::size_t from,
                                                         std::size_t to) const {
        validateQubit(from);
        validateQubit(to);

        if (from == to) {
            return {from};
        }

        // BFS to find path
        std::vector<std::size_t> parent(num_qubits_, INFINITE);
        std::queue<std::size_t> queue;
        queue.push(from);
        parent[from] = from;  // Mark as visited with self-reference

        while (!queue.empty()) {
            std::size_t current = queue.front();
            queue.pop();

            if (current == to) {
                break;
            }

            for (std::size_t neighbor : adjacency_[current]) {
                if (parent[neighbor] == INFINITE) {
                    parent[neighbor] = current;
                    queue.push(neighbor);
                }
            }
        }

        if (parent[to] == INFINITE) {
            throw std::runtime_error(
                "No path exists between qubits " + std::to_string(from) +
                " and " + std::to_string(to));
        }

        // Reconstruct path
        std::vector<std::size_t> path;
        for (std::size_t current = to; current != from; current = parent[current]) {
            path.push_back(current);
        }
        path.push_back(from);
        std::reverse(path.begin(), path.end());

        return path;
    }

    /**
     * @brief Checks if the topology is connected (all qubits reachable).
     * @return true if every qubit can reach every other qubit
     */
    [[nodiscard]] bool isConnected() const {
        if (num_qubits_ <= 1) {
            return true;
        }

        // BFS from qubit 0
        std::vector<bool> visited(num_qubits_, false);
        std::queue<std::size_t> queue;
        queue.push(0);
        visited[0] = true;
        std::size_t count = 1;

        while (!queue.empty()) {
            std::size_t current = queue.front();
            queue.pop();

            for (std::size_t neighbor : adjacency_[current]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    queue.push(neighbor);
                    ++count;
                }
            }
        }

        return count == num_qubits_;
    }

    // -------------------------------------------------------------------------
    // Factory Methods for Common Topologies
    // -------------------------------------------------------------------------

    /**
     * @brief Creates a linear topology (chain).
     *
     * Qubits are connected in a line: 0-1-2-...-n-1
     *
     * @param n Number of qubits
     * @return Linear topology
     */
    [[nodiscard]] static Topology linear(std::size_t n) {
        if (n == 0) {
            throw std::invalid_argument("Linear topology requires at least 1 qubit");
        }
        Topology t(n);
        for (std::size_t i = 0; i + 1 < n; ++i) {
            t.addEdge(i, i + 1);
        }
        return t;
    }

    /**
     * @brief Creates a ring topology.
     *
     * Like linear, but with an additional edge from last to first qubit.
     *
     * @param n Number of qubits
     * @return Ring topology
     */
    [[nodiscard]] static Topology ring(std::size_t n) {
        if (n < 2) {
            throw std::invalid_argument("Ring topology requires at least 2 qubits");
        }
        Topology t = linear(n);
        t.addEdge(0, n - 1);  // Close the ring
        return t;
    }

    /**
     * @brief Creates a 2D grid topology.
     *
     * Qubits are arranged in a rows x cols grid with nearest-neighbor
     * connectivity. Qubit indexing is row-major: qubit[r][c] = r * cols + c.
     *
     * @param rows Number of rows
     * @param cols Number of columns
     * @return Grid topology
     */
    [[nodiscard]] static Topology grid(std::size_t rows, std::size_t cols) {
        if (rows == 0 || cols == 0) {
            throw std::invalid_argument("Grid dimensions must be positive");
        }
        std::size_t n = rows * cols;
        Topology t(n);

        for (std::size_t r = 0; r < rows; ++r) {
            for (std::size_t c = 0; c < cols; ++c) {
                std::size_t q = r * cols + c;
                // Connect to right neighbor
                if (c + 1 < cols) {
                    t.addEdge(q, q + 1);
                }
                // Connect to bottom neighbor
                if (r + 1 < rows) {
                    t.addEdge(q, q + cols);
                }
            }
        }

        return t;
    }

    /**
     * @brief Creates an IBM heavy-hex topology.
     *
     * The heavy-hex lattice is IBM's preferred topology for their quantum
     * processors. It consists of hexagonal cells with additional "heavy"
     * qubits on each edge.
     *
     * For simplicity, this implementation creates a heavy-hex pattern
     * with 'd' distance code, resulting in approximately 3*d^2 qubits.
     *
     * Reference: Chamberland et al., "Topological and Subsystem Codes on
     * Low-Degree Graphs with Flag Qubits" (2020)
     *
     * @param d Distance parameter (1, 2, 3, ...)
     * @return Heavy-hex topology
     */
    [[nodiscard]] static Topology heavyHex(std::size_t d) {
        if (d == 0) {
            throw std::invalid_argument("Heavy-hex distance must be positive");
        }

        // For d=1: simple 7-qubit hexagon with center
        // For larger d: build a lattice of heavy hexagons
        if (d == 1) {
            // Basic heavy-hex unit cell (7 qubits)
            // Layout:
            //     0---1
            //   /       |
            //  5    6    2
            //   |       /
            //     4---3
            Topology t(7);
            t.addEdge(0, 1);
            t.addEdge(1, 2);
            t.addEdge(2, 3);
            t.addEdge(3, 4);
            t.addEdge(4, 5);
            t.addEdge(5, 0);
            // Connect center to all edges
            t.addEdge(6, 0);
            t.addEdge(6, 1);
            t.addEdge(6, 2);
            t.addEdge(6, 3);
            t.addEdge(6, 4);
            t.addEdge(6, 5);
            return t;
        }

        // For d >= 2: Build a larger heavy-hex lattice
        // Use a simplified model: create d x d hexagonal cells
        // Each cell has 6 boundary qubits + 1 center qubit, with shared edges
        // Total qubits approximately: 2*d*(d+1) + d*d (simplified formula)
        std::size_t n = 5 * d * d + 4 * d + 1;  // Approximate
        Topology t(n);

        // Build as a modified grid with heavy-hex connectivity pattern
        // This is a simplified approximation of IBM's actual topology
        std::size_t rows = 2 * d + 1;
        std::size_t cols = 2 * d + 1;
        n = rows * cols;
        t = Topology(n);

        for (std::size_t r = 0; r < rows; ++r) {
            for (std::size_t c = 0; c < cols; ++c) {
                std::size_t q = r * cols + c;

                // Horizontal connections (every other row has different pattern)
                if (c + 1 < cols) {
                    // Connect horizontally
                    t.addEdge(q, q + 1);
                }

                // Vertical connections (heavy-hex pattern)
                if (r + 1 < rows && (c % 2 == r % 2)) {
                    t.addEdge(q, q + cols);
                }
            }
        }

        return t;
    }

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /**
     * @brief Returns a string representation of the topology.
     * @return Multi-line string showing qubits and edges
     */
    [[nodiscard]] std::string toString() const {
        std::string result = "Topology(" + std::to_string(num_qubits_) +
                             " qubits, " + std::to_string(edges_.size()) +
                             " edges):\n  Edges: ";
        for (std::size_t i = 0; i < edges_.size(); ++i) {
            if (i > 0) result += ", ";
            result += "(" + std::to_string(edges_[i].first) + "-" +
                      std::to_string(edges_[i].second) + ")";
        }
        return result;
    }

    /// @brief Sentinel value for infinite distance (disconnected qubits)
    static constexpr std::size_t INFINITE = std::numeric_limits<std::size_t>::max();

private:
    std::size_t num_qubits_;
    std::vector<std::vector<std::size_t>> adjacency_;  // Adjacency list
    std::vector<Edge> edges_;                          // All edges
    mutable std::vector<std::vector<std::size_t>> distance_cache_;
    mutable bool distance_computed_;

    /**
     * @brief Validates a qubit index.
     * @param q Qubit index
     * @throws std::out_of_range if index is invalid
     */
    void validateQubit(std::size_t q) const {
        if (q >= num_qubits_) {
            throw std::out_of_range(
                "Qubit index " + std::to_string(q) +
                " out of range [0, " + std::to_string(num_qubits_) + ")");
        }
    }

    /**
     * @brief Computes all-pairs shortest distances using BFS.
     *
     * Distances are computed lazily and cached.
     */
    void ensureDistanceComputed() const {
        if (distance_computed_) {
            return;
        }

        distance_cache_.assign(num_qubits_,
                               std::vector<std::size_t>(num_qubits_, INFINITE));

        // BFS from each qubit
        for (std::size_t start = 0; start < num_qubits_; ++start) {
            std::queue<std::size_t> queue;
            queue.push(start);
            distance_cache_[start][start] = 0;

            while (!queue.empty()) {
                std::size_t current = queue.front();
                queue.pop();

                for (std::size_t neighbor : adjacency_[current]) {
                    if (distance_cache_[start][neighbor] == INFINITE) {
                        distance_cache_[start][neighbor] =
                            distance_cache_[start][current] + 1;
                        queue.push(neighbor);
                    }
                }
            }
        }

        distance_computed_ = true;
    }
};

}  // namespace qopt::routing
