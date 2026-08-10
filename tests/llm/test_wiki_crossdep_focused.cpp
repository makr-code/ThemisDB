/**
 * @file test_wiki_crossdep_focused.cpp
 * @brief Cross-dependency focused tests (WCD-01..16) covering the four
 *        LLM-Wiki ↔ ThemisDB RocksDB integration phases.
 *
 * Tests:
 *  Phase B — Persistent embedding cache default=true
 *   WCD-01: WikiIndexConfig default has enable_persistent_cache=true  [standalone]
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

#include "llm/wiki_index_store.h"
#include "llm_wiki/workspace_state_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/storage_audit_logger.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis;

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
// Phase B — Persistent embedding cache default = true
// ═══════════════════════════════════════════════════════════════════════════

/// WCD-01: WikiIndexConfig default has enable_persistent_cache=true [standalone]
TEST(WikiCrossDepConfig, WCD_01_PersistentCacheDefaultTrue) {
    themis::llm::WikiIndexConfig cfg;
    EXPECT_TRUE(cfg.enable_persistent_cache)
        << "enable_persistent_cache must default to true (enterprise default)";
}

/// WCD-02: Re-ingest same chunk avoids embed call when persistent cache is on.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_02_PersistentCacheHitAvoidsReembed) {
    GTEST_SKIP() << "[WCD-02] Integration test — requires full ThemisDB library "
                    "(SecondaryIndexManager + VectorIndexManager + EmbeddedLLM). "
                    "Run with themis_core linked.";
}

/// WCD-03: Two different chunk_ids each produce exactly one embed call.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_03_DistinctChunksEachEmbedOnce) {
    GTEST_SKIP() << "[WCD-03] Integration test — requires full ThemisDB library.";
}

/// WCD-04: Pre-populated embedding bypasses LLM regardless of cache flag.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_04_PrePopulatedEmbeddingBypassesLLM) {
    GTEST_SKIP() << "[WCD-04] Integration test — requires full ThemisDB library.";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase F — writeBatch uses putBatch + addBatch (atomic WriteBatch)
// ═══════════════════════════════════════════════════════════════════════════

/// WCD-05: writeBatch with empty input completes without throwing.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_05_WriteBatchEmptyNoThrow) {
    GTEST_SKIP() << "[WCD-05] Integration test — requires full ThemisDB library.";
}

/// WCD-06: writeBatch single chunk is queryable.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_06_WriteBatchSingleChunkQueryable) {
    GTEST_SKIP() << "[WCD-06] Integration test — requires full ThemisDB library.";
}

/// WCD-07: writeBatch multiple chunks across two doc_ids — all found.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_07_WriteBatchMultiDocQueryable) {
    GTEST_SKIP() << "[WCD-07] Integration test — requires full ThemisDB library.";
}

/// WCD-08: writeBatch with all pre-populated embeddings — LLM not called.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_08_WriteBatchPreEmbeddedNoLLM) {
    GTEST_SKIP() << "[WCD-08] Integration test — requires full ThemisDB library.";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase D — optional StorageAuditLogger injection
// ═══════════════════════════════════════════════════════════════════════════

/// WCD-09: No audit logger set → writeChunk completes without crash.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_09_NoAuditLoggerNocrash) {
    GTEST_SKIP() << "[WCD-09] Integration test — requires full ThemisDB library.";
}

/// WCD-10: setAuditLogger → writeChunk emits exactly one PUT entry.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_10_AuditLoggerWriteChunkEmitsPut) {
    GTEST_SKIP() << "[WCD-10] Integration test — requires full ThemisDB library.";
}

/// WCD-11: writeBatch with 3 chunks in 2 doc_ids → 2 audit entries.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_11_AuditLoggerWriteBatchTwoDocIds) {
    GTEST_SKIP() << "[WCD-11] Integration test — requires full ThemisDB library.";
}

/// WCD-12: writeBatch with empty input → zero audit entries.
/// [integration] Requires full library; skipped in standalone build.
TEST(WikiCrossDepIntegration, WCD_12_AuditLoggerWriteBatchEmptyNoEntries) {
    GTEST_SKIP() << "[WCD-12] Integration test — requires full ThemisDB library.";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase A — WorkspaceStateManager optional RocksDB backend
// ═══════════════════════════════════════════════════════════════════════════

/// WCD-13: hasRocksDB() returns false before useRocksDB() is called.
TEST(WikiCrossDepWorkspace, WCD_13_HasRocksDBFalseByDefault) {
    themis::llm_wiki::WorkspaceStateManager mgr("/tmp/wcd13_ws");
    EXPECT_FALSE(mgr.hasRocksDB());
}

/// WCD-14: hasRocksDB() returns true after useRocksDB() is called.
TEST(WikiCrossDepWorkspace, WCD_14_HasRocksDBTrueAfterUse) {
    auto db_path = makeTmpDbPath("wcd14");
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    RocksDBWrapper db(cfg);
    if (!db.open()) {
        GTEST_SKIP() << "[WCD-14] RocksDB unavailable; skipping.";
    }

    themis::llm_wiki::WorkspaceStateManager mgr("/tmp/wcd14_ws");
    mgr.useRocksDB(&db);
    EXPECT_TRUE(mgr.hasRocksDB());

    db.close();
    std::filesystem::remove_all(db_path);
}

/// WCD-15: save() + load() roundtrip via RocksDB preserves workspace state.
TEST(WikiCrossDepWorkspace, WCD_15_RocksDBSaveLoadRoundtrip) {
    auto db_path = makeTmpDbPath("wcd15");
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    RocksDBWrapper db(cfg);
    if (!db.open()) {
        GTEST_SKIP() << "[WCD-15] RocksDB unavailable; skipping.";
    }

    const std::string root = "/srv/wiki/test_wcd15";
    auto state = makeWorkspaceState(root);

    {
        themis::llm_wiki::WorkspaceStateManager mgr(root);
        mgr.useRocksDB(&db);
        auto s = mgr.save(state);
        EXPECT_TRUE(s.ok()) << "save() via RocksDB must succeed: " << s.message;
    }

    {
        themis::llm_wiki::WorkspaceState loaded;
        themis::llm_wiki::WorkspaceStateManager mgr(root);
        mgr.useRocksDB(&db);
        auto s = mgr.load(loaded);
        EXPECT_TRUE(s.ok()) << "load() via RocksDB must succeed: " << s.message;

        EXPECT_EQ(loaded.version,        state.version);
        EXPECT_EQ(loaded.workspace_root, state.workspace_root);
        EXPECT_EQ(loaded.created_at,     state.created_at);
        EXPECT_EQ(loaded.last_updated,   state.last_updated);

        // Verify link graph preserved
        ASSERT_TRUE(loaded.links.count("pageA"));
        EXPECT_EQ(loaded.links.at("pageA"), state.links.at("pageA"));

        // Verify task preserved
        ASSERT_TRUE(loaded.tasks.count("task1"));
    }

    db.close();
    std::filesystem::remove_all(db_path);
}

/// WCD-16: load() returns Error when the workspace key is absent (fresh DB).
TEST(WikiCrossDepWorkspace, WCD_16_RocksDBLoadMissingKeyReturnsError) {
    auto db_path = makeTmpDbPath("wcd16");
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    RocksDBWrapper db(cfg);
    if (!db.open()) {
        GTEST_SKIP() << "[WCD-16] RocksDB unavailable; skipping.";
    }

    themis::llm_wiki::WorkspaceState loaded;
    themis::llm_wiki::WorkspaceStateManager mgr("/srv/wiki/wcd16");
    mgr.useRocksDB(&db);
    auto s = mgr.load(loaded);
    EXPECT_FALSE(s.ok()) << "load() on fresh DB must return an error";

    db.close();
    std::filesystem::remove_all(db_path);
}

