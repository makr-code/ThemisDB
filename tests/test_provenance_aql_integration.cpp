// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_provenance_aql_integration.cpp
 * @brief Integration tests for ProvenanceTracker AQL live-connection path
 *
 * Covers the acceptance criteria from FUTURE_ENHANCEMENTS.md:
 *   "Add integration test for provenance lineage round-trip: create training
 *    sample, verify AQL provenance query returns correct lineage."
 *
 * Tests:
 *  - Offline (engine=nullptr): write + queryLineage round-trip via in-process store
 *  - With QueryEngine: write() persists to in-process store and calls AQL path
 *  - queryLineage() falls back to in-process store when AQL returns empty
 *  - getRecord() returns the persisted provenance record
 *  - Multiple records: lineage includes all written samples
 *  - reject_without_urn rejects records without source URN regardless of engine
 */

#include <gtest/gtest.h>

#include "training/provenance_tracker.h"
#include "query/aql_runner.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"

#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

using namespace themis;
using namespace themis::training;

// ============================================================================
// Helpers
// ============================================================================

namespace {

std::string makeTmpPath(const std::string& suffix) {
    namespace fs = std::filesystem;
    auto ts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_prov_" + suffix + std::to_string(ts))).string();
}

ProvenanceTrackerConfig defaultConfig() {
    ProvenanceTrackerConfig cfg;
    cfg.graph_collection   = "TrainingSamples";
    cfg.edge_collection    = "DerivedFrom";
    cfg.batch_write_size   = 10;
    cfg.reject_without_urn = true;
    cfg.emit_audit_events  = true;
    return cfg;
}

ProvenanceRecord makeRecord(const std::string& id,
                            const std::string& urn = "urn:document:legal-001") {
    ProvenanceRecord rec;
    rec.sample_id              = id;
    rec.source_doc_urn         = urn;
    rec.extraction_timestamp   = 1700000000;
    rec.labeler_version        = "auto_labeler-abc123";
    rec.modality               = "text";
    rec.enrichment_query_fingerprints = {"aql_fp_01"};
    return rec;
}

} // namespace

// ============================================================================
// Offline mode (engine = nullptr) – round-trip via in-process store
// ============================================================================

class ProvenanceOfflineRoundTripTest : public ::testing::Test {
protected:
    ProvenanceTrackerConfig cfg_ = defaultConfig();
    const std::string db_conn_   = "";
};

TEST_F(ProvenanceOfflineRoundTripTest, WriteAndQueryLineage_RoundTrip) {
    ProvenanceTracker tracker(cfg_, db_conn_, nullptr);

    // Write two provenance records
    auto stats = tracker.write({makeRecord("s001"), makeRecord("s002")});
    EXPECT_EQ(stats.records_written, 2u);
    EXPECT_EQ(stats.records_rejected, 0u);

    // Lineage should contain both samples
    auto lineage = tracker.queryLineage("model_legal_v1");
    EXPECT_EQ(lineage.node_type, "model");
    EXPECT_EQ(lineage.node_id,   "model_legal_v1");
    EXPECT_GE(lineage.parents.size(), 2u);

    // Each sample node should carry a document parent
    for (const auto& sample_node : lineage.parents) {
        EXPECT_EQ(sample_node.node_type, "sample");
        ASSERT_FALSE(sample_node.parents.empty());
        EXPECT_EQ(sample_node.parents[0].node_type, "document");
        EXPECT_EQ(sample_node.parents[0].node_id,   "urn:document:legal-001");
    }
}

TEST_F(ProvenanceOfflineRoundTripTest, GetRecord_ReturnsPersistedRecord) {
    ProvenanceTracker tracker(cfg_, db_conn_, nullptr);
    tracker.write({makeRecord("s_get_001")});

    auto rec = tracker.getRecord("s_get_001");
    EXPECT_EQ(rec.sample_id,       "s_get_001");
    EXPECT_EQ(rec.source_doc_urn,  "urn:document:legal-001");
    EXPECT_EQ(rec.modality,        "text");
    EXPECT_EQ(rec.labeler_version, "auto_labeler-abc123");
}

TEST_F(ProvenanceOfflineRoundTripTest, GetRecord_Unknown_ReturnsEmpty) {
    ProvenanceTracker tracker(cfg_, db_conn_, nullptr);
    auto rec = tracker.getRecord("does_not_exist");
    EXPECT_TRUE(rec.sample_id.empty());
}

TEST_F(ProvenanceOfflineRoundTripTest, QueryLineage_EmptyStore_NoParents) {
    ProvenanceTracker tracker(cfg_, db_conn_, nullptr);
    auto lineage = tracker.queryLineage("model_v2");
    EXPECT_TRUE(lineage.parents.empty());
}

TEST_F(ProvenanceOfflineRoundTripTest, Reject_MissingUrn_WhenConfigured) {
    ProvenanceTracker tracker(cfg_, db_conn_, nullptr);
    auto stats = tracker.write({makeRecord("no_urn", "")});
    EXPECT_EQ(stats.records_written,  0u);
    EXPECT_EQ(stats.records_rejected, 1u);
}

TEST_F(ProvenanceOfflineRoundTripTest, Accept_MissingUrn_WhenNotConfigured) {
    cfg_.reject_without_urn = false;
    ProvenanceTracker tracker(cfg_, db_conn_, nullptr);
    auto stats = tracker.write({makeRecord("no_urn", "")});
    EXPECT_EQ(stats.records_written, 1u);
}

// ============================================================================
// DB-backed round-trip – QueryEngine wired (AQL execution path)
// ============================================================================

class ProvenanceAqlRoundTripTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = makeTmpPath("prov_");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = dbPath_;
        cfg.enable_blobdb = false;
        db_  = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        engine_ = std::make_unique<themis::query::QueryEngine>(*db_, *idx_);
    }

    void TearDown() override {
        engine_.reset();
        idx_.reset();
        if (db_) db_->close();
        db_.reset();
        std::filesystem::remove_all(dbPath_);
    }

    std::string dbPath_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<themis::query::QueryEngine> engine_;
};

TEST_F(ProvenanceAqlRoundTripTest, Write_WithEngine_WritesAndMaintainsInProcessStore) {
    auto cfg = defaultConfig();
    ProvenanceTracker tracker(cfg, "", engine_.get());

    auto stats = tracker.write({makeRecord("e_s001"), makeRecord("e_s002")});
    EXPECT_EQ(stats.records_written, 2u);
    EXPECT_EQ(stats.records_rejected, 0u);

    // In-process store must be populated (lineage falls back to it when AQL
    // traversal returns empty for a flat RocksDB without graph edges)
    auto lineage = tracker.queryLineage("model_v1_engine");
    EXPECT_EQ(lineage.node_type, "model");
    EXPECT_GE(lineage.parents.size(), 2u);
}

TEST_F(ProvenanceAqlRoundTripTest, GetRecord_WithEngine_ReturnsRecordFromStore) {
    auto cfg = defaultConfig();
    ProvenanceTracker tracker(cfg, "", engine_.get());
    tracker.write({makeRecord("e_get_001")});

    auto rec = tracker.getRecord("e_get_001");
    EXPECT_EQ(rec.sample_id, "e_get_001");
    EXPECT_FALSE(rec.source_doc_urn.empty());
}

TEST_F(ProvenanceAqlRoundTripTest, Write_WithEngine_RejectsRecordWithMissingSampleId) {
    auto cfg = defaultConfig();
    ProvenanceTracker tracker(cfg, "", engine_.get());

    ProvenanceRecord bad;
    bad.sample_id      = "";   // missing
    bad.source_doc_urn = "urn:x:1";
    auto stats = tracker.write({bad});
    EXPECT_EQ(stats.records_rejected, 1u);
}

TEST_F(ProvenanceAqlRoundTripTest, Write_LargeBatch_AllRecordsWritten) {
    auto cfg = defaultConfig();
    cfg.batch_write_size = 5;
    ProvenanceTracker tracker(cfg, "", engine_.get());

    std::vector<ProvenanceRecord> records;
    for (int i = 0; i < 13; ++i) {
        records.push_back(makeRecord("batch_" + std::to_string(i)));
    }

    auto stats = tracker.write(records);
    EXPECT_EQ(stats.records_written, 13u);
    EXPECT_EQ(stats.records_rejected, 0u);

    // All records should be queryable via lineage
    auto lineage = tracker.queryLineage("model_batch");
    EXPECT_GE(lineage.parents.size(), 13u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PT-05: setQueryEngine() — late injection wires engine post-construction
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ProvenanceAqlRoundTripTest, SetQueryEngine_LateInjection_Works) {
    auto cfg = defaultConfig();
    // Construct without engine (offline mode)
    ProvenanceTracker tracker(cfg, "");

    // Write two records offline (in-process store only)
    auto stats = tracker.write({makeRecord("late_s001"), makeRecord("late_s002")});
    EXPECT_EQ(stats.records_written, 2u);

    // Late-inject the engine
    tracker.setQueryEngine(engine_.get());

    // Write another record; should now also attempt AQL INSERT
    auto stats2 = tracker.write({makeRecord("late_s003")});
    EXPECT_EQ(stats2.records_written, 1u);

    // In-process lineage must include all three records (engine fallback)
    auto lineage = tracker.queryLineage("model_late");
    EXPECT_GE(lineage.parents.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// PT-06: setQueryEngine(nullptr) — reverts to offline mode
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(ProvenanceAqlRoundTripTest, SetQueryEngine_NullReverts_OfflineMode) {
    auto cfg = defaultConfig();
    ProvenanceTracker tracker(cfg, "", engine_.get());

    // Write one record with the engine wired
    tracker.write({makeRecord("revert_s001")});

    // Revert to offline mode
    tracker.setQueryEngine(nullptr);

    // Write another record; must succeed against in-process store only
    auto stats = tracker.write({makeRecord("revert_s002")});
    EXPECT_EQ(stats.records_written, 1u);

    // Both records accessible via lineage (in-process store)
    auto lineage = tracker.queryLineage("model_revert");
    EXPECT_GE(lineage.parents.size(), 2u);
}
