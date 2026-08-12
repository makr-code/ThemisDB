/**
 * @file test_training_diagnostics_consistency.cpp
 * @brief Diagnostics-consistency regression for issue #5414 batch 5 (TRN-AUD-02).
 *
 * Verifies that error codes, stats fields, and failure signals are consistent
 * and non-ambiguous across the labeling, checkpoint, and serving stages.
 *
 * Test IDs
 * --------
 * TDC-01  LabelingStats default fields are all zero (clean baseline)
 * TDC-02  DeployResult::fail("reason") sets success=false, non-empty error
 * TDC-03  DeployResult::ok("v", 0.5f) sets success=true, empty error
 * TDC-04  DeployResult::fail with different reasons produces distinct error strings
 * TDC-05  ProvenanceWriteStats default fields are all zero (clean baseline)
 * TDC-06  ProvenanceWriteStats written+rejected invariant holds after a no-DB write
 * TDC-07  CheckpointManifestEntry default numeric fields are zero-initialised
 * TDC-08  DeployResult fail codes used by deployVersionEx are known constant strings
 * TDC-09  DeployResult ok split_applied roundtrips correctly
 * TDC-10  LabelingStats elapsed_seconds initialises to 0.0 (non-negative sentinel)
 */

#include <gtest/gtest.h>
#include "training/auto_labeler.h"
#include "training/adapter_serving.h"
#include "training/provenance_tracker.h"
#include "training/lora_checkpoint_manager.h"

#include <string>
#include <vector>

using namespace themis::training;

// ─────────────────────────────────────────────────────────────────────────────
// TDC-01: LabelingStats default values are all zero
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_01_labeling_stats_default_zero) {
    LabelingStats s;
    EXPECT_EQ(s.documents_processed,    0u);
    EXPECT_EQ(s.samples_created,        0u);
    EXPECT_EQ(s.high_confidence_samples, 0u);
    EXPECT_EQ(s.low_confidence_samples,  0u);
    EXPECT_DOUBLE_EQ(s.elapsed_seconds, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-02: DeployResult::fail signals failure unambiguously
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_02_deploy_fail_has_error_and_false_success) {
    auto r = DeployResult::fail("version_not_found");
    EXPECT_FALSE(r.success);
    EXPECT_FALSE(r.error.empty());
    EXPECT_EQ(r.error, "version_not_found");
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-03: DeployResult::ok signals success with empty error
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_03_deploy_ok_has_no_error) {
    auto r = DeployResult::ok("adapter_v2", 0.5f);
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.error.empty());
    EXPECT_EQ(r.active_version, "adapter_v2");
    EXPECT_FLOAT_EQ(r.split_applied, 0.5f);
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-04: Different fail codes produce distinct error strings
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_04_deploy_fail_codes_are_distinct) {
    const std::vector<std::string> fail_codes = {
        "version_not_found",
        "invalid_split",
        "integrity_failure",
        "router_unavailable",
        "router_update_failed",
    };
    for (size_t i = 0; i < fail_codes.size(); ++i) {
        auto ri = DeployResult::fail(fail_codes[i]);
        EXPECT_FALSE(ri.success) << "code: " << fail_codes[i];
        EXPECT_EQ(ri.error, fail_codes[i]);
        for (size_t j = i + 1; j < fail_codes.size(); ++j) {
            auto rj = DeployResult::fail(fail_codes[j]);
            EXPECT_NE(ri.error, rj.error)
                << "collision: " << fail_codes[i] << " vs " << fail_codes[j];
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-05: ProvenanceWriteStats default fields are all zero
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_05_provenance_stats_default_zero) {
    ProvenanceWriteStats s;
    EXPECT_EQ(s.records_written,   0u);
    EXPECT_EQ(s.records_rejected,  0u);
    EXPECT_DOUBLE_EQ(s.elapsed_seconds, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-06: ProvenanceWriteStats written+rejected invariant holds for empty write
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_06_provenance_write_empty_input_zero_stats) {
    ProvenanceTrackerConfig cfg;
    cfg.emit_audit_events = false;
    ProvenanceTracker tracker(cfg, /*db_connection=*/"");

    const std::vector<ProvenanceRecord> empty;
    auto stats = tracker.write(empty);

    EXPECT_EQ(stats.records_written + stats.records_rejected, 0u);
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-07: CheckpointManifestEntry default numeric fields are zero-initialised
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_07_checkpoint_entry_default_zero) {
    CheckpointManifestEntry e;
    EXPECT_EQ(e.epoch,  0u);
    EXPECT_EQ(e.step,   0u);
    EXPECT_DOUBLE_EQ(e.loss,     0.0);
    EXPECT_DOUBLE_EQ(e.accuracy, 0.0);
    EXPECT_EQ(e.saved_at, static_cast<std::time_t>(0));
    EXPECT_TRUE(e.checkpoint_path.empty());
    EXPECT_TRUE(e.sha256.empty());
    EXPECT_TRUE(e.adapter_version.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-08: fail codes used by IncrementalLoRATrainer::deployVersionEx are
//         consistent with DeployResult contract (non-empty string on failure)
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_08_deploy_fail_codes_non_empty) {
    for (const auto& code : {"version_not_found", "invalid_split",
                              "integrity_failure", "router_unavailable",
                              "router_update_failed"}) {
        auto r = DeployResult::fail(code);
        EXPECT_FALSE(r.success);
        EXPECT_FALSE(r.error.empty()) << "fail code is empty for: " << code;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-09: DeployResult::ok split_applied roundtrips exactly (no silent clamp)
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_09_deploy_ok_split_roundtrip) {
    for (float split : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
        auto r = DeployResult::ok("v1", split);
        EXPECT_FLOAT_EQ(r.split_applied, split) << "split=" << split;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TDC-10: LabelingStats elapsed_seconds initialises to non-negative sentinel
// ─────────────────────────────────────────────────────────────────────────────
TEST(TrainingDiagnosticsConsistency, TDC_10_labeling_stats_elapsed_non_negative) {
    LabelingStats s;
    EXPECT_GE(s.elapsed_seconds, 0.0);
}
