/**
 * @file test_wiki_eval_metrics.cpp
 * @brief EVAL-01..10 — WikiIndexStore evaluation metric tests (Recall\@k / MRR / p95).
 *
 * Gate criterion (Wave B LLM): implementation of Recall\@k / MRR / p95 reporting
 * in WikiIndexStore (ROADMAP item Q4 2026).  All tests run against a real
 * in-memory RocksDB-backed WikiIndexStore; skipped automatically when RocksDB
 * is unavailable in the build environment.
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 */

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "llm/embedded_llm.h"
#include "llm/wiki_index_store.h"
#include "storage/rocksdb_wrapper.h"

using namespace themis::llm;
using namespace themis;

namespace {

std::string makeDbPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("wiki_eval_" + tag + "_" + std::to_string(ts)))
               .string();
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Fixture
// ═══════════════════════════════════════════════════════════════════════════

class WikiEvalMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeDbPath(
            ::testing::UnitTest::GetInstance()->current_test_info()->name());

        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path_;
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            GTEST_SKIP() << "[EVAL] RocksDB unavailable; skipping eval metric tests.";
        }

        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        vim_ = std::make_unique<VectorIndexManager>(*db_);

        // Deterministic 4-dim embed: one-hot by hash.
        llm_.setEmbedFn([](const std::string& text) -> std::vector<float> {
            size_t h = std::hash<std::string>{}(text);
            std::vector<float> v(4, 0.0f);
            v[h % 4] = 1.0f;
            return v;
        });
    }

    void TearDown() override {
        sim_.reset();
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::unique_ptr<WikiIndexStore> makeStore(const std::string& table) {
        WikiIndexConfig c;
        c.enable_phase_b = true;
        c.table_name     = table;
        c.embedding_dim  = 4;
        c.top_k          = 10;
        c.enable_bm25    = true;
        c.enable_vector  = true;
        c.rrf_k          = 60.0;
        return std::make_unique<WikiIndexStore>(*sim_, *vim_, llm_, c);
    }

    static WikiChunk makeChunk(const std::string& chunk_id,
                               const std::string& text,
                               const std::string& doc_id) {
        WikiChunk c;
        c.chunk_id = chunk_id;
        c.text     = text;
        c.doc_id   = doc_id;
        return c;
    }

    std::string                            db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::unique_ptr<VectorIndexManager>    vim_;
    EmbeddedLLM                            llm_;
};

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-01: empty stats before any query
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL01_EmptyBeforeAnyQuery) {
    auto store = makeStore("eval01");
    const WikiEvalStats s = store->getEvaluationStats();

    EXPECT_DOUBLE_EQ(s.recall_at_k1,  0.0);
    EXPECT_DOUBLE_EQ(s.recall_at_k3,  0.0);
    EXPECT_DOUBLE_EQ(s.recall_at_k5,  0.0);
    EXPECT_DOUBLE_EQ(s.recall_at_k10, 0.0);
    EXPECT_DOUBLE_EQ(s.mrr,           0.0);
    EXPECT_DOUBLE_EQ(s.p95_query_latency_ms, 0.0);
    EXPECT_EQ(s.query_count,       0u);
    EXPECT_EQ(s.total_query_count, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-02: p95 latency is non-zero after evaluateQuery() calls
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL02_P95LatencyRecorded) {
    auto store = makeStore("eval02");
    store->writeChunk(makeChunk("c1", "alpha text", "doc_a"));
    store->flush();

    for (int i = 0; i < 20; ++i) {
        store->evaluateQuery("alpha", 10, 0.0f, {});
    }

    const WikiEvalStats s = store->getEvaluationStats();
    EXPECT_EQ(s.total_query_count, 20u);
    EXPECT_GT(s.p95_query_latency_ms, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-03: perfect recall — relevant document returned at rank 1
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL03_PerfectRecall) {
    auto store = makeStore("eval03");
    store->writeChunk(makeChunk("c1", "alpha retrieval", "doc_alpha"));
    store->flush();

    store->evaluateQuery("alpha retrieval", 10, 0.0f, {"doc_alpha"});

    const WikiEvalStats s = store->getEvaluationStats();
    EXPECT_EQ(s.query_count, 1u);
    // recall@1 = 1/1 = 1.0 (only one relevant doc, returned in top-1)
    EXPECT_NEAR(s.recall_at_k1, 1.0, 1e-9);
    EXPECT_NEAR(s.recall_at_k5, 1.0, 1e-9);
    // MRR: first hit at rank 1 → 1.0
    EXPECT_NEAR(s.mrr, 1.0, 1e-9);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-04: zero recall — no relevant documents in results
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL04_ZeroRecall) {
    auto store = makeStore("eval04");
    store->writeChunk(makeChunk("c1", "unrelated content", "doc_x"));
    store->flush();

    // Ground truth contains a doc_id that is not in the corpus.
    store->evaluateQuery("unrelated content", 10, 0.0f, {"nonexistent_doc"});

    const WikiEvalStats s = store->getEvaluationStats();
    EXPECT_EQ(s.query_count, 1u);
    EXPECT_DOUBLE_EQ(s.recall_at_k1, 0.0);
    EXPECT_DOUBLE_EQ(s.recall_at_k5, 0.0);
    EXPECT_DOUBLE_EQ(s.mrr,          0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-05: evaluateQuery() with empty ground-truth still records latency
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL05_EmptyGroundTruthRecordsLatencyOnly) {
    auto store = makeStore("eval05");
    store->writeChunk(makeChunk("c1", "some content", "doc_a"));
    store->flush();

    store->evaluateQuery("some content", 10, 0.0f, {}); // no ground truth

    const WikiEvalStats s = store->getEvaluationStats();
    // With no ground truth, no recall/MRR update occurs.
    EXPECT_EQ(s.query_count, 0u);        // eval_query_count_ unchanged
    EXPECT_EQ(s.total_query_count, 1u);  // total_query_count_ incremented by query()
    EXPECT_GT(s.p95_query_latency_ms, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-06: MRR computation
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL06_MRR_FirstHitRank1) {
    auto store = makeStore("eval06");
    store->writeChunk(makeChunk("c1", "target document content", "target_doc"));
    store->flush();

    store->evaluateQuery("target document content", 10, 0.0f, {"target_doc"});

    const WikiEvalStats s = store->getEvaluationStats();
    // Relevant doc should be first result → MRR = 1.0
    EXPECT_NEAR(s.mrr, 1.0, 1e-9);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-07: running mean updates correctly across multiple calls
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL07_RunningMeanMultipleCalls) {
    auto store = makeStore("eval07");
    store->writeChunk(makeChunk("c1", "recall test content", "hit_doc"));
    store->flush();

    // Query 1: hit_doc is relevant and returned → recall@1 = 1.0
    store->evaluateQuery("recall test content", 10, 0.0f, {"hit_doc"});
    // Query 2: ground truth has a missing doc → recall@1 = 0.0
    store->evaluateQuery("recall test content", 10, 0.0f, {"missing_doc"});

    const WikiEvalStats s = store->getEvaluationStats();
    EXPECT_EQ(s.query_count, 2u);
    // Mean recall@1 = (1.0 + 0.0) / 2 = 0.5
    EXPECT_NEAR(s.recall_at_k1, 0.5, 1e-9);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-08: resetEvaluationStats() clears all accumulators and ring buffer
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL08_ResetClearsAllState) {
    auto store = makeStore("eval08");
    store->writeChunk(makeChunk("c1", "test chunk", "doc_a"));
    store->flush();

    store->evaluateQuery("test chunk", 10, 0.0f, {"doc_a"});
    {
        const WikiEvalStats s = store->getEvaluationStats();
        EXPECT_EQ(s.query_count, 1u);
        EXPECT_GT(s.total_query_count, 0u);
    }

    store->resetEvaluationStats();

    {
        const WikiEvalStats s = store->getEvaluationStats();
        EXPECT_EQ(s.query_count,       0u);
        EXPECT_EQ(s.total_query_count, 0u);
        EXPECT_DOUBLE_EQ(s.recall_at_k1,         0.0);
        EXPECT_DOUBLE_EQ(s.mrr,                  0.0);
        EXPECT_DOUBLE_EQ(s.p95_query_latency_ms, 0.0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-09: concurrent evaluateQuery() calls are thread-safe
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL09_ConcurrentCallsAreSafe) {
    constexpr int kThreads          = 4;
    constexpr int kQueriesPerThread = 10;

    auto store = makeStore("eval09");
    store->writeChunk(makeChunk("c1", "concurrent test chunk", "cdoc"));
    store->flush();

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    std::atomic<int> errors{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&store, &errors]() {
            for (int i = 0; i < kQueriesPerThread; ++i) {
                try {
                    store->evaluateQuery("concurrent test chunk", 5, 0.0f, {"cdoc"});
                } catch (...) {
                    ++errors;
                }
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(errors.load(), 0);

    const WikiEvalStats s = store->getEvaluationStats();
    EXPECT_EQ(s.query_count,
              static_cast<std::size_t>(kThreads * kQueriesPerThread));
    // All hits → recall@1 converges to 1.0
    EXPECT_NEAR(s.recall_at_k1, 1.0, 1e-6);
}

// ─────────────────────────────────────────────────────────────────────────────
// EVAL-10: ring buffer wraps correctly after > kLatencyRingSize queries
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(WikiEvalMetricsTest, EVAL10_LatencyRingBufferWraps) {
    auto store = makeStore("eval10");
    store->writeChunk(makeChunk("c1", "ring buffer test", "rdoc"));
    store->flush();

    // Issue more queries than kLatencyRingSize (1024) to force wrapping.
    constexpr std::size_t kQueries = 1100;
    for (std::size_t i = 0; i < kQueries; ++i) {
        store->evaluateQuery("ring buffer test", 5, 0.0f, {});
    }

    const WikiEvalStats s = store->getEvaluationStats();
    EXPECT_EQ(s.total_query_count, kQueries);
    // p95 must remain a sane positive value after ring-buffer wrap.
    EXPECT_GT(s.p95_query_latency_ms, 0.0);
}
