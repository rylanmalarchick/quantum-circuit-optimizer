// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_routing.cpp
 * @brief Unit tests for qubit routing
 *
 * Tests for Topology, Router, TrivialRouter, and SabreRouter.
 */

#include "routing/Topology.hpp"
#include "routing/Router.hpp"
#include "routing/SabreRouter.hpp"
#include "ir/Circuit.hpp"
#include "ir/Gate.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>

using namespace qopt;
using namespace qopt::ir;
using namespace qopt::routing;

// =============================================================================
// Topology Construction Tests
// =============================================================================

TEST(TopologyTest, ConstructorRequiresPositiveQubits) {
    EXPECT_THROW(Topology(0), std::invalid_argument);
}

TEST(TopologyTest, ConstructorCreatesEmptyTopology) {
    Topology t(5);
    EXPECT_EQ(t.numQubits(), 5);
    EXPECT_EQ(t.numEdges(), 0);
}

TEST(TopologyTest, AddEdgeIncrementsEdgeCount) {
    Topology t(4);
    t.addEdge(0, 1);
    EXPECT_EQ(t.numEdges(), 1);
    t.addEdge(1, 2);
    EXPECT_EQ(t.numEdges(), 2);
}

TEST(TopologyTest, AddEdgeIsBidirectional) {
    Topology t(3);
    t.addEdge(0, 2);
    EXPECT_TRUE(t.connected(0, 2));
    EXPECT_TRUE(t.connected(2, 0));
}

TEST(TopologyTest, AddEdgeValidatesQubits) {
    Topology t(3);
    EXPECT_THROW(t.addEdge(0, 5), std::out_of_range);
    EXPECT_THROW(t.addEdge(10, 1), std::out_of_range);
}

TEST(TopologyTest, AddEdgeRejectsSelfLoop) {
    Topology t(3);
    EXPECT_THROW(t.addEdge(1, 1), std::invalid_argument);
}

TEST(TopologyTest, AddEdgeIgnoresDuplicates) {
    Topology t(3);
    t.addEdge(0, 1);
    t.addEdge(0, 1);  // Duplicate
    t.addEdge(1, 0);  // Reverse order duplicate
    EXPECT_EQ(t.numEdges(), 1);
}

// =============================================================================
// Topology Query Tests
// =============================================================================

TEST(TopologyTest, ConnectedSameQubitReturnsTrue) {
    Topology t(3);
    EXPECT_TRUE(t.connected(0, 0));
    EXPECT_TRUE(t.connected(2, 2));
}

TEST(TopologyTest, ConnectedUnconnectedReturnsFalse) {
    Topology t(3);
    t.addEdge(0, 1);
    EXPECT_FALSE(t.connected(0, 2));
    EXPECT_FALSE(t.connected(1, 2));
}

TEST(TopologyTest, NeighborsReturnsCorrectList) {
    Topology t(5);
    t.addEdge(2, 0);
    t.addEdge(2, 1);
    t.addEdge(2, 4);

    const auto& neighbors = t.neighbors(2);
    EXPECT_EQ(neighbors.size(), 3);
    EXPECT_TRUE(std::find(neighbors.begin(), neighbors.end(), 0) != neighbors.end());
    EXPECT_TRUE(std::find(neighbors.begin(), neighbors.end(), 1) != neighbors.end());
    EXPECT_TRUE(std::find(neighbors.begin(), neighbors.end(), 4) != neighbors.end());
}

TEST(TopologyTest, NeighborsValidatesQubit) {
    Topology t(3);
    EXPECT_THROW((void)t.neighbors(5), std::out_of_range);
}

TEST(TopologyTest, DistanceSameQubitReturnsZero) {
    auto t = Topology::linear(5);
    EXPECT_EQ(t.distance(0, 0), 0);
    EXPECT_EQ(t.distance(4, 4), 0);
}

TEST(TopologyTest, DistanceAdjacentReturnsOne) {
    auto t = Topology::linear(5);
    EXPECT_EQ(t.distance(0, 1), 1);
    EXPECT_EQ(t.distance(2, 3), 1);
}

TEST(TopologyTest, DistanceLinearChain) {
    auto t = Topology::linear(5);
    EXPECT_EQ(t.distance(0, 4), 4);
    EXPECT_EQ(t.distance(1, 4), 3);
    EXPECT_EQ(t.distance(0, 2), 2);
}

TEST(TopologyTest, DistanceValidatesQubits) {
    auto t = Topology::linear(3);
    EXPECT_THROW((void)t.distance(0, 10), std::out_of_range);
    EXPECT_THROW((void)t.distance(10, 0), std::out_of_range);
}

TEST(TopologyTest, ShortestPathSameQubit) {
    auto t = Topology::linear(5);
    auto path = t.shortestPath(2, 2);
    EXPECT_EQ(path.size(), 1);
    EXPECT_EQ(path[0], 2);
}

TEST(TopologyTest, ShortestPathAdjacent) {
    auto t = Topology::linear(5);
    auto path = t.shortestPath(1, 2);
    EXPECT_EQ(path.size(), 2);
    EXPECT_EQ(path[0], 1);
    EXPECT_EQ(path[1], 2);
}

TEST(TopologyTest, ShortestPathLinear) {
    auto t = Topology::linear(5);
    auto path = t.shortestPath(0, 4);
    EXPECT_EQ(path.size(), 5);
    EXPECT_EQ(path[0], 0);
    EXPECT_EQ(path[4], 4);
}

TEST(TopologyTest, IsConnectedLinear) {
    auto t = Topology::linear(5);
    EXPECT_TRUE(t.isConnected());
}

TEST(TopologyTest, IsConnectedDisconnected) {
    Topology t(4);
    t.addEdge(0, 1);
    t.addEdge(2, 3);
    EXPECT_FALSE(t.isConnected());
}

TEST(TopologyTest, IsConnectedSingleQubit) {
    Topology t(1);
    EXPECT_TRUE(t.isConnected());
}

// =============================================================================
// Topology Factory Method Tests
// =============================================================================

TEST(TopologyTest, LinearCreatesChain) {
    auto t = Topology::linear(4);
    EXPECT_EQ(t.numQubits(), 4);
    EXPECT_EQ(t.numEdges(), 3);  // 0-1, 1-2, 2-3
    EXPECT_TRUE(t.connected(0, 1));
    EXPECT_TRUE(t.connected(1, 2));
    EXPECT_TRUE(t.connected(2, 3));
    EXPECT_FALSE(t.connected(0, 2));
    EXPECT_FALSE(t.connected(0, 3));
}

TEST(TopologyTest, LinearSingleQubit) {
    auto t = Topology::linear(1);
    EXPECT_EQ(t.numQubits(), 1);
    EXPECT_EQ(t.numEdges(), 0);
}

TEST(TopologyTest, LinearValidation) {
    EXPECT_THROW((void)Topology::linear(0), std::invalid_argument);
}

TEST(TopologyTest, RingClosesLoop) {
    auto t = Topology::ring(4);
    EXPECT_EQ(t.numQubits(), 4);
    EXPECT_EQ(t.numEdges(), 4);  // 0-1, 1-2, 2-3, 3-0
    EXPECT_TRUE(t.connected(3, 0));
    EXPECT_TRUE(t.connected(0, 3));
}

TEST(TopologyTest, RingReducesMaxDistance) {
    auto linear = Topology::linear(4);
    auto ring = Topology::ring(4);

    // In linear: distance(0,3) = 3
    // In ring: distance(0,3) = 1 (direct edge)
    EXPECT_EQ(linear.distance(0, 3), 3);
    EXPECT_EQ(ring.distance(0, 3), 1);
}

TEST(TopologyTest, RingValidation) {
    EXPECT_THROW((void)Topology::ring(0), std::invalid_argument);
    EXPECT_THROW((void)Topology::ring(1), std::invalid_argument);
}

TEST(TopologyTest, Grid2x2) {
    auto t = Topology::grid(2, 2);
    //  0 - 1
    //  |   |
    //  2 - 3
    EXPECT_EQ(t.numQubits(), 4);
    EXPECT_EQ(t.numEdges(), 4);
    EXPECT_TRUE(t.connected(0, 1));
    EXPECT_TRUE(t.connected(0, 2));
    EXPECT_TRUE(t.connected(1, 3));
    EXPECT_TRUE(t.connected(2, 3));
    EXPECT_FALSE(t.connected(0, 3));  // Diagonal
}

TEST(TopologyTest, Grid3x3) {
    auto t = Topology::grid(3, 3);
    EXPECT_EQ(t.numQubits(), 9);
    // Horizontal: 6 edges, Vertical: 6 edges = 12 total
    EXPECT_EQ(t.numEdges(), 12);
}

TEST(TopologyTest, GridDistances) {
    auto t = Topology::grid(3, 3);
    //  0 - 1 - 2
    //  |   |   |
    //  3 - 4 - 5
    //  |   |   |
    //  6 - 7 - 8
    EXPECT_EQ(t.distance(0, 8), 4);  // Manhattan distance
    EXPECT_EQ(t.distance(0, 4), 2);
    EXPECT_EQ(t.distance(1, 7), 2);
}

TEST(TopologyTest, GridValidation) {
    EXPECT_THROW((void)Topology::grid(0, 3), std::invalid_argument);
    EXPECT_THROW((void)Topology::grid(3, 0), std::invalid_argument);
}

TEST(TopologyTest, HeavyHexD1) {
    auto t = Topology::heavyHex(1);
    EXPECT_EQ(t.numQubits(), 7);
    EXPECT_TRUE(t.isConnected());
}

TEST(TopologyTest, HeavyHexD2) {
    auto t = Topology::heavyHex(2);
    EXPECT_TRUE(t.isConnected());
    EXPECT_GT(t.numQubits(), 7);
}

TEST(TopologyTest, HeavyHexValidation) {
    EXPECT_THROW((void)Topology::heavyHex(0), std::invalid_argument);
}

TEST(TopologyTest, ToStringIncludesInfo) {
    auto t = Topology::linear(3);
    std::string str = t.toString();
    EXPECT_TRUE(str.find("3 qubits") != std::string::npos);
    EXPECT_TRUE(str.find("2 edges") != std::string::npos);
}

// =============================================================================
// RoutingResult Tests
// =============================================================================

TEST(RoutingResultTest, DepthOverheadCalculation) {
    Circuit c(2);
    RoutingResult result(std::move(c));
    result.original_depth = 5;
    result.final_depth = 8;
    EXPECT_EQ(result.depthOverhead(), 3);
}

TEST(RoutingResultTest, DepthOverheadNoIncrease) {
    Circuit c(2);
    RoutingResult result(std::move(c));
    result.original_depth = 5;
    result.final_depth = 5;
    EXPECT_EQ(result.depthOverhead(), 0);
}

TEST(RoutingResultTest, GateOverheadCalculation) {
    Circuit c(2);
    RoutingResult result(std::move(c));
    result.swaps_inserted = 4;
    EXPECT_EQ(result.gateOverhead(), 12);  // 4 * 3 CNOTs per SWAP
}

TEST(RoutingResultTest, ToStringIncludesStats) {
    Circuit c(2);
    RoutingResult result(std::move(c));
    result.swaps_inserted = 3;
    result.original_depth = 5;
    result.final_depth = 10;
    result.initial_mapping = {0, 1};
    result.final_mapping = {1, 0};

    std::string str = result.toString();
    EXPECT_TRUE(str.find("3") != std::string::npos);  // swaps
    EXPECT_TRUE(str.find("5") != std::string::npos);  // original depth
}

// =============================================================================
// TrivialRouter Tests
// =============================================================================

TEST(TrivialRouterTest, NameReturnsCorrectValue) {
    TrivialRouter router;
    EXPECT_EQ(router.name(), "TrivialRouter");
}

TEST(TrivialRouterTest, EmptyCircuit) {
    TrivialRouter router;
    Circuit c(3);
    auto topology = Topology::linear(3);

    auto result = router.route(c, topology);
    EXPECT_EQ(result.routed_circuit.numGates(), 0);
    EXPECT_EQ(result.swaps_inserted, 0);
}

TEST(TrivialRouterTest, IdentityMapping) {
    TrivialRouter router;
    Circuit c(3);
    c.addGate(Gate::h(0));
    auto topology = Topology::linear(3);

    auto result = router.route(c, topology);
    EXPECT_EQ(result.initial_mapping.size(), 3);
    EXPECT_EQ(result.initial_mapping[0], 0);
    EXPECT_EQ(result.initial_mapping[1], 1);
    EXPECT_EQ(result.initial_mapping[2], 2);
}

TEST(TrivialRouterTest, PreservesGates) {
    TrivialRouter router;
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    auto topology = Topology::linear(2);

    auto result = router.route(c, topology);
    EXPECT_EQ(result.routed_circuit.numGates(), 2);
}

TEST(TrivialRouterTest, RejectsTooManyQubits) {
    TrivialRouter router;
    Circuit c(5);
    auto topology = Topology::linear(3);

    EXPECT_THROW((void)router.route(c, topology), std::invalid_argument);
}

// =============================================================================
// SabreRouter Tests
// =============================================================================

TEST(SabreRouterTest, NameReturnsCorrectValue) {
    SabreRouter router;
    EXPECT_EQ(router.name(), "SabreRouter");
}

TEST(SabreRouterTest, EmptyCircuit) {
    SabreRouter router;
    Circuit c(3);
    auto topology = Topology::linear(5);

    auto result = router.route(c, topology);
    EXPECT_EQ(result.routed_circuit.numGates(), 0);
    EXPECT_EQ(result.swaps_inserted, 0);
}

TEST(SabreRouterTest, SingleQubitGates) {
    SabreRouter router;
    Circuit c(3);
    c.addGate(Gate::h(0));
    c.addGate(Gate::x(1));
    c.addGate(Gate::z(2));
    auto topology = Topology::linear(5);

    auto result = router.route(c, topology);
    // Single-qubit gates need no SWAPs
    EXPECT_EQ(result.swaps_inserted, 0);
    EXPECT_EQ(result.routed_circuit.numGates(), 3);
}

TEST(SabreRouterTest, AdjacentCNOT) {
    SabreRouter router;
    Circuit c(2);
    c.addGate(Gate::cnot(0, 1));
    auto topology = Topology::linear(5);

    auto result = router.route(c, topology);
    // Adjacent qubits - no SWAP needed
    EXPECT_EQ(result.swaps_inserted, 0);
}

TEST(SabreRouterTest, NonAdjacentCNOT) {
    SabreRouter router;
    Circuit c(4);
    c.addGate(Gate::cnot(0, 3));  // Distance 3 on linear topology
    auto topology = Topology::linear(4);

    auto result = router.route(c, topology);
    // Should insert SWAPs
    EXPECT_GT(result.swaps_inserted, 0);
}

TEST(SabreRouterTest, AllTwoQubitGatesExecutable) {
    SabreRouter router;
    Circuit c(4);
    c.addGate(Gate::cnot(0, 3));
    c.addGate(Gate::cz(1, 2));
    auto topology = Topology::linear(4);

    auto result = router.route(c, topology);

    // Verify all two-qubit gates in result are on adjacent qubits
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            std::size_t p0 = gate.qubits()[0];
            std::size_t p1 = gate.qubits()[1];
            EXPECT_TRUE(topology.connected(p0, p1))
                << "Gate " << gate.toString() << " on non-adjacent qubits";
        }
    }
}

TEST(SabreRouterTest, GridTopology) {
    SabreRouter router;
    Circuit c(4);
    // On 2x2 grid, diagonal qubits are non-adjacent
    c.addGate(Gate::cnot(0, 3));  // Diagonal on grid
    auto topology = Topology::grid(2, 2);

    auto result = router.route(c, topology);

    // Should insert at least one SWAP
    EXPECT_GT(result.swaps_inserted, 0);

    // All gates should be on adjacent qubits
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            EXPECT_TRUE(topology.connected(gate.qubits()[0], gate.qubits()[1]));
        }
    }
}

TEST(SabreRouterTest, RingTopologyReducesSWAPs) {
    SabreRouter router;
    Circuit c(4);
    c.addGate(Gate::cnot(0, 3));
    auto linear = Topology::linear(4);
    auto ring = Topology::ring(4);

    auto linear_result = router.route(c, linear);
    auto ring_result = router.route(c, ring);

    // Ring should need fewer or equal SWAPs (0,3 is adjacent in ring)
    EXPECT_LE(ring_result.swaps_inserted, linear_result.swaps_inserted);
}

TEST(SabreRouterTest, MultipleTwoQubitGates) {
    SabreRouter router;
    Circuit c(5);
    c.addGate(Gate::cnot(0, 1));
    c.addGate(Gate::cnot(1, 2));
    c.addGate(Gate::cnot(2, 3));
    c.addGate(Gate::cnot(3, 4));
    auto topology = Topology::linear(5);

    auto result = router.route(c, topology);
    // All gates are on adjacent qubits - no SWAPs needed
    EXPECT_EQ(result.swaps_inserted, 0);
}

TEST(SabreRouterTest, MixedCircuit) {
    SabreRouter router;
    Circuit c(4);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    c.addGate(Gate::rz(2, 0.5));
    c.addGate(Gate::cnot(1, 2));
    c.addGate(Gate::h(3));
    auto topology = Topology::linear(4);

    auto result = router.route(c, topology);

    // Verify all two-qubit gates are executable
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            EXPECT_TRUE(topology.connected(gate.qubits()[0], gate.qubits()[1]));
        }
    }
}

TEST(SabreRouterTest, RejectsTooManyQubits) {
    SabreRouter router;
    Circuit c(10);
    auto topology = Topology::linear(5);

    EXPECT_THROW((void)router.route(c, topology), std::invalid_argument);
}

TEST(SabreRouterTest, StatisticsAreRecorded) {
    SabreRouter router;
    Circuit c(4);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 3));
    auto topology = Topology::linear(4);

    auto result = router.route(c, topology);

    EXPECT_EQ(result.original_depth, c.depth());
    EXPECT_GE(result.final_depth, result.original_depth);
    EXPECT_EQ(result.initial_mapping.size(), c.numQubits());
    EXPECT_EQ(result.final_mapping.size(), c.numQubits());
}

TEST(SabreRouterTest, LargerCircuit) {
    SabreRouter router;
    Circuit c(6);

    // Build a circuit with some non-adjacent interactions
    c.addGate(Gate::h(0));
    c.addGate(Gate::h(1));
    c.addGate(Gate::h(2));
    c.addGate(Gate::cnot(0, 5));  // Far apart
    c.addGate(Gate::cnot(1, 4));  // Far apart
    c.addGate(Gate::cnot(2, 3));  // Adjacent

    auto topology = Topology::linear(6);
    auto result = router.route(c, topology);

    // Verify routing was successful
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            EXPECT_TRUE(topology.connected(gate.qubits()[0], gate.qubits()[1]));
        }
    }
}

TEST(SabreRouterTest, SwapOverheadReasonable) {
    SabreRouter router;
    Circuit c(10);

    // Circuit requiring routing
    for (std::size_t i = 0; i < 5; ++i) {
        c.addGate(Gate::cnot(i, 9 - i));
    }

    auto topology = Topology::linear(10);
    auto result = router.route(c, topology);

    // SWAP overhead should be less than 3x gate count (reasonable heuristic)
    std::size_t original_gates = c.numGates();
    std::size_t final_gates = result.routed_circuit.numGates();
    EXPECT_LT(final_gates, original_gates * 6);  // Conservative bound
}

TEST(SabreRouterTest, CustomParameters) {
    SabreRouter router(10, 0.3, 0.7);  // Custom lookahead, decay, weight
    Circuit c(4);
    c.addGate(Gate::cnot(0, 3));
    auto topology = Topology::linear(4);

    // Should still produce valid result
    auto result = router.route(c, topology);
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            EXPECT_TRUE(topology.connected(gate.qubits()[0], gate.qubits()[1]));
        }
    }
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST(RoutingIntegrationTest, BellState) {
    SabreRouter router;
    Circuit c(2);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    auto topology = Topology::linear(2);

    auto result = router.route(c, topology);

    // Bell state on 2-qubit linear topology needs no routing
    EXPECT_EQ(result.swaps_inserted, 0);
    EXPECT_EQ(result.routed_circuit.numGates(), 2);
}

TEST(RoutingIntegrationTest, GHZState) {
    SabreRouter router;
    Circuit c(4);
    c.addGate(Gate::h(0));
    c.addGate(Gate::cnot(0, 1));
    c.addGate(Gate::cnot(1, 2));
    c.addGate(Gate::cnot(2, 3));
    auto topology = Topology::linear(4);

    auto result = router.route(c, topology);

    // GHZ on linear topology with adjacent CNOTs needs no routing
    EXPECT_EQ(result.swaps_inserted, 0);
}

TEST(RoutingIntegrationTest, QFTLike) {
    SabreRouter router;
    Circuit c(4);

    // QFT-like pattern: controlled rotations between all pairs
    for (std::size_t i = 0; i < 4; ++i) {
        c.addGate(Gate::h(i));
        for (std::size_t j = i + 1; j < 4; ++j) {
            c.addGate(Gate::cz(i, j));
        }
    }

    auto topology = Topology::linear(4);
    auto result = router.route(c, topology);

    // All gates should be executable
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            EXPECT_TRUE(topology.connected(gate.qubits()[0], gate.qubits()[1]));
        }
    }
}

TEST(RoutingIntegrationTest, RandomCircuit) {
    SabreRouter router;
    Circuit c(6);

    // Random-ish pattern
    c.addGate(Gate::h(0));
    c.addGate(Gate::h(3));
    c.addGate(Gate::cnot(0, 2));
    c.addGate(Gate::cnot(3, 5));
    c.addGate(Gate::cz(1, 4));
    c.addGate(Gate::cnot(2, 4));

    auto topology = Topology::grid(2, 3);  // 2x3 grid
    auto result = router.route(c, topology);

    // All gates should be executable
    for (const auto& gate : result.routed_circuit) {
        if (gate.numQubits() == 2) {
            EXPECT_TRUE(topology.connected(gate.qubits()[0], gate.qubits()[1]));
        }
    }
}
