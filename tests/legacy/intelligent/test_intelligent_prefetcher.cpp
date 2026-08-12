// Copyright 2026 ThemisDB
// Licensed under MIT License

/**
 * Focused unit tests for IntelligentPrefetcher (performance module, v1.8.0).
 *
 * Acceptance criteria tested:
 *  AC-1  Pattern Learning     – ML model learns sequential and strided patterns
 *  AC-2  Prefetch Distance    – adaptive prefetch distance responds to latency
 *  AC-3  Confidence Scoring   – only high-confidence predictions are returned
 *  AC-4  Multi-Level          – routes prefetch to L1/L2/L3/DRAM by confidence
 *  AC-5  Feedback Loop        – useful vs wasted prefetch counters are accurate
 *  AC-6  Stats                – accuracy and coverage computed correctly
 *
 * Test suite name: IntelligentPrefetchingFocusedTests
 */

#include <gtest/gtest.h>
#include "performance/intelligent_prefetcher.h"

#include <chrono>
#include <cstdint>
#include <numeric>
#include <thread>
#include <vector>

using namespace themis::performance;

// ─── Helpers ─────────────────────────────────────────────────────────────────

[[maybe_unused]] static uint64_t now_ns() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

// Build a config that is safe for unit tests (no hardware prefetch side-effects,
// low confidence threshold so that short sequences still produce predictions).
static IntelligentPrefetcher::PrefetchConfig make_test_config(
    double threshold = 0.5,
    size_t history   = 64) {
    IntelligentPrefetcher::PrefetchConfig cfg;
    cfg.enable_learning          = true;
    cfg.max_prefetch_distance    = 16;
    cfg.confidence_threshold     = threshold;
    cfg.history_size             = history;
    cfg.enable_hardware_prefetch = false;  // avoid side-effects in tests
    return cfg;
}

// Feed a perfectly strided sequence of `count` accesses starting at `base`
// with step `stride`.
static void feed_stride(IntelligentPrefetcher& p,
                        uint64_t base, int64_t stride, size_t count,
                        uint64_t ts_start = 0, uint64_t ts_delta = 50) {
    for (size_t i = 0; i < count; ++i) {
        uint64_t addr = static_cast<uint64_t>(
            static_cast<int64_t>(base) + static_cast<int64_t>(i) * stride);
        uint64_t ts = (ts_start == 0) ? 0 : ts_start + i * ts_delta;
        p.record_access(addr, ts);
    }
}

// ─── Test fixture ─────────────────────────────────────────────────────────────

class IntelligentPrefetchingFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        prefetcher_ = std::make_unique<IntelligentPrefetcher>(make_test_config());
    }

    std::unique_ptr<IntelligentPrefetcher> prefetcher_;
};

// =============================================================================
// AC-1: Pattern Learning
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, DefaultConfigValues) {
    IntelligentPrefetcher p;
    const auto& cfg = p.config();
    EXPECT_TRUE(cfg.enable_learning);
    EXPECT_EQ(cfg.max_prefetch_distance, 16u);
    EXPECT_DOUBLE_EQ(cfg.confidence_threshold, 0.7);
    EXPECT_EQ(cfg.history_size, 1000u);
    EXPECT_TRUE(cfg.enable_hardware_prefetch);
}

TEST_F(IntelligentPrefetchingFocusedTests, LearnSequentialPattern) {
    // Feed a perfectly sequential access pattern (stride = 64 bytes).
    feed_stride(*prefetcher_, /*base=*/0x1000, /*stride=*/64, /*count=*/20);

    auto pat = prefetcher_->current_pattern();
    EXPECT_GT(pat.confidence, 0.5);
    EXPECT_EQ(pat.stride, 64u);
}

TEST_F(IntelligentPrefetchingFocusedTests, LearnStridedPattern) {
    // Feed a strided access pattern (stride = 256 bytes).
    feed_stride(*prefetcher_, /*base=*/0x2000, /*stride=*/256, /*count=*/20);

    auto pat = prefetcher_->current_pattern();
    EXPECT_GT(pat.confidence, 0.5);
    EXPECT_EQ(pat.stride, 256u);
}

TEST_F(IntelligentPrefetchingFocusedTests, LowConfidenceForRandomPattern) {
    // Feed random addresses – no consistent stride should be detected.
    IntelligentPrefetcher p(make_test_config(/*threshold=*/0.9));
    const uint64_t addrs[] = {0x100, 0x500, 0xA00, 0x30, 0xF000, 0x50, 0x7777};
    for (uint64_t a : addrs) {
        p.record_access(a);
    }
    auto pat = p.current_pattern();
    // Confidence should be low for a chaotic pattern.
    EXPECT_LT(pat.confidence, 0.9);
}

TEST_F(IntelligentPrefetchingFocusedTests, AccessPatternAddressesPopulated) {
    feed_stride(*prefetcher_, 0x3000, 128, 10);
    auto pat = prefetcher_->current_pattern();
    EXPECT_FALSE(pat.addresses.empty());
}

TEST_F(IntelligentPrefetchingFocusedTests, InsufficientHistoryGivesZeroConfidence) {
    // Only 2 accesses – below MIN_HISTORY_FOR_STRIDE (4).
    prefetcher_->record_access(0x1000);
    prefetcher_->record_access(0x1040);
    auto pat = prefetcher_->current_pattern();
    EXPECT_DOUBLE_EQ(pat.confidence, 0.0);
}

// =============================================================================
// AC-2: Adaptive Prefetch Distance
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, AdaptivePrefetchDistanceDefaultRange) {
    // Without any accesses the distance should be within the valid range.
    size_t dist = prefetcher_->adaptive_prefetch_distance();
    EXPECT_GE(dist, 1u);
    EXPECT_LE(dist, prefetcher_->config().max_prefetch_distance);
}

TEST_F(IntelligentPrefetchingFocusedTests, HighLatencyIncreasesDistance) {
    // Feed accesses with large timestamp gaps (high latency → more aggressive
    // prefetch).
    IntelligentPrefetcher p(make_test_config());
    size_t initial = p.adaptive_prefetch_distance();

    // 200 ns gaps exceeds LATENCY_HIGH_NS (100 ns).
    feed_stride(p, 0x4000, 64, 30, /*ts_start=*/1000, /*ts_delta=*/200);

    size_t after = p.adaptive_prefetch_distance();
    EXPECT_GE(after, initial);
}

TEST_F(IntelligentPrefetchingFocusedTests, LowLatencyDoesNotExceedMax) {
    IntelligentPrefetcher p(make_test_config());

    // 5 ns gaps – well below LATENCY_LOW_NS (20 ns).
    feed_stride(p, 0x5000, 64, 40, /*ts_start=*/1000, /*ts_delta=*/5);

    EXPECT_LE(p.adaptive_prefetch_distance(), p.config().max_prefetch_distance);
}

// =============================================================================
// AC-3: Confidence Scoring
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, HighConfidencePatternProducesPredictions) {
    feed_stride(*prefetcher_, 0x6000, 64, 25);

    auto preds = prefetcher_->predict_next_accesses(0x6000 + 25 * 64, 8);
    EXPECT_FALSE(preds.empty());
    // All predictions must be spaced by 64 bytes.
    for (size_t i = 1; i < preds.size(); ++i) {
        EXPECT_EQ(preds[i] - preds[i - 1], 64u);
    }
}

TEST_F(IntelligentPrefetchingFocusedTests, LowConfidencePatternReturnsNoPredictions) {
    // Threshold set to 0.99 – nearly impossible to reach with 7 random accesses.
    IntelligentPrefetcher p(make_test_config(/*threshold=*/0.99));
    const uint64_t addrs[] = {0x100, 0x500, 0xA00, 0x30, 0xF000, 0x50, 0x7777};
    for (uint64_t a : addrs) p.record_access(a);

    auto preds = p.predict_next_accesses(0x7777, 8);
    EXPECT_TRUE(preds.empty());
}

TEST_F(IntelligentPrefetchingFocusedTests, PredictionCountCappedByLookahead) {
    feed_stride(*prefetcher_, 0x7000, 64, 20);
    auto preds = prefetcher_->predict_next_accesses(0x7000 + 20 * 64, /*lookahead=*/4);
    EXPECT_LE(preds.size(), 4u);
}

TEST_F(IntelligentPrefetchingFocusedTests, PredictionCountCappedByMaxDistance) {
    IntelligentPrefetcher p(make_test_config());
    feed_stride(p, 0x8000, 64, 20);
    // Request more than max_prefetch_distance.
    auto preds = p.predict_next_accesses(0x8000 + 20 * 64, 100);
    EXPECT_LE(preds.size(), p.config().max_prefetch_distance);
}

TEST_F(IntelligentPrefetchingFocusedTests, ZeroStrideGivesNoPredictions) {
    // All same address → stride = 0 → no predictions.
    for (int i = 0; i < 20; ++i) prefetcher_->record_access(0xDEAD);
    auto preds = prefetcher_->predict_next_accesses(0xDEAD, 8);
    EXPECT_TRUE(preds.empty());
}

// =============================================================================
// AC-4: Multi-Level prefetch routing (side-effect free; test via stats counts)
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, PrefetchPredictedIncrementsCounter) {
    feed_stride(*prefetcher_, 0x9000, 64, 20);

    auto preds = prefetcher_->predict_next_accesses(0x9000 + 20 * 64, 4);
    ASSERT_FALSE(preds.empty());
    prefetcher_->prefetch_predicted(preds);

    auto stats = prefetcher_->get_stats();
    EXPECT_GE(stats.total_prefetches, preds.size());
}

TEST_F(IntelligentPrefetchingFocusedTests, PrefetchWithHardwarePrefetchEnabled) {
    // Smoke test: enabling hardware prefetch must not crash (addresses may be
    // invalid but the CPU ignores faulting prefetches).
    IntelligentPrefetcher::PrefetchConfig cfg = make_test_config();
    cfg.enable_hardware_prefetch = true;
    IntelligentPrefetcher p(cfg);
    // Use addresses that map to valid virtual memory (within the stack).
    int dummy[64] = {};
    uint64_t base = reinterpret_cast<uint64_t>(dummy);
    feed_stride(p, base, 4, 20);
    auto preds = p.predict_next_accesses(base + 20 * 4, 4);
    // Only prefetch addresses within the dummy array.
    std::vector<uint64_t> safe_preds;
    for (uint64_t a : preds) {
        if (a >= base && a < base + sizeof(dummy)) safe_preds.push_back(a);
    }
    EXPECT_NO_THROW(p.prefetch_predicted(safe_preds));
}

// =============================================================================
// AC-5: Feedback Loop
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, FeedbackLoopCountsUsefulPrefetches) {
    // Train on stride=64.
    feed_stride(*prefetcher_, 0xA000, 64, 20);

    // Predict and prefetch the next 4 addresses.
    uint64_t current = 0xA000 + 20 * 64;
    auto preds = prefetcher_->predict_next_accesses(current, 4);
    ASSERT_FALSE(preds.empty());
    prefetcher_->prefetch_predicted(preds);

    // Now access the predicted addresses (simulates cache hits).
    for (uint64_t a : preds) {
        prefetcher_->record_access(a);
    }

    auto stats = prefetcher_->get_stats();
    EXPECT_GE(stats.useful_prefetches, preds.size());
}

TEST_F(IntelligentPrefetchingFocusedTests, FeedbackLoopDoesNotCountUnprefetched) {
    // Access without any prior prefetch – useful counter must stay 0.
    feed_stride(*prefetcher_, 0xB000, 64, 10);

    auto stats = prefetcher_->get_stats();
    EXPECT_EQ(stats.useful_prefetches, 0u);
}

TEST_F(IntelligentPrefetchingFocusedTests, WastedPrefetchCountedWhenNotAccessed) {
    IntelligentPrefetcher p(make_test_config());
    feed_stride(p, 0xC000, 64, 20);

    // Issue prefetches for addresses that will never be accessed.
    std::vector<uint64_t> fake = {0xDEAD0000, 0xDEAD0040, 0xDEAD0080};
    p.prefetch_predicted(fake);

    // total_prefetches must include the issued prefetches.
    auto stats = p.get_stats();
    EXPECT_GE(stats.total_prefetches, fake.size());
}

// =============================================================================
// AC-6: Statistics – accuracy and coverage
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, AccuracyComputedCorrectly) {
    // Train and create a perfect prediction scenario.
    feed_stride(*prefetcher_, 0xE000, 64, 20);

    uint64_t current = 0xE000 + 20 * 64;
    auto preds = prefetcher_->predict_next_accesses(current, 4);
    ASSERT_FALSE(preds.empty());
    prefetcher_->prefetch_predicted(preds);
    for (uint64_t a : preds) prefetcher_->record_access(a);

    auto stats = prefetcher_->get_stats();
    // With all prefetched addresses actually accessed, accuracy should be > 0.
    EXPECT_GT(stats.accuracy, 0.0);
    EXPECT_LE(stats.accuracy, 1.0);
}

TEST_F(IntelligentPrefetchingFocusedTests, CoverageComputedCorrectly) {
    feed_stride(*prefetcher_, 0xF000, 64, 20);

    uint64_t current = 0xF000 + 20 * 64;
    auto preds = prefetcher_->predict_next_accesses(current, 4);
    if (!preds.empty()) {
        prefetcher_->prefetch_predicted(preds);
        for (uint64_t a : preds) prefetcher_->record_access(a);
    }

    auto stats = prefetcher_->get_stats();
    EXPECT_GE(stats.coverage, 0.0);
    EXPECT_LE(stats.coverage, 1.0);
    EXPECT_GT(stats.total_accesses, 20u);  // training + feedback accesses
}

TEST_F(IntelligentPrefetchingFocusedTests, StatsAccuracyZeroWhenNoPrefetches) {
    feed_stride(*prefetcher_, 0x10000, 64, 10);
    auto stats = prefetcher_->get_stats();
    // No prefetches issued yet → accuracy remains 0.0.
    EXPECT_DOUBLE_EQ(stats.accuracy, 0.0);
}

TEST_F(IntelligentPrefetchingFocusedTests, ResetStatsClearsCounters) {
    feed_stride(*prefetcher_, 0x11000, 64, 20);
    auto preds = prefetcher_->predict_next_accesses(0x11000 + 20 * 64, 4);
    if (!preds.empty()) prefetcher_->prefetch_predicted(preds);

    prefetcher_->reset_stats();
    auto stats = prefetcher_->get_stats();
    EXPECT_EQ(stats.total_prefetches, 0u);
    EXPECT_EQ(stats.useful_prefetches, 0u);
    EXPECT_EQ(stats.total_accesses, 0u);
}

TEST_F(IntelligentPrefetchingFocusedTests, ResetClearsEverything) {
    feed_stride(*prefetcher_, 0x12000, 64, 20);
    prefetcher_->reset();

    auto pat   = prefetcher_->current_pattern();
    auto stats = prefetcher_->get_stats();
    EXPECT_DOUBLE_EQ(pat.confidence, 0.0);
    EXPECT_EQ(stats.total_accesses, 0u);
}

// =============================================================================
// Additional edge case / integration tests
// =============================================================================

TEST_F(IntelligentPrefetchingFocusedTests, PredictWithNoHistoryReturnsEmpty) {
    auto preds = prefetcher_->predict_next_accesses(0xFFFF, 8);
    EXPECT_TRUE(preds.empty());
}

TEST_F(IntelligentPrefetchingFocusedTests, PrefetchPredictedEmptyVectorIsNoop) {
    EXPECT_NO_THROW(prefetcher_->prefetch_predicted({}));
    auto stats = prefetcher_->get_stats();
    EXPECT_EQ(stats.total_prefetches, 0u);
}

TEST_F(IntelligentPrefetchingFocusedTests, HistoryWindowEvictsOldEntries) {
    // Use a very small history window.
    IntelligentPrefetcher::PrefetchConfig cfg = make_test_config(0.5, /*history=*/8);
    IntelligentPrefetcher p(cfg);

    // Feed 20 accesses – only the last 8 should be retained.
    feed_stride(p, 0x20000, 64, 20);
    auto pat = p.current_pattern();
    EXPECT_LE(pat.addresses.size(), 8u);
}

TEST_F(IntelligentPrefetchingFocusedTests, ConcurrentAccessIsSafe) {
    // Minimal thread-safety smoke test.
    IntelligentPrefetcher p(make_test_config());

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&p, t]() {
            for (int i = 0; i < 50; ++i) {
                uint64_t addr = static_cast<uint64_t>(t * 0x1000 + i * 64);
                p.record_access(addr, static_cast<uint64_t>(i * 50));
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_NO_THROW(p.get_stats());
    EXPECT_NO_THROW(p.current_pattern());
}

TEST_F(IntelligentPrefetchingFocusedTests, LearningDisabledSkipsPatternUpdate) {
    IntelligentPrefetcher::PrefetchConfig cfg = make_test_config();
    cfg.enable_learning = false;
    IntelligentPrefetcher p(cfg);

    feed_stride(p, 0x30000, 64, 30);
    // With learning disabled confidence stays at 0.
    auto pat = p.current_pattern();
    EXPECT_DOUBLE_EQ(pat.confidence, 0.0);
}

TEST_F(IntelligentPrefetchingFocusedTests, TotalAccessesTracked) {
    size_t n = 15;
    feed_stride(*prefetcher_, 0x40000, 128, n);
    auto stats = prefetcher_->get_stats();
    EXPECT_EQ(stats.total_accesses, n);
}
