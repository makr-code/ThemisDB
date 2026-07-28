/**
 * @file test_ann_frontdoor_single_shard.cpp
 * @brief EPIC-1 / Phase A gate: Single-shard exact retrieval end-to-end.
 *
 * Verifies that the ANN Frontdoor correctly routes a single-shard exact
 * retrieval request end-to-end:
 *  1. A scoped backend is registered for a single logical shard.
 *  2. search() dispatches to that backend (HNSW / FLAT_BRUTE_FORCE strategy).
 *  3. Result cardinality equals the requested k.
 *  4. The non-distributed flag is set (shard_aware = false).
 *  5. Candidate IDs and distances are propagated intact.
 *
 * These are the acceptance criteria from src/retrieval/ROADMAP.md Phase A:
 *  "Single-Shard-Exact-Retrieval End-to-End verdrahten"
 *
 * Test IDs: EPIC1-RS-01 .. EPIC1-RS-05
 */

#include "index/ann_frontdoor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using namespace themis::index;

// ---------------------------------------------------------------------------
// Deterministic stub backend — simulates a real single-shard HNSW index.
// Returns a fixed, sorted candidate list regardless of the query vector.
// ---------------------------------------------------------------------------

namespace {

constexpr std::array<std::pair<int64_t, float>, 5> kStubCandidates{{
    {10, 0.05F},
    {20, 0.12F},
    {30, 0.27F},
    {40, 0.41F},
    {50, 0.58F},
}};

class DeterministicSingleShardIndex final : public IAnnIndex {
public:
    bool build(const float*, const int64_t*, size_t, size_t) override { return true; }
    [[nodiscard]] bool add(int64_t, const float*, size_t) override { return true; }

    std::vector<AnnSearchResult> search(const float*, size_t, int k) const override {
        std::vector<AnnSearchResult> results;
        const int take = std::min(k, static_cast<int>(kStubCandidates.size()));
        results.reserve(static_cast<std::size_t>(take));
        for (int i = 0; i < take; ++i) {
            results.push_back({kStubCandidates[static_cast<std::size_t>(i)].first,
                               kStubCandidates[static_cast<std::size_t>(i)].second});
        }
        return results;
    }

    [[nodiscard]] std::size_t size() const override { return kStubCandidates.size(); }
};

// Shared query vector (4-dimensional unit vector along first axis).
static constexpr std::array<float, 4> kQueryVec = {1.0F, 0.0F, 0.0F, 0.0F};

} // namespace

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class Epic1SingleShardRetrievalTest : public ::testing::Test {
protected:
    void SetUp() override {
        frontdoor_.registerBackend(kScope,
                                   std::make_shared<DeterministicSingleShardIndex>(),
                                   AnnScopeKind::Document);

        ctx_.scope_id      = kScope;
        ctx_.dataset_size  = static_cast<std::size_t>(kStubCandidates.size());
        ctx_.hot_tier      = true;
        ctx_.shard_aware   = false;
        ctx_.recall_target = 1.0;
    }

    static constexpr const char* kScope = "retrieval:single-shard-exact";

    AnnFrontdoor   frontdoor_;
    AnnQueryContext ctx_{};
};

// ---------------------------------------------------------------------------
// EPIC1-RS-01 — result cardinality equals requested k
// ---------------------------------------------------------------------------

TEST_F(Epic1SingleShardRetrievalTest, ResultCardinalityEqualsK) {
    constexpr int k = 3;
    const auto result = frontdoor_.search(kQueryVec.data(), kQueryVec.size(), k, ctx_);
    EXPECT_EQ(result.candidates.size(), static_cast<std::size_t>(k))
        << "Expected exactly k=" << k << " candidates from single-shard search";
}

// ---------------------------------------------------------------------------
// EPIC1-RS-02 — strategy is non-distributed (single-shard path)
// ---------------------------------------------------------------------------

TEST_F(Epic1SingleShardRetrievalTest, StrategyIsNonDistributed) {
    const auto result = frontdoor_.search(kQueryVec.data(), kQueryVec.size(), 2, ctx_);
    EXPECT_FALSE(result.is_distributed)
        << "Single-shard retrieval must not set is_distributed=true; "
           "routing_reason: " << result.routing_reason;
}

// ---------------------------------------------------------------------------
// EPIC1-RS-03 — strategy is HNSW or FLAT_BRUTE_FORCE (no cross-shard path)
// ---------------------------------------------------------------------------

TEST_F(Epic1SingleShardRetrievalTest, StrategyIsLocalIndex) {
    const auto result = frontdoor_.search(kQueryVec.data(), kQueryVec.size(), 2, ctx_);
    EXPECT_NE(result.strategy_used, AnnStrategy::DISTRIBUTED)
        << "Single-shard retrieval must not use the DISTRIBUTED strategy; "
           "routing_reason: " << result.routing_reason;
}

// ---------------------------------------------------------------------------
// EPIC1-RS-04 — candidate IDs propagated intact (no silent drops/duplicates)
// ---------------------------------------------------------------------------

TEST_F(Epic1SingleShardRetrievalTest, CandidateIdsAreIntact) {
    constexpr int k = 5;
    const auto result = frontdoor_.search(kQueryVec.data(), kQueryVec.size(), k, ctx_);
    ASSERT_EQ(result.candidates.size(), static_cast<std::size_t>(k));

    for (std::size_t i = 0; i < static_cast<std::size_t>(k); ++i) {
        EXPECT_EQ(result.candidates[i].id, kStubCandidates[i].first)
            << "Candidate[" << i << "] id mismatch";
    }
}

// ---------------------------------------------------------------------------
// EPIC1-RS-05 — distances are non-negative and sorted ascending
// ---------------------------------------------------------------------------

TEST_F(Epic1SingleShardRetrievalTest, DistancesNonNegativeAndSorted) {
    constexpr int k = 4;
    const auto result = frontdoor_.search(kQueryVec.data(), kQueryVec.size(), k, ctx_);
    ASSERT_EQ(result.candidates.size(), static_cast<std::size_t>(k));

    for (const auto& c : result.candidates) {
        EXPECT_GE(c.distance, 0.0F) << "Distance must be non-negative; id=" << c.id;
    }

    for (std::size_t i = 1; i < result.candidates.size(); ++i) {
        EXPECT_LE(result.candidates[i - 1].distance, result.candidates[i].distance)
            << "Candidates must be sorted by ascending distance at positions "
            << (i - 1) << " and " << i;
    }
}
