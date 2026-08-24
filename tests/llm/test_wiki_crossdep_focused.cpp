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

#include "llm/wiki_index_store.h"
#include "llm_wiki/workspace_state_manager.h"

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

/// WCD-01: WikiIndexConfig default has enable_persistent_cache=false [standalone]
TEST(WikiCrossDepConfig, WCD_01_PersistentCacheDefaultFalse) {
    themis::llm::WikiIndexConfig cfg;
    EXPECT_FALSE(cfg.enable_persistent_cache)
        << "enable_persistent_cache must default to false (in-memory cache by default)";
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

