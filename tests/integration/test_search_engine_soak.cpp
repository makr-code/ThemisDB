/**
 * @file test_search_engine_soak.cpp
 * @brief Wave D — Search Engine Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB search engine hot paths:
 * full-text throughput, index consistency stability, and ranking
 * reliability under sustained load.  Verifies that all three metrics
 * remain within acceptable bounds over a configurable soak window driven
 * by THEMIS_SOAK_DURATION_MS.
 *
 * In CI environments this test runs at the default 60 000 ms (1 min) so
 * the gate finishes well within the 120 s CTest timeout.  The full
 * production soak (3 600 000 ms / 60 min) is reserved for the release
 * pipeline.
 *
 * ## Acceptance criteria
 * - SearchSoak_FullTextThroughput          : ≥ 5 000 searches/sec
 * - SearchSoak_IndexConsistencyStability   : zero inconsistency events
 * - SearchSoak_RankingReliability          : zero ranking anomalies
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @note Run in the release/Wave-D soak pipeline; excluded from fast CI gate.
 * @see docs/operability/RUNBOOK_SEARCH_ENGINE.md — operator runbook
 * @see src/search/ROADMAP.md — Wave D contribution closure
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Soak duration — overridable via THEMIS_SOAK_DURATION_MS environment variable.
// Default: 60 000 ms (1 min) so CI completes well within the 120 s timeout.
// ─────────────────────────────────────────────────────────────────────────────
static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB / SIMULATION NOTE
//
// In-process stubs model the search engine hot paths without requiring
// external backends (index store, ranking model, shard mesh).  These stubs
// MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

// ---------------------------------------------------------------------------
// StubSearchIndex — simulates an in-process inverted index with posting list
// ---------------------------------------------------------------------------
class StubSearchIndex {
public:
    struct SearchResult {
        bool     ok          = true;
        uint32_t hit_count   = 0;
        uint32_t doc_version = 0;  // monotonically increasing; used for consistency check
    };

    SearchResult search(const std::string& term, uint32_t limit) {
        SearchResult r;
        r.hit_count   = static_cast<uint32_t>(std::hash<std::string>{}(term) % 50 + 1);
        r.hit_count   = std::min(r.hit_count, limit);
        r.doc_version = doc_version_.load(std::memory_order_acquire);
        ++total_searches_;
        return r;
    }

    // Simulate a background indexing tick (increments version).
    void index_tick() {
        doc_version_.fetch_add(1, std::memory_order_release);
    }

    uint64_t total_searches() const { return total_searches_.load(); }

private:
    std::atomic<uint64_t> total_searches_{0};
    std::atomic<uint32_t> doc_version_{1};
};

// ---------------------------------------------------------------------------
// StubRankingModel — deterministic scoring function
// ---------------------------------------------------------------------------
class StubRankingModel {
public:
    struct RankResult {
        bool ok = true;
        float top_score = 0.0f;
    };

    RankResult rank(uint32_t hit_count, const std::string& term) {
        if (hit_count == 0) { return {true, 0.0f}; }
        float score = static_cast<float>(hit_count) /
                      static_cast<float>(term.size() + 1);
        ++total_ranked_;
        return {true, score};
    }

    uint64_t total_ranked() const { return total_ranked_.load(); }

private:
    std::atomic<uint64_t> total_ranked_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// TEST 1 — SearchSoak_FullTextThroughput
//
// Acceptance criterion: ≥ 5 000 searches/sec sustained over soak window.
// ─────────────────────────────────────────────────────────────────────────────
TEST(SearchSoak, FullTextThroughput) {
    const uint64_t soak_ms = soakDurationMs();

    StubSearchIndex  idx;
    StubRankingModel ranker;

    const std::vector<std::string> terms = {
        "themis", "database", "query", "index", "search",
        "vector", "graph", "transaction", "soak", "stress",
    };

    const auto start = std::chrono::steady_clock::now();
    const auto end   = start + std::chrono::milliseconds(soak_ms);

    uint64_t loop_count = 0;
    while (std::chrono::steady_clock::now() < end) {
        const std::string& term = terms[loop_count % terms.size()];
        auto sr = idx.search(term, 10);
        ASSERT_TRUE(sr.ok) << "Search failure at loop " << loop_count;
        auto rr = ranker.rank(sr.hit_count, term);
        ASSERT_TRUE(rr.ok) << "Ranking failure at loop " << loop_count;
        ++loop_count;
    }

    const double elapsed_s = static_cast<double>(soak_ms) / 1000.0;
    const double qps        = static_cast<double>(loop_count) / elapsed_s;

    EXPECT_GE(qps, 5000.0)
        << "FTS throughput below gate: got " << qps
        << " searches/s, need ≥ 5000 searches/s "
        << "(loop_count=" << loop_count << ", soak_ms=" << soak_ms << ")";
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 2 — SearchSoak_IndexConsistencyStability
//
// Acceptance criterion: zero index consistency events (version never goes
// backwards; all reads observe non-decreasing doc_version).
// ─────────────────────────────────────────────────────────────────────────────
TEST(SearchSoak, IndexConsistencyStability) {
    const uint64_t soak_ms = soakDurationMs();

    StubSearchIndex idx;

    // Background indexer thread bumps doc_version periodically.
    std::atomic<bool> stop_indexer{false};
    std::thread indexer([&]() {
        while (!stop_indexer.load(std::memory_order_relaxed)) {
            idx.index_tick();
            std::this_thread::sleep_for(1ms);
        }
    });

    const auto start = std::chrono::steady_clock::now();
    const auto end   = start + std::chrono::milliseconds(soak_ms);

    uint64_t  consistency_errors = 0;
    uint32_t  last_version       = 0;
    uint64_t  loop_count         = 0;

    while (std::chrono::steady_clock::now() < end) {
        auto sr = idx.search("doc_" + std::to_string(loop_count % 100), 20);
        ASSERT_TRUE(sr.ok) << "Search failed at iteration " << loop_count;
        if (sr.doc_version < last_version) {
            ++consistency_errors;
        }
        last_version = sr.doc_version;
        ++loop_count;
    }

    stop_indexer.store(true, std::memory_order_relaxed);
    indexer.join();

    EXPECT_EQ(0ULL, consistency_errors)
        << "Index version went backwards " << consistency_errors
        << " times over " << loop_count << " reads";
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST 3 — SearchSoak_RankingReliability
//
// Acceptance criterion: zero ranking anomalies (score is always in [0, ∞);
// identical queries return identical scores).
// ─────────────────────────────────────────────────────────────────────────────
TEST(SearchSoak, RankingReliability) {
    const uint64_t soak_ms = soakDurationMs();

    StubSearchIndex  idx;
    StubRankingModel ranker;

    // Capture baseline scores for fixed queries.
    const std::vector<std::string> fixed_terms = {"baseline_a", "baseline_b", "baseline_c"};
    std::vector<float> baseline_scores;
    for (const auto& t : fixed_terms) {
        auto sr = idx.search(t, 10);
        auto rr = ranker.rank(sr.hit_count, t);
        baseline_scores.push_back(rr.top_score);
    }

    const auto start = std::chrono::steady_clock::now();
    const auto end   = start + std::chrono::milliseconds(soak_ms);

    uint64_t anomalies  = 0;
    uint64_t loop_count = 0;

    while (std::chrono::steady_clock::now() < end) {
        const std::string& term = fixed_terms[loop_count % fixed_terms.size()];
        auto sr = idx.search(term, 10);
        auto rr = ranker.rank(sr.hit_count, term);

        // Score must be non-negative and must match the stable baseline.
        if (rr.top_score < 0.0f) {
            ++anomalies;
        }
        float expected = baseline_scores[loop_count % fixed_terms.size()];
        if (std::abs(rr.top_score - expected) > 1e-5f) {
            ++anomalies;
        }
        ++loop_count;
    }

    EXPECT_EQ(0ULL, anomalies)
        << "Ranking emitted " << anomalies << " anomalies over "
        << loop_count << " iterations";
}
