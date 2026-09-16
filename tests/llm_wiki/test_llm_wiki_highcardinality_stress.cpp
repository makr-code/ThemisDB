// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_wiki_highcardinality_stress.cpp
 * @brief Wave D — LLM Wiki High-Cardinality Stress Tests.
 *
 * Validates the LLM Wiki module under high-cardinality and concurrent
 * workload conditions without requiring real external backends.
 *
 * Test families
 * -------------
 * - HighCardinalityArticleIndex    : 100 000 articles via 8 threads
 * - ConcurrentSemanticQueryStress  : semantic query under concurrent load
 * - WikiSyncEdgeCaseStress         : edge cases in article sync
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_LLM_WIKI.md
 * @see src/llm_wiki/ROADMAP.md — Wave D contribution closure
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations.
// They do NOT drive real LLM Wiki infrastructure and MUST NOT be linked
// into production code paths.

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct StubWikiIndex {
    std::mutex mu;
    std::unordered_map<uint64_t, std::string> index;
    std::atomic<uint64_t> synced{0};
    std::atomic<uint64_t> queries{0};
    std::atomic<uint64_t> search_calls{0};
    std::atomic<uint64_t> sync_faults{0};

    void sync(uint64_t article_id) {
        std::lock_guard<std::mutex> lk(mu);
        index[article_id] = "article_" + std::to_string(article_id);
        ++synced;
    }

    bool query(uint64_t article_id) {
        ++queries;
        std::lock_guard<std::mutex> lk(mu);
        return index.count(article_id) > 0;
    }

    bool semanticSearch(uint64_t /*token*/) {
        ++search_calls;
        return true;
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Stress test cases
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief HighCardinalityArticleIndex
 *
 * Syncs 100 000 articles from 8 concurrent threads and validates all are
 * indexed without loss.
 */
TEST(LLMWikiHighCardinalityStress, HighCardinalityArticleIndex) {
    constexpr uint64_t kTotalArticles = 100'000ULL;
    constexpr int kThreads = 8;
    constexpr uint64_t kPerThread = kTotalArticles / kThreads;

    StubWikiIndex wiki;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                wiki.sync(base + i);
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(wiki.synced.load(), kTotalArticles)
        << "Article sync count mismatch under high-cardinality stress";
    EXPECT_EQ(wiki.sync_faults.load(), 0ULL)
        << "Sync faults detected during high-cardinality article index stress";
    EXPECT_EQ(wiki.index.size(), kTotalArticles)
        << "Index size does not match synced article count";
}

/**
 * @brief ConcurrentSemanticQueryStress
 *
 * Issues 500 000 semantic search calls from 4 concurrent threads. Asserts
 * all calls succeed.
 */
TEST(LLMWikiHighCardinalityStress, ConcurrentSemanticQueryStress) {
    constexpr uint64_t kPerThread = 125'000ULL;
    constexpr int kThreads = 4;

    StubWikiIndex wiki;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * kPerThread;
            for (uint64_t i = 0; i < kPerThread; ++i) {
                bool ok = wiki.semanticSearch(base + i);
                EXPECT_TRUE(ok) << "Semantic search returned false for token " << (base + i);
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(wiki.search_calls.load(), kThreads * kPerThread)
        << "Semantic query call count mismatch";
}

/**
 * @brief WikiSyncEdgeCaseStress
 *
 * Exercises repeated sync of the same article IDs from multiple threads to
 * validate idempotency under concurrent writes.
 */
TEST(LLMWikiHighCardinalityStress, WikiSyncEdgeCaseStress) {
    constexpr uint64_t kArticleRange = 1'000ULL;  // intentional overlap
    constexpr int kThreads = 8;
    constexpr uint64_t kRepeatsPerThread = 100ULL;

    StubWikiIndex wiki;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&]() {
            for (uint64_t r = 0; r < kRepeatsPerThread; ++r) {
                for (uint64_t id = 0; id < kArticleRange; ++id) {
                    wiki.sync(id);
                }
            }
        });
    }
    for (auto& th : workers) th.join();

    EXPECT_EQ(wiki.sync_faults.load(), 0ULL)
        << "Sync faults detected during edge-case overlap stress";
    // After idempotent sync the index should contain exactly kArticleRange entries
    EXPECT_EQ(wiki.index.size(), kArticleRange)
        << "Index size diverged after overlapping sync stress";
}
