/**
 * @file test_wiki_index_store_phase_b.cpp
 * @brief Phase B integration tests (WIS-B-01..16) for WikiIndexStore.
 *
 * These tests cover the full RocksDB-backed path of WikiIndexStore:
 *  - BM25 fulltext retrieval via SecondaryIndexManager
 *  - HNSW vector retrieval via VectorIndexManager
 *  - RRF hybrid fusion via HybridRetriever
 *  - shared_mutex thread-safety contract
 *
 * Tests require a functional RocksDB environment.  If the database cannot
 * be opened, all tests in this suite are skipped with GTEST_SKIP().
 *
 * Tests:
 *  WIS-B-01: isReady() returns true after successful construction
 *  WIS-B-02: query on empty store returns empty vector
 *  WIS-B-03: writeChunk() auto-embeds and BM25 query finds the chunk
 *  WIS-B-04: writeChunk() with pre-populated embedding; KNN query finds it
 *  WIS-B-05: writeBatch() embeds missing vectors; hybrid query finds all chunks
 *  WIS-B-06: top_k parameter limits the number of results
 *  WIS-B-07: min_score=1.0 filters out all results (returns empty)
 *  WIS-B-08: pre-embedded chunk: EmbeddedLLM::embed() not called for that chunk
 *  WIS-B-09: concurrent query() calls complete without crash or data race
 *  WIS-B-10: concurrent writeChunk() + query() maintain locking contract
 *  WIS-B-11: returned results are in descending score order
 *  WIS-B-12: BM25-only mode (enable_vector=false) returns results from fulltext scan
 *  WIS-B-13: vector-only mode (enable_bm25=false) returns results from KNN scan
 *  WIS-B-14: flush() completes without throwing
 *  WIS-B-15: query embedding cache: embed() called once per unique query text
 *  WIS-B-16: chunks from multiple doc_ids are stored and independently queryable
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 */

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "llm/embedded_llm.h"
#include "llm/wiki_index_store.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace themis::llm;
using namespace themis;

namespace {

/// Generate a unique temp-dir path to isolate each test's RocksDB instance.
std::string makeDbPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("wiki_phase_b_" + tag + "_" + std::to_string(ts)))
               .string();
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Test fixture
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief GTest fixture for WikiIndexStore Phase B integration tests.
 *
 * Lifecycle:
 *  - SetUp  : opens a fresh RocksDB instance in a temp directory, constructs
 *             SecondaryIndexManager + VectorIndexManager, and configures a
 *             deterministic 4-dimensional embed function via EmbeddedLLM::setEmbedFn().
 *  - TearDown: destroys managers, closes DB, removes temp directory.
 *
 * Individual tests may override the embed function via llm_.setEmbedFn() and
 * create their own WikiIndexStore via makeStore() with a per-test table name
 * to prevent cross-test state pollution.
 */
class WikiIndexStorePhaseB : public ::testing::Test {
protected:
    // ─── Fixture setup ────────────────────────────────────────────────────

    void SetUp() override {
        db_path_ = makeDbPath(
            ::testing::UnitTest::GetInstance()->current_test_info()->name());

        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path_;
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            GTEST_SKIP() << "[WIS-B] RocksDB unavailable; skipping Phase B tests.";
        }

        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        vim_ = std::make_unique<VectorIndexManager>(*db_);

        setDefaultEmbedFn();
    }

    void TearDown() override {
        // Destroy index managers before closing DB
        sim_.reset();
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // ─── Helpers ──────────────────────────────────────────────────────────

    /// Install the default 4-dimensional deterministic embed function and
    /// reset the embed call counter.
    ///
    /// The function produces a one-hot basis vector based on `std::hash<std::string>`
    /// so that distinct texts reliably map to distinct (or at worst the same)
    /// 4-dim unit vector — deterministic across a single test run.
    void setDefaultEmbedFn() {
        embed_calls_.store(0);
        llm_.setEmbedFn([this](const std::string& text) -> std::vector<float> {
            ++embed_calls_;
            size_t h = std::hash<std::string>{}(text);
            std::vector<float> v(4, 0.0f);
            v[h % 4] = 1.0f;
            return v;
        });
    }

    /// Create a WikiIndexStore backed by the fixture's shared DB managers.
    ///
    /// @param table        RocksDB key-space prefix; use a per-test name to
    ///                     avoid cross-test state pollution.
    /// @param enable_bm25  Include BM25 fulltext in retrieval.
    /// @param enable_vec   Include KNN vector in retrieval.
    /// @return             Heap-allocated store; caller owns lifetime.
    std::unique_ptr<WikiIndexStore> makeStore(
        const std::string& table,
        bool               enable_bm25 = true,
        bool               enable_vec  = true,
        bool               enable_persistent_cache = false,
        std::size_t        embedding_cache_max_bytes = 0) {
        WikiIndexConfig c;
        c.enable_phase_b = true;
        c.table_name    = table;
        c.embedding_dim = 4;
        c.top_k         = 10;
        c.enable_bm25   = enable_bm25;
        c.enable_vector = enable_vec;
        c.rrf_k         = 60.0;
        c.enable_persistent_cache = enable_persistent_cache;
        c.embedding_cache_max_bytes = embedding_cache_max_bytes;
        return std::make_unique<WikiIndexStore>(*sim_, *vim_, llm_, c);
    }

    /// Build a minimal WikiChunk for use in tests.
    static WikiChunk makeChunk(const std::string& chunk_id,
                               const std::string& text,
                               const std::string& doc_id = "doc1") {
        WikiChunk c;
        c.chunk_id = chunk_id;
        c.text     = text;
        c.doc_id   = doc_id;
        return c;
    }

    /// Build a WikiChunk with a pre-populated embedding.
    static WikiChunk makeEmbeddedChunk(const std::string& chunk_id,
                                        const std::string& text,
                                        std::vector<float> emb,
                                        const std::string& doc_id = "doc1") {
        WikiChunk c = makeChunk(chunk_id, text, doc_id);
        c.embedding = std::move(emb);
        return c;
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::unique_ptr<VectorIndexManager>    vim_;
    EmbeddedLLM                            llm_;
    std::atomic<int>                       embed_calls_{0};
};

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-01: isReady() after construction
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-01 — `WikiIndexStore::isReady()` returns true after the
///       constructor has initialised both the fulltext and vector indexes.
TEST_F(WikiIndexStorePhaseB, WIS_B_01_IsReadyAfterConstruction) {
    auto store = makeStore("wis_b_01");
    EXPECT_TRUE(store->isReady());
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-02: empty store returns empty query result
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-02 — Querying an empty WikiIndexStore returns an empty vector.
TEST_F(WikiIndexStorePhaseB, WIS_B_02_EmptyStoreReturnsEmpty) {
    auto store  = makeStore("wis_b_02");
    auto result = store->query("any query text", 10, 0.0f);
    EXPECT_TRUE(result.empty());
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-03: writeChunk() + BM25 query
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-03 — `writeChunk()` stores a chunk and BM25 fulltext scanning
///       returns it for a query containing a distinctive term present in the
///       chunk text.
TEST_F(WikiIndexStorePhaseB, WIS_B_03_WriteChunkBM25Query) {
    auto store = makeStore("wis_b_03", /*enable_bm25=*/true, /*enable_vec=*/false);

    // Use a token unlikely to collide with any other document.
    WikiChunk c = makeChunk("c-bm25-01",
                             "xyzzy_bm25_marker alpha bravo gamma");
    store->writeChunk(std::move(c));

    auto results = store->query("xyzzy_bm25_marker", 10, 0.0f);
    ASSERT_FALSE(results.empty()) << "BM25 query should return at least one result";
    // The stored chunk must be in the result set.
    bool found = false;
    for (const auto& r : results) {
        if (r.chunk_id == "c-bm25-01") {
            found = true;
            EXPECT_GT(r.score, 0.0f) << "BM25 score must be positive";
        }
    }
    EXPECT_TRUE(found) << "Written chunk not found by BM25 query";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-04: writeChunk() with pre-embedded chunk + KNN query
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-04 — A chunk with a pre-populated embedding is stored as-is
///       and returned by a KNN query whose embedding is close to the stored one.
TEST_F(WikiIndexStorePhaseB, WIS_B_04_PreEmbeddedChunkKNNQuery) {
    // Override embed to always return {1,0,0,0} so query embedding is known.
    llm_.setEmbedFn([this](const std::string&) -> std::vector<float> {
        ++embed_calls_;
        return {1.0f, 0.0f, 0.0f, 0.0f};
    });

    auto store = makeStore("wis_b_04", /*enable_bm25=*/false, /*enable_vec=*/true);

    // Pre-embedded chunk: identical embedding → cosine distance = 0, similarity = 1.
    WikiChunk c = makeEmbeddedChunk("c-knn-01", "vector search content",
                                    {1.0f, 0.0f, 0.0f, 0.0f});
    int calls_before = embed_calls_.load();
    store->writeChunk(std::move(c));
    // A pre-embedded chunk must NOT trigger embed().
    EXPECT_EQ(embed_calls_.load(), calls_before)
        << "embed() must not be called for a pre-embedded chunk";

    auto results = store->query("vector query text", 10, 0.0f);
    ASSERT_FALSE(results.empty()) << "KNN query should return the stored chunk";
    bool found = false;
    for (const auto& r : results) {
        if (r.chunk_id == "c-knn-01") {
            found = true;
            EXPECT_GE(r.score, 0.0f) << "KNN score must be non-negative";
        }
    }
    EXPECT_TRUE(found) << "Pre-embedded chunk not found by KNN query";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-05: writeBatch() + hybrid query finds all chunks
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-05 — `writeBatch()` embeds chunks lacking embeddings and stores
///       all of them; a subsequent hybrid query (BM25 + KNN) finds them all.
TEST_F(WikiIndexStorePhaseB, WIS_B_05_WriteBatchHybridQuery) {
    auto store = makeStore("wis_b_05");

    std::vector<WikiChunk> batch;
    for (int i = 0; i < 5; ++i) {
        batch.push_back(makeChunk(
            "c-batch-" + std::to_string(i),
            "batch_term_wis_b_05 document " + std::to_string(i)));
    }

    store->writeBatch(std::move(batch));

    auto results = store->query("batch_term_wis_b_05", 20, 0.0f);
    EXPECT_FALSE(results.empty())
        << "Hybrid query after writeBatch() must return at least one result";
    // All 5 chunk IDs must be retrievable.
    for (int i = 0; i < 5; ++i) {
        std::string id = "c-batch-" + std::to_string(i);
        bool found = false;
        for (const auto& r : results) {
            if (r.chunk_id == id) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Chunk " << id << " missing from batch query results";
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-06: top_k limits results
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-06 — `query()` respects the `top_k` parameter: no more than
///       `top_k` results are returned even when more matching chunks exist.
TEST_F(WikiIndexStorePhaseB, WIS_B_06_TopKLimitsResults) {
    auto store = makeStore("wis_b_06");

    for (int i = 0; i < 8; ++i) {
        store->writeChunk(makeChunk(
            "c-topk-" + std::to_string(i),
            "topk_wis_b_06_term chunk number " + std::to_string(i)));
    }

    auto results_3 = store->query("topk_wis_b_06_term", /*top_k=*/3, 0.0f);
    EXPECT_LE(static_cast<int>(results_3.size()), 3)
        << "top_k=3 must not return more than 3 results";

    auto results_1 = store->query("topk_wis_b_06_term", /*top_k=*/1, 0.0f);
    EXPECT_LE(static_cast<int>(results_1.size()), 1)
        << "top_k=1 must not return more than 1 result";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-07: min_score filters out all results
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-07 — When `min_score` is set to 1.0f (above all attainable
///       RRF scores), the result vector is empty.
TEST_F(WikiIndexStorePhaseB, WIS_B_07_MinScoreFiltersAll) {
    auto store = makeStore("wis_b_07");

    store->writeChunk(makeChunk("c-minscore-01",
                                "min_score_wis_b_07 filterable content"));

    // RRF hybrid scores are in (0, 1) range; requiring exactly 1.0 should
    // filter all results.
    auto results = store->query("min_score_wis_b_07", 10, /*min_score=*/1.0f);
    EXPECT_TRUE(results.empty())
        << "min_score=1.0 must filter out all results";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-08: pre-embedded chunk does not trigger re-embedding
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-08 — A chunk whose `embedding` field is already populated is
///       written as-is; `EmbeddedLLM::embed()` must not be invoked for it.
TEST_F(WikiIndexStorePhaseB, WIS_B_08_PreEmbeddedNoReEmbed) {
    auto store = makeStore("wis_b_08");

    WikiChunk pre_emb = makeEmbeddedChunk("c-preemb-01",
                                           "pre_embedded content",
                                           {0.5f, 0.5f, 0.0f, 0.0f});
    WikiChunk no_emb  = makeChunk("c-noemb-01", "plain content needs embedding");

    int before = embed_calls_.load();
    store->writeChunk(std::move(pre_emb));
    int after_pre = embed_calls_.load();
    store->writeChunk(std::move(no_emb));
    int after_no = embed_calls_.load();

    EXPECT_EQ(after_pre, before)
        << "embed() must not be called for a pre-embedded chunk";
    EXPECT_GT(after_no, after_pre)
        << "embed() must be called for a chunk without an embedding";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-09: concurrent readers
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-09 — Multiple concurrent `query()` calls complete without
///       crash, data corruption, or assertion failure.  Validates that
///       `shared_mutex` allows concurrent readers.
TEST_F(WikiIndexStorePhaseB, WIS_B_09_ConcurrentReaders) {
    auto store = makeStore("wis_b_09");

    for (int i = 0; i < 5; ++i) {
        store->writeChunk(makeChunk(
            "c-conc-r-" + std::to_string(i),
            "concurrent_reader_wis_b_09 doc " + std::to_string(i)));
    }

    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    std::atomic<int> errors{0};

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&]() {
            try {
                auto r = store->query("concurrent_reader_wis_b_09", 10, 0.0f);
                (void)r; // result existence is sufficient; size checked in -05
            } catch (...) {
                ++errors;
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(errors.load(), 0) << "Concurrent readers must not throw";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-10: concurrent writer + readers
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-10 — A writer thread calling `writeChunk()` concurrently with
///       multiple reader threads calling `query()` must not produce crashes,
///       deadlocks, or thrown exceptions.  Validates the exclusive/shared
///       locking contract.
TEST_F(WikiIndexStorePhaseB, WIS_B_10_ConcurrentWriterAndReaders) {
    auto store = makeStore("wis_b_10");

    // Seed with some initial data so readers can find results immediately.
    for (int i = 0; i < 4; ++i) {
        store->writeChunk(makeChunk(
            "c-seed-" + std::to_string(i),
            "writer_reader_wis_b_10 seed " + std::to_string(i)));
    }

    std::atomic<int> errors{0};
    constexpr int kReaders = 6;
    constexpr int kWrites  = 4;

    std::vector<std::thread> threads;

    // Writer thread
    threads.emplace_back([&]() {
        for (int i = 0; i < kWrites; ++i) {
            try {
                store->writeChunk(makeChunk(
                    "c-write-" + std::to_string(i),
                    "writer_reader_wis_b_10 write " + std::to_string(i)));
            } catch (...) {
                ++errors;
            }
        }
    });

    // Reader threads
    for (int t = 0; t < kReaders; ++t) {
        threads.emplace_back([&]() {
            for (int q = 0; q < 4; ++q) {
                try {
                    auto r = store->query("writer_reader_wis_b_10", 10, 0.0f);
                    (void)r;
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(errors.load(), 0)
        << "Concurrent writer + readers must not throw or crash";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-11: results are in descending score order
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-11 — When multiple chunks are returned, their `score` fields
///       are in non-ascending (descending) order.
TEST_F(WikiIndexStorePhaseB, WIS_B_11_ResultsDescendingOrder) {
    auto store = makeStore("wis_b_11");

    for (int i = 0; i < 6; ++i) {
        store->writeChunk(makeChunk(
            "c-ord-" + std::to_string(i),
            "ordering_wis_b_11 token delta " + std::to_string(i)));
    }

    auto results = store->query("ordering_wis_b_11", 10, 0.0f);
    ASSERT_GE(results.size(), 2u)
        << "Need at least 2 results to check ordering";

    for (std::size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].score, results[i].score)
            << "Results must be in non-ascending score order at index "
            << i - 1 << " vs " << i;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-12: BM25-only mode
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-12 — With `enable_vector=false`, only BM25 results are returned.
///       A chunk containing a distinctive token is found via fulltext scan alone.
TEST_F(WikiIndexStorePhaseB, WIS_B_12_BM25OnlyMode) {
    auto store = makeStore("wis_b_12", /*enable_bm25=*/true, /*enable_vec=*/false);

    store->writeChunk(
        makeChunk("c-bm25only-01", "bm25only_wis_b_12_token unique content here"));

    auto results = store->query("bm25only_wis_b_12_token", 10, 0.0f);
    ASSERT_FALSE(results.empty())
        << "BM25-only store must return results for matching query";
    bool found = false;
    for (const auto& r : results) {
        if (r.chunk_id == "c-bm25only-01") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Chunk not found in BM25-only retrieval";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-13: vector-only mode
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-13 — With `enable_bm25=false`, only KNN results are returned.
///       A pre-embedded chunk is found via vector similarity alone.
TEST_F(WikiIndexStorePhaseB, WIS_B_13_VectorOnlyMode) {
    // All embeds return {0,1,0,0} so query and stored chunk share the same
    // 4-dim unit vector → cosine distance = 0, similarity = 1.
    llm_.setEmbedFn([this](const std::string&) -> std::vector<float> {
        ++embed_calls_;
        return {0.0f, 1.0f, 0.0f, 0.0f};
    });

    auto store = makeStore("wis_b_13", /*enable_bm25=*/false, /*enable_vec=*/true);

    store->writeChunk(makeChunk("c-veconly-01", "vector only content wis b thirteen"));

    auto results = store->query("any query text", 10, 0.0f);
    ASSERT_FALSE(results.empty())
        << "Vector-only store must return results for embedded query";
    bool found = false;
    for (const auto& r : results) {
        if (r.chunk_id == "c-veconly-01") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Chunk not found in vector-only retrieval";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-14: flush() is a no-op and does not throw
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-14 — `flush()` completes without throwing an exception, on
///       both an empty store and after writing data.
TEST_F(WikiIndexStorePhaseB, WIS_B_14_FlushNoOp) {
    auto store = makeStore("wis_b_14");

    // flush on empty store
    EXPECT_NO_THROW(store->flush());

    store->writeChunk(makeChunk("c-flush-01", "flush test content"));
    // flush after write
    EXPECT_NO_THROW(store->flush());
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-15: query embedding cache
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-15 — The same query text repeated across two `query()` calls
///       only invokes `EmbeddedLLM::embed()` once; the second call uses the
///       cached embedding.
TEST_F(WikiIndexStorePhaseB, WIS_B_15_QueryEmbeddingCache) {
    auto store = makeStore("wis_b_15", /*enable_bm25=*/false, /*enable_vec=*/true);

    store->writeChunk(makeChunk("c-cache-01", "embedding cache wis b fifteen"));

    const std::string q = "unique_cache_query_wis_b_15";

    int before = embed_calls_.load();
    store->query(q, 10, 0.0f);
    int after_first = embed_calls_.load();
    EXPECT_GT(after_first, before) << "First query must call embed()";

    // Reset counter to isolate second-call behaviour.
    embed_calls_.store(0);
    store->query(q, 10, 0.0f);
    EXPECT_EQ(embed_calls_.load(), 0)
        << "Second query with same text must use cache (no embed() call)";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-16: multiple doc_ids are independently queryable
// ═══════════════════════════════════════════════════════════════════════════

/// @test WIS-B-16 — Chunks from distinct doc_ids stored via `writeBatch()` are
///       all retrievable.  The store does not conflate or lose chunks from
///       different source documents.
TEST_F(WikiIndexStorePhaseB, WIS_B_16_MultipleDocIds) {
    auto store = makeStore("wis_b_16");

    std::vector<WikiChunk> batch;
    for (int d = 0; d < 3; ++d) {
        for (int c = 0; c < 2; ++c) {
            batch.push_back(makeChunk(
                "c-doc" + std::to_string(d) + "-" + std::to_string(c),
                "multidoc_wis_b_16_term doc" + std::to_string(d) + " chunk" +
                    std::to_string(c),
                "doc-" + std::to_string(d)));
        }
    }

    store->writeBatch(std::move(batch));

    auto results = store->query("multidoc_wis_b_16_term", 20, 0.0f);
    EXPECT_FALSE(results.empty())
        << "Query must return results from multiple doc_ids";

    // Collect all returned chunk IDs.
    std::vector<std::string> returned_ids;
    for (const auto& r : results) {
        returned_ids.push_back(r.chunk_id);
    }

    // All 6 chunks must be findable.
    for (int d = 0; d < 3; ++d) {
        for (int c = 0; c < 2; ++c) {
            std::string id =
                "c-doc" + std::to_string(d) + "-" + std::to_string(c);
            bool found = false;
            for (const auto& rid : returned_ids) {
                if (rid == id) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << "Chunk " << id << " missing from results";
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-17: persistent cache key uses doc_id+content hash (chunk_id-independent)
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WikiIndexStorePhaseB, WIS_B_17_PersistentCacheHashKeyIndependentFromChunkId) {
    auto store = makeStore("wis_b_17", true, true, /*persistent*/true);

    WikiChunk first = makeChunk("hash-chunk-1", "same-content-for-hash-key", "doc-hash");
    WikiChunk second = makeChunk("hash-chunk-2", "same-content-for-hash-key", "doc-hash");

    store->writeChunk(first);
    EXPECT_EQ(embed_calls_.load(), 1) << "First insert must compute embedding";

    store->writeChunk(second);
    EXPECT_EQ(embed_calls_.load(), 1)
        << "Second insert with same doc_id+content must reuse hash-keyed cache";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-18: deterministic LRU eviction by byte limit
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WikiIndexStorePhaseB, WIS_B_18_EmbeddingCacheLruEvictionByBytes) {
    auto store = makeStore("wis_b_18", true, true, /*persistent*/false, /*max_bytes*/120);

    store->writeChunk(makeChunk("lru-a", "content-a-for-lru", "doc-lru"));
    store->writeChunk(makeChunk("lru-b", "content-b-for-lru", "doc-lru"));
    EXPECT_EQ(embed_calls_.load(), 2);

    embed_calls_.store(0);
    // Reinsert same logical content. If A was evicted, embedding is recomputed.
    store->writeChunk(makeChunk("lru-a-reinsert", "content-a-for-lru", "doc-lru"));
    EXPECT_EQ(embed_calls_.load(), 1)
        << "Reinsert should recompute after deterministic LRU eviction";
}

// ═══════════════════════════════════════════════════════════════════════════
// WIS-B-19: Phase A legacy cache migration is idempotent and survives restart
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(WikiIndexStorePhaseB, WIS_B_19_LegacyCacheMigrationPersistsAcrossRestart) {
    const std::string table = "wis_b_19";

    // Seed legacy cache schema (<table>_emb_cache keyed by chunk_id).
    BaseEntity legacy;
    legacy.setPrimaryKey("legacy-pk");
    legacy.setField("chunk_id", "legacy-01");
    legacy.setField("embedding", std::vector<float>{0.0f, 1.0f, 0.0f, 0.0f});
    auto seed_status = sim_->put(table + "_emb_cache", legacy);
    ASSERT_TRUE(seed_status.ok) << seed_status.message;

    {
        auto store = makeStore(table, true, true, /*persistent*/true);
        embed_calls_.store(0);
        store->writeChunk(makeChunk("legacy-01", "legacy-content", "legacy-doc"));
        EXPECT_LE(embed_calls_.load(), 1)
            << "Legacy migration may perform at most one bootstrap embed on first run";
    }

    {
        auto store = makeStore(table, true, true, /*persistent*/true);
        embed_calls_.store(0);
        store->writeChunk(makeChunk("legacy-02", "legacy-content", "legacy-doc"));
        EXPECT_EQ(embed_calls_.load(), 0)
            << "Second run must reuse migrated hash-key cache (idempotent migration)";
    }
}
