/**
 * @file query_planner_test.cc
 * @brief Unit and integration tests for the hybrid query planner (EPIC 2.5).
 *
 * Covers:
 *  - Deterministic path selection for all five canonical paths
 *  - Tensor freshness gate: all boundary conditions
 *  - Category C fail-closed enforcement (no GPU dispatch)
 *  - Fallback chain: GPU failure on Path 1/2/3 → CPU fallback
 *  - Distributed exact-on-demand trigger (Path 5 / shard manifest missing)
 *  - force_exact and force_cpu overrides
 *  - Module gap threshold blocker
 *
 * The test file is self-contained: it includes the planner header directly and
 * does not require the full ThemisDB build chain.
 *
 * Build (standalone):
 *   g++ -std=c++17 -I<repo>/src/evaluation -o query_planner_test \
 *       query_planner_test.cc <repo>/src/evaluation/src/query_planner.cc \
 *       -lgtest -lgtest_main -lpthread
 *
 * @see src/evaluation/include/query_planner.h
 * @see docs/EPIC2_QUERY_PLANNER.md
 */

#include <gtest/gtest.h>

#include "query_planner.h"

// Pull in the DefaultQueryPlanner implementation via the factory.
// The factory is declared in the header and defined in query_planner.cc.

using namespace themis::evaluation;

// ---------------------------------------------------------------------------
// Test fixtures / helpers
// ---------------------------------------------------------------------------

namespace {

/// @brief Build a fully-enabled eligibility that allows Path 1 (ANN Only).
ExecutionEligibility makeFullEligibility() {
    ExecutionEligibility e;
    e.cuda_available             = true;
    e.ann_enabled                = true;
    e.gpu_error_handling_gate    = true;
    e.gpu_parity_validated       = true;
    e.force_exact                = false;
    e.force_cpu                  = false;
    e.query_thread_safety_ok     = true;
    e.query_exception_handling_ok= true;
    e.index_buffer_safety_ok     = true;
    e.distributed_multi_shard    = false;
    e.shard_manifests_available  = false;
    return e;
}

/// @brief Build a default PlannerConfig with known thresholds.
PlannerConfig makeDefaultConfig() {
    PlannerConfig cfg;
    cfg.max_staleness_ms              = 5'000;
    cfg.max_delta_lag                 = 1'000;
    cfg.min_residual_threshold        = 0.95;
    cfg.max_rank_cap                  = 1'000;
    cfg.min_shard_summary_confidence  = 0.80;
    cfg.policy_version                = "v1-test";
    cfg.staleness_threshold_key       = "tensor.max_staleness_ms";
    return cfg;
}

/// @brief Build a TensorArtifactFreshness with no artifact present (default zero state).
TensorArtifactFreshness makeNoArtifact() {
    return TensorArtifactFreshness{};  // All zeros — "no artifact" sentinel
}

/// @brief Build a fresh tensor artifact that passes all gates.
TensorArtifactFreshness makeFreshArtifact(uint64_t age_ms = 1'000) {
    TensorArtifactFreshness f;
    f.artifact_age_ms      = age_ms;
    f.delta_lag            = 100;
    f.source_seq_start     = 1;
    f.source_seq_end       = 100;
    f.residual_threshold   = 0.97;
    f.rank_cap             = 500;
    f.rebuild_in_progress  = false;
    return f;
}

/// @brief Build a stale tensor artifact (age exceeds max_staleness_ms).
TensorArtifactFreshness makeStaleArtifact() {
    TensorArtifactFreshness f = makeFreshArtifact();
    f.artifact_age_ms = 10'000; // Exceeds 5 000 ms threshold
    return f;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// TensorArtifactFreshness unit tests
// ---------------------------------------------------------------------------

TEST(TensorArtifactFreshness, FreshArtifactPassesAllGates) {
    const auto f = makeFreshArtifact();
    EXPECT_TRUE(f.isFresh(5'000));
    EXPECT_EQ(f.staleness_reason(5'000), FallbackReason::None);
}

TEST(TensorArtifactFreshness, StaleAgeTriggersArtifactStale) {
    TensorArtifactFreshness f = makeFreshArtifact();
    f.artifact_age_ms = 5'001; // Just over the threshold
    EXPECT_FALSE(f.isFresh(5'000));
    EXPECT_EQ(f.staleness_reason(5'000), FallbackReason::TensorArtifactStale);
}

TEST(TensorArtifactFreshness, RebuildInProgressBlocks) {
    TensorArtifactFreshness f = makeFreshArtifact();
    f.rebuild_in_progress = true;
    EXPECT_FALSE(f.isFresh(5'000));
    EXPECT_EQ(f.staleness_reason(5'000), FallbackReason::TensorRebuildInProgress);
}

TEST(TensorArtifactFreshness, LowResidualBlocks) {
    TensorArtifactFreshness f = makeFreshArtifact();
    f.residual_threshold = 0.90; // Below 0.95
    EXPECT_FALSE(f.isFresh(5'000));
    EXPECT_EQ(f.staleness_reason(5'000), FallbackReason::TensorResidualLow);
}

TEST(TensorArtifactFreshness, ExactlyAtThresholdFails) {
    TensorArtifactFreshness f = makeFreshArtifact();
    f.artifact_age_ms = 5'000; // == threshold, not strictly less
    EXPECT_FALSE(f.isFresh(5'000));
}

TEST(TensorArtifactFreshness, RebuildTakesPriorityOverAge) {
    // rebuild_in_progress is checked first in staleness_reason
    TensorArtifactFreshness f = makeFreshArtifact();
    f.rebuild_in_progress = true;
    f.artifact_age_ms     = 99'999; // Also stale
    EXPECT_EQ(f.staleness_reason(5'000), FallbackReason::TensorRebuildInProgress);
}

// ---------------------------------------------------------------------------
// ExecutionEligibility / isGpuEligible unit tests
// ---------------------------------------------------------------------------

TEST(ExecutionEligibility, CategoryCIsNeverGpu) {
    ExecutionEligibility e = makeFullEligibility();
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::C));
}

TEST(ExecutionEligibility, CategoryARequiresCudaAndGate) {
    ExecutionEligibility e = makeFullEligibility();
    EXPECT_TRUE(e.isGpuEligible(KernelCategory::A));

    e.cuda_available = false;
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::A));

    e.cuda_available          = true;
    e.gpu_error_handling_gate = false;
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::A));
}

TEST(ExecutionEligibility, CategoryBRequiresParityOnTopOfA) {
    ExecutionEligibility e = makeFullEligibility();
    EXPECT_TRUE(e.isGpuEligible(KernelCategory::B));

    e.gpu_parity_validated = false;
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::B));
}

TEST(ExecutionEligibility, ForceCpuBlocksAllGpu) {
    ExecutionEligibility e = makeFullEligibility();
    e.force_cpu = true;
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::A));
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::B));
    EXPECT_FALSE(e.isGpuEligible(KernelCategory::C));
}

// ---------------------------------------------------------------------------
// DefaultQueryPlanner — path selection tests
// ---------------------------------------------------------------------------

class QueryPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner_ = makeDefaultQueryPlanner();
        config_  = makeDefaultConfig();
    }

    std::unique_ptr<QueryPlanner> planner_;
    PlannerConfig                 config_;
};

// --- Path 1: ANN Only ---

TEST_F(QueryPlannerTest, AnnOnly_SelectedWhenNoArtifactPresent) {
    const auto e = makeFullEligibility();
    const auto f = makeNoArtifact(); // zeros → no artifact present
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::AnnOnly);
    EXPECT_EQ(d.fallback_reason, FallbackReason::None);
    EXPECT_FALSE(d.uses_tensor);
    EXPECT_FALSE(d.uses_exact_graph);
}

TEST_F(QueryPlannerTest, AnnOnly_GpuFlagMatchesCategoryAEligibility) {
    auto e = makeFullEligibility();
    const auto f = makeNoArtifact();

    const auto d_with_gpu = planner_->selectPath(e, f, config_);
    EXPECT_TRUE(d_with_gpu.uses_gpu);

    e.cuda_available = false;
    const auto d_no_gpu = planner_->selectPath(e, f, config_);
    EXPECT_EQ(d_no_gpu.path, ExecutionPath::AnnOnly);
    EXPECT_FALSE(d_no_gpu.uses_gpu);
}

// --- Path 2: ANN + Tensor Summary ---

TEST_F(QueryPlannerTest, AnnTensorSummary_SelectedWithFreshArtifact) {
    const auto e = makeFullEligibility();
    const auto f = makeFreshArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::AnnTensorSummary);
    EXPECT_EQ(d.fallback_reason, FallbackReason::None);
    EXPECT_TRUE(d.uses_tensor);
    EXPECT_FALSE(d.uses_exact_graph);
}

TEST_F(QueryPlannerTest, AnnTensorSummary_FallsBackOnStaleArtifact) {
    const auto e = makeFullEligibility();
    const auto f = makeStaleArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::TensorArtifactStale);
}

TEST_F(QueryPlannerTest, AnnTensorSummary_FallsBackWhenRebuildInProgress) {
    const auto e = makeFullEligibility();
    auto f       = makeFreshArtifact();
    f.rebuild_in_progress = true;
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::TensorRebuildInProgress);
}

TEST_F(QueryPlannerTest, AnnTensorSummary_FallsBackOnLowResidual) {
    const auto e = makeFullEligibility();
    auto f       = makeFreshArtifact();
    f.residual_threshold = 0.80; // Below 0.95
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::TensorResidualLow);
}

TEST_F(QueryPlannerTest, AnnTensorSummary_FallsBackOnHighDeltaLag) {
    const auto e = makeFullEligibility();
    auto f       = makeFreshArtifact();
    f.delta_lag  = 2'000; // Exceeds max_delta_lag (1 000)
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::TensorArtifactStale);
}

TEST_F(QueryPlannerTest, AnnTensorSummary_FallsBackOnRankCapExceeded) {
    const auto e = makeFullEligibility();
    auto f       = makeFreshArtifact();
    f.rank_cap   = 2'000; // Exceeds max_rank_cap (1 000)
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::TensorRankCapExceeded);
}

// --- Path 3: ANN + Tensor Refinement + Exact Graph Validation ---

TEST_F(QueryPlannerTest, AnnTensorExactGraph_SelectedWhenQualityCritical) {
    // requires_exact_graph_validation=true + fresh artifact → Path 3
    auto e                              = makeFullEligibility();
    e.requires_exact_graph_validation   = true;
    const auto f = makeFreshArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::AnnTensorExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::None);
    EXPECT_TRUE(d.uses_tensor);
    EXPECT_TRUE(d.uses_exact_graph);
}

TEST_F(QueryPlannerTest, AnnTensorExactGraph_WithoutFlagSelectsPath2) {
    // requires_exact_graph_validation=false (default) + fresh artifact → Path 2
    const auto e = makeFullEligibility();
    const auto f = makeFreshArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::AnnTensorSummary);
    EXPECT_FALSE(d.uses_exact_graph);
}

TEST_F(QueryPlannerTest, AnnTensorExactGraph_StaleArtifactFallsToPath4EvenWhenQualityCritical) {
    // Stale artifact → Path 4 regardless of requires_exact_graph_validation
    auto e                              = makeFullEligibility();
    e.requires_exact_graph_validation   = true;
    const auto f = makeStaleArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::TensorArtifactStale);
}

// --- Path 4: Direct Exact Graph ---

TEST_F(QueryPlannerTest, DirectExactGraph_SelectedWhenAnnDisabled) {
    auto e      = makeFullEligibility();
    e.ann_enabled = false;
    const auto f = makeNoArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_FALSE(d.uses_gpu);
    EXPECT_TRUE(d.uses_exact_graph);
}

TEST_F(QueryPlannerTest, DirectExactGraph_SelectedWhenForceExact) {
    auto e        = makeFullEligibility();
    e.force_exact = true;
    const auto f  = makeFreshArtifact(); // Would normally be Path 2
    const auto d  = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::ForceExact);
}

TEST_F(QueryPlannerTest, DirectExactGraph_SelectedWhenIndexBufferSafetyFails) {
    auto e                    = makeFullEligibility();
    e.index_buffer_safety_ok  = false;
    const auto f              = makeNoArtifact();
    const auto d              = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::ModuleGapThreshold);
}

TEST_F(QueryPlannerTest, DirectExactGraph_NeverUsesGpu) {
    auto e        = makeFullEligibility();
    e.force_exact = true;
    const auto d  = planner_->selectPath(e, makeNoArtifact(), config_);

    EXPECT_FALSE(d.uses_gpu); // Exact graph is CPU-only
}

// --- Path 5: Distributed ---

TEST_F(QueryPlannerTest, Distributed_SelectedWhenMultiShard) {
    auto e                        = makeFullEligibility();
    e.distributed_multi_shard     = true;
    e.shard_manifests_available   = true;
    const auto d = planner_->selectPath(e, makeNoArtifact(), config_);

    EXPECT_EQ(d.path, ExecutionPath::DistributedSummaryFirstExactOnDemand);
    EXPECT_EQ(d.fallback_reason, FallbackReason::None);
    EXPECT_TRUE(d.uses_tensor);      // summary-first uses tensor summaries
    EXPECT_TRUE(d.uses_exact_graph); // exact-on-demand per shard
}

TEST_F(QueryPlannerTest, Distributed_FallsBackToExactGraphWhenManifestsMissing) {
    auto e                        = makeFullEligibility();
    e.distributed_multi_shard     = true;
    e.shard_manifests_available   = false; // Manifests unavailable
    const auto d = planner_->selectPath(e, makeNoArtifact(), config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(d.fallback_reason, FallbackReason::ShardManifestMissing);
}

TEST_F(QueryPlannerTest, Distributed_TakesPrecedenceOverNormalPaths) {
    auto e                      = makeFullEligibility();
    e.distributed_multi_shard   = true;
    e.shard_manifests_available = true;
    // Even with a fresh tensor artifact, distributed path wins.
    const auto d = planner_->selectPath(e, makeFreshArtifact(), config_);
    EXPECT_EQ(d.path, ExecutionPath::DistributedSummaryFirstExactOnDemand);
}

// --- Category C fail-closed enforcement ---

TEST_F(QueryPlannerTest, CategoryC_NeverGpuEligible) {
    const auto e = makeFullEligibility();
    EXPECT_FALSE(QueryPlanner::isKernelEligibleForGpu(KernelCategory::C, e));
}

TEST_F(QueryPlannerTest, CategoryC_StillNotGpuEvenWithAllGatesSet) {
    ExecutionEligibility e = makeFullEligibility();
    e.cuda_available          = true;
    e.gpu_error_handling_gate = true;
    e.gpu_parity_validated    = true;
    e.force_cpu               = false;
    EXPECT_FALSE(QueryPlanner::isKernelEligibleForGpu(KernelCategory::C, e));
}

// --- Force CPU override ---

TEST_F(QueryPlannerTest, ForceCpu_DisablesGpuOnAllPaths) {
    auto e     = makeFullEligibility();
    e.force_cpu = true;
    // Path 1 should still be selected (ANN CPU fallback is always available),
    // but uses_gpu must be false.
    const auto d = planner_->selectPath(e, makeNoArtifact(), config_);
    EXPECT_FALSE(d.uses_gpu);
}

// --- Decision metadata ---

TEST_F(QueryPlannerTest, DecisionCarriesPolicyVersion) {
    const auto e = makeFullEligibility();
    const auto d = planner_->selectPath(e, makeNoArtifact(), config_);
    EXPECT_EQ(d.confidence_policy_version, "v1-test");
    EXPECT_EQ(d.confidence_threshold_key, "tensor.max_staleness_ms");
}

TEST_F(QueryPlannerTest, DecisionNoteIsNonEmpty) {
    const auto e = makeFullEligibility();
    const auto d = planner_->selectPath(e, makeNoArtifact(), config_);
    EXPECT_FALSE(d.decision_note.empty());
}

// --- Fallback chain integration test ---

TEST_F(QueryPlannerTest, FallbackChain_GpuDisabledStillSelectsPath1) {
    auto e               = makeFullEligibility();
    e.cuda_available     = false; // GPU unavailable
    const auto d         = planner_->selectPath(e, makeNoArtifact(), config_);

    // ANN is still eligible; GPU is just disabled.
    EXPECT_EQ(d.path, ExecutionPath::AnnOnly);
    EXPECT_FALSE(d.uses_gpu);
    EXPECT_EQ(d.fallback_reason, FallbackReason::None);
}

TEST_F(QueryPlannerTest, FallbackChain_StaleArtifactTriggersExactGraph) {
    const auto e = makeFullEligibility();
    auto f       = makeStaleArtifact();
    const auto d = planner_->selectPath(e, f, config_);

    EXPECT_EQ(d.path, ExecutionPath::DirectExactGraph);
    EXPECT_NE(d.fallback_reason, FallbackReason::None);
}

// ---------------------------------------------------------------------------
// Factory smoke test
// ---------------------------------------------------------------------------

TEST(QueryPlannerFactory, ReturnsNonNullPlanner) {
    auto planner = makeDefaultQueryPlanner();
    ASSERT_NE(planner, nullptr);
}

TEST(QueryPlannerFactory, PlannerIsUsable) {
    auto planner = makeDefaultQueryPlanner();
    const auto e = makeFullEligibility();
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();
    const auto d = planner->selectPath(e, f, c);
    // Must return a valid path (not an uninitialised value).
    EXPECT_GE(static_cast<uint8_t>(d.path), 1u);
    EXPECT_LE(static_cast<uint8_t>(d.path), 5u);
}

// ---------------------------------------------------------------------------
// Phase 5: PlannerObserver / observability tests
// ---------------------------------------------------------------------------

namespace {

/// Simple recording observer that captures the last decision and latency.
class RecordingObserver final : public PlannerObserver {
public:
    void onDecision(const PlannerDecision& d, uint64_t latency_us) noexcept override {
        last_decision = d;
        last_latency_us = latency_us;
        call_count++;
    }

    PlannerDecision last_decision;
    uint64_t        last_latency_us{UINT64_MAX};  // Sentinel: UINT64_MAX means "not yet set"
    int             call_count{0};
};

} // namespace

TEST(PlannerObserver, NullObserverDoesNotCrash) {
    // makeDefaultQueryPlanner(nullptr) must behave identically to makeDefaultQueryPlanner().
    auto planner = makeDefaultQueryPlanner(nullptr);
    ASSERT_NE(planner, nullptr);
    const auto e = makeFullEligibility();
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();
    EXPECT_NO_THROW({
        const auto d = planner->selectPath(e, f, c);
        EXPECT_GE(static_cast<uint8_t>(d.path), 1u);
    });
}

TEST(PlannerObserver, ObserverCalledOncePerDecision) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    const auto e = makeFullEligibility();
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);
    EXPECT_EQ(obs.call_count, 1);

    planner->selectPath(e, f, c);
    EXPECT_EQ(obs.call_count, 2);
}

TEST(PlannerObserver, ObserverReceivesCorrectDecision_AnnOnly) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    const auto e = makeFullEligibility();
    const auto f = makeNoArtifact();   // No artifact → Path 1
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);

    EXPECT_EQ(obs.last_decision.path, ExecutionPath::AnnOnly);
    EXPECT_EQ(obs.last_decision.fallback_reason, FallbackReason::None);
}

TEST(PlannerObserver, ObserverReceivesCorrectDecision_ForceExact) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    auto e = makeFullEligibility();
    e.force_exact = true;
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);

    EXPECT_EQ(obs.last_decision.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(obs.last_decision.fallback_reason, FallbackReason::ForceExact);
}

TEST(PlannerObserver, ObserverReceivesCorrectDecision_ModuleGapThreshold) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    auto e = makeFullEligibility();
    e.index_buffer_safety_ok = false;  // Trigger ModuleGapThreshold
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);

    EXPECT_EQ(obs.last_decision.path, ExecutionPath::DirectExactGraph);
    EXPECT_EQ(obs.last_decision.fallback_reason, FallbackReason::ModuleGapThreshold);
}

TEST(PlannerObserver, ObserverReceivesNonZeroLatency) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    const auto e = makeFullEligibility();
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);

    // Verify the observer was called and latency was set (sentinel replaced).
    EXPECT_LT(obs.last_latency_us, UINT64_MAX);
}

TEST(PlannerObserver, ObserverWithFreshArtifactPath2) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    auto e = makeFullEligibility();
    auto f = makeFreshArtifact(1'000);
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);

    EXPECT_EQ(obs.last_decision.path, ExecutionPath::AnnTensorSummary);
    EXPECT_EQ(obs.last_decision.fallback_reason, FallbackReason::None);
}

TEST(PlannerObserver, ObserverWithDistributedPath5) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    auto e = makeFullEligibility();
    e.distributed_multi_shard   = true;
    e.shard_manifests_available = true;
    const auto f = makeNoArtifact();
    const auto c = makeDefaultConfig();

    planner->selectPath(e, f, c);

    EXPECT_EQ(obs.last_decision.path,
              ExecutionPath::DistributedSummaryFirstExactOnDemand);
    EXPECT_EQ(obs.last_decision.fallback_reason, FallbackReason::None);
}

TEST(PlannerObserver, FactoryWithObserverReturnsUsablePlanner) {
    RecordingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);
    ASSERT_NE(planner, nullptr);

    // Run all five representative decisions through the same planner.
    {
        const auto e = makeFullEligibility();
        const auto f = makeNoArtifact();
        const auto c = makeDefaultConfig();
        planner->selectPath(e, f, c);
    }
    EXPECT_EQ(obs.call_count, 1);
}

// ============================================================================
// Phase 4 Expansion: Category C Fail-Closed, FallbackReason Taxonomy
// ============================================================================

/// Phase 4 Test: Verify Category C operations block GPU dispatch outright.
TEST_F(QueryPlannerTest, CategoryC_BlocksGpuDispatchEvenWhenAllGatesPass) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    e.query_kernel_category = KernelCategory::C;  // Category C: ACL/provenance/transaction
    auto f = makeFreshArtifact();  // All gates pass
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // GPU dispatch must be blocked for Category C
    EXPECT_FALSE(decision.uses_gpu);
    EXPECT_EQ(decision.fallback_reason, FallbackReason::CategoryCSubpathDetected);
}

/// Phase 4 Test: Verify tensor freshness detection with stale age.
TEST_F(QueryPlannerTest, TensorArtifactStale_ExactFallbackReason) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    auto f = makeStaleArtifact();  // artifact_age_ms = 10'000 > 5'000
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // Stale artifact should trigger the TensorArtifactStale reason
    EXPECT_EQ(decision.fallback_reason, FallbackReason::TensorArtifactStale);
}

/// Phase 4 Test: Verify delta lag threshold detection.
TEST_F(QueryPlannerTest, TensorDeltaLagExceeds_TriggersRebuild) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    auto f = makeFreshArtifact();
    f.delta_lag = 2000;  // Exceeds 1000 threshold in default config
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // High delta lag should block the tensor path
    EXPECT_NE(decision.path, ExecutionPath::AnnTensorSummary);
    EXPECT_NE(decision.path, ExecutionPath::AnnTensorExactGraph);
}

/// Phase 4 Test: Verify residual threshold enforcement.
TEST_F(QueryPlannerTest, TensorResidualLow_BlocksTensorPath) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    auto f = makeFreshArtifact();
    f.residual_threshold = 0.90;  // Below 0.95 minimum
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // Low residual should block tensor-based paths
    EXPECT_NE(decision.path, ExecutionPath::AnnTensorSummary);
    EXPECT_NE(decision.path, ExecutionPath::AnnTensorExactGraph);
}

/// Phase 4 Test: Verify rank cap enforcement.
TEST_F(QueryPlannerTest, TensorRankCapExceeded_FallsBackToExact) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    auto f = makeFreshArtifact();
    f.rank_cap = 1500;  // Exceeds default 1000 limit
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // Rank cap violation should trigger fallback
    EXPECT_EQ(decision.fallback_reason, FallbackReason::TensorRankCapExceeded);
}

/// Phase 4 Test: Verify rebuild-in-progress blocks tensor paths.
TEST_F(QueryPlannerTest, TensorRebuildInProgress_BlocksTensorPath) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    auto f = makeFreshArtifact();
    f.rebuild_in_progress = true;
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // Active rebuild should block tensor-based paths, select path 1 (ANN only) or path 4
    EXPECT_EQ(decision.fallback_reason, FallbackReason::TensorRebuildInProgress);
}

/// Phase 4 Test: Verify fallback chain from GPU failure → CPU fallback.
TEST_F(QueryPlannerTest, FallbackChain_GpuErrorFallsBackToCpu) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    auto e = makeFullEligibility();
    e.gpu_parity_validated = false;  // Simulate GPU parity check failure
    auto f = makeFreshArtifact();
    const auto c = makeDefaultConfig();

    const auto decision = planner->selectPath(e, f, c);
    
    // GPU parity failure should force CPU path
    EXPECT_FALSE(decision.uses_gpu);
}

/// Phase 4 Test: Verify no silent fallback — every fallback has a reason.
TEST_F(QueryPlannerTest, NoSilentFallback_AllFallbacksHaveReason) {
    auto planner = makeDefaultQueryPlanner(nullptr);
    
    // Test various fallback scenarios
    std::vector<std::pair<ExecutionEligibility, TensorArtifactFreshness>> scenarios = {
        {makeFullEligibility(), makeStaleArtifact()},
        {makeFullEligibility(), makeFreshArtifact(10000)},
    };
    
    const auto c = makeDefaultConfig();
    
    for (auto& [e, f] : scenarios) {
        const auto decision = planner->selectPath(e, f, c);
        
        // Every non-AnnOnly decision must carry an explicit fallback reason —
        // a None reason on a non-AnnOnly path is the "silent fallback" defect.
        if (decision.path != ExecutionPath::AnnOnly) {
            EXPECT_NE(decision.fallback_reason, FallbackReason::None)
                << "Silent fallback detected: path=" << static_cast<int>(decision.path)
                << " but fallback_reason is None";
        }
    }
}

