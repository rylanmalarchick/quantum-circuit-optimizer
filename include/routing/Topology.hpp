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
     * @brief Creates a heavy-hex lattice topology.
     *
     * The heavy-hex lattice (IBM's processor family) is a hexagonal lattice in
     * which every edge carries an extra degree-2 "link" qubit. Lattice vertices
     * therefore have degree at most 3 and link qubits have degree exactly 2 --
     * the low-degree property the architecture is built around.
     *
     * The construction lays out a honeycomb of (2d+1) x (2d+1) vertex sites as a
     * brick wall (horizontal edges everywhere, vertical edges only where
     * row + col is even, which caps vertex degree at 3) and then subdivides each
     * edge with one link qubit. The qubit count grows with d; call numQubits()
     * for the exact size.
     *
     * Reference: Chamberland et al., "Topological and Subsystem Codes on
     * Low-Degree Graphs with Flag Qubits", Phys. Rev. X 10, 011022 (2020).
     *
     * @param d Lattice size parameter (1, 2, 3, ...)
     * @return Heavy-hex topology
     */
    [[nodiscard]] static Topology heavyHex(std::size_t d) {
        if (d == 0) {
            throw std::invalid_argument("Heavy-hex distance must be positive");
        }

        const std::size_t rows = 2 * d + 1;
        const std::size_t cols = 2 * d + 1;
        const std::size_t num_vertices = rows * cols;
        auto vertex = [cols](std::size_t r, std::size_t c) { return r * cols + c; };

        // Honeycomb (brick-wall) edges between vertex sites; degree <= 3.
        std::vector<Edge> hex_edges;
        for (std::size_t r = 0; r < rows; ++r) {
            for (std::size_t c = 0; c < cols; ++c) {
                if (c + 1 < cols) {
                    hex_edges.emplace_back(vertex(r, c), vertex(r, c + 1));
                }
                if (r + 1 < rows && (r + c) % 2 == 0) {
                    hex_edges.emplace_back(vertex(r, c), vertex(r + 1, c));
                }
            }
        }

        // Subdivide each edge with a degree-2 link qubit (the "heavy" sites).
        Topology t(num_vertices + hex_edges.size());
        std::size_t link = num_vertices;
        for (const auto& [u, v] : hex_edges) {
            t.addEdge(u, link);
            t.addEdge(link, v);
            ++link;
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
