// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Rylan Malarchick

/**
 * @file test_passes.cpp
 * @brief Unit tests for optimization passes
 *
 * Tests for Pass, PassManager, CancellationPass, RotationMergePass,
 * IdentityEliminationPass, and CommutationPass.
 */

#include "passes/Pass.hpp"
#include "passes/PassManager.hpp"
#include "passes/CancellationPass.hpp"
#include "passes/RotationMergePass.hpp"
#include "passes/IdentityEliminationPass.hpp"
#include "passes/CommutationPass.hpp"
#include "ir/Circuit.hpp"
#include "ir/DAG.hpp"
#include "ir/Gate.hpp"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>

using namespace qopt;
using namespace qopt::ir;
using namespace qopt::passes;

// =============================================================================
// Pass Base Class Tests
// =============================================================================

// Concrete pass for testing base class
class TestPass : public Pass {
public:
    std::string name() const override { return "TestPass"; }
    void run(DAG& /*dag*/) override {
        resetStatistics();
        gates_removed_ = 5;
        gates_added_ = 2;
    }
};

TEST(PassTest, NameReturnsCorrectValue) {
    TestPass pass;
    EXPECT_EQ(pass.name(), "TestPass");
}

TEST(PassTest, InitialStatisticsAreZero) {
    TestPass pass;
    EXPECT_EQ(pass.gatesRemoved(), 0);
    EXPECT_EQ(pass.gatesAdded(), 0);
    EXPECT_EQ(pass.netChange(), 0);
}

TEST(PassTest, RunUpdatesStatistics) {
    TestPass pass;
    DAG dag(2);
    pass.run(dag);
    
    EXPECT_EQ(pass.gatesRemoved(), 5);
    EXPECT_EQ(pass.gatesAdded(), 2);
    EXPECT_EQ(pass.netChange(), -3);  // 2 - 5 = -3
}

TEST(PassTest, ResetStatisticsClearsCounters) {
    TestPass pass;
    DAG dag(2);
    pass.run(dag);
    pass.resetStatistics();
    
    EXPECT_EQ(pass.gatesRemoved(), 0);
    EXPECT_EQ(pass.gatesAdded(), 0);
}

// =============================================================================
// PassManager Tests
// =============================================================================

TEST(PassManagerTest, DefaultConstructorCreatesEmptyPipeline) {
    PassManager pm;
    EXPECT_EQ(pm.numPasses(), 0);
    EXPECT_TRUE(pm.empty());
}

TEST(PassManagerTest, AddPassIncrementsCount) {
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    EXPECT_EQ(pm.numPasses(), 1);
    EXPECT_FALSE(pm.empty());
    
    pm.addPass(std::make_unique<RotationMergePass>());
    EXPECT_EQ(pm.numPasses(), 2);
}

TEST(PassManagerTest, ClearRemovesAllPasses) {
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.addPass(std::make_unique<RotationMergePass>());
    pm.clear();
    
    EXPECT_EQ(pm.numPasses(), 0);
    EXPECT_TRUE(pm.empty());
}

TEST(PassManagerTest, RunOnEmptyDAG) {
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    
    DAG dag(2);
    pm.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pm.statistics().initial_gate_count, 0);
    EXPECT_EQ(pm.statistics().final_gate_count, 0);
}

TEST(PassManagerTest, RunExecutesPassesInOrder) {
    Circuit circuit(2);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));  // Should cancel
    circuit.addGate(Gate::x(1));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    
    DAG dag = DAG::fromCircuit(circuit);
    EXPECT_EQ(dag.numNodes(), 3);
    
    pm.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);  // Only X remains
    EXPECT_EQ(pm.statistics().initial_gate_count, 3);
    EXPECT_EQ(pm.statistics().final_gate_count, 1);
}

TEST(PassManagerTest, StatisticsTracksPerPass) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.addPass(std::make_unique<RotationMergePass>());
    
    DAG dag = DAG::fromCircuit(circuit);
    pm.run(dag);
    
    const auto& stats = pm.statistics();
    EXPECT_EQ(stats.per_pass.size(), 2);
    EXPECT_EQ(std::get<0>(stats.per_pass[0]), "CancellationPass");
    EXPECT_EQ(std::get<0>(stats.per_pass[1]), "RotationMergePass");
}

TEST(PassManagerTest, RunOnCircuitModifiesCircuit) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::x(0));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), 1);  // Only X remains
}

TEST(PassManagerTest, ReductionPercentCalculation) {
    Circuit circuit(1);
    for (int i = 0; i < 10; ++i) {
        circuit.addGate(Gate::h(0));
    }  // 10 H gates -> 5 pairs -> all cancel
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(pm.statistics().initial_gate_count, 10);
    EXPECT_EQ(pm.statistics().final_gate_count, 0);
    EXPECT_DOUBLE_EQ(pm.statistics().reductionPercent(), 100.0);
}

// =============================================================================
// CancellationPass Tests
// =============================================================================

TEST(CancellationPassTest, NameReturnsCorrectValue) {
    CancellationPass pass;
    EXPECT_EQ(pass.name(), "CancellationPass");
}

TEST(CancellationPassTest, EmptyDAGRemainsEmpty) {
    CancellationPass pass;
    DAG dag(2);
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 0);
}

TEST(CancellationPassTest, HadamardCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, PauliXCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::x(0));
    circuit.addGate(Gate::x(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, PauliYCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::y(0));
    circuit.addGate(Gate::y(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, PauliZCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::z(0));
    circuit.addGate(Gate::z(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, SSdgCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::s(0));
    circuit.addGate(Gate::sdg(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, SdgSCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::sdg(0));
    circuit.addGate(Gate::s(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, TTdgCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::t(0));
    circuit.addGate(Gate::tdg(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, TdgTCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::tdg(0));
    circuit.addGate(Gate::t(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, CNOTCancellation) {
    Circuit circuit(2);
    circuit.addGate(Gate::cnot(0, 1));
    circuit.addGate(Gate::cnot(0, 1));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, CZCancellation) {
    Circuit circuit(2);
    circuit.addGate(Gate::cz(0, 1));
    circuit.addGate(Gate::cz(0, 1));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, SWAPCancellation) {
    Circuit circuit(2);
    circuit.addGate(Gate::swap(0, 1));
    circuit.addGate(Gate::swap(0, 1));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, NonAdjacentGatesDoNotCancel) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::x(0));  // Intervening gate
    circuit.addGate(Gate::h(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 3);  // No cancellation
    EXPECT_EQ(pass.gatesRemoved(), 0);
}

TEST(CancellationPassTest, DifferentQubitsDoNotCancel) {
    Circuit circuit(2);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(1));  // Different qubit
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);  // No cancellation
    EXPECT_EQ(pass.gatesRemoved(), 0);
}

TEST(CancellationPassTest, MultiplePairsCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::x(0));
    circuit.addGate(Gate::x(0));
    circuit.addGate(Gate::z(0));
    circuit.addGate(Gate::z(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 6);
}

TEST(CancellationPassTest, PartialCancellation) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));  // Cancel
    circuit.addGate(Gate::x(0));  // Remains
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    EXPECT_EQ(pass.gatesRemoved(), 2);
}

TEST(CancellationPassTest, SDoesNotCancelWithS) {
    Circuit circuit(1);
    circuit.addGate(Gate::s(0));
    circuit.addGate(Gate::s(0));  // S·S = Z, not I
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);  // No cancellation
}

TEST(CancellationPassTest, TDoesNotCancelWithT) {
    Circuit circuit(1);
    circuit.addGate(Gate::t(0));
    circuit.addGate(Gate::t(0));  // T·T = S, not I
    
    DAG dag = DAG::fromCircuit(circuit);
    CancellationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);  // No cancellation
}

// =============================================================================
// RotationMergePass Tests
// =============================================================================

TEST(RotationMergePassTest, NameReturnsCorrectValue) {
    RotationMergePass pass;
    EXPECT_EQ(pass.name(), "RotationMergePass");
}

TEST(RotationMergePassTest, EmptyDAGRemainsEmpty) {
    RotationMergePass pass;
    DAG dag(2);
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(RotationMergePassTest, RzMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(0, constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    EXPECT_EQ(pass.gatesRemoved(), 1);
    
    // Check merged angle
    auto order = dag.topologicalOrder();
    const Gate& g = dag.node(order[0]).gate();
    EXPECT_EQ(g.type(), GateType::Rz);
    EXPECT_NEAR(g.parameter().value(), constants::PI_2, 1e-10);
}

TEST(RotationMergePassTest, RxMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::rx(0, constants::PI_4));
    circuit.addGate(Gate::rx(0, constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    
    auto order = dag.topologicalOrder();
    const Gate& g = dag.node(order[0]).gate();
    EXPECT_EQ(g.type(), GateType::Rx);
    EXPECT_NEAR(g.parameter().value(), constants::PI_2, 1e-10);
}

TEST(RotationMergePassTest, RyMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::ry(0, constants::PI_4));
    circuit.addGate(Gate::ry(0, constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    
    auto order = dag.topologicalOrder();
    const Gate& g = dag.node(order[0]).gate();
    EXPECT_EQ(g.type(), GateType::Ry);
    EXPECT_NEAR(g.parameter().value(), constants::PI_2, 1e-10);
}

TEST(RotationMergePassTest, DifferentRotationTypesDoNotMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rx(0, constants::PI_4));  // Different type
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);  // No merge
}

TEST(RotationMergePassTest, DifferentQubitsDoNotMerge) {
    Circuit circuit(2);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(1, constants::PI_4));  // Different qubit
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);  // No merge
}

TEST(RotationMergePassTest, NonAdjacentRotationsDoNotMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::h(0));  // Intervening gate
    circuit.addGate(Gate::rz(0, constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 3);  // No merge
}

TEST(RotationMergePassTest, MultipleConsecutiveRotationsMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(0, constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    
    auto order = dag.topologicalOrder();
    const Gate& g = dag.node(order[0]).gate();
    EXPECT_NEAR(g.parameter().value(), constants::PI, 1e-10);
}

TEST(RotationMergePassTest, NegativeAnglesMerge) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(0, -constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    
    auto order = dag.topologicalOrder();
    const Gate& g = dag.node(order[0]).gate();
    EXPECT_NEAR(g.parameter().value(), 0.0, 1e-10);
}

TEST(RotationMergePassTest, AngleNormalization) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI));
    circuit.addGate(Gate::rz(0, constants::PI));  // Sum is 2π
    
    DAG dag = DAG::fromCircuit(circuit);
    RotationMergePass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);
    
    auto order = dag.topologicalOrder();
    const Gate& g = dag.node(order[0]).gate();
    // 2π should normalize to 0 or stay as is depending on implementation
    EXPECT_TRUE(std::abs(g.parameter().value()) < 1e-10 ||
                std::abs(std::abs(g.parameter().value()) - 2*constants::PI) < 1e-10);
}

// =============================================================================
// IdentityEliminationPass Tests
// =============================================================================

TEST(IdentityEliminationPassTest, NameReturnsCorrectValue) {
    IdentityEliminationPass pass;
    EXPECT_EQ(pass.name(), "IdentityEliminationPass");
}

TEST(IdentityEliminationPassTest, EmptyDAGRemainsEmpty) {
    IdentityEliminationPass pass;
    DAG dag(2);
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(IdentityEliminationPassTest, RzZeroRemoved) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, 0.0));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
    EXPECT_EQ(pass.gatesRemoved(), 1);
}

TEST(IdentityEliminationPassTest, RxZeroRemoved) {
    Circuit circuit(1);
    circuit.addGate(Gate::rx(0, 0.0));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(IdentityEliminationPassTest, RyZeroRemoved) {
    Circuit circuit(1);
    circuit.addGate(Gate::ry(0, 0.0));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(IdentityEliminationPassTest, Rz2PiRemoved) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, 2.0 * constants::PI));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(IdentityEliminationPassTest, RzNegative2PiRemoved) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, -2.0 * constants::PI));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(IdentityEliminationPassTest, NonZeroRotationPreserved) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 1);  // Preserved
}

TEST(IdentityEliminationPassTest, NonRotationGatesPreserved) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::x(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);  // Preserved
}

TEST(IdentityEliminationPassTest, MixedGatesPartialRemoval) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::rz(0, 0.0));  // Should be removed
    circuit.addGate(Gate::x(0));
    
    DAG dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 2);
    EXPECT_EQ(pass.gatesRemoved(), 1);
}

TEST(IdentityEliminationPassTest, CustomToleranceWorks) {
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, 1e-8));  // Small but not zero
    
    DAG dag = DAG::fromCircuit(circuit);
    
    // Default tolerance (1e-10) should preserve it
    IdentityEliminationPass pass1(constants::TOLERANCE);
    pass1.run(dag);
    EXPECT_EQ(dag.numNodes(), 1);
    
    // Larger tolerance should remove it
    dag = DAG::fromCircuit(circuit);
    IdentityEliminationPass pass2(1e-6);
    pass2.run(dag);
    EXPECT_EQ(dag.numNodes(), 0);
}

// =============================================================================
// CommutationPass Tests
// =============================================================================

TEST(CommutationPassTest, NameReturnsCorrectValue) {
    CommutationPass pass;
    EXPECT_EQ(pass.name(), "CommutationPass");
}

TEST(CommutationPassTest, EmptyDAGRemainsEmpty) {
    CommutationPass pass;
    DAG dag(2);
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), 0);
}

TEST(CommutationPassTest, DisjointGatesCommute) {
    // Gates on different qubits should be independent
    Circuit circuit(2);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::x(1));
    
    DAG dag = DAG::fromCircuit(circuit);
    CommutationPass pass;
    pass.run(dag);
    
    // Both gates should still be present
    EXPECT_EQ(dag.numNodes(), 2);
}

TEST(CommutationPassTest, PreservesCircuitSemantics) {
    // After commutation, circuit should have same gates
    Circuit circuit(2);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::z(0));
    circuit.addGate(Gate::cnot(0, 1));
    
    DAG dag = DAG::fromCircuit(circuit);
    std::size_t initial_count = dag.numNodes();
    
    CommutationPass pass;
    pass.run(dag);
    
    EXPECT_EQ(dag.numNodes(), initial_count);  // Same gate count
}

// =============================================================================
// Integration Tests - PassManager with Multiple Passes
// =============================================================================

TEST(IntegrationTest, CancellationThenIdentityElimination) {
    // Rz(π/4) followed by Rz(-π/4) should merge to Rz(0), then be eliminated
    Circuit circuit(1);
    circuit.addGate(Gate::rz(0, constants::PI_4));
    circuit.addGate(Gate::rz(0, -constants::PI_4));
    
    PassManager pm;
    pm.addPass(std::make_unique<RotationMergePass>());
    pm.addPass(std::make_unique<IdentityEliminationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), 0);
}

TEST(IntegrationTest, FullOptimizationPipeline) {
    Circuit circuit(2);
    // H H should cancel
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    // Rz gates should merge then be eliminated
    circuit.addGate(Gate::rz(1, constants::PI_4));
    circuit.addGate(Gate::rz(1, -constants::PI_4));
    // X remains
    circuit.addGate(Gate::x(0));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.addPass(std::make_unique<RotationMergePass>());
    pm.addPass(std::make_unique<IdentityEliminationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), 1);  // Only X remains
}

TEST(IntegrationTest, StatisticsToString) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.run(circuit);
    
    std::string stats_str = pm.statistics().toString();
    EXPECT_FALSE(stats_str.empty());
    EXPECT_NE(stats_str.find("CancellationPass"), std::string::npos);
}

TEST(IntegrationTest, LargeCircuitOptimization) {
    Circuit circuit(4);
    
    // Add many cancellable pairs
    for (int i = 0; i < 50; ++i) {
        circuit.addGate(Gate::h(static_cast<QubitIndex>(i % 4)));
        circuit.addGate(Gate::h(static_cast<QubitIndex>(i % 4)));
    }
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), 0);  // All cancelled
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(EdgeCaseTest, SingleGateCircuit) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.addPass(std::make_unique<RotationMergePass>());
    pm.addPass(std::make_unique<IdentityEliminationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), 1);  // H preserved
}

TEST(EdgeCaseTest, AllGatesCancel) {
    Circuit circuit(1);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::h(0));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), 0);
}

TEST(EdgeCaseTest, NoOptimizationOpportunities) {
    Circuit circuit(3);
    circuit.addGate(Gate::h(0));
    circuit.addGate(Gate::x(1));
    circuit.addGate(Gate::z(2));
    circuit.addGate(Gate::cnot(0, 1));
    
    PassManager pm;
    pm.addPass(std::make_unique<CancellationPass>());
    pm.addPass(std::make_unique<RotationMergePass>());
    pm.addPass(std::make_unique<IdentityEliminationPass>());
    
    std::size_t initial = circuit.numGates();
    pm.run(circuit);
    
    EXPECT_EQ(circuit.numGates(), initial);  // No change
}
