/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            training_pipeline.h                                ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     234                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/auto_labeler.h"
#include "training/knowledge_graph_enricher.h"
#include "training/incremental_lora_trainer.h"
#include "training/lora_data_selection.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <map>

namespace themis {
namespace training {

/**
 * @brief Overall pipeline execution statistics (Phase 7)
 */
struct PipelineStats {
    // Auto-labeler stage
    size_t documents_labeled     = 0;
    size_t samples_created       = 0;
    size_t high_confidence       = 0;

    // Enricher stage
    size_t samples_enriched      = 0;
    size_t context_items_added   = 0;

    // Trainer stage
    bool   training_success      = false;
    double training_loss         = 0.0;
    double accuracy              = 0.0;
    std::string adapter_version;

    // Data selection stage
    size_t selection_input_count    = 0;  ///< Candidates fed to the selection pipeline
    size_t selection_output_count   = 0;  ///< Samples passing all selection stages
    size_t selection_filtered_count = 0;  ///< Samples removed by selection stages

    // Timing
    double total_elapsed_seconds = 0.0;

    // Data quality
    size_t quality_issues_found  = 0;
    bool   drift_detected        = false;

    PipelineStats() = default;
};

/**
 * @brief Data quality check result (Phase 7)
 */
struct DataQualityReport {
    size_t total_samples         = 0;
    size_t missing_input         = 0;   ///< Samples without input text
    size_t missing_output        = 0;   ///< Samples without expected output
    size_t low_confidence        = 0;   ///< Samples below confidence threshold
    size_t invalid_category      = 0;   ///< Samples with unknown category
    bool   passes_quality_check  = false;
    std::string summary;

    DataQualityReport() = default;
};

/**
 * @brief Label-drift detection result (Phase 7)
 */
struct DriftReport {
    bool   drift_detected        = false;
    double drift_score           = 0.0;  ///< 0..1 – higher means more drift
    std::map<std::string, double> category_shifts;  ///< Per-category confidence delta
    std::string summary;

    DriftReport() = default;
};

/**
 * @brief Per-category calibrated confidence threshold result.
 */
struct CalibratedThreshold {
    std::string category;          ///< Legal category (e.g., "obligation", "permission")
    float       threshold = 0.5f;  ///< Calibrated threshold in [0, 1]
    size_t      sample_count = 0;  ///< Number of samples used for calibration
    double      f1_improvement = 0.0; ///< Estimated F1 improvement vs. static baseline

    CalibratedThreshold() = default;
};

/**
 * @brief Result of a confidence calibration run.
 */
struct CalibrationResult {
    std::vector<CalibratedThreshold> thresholds; ///< Per-category calibrated thresholds
    double elapsed_seconds = 0.0;                ///< Calibration wall-clock time
    bool   success         = false;
    std::string summary;

    CalibrationResult() = default;
};

/**
 * @brief Isotonic-regression-based per-category confidence calibrator.
 *
 * Consumes per-sample (confidence, model_correct) pairs produced by the
 * validation loop in IncrementalLoRATrainer and computes optimal per-category
 * thresholds using the Pool Adjacent Violators (PAV) algorithm.
 *
 * Calibrated thresholds are stored alongside the LoRA checkpoint in
 * `calibration_manifest.json`.
 *
 * Example usage:
 * @code
 * ConfidenceCalibrator calibrator;
 * calibrator.addSample("obligation", 0.82f, true);
 * calibrator.addSample("obligation", 0.41f, false);
 * // ... add more samples
 * auto result = calibrator.calibrate();
 * for (auto& t : result.thresholds)
 *     std::cout << t.category << " -> " << t.threshold << "\n";
 * @endcode
 */
class ConfidenceCalibrator {
public:
    ConfidenceCalibrator() = default;

    /**
     * @brief Record a validation sample for calibration.
     * @param category      Legal category of the sample.
     * @param confidence    Model confidence score in [0, 1].
     * @param model_correct Whether the model produced the correct label.
     */
    void addSample(const std::string& category, float confidence, bool model_correct);

    /**
     * @brief Compute calibrated thresholds using isotonic regression (PAV).
     *
     * For each category, fits a monotone non-decreasing step function to the
     * (confidence → correct) pairs and selects the threshold that maximises F1.
     *
     * @return Calibration result with per-category thresholds.
     */
    CalibrationResult calibrate() const;

    /**
     * @brief Reset all accumulated samples.
     */
    void reset();

    /**
     * @brief Number of samples accumulated so far.
     */
    size_t sampleCount() const;

private:
    struct Sample { std::string category; float confidence; bool correct; };
    std::vector<Sample> samples_;
};

/**
 * @brief Pipeline progress callback (Phase 7)
 */
using PipelineCallback = std::function<void(const std::string& stage,
                                            size_t step,
                                            const std::string& message)>;

/**
 * @brief Full training pipeline configuration (Phase 7)
 */
struct PipelineConfig {
    AutoLabelConfig          labeler_config;
    EnrichmentConfig         enricher_config;
    IncrementalTrainingConfig trainer_config;
    LoRADataSelectionConfig  data_selection_config;  ///< Automated data selection settings

    bool enable_labeling        = true;   ///< Run auto-labeling stage
    bool enable_enrichment      = true;   ///< Run graph enrichment stage
    bool enable_data_selection  = true;   ///< Run automated data selection stage
    bool enable_training        = true;   ///< Run LoRA training stage
    bool enable_quality_checks  = true;   ///< Run data-quality checks
    bool enable_drift_detection = true;   ///< Run label-drift detection

    double min_quality_score = 0.7;  ///< Minimum overall quality score [0..1]
    double drift_threshold   = 0.2;  ///< Maximum acceptable drift score [0..1]

    PipelineConfig() = default;
};

/**
 * @brief End-to-end training pipeline orchestrator (Phase 7)
 *
 * Coordinates all training stages:
 *   Stage 1 – Auto-Labeling:    label raw legal documents → training samples
 *   Stage 2 – Graph Enrichment: add knowledge-graph context to samples
 *   Stage 3 – Data Quality:     validate sample quality, detect drift
 *   Stage 4 – LoRA Training:    fine-tune LoRA adapter on enriched samples
 *
 * Example usage:
 * @code
 * PipelineConfig cfg;
 * cfg.labeler_config.source_collection = "legal_documents";
 * cfg.trainer_config.num_epochs = 3;
 *
 * TrainingPipeline pipeline(cfg, "arango://localhost:8529");
 * auto stats = pipeline.run();
 * std::cout << "Adapter version: " << stats.adapter_version << "\n";
 * @endcode
 */
class TrainingPipeline {
public:
    /**
     * @brief Construct pipeline
     * @param config    Pipeline configuration
     * @param db_connection  Database connection string
     */
    explicit TrainingPipeline(const PipelineConfig& config,
                               const std::string& db_connection);

    ~TrainingPipeline();

    // Non-copyable
    TrainingPipeline(const TrainingPipeline&)            = delete;
    TrainingPipeline& operator=(const TrainingPipeline&) = delete;

    /**
     * @brief Execute the full training pipeline
     * @param callback Optional per-stage progress callback
     * @return Aggregated pipeline statistics
     */
    PipelineStats run(PipelineCallback callback = nullptr);

    /**
     * @brief Run only the auto-labeling stage
     */
    LabelingStats runLabeling(LabelingCallback callback = nullptr);

    /**
     * @brief Run only the enrichment stage
     */
    EnrichmentStats runEnrichment(EnrichmentCallback callback = nullptr);

    /**
     * @brief Run only the automated data selection stage
     *
     * Executes all five selection sub-stages (quality filter, deduplication,
     * clustering, scoring, curriculum sampling) on the current training
     * collection and returns the selection result with audit entry.
     *
     * @param callback Optional per-stage progress callback.
     * @return Selection result including selected samples and provenance.
     */
    DataSelectionResult runDataSelection(
        SelectionProgressCallback callback = nullptr);

    /**
     * @brief Run only the training stage
     */
    TrainingResult runTraining(TrainingCallback callback = nullptr);

    /**
     * @brief Perform data-quality checks on the training collection
     * @param min_confidence  Minimum acceptable sample confidence
     * @return Data quality report
     */
    DataQualityReport checkDataQuality(float min_confidence = 0.5f);

    /**
     * @brief Detect label drift between training samples and new data
     * @param reference_samples  Baseline sample IDs for comparison
     * @return Drift detection report
     */
    DriftReport detectLabelDrift(const std::vector<std::string>& reference_samples = {});

    /**
     * @brief Schedule automated retraining at a fixed interval
     * @param interval_hours  How often to retrain (in hours)
     * @param callback        Optional per-run callback
     */
    void scheduleRetraining(size_t interval_hours,
                             PipelineCallback callback = nullptr);

    /**
     * @brief Get the last pipeline execution statistics
     */
    PipelineStats getLastStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
