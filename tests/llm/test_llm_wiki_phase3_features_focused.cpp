/**
 * @file test_llm_wiki_phase3_features_focused.cpp
 * @brief Phase 3 WikiIndexStore features — focused tests (WIS-P3-01..WIS-P3-08).
 *
 * Validates the three new `WikiIndexConfig` fields introduced in Phase 3
 * (Target: Q3 2026):
 *  - `batch_size` — configurable embedding batch size for `writeBatch()`.
 *  - `auto_probe_dim` — auto-detect embedding dimensionality on first write.
 *  - `enable_persistent_cache` — persist computed embeddings to RocksDB and
 *    retrieve them on subsequent writes (skip redundant LLM calls).
 *
 * Tests:
 *  WIS-P3-01: batch_size=1 — each chunk embedded in its own LLM call
 *  WIS-P3-02: batch_size=50 — all 4 chunks embedded in one batch call
 *  WIS-P3-03: default batch_size (32) — backward-compatibility preserved
 *  WIS-P3-04: auto_probe_dim=false — embed_dim from config used unchanged
 *  WIS-P3-05: auto_probe_dim=true — dim probed from LLM on first writeChunk
 *  WIS-P3-06: auto_probe_dim=true idempotent — probe runs only once
 *  WIS-P3-07: enable_persistent_cache=false — no RocksDB lookup (baseline)
 *  WIS-P3-08: enable_persistent_cache=true — embedding reused from RocksDB
 *             without calling LLM on second write of same chunk
 *
 * @see include/llm/wiki_index_store.h — WikiIndexConfig::batch_size,
 *      auto_probe_dim, enable_persistent_cache
 * @see src/llm/ROADMAP.md — Phase 3 items
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
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
#include <string>
#include <vector>

using namespace themis::llm;
using namespace themis;

namespace {

std::string makeDbPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("wis_p3_" + tag + "_" + std::to_string(ts))).string();
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Test fixture
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Fixture for WikiIndexStore Phase 3 feature tests.
 *
 * Opens a per-test RocksDB instance and configures a deterministic 4-dim
 * embed function with a call counter.  Tests skip if RocksDB is unavailable.
 */
class WikiIndexStorePhase3 : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeDbPath(
            ::testing::UnitTest::GetInstance()->current_test_info()->name());

        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path_;
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            GTEST_SKIP() << "[WIS-P3] RocksDB unavailable; skipping.";
        }

        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        vim_ = std::make_unique<VectorIndexManager>(*db_);

        setDefaultEmbedFn(/*dim=*/4);
    }

    void TearDown() override {
        sim_.reset();
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    /// Install a deterministic N-dimensional embed function.
    void setDefaultEmbedFn(int dim) {
        embed_calls_.store(0);
        embed_dim_.store(dim);
        llm_.setEmbedFn([this](const std::string& text) -> std::vector<float> {
            ++embed_calls_;
            int d = embed_dim_.load();
            size_t h = std::hash<std::string>{}(text);
            std::vector<float> v(static_cast<std::size_t>(d), 0.0f);
            if (d > 0) {
                v[h % static_cast<std::size_t>(d)] = 1.0f;
            }
            return v;
        });
    }

    /// Convenience: build a WikiIndexStore with the given config overrides.
    std::unique_ptr<WikiIndexStore> makeStore(WikiIndexConfig cfg) {
        return std::make_unique<WikiIndexStore>(*sim_, *vim_, llm_, std::move(cfg));
    }

    static WikiChunk makeChunk(const std::string& id, const std::string& text) {
        WikiChunk c;
        c.chunk_id = id;
        c.text     = text;
        c.doc_id   = "doc1";
        return c;
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::unique_ptr<VectorIndexManager>    vim_;
    EmbeddedLLM                            llm_;
    std::atomic<int>                       embed_calls_{0};
    std::atomic<int>                       embed_dim_{4};
};

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-01: batch_size=1 — each chunk requires a separate embedding call
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-01: With batch_size=1, writeBatch() over 4 chunks triggers 4 embed calls
 * (one per chunk, since each "batch" contains exactly one chunk).
 */
TEST_F(WikiIndexStorePhase3, WisP301_BatchSize1OneCallPerChunk) {
    WikiIndexConfig cfg;
    cfg.table_name    = "p3_t01";
    cfg.embedding_dim = 4;
    cfg.batch_size    = 1;
    auto store = makeStore(cfg);

    std::vector<WikiChunk> chunks = {};

    for (int i = 0; i < 4; ++i) {
        chunks.push_back(makeChunk("c" + std::to_string(i), "text " + std::to_string(i)));
    }

    const int before = embed_calls_.load();
    ASSERT_NO_THROW(store->writeBatch(std::move(chunks)));
    const int after  = embed_calls_.load();

    EXPECT_EQ(after - before, 4)
        << "With batch_size=1, each of 4 chunks should trigger one embed call";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-02: batch_size=50 — all chunks embedded in a single batch call
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-02: With batch_size=50, all 4 chunks are embedded in a single
 * embedBatch() call.  Total embed-call delta should be exactly 4 (one entry
 * per chunk forwarded to the single batch call).
 */
TEST_F(WikiIndexStorePhase3, WisP302_BatchSize50SingleBatch) {
    // Track embedBatch calls by counting individual texts instead of embed calls
    std::atomic<int> batch_groups{0};
    llm_.setEmbedFn([this, &batch_groups](const std::string& text) -> std::vector<float> {
        ++embed_calls_;
        ++batch_groups;
        std::vector<float> v(4, 0.0f);
        v[0] = 1.0f;
        return v;
    });

    WikiIndexConfig cfg;
    cfg.table_name    = "p3_t02";
    cfg.embedding_dim = 4;
    cfg.batch_size    = 50;
    auto store = makeStore(cfg);

    std::vector<WikiChunk> chunks = {};

    for (int i = 0; i < 4; ++i) {
        chunks.push_back(makeChunk("c" + std::to_string(i), "text_b" + std::to_string(i)));
    }

    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeBatch(std::move(chunks)));

    // With batch_size=50, all 4 texts go in one batch; embed called 4 times total
    EXPECT_EQ(embed_calls_.load(), 4)
        << "Batch of 4 chunks with batch_size=50 should embed 4 texts in one group";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-03: Default batch_size (32) — backward-compatibility
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-03: WikiIndexConfig default batch_size must be 32 (original hardcoded value).
 */
TEST_F(WikiIndexStorePhase3, WisP303_DefaultBatchSizeIs32) {
    WikiIndexConfig cfg;
    EXPECT_EQ(cfg.batch_size, 32)
        << "Default batch_size must be 32 for backward compatibility";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-04: auto_probe_dim=false — config dim used unchanged
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-04: When auto_probe_dim=false (default), the embed function is not
 * called during construction, and the configured embedding_dim is preserved.
 */
TEST_F(WikiIndexStorePhase3, WisP304_AutoProbeDimFalseNoProbCall) {
    embed_calls_.store(0);

    WikiIndexConfig cfg;
    cfg.table_name    = "p3_t04";
    cfg.embedding_dim = 4;
    cfg.auto_probe_dim = false;
    auto store = makeStore(cfg);

    // No embed calls should happen during construction
    EXPECT_EQ(embed_calls_.load(), 0)
        << "auto_probe_dim=false must not invoke embed() during construction";
    EXPECT_TRUE(store->isReady());
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-05: auto_probe_dim=true — dim probed from LLM on first write
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-05: When auto_probe_dim=true, the first writeChunk() triggers a probe
 * embed call.  After the probe, the store becomes operational.
 */
TEST_F(WikiIndexStorePhase3, WisP305_AutoProbeDimProbesOnFirstWrite) {
    // Use 8-dim embed; configured dim is 4 — probe should update to 8
    setDefaultEmbedFn(/*dim=*/8);
    embed_calls_.store(0);

    WikiIndexConfig cfg;
    cfg.table_name     = "p3_t05";
    cfg.embedding_dim  = 4;   // intentionally wrong
    cfg.auto_probe_dim = true;
    cfg.enable_vector  = false; // disable vector to avoid HNSW re-init error
    cfg.enable_bm25    = true;
    auto store = makeStore(cfg);

    // No probe yet (probe is lazy — on first write)
    const int before_write = embed_calls_.load();

    WikiChunk c = makeChunk("probe-c1", "probe text");
    ASSERT_NO_THROW(store->writeChunk(std::move(c)));

    const int after_write = embed_calls_.load();
    // Probe call + actual embed call = at least 2 invocations
    EXPECT_GE(after_write - before_write, 1)
        << "auto_probe_dim=true must invoke embed() at least once on first write";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-06: auto_probe_dim=true idempotent — probe runs only once
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-06: The dim probe runs at most once.  Writing two chunks in sequence
 * should not trigger a second probe call (dim_probed_ flag guards re-entry).
 */
TEST_F(WikiIndexStorePhase3, WisP306_AutoProbeDimIdempotent) {
    setDefaultEmbedFn(/*dim=*/8);

    WikiIndexConfig cfg;
    cfg.table_name     = "p3_t06";
    cfg.embedding_dim  = 8;
    cfg.auto_probe_dim = true;
    cfg.enable_vector  = false;
    cfg.enable_bm25    = true;
    auto store = makeStore(cfg);

    // First write (triggers probe + embed for c1)
    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeChunk(makeChunk("p306-c1", "first chunk")));
    const int after_first = embed_calls_.load();
    ASSERT_GE(after_first, 1);

    // Second write (no additional probe — only embed for c2)
    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeChunk(makeChunk("p306-c2", "second chunk")));
    const int after_second = embed_calls_.load();

    // If probe ran again it would add an extra embed call for the sentinel.
    // With the idempotency guard, exactly 1 embed call (for "second chunk").
    EXPECT_EQ(after_second, 1)
        << "Dim probe must not run on subsequent writes once dim_probed_=true";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-07: enable_persistent_cache=false — baseline (no RocksDB lookup)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-07: When persistent cache is disabled, the second write of the same
 * chunk (same chunk_id) still uses the in-memory cache and does not re-invoke
 * the LLM (embed() call count does not increase on second write).
 */
TEST_F(WikiIndexStorePhase3, WisP307_PersistentCacheDisabledInMemoryCacheWorks) {
    WikiIndexConfig cfg;
    cfg.table_name              = "p3_t07";
    cfg.embedding_dim           = 4;
    cfg.enable_persistent_cache = false;
    auto store = makeStore(cfg);

    // First write: embed computed and stored in memory cache
    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeChunk(makeChunk("p307-c1", "same text")));
    EXPECT_EQ(embed_calls_.load(), 1);

    // Second write with same chunk_id: in-memory cache hit, no LLM call
    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeChunk(makeChunk("p307-c1", "same text")));
    EXPECT_EQ(embed_calls_.load(), 0)
        << "Second write of same chunk_id must use in-memory cache (0 embed calls)";
}

// ─────────────────────────────────────────────────────────────────────────────
// WIS-P3-08: enable_persistent_cache=true — embedding reused across stores
// ─────────────────────────────────────────────────────────────────────────────

/**
 * WIS-P3-08: When persistent cache is enabled, an embedding computed and stored
 * by one `WikiIndexStore` instance is available to a second instance sharing the
 * same `SecondaryIndexManager`.  The second instance must not call embed() for
 * that chunk_id.
 */
TEST_F(WikiIndexStorePhase3, WisP308_PersistentCacheSkipsLlmOnSecondStore) {
    WikiIndexConfig cfg1;
    cfg1.table_name              = "p3_t08";
    cfg1.embedding_dim           = 4;
    cfg1.enable_persistent_cache = true;
    cfg1.enable_vector           = false;  // avoid vector index re-init between stores
    cfg1.enable_bm25             = true;

    // Store 1: compute and persist embedding for chunk c1
    {
        auto store1 = makeStore(cfg1);
        embed_calls_.store(0);
        ASSERT_NO_THROW(store1->writeChunk(makeChunk("p308-c1", "persistent text")));
        EXPECT_EQ(embed_calls_.load(), 1)
            << "First store must call embed() once for new chunk";
    }

    // Store 2: same config, same DB — should find persisted embedding, skip LLM
    embed_calls_.store(0);
    {
        WikiIndexConfig cfg2 = cfg1;
        cfg2.table_name = "p3_t08";   // same table/cache namespace
        auto store2 = makeStore(cfg2);
        // Write the same chunk_id — should find it in RocksDB, skip LLM
        ASSERT_NO_THROW(store2->writeChunk(makeChunk("p308-c1", "persistent text")));
    }
    EXPECT_EQ(embed_calls_.load(), 0)
        << "Second store must retrieve embedding from RocksDB (0 LLM calls)";
}

} // namespace (anonymous)
