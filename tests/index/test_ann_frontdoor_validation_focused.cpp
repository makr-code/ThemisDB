/**
 * @file test_ann_frontdoor_validation_focused.cpp
 * @brief Phase B gate: ANN output cardinality + range validation tests (ANN-VAL-01..08).
 *
 * Validates the defensive validation layer added to `AnnFrontdoor::search()`:
 *  1. Cardinality: candidates are truncated to `top_k` when a backend returns more.
 *  2. Range: candidates with negative or NaN distances are removed before results
 *     are passed to the tensor layer.
 *
 * These tests exercise the AnnFrontdoor via a `StubAnnIndex` that can return
 * results with configurable distances.  The backends intentionally violate the
 * distance contract so the validation logic is triggered.
 *
 * Tests:
 *  ANN-VAL-01: Exact top_k results from backend — no truncation
 *  ANN-VAL-02: Backend returns more than top_k — truncated to top_k
 *  ANN-VAL-03: All distances non-negative — no candidates removed
 *  ANN-VAL-04: One candidate with negative distance — that entry removed
 *  ANN-VAL-05: One candidate with NaN distance — that entry removed
 *  ANN-VAL-06: Combined: over-cardinality + NaN entry — truncate then filter
 *  ANN-VAL-07: Empty candidate list from backend — returns empty, no crash
 *  ANN-VAL-08: Sort order preserved after range filtering
 *
 * @see src/index/ann_frontdoor.cpp — search() validation block (Phase B gate)
 * @see src/index/ROADMAP.md — Phase B gate items
 * @version 1.0.0
 * @note Maturity: PRODUCTION-READY (validation layer tests)
 */

#include "index/ann_frontdoor.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

using namespace themis::index;

// ============================================================================
// Stub backend that returns a configurable fixed result set
// ============================================================================

/**
 * @brief IAnnIndex stub that returns a fixed, unfiltered result list.
 *
 * Intentionally ignores the `k` parameter to always return its full result set,
 * allowing tests to trigger the cardinality guard in `AnnFrontdoor::search()`.
 */
class ValidationStubIndex : public IAnnIndex {
public:
    struct Entry { int64_t id; float distance; };

    /// Construct with a result list.  The stub returns ALL entries regardless
    /// of the `k` argument (to exercise the cardinality guard).
    explicit ValidationStubIndex(std::vector<Entry> data)
        : data_(std::move(data)) {}

    bool build(const float*, const int64_t*, size_t, size_t) override {
        return true;
    }
    [[nodiscard]] bool add(int64_t, const float*, size_t) override {
        return true;
    }

    /// Return ALL entries — ignores k on purpose to test cardinality guard.
    std::vector<AnnSearchResult> search(const float*, size_t, int /*k*/) const override {
        std::vector<AnnSearchResult> out;
        out.reserve(data_.size());
        for (const auto& e : data_) {
            out.push_back({e.id, e.distance});
        }
        return out;
    }

    [[nodiscard]] std::size_t size() const override { return data_.size(); }

private:
    std::vector<Entry> data_;
};

// ============================================================================
// Fixture
// ============================================================================

class AnnValidationTest : public ::testing::Test {
protected:
    /// Build an AnnFrontdoor in HNSW mode with a single backend stub.
    AnnFrontdoor makeFrontdoor(std::vector<ValidationStubIndex::Entry> data,
                               int default_k = 5) {
        AnnFrontdoor::Config cfg;
        cfg.hnsw_max_dataset_size = 1'000'000;
        cfg.default_k             = default_k;

        AnnFrontdoor fd(cfg);

        auto stub = std::make_shared<ValidationStubIndex>(std::move(data));
        fd.registerBackend(AnnScopeKind::Document, stub, /*global=*/true);

        return fd;
    }

    /// Minimal query vector (1-dim, value=0.5).
    const float kQuery = 0.5f;

    /// Build a simple AnnQueryContext that forces HNSW selection.
    static AnnQueryContext makeContext() {
        AnnQueryContext ctx;
        ctx.scope_kind    = AnnScopeKind::Document;
        ctx.dataset_size  = 100;  // well within hnsw_max → HNSW path
        ctx.hot_tier      = true;
        return ctx;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-01: Exact top_k results from backend — no truncation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-01: When the backend returns exactly k=3 candidates, the result size
 * is 3 — no truncation needed.
 */
TEST_F(AnnValidationTest, AnnVal01_ExactTopKNoTruncation) {
    auto fd = makeFrontdoor({{1, 0.1f}, {2, 0.2f}, {3, 0.3f}}, /*default_k=*/5);
    auto result = fd.search(&kQuery, 1, /*k=*/3, makeContext());
    EXPECT_EQ(result.candidates.size(), 3u)
        << "Exactly 3 valid candidates with k=3 should not be truncated";
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-02: Backend returns more than top_k — truncated to top_k
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-02: The stub returns 6 entries for k=3.  The cardinality guard must
 * truncate the result to 3 candidates.
 */
TEST_F(AnnValidationTest, AnnVal02_OverCardinality_TruncatedToK) {
    auto fd = makeFrontdoor(
        {{1, 0.1f}, {2, 0.2f}, {3, 0.3f}, {4, 0.4f}, {5, 0.5f}, {6, 0.6f}},
        /*default_k=*/5);

    auto result = fd.search(&kQuery, 1, /*k=*/3, makeContext());

    EXPECT_EQ(result.candidates.size(), 3u)
        << "6 candidates returned for k=3 must be truncated to 3";
    // Verify the remaining candidates are the first 3 (best distances)
    EXPECT_FLOAT_EQ(result.candidates[0].distance, 0.1f);
    EXPECT_FLOAT_EQ(result.candidates[1].distance, 0.2f);
    EXPECT_FLOAT_EQ(result.candidates[2].distance, 0.3f);
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-03: All distances non-negative — no candidates removed
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-03: When all distances are valid (≥ 0.0), the range filter must not
 * remove any candidates.
 */
TEST_F(AnnValidationTest, AnnVal03_AllValidDistances_NoneRemoved) {
    auto fd = makeFrontdoor({{1, 0.0f}, {2, 0.5f}, {3, 1.0f}}, /*default_k=*/5);
    auto result = fd.search(&kQuery, 1, /*k=*/3, makeContext());
    EXPECT_EQ(result.candidates.size(), 3u)
        << "All distances ≥ 0 — no candidates should be removed";
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-04: One candidate with negative distance — that entry removed
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-04: A single candidate with distance = -1.0 (invalid) must be
 * removed by the range filter.  The remaining 2 valid candidates are preserved.
 */
TEST_F(AnnValidationTest, AnnVal04_NegativeDistance_EntryRemoved) {
    auto fd = makeFrontdoor({{1, 0.1f}, {2, -1.0f}, {3, 0.3f}}, /*default_k=*/5);
    auto result = fd.search(&kQuery, 1, /*k=*/5, makeContext());

    EXPECT_EQ(result.candidates.size(), 2u)
        << "Candidate with distance=-1.0 must be removed";
    for (const auto& c : result.candidates) {
        EXPECT_GE(c.distance, 0.0f) << "All remaining distances must be ≥ 0";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-05: One candidate with NaN distance — that entry removed
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-05: A candidate with NaN distance must be detected and removed.
 * The check `!(r.distance >= 0.0f)` correctly handles NaN because NaN
 * comparisons always return false.
 */
TEST_F(AnnValidationTest, AnnVal05_NanDistance_EntryRemoved) {
    const float kNaN = std::numeric_limits<float>::quiet_NaN();
    auto fd = makeFrontdoor({{1, 0.1f}, {2, kNaN}, {3, 0.3f}}, /*default_k=*/5);
    auto result = fd.search(&kQuery, 1, /*k=*/5, makeContext());

    EXPECT_EQ(result.candidates.size(), 2u)
        << "Candidate with NaN distance must be removed";
    for (const auto& c : result.candidates) {
        EXPECT_FALSE(std::isnan(c.distance)) << "No NaN distances in output";
        EXPECT_GE(c.distance, 0.0f) << "All remaining distances must be ≥ 0";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-06: Combined over-cardinality + NaN entry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-06: Backend returns 5 entries for k=3; one of the first 3 after
 * truncation has a NaN distance.  After truncation to 3 and range filtering,
 * exactly 2 valid candidates must remain.
 */
TEST_F(AnnValidationTest, AnnVal06_OverCardinalityPlusNanEntry) {
    const float kNaN = std::numeric_limits<float>::quiet_NaN();
    // First 3 after truncation: {0.1, NaN, 0.3} → 2 valid; last 2 discarded
    auto fd = makeFrontdoor(
        {{1, 0.1f}, {2, kNaN}, {3, 0.3f}, {4, 0.4f}, {5, 0.5f}},
        /*default_k=*/5);

    auto result = fd.search(&kQuery, 1, /*k=*/3, makeContext());

    EXPECT_EQ(result.candidates.size(), 2u)
        << "After truncate-to-3 then NaN-filter, exactly 2 candidates remain";
    for (const auto& c : result.candidates) {
        EXPECT_GE(c.distance, 0.0f);
        EXPECT_FALSE(std::isnan(c.distance));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-07: Empty candidate list from backend — no crash
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-07: When the backend returns an empty list, both validation passes
 * are no-ops and the result contains 0 candidates without crashing.
 */
TEST_F(AnnValidationTest, AnnVal07_EmptyResultNoCrash) {
    auto fd = makeFrontdoor({}, /*default_k=*/5);
    AnnFrontdoorResult result;
    ASSERT_NO_THROW(result = fd.search(&kQuery, 1, /*k=*/5, makeContext()));
    EXPECT_TRUE(result.candidates.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ANN-VAL-08: Sort order preserved after range filtering
// ─────────────────────────────────────────────────────────────────────────────

/**
 * ANN-VAL-08: After range filtering, the remaining candidates must still be
 * in ascending distance order (closest first), as the candidates list is not
 * re-sorted by the validation step.
 */
TEST_F(AnnValidationTest, AnnVal08_SortOrderPreservedAfterFiltering) {
    // Two valid candidates interspersed with one negative-distance entry
    auto fd = makeFrontdoor({{1, 0.1f}, {2, -0.5f}, {3, 0.3f}}, /*default_k=*/5);
    auto result = fd.search(&kQuery, 1, /*k=*/5, makeContext());

    ASSERT_EQ(result.candidates.size(), 2u);
    EXPECT_LT(result.candidates[0].distance, result.candidates[1].distance)
        << "Candidates must remain in ascending distance order after filtering";
}
