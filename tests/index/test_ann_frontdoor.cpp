// Unit tests for AnnFrontdoor — explicit ANN frontdoor abstraction
//
// Covers:
//   - planStrategy() routing rules for all 6 strategies
//   - search() delegating to scope-specific and global IAnnIndex backends
//   - search() falling back to FLAT_BRUTE_FORCE when no backend is registered
//   - Distributed fan-out: candidates merged and sorted from multiple shards
//   - Hot/cold tier routing: cold tier demotes HNSW → SCANN / DISKANN
//   - Invalid argument guards (nullptr query, dim == 0)
//   - explainStrategy() and annStrategyName() utility

#include "index/ann_frontdoor.h"
#include "observability/reason_codes.h"
#include "utils/logger.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::index;

// ============================================================================
// Helpers
// ============================================================================

/// Minimal IAnnIndex stub that returns a fixed ordered result list.
class StubAnnIndex : public IAnnIndex {
public:
    struct Entry { int64_t id; float distance; };

    explicit StubAnnIndex(std::vector<Entry> data) : data_(std::move(data)) {}

    bool build(const float*, const int64_t*, size_t, size_t) override {
        return true;
    }

    [[nodiscard]] bool add(int64_t, const float*, size_t) override {
        return true;
    }

    std::vector<AnnSearchResult> search(const float*, size_t, int k) const override {
        std::vector<AnnSearchResult> out;
        int take = std::min<int>(k, static_cast<int>(data_.size()));
        out.reserve(static_cast<std::size_t>(take));
        for (int i = 0; i < take; ++i) {
            out.push_back({data_[static_cast<std::size_t>(i)].id,
                           data_[static_cast<std::size_t>(i)].distance});
        }
        return out;
    }

    [[nodiscard]] std::size_t size() const override { return data_.size(); }

private:
    std::vector<Entry> data_;
};

class FlakyAnnIndex : public IAnnIndex {
public:
    FlakyAnnIndex(int fail_count_before_success,
                  std::vector<StubAnnIndex::Entry> data)
        : remaining_failures_(fail_count_before_success),
          data_(std::move(data)) {}

    bool build(const float*, const int64_t*, size_t, size_t) override {
        return true;
    }

    [[nodiscard]] bool add(int64_t, const float*, size_t) override {
        return true;
    }

    std::vector<AnnSearchResult> search(const float*, size_t, int k) const override {
        ++calls_;
        if (remaining_failures_ > 0) {
            --remaining_failures_;
            throw std::runtime_error("simulated shard backend failure");
        }

        std::vector<AnnSearchResult> out;
        const int take = std::min<int>(k, static_cast<int>(data_.size()));
        out.reserve(static_cast<std::size_t>(take));
        for (int i = 0; i < take; ++i) {
            out.push_back({data_[static_cast<std::size_t>(i)].id,
                           data_[static_cast<std::size_t>(i)].distance});
        }
        return out;
    }

    [[nodiscard]] std::size_t size() const override { return data_.size(); }
    [[nodiscard]] int calls() const { return calls_; }

private:
    mutable int remaining_failures_ = 0;
    mutable int calls_ = 0;
    std::vector<StubAnnIndex::Entry> data_;
};

static std::shared_ptr<StubAnnIndex> makeStub(int n_results,
                                               float base_distance = 0.1f) {
    std::vector<StubAnnIndex::Entry> entries;
    entries.reserve(static_cast<std::size_t>(n_results));
    for (int i = 0; i < n_results; ++i) {
        entries.push_back({static_cast<int64_t>(i),
                           base_distance * static_cast<float>(i + 1)});
    }
    return std::make_shared<StubAnnIndex>(std::move(entries));
}

static const float kQuery[4] = {1.0f, 0.0f, 0.0f, 0.0f};
static constexpr std::size_t kDim = 4;

// ============================================================================
// planStrategy tests
// ============================================================================

TEST(AnnFrontdoorPlanStrategy, DefaultContextNoBackendReturnsFlatBruteForce) {
    AnnFrontdoor fd;
    EXPECT_EQ(fd.planStrategy({}), AnnStrategy::FLAT_BRUTE_FORCE);
}

TEST(AnnFrontdoorPlanStrategy, HotSmallScopedDatasetReturnsHnsw) {
    AnnFrontdoor fd;
    fd.registerBackend("adapter-hot", makeStub(5), AnnScopeKind::Adapter);

    AnnQueryContext ctx;
    ctx.dataset_size = 100'000;
    ctx.hot_tier     = true;
    ctx.scope_id     = "adapter-hot";
    EXPECT_EQ(fd.planStrategy(ctx), AnnStrategy::HNSW);
}

TEST(AnnFrontdoorPlanStrategy, MediumDatasetUsesScann) {
    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(5));

    AnnQueryContext ctx;
    ctx.dataset_size = 5'000'000;   // between hnsw_max and scann_max
    ctx.hot_tier     = true;
    EXPECT_EQ(fd.planStrategy(ctx), AnnStrategy::SCANN);
}

TEST(AnnFrontdoorPlanStrategy, LargeDatasetDiskAnnWhenAvailable) {
    AnnFrontdoor::Config cfg;
    cfg.diskann_available = true;
    AnnFrontdoor fd(cfg);
    fd.registerBackend("", makeStub(5));

    AnnQueryContext ctx;
    ctx.dataset_size = 200'000'000;  // > scann_max_elements (50M)
    ctx.hot_tier     = false;
    EXPECT_EQ(fd.planStrategy(ctx), AnnStrategy::DISKANN);
}

TEST(AnnFrontdoorPlanStrategy, ShardAwareMultipleBackendsReturnsDistributed) {
    AnnFrontdoor fd;
    fd.registerBackend("shard-0", makeStub(3));
    fd.registerBackend("shard-1", makeStub(3));

    AnnQueryContext ctx;
    ctx.shard_aware = true;
    EXPECT_EQ(fd.planStrategy(ctx), AnnStrategy::DISTRIBUTED);
}

TEST(AnnFrontdoorPlanStrategy, ScopeSpecificBackendPreferredOverGlobal) {
    AnnFrontdoor fd;
    fd.registerBackend("",          makeStub(5));   // global ScaNN
    fd.registerBackend("adapter-x", makeStub(3), AnnScopeKind::Adapter);

    AnnQueryContext ctx;
    ctx.scope_id     = "adapter-x";
    ctx.dataset_size = 5'000'000;
    ctx.hot_tier     = true;
    // Scope-specific backend should be selected and searched, even if the
    // strategy downgrades to ScaNN for the larger dataset.
    auto result = fd.search(kQuery, kDim, 3, ctx);
    ASSERT_EQ(result.candidates.size(), 3u);
    EXPECT_EQ(result.candidates[0].id, 0);
    EXPECT_EQ(fd.planStrategy(ctx), AnnStrategy::SCANN);
}

TEST(AnnFrontdoorPlanStrategy, ColdTierScopeSpecificUsesDiskAnnWhenAvailable) {
    AnnFrontdoor::Config cfg;
    cfg.diskann_available = true;
    AnnFrontdoor fd(cfg);
    fd.registerBackend("shard-cold", makeStub(5), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.scope_id = "shard-cold";
    ctx.hot_tier = false;
    EXPECT_EQ(fd.planStrategy(ctx), AnnStrategy::DISKANN);
}

TEST(AnnFrontdoorPlanStrategy, ScopeKindAdapterUsesDiskAnnPlanWhenCold) {
    AnnFrontdoor::Config cfg;
    cfg.diskann_available = true;
    AnnFrontdoor fd(cfg);
    fd.registerBackend("pkg:catalog", makeStub(4), AnnScopeKind::Package);

    AnnQueryContext ctx;
    ctx.scope_id = "pkg:catalog";
    ctx.hot_tier = false;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_EQ(plan.strategy, AnnStrategy::DISKANN);
    EXPECT_EQ(plan.scope_kind, AnnScopeKind::Package);
    EXPECT_EQ(plan.effective_tier, IndexTierMeta::Tier::COLD);
    EXPECT_FALSE(plan.reason.empty());
}

TEST(AnnFrontdoorPlanStrategy, ShardSummaryScopeRoutesDistributedWhenShardAware) {
    AnnFrontdoor fd;
    fd.registerBackend("summary-a", makeStub(2), AnnScopeKind::ShardSummary);
    fd.registerBackend("summary-b", makeStub(2), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.scope_id = "summary-a";
    ctx.shard_aware = true;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_EQ(plan.strategy, AnnStrategy::DISTRIBUTED);
    EXPECT_TRUE(plan.distributed);
    EXPECT_EQ(plan.scope_kind, AnnScopeKind::ShardSummary);
}

// ============================================================================
// search() tests — result correctness
// ============================================================================

TEST(AnnFrontdoorSearch, GlobalBackendReturnsSortedCandidates) {
    AnnFrontdoor fd;
    // Stub returns entries with distances 0.3, 0.1, 0.2 (unsorted)
    std::vector<StubAnnIndex::Entry> entries = {{10, 0.3f}, {11, 0.1f}, {12, 0.2f}};
    fd.registerBackend("", std::make_shared<StubAnnIndex>(std::move(entries)));

    auto result = fd.search(kQuery, kDim, 3);

    ASSERT_EQ(result.candidates.size(), 3u);
    // Must be sorted ascending by distance
    EXPECT_FLOAT_EQ(result.candidates[0].distance, 0.1f);
    EXPECT_FLOAT_EQ(result.candidates[1].distance, 0.2f);
    EXPECT_FLOAT_EQ(result.candidates[2].distance, 0.3f);
    EXPECT_EQ(result.candidates[0].id, 11);
}

TEST(AnnFrontdoorSearch, KLimitIsRespected) {
    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(10));

    auto result = fd.search(kQuery, kDim, 3);
    EXPECT_LE(result.candidates.size(), 3u);
}

TEST(AnnFrontdoorSearch, DefaultKUsedWhenKIsZero) {
    AnnFrontdoor::Config cfg;
    cfg.default_k = 5;
    AnnFrontdoor fd(cfg);
    fd.registerBackend("", makeStub(20));

    auto result = fd.search(kQuery, kDim, 0);
    EXPECT_LE(result.candidates.size(), 5u);
}

TEST(AnnFrontdoorSearch, ScopeSpecificBackendTakesPriority) {
    AnnFrontdoor fd;
    // Global returns ids 0..4; scope returns ids 100..102
    fd.registerBackend("",      makeStub(5));
    std::vector<StubAnnIndex::Entry> scoped = {{100, 0.05f}, {101, 0.10f}, {102, 0.15f}};
    fd.registerBackend("my-adapter", std::make_shared<StubAnnIndex>(scoped));

    AnnQueryContext ctx;
    ctx.scope_id = "my-adapter";
    auto result = fd.search(kQuery, kDim, 3, ctx);

    ASSERT_EQ(result.candidates.size(), 3u);
    EXPECT_EQ(result.candidates[0].id, 100);
}

TEST(AnnFrontdoorSearch, DistributedMergesAndSortsAllShards) {
    AnnFrontdoor fd;
    // Shard-0 returns ids 0,1 with distances 0.2, 0.4
    fd.registerBackend("shard-0",
        std::make_shared<StubAnnIndex>(std::vector<StubAnnIndex::Entry>{
            {0, 0.2f}, {1, 0.4f}}));
    // Shard-1 returns ids 10,11 with distances 0.1, 0.3
    fd.registerBackend("shard-1",
        std::make_shared<StubAnnIndex>(std::vector<StubAnnIndex::Entry>{
            {10, 0.1f}, {11, 0.3f}}));

    AnnQueryContext ctx;
    ctx.shard_aware = true;
    auto result = fd.search(kQuery, kDim, 4, ctx);

    ASSERT_EQ(result.candidates.size(), 4u);
    EXPECT_EQ(result.strategy_used, AnnStrategy::DISTRIBUTED);
    EXPECT_TRUE(result.is_distributed);

    // Sorted: 0.1, 0.2, 0.3, 0.4
    EXPECT_FLOAT_EQ(result.candidates[0].distance, 0.1f);
    EXPECT_EQ(result.candidates[0].id, 10);
    EXPECT_FLOAT_EQ(result.candidates[3].distance, 0.4f);
}

TEST(AnnFrontdoorSearch, DistributedTopKTruncated) {
    AnnFrontdoor fd;
    fd.registerBackend("s0", makeStub(10, 0.1f));
    fd.registerBackend("s1", makeStub(10, 0.05f));

    AnnQueryContext ctx;
    ctx.shard_aware = true;
    auto result = fd.search(kQuery, kDim, 5, ctx);
    EXPECT_LE(result.candidates.size(), 5u);
}

TEST(AnnFrontdoorSearch, DistributedFanoutLimitRespected) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_max_fanout = 1;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    fd.registerBackend("a", makeStub(2, 0.1f));
    fd.registerBackend("b", makeStub(2, 0.05f));
    fd.registerBackend("c", makeStub(2, 0.2f));

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 5, ctx);
    EXPECT_TRUE(result.is_distributed);
    EXPECT_EQ(result.shards_attempted, 1u);
    EXPECT_EQ(result.shards_succeeded, 1u);
    EXPECT_EQ(result.shards_failed, 0u);
}

TEST(AnnFrontdoorSearch, DistributedRetryRecoversFlakyShard) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_retry_attempts = 1;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    auto flaky = std::make_shared<FlakyAnnIndex>(
        1, std::vector<StubAnnIndex::Entry>{{7, 0.12f}, {8, 0.20f}});
    fd.registerBackend("flaky", flaky, AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_EQ(result.shards_attempted, 1u);
    EXPECT_EQ(result.shards_succeeded, 1u);
    EXPECT_EQ(result.shards_failed, 0u);
    EXPECT_FALSE(result.candidates.empty());
    EXPECT_GE(flaky->calls(), 2);
}

TEST(AnnFrontdoorSearch, DistributedPartialFailureReturnsDegradedWhenAllowed) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_retry_attempts = 0;
    cfg.distributed_allow_partial_results = true;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    auto always_fail = std::make_shared<FlakyAnnIndex>(
        100, std::vector<StubAnnIndex::Entry>{{99, 0.5f}});
    fd.registerBackend("fail", always_fail, AnnScopeKind::ShardSummary);
    fd.registerBackend("ok", makeStub(2, 0.05f), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_TRUE(result.partial_results);
    EXPECT_EQ(result.fallback_mode, "degraded_continue");
    EXPECT_EQ(result.fallback_reason_code,
              std::string(themis::observability::reason_codes::ann::kFallbackDistributedPartialFailure));
    EXPECT_EQ(result.shards_failed, 1u);
    EXPECT_EQ(result.shards_succeeded, 1u);
    EXPECT_FALSE(result.candidates.empty());
}

TEST(AnnFrontdoorSearch, DistributedPartialFailureFailsClosedWhenPartialDisabled) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_retry_attempts = 0;
    cfg.distributed_allow_partial_results = false;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    auto always_fail = std::make_shared<FlakyAnnIndex>(
        100, std::vector<StubAnnIndex::Entry>{{99, 0.5f}});
    fd.registerBackend("fail", always_fail, AnnScopeKind::ShardSummary);
    fd.registerBackend("ok", makeStub(2, 0.05f), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_TRUE(result.candidates.empty());
    EXPECT_FALSE(result.partial_results);
    EXPECT_EQ(result.fallback_mode, "fail_closed");
    EXPECT_EQ(result.fallback_reason_code,
              std::string(themis::observability::reason_codes::ann::kFallbackDistributedPartialFailure));
    EXPECT_EQ(result.shards_failed, 1u);
    EXPECT_EQ(result.shards_succeeded, 1u);
}

TEST(AnnFrontdoorSearch, DistributedAllShardFailuresFailClosedWhenPartialDisabled) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_retry_attempts = 0;
    cfg.distributed_allow_partial_results = false;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    fd.registerBackend("f0", std::make_shared<FlakyAnnIndex>(
        100, std::vector<StubAnnIndex::Entry>{{1, 0.2f}}), AnnScopeKind::ShardSummary);
    fd.registerBackend("f1", std::make_shared<FlakyAnnIndex>(
        100, std::vector<StubAnnIndex::Entry>{{2, 0.3f}}), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_TRUE(result.candidates.empty());
    EXPECT_EQ(result.shards_succeeded, 0u);
    EXPECT_EQ(result.shards_failed, 2u);
    EXPECT_EQ(result.fallback_mode, "fail_closed");
    EXPECT_EQ(result.fallback_reason_code,
              std::string(themis::observability::reason_codes::ann::kFallbackDistributedAllShardsFailed));
}

TEST(AnnFrontdoorSearch, DistributedMergeDeterministicDistanceThenId) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    fd.registerBackend("s0", std::make_shared<StubAnnIndex>(
        std::vector<StubAnnIndex::Entry>{{42, 0.2f}, {10, 0.1f}, {20, 0.1f}}));
    fd.registerBackend("s1", std::make_shared<StubAnnIndex>(
        std::vector<StubAnnIndex::Entry>{{10, 0.3f}, {30, 0.1f}}));

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 10, ctx);
    ASSERT_GE(result.candidates.size(), 4u);
    EXPECT_EQ(result.candidates[0].id, 10);
    EXPECT_EQ(result.candidates[1].id, 20);
    EXPECT_EQ(result.candidates[2].id, 30);
    EXPECT_EQ(result.candidates[3].id, 42);
    EXPECT_EQ(result.merged_candidates_before_trim, result.candidates.size());
}

TEST(AnnFrontdoorSearch, DistributedCostAwarePruningRespectsQualityFloor) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_cost_budget = 2.0;
    cfg.distributed_quality_floor = 0.9;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    fd.registerBackend("high-quality", makeStub(3, 0.1f), AnnScopeKind::ShardSummary);
    fd.registerBackend("low-quality", makeStub(3, 0.15f), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_TRUE(plan.distributed);
    EXPECT_FALSE(plan.pruned_shard_ids.empty());
}

TEST(AnnFrontdoorSearch, DistributedCostBudgetPruning) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_cost_budget = 1.0;
    cfg.distributed_max_fanout = 0;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    fd.registerBackend("s0", makeStub(2, 0.1f), AnnScopeKind::ShardSummary);
    fd.registerBackend("s1", makeStub(2, 0.15f), AnnScopeKind::ShardSummary);
    fd.registerBackend("s2", makeStub(2, 0.2f), AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_TRUE(plan.distributed);
    EXPECT_LE(plan.pruned_shard_ids.size(), 2u);
}

TEST(AnnFrontdoorSearch, DistributedSearchRespectsPrunedShardList) {
    AnnFrontdoor::Config cfg;
    cfg.distributed_cost_budget = 1.0;
    cfg.distributed_include_global_backend = false;
    AnnFrontdoor fd(cfg);

    auto s0 = makeStub(2, 0.1f);
    auto s1 = makeStub(2, 0.2f);
    fd.registerBackend("s0", s0, AnnScopeKind::ShardSummary);
    fd.registerBackend("s1", s1, AnnScopeKind::ShardSummary);

    AnnQueryContext ctx;
    ctx.shard_aware = true;

    auto result = fd.search(kQuery, kDim, 5, ctx);
    EXPECT_TRUE(result.is_distributed);
    EXPECT_FALSE(result.candidates.empty());
}

TEST(AnnFrontdoorSearch, NoBackendReturnsEmptyCandidateList) {
    AnnFrontdoor fd;  // no backend, no VIM
    auto result = fd.search(kQuery, kDim, 5);
    EXPECT_EQ(result.strategy_used, AnnStrategy::FLAT_BRUTE_FORCE);
    EXPECT_TRUE(result.candidates.empty());
}

// ============================================================================
// Routing metadata
// ============================================================================

TEST(AnnFrontdoorRouting, RoutingReasonIsNonEmpty) {
    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(3));

    AnnQueryContext ctx;
    ctx.dataset_size = 1'000;
    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_FALSE(result.routing_reason.empty());
}

TEST(AnnFrontdoorRouting, ExplainStrategyMatchesSearchStrategy) {
    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(3));

    AnnQueryContext ctx;
    ctx.dataset_size = 60'000'000;   // > scann_max → DISKANN if available, else fallback
    const auto strategy  = fd.planStrategy(ctx);
    const auto explained = fd.explainStrategy(ctx);
    EXPECT_FALSE(explained.empty());
    EXPECT_NE(explained.find(annStrategyName(strategy)), std::string::npos);
}

TEST(AnnFrontdoorRouting, EstimatedRecallInRange) {
    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(5));

    auto result = fd.search(kQuery, kDim, 5);
    EXPECT_GE(result.estimated_recall, 0.0);
    EXPECT_LE(result.estimated_recall, 1.0);
}

TEST(AnnFrontdoorRouting, CorrelationAndPolicyMetadataPropagated) {
    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(3));

    AnnQueryContext ctx;
    ctx.correlation_id = "corr-ann-01";
    ctx.confidence_policy_version = "policy-v1";
    ctx.confidence_threshold_key = "ann.strict";

    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_EQ(result.correlation_id, "corr-ann-01");
    EXPECT_EQ(result.confidence_policy_version, "policy-v1");
    EXPECT_EQ(result.confidence_threshold_key, "ann.strict");
    EXPECT_FALSE(result.routing_reason_code.empty());
}

TEST(AnnFrontdoorRouting, MissingBackendSetsDegradedFallbackReason) {
    AnnFrontdoor fd;
    AnnQueryContext ctx;
    ctx.scope_id = "missing-scope";

    auto result = fd.search(kQuery, kDim, 3, ctx);
    EXPECT_EQ(result.strategy_used, AnnStrategy::FLAT_BRUTE_FORCE);
    EXPECT_EQ(result.fallback_mode, "degraded_continue");
    EXPECT_EQ(result.fallback_reason_code, std::string(themis::observability::reason_codes::ann::kFallbackBackendUnavailable));
}

// ============================================================================
// Diagnostics
// ============================================================================

TEST(AnnFrontdoorDiagnostics, RegisteredBackendCount) {
    AnnFrontdoor fd;
    EXPECT_EQ(fd.registeredBackendCount(), 0u);
    fd.registerBackend("", makeStub(1));
    EXPECT_EQ(fd.registeredBackendCount(), 1u);
    fd.registerBackend("shard-1", makeStub(1));
    EXPECT_EQ(fd.registeredBackendCount(), 2u);
}

TEST(AnnFrontdoorDiagnostics, ConfigAccessors) {
    AnnFrontdoor::Config cfg;
    cfg.hnsw_max_elements = 500'000;
    AnnFrontdoor fd(cfg);
    EXPECT_EQ(fd.config().hnsw_max_elements, 500'000u);
}

TEST(AnnFrontdoorDiagnostics, AnnStrategyNameCoverage) {
    EXPECT_STREQ(annStrategyName(AnnStrategy::HNSW),             "HNSW");
    EXPECT_STREQ(annStrategyName(AnnStrategy::SCANN),            "SCANN");
    EXPECT_STREQ(annStrategyName(AnnStrategy::DISKANN),          "DISKANN");
    EXPECT_STREQ(annStrategyName(AnnStrategy::DISTRIBUTED),      "DISTRIBUTED");
    EXPECT_STREQ(annStrategyName(AnnStrategy::FLAT_BRUTE_FORCE), "FLAT_BRUTE_FORCE");
}

TEST(AnnFrontdoorDiagnostics, AnnScopeKindNameCoverage) {
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::Generic),      "Generic");
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::Adapter),      "Adapter");
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::Package),      "Package");
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::ShardSummary), "ShardSummary");
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::Document),     "Document");
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::Chunk),        "Chunk");
    EXPECT_STREQ(annScopeKindName(AnnScopeKind::Entity),       "Entity");
}

// ============================================================================
// Document / Chunk / Entity scope kinds — ANN artifact class routing
// ============================================================================

TEST(AnnFrontdoorArtifactClasses, DocumentScopeHotReturnsHnsw) {
    AnnFrontdoor fd;
    fd.registerBackend("doc:corpus-main", makeStub(5), AnnScopeKind::Document);

    AnnQueryContext ctx;
    ctx.scope_id     = "doc:corpus-main";
    ctx.dataset_size = 50'000;
    ctx.hot_tier     = true;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_EQ(plan.strategy, AnnStrategy::HNSW);
    EXPECT_EQ(plan.scope_kind, AnnScopeKind::Document);
    EXPECT_TRUE(plan.hot_path);
}

TEST(AnnFrontdoorArtifactClasses, ChunkScopeMediumUsesScann) {
    AnnFrontdoor fd;
    fd.registerBackend("chunk:passages", makeStub(5), AnnScopeKind::Chunk);

    AnnQueryContext ctx;
    ctx.scope_id     = "chunk:passages";
    ctx.dataset_size = 10'000'000;   // between hnsw_max and scann_max
    ctx.hot_tier     = true;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_EQ(plan.strategy, AnnStrategy::SCANN);
    EXPECT_EQ(plan.scope_kind, AnnScopeKind::Chunk);
}

TEST(AnnFrontdoorArtifactClasses, EntityScopeColdUsesDiskAnn) {
    AnnFrontdoor::Config cfg;
    cfg.diskann_available = true;
    AnnFrontdoor fd(cfg);
    fd.registerBackend("entity:kg-nodes", makeStub(5), AnnScopeKind::Entity);

    AnnQueryContext ctx;
    ctx.scope_id = "entity:kg-nodes";
    ctx.hot_tier = false;

    const auto plan = fd.planRetrieval(ctx);
    EXPECT_EQ(plan.strategy, AnnStrategy::DISKANN);
    EXPECT_EQ(plan.scope_kind, AnnScopeKind::Entity);
    EXPECT_EQ(plan.effective_tier, IndexTierMeta::Tier::COLD);
}

TEST(AnnFrontdoorArtifactClasses, DocumentSearchReturnsCandidates) {
    AnnFrontdoor fd;
    std::vector<StubAnnIndex::Entry> entries = {{101, 0.1f}, {102, 0.2f}};
    fd.registerBackend("doc:wiki", std::make_shared<StubAnnIndex>(std::move(entries)),
                       AnnScopeKind::Document);

    AnnQueryContext ctx;
    ctx.scope_id     = "doc:wiki";
    ctx.dataset_size = 100'000;
    ctx.hot_tier     = true;

    const auto result = fd.search(kQuery, kDim, 2, ctx);
    ASSERT_EQ(result.candidates.size(), 2u);
    EXPECT_EQ(result.candidates[0].id, 101);
    EXPECT_EQ(result.candidates[1].id, 102);
}

TEST(AnnFrontdoorArtifactClasses, ChunkSearchReturnsCandidates) {
    AnnFrontdoor fd;
    std::vector<StubAnnIndex::Entry> entries = {{201, 0.05f}, {202, 0.15f}};
    fd.registerBackend("chunk:paras", std::make_shared<StubAnnIndex>(std::move(entries)),
                       AnnScopeKind::Chunk);

    AnnQueryContext ctx;
    ctx.scope_id     = "chunk:paras";
    ctx.dataset_size = 200'000;
    ctx.hot_tier     = true;

    const auto result = fd.search(kQuery, kDim, 2, ctx);
    ASSERT_EQ(result.candidates.size(), 2u);
    EXPECT_EQ(result.candidates[0].id, 201);
}

TEST(AnnFrontdoorArtifactClasses, EntitySearchReturnsCandidates) {
    AnnFrontdoor fd;
    std::vector<StubAnnIndex::Entry> entries = {{301, 0.08f}, {302, 0.12f}};
    fd.registerBackend("entity:persons", std::make_shared<StubAnnIndex>(std::move(entries)),
                       AnnScopeKind::Entity);

    AnnQueryContext ctx;
    ctx.scope_id     = "entity:persons";
    ctx.dataset_size = 300'000;
    ctx.hot_tier     = true;

    const auto result = fd.search(kQuery, kDim, 2, ctx);
    ASSERT_EQ(result.candidates.size(), 2u);
    EXPECT_EQ(result.candidates[0].id, 301);
}

TEST(AnnFrontdoorArtifactClasses, RoutingReasonIncludesScopeKindName) {
    AnnFrontdoor fd;
    fd.registerBackend("chunk:sections", makeStub(3), AnnScopeKind::Chunk);

    AnnQueryContext ctx;
    ctx.scope_id     = "chunk:sections";
    ctx.dataset_size = 80'000;
    ctx.hot_tier     = true;

    const std::string reason = fd.explainStrategy(ctx);
    // The routing reason must include the human-readable scope kind name.
    EXPECT_NE(reason.find("Chunk"), std::string::npos);
}

// ============================================================================
// Guard tests
// ============================================================================

TEST(AnnFrontdoorGuards, NullptrQueryThrows) {
    AnnFrontdoor fd;
    EXPECT_THROW(fd.search(nullptr, kDim, 5), std::invalid_argument);
}

TEST(AnnFrontdoorGuards, ZeroDimThrows) {
    AnnFrontdoor fd;
    EXPECT_THROW(fd.search(kQuery, 0, 5), std::invalid_argument);
}

TEST(AnnFrontdoorGuards, RegisterNullBackendThrows) {
    AnnFrontdoor fd;
    EXPECT_THROW(fd.registerBackend("", nullptr), std::invalid_argument);
}

TEST(AnnFrontdoorGuards, RegisterNullVimThrows) {
    AnnFrontdoor fd;
    EXPECT_THROW(fd.registerVectorIndexManager(nullptr), std::invalid_argument);
}

TEST(AnnFrontdoorGuards, RegisterNullTieredManagerIsAllowed) {
    AnnFrontdoor fd;
    // nullptr is explicitly allowed for TieredIndexManager
    EXPECT_NO_THROW(fd.registerTieredIndexManager(nullptr));
}

// ============================================================================
// Structured JSON log emission
// ============================================================================

TEST(AnnFrontdoorLogging, StructuredLayerHandoffJsonLogEmitted) {
    const auto now_ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto log_path = std::filesystem::temp_directory_path() /
                          ("themis_ann_log_" + std::to_string(now_ticks) + ".log");

    themis::utils::Logger::shutdown();
    themis::utils::Logger::init(log_path.string(), themis::utils::Logger::Level::INFO);
    themis::utils::Logger::setPattern("%v");

    AnnFrontdoor fd;
    fd.registerBackend("", makeStub(5));

    AnnQueryContext ctx;
    ctx.correlation_id = "corr-ann-log";
    fd.search(kQuery, kDim, 3, ctx);

    const auto logger = themis::utils::Logger::get();
    ASSERT_NE(logger, nullptr);
    logger->flush();
    themis::utils::Logger::shutdown();

    std::ifstream in(log_path, std::ios::binary);
    ASSERT_TRUE(in.is_open());
    const std::string content((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());

    EXPECT_NE(content.find("\"event\":\"layer_handoff_decision\""), std::string::npos);
    EXPECT_NE(content.find("\"layer_name\":\"ann\""), std::string::npos);
    EXPECT_NE(content.find("\"correlation_id\":\"corr-ann-log\""), std::string::npos);
    EXPECT_NE(content.find("\"routing_reason_code\":"), std::string::npos);
    EXPECT_NE(content.find("\"resolved\":"), std::string::npos);

    std::error_code ec;
    std::filesystem::remove(log_path, ec);
}
