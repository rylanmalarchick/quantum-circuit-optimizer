// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_dag.cpp
 * @brief Unit tests for the DAGNode and DAG classes
 */

#include "ir/DAG.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <unordered_set>

namespace qopt::ir {
namespace {

// =============================================================================
// DAG Construction Tests
// =============================================================================

TEST(DAGConstructionTest, ConstructsWithValidQubitCount) {
    DAG dag(5);
    EXPECT_EQ(dag.numQubits(), 5);
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_TRUE(dag.empty());
}

TEST(DAGConstructionTest, ThrowsOnZeroQubits) {
    EXPECT_THROW(DAG(0), std::invalid_argument);
}

TEST(DAGConstructionTest, ThrowsOnExcessiveQubits) {
    EXPECT_THROW(DAG(constants::MAX_QUBITS + 1), std::invalid_argument);
}

TEST(DAGConstructionTest, AcceptsMaxQubits) {
    DAG dag(constants::MAX_QUBITS);
    EXPECT_EQ(dag.numQubits(), constants::MAX_QUBITS);
}

// =============================================================================
// Node Addition Tests
// =============================================================================

TEST(DAGNodeAddTest, AddGateReturnsSequentialIds) {
    DAG dag(2);
    GateId id0 = dag.addGate(Gate::h(0));
    GateId id1 = dag.addGate(Gate::x(1));
    GateId id2 = dag.addGate(Gate::cnot(0, 1));

    EXPECT_EQ(id0, 0);
    EXPECT_EQ(id1, 1);
    EXPECT_EQ(id2, 2);
}

TEST(DAGNodeAddTest, AddGateIncreasesNodeCount) {
    DAG dag(2);
    EXPECT_EQ(dag.numNodes(), 0);

    dag.addGate(Gate::h(0));
    EXPECT_EQ(dag.numNodes(), 1);

    dag.addGate(Gate::cnot(0, 1));
    EXPECT_EQ(dag.numNodes(), 2);
}

TEST(DAGNodeAddTest, AddGateThrowsOnInvalidQubit) {
    DAG dag(2);  // qubits 0 and 1 only

    EXPECT_THROW(dag.addGate(Gate::h(2)), std::out_of_range);
    EXPECT_THROW(dag.addGate(Gate::cnot(0, 5)), std::out_of_range);
}

TEST(DAGNodeAddTest, NodeAccessorReturnsCorrectNode) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::x(1));

    EXPECT_EQ(dag.node(0).gate().type(), GateType::H);
    EXPECT_EQ(dag.node(1).gate().type(), GateType::X);
}

TEST(DAGNodeAddTest, NodeAccessorThrowsOnInvalidId) {
    DAG dag(2);
    dag.addGate(Gate::h(0));

    EXPECT_THROW((void)dag.node(1), std::out_of_range);
    EXPECT_THROW((void)dag.node(100), std::out_of_range);
}

TEST(DAGNodeAddTest, HasNodeReturnsCorrectly) {
    DAG dag(2);
    dag.addGate(Gate::h(0));

    EXPECT_TRUE(dag.hasNode(0));
    EXPECT_FALSE(dag.hasNode(1));
    EXPECT_FALSE(dag.hasNode(100));
}

// =============================================================================
// Dependency Tests
// =============================================================================

TEST(DAGDependencyTest, IndependentGatesHaveNoDependencies) {
    DAG dag(3);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::h(1));
    dag.addGate(Gate::h(2));

    // All gates are on different qubits - no dependencies
    EXPECT_TRUE(dag.node(0).predecessors().empty());
    EXPECT_TRUE(dag.node(1).predecessors().empty());
    EXPECT_TRUE(dag.node(2).predecessors().empty());

    EXPECT_TRUE(dag.node(0).successors().empty());
    EXPECT_TRUE(dag.node(1).successors().empty());
    EXPECT_TRUE(dag.node(2).successors().empty());
}

TEST(DAGDependencyTest, SequentialGatesOnSameQubitHaveDependency) {
    DAG dag(1);
    dag.addGate(Gate::h(0));   // id 0
    dag.addGate(Gate::x(0));   // id 1
    dag.addGate(Gate::z(0));   // id 2

    // H -> X -> Z chain
    EXPECT_TRUE(dag.node(0).predecessors().empty());
    EXPECT_EQ(dag.node(0).successors().size(), 1);
    EXPECT_EQ(dag.node(0).successors()[0], 1);

    EXPECT_EQ(dag.node(1).predecessors().size(), 1);
    EXPECT_EQ(dag.node(1).predecessors()[0], 0);
    EXPECT_EQ(dag.node(1).successors().size(), 1);
    EXPECT_EQ(dag.node(1).successors()[0], 2);

    EXPECT_EQ(dag.node(2).predecessors().size(), 1);
    EXPECT_EQ(dag.node(2).predecessors()[0], 1);
    EXPECT_TRUE(dag.node(2).successors().empty());
}

TEST(DAGDependencyTest, CNOTDependsOnBothQubits) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // id 0
    dag.addGate(Gate::x(1));       // id 1
    dag.addGate(Gate::cnot(0, 1)); // id 2

    // CNOT should have both H and X as predecessors
    const auto& preds = dag.node(2).predecessors();
    EXPECT_EQ(preds.size(), 2);
    EXPECT_TRUE(std::find(preds.begin(), preds.end(), 0) != preds.end());
    EXPECT_TRUE(std::find(preds.begin(), preds.end(), 1) != preds.end());
}

TEST(DAGDependencyTest, BellCircuitDependencies) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // id 0: H on q0
    dag.addGate(Gate::cnot(0, 1)); // id 1: CNOT (depends on H)

    // H is source
    EXPECT_TRUE(dag.node(0).isSource());
    EXPECT_FALSE(dag.node(0).isSink());

    // CNOT is sink, depends on H
    EXPECT_FALSE(dag.node(1).isSource());
    EXPECT_TRUE(dag.node(1).isSink());
    EXPECT_EQ(dag.node(1).predecessors()[0], 0);
}

TEST(DAGDependencyTest, InDegreeOutDegreeCorrect) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // id 0
    dag.addGate(Gate::h(1));       // id 1
    dag.addGate(Gate::cnot(0, 1)); // id 2

    EXPECT_EQ(dag.node(0).inDegree(), 0);
    EXPECT_EQ(dag.node(0).outDegree(), 1);

    EXPECT_EQ(dag.node(1).inDegree(), 0);
    EXPECT_EQ(dag.node(1).outDegree(), 1);

    EXPECT_EQ(dag.node(2).inDegree(), 2);
    EXPECT_EQ(dag.node(2).outDegree(), 0);
}

// =============================================================================
// Source/Sink Tests
// =============================================================================

TEST(DAGSourceSinkTest, EmptyDAGHasNoSourcesOrSinks) {
    DAG dag(2);
    EXPECT_TRUE(dag.sources().empty());
    EXPECT_TRUE(dag.sinks().empty());
}

TEST(DAGSourceSinkTest, SingleNodeIsBothSourceAndSink) {
    DAG dag(1);
    dag.addGate(Gate::h(0));

    auto sources = dag.sources();
    auto sinks = dag.sinks();

    EXPECT_EQ(sources.size(), 1);
    EXPECT_EQ(sinks.size(), 1);
    EXPECT_EQ(sources[0], 0);
    EXPECT_EQ(sinks[0], 0);
}

TEST(DAGSourceSinkTest, BellCircuitSourcesAndSinks) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // source
    dag.addGate(Gate::cnot(0, 1)); // sink

    auto sources = dag.sources();
    auto sinks = dag.sinks();

    EXPECT_EQ(sources.size(), 1);
    EXPECT_EQ(sources[0], 0);

    EXPECT_EQ(sinks.size(), 1);
    EXPECT_EQ(sinks[0], 1);
}

TEST(DAGSourceSinkTest, ParallelGatesAreBothSourcesAndSinks) {
    DAG dag(3);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::h(1));
    dag.addGate(Gate::h(2));

    EXPECT_EQ(dag.sources().size(), 3);
    EXPECT_EQ(dag.sinks().size(), 3);
}

// =============================================================================
// Topological Order Tests
// =============================================================================

TEST(DAGTopologicalTest, EmptyDAGReturnsEmptyOrder) {
    DAG dag(2);
    auto order = dag.topologicalOrder();
    EXPECT_TRUE(order.empty());
}

TEST(DAGTopologicalTest, SingleNodeTopologicalOrder) {
    DAG dag(1);
    dag.addGate(Gate::h(0));

    auto order = dag.topologicalOrder();
    EXPECT_EQ(order.size(), 1);
    EXPECT_EQ(order[0], 0);
}

TEST(DAGTopologicalTest, LinearChainPreservesOrder) {
    DAG dag(1);
    dag.addGate(Gate::h(0));   // 0
    dag.addGate(Gate::x(0));   // 1
    dag.addGate(Gate::z(0));   // 2

    auto order = dag.topologicalOrder();
    EXPECT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 0);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 2);
}

TEST(DAGTopologicalTest, BellCircuitTopologicalOrder) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // 0
    dag.addGate(Gate::cnot(0, 1)); // 1

    auto order = dag.topologicalOrder();
    EXPECT_EQ(order.size(), 2);
    EXPECT_EQ(order[0], 0);  // H must come first
    EXPECT_EQ(order[1], 1);  // CNOT second
}

TEST(DAGTopologicalTest, ParallelGatesValidOrder) {
    DAG dag(3);
    dag.addGate(Gate::h(0));  // 0
    dag.addGate(Gate::h(1));  // 1
    dag.addGate(Gate::h(2));  // 2

    auto order = dag.topologicalOrder();
    EXPECT_EQ(order.size(), 3);

    // All should appear (any order is valid since independent)
    std::unordered_set<GateId> in_order(order.begin(), order.end());
    EXPECT_TRUE(in_order.count(0));
    EXPECT_TRUE(in_order.count(1));
    EXPECT_TRUE(in_order.count(2));
}

TEST(DAGTopologicalTest, ComplexCircuitValidOrder) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // 0
    dag.addGate(Gate::h(1));       // 1
    dag.addGate(Gate::cnot(0, 1)); // 2 (depends on 0 and 1)
    dag.addGate(Gate::x(0));       // 3 (depends on 2)
    dag.addGate(Gate::x(1));       // 4 (depends on 2)

    auto order = dag.topologicalOrder();
    EXPECT_EQ(order.size(), 5);

    // Find positions
    auto pos = [&order](GateId id) {
        return std::find(order.begin(), order.end(), id) - order.begin();
    };

    // Check dependencies are satisfied
    EXPECT_LT(pos(0), pos(2));  // H(0) before CNOT
    EXPECT_LT(pos(1), pos(2));  // H(1) before CNOT
    EXPECT_LT(pos(2), pos(3));  // CNOT before X(0)
    EXPECT_LT(pos(2), pos(4));  // CNOT before X(1)
}

// =============================================================================
// Layer Tests
// =============================================================================

TEST(DAGLayerTest, EmptyDAGHasNoLayers) {
    DAG dag(2);
    auto layers = dag.layers();
    EXPECT_TRUE(layers.empty());
}

TEST(DAGLayerTest, SingleNodeIsOneLayer) {
    DAG dag(1);
    dag.addGate(Gate::h(0));

    auto layers = dag.layers();
    EXPECT_EQ(layers.size(), 1);
    EXPECT_EQ(layers[0].size(), 1);
    EXPECT_EQ(layers[0][0], 0);
}

TEST(DAGLayerTest, ParallelGatesInSameLayer) {
    DAG dag(3);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::h(1));
    dag.addGate(Gate::h(2));

    auto layers = dag.layers();
    EXPECT_EQ(layers.size(), 1);
    EXPECT_EQ(layers[0].size(), 3);
}

TEST(DAGLayerTest, LinearChainHasSeparateLayers) {
    DAG dag(1);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::x(0));
    dag.addGate(Gate::z(0));

    auto layers = dag.layers();
    EXPECT_EQ(layers.size(), 3);
    EXPECT_EQ(layers[0].size(), 1);
    EXPECT_EQ(layers[1].size(), 1);
    EXPECT_EQ(layers[2].size(), 1);
}

TEST(DAGLayerTest, BellCircuitLayers) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::cnot(0, 1));

    auto layers = dag.layers();
    EXPECT_EQ(layers.size(), 2);
    EXPECT_EQ(layers[0].size(), 1);  // H
    EXPECT_EQ(layers[1].size(), 1);  // CNOT
}

TEST(DAGLayerTest, ComplexCircuitLayers) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // Layer 0
    dag.addGate(Gate::h(1));       // Layer 0
    dag.addGate(Gate::cnot(0, 1)); // Layer 1
    dag.addGate(Gate::x(0));       // Layer 2
    dag.addGate(Gate::x(1));       // Layer 2

    auto layers = dag.layers();
    EXPECT_EQ(layers.size(), 3);
    EXPECT_EQ(layers[0].size(), 2);  // Two H gates
    EXPECT_EQ(layers[1].size(), 1);  // CNOT
    EXPECT_EQ(layers[2].size(), 2);  // Two X gates
}

// =============================================================================
// Depth Tests
// =============================================================================

TEST(DAGDepthTest, EmptyDAGHasDepthZero) {
    DAG dag(2);
    EXPECT_EQ(dag.depth(), 0);
}

TEST(DAGDepthTest, SingleGateHasDepthOne) {
    DAG dag(1);
    dag.addGate(Gate::h(0));
    EXPECT_EQ(dag.depth(), 1);
}

TEST(DAGDepthTest, ParallelGatesHaveDepthOne) {
    DAG dag(3);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::h(1));
    dag.addGate(Gate::h(2));
    EXPECT_EQ(dag.depth(), 1);
}

TEST(DAGDepthTest, LinearChainDepth) {
    DAG dag(1);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::x(0));
    dag.addGate(Gate::z(0));
    EXPECT_EQ(dag.depth(), 3);
}

TEST(DAGDepthTest, BellCircuitDepth) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::cnot(0, 1));
    EXPECT_EQ(dag.depth(), 2);
}

// =============================================================================
// Edge Query Tests
// =============================================================================

TEST(DAGEdgeTest, HasEdgeReturnsCorrectly) {
    DAG dag(1);
    dag.addGate(Gate::h(0));  // 0
    dag.addGate(Gate::x(0));  // 1

    EXPECT_TRUE(dag.hasEdge(0, 1));
    EXPECT_FALSE(dag.hasEdge(1, 0));
    EXPECT_FALSE(dag.hasEdge(0, 2));
}

TEST(DAGEdgeTest, EdgesReturnsAllEdges) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // 0
    dag.addGate(Gate::h(1));       // 1
    dag.addGate(Gate::cnot(0, 1)); // 2

    auto edges = dag.edges();
    EXPECT_EQ(edges.size(), 2);

    // Check both edges exist
    bool has_0_2 = false, has_1_2 = false;
    for (const auto& [from, to] : edges) {
        if (from == 0 && to == 2) has_0_2 = true;
        if (from == 1 && to == 2) has_1_2 = true;
    }
    EXPECT_TRUE(has_0_2);
    EXPECT_TRUE(has_1_2);
}

// =============================================================================
// Node Removal Tests
// =============================================================================

TEST(DAGRemoveTest, RemoveNodeDecreaseCount) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::x(1));

    EXPECT_EQ(dag.numNodes(), 2);
    dag.removeNode(0);
    EXPECT_EQ(dag.numNodes(), 1);
    EXPECT_FALSE(dag.hasNode(0));
    EXPECT_TRUE(dag.hasNode(1));
}

TEST(DAGRemoveTest, RemoveNodeThrowsOnInvalidId) {
    DAG dag(2);
    dag.addGate(Gate::h(0));

    EXPECT_THROW(dag.removeNode(1), std::out_of_range);
    EXPECT_THROW(dag.removeNode(100), std::out_of_range);
}

TEST(DAGRemoveTest, RemoveMiddleNodeReconnects) {
    DAG dag(1);
    dag.addGate(Gate::h(0));  // 0
    dag.addGate(Gate::x(0));  // 1
    dag.addGate(Gate::z(0));  // 2

    // Remove middle node (X)
    dag.removeNode(1);

    // H should now connect directly to Z
    EXPECT_TRUE(dag.hasEdge(0, 2));
    EXPECT_EQ(dag.node(0).successors().size(), 1);
    EXPECT_EQ(dag.node(0).successors()[0], 2);
    EXPECT_EQ(dag.node(2).predecessors().size(), 1);
    EXPECT_EQ(dag.node(2).predecessors()[0], 0);
}

TEST(DAGRemoveTest, RemoveSourceReconnects) {
    DAG dag(1);
    dag.addGate(Gate::h(0));  // 0
    dag.addGate(Gate::x(0));  // 1

    dag.removeNode(0);

    // X should now be a source
    EXPECT_TRUE(dag.node(1).isSource());
    EXPECT_EQ(dag.sources().size(), 1);
    EXPECT_EQ(dag.sources()[0], 1);
}

TEST(DAGRemoveTest, RemoveSinkReconnects) {
    DAG dag(1);
    dag.addGate(Gate::h(0));  // 0
    dag.addGate(Gate::x(0));  // 1

    dag.removeNode(1);

    // H should now be a sink
    EXPECT_TRUE(dag.node(0).isSink());
    EXPECT_EQ(dag.sinks().size(), 1);
    EXPECT_EQ(dag.sinks()[0], 0);
}

// =============================================================================
// Circuit Conversion Tests
// =============================================================================

TEST(DAGConversionTest, FromCircuitCreatesCorrectNodes) {
    Circuit circuit(2);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::cnot(0, 1));

    DAG dag = DAG::fromCircuit(circuit);

    EXPECT_EQ(dag.numQubits(), 2);
    EXPECT_EQ(dag.numNodes(), 2);
    EXPECT_EQ(dag.node(0).gate().type(), GateType::H);
    EXPECT_EQ(dag.node(1).gate().type(), GateType::CNOT);
}

TEST(DAGConversionTest, FromCircuitCreatesCorrectDependencies) {
    Circuit circuit(2);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(1));
    circuit.addGate(Gate::cnot(0, 1));

    DAG dag = DAG::fromCircuit(circuit);

    // CNOT depends on both H gates
    EXPECT_EQ(dag.node(2).predecessors().size(), 2);
    EXPECT_TRUE(dag.hasEdge(0, 2));
    EXPECT_TRUE(dag.hasEdge(1, 2));
}

TEST(DAGConversionTest, ToCircuitPreservesGates) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::cnot(0, 1));

    Circuit circuit = dag.toCircuit();

    EXPECT_EQ(circuit.numQubits(), 2);
    EXPECT_EQ(circuit.numGates(), 2);
    EXPECT_EQ(circuit.gate(0).type(), GateType::H);
    EXPECT_EQ(circuit.gate(1).type(), GateType::CNOT);
}

TEST(DAGConversionTest, ToCircuitPreservesTopologicalOrder) {
    DAG dag(2);
    dag.addGate(Gate::h(0));       // 0
    dag.addGate(Gate::h(1));       // 1
    dag.addGate(Gate::cnot(0, 1)); // 2

    Circuit circuit = dag.toCircuit();

    // CNOT must come after both H gates
    // Gates 0 and 1 can be in any order, but gate 2 (CNOT) must be last
    EXPECT_EQ(circuit.gate(2).type(), GateType::CNOT);
}

TEST(DAGConversionTest, RoundTripPreservesStructure) {
    Circuit original(3);
    original.addGate(Gate::h(0));
    original.addGate(Gate::h(1));
    original.addGate(Gate::h(2));
    original.addGate(Gate::cnot(0, 1));
    original.addGate(Gate::cnot(1, 2));
    original.addGate(Gate::rz(0, constants::PI_4));

    DAG dag = DAG::fromCircuit(original);
    Circuit recovered = dag.toCircuit();

    EXPECT_EQ(recovered.numQubits(), original.numQubits());
    EXPECT_EQ(recovered.numGates(), original.numGates());
    EXPECT_EQ(recovered.depth(), original.depth());
}

TEST(DAGConversionTest, ParameterizedGatesPreserved) {
    Circuit original(1);
    original.addGate(Gate::rx(0, 1.5));
    original.addGate(Gate::ry(0, 2.5));
    original.addGate(Gate::rz(0, 3.5));

    DAG dag = DAG::fromCircuit(original);
    Circuit recovered = dag.toCircuit();

    EXPECT_EQ(recovered.gate(0).parameter().value(), 1.5);
    EXPECT_EQ(recovered.gate(1).parameter().value(), 2.5);
    EXPECT_EQ(recovered.gate(2).parameter().value(), 3.5);
}

// =============================================================================
// Clear Tests
// =============================================================================

TEST(DAGClearTest, ClearRemovesAllNodes) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::cnot(0, 1));

    EXPECT_EQ(dag.numNodes(), 2);
    dag.clear();
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_TRUE(dag.empty());
}

TEST(DAGClearTest, ClearAllowsNewAdditions) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.clear();

    GateId id = dag.addGate(Gate::x(1));
    EXPECT_EQ(id, 0);  // IDs reset
    EXPECT_EQ(dag.numNodes(), 1);
}

// =============================================================================
// ToString Tests
// =============================================================================

TEST(DAGToStringTest, FormatsCorrectly) {
    DAG dag(2);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::cnot(0, 1));

    std::string str = dag.toString();
    EXPECT_TRUE(str.find("2 qubits") != std::string::npos);
    EXPECT_TRUE(str.find("2 nodes") != std::string::npos);
    EXPECT_TRUE(str.find("H q[0]") != std::string::npos);
    EXPECT_TRUE(str.find("CNOT") != std::string::npos);
}

// =============================================================================
// Node IDs Tests
// =============================================================================

TEST(DAGNodeIdsTest, ReturnsAllIds) {
    DAG dag(3);
    dag.addGate(Gate::h(0));
    dag.addGate(Gate::h(1));
    dag.addGate(Gate::h(2));

    auto ids = dag.nodeIds();
    EXPECT_EQ(ids.size(), 3);

    std::unordered_set<GateId> id_set(ids.begin(), ids.end());
    EXPECT_TRUE(id_set.count(0));
    EXPECT_TRUE(id_set.count(1));
    EXPECT_TRUE(id_set.count(2));
}

// =============================================================================
// GHZ Circuit Tests (Integration)
// =============================================================================

TEST(DAGIntegrationTest, GHZCircuit) {
    // GHZ state: H(0), CNOT(0,1), CNOT(1,2), CNOT(2,3)
    Circuit circuit(4);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::cnot(0, 1));
    circuit.addGate(Gate::cnot(1, 2));
    circuit.addGate(Gate::cnot(2, 3));

    DAG dag = DAG::fromCircuit(circuit);

    // Check structure
    EXPECT_EQ(dag.numNodes(), 4);
    EXPECT_EQ(dag.depth(), 4);  // Linear chain

    // Check dependencies form a chain
    EXPECT_TRUE(dag.hasEdge(0, 1));
    EXPECT_TRUE(dag.hasEdge(1, 2));
    EXPECT_TRUE(dag.hasEdge(2, 3));

    // Verify topological order
    auto order = dag.topologicalOrder();
    EXPECT_EQ(order[0], 0);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 2);
    EXPECT_EQ(order[3], 3);
}

}  // namespace
}  // namespace qopt::ir
