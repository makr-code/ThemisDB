// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_provenance_tracker.cpp
 * @brief Tests for ProvenanceTracker and ProvenanceRecord (Phase 3)
 *
 * Covers:
 *  - Construction with default and custom configurations
 *  - write() accepts valid ProvenanceRecords and returns correct stats
 *  - write() rejects records with missing source_doc_urn when configured
 *  - write() rejects records with missing sample_id
 *  - write() elapsed_seconds >= 0
 *  - recordFilteredSample() does not throw
 *  - queryLineage() returns a root LineageNode with correct model_id
 *  - queryLineage() includes samples that were previously written
 *  - getRecord() returns the stored provenance record
 *  - getRecord() returns empty record for unknown sample_id
 *  - ProvenanceRecord default values
 *  - ProvenanceTrackerConfig default values
 *  - ConfidenceCalibrator – addSample, calibrate, reset, sampleCount
 */

#include <gtest/gtest.h>
#include "training/provenance_tracker.h"
#include "training/training_pipeline.h"

#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Fixture
// ============================================================================
class ProvenanceTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.graph_collection   = "TrainingSamples";
        cfg_.edge_collection    = "DerivedFrom";
        cfg_.batch_write_size   = 10;
        cfg_.reject_without_urn = true;
        cfg_.emit_audit_events  = true;
    }

    ProvenanceTrackerConfig cfg_;
    const std::string db_conn_ = "";

    ProvenanceRecord makeRecord(const std::string& id,
                                const std::string& urn = "urn:document:legal-001") {
        ProvenanceRecord rec;
        rec.sample_id              = id;
        rec.source_doc_urn         = urn;
        rec.extraction_timestamp   = 1700000000;
        rec.labeler_version        = "auto_labeler-abc123";
        rec.modality               = "text";
        rec.enrichment_query_fingerprints = {"aql_fp_01", "aql_fp_02"};
        return rec;
    }
};

// ============================================================================
// Construction tests
// ============================================================================

TEST_F(ProvenanceTrackerTest, Construction_Succeeds) {
    EXPECT_NO_THROW(ProvenanceTracker tracker(cfg_, db_conn_));
}

TEST_F(ProvenanceTrackerTest, Construction_EmptyDbConnection_Succeeds) {
    EXPECT_NO_THROW(ProvenanceTracker tracker(cfg_, ""));
}

// ============================================================================
// write() tests
// ============================================================================

TEST_F(ProvenanceTrackerTest, Write_ValidRecords_ReturnsCorrectCount) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    std::vector<ProvenanceRecord> records = {
        makeRecord("s001"),
        makeRecord("s002"),
        makeRecord("s003"),
    };

    auto stats = tracker.write(records);
    EXPECT_EQ(stats.records_written, 3u);
    EXPECT_EQ(stats.records_rejected, 0u);
}

TEST_F(ProvenanceTrackerTest, Write_EmptyList_ReturnsZeroCounts) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    auto stats = tracker.write({});
    EXPECT_EQ(stats.records_written, 0u);
    EXPECT_EQ(stats.records_rejected, 0u);
}

TEST_F(ProvenanceTrackerTest, Write_MissingUrn_Rejected_WhenConfigured) {
    cfg_.reject_without_urn = true;
    ProvenanceTracker tracker(cfg_, db_conn_);

    ProvenanceRecord rec = makeRecord("s001", ""); // empty URN
    auto stats = tracker.write({rec});
    EXPECT_EQ(stats.records_written, 0u);
    EXPECT_EQ(stats.records_rejected, 1u);
}

TEST_F(ProvenanceTrackerTest, Write_MissingUrn_Accepted_WhenNotConfigured) {
    cfg_.reject_without_urn = false;
    ProvenanceTracker tracker(cfg_, db_conn_);

    ProvenanceRecord rec = makeRecord("s001", ""); // empty URN
    auto stats = tracker.write({rec});
    EXPECT_EQ(stats.records_written, 1u);
}

TEST_F(ProvenanceTrackerTest, Write_MissingSampleId_AlwaysRejected) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    ProvenanceRecord rec = makeRecord(""); // empty sample_id
    auto stats = tracker.write({rec});
    EXPECT_EQ(stats.records_rejected, 1u);
}

TEST_F(ProvenanceTrackerTest, Write_ElapsedSeconds_NonNegative) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    auto stats = tracker.write({makeRecord("s001")});
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(ProvenanceTrackerTest, Write_LargeBatch_WrittenInBatches) {
    cfg_.batch_write_size = 5;
    ProvenanceTracker tracker(cfg_, db_conn_);

    std::vector<ProvenanceRecord> records = {};

    for (int i = 0; i < 13; ++i) {
        records.push_back(makeRecord("s" + std::to_string(i)));
    }

    auto stats = tracker.write(records);
    EXPECT_EQ(stats.records_written, 13u);
}

// ============================================================================
// recordFilteredSample() tests
// ============================================================================

TEST_F(ProvenanceTrackerTest, RecordFilteredSample_DoesNotThrow) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    EXPECT_NO_THROW(
        tracker.recordFilteredSample("s001", "obligation", 0.38f, 0.5f)
    );
}

TEST_F(ProvenanceTrackerTest, RecordFilteredSample_AuditDisabled_DoesNotThrow) {
    cfg_.emit_audit_events = false;
    ProvenanceTracker tracker(cfg_, db_conn_);
    EXPECT_NO_THROW(
        tracker.recordFilteredSample("s002", "permission", 0.25f, 0.6f)
    );
}

// ============================================================================
// queryLineage() tests
// ============================================================================

TEST_F(ProvenanceTrackerTest, QueryLineage_ReturnsRootWithModelId) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    auto lineage = tracker.queryLineage("legal_v1.0");
    EXPECT_EQ(lineage.node_id, "legal_v1.0");
    EXPECT_EQ(lineage.node_type, "model");
}

TEST_F(ProvenanceTrackerTest, QueryLineage_EmptyStore_NoParents) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    auto lineage = tracker.queryLineage("legal_v1.0");
    EXPECT_TRUE(lineage.parents.empty());
}

TEST_F(ProvenanceTrackerTest, QueryLineage_AfterWrite_HasSampleParents) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    tracker.write({makeRecord("s001"), makeRecord("s002")});

    auto lineage = tracker.queryLineage("legal_v1.0");
    EXPECT_GE(lineage.parents.size(), 2u);
}

TEST_F(ProvenanceTrackerTest, QueryLineage_SampleParent_HasDocumentGrandparent) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    tracker.write({makeRecord("s001")});

    auto lineage = tracker.queryLineage("model");
    ASSERT_FALSE(lineage.parents.empty());
    auto& sample_node = lineage.parents[0];
    EXPECT_EQ(sample_node.node_type, "sample");
    // The sample should have a document parent
    EXPECT_FALSE(sample_node.parents.empty());
    EXPECT_EQ(sample_node.parents[0].node_type, "document");
}

// ============================================================================
// getRecord() tests
// ============================================================================

TEST_F(ProvenanceTrackerTest, GetRecord_AfterWrite_ReturnsRecord) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    tracker.write({makeRecord("s001")});

    auto rec = tracker.getRecord("s001");
    EXPECT_EQ(rec.sample_id, "s001");
    EXPECT_EQ(rec.source_doc_urn, "urn:document:legal-001");
    EXPECT_EQ(rec.modality, "text");
}

TEST_F(ProvenanceTrackerTest, GetRecord_Unknown_ReturnsEmptyRecord) {
    ProvenanceTracker tracker(cfg_, db_conn_);
    auto rec = tracker.getRecord("nonexistent");
    EXPECT_TRUE(rec.sample_id.empty());
}

// ============================================================================
// Default struct values
// ============================================================================

TEST_F(ProvenanceTrackerTest, ProvenanceRecord_DefaultValues) {
    ProvenanceRecord rec;
    EXPECT_TRUE(rec.source_doc_urn.empty());
    EXPECT_EQ(rec.extraction_timestamp, 0);
    EXPECT_TRUE(rec.labeler_version.empty());
    EXPECT_TRUE(rec.modality.empty());
    EXPECT_TRUE(rec.enrichment_query_fingerprints.empty());
    EXPECT_TRUE(rec.sample_id.empty());
}

TEST_F(ProvenanceTrackerTest, ProvenanceTrackerConfig_DefaultValues) {
    ProvenanceTrackerConfig cfg;
    EXPECT_EQ(cfg.graph_collection, "TrainingSamples");
    EXPECT_EQ(cfg.edge_collection, "DerivedFrom");
    EXPECT_EQ(cfg.batch_write_size, 200u);
    EXPECT_TRUE(cfg.reject_without_urn);
    EXPECT_TRUE(cfg.emit_audit_events);
    EXPECT_EQ(cfg.write_timeout_ms, 0u);
}

// ============================================================================
// Write-timeout tests (#5414 – no_timeout fix)
// ============================================================================

// With write_timeout_ms == 0 (default) all records are written normally.
TEST_F(ProvenanceTrackerTest, WriteTimeout_ZeroMeans_NoLimit) {
    cfg_.write_timeout_ms = 0;
    ProvenanceTracker tracker(cfg_, db_conn_);

    std::vector<ProvenanceRecord> records = {};

    for (int i = 0; i < 5; ++i) {
        ProvenanceRecord r;
        r.sample_id       = "s" + std::to_string(i);
        r.source_doc_urn  = "urn:doc:" + std::to_string(i);
        r.labeler_version = "v1";
        r.modality        = "text";
        records.push_back(r);
    }
    auto stats = tracker.write(records);
    EXPECT_EQ(stats.records_written + stats.records_rejected,
              static_cast<size_t>(5))
        << "All records must be accounted for when no timeout is set";
    EXPECT_EQ(stats.records_rejected, 0u);
}

// With a very large timeout (100 s), all records should still be written.
TEST_F(ProvenanceTrackerTest, WriteTimeout_LargeTimeout_AllRecordsWritten) {
    cfg_.write_timeout_ms = 100000; // 100 s
    ProvenanceTracker tracker(cfg_, db_conn_);

    std::vector<ProvenanceRecord> records = {};

    for (int i = 0; i < 3; ++i) {
        ProvenanceRecord r;
        r.sample_id       = "t" + std::to_string(i);
        r.source_doc_urn  = "urn:doc:" + std::to_string(i);
        r.labeler_version = "v1";
        r.modality        = "text";
        records.push_back(r);
    }
    auto stats = tracker.write(records);
    EXPECT_EQ(stats.records_written, 3u);
    EXPECT_EQ(stats.records_rejected, 0u);
}

// With a 1 ms timeout the call must return without blocking and account for
// every input record (written + rejected == total).
TEST_F(ProvenanceTrackerTest, WriteTimeout_VeryShortTimeout_ReturnsEarly) {
    cfg_.write_timeout_ms    = 1; // 1 ms
    cfg_.batch_write_size    = 1; // process one record per batch iteration
    ProvenanceTracker tracker(cfg_, db_conn_);

    // Build a large record set so the loop would normally take longer than 1 ms.
    std::vector<ProvenanceRecord> records = {};

    for (int i = 0; i < 1000; ++i) {
        ProvenanceRecord r;
        r.sample_id       = "u" + std::to_string(i);
        r.source_doc_urn  = "urn:doc:" + std::to_string(i);
        r.labeler_version = "v1";
        r.modality        = "text";
        records.push_back(r);
    }
    auto stats = tracker.write(records);
    EXPECT_EQ(stats.records_written + stats.records_rejected, 1000u)
        << "written + rejected must equal total input regardless of timeout";
}

// ============================================================================
// ConfidenceCalibrator tests
// ============================================================================

class ConfidenceCalibratorTest : public ::testing::Test {};

TEST_F(ConfidenceCalibratorTest, DefaultConstruction_NoSamples) {
    ConfidenceCalibrator cal;
    EXPECT_EQ(cal.sampleCount(), 0u);
}

TEST_F(ConfidenceCalibratorTest, AddSample_IncreasesCount) {
    ConfidenceCalibrator cal;
    cal.addSample("obligation", 0.8f, true);
    cal.addSample("obligation", 0.3f, false);
    EXPECT_EQ(cal.sampleCount(), 2u);
}

TEST_F(ConfidenceCalibratorTest, Reset_ClearsSamples) {
    ConfidenceCalibrator cal;
    cal.addSample("obligation", 0.7f, true);
    cal.reset();
    EXPECT_EQ(cal.sampleCount(), 0u);
}

TEST_F(ConfidenceCalibratorTest, Calibrate_EmptySamples_Succeeds) {
    ConfidenceCalibrator cal;
    auto result = cal.calibrate();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.thresholds.empty());
}

TEST_F(ConfidenceCalibratorTest, Calibrate_SingleCategory_ProducesThreshold) {
    ConfidenceCalibrator cal;
    // High-confidence samples that are correct → threshold should be low
    for (int i = 0; i < 20; ++i) {
        float conf = static_cast<float>(i) / 20.0f;
        cal.addSample("obligation", conf, conf >= 0.5f);
    }
    auto result = cal.calibrate();
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.thresholds.size(), 1u);
    EXPECT_EQ(result.thresholds[0].category, "obligation");
    EXPECT_GE(result.thresholds[0].threshold, 0.0f);
    EXPECT_LE(result.thresholds[0].threshold, 1.0f);
    EXPECT_EQ(result.thresholds[0].sample_count, 20u);
}

TEST_F(ConfidenceCalibratorTest, Calibrate_MultipleCategories_ProducesOneThresholdEach) {
    ConfidenceCalibrator cal;
    for (int i = 0; i < 10; ++i) {
        float c = static_cast<float>(i) / 10.0f;
        cal.addSample("obligation", c, c > 0.5f);
        cal.addSample("permission", c, c > 0.4f);
    }
    auto result = cal.calibrate();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.thresholds.size(), 2u);
}

TEST_F(ConfidenceCalibratorTest, Calibrate_ElapsedSeconds_NonNegative) {
    ConfidenceCalibrator cal;
    cal.addSample("a", 0.5f, true);
    auto result = cal.calibrate();
    EXPECT_GE(result.elapsed_seconds, 0.0);
}

TEST_F(ConfidenceCalibratorTest, Calibrate_SummaryNotEmpty) {
    ConfidenceCalibrator cal;
    cal.addSample("obligation", 0.7f, true);
    auto result = cal.calibrate();
    EXPECT_FALSE(result.summary.empty());
}

TEST_F(ConfidenceCalibratorTest, CalibratedThreshold_DefaultValues) {
    CalibratedThreshold t;
    EXPECT_TRUE(t.category.empty());
    EXPECT_EQ(t.threshold, 0.5f);
    EXPECT_EQ(t.sample_count, 0u);
    EXPECT_EQ(t.f1_improvement, 0.0);
}

TEST_F(ConfidenceCalibratorTest, CalibrationResult_DefaultValues) {
    CalibrationResult r;
    EXPECT_FALSE(r.success);
    EXPECT_TRUE(r.thresholds.empty());
    EXPECT_GE(r.elapsed_seconds, 0.0);
}
