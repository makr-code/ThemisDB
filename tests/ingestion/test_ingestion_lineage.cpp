/**
 * @file test_ingestion_lineage.cpp
 * @brief Unit tests for end-to-end ingestion lineage tracking (Issue #1901)
 *
 * Validates IngestionLineageRecord, IngestionLineageStore, and the
 * IngestionManager lineage API (enableLineageTracking, getLineageRecords,
 * getLineageRecordsByRun, getAllLineageRecords, clearLineageRecords).
 */

#include <gtest/gtest.h>
#include "ingestion/ingestion_manager.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::ingestion;

// ============================================================================
// IngestionLineageRecord – struct defaults and field assignment
// ============================================================================

TEST(IngestionLineageRecordTest, DefaultValues) {
    IngestionLineageRecord r;
    EXPECT_TRUE(r.run_correlation_id.empty());
    EXPECT_TRUE(r.source_id.empty());
    EXPECT_TRUE(r.connector_type.empty());
    EXPECT_TRUE(r.connector_version.empty());
    EXPECT_TRUE(r.doc_id.empty());
    EXPECT_TRUE(r.ingested_at.empty());
    EXPECT_EQ(r.bytes, 0u);
    EXPECT_EQ(r.doc_count, 0u);
    EXPECT_TRUE(r.transformation_steps.empty());
    EXPECT_EQ(r.status, LineageStatus::SUCCESS);
}

TEST(IngestionLineageRecordTest, FieldAssignment) {
    IngestionLineageRecord r;
    r.run_correlation_id  = "corr-123";
    r.source_id           = "src_a";
    r.connector_type      = "FILESYSTEM";
    r.connector_version   = "1.0.0";
    r.doc_id              = "batch:42";
    r.ingested_at         = "2026-03-10T18:00:00Z";
    r.bytes               = 4096;
    r.doc_count           = 42;
    r.transformation_steps = {"mime_detection", "schema_validation"};
    r.status              = LineageStatus::SUCCESS;

    EXPECT_EQ(r.run_correlation_id, "corr-123");
    EXPECT_EQ(r.source_id, "src_a");
    EXPECT_EQ(r.connector_type, "FILESYSTEM");
    EXPECT_EQ(r.connector_version, "1.0.0");
    EXPECT_EQ(r.doc_id, "batch:42");
    EXPECT_EQ(r.bytes, 4096u);
    EXPECT_EQ(r.doc_count, 42u);
    ASSERT_EQ(r.transformation_steps.size(), 2u);
    EXPECT_EQ(r.transformation_steps[0], "mime_detection");
    EXPECT_EQ(r.transformation_steps[1], "schema_validation");
    EXPECT_EQ(r.status, LineageStatus::SUCCESS);
}

TEST(IngestionLineageRecordTest, AllLineageStatuses) {
    EXPECT_EQ(static_cast<int>(LineageStatus::SUCCESS),     0);
    EXPECT_EQ(static_cast<int>(LineageStatus::FAILED),      1);
    EXPECT_EQ(static_cast<int>(LineageStatus::QUARANTINED), 2);
    EXPECT_EQ(static_cast<int>(LineageStatus::DRY_RUN),     3);

    IngestionLineageRecord r;
    r.status = LineageStatus::QUARANTINED;
    EXPECT_EQ(r.status, LineageStatus::QUARANTINED);
    r.status = LineageStatus::FAILED;
    EXPECT_EQ(r.status, LineageStatus::FAILED);
    r.status = LineageStatus::DRY_RUN;
    EXPECT_EQ(r.status, LineageStatus::DRY_RUN);
}

// ============================================================================
// IngestionLineageStore – standalone unit tests
// ============================================================================

TEST(IngestionLineageStoreTest, EmptyByDefault) {
    IngestionLineageStore store;
    EXPECT_EQ(store.size(), 0u);
    EXPECT_TRUE(store.getAll().empty());
    EXPECT_TRUE(store.getBySource("any").empty());
    EXPECT_TRUE(store.getByCorrelationId("any").empty());
}

TEST(IngestionLineageStoreTest, RecordAndRetrieve) {
    IngestionLineageStore store;

    IngestionLineageRecord r;
    r.source_id           = "src_a";
    r.run_correlation_id  = "run-1";
    r.connector_type      = "FILESYSTEM";
    r.doc_id              = "batch:10";
    r.doc_count           = 10;
    r.status              = LineageStatus::SUCCESS;
    store.record(r);

    EXPECT_EQ(store.size(), 1u);
    auto all = store.getAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].source_id, "src_a");
    EXPECT_EQ(all[0].doc_count, 10u);
}

TEST(IngestionLineageStoreTest, GetBySource) {
    IngestionLineageStore store;

    for (const char* source_name : {"alpha", "beta", "alpha"}) {
        IngestionLineageRecord r;
        r.source_id = source_name;
        r.doc_count = 1;
        store.record(r);
    }

    EXPECT_EQ(store.size(), 3u);
    EXPECT_EQ(store.getBySource("alpha").size(), 2u);
    EXPECT_EQ(store.getBySource("beta").size(), 1u);
    EXPECT_EQ(store.getBySource("gamma").size(), 0u);
}

TEST(IngestionLineageStoreTest, GetByCorrelationId) {
    IngestionLineageStore store;

    for (int i = 0; i < 3; ++i) {
        IngestionLineageRecord r;
        r.run_correlation_id = (i < 2) ? "run-A" : "run-B";
        r.source_id          = "s" + std::to_string(i);
        store.record(r);
    }

    EXPECT_EQ(store.getByCorrelationId("run-A").size(), 2u);
    EXPECT_EQ(store.getByCorrelationId("run-B").size(), 1u);
    EXPECT_EQ(store.getByCorrelationId("run-C").size(), 0u);
}

TEST(IngestionLineageStoreTest, Clear) {
    IngestionLineageStore store;
    IngestionLineageRecord r;
    r.source_id = "x";
    store.record(r);
    store.record(r);
    EXPECT_EQ(store.size(), 2u);
    store.clear();
    EXPECT_EQ(store.size(), 0u);
    EXPECT_TRUE(store.getAll().empty());
}

TEST(IngestionLineageStoreTest, ThreadSafeRecord) {
    IngestionLineageStore store;
    const int kThreads = 4;
    const int kPerThread = 25;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&store, t, kPerThread]() {
            for (int i = 0; i < kPerThread; ++i) {
                IngestionLineageRecord r;
                r.source_id = "src_" + std::to_string(t);
                r.doc_count = 1;
                store.record(r);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(store.size(), static_cast<size_t>(kThreads * kPerThread));
}

// ============================================================================
// IngestionManager – lineage tracking API
// ============================================================================

TEST(IngestionManagerLineageTest, DisabledByDefault) {
    IngestionManager mgr("test_db");
    EXPECT_FALSE(mgr.isLineageTrackingEnabled());
}

TEST(IngestionManagerLineageTest, EnableDisableToggle) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);
    EXPECT_TRUE(mgr.isLineageTrackingEnabled());
    mgr.enableLineageTracking(false);
    EXPECT_FALSE(mgr.isLineageTrackingEnabled());
}

TEST(IngestionManagerLineageTest, NoRecordsWhenDisabled) {
    IngestionManager mgr("test_db");
    // lineage tracking is off by default

    SourceConfig cfg;
    cfg.source_id = "fs_src";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_lineage_test_dir";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    mgr.setDryRun(true);
    mgr.ingestAll();

    EXPECT_TRUE(mgr.getAllLineageRecords().empty());
}

TEST(IngestionManagerLineageTest, DryRunRecordsLineage) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);

    SourceConfig cfg;
    cfg.source_id = "fs_dry";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_lineage_test_dir";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    mgr.setDryRun(true);
    auto report = mgr.ingestAll();
    (void)report;

    // At least one lineage record should be emitted per source
    auto recs = mgr.getLineageRecords("fs_dry");
    ASSERT_FALSE(recs.empty());
    // Dry-run records must have DRY_RUN status
    for (const auto& r : recs) {
        EXPECT_EQ(r.status, LineageStatus::DRY_RUN);
        EXPECT_EQ(r.source_id, "fs_dry");
        EXPECT_EQ(r.connector_type, "FILESYSTEM");
        EXPECT_FALSE(r.run_correlation_id.empty());
        EXPECT_FALSE(r.ingested_at.empty());
    }
}

TEST(IngestionManagerLineageTest, GetLineageRecordsByRun) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);

    SourceConfig cfg;
    cfg.source_id = "src_run";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_lineage_run_dir";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    mgr.setDryRun(true);
    auto stats = mgr.ingestSource("src_run");

    EXPECT_FALSE(stats.correlation_id.empty());
    auto recs = mgr.getLineageRecordsByRun(stats.correlation_id);
    // Every record from this run must carry the same correlation_id
    for (const auto& r : recs) {
        EXPECT_EQ(r.run_correlation_id, stats.correlation_id);
    }
}

TEST(IngestionManagerLineageTest, ClearLineageRecords) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);

    SourceConfig cfg;
    cfg.source_id = "src_clear";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_lineage_clear_dir";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    mgr.setDryRun(true);
    mgr.ingestAll();
    EXPECT_FALSE(mgr.getAllLineageRecords().empty());

    mgr.clearLineageRecords();
    EXPECT_TRUE(mgr.getAllLineageRecords().empty());
}

TEST(IngestionManagerLineageTest, GetLineageRecordsForUnknownSourceEmpty) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);
    EXPECT_TRUE(mgr.getLineageRecords("no_such_source").empty());
}

TEST(IngestionManagerLineageTest, LineageRecordHasTransformationSteps) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);

    SourceConfig cfg;
    cfg.source_id = "src_steps";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_lineage_steps_dir";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    // Register a schema so schema_validation step appears
    SchemaConfig sc;
    sc.min_content_length = 1;
    mgr.setSchemaConfig("src_steps", sc);

    mgr.setDryRun(true);
    mgr.ingestAll();

    auto recs = mgr.getLineageRecords("src_steps");
    ASSERT_FALSE(recs.empty());
    const auto& r = recs.front();
    // FILESYSTEM connector adds mime_detection step
    bool found_mime = false;
    for (const auto& step : r.transformation_steps) {
        if (step == "mime_detection") found_mime = true;
    }
    EXPECT_TRUE(found_mime) << "Expected mime_detection step for FILESYSTEM connector";
}

TEST(IngestionManagerLineageTest, MultipleSourcesProduceSeparateRecords) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);
    mgr.setDryRun(true);

    for (const char* sid : {"src_m1", "src_m2", "src_m3"}) {
        SourceConfig cfg;
        cfg.source_id = sid;
        cfg.type      = SourceType::FILESYSTEM;
        cfg.location  = "/tmp/nonexistent_dir_" + std::string(sid);
        cfg.enabled   = true;
        mgr.registerSource(cfg);
    }

    mgr.ingestAll();

    for (const char* sid : {"src_m1", "src_m2", "src_m3"}) {
        EXPECT_FALSE(mgr.getLineageRecords(sid).empty())
            << "Expected lineage records for " << sid;
    }
}

TEST(IngestionManagerLineageTest, SourceIdInLineageRecord) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);

    SourceConfig cfg;
    cfg.source_id = "unique_src_42";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_unique_42";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    mgr.setDryRun(true);
    mgr.ingestAll();

    auto recs = mgr.getAllLineageRecords();
    ASSERT_FALSE(recs.empty());
    for (const auto& r : recs) {
        if (r.source_id == "unique_src_42") {
            EXPECT_EQ(r.connector_type, "FILESYSTEM");
            return;
        }
    }
    FAIL() << "No lineage record with source_id == unique_src_42";
}

TEST(IngestionManagerLineageTest, ConnectorVersionNotEmpty) {
    IngestionManager mgr("test_db");
    mgr.enableLineageTracking(true);

    SourceConfig cfg;
    cfg.source_id = "src_ver";
    cfg.type      = SourceType::FILESYSTEM;
    cfg.location  = "/tmp/nonexistent_ver";
    cfg.enabled   = true;
    mgr.registerSource(cfg);

    mgr.setDryRun(true);
    mgr.ingestAll();

    auto recs = mgr.getLineageRecords("src_ver");
    ASSERT_FALSE(recs.empty());
    EXPECT_FALSE(recs.front().connector_version.empty());
}
