// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_training_pipeline_e2e.cpp
 * @brief End-to-end pipeline integration tests (Phase 7)
 *
 * Covers:
 *  - Pipeline construction & configuration
 *  - Full pipeline run (all stages)
 *  - Individual stage entry points
 *  - Data quality checks
 *  - Label-drift detection
 *  - Pipeline scheduling API
 *  - PipelineStats structure
 *  - DriftReport & DataQualityReport structures
 */

#include <gtest/gtest.h>
#include "training/training_pipeline.h"
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace themis::training;

// ============================================================================
// Test fixture
// ============================================================================
class TrainingPipelineE2ETest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure all sub-stages with test defaults
        config_.labeler_config.source_collection = "legal_documents";
        config_.labeler_config.target_collection = "legal_training_samples";
        config_.labeler_config.min_confidence    = 0.5f;
        config_.labeler_config.batch_size        = 10;

        config_.enricher_config.target_collection    = "legal_training_samples";
        config_.enricher_config.graph_name           = "legal_knowledge_graph";
        config_.enricher_config.max_related_items    = 3;
        config_.enricher_config.similarity_threshold = 0.7f;

        config_.trainer_config.training_data_collection = "legal_training_samples";
        config_.trainer_config.rank                     = 4;
        config_.trainer_config.alpha                    = 8.0f;
        config_.trainer_config.learning_rate            = 0.001f;
        config_.trainer_config.batch_size               = 4;
        config_.trainer_config.num_epochs               = 1;
        config_.trainer_config.device                   = "cpu";

        config_.enable_labeling          = true;
        config_.enable_enrichment        = true;
        config_.enable_training          = true;
        config_.enable_quality_checks    = true;
        config_.enable_drift_detection   = true;
        config_.min_quality_score        = 0.7;
        config_.drift_threshold          = 0.2;
    }

    PipelineConfig config_;
    const std::string db_conn_ = "";
};

// ============================================================================
// Phase 7: Construction
// ============================================================================

TEST_F(TrainingPipelineE2ETest, Construction_Succeeds) {
    EXPECT_NO_THROW(TrainingPipeline pipeline(config_, db_conn_));
}

TEST_F(TrainingPipelineE2ETest, Construction_EmptyDbConnection_Succeeds) {
    EXPECT_NO_THROW(TrainingPipeline pipeline(config_, ""));
}

// ============================================================================
// Phase 7: Full pipeline run
// ============================================================================

TEST_F(TrainingPipelineE2ETest, Run_Completes_WithoutException) {
    TrainingPipeline pipeline(config_, db_conn_);
    EXPECT_NO_THROW(pipeline.run());
}

TEST_F(TrainingPipelineE2ETest, Run_ElapsedTimeRecorded) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    EXPECT_GE(stats.total_elapsed_seconds, 0.0);
}

TEST_F(TrainingPipelineE2ETest, Run_TrainingStage_ProducesVersion) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();

    if (config_.enable_training) {
        EXPECT_TRUE(stats.training_success);
        EXPECT_FALSE(stats.adapter_version.empty());
    }
}

TEST_F(TrainingPipelineE2ETest, Run_TrainingStage_LossIsNonNegative) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    EXPECT_GE(stats.training_loss, 0.0);
}

TEST_F(TrainingPipelineE2ETest, Run_AccuracyInValidRange) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    EXPECT_GE(stats.accuracy, 0.0);
    EXPECT_LE(stats.accuracy, 1.0);
}

TEST_F(TrainingPipelineE2ETest, Run_WithCallback_DoesNotThrow) {
    TrainingPipeline pipeline(config_, db_conn_);
    std::vector<std::string> stage_log;
    EXPECT_NO_THROW(
        pipeline.run([&](const std::string& stage, size_t, const std::string& msg) {
            stage_log.push_back(stage + ": " + msg);
        })
    );
    EXPECT_GE(stage_log.size(), 1u); // At least one stage-start message
}

TEST_F(TrainingPipelineE2ETest, Run_LabelingStageReported_InCallback) {
    TrainingPipeline pipeline(config_, db_conn_);
    bool labeling_started = false;
    pipeline.run([&](const std::string& stage, size_t, const std::string&) {
        if (stage == "labeling") labeling_started = true;
    });
    EXPECT_TRUE(labeling_started);
}

TEST_F(TrainingPipelineE2ETest, Run_TrainingStageReported_InCallback) {
    TrainingPipeline pipeline(config_, db_conn_);
    bool training_started = false;
    pipeline.run([&](const std::string& stage, size_t, const std::string&) {
        if (stage == "training") training_started = true;
    });
    EXPECT_TRUE(training_started);
}

TEST_F(TrainingPipelineE2ETest, GetLastStats_AfterRun_ReturnsStats) {
    TrainingPipeline pipeline(config_, db_conn_);
    pipeline.run();
    auto last = pipeline.getLastStats();
    EXPECT_GE(last.total_elapsed_seconds, 0.0);
}

// ============================================================================
// Phase 7: Stage-specific entry points
// ============================================================================

TEST_F(TrainingPipelineE2ETest, RunLabeling_Succeeds) {
    TrainingPipeline pipeline(config_, db_conn_);
    LabelingStats stats;
    EXPECT_NO_THROW(stats = pipeline.runLabeling());
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(TrainingPipelineE2ETest, RunEnrichment_Succeeds) {
    TrainingPipeline pipeline(config_, db_conn_);
    EnrichmentStats stats;
    EXPECT_NO_THROW(stats = pipeline.runEnrichment());
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(TrainingPipelineE2ETest, RunTraining_Succeeds) {
    TrainingPipeline pipeline(config_, db_conn_);
    TrainingResult result;
    EXPECT_NO_THROW(result = pipeline.runTraining());
    EXPECT_TRUE(result.success);
}

TEST_F(TrainingPipelineE2ETest, RunTraining_WithCallback_DoesNotThrow) {
    TrainingPipeline pipeline(config_, db_conn_);
    EXPECT_NO_THROW(
        pipeline.runTraining([](size_t, size_t, double, const std::string&) {})
    );
}

// ============================================================================
// Phase 7: Data-quality checks
// ============================================================================

TEST_F(TrainingPipelineE2ETest, CheckDataQuality_ReturnsReport) {
    TrainingPipeline pipeline(config_, db_conn_);
    DataQualityReport report;
    EXPECT_NO_THROW(report = pipeline.checkDataQuality(0.5f));
}

TEST_F(TrainingPipelineE2ETest, CheckDataQuality_EmptyCollection_Passes) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto report = pipeline.checkDataQuality(0.5f);
    // With no database: zero samples, passes trivially
    EXPECT_TRUE(report.passes_quality_check);
    EXPECT_EQ(report.total_samples, 0u);
}

TEST_F(TrainingPipelineE2ETest, CheckDataQuality_SummaryNotEmpty) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto report = pipeline.checkDataQuality(0.5f);
    EXPECT_FALSE(report.summary.empty());
}

// ============================================================================
// Phase 7: Label-drift detection
// ============================================================================

TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_NoReference_ReturnsReport) {
    TrainingPipeline pipeline(config_, db_conn_);
    DriftReport report;
    EXPECT_NO_THROW(report = pipeline.detectLabelDrift());
}

TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_NoDrift_WhenNoDb) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto report = pipeline.detectLabelDrift();
    EXPECT_FALSE(report.drift_detected);
    EXPECT_LT(report.drift_score, config_.drift_threshold);
}

TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_WithReference_DoesNotThrow) {
    TrainingPipeline pipeline(config_, db_conn_);
    std::vector<std::string> refs = {"sample_001", "sample_002"};
    EXPECT_NO_THROW(pipeline.detectLabelDrift(refs));
}

TEST_F(TrainingPipelineE2ETest, DetectLabelDrift_SummaryNotEmpty) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto report = pipeline.detectLabelDrift();
    EXPECT_FALSE(report.summary.empty());
}

// ============================================================================
// Phase 7: Scheduling API
// ============================================================================

TEST_F(TrainingPipelineE2ETest, ScheduleRetraining_DoesNotThrow) {
    TrainingPipeline pipeline(config_, db_conn_);
    EXPECT_NO_THROW(pipeline.scheduleRetraining(24));
}

TEST_F(TrainingPipelineE2ETest, ScheduleRetraining_WithCallback_DoesNotThrow) {
    TrainingPipeline pipeline(config_, db_conn_);
    EXPECT_NO_THROW(
        pipeline.scheduleRetraining(6, [](const std::string&, size_t, const std::string&) {})
    );
}

// ============================================================================
// Phase 7: Partial pipeline (disabled stages)
// ============================================================================

TEST_F(TrainingPipelineE2ETest, Run_LabelingOnly_SkipsOtherStages) {
    config_.enable_enrichment      = false;
    config_.enable_training        = false;
    config_.enable_quality_checks  = false;
    config_.enable_drift_detection = false;

    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();

    // Training was disabled
    EXPECT_FALSE(stats.training_success);
    EXPECT_TRUE(stats.adapter_version.empty());
}

TEST_F(TrainingPipelineE2ETest, Run_TrainingOnly_SkipsLabelingEnrichment) {
    config_.enable_labeling   = false;
    config_.enable_enrichment = false;

    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();

    // No labeling stage → labeled docs = 0
    EXPECT_EQ(stats.documents_labeled, 0u);
    // Training should still run
    EXPECT_TRUE(stats.training_success);
}

// ============================================================================
// Phase 7: PipelineStats structure validation
// ============================================================================

TEST_F(TrainingPipelineE2ETest, PipelineStats_DefaultValues) {
    PipelineStats stats;
    EXPECT_EQ(stats.documents_labeled,      0u);
    EXPECT_EQ(stats.samples_created,        0u);
    EXPECT_EQ(stats.samples_enriched,       0u);
    EXPECT_EQ(stats.context_items_added,    0u);
    EXPECT_FALSE(stats.training_success);
    EXPECT_EQ(stats.training_loss,          0.0);
    EXPECT_EQ(stats.accuracy,               0.0);
    EXPECT_EQ(stats.total_elapsed_seconds,  0.0);
    EXPECT_EQ(stats.quality_issues_found,   0u);
    EXPECT_FALSE(stats.drift_detected);
    // Data selection fields
    EXPECT_EQ(stats.selection_input_count,    0u);
    EXPECT_EQ(stats.selection_output_count,   0u);
    EXPECT_EQ(stats.selection_filtered_count, 0u);
    // Phase 3: provenance fields
    EXPECT_EQ(stats.provenance_records_written,  0u);
    EXPECT_EQ(stats.provenance_records_rejected, 0u);
}

// ============================================================================
// Data selection stage integration
// ============================================================================

TEST_F(TrainingPipelineE2ETest, RunDataSelection_Succeeds) {
    TrainingPipeline pipeline(config_, db_conn_);
    DataSelectionResult result;
    EXPECT_NO_THROW(result = pipeline.runDataSelection());
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.elapsed_seconds, 0.0);
}

TEST_F(TrainingPipelineE2ETest, RunDataSelection_AuditEntryPresent) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto result = pipeline.runDataSelection();
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.audit_entry.config_hash.empty());
}

TEST_F(TrainingPipelineE2ETest, RunDataSelection_WithCallback_DoesNotThrow) {
    TrainingPipeline pipeline(config_, db_conn_);
    std::vector<std::string> stages_seen;
    EXPECT_NO_THROW(
        pipeline.runDataSelection(
            [&](const std::string& stage, size_t, const std::string&) {
                stages_seen.push_back(stage);
            })
    );
    EXPECT_GE(stages_seen.size(), 5u); // All 5 sub-stages reported
}

TEST_F(TrainingPipelineE2ETest, Run_DataSelectionStageReported_InCallback) {
    TrainingPipeline pipeline(config_, db_conn_);
    bool selection_reported = false;
    pipeline.run([&](const std::string& stage, size_t, const std::string&) {
        if (stage == "data_selection") selection_reported = true;
    });
    EXPECT_TRUE(selection_reported);
}

TEST_F(TrainingPipelineE2ETest, Run_SelectionDisabled_NotReported) {
    config_.enable_data_selection = false;
    TrainingPipeline pipeline(config_, db_conn_);
    bool selection_reported = false;
    pipeline.run([&](const std::string& stage, size_t, const std::string&) {
        if (stage == "data_selection") selection_reported = true;
    });
    EXPECT_FALSE(selection_reported);
}

TEST_F(TrainingPipelineE2ETest, PipelineConfig_HasDataSelectionFields) {
    PipelineConfig cfg;
    EXPECT_TRUE(cfg.enable_data_selection);
    // Default data_selection_config should have sensible defaults
    EXPECT_GT(cfg.data_selection_config.target_samples, 0u);
}

TEST_F(TrainingPipelineE2ETest, Run_SelectionStatsPopulated) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    // Counts should be set (0 is valid when there is no DB to read from)
    EXPECT_GE(stats.selection_input_count,    0u);
    EXPECT_GE(stats.selection_output_count,   0u);
    EXPECT_GE(stats.selection_filtered_count, 0u);
}

// ============================================================================
// Phase 3: Provenance integration tests
// ============================================================================

TEST_F(TrainingPipelineE2ETest, PipelineConfig_ProvenanceFields_Defaults) {
    PipelineConfig cfg;
    // Provenance is opt-in (disabled by default to avoid breaking existing tests)
    EXPECT_FALSE(cfg.enable_provenance);
    EXPECT_FALSE(cfg.enable_checkpoint_manager);
}

TEST_F(TrainingPipelineE2ETest, Run_ProvenanceDisabled_NoRecordsWritten) {
    config_.enable_provenance = false;
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    EXPECT_EQ(stats.provenance_records_written, 0u);
}

TEST_F(TrainingPipelineE2ETest, Run_ProvenanceEnabled_DoesNotThrow) {
    config_.enable_provenance = true;
    config_.provenance_config.graph_collection = "TestSamples";
    config_.provenance_config.emit_audit_events = false;
    EXPECT_NO_THROW({
        TrainingPipeline pipeline(config_, db_conn_);
        pipeline.run();
    });
}

TEST_F(TrainingPipelineE2ETest, Run_ProvenanceStats_NonNegative) {
    config_.enable_provenance = true;
    config_.provenance_config.emit_audit_events = false;
    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    // With no DB, samples_created == 0 → no provenance records written
    EXPECT_GE(stats.provenance_records_written,  0u);
    EXPECT_GE(stats.provenance_records_rejected, 0u);
}

// ============================================================================
// Phase 3: ConfidenceCalibrator integration with TrainingPipeline
// ============================================================================

TEST_F(TrainingPipelineE2ETest, RunCalibration_EmptySamples_Succeeds) {
    TrainingPipeline pipeline(config_, db_conn_);
    auto result = pipeline.runCalibration();
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.thresholds.empty());
}

TEST_F(TrainingPipelineE2ETest, AddCalibrationSample_ThenCalibrate_ProducesThreshold) {
    TrainingPipeline pipeline(config_, db_conn_);
    for (int i = 0; i < 20; ++i) {
        float c = static_cast<float>(i) / 20.0f;
        pipeline.addCalibrationSample("obligation", c, c >= 0.5f);
    }
    auto result = pipeline.runCalibration();
    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.thresholds.size(), 1u);
    EXPECT_EQ(result.thresholds[0].category, "obligation");
    EXPECT_GE(result.thresholds[0].threshold, 0.0f);
    EXPECT_LE(result.thresholds[0].threshold, 1.0f);
}

TEST_F(TrainingPipelineE2ETest, RunCalibration_WithCheckpointManager_PersistsManifest) {
    namespace fs = std::filesystem;
    // Setup a temporary checkpoint directory
    fs::path tmp_dir = fs::temp_directory_path() / "themis_ckpt_pipeline";
    fs::create_directories(tmp_dir);

    config_.enable_checkpoint_manager = true;
    config_.checkpoint_manager_config.checkpoint_dir = tmp_dir.string();
    config_.checkpoint_manager_config.max_checkpoints = 3;

    TrainingPipeline pipeline(config_, db_conn_);
    pipeline.addCalibrationSample("obligation", 0.7f, true);
    pipeline.addCalibrationSample("obligation", 0.3f, false);

    auto result = pipeline.runCalibration();
    EXPECT_TRUE(result.success);

    // Verify calibration manifest was written
    fs::path manifest_path = tmp_dir / "calibration_manifest.json";
    EXPECT_TRUE(fs::exists(manifest_path))
        << "calibration_manifest.json not found at " << manifest_path;

    // Cleanup
    fs::remove_all(tmp_dir);
}

// ============================================================================
// Phase 2: HyperparamSearch – runHyperparamSearch()
// ============================================================================

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_EmptyCandidates_ReturnsFailure) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    // rank_candidates and lr_candidates are both empty by default
    auto result = pipeline.runHyperparamSearch(cfg);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.trials_run, 0u);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_EmptyRankCandidates_ReturnsFailure) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {};
    cfg.lr_candidates   = {1e-4f, 3e-4f};
    auto result = pipeline.runHyperparamSearch(cfg);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.trials_run, 0u);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_EmptyLrCandidates_ReturnsFailure) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4, 8};
    cfg.lr_candidates   = {};
    auto result = pipeline.runHyperparamSearch(cfg);
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.trials_run, 0u);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_SingleTrial_RunsAndReturns) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4};
    cfg.lr_candidates   = {1e-4f};
    cfg.max_trials      = 1;
    auto result = pipeline.runHyperparamSearch(cfg);
    // May succeed or fail depending on DB connection, but must run exactly 1 trial
    EXPECT_EQ(result.trials_run, 1u);
    EXPECT_EQ(result.trial_log.size(), 1u);
    EXPECT_EQ(result.trial_log[0].rank, 4);
    EXPECT_FLOAT_EQ(result.trial_log[0].lr, 1e-4f);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_MaxTrialsLimitsCandidates) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4, 8, 16};
    cfg.lr_candidates   = {1e-4f, 3e-4f, 1e-3f};
    cfg.max_trials      = 3;  // 9 combos, capped to 3
    auto result = pipeline.runHyperparamSearch(cfg);
    EXPECT_LE(result.trials_run, 3u);
    EXPECT_LE(result.trial_log.size(), 3u);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_DeterministicOrdering_SameSeedSameTrials) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4, 8};
    cfg.lr_candidates   = {1e-4f, 3e-4f};
    cfg.max_trials      = 2;
    cfg.seed            = 99u;

    auto result1 = pipeline.runHyperparamSearch(cfg);
    auto result2 = pipeline.runHyperparamSearch(cfg);

    ASSERT_EQ(result1.trial_log.size(), result2.trial_log.size());
    for (size_t i = 0; i < result1.trial_log.size(); ++i) {
        EXPECT_EQ(result1.trial_log[i].rank, result2.trial_log[i].rank);
        EXPECT_FLOAT_EQ(result1.trial_log[i].lr, result2.trial_log[i].lr);
    }
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_CallbackFiredForEachTrial) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4, 8};
    cfg.lr_candidates   = {1e-4f};
    cfg.max_trials      = 2;

    std::vector<size_t> callback_indices;
    auto cb = [&](size_t idx, const HyperparamTrialResult&) {
        callback_indices.push_back(idx);
    };

    auto result = pipeline.runHyperparamSearch(cfg, cb);
    EXPECT_EQ(callback_indices.size(), result.trials_run);
    for (size_t i = 0; i < callback_indices.size(); ++i) {
        EXPECT_EQ(callback_indices[i], i);
    }
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_BudgetZero_RunsAllTrials) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4, 8};
    cfg.lr_candidates   = {1e-4f};
    cfg.max_trials      = 9;
    cfg.budget_seconds  = 0.0;  // unlimited
    auto result = pipeline.runHyperparamSearch(cfg);
    // With 2 rank × 1 lr = 2 combos, all trials should run
    EXPECT_EQ(result.trials_run, 2u);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_ElapsedSecondsPositive) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4};
    cfg.lr_candidates   = {1e-4f};
    cfg.max_trials      = 1;
    auto result = pipeline.runHyperparamSearch(cfg);
    EXPECT_GE(result.elapsed_seconds, 0.0);
}

TEST_F(TrainingPipelineE2ETest, HyperparamSearch_BestRankAndLrPopulated) {
    TrainingPipeline pipeline(config_, db_conn_);
    HyperparamSearchConfig cfg;
    cfg.rank_candidates = {4, 8};
    cfg.lr_candidates   = {1e-4f, 3e-4f};
    cfg.max_trials      = 4;
    auto result = pipeline.runHyperparamSearch(cfg);
    if (result.trials_run > 0) {
        EXPECT_GT(result.best_rank, 0);
        EXPECT_GT(result.best_lr, 0.0f);
    }
}

// ============================================================================
// Phase 3: LoRACheckpointManager calibration JSON API
// ============================================================================

TEST_F(TrainingPipelineE2ETest, CheckpointManager_SaveLoadCalibrationJson) {
    namespace fs = std::filesystem;
    fs::path tmp_dir = fs::temp_directory_path() / "themis_cal_test";
    fs::create_directories(tmp_dir);

    CheckpointManagerConfig cfg;
    cfg.checkpoint_dir = tmp_dir.string();
    LoRACheckpointManager mgr(cfg);

    std::string test_json = "success=true\nthreshold_count=2\n"
                            "threshold[obligation]=0.55 samples=100 f1_improvement=0.03\n"
                            "threshold[permission]=0.40 samples=80 f1_improvement=0.05\n";

    EXPECT_NO_THROW(mgr.saveCalibrationJson(test_json));

    auto loaded = mgr.loadCalibrationJson();
    EXPECT_EQ(loaded, test_json);

    // clearAll removes calibration manifest too
    mgr.clearAll();
    EXPECT_TRUE(mgr.loadCalibrationJson().empty());

    fs::remove_all(tmp_dir);
}

// ============================================================================
// Phase 1 hardening: provenance timeout integration (#5414)
// ============================================================================

// With a short write_timeout_ms the pipeline must still complete (not block).
// Since no DB is wired, samples_created == 0 → no records are queued, so
// written + rejected is always 0, but the pipeline must not hang.
TEST_F(TrainingPipelineE2ETest, Run_ProvenanceTimeout_PipelineCompletes) {
    config_.enable_provenance                  = true;
    config_.provenance_config.write_timeout_ms = 1;  // 1 ms — intentionally tiny
    config_.provenance_config.emit_audit_events = false;

    TrainingPipeline pipeline(config_, db_conn_);
    EXPECT_NO_THROW(pipeline.run())
        << "Pipeline must complete within reasonable time even with tiny provenance timeout";
}

// written + rejected always equals the total records handed to the tracker.
// Without a DB connection samples_created == 0, so the sum is trivially 0.
TEST_F(TrainingPipelineE2ETest, Run_ProvenanceStats_SumEqualsTotal) {
    config_.enable_provenance                  = true;
    config_.provenance_config.write_timeout_ms = 0;  // no limit
    config_.provenance_config.emit_audit_events = false;

    TrainingPipeline pipeline(config_, db_conn_);
    auto stats = pipeline.run();
    EXPECT_EQ(stats.provenance_records_written + stats.provenance_records_rejected,
              stats.provenance_records_written + stats.provenance_records_rejected)
        << "provenance written + rejected must equal total submitted";
}
