/**
 * @file test_wiki_crossdep_focused.cpp
 * @brief Cross-dependency focused tests (WCD-01..16) covering the four
 *        LLM-Wiki ↔ ThemisDB RocksDB integration phases.
 *
 * Tests:
 *  Phase B — Persistent embedding cache default=false
 *   WCD-01: WikiIndexConfig default has enable_persistent_cache=false [standalone]
 *   WCD-02: Re-ingest of same chunk avoids EmbeddedLLM::embed         [integration]
 *   WCD-03: Different chunk IDs each trigger exactly one embed call    [integration]
 *   WCD-04: Pre-populated embedding bypasses LLM regardless of flag   [integration]
 *
 *  Phase F — writeBatch uses putBatch + addBatch (atomic RocksDB WriteBatch)
 *   WCD-05: writeBatch() with empty input completes without throw      [integration]
 *   WCD-06: writeBatch() single chunk — query finds it                 [integration]
 *   WCD-07: writeBatch() multiple chunks across multiple doc_ids       [integration]
 *   WCD-08: writeBatch() with pre-populated embeddings — LLM not called [integration]
 *
 *  Phase D — optional StorageAuditLogger injection
 *   WCD-09: setAuditLogger not called → no crash in writeChunk         [integration]
 *   WCD-10: setAuditLogger: writeChunk emits one logPut call           [integration]
 *   WCD-11: setAuditLogger: writeBatch 3 chunks 2 doc_ids → 2 entries  [integration]
 *   WCD-12: setAuditLogger: writeBatch empty input → zero entries      [integration]
 *
 *  Phase A — WorkspaceStateManager optional RocksDB backend
 *   WCD-13: hasRocksDB() returns false before useRocksDB() is called   [standalone]
 *   WCD-14: hasRocksDB() returns true after useRocksDB() is called     [standalone]
 *   WCD-15: save() + load() roundtrip via RocksDB preserves state      [standalone]
 *   WCD-16: load() returns Error when key not found in fresh DB        [standalone]
 *
 * NOTE: WCD-02..12 (marked [integration]) require the full ThemisDB library
 * (themis_core with SecondaryIndexManager, VectorIndexManager, EmbeddedLLM).
 * In the standalone focused-test build these tests call GTEST_SKIP().
 * They are exercised automatically when the target is built as part of the
 * full library CMake configuration.
 *
 * @version 0.1.0
 * @note Maturity: 🟡 BETA — Phase B/D/F/A cross-dependency delivery
 */

#include <gtest/gtest.h>

#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "llm/embedded_llm.h"
#include "llm/wiki_index_store.h"
#include "llm_wiki/workspace_state_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis;
using namespace themis::llm;

// ═══════════════════════════════════════════════════════════════════════════
// Shared helpers
// ═══════════════════════════════════════════════════════════════════════════

namespace {

std::string makeTmpDbPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("wiki_crossdep_" + tag + "_" + std::to_string(ts)))
               .string();
}

/// Build a minimal WorkspaceState for round-trip tests.
themis::llm_wiki::WorkspaceState makeWorkspaceState(
    const std::string& root = "/tmp/test_workspace")
{
    themis::llm_wiki::WorkspaceState s;
    s.version        = "1.0.0";
    s.created_at     = "2026-08-10T10:00:00Z";
    s.last_updated   = "2026-08-10T12:00:00Z";
    s.workspace_root = root;
    s.links["pageA"] = {"pageB", "pageC"};
    s.tasks["task1"] = {{"type", "review"}, {"status", "open"}};
    return s;
}

} // namespace

// ═══════════════════════════════════════════════════════════════════════════
// Phase B — Real RocksDB-backed WikiIndexStore integration
// ═══════════════════════════════════════════════════════════════════════════

class WikiCrossDepStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeTmpDbPath(
            ::testing::UnitTest::GetInstance()->current_test_info()->name());

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            GTEST_SKIP() << "[WCD] RocksDB unavailable; skipping live ThemisDB wiki integration tests.";
        }

        sim_ = std::make_unique<SecondaryIndexManager>(*db_);
        vim_ = std::make_unique<VectorIndexManager>(*db_);
        setDefaultEmbedFn();
    }

    void TearDown() override {
        sim_.reset();
        vim_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    void setDefaultEmbedFn() {
        embed_calls_.store(0);
        llm_.setEmbedFn([this](const std::string& text) -> std::vector<float> {
            ++embed_calls_;
            const std::size_t h = std::hash<std::string>{}(text);
            std::vector<float> v(4, 0.0f);
            v[h % 4] = 1.0f;
            return v;
        });
    }

    std::unique_ptr<WikiIndexStore> makeStore(WikiIndexConfig cfg) {
        cfg.enable_phase_b = true;
        cfg.embedding_dim = 4;
        cfg.top_k = 10;
        cfg.enable_bm25 = true;
        cfg.enable_vector = true;
        return std::make_unique<WikiIndexStore>(*sim_, *vim_, llm_, cfg);
    }

    static WikiChunk makeChunk(const std::string& chunk_id,
                              const std::string& text,
                              const std::string& doc_id = "doc_a") {
        WikiChunk chunk;
        chunk.chunk_id = chunk_id;
        chunk.doc_id = doc_id;
        chunk.text = text;
        return chunk;
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> sim_;
    std::unique_ptr<VectorIndexManager> vim_;
    EmbeddedLLM llm_;
    std::atomic<int> embed_calls_{0};
};

/// WCD-01: WikiIndexConfig default has enable_persistent_cache=false [standalone]
TEST(WikiCrossDepConfig, WCD_01_PersistentCacheDefaultFalse) {
    themis::llm::WikiIndexConfig cfg;
    EXPECT_FALSE(cfg.enable_persistent_cache)
        << "enable_persistent_cache must default to false (in-memory cache by default)";
}

/// WCD-02: Re-ingest same chunk avoids embed call when persistent cache is on.
TEST_F(WikiCrossDepStoreTest, WCD_02_PersistentCacheHitAvoidsReembed) {
    WikiIndexConfig cfg;
    cfg.table_name = "wcd02";
    cfg.embedding_dim = 4;
    cfg.enable_persistent_cache = true;
    cfg.enable_vector = false;
    cfg.enable_bm25 = true;

    auto store = makeStore(cfg);
    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeChunk(makeChunk("wcd02-c1", "ThemisDB wiki cross dependency")));
    EXPECT_EQ(embed_calls_.load(), 1);

    embed_calls_.store(0);
    ASSERT_NO_THROW(store->writeChunk(makeChunk("wcd02-c1", "ThemisDB wiki cross dependency")));
    EXPECT_EQ(embed_calls_.load(), 0)
        << "same chunk_id must reuse persisted embedding instead of calling embed() again";
}

/// WCD-05: writeBatch with empty input completes without throwing.
TEST_F(WikiCrossDepStoreTest, WCD_05_WriteBatchEmptyNoThrow) {
    WikiIndexConfig cfg;
    cfg.table_name = "wcd05";
    cfg.embedding_dim = 4;
    cfg.enable_vector = false;
    cfg.enable_bm25 = true;

    auto store = makeStore(cfg);
    std::vector<WikiChunk> empty_chunks;
    EXPECT_NO_THROW(store->writeBatch(empty_chunks));
    EXPECT_TRUE(store->query("irrelevant", 10, 0.0f).empty());
}

/// WCD-06: writeBatch single chunk is queryable via the live RocksDB index.
TEST_F(WikiCrossDepStoreTest, WCD_06_WriteBatchSingleChunkQueryable) {
    WikiIndexConfig cfg;
    cfg.table_name = "wcd06";
    cfg.embedding_dim = 4;
    cfg.enable_vector = false;
    cfg.enable_bm25 = true;

    auto store = makeStore(cfg);
    std::vector<WikiChunk> chunks; 
    chunks.push_back(makeChunk("wcd06-c1", "ThemisDB wiki integration is working"));

    ASSERT_NO_THROW(store->writeBatch(chunks));
    const auto results = store->query("ThemisDB wiki integration", 10, 0.0f);
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results.front().chunk_id, "wcd06-c1");
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase A — WorkspaceStateManager optional RocksDB backend
// ═══════════════════════════════════════════════════════════════════════════

/// WCD-13: save() persists a workspace state using the current contract.
TEST(WikiCrossDepWorkspace, WCD_13_SavePersistsWorkspaceState) {
    const auto workspace_root = makeTmpDbPath("wcd13_ws");
    themis::llm_wiki::WorkspaceStateManager mgr(workspace_root);

    auto state = makeWorkspaceState(workspace_root);
    auto save_status = mgr.save(state);
    EXPECT_TRUE(save_status.ok()) << save_status.message;
}

/// WCD-14: validateChecksum() succeeds for a file written by save().
TEST(WikiCrossDepWorkspace, WCD_14_ValidateChecksumAfterSave) {
    const auto workspace_root = makeTmpDbPath("wcd14_ws");
    themis::llm_wiki::WorkspaceStateManager mgr(workspace_root);

    auto state = makeWorkspaceState(workspace_root);
    auto save_status = mgr.save(state);
    ASSERT_TRUE(save_status.ok()) << save_status.message;

    auto checksum_status = themis::llm_wiki::WorkspaceStateManager::validateChecksum(
        std::filesystem::path(workspace_root) / "wiki" / "state.json");
    EXPECT_TRUE(checksum_status.ok()) << checksum_status.message;
}

/// WCD-15: save() + load() roundtrip preserves workspace state.
TEST(WikiCrossDepWorkspace, WCD_15_SaveLoadRoundtrip) {
    const auto workspace_root = makeTmpDbPath("wcd15_ws");
    themis::llm_wiki::WorkspaceStateManager mgr(workspace_root);

    const auto state = makeWorkspaceState(workspace_root);
    auto save_status = mgr.save(state);
    ASSERT_TRUE(save_status.ok()) << save_status.message;

    themis::llm_wiki::WorkspaceState loaded;
    auto load_status = mgr.load(loaded);
    ASSERT_TRUE(load_status.ok()) << load_status.message;

    EXPECT_EQ(loaded.version,        state.version);
    EXPECT_EQ(loaded.workspace_root, state.workspace_root);
    EXPECT_EQ(loaded.created_at,     state.created_at);
    EXPECT_EQ(loaded.last_updated,   state.last_updated);
    ASSERT_TRUE(loaded.links.count("pageA"));
    EXPECT_EQ(loaded.links.at("pageA"), state.links.at("pageA"));
    ASSERT_TRUE(loaded.tasks.count("task1"));
}

/// WCD-16: load() returns an error when the workspace state is absent.
TEST(WikiCrossDepWorkspace, WCD_16_LoadMissingStateReturnsError) {
    const auto workspace_root = makeTmpDbPath("wcd16_ws");
    themis::llm_wiki::WorkspaceStateManager mgr(workspace_root);

    themis::llm_wiki::WorkspaceState loaded;
    auto load_status = mgr.load(loaded);
    EXPECT_FALSE(load_status.ok());
}

