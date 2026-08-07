/**
 * @file training_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: training_pipeline.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 96/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/auto_labeler.h"
#include "training/knowledge_graph_enricher.h"
#include "training/incremental_lora_trainer.h"
#include "training/lora_data_selection.h"
#include "training/provenance_tracker.h"
#include "training/lora_checkpoint_manager.h"
#include "training/training_error_codes.h"
#include "training/training_exceptions.h"
#include "training/training_error_diagnostics.h"

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

    // Provenance tracking (Phase 3)
    size_t provenance_records_written  = 0;  ///< Records written to provenance graph
    size_t provenance_records_rejected = 0;  ///< Records rejected (missing URN, etc.)

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

// ============================================================================
// Hyperparameter search (Phase 2 – automated rank / lr sweep)
// ============================================================================

/**
 * @brief Configuration for an automated hyperparameter search sweep.
 *
 * The search performs a Cartesian grid of (rank, lr) trials using a
 * deterministic, seeded random trial ordering.  It is budget-aware:
 * when `budget_seconds` elapses the best result seen so far is returned.
 *
 * Example usage:
 * @code
 * HyperparamSearchConfig cfg;
 * cfg.rank_candidates = {4, 8, 16};
 * cfg.lr_candidates   = {1e-4f, 3e-4f, 1e-3f};
 * cfg.max_trials      = 6;
 * cfg.budget_seconds  = 120.0;
 *
 * auto result = pipeline.runHyperparamSearch(cfg);
 * std::cout << "Best rank=" << result.best_rank
 *           << " lr=" << result.best_lr
 *           << " val_loss=" << result.best_val_loss << "\n";
 * @endcode
 */
struct HyperparamSearchConfig {
    std::vector<int>   rank_candidates;    ///< LoRA rank values to try
    std::vector<float> lr_candidates;      ///< Learning-rate values to try
    size_t             max_trials         = 9;    ///< Hard cap on number of trials
    double             budget_seconds     = 0.0;  ///< Wall-clock budget (0 = unlimited)
    float              validation_split   = 0.1f; ///< Validation fraction for each trial
    unsigned int       seed               = 42u;  ///< RNG seed for deterministic ordering

    HyperparamSearchConfig() = default;
};

/**
 * @brief Result for a single hyperparameter search trial.
 */
struct HyperparamTrialResult {
    int    rank      = 0;
    float  lr        = 0.0f;
    double val_loss  = 0.0;
    bool   success   = false;

    HyperparamTrialResult() = default;
};

/**
 * @brief Aggregated result of a hyperparameter search sweep.
 *
 * After a successful search the best configuration is automatically applied
 * to the pipeline's internal trainer (rank, alpha, learning_rate, validation_split).
 */
struct HyperparamResult {
    int    best_rank     = 0;
    float  best_lr       = 0.0f;
    double best_val_loss = 0.0;
    bool   success       = false;
    size_t trials_run    = 0;
    double elapsed_seconds = 0.0;
    std::string summary;

    /** @brief Per-trial log; one entry per executed trial. */
    std::vector<HyperparamTrialResult> trial_log;

    HyperparamResult() = default;
};

/**
 * @brief Per-trial progress callback for hyperparameter search.
 *
 * Called once after each trial completes.
 * @param trial_index  Zero-based index of the completed trial.
 * @param result       Result for that trial.
 */
using HyperparamSearchCallback =
    std::function<void(size_t trial_index, const HyperparamTrialResult& result)>;

/**
 * @brief Full training pipeline configuration (Phase 7)
 */
struct PipelineConfig {
    AutoLabelConfig          labeler_config;
    EnrichmentConfig         enricher_config;
    IncrementalTrainingConfig trainer_config;
    LoRADataSelectionConfig  data_selection_config;  ///< Automated data selection settings

    // Phase 3: Provenance tracking
    ProvenanceTrackerConfig  provenance_config;      ///< Provenance tracker settings
    bool enable_provenance      = false;  ///< Write provenance records after labeling (Phase 3)

    // Phase 3: Checkpoint manager + calibration manifest
    CheckpointManagerConfig  checkpoint_manager_config; ///< Checkpoint manager settings
    bool enable_checkpoint_manager = false; ///< Enable checkpoint manager for calibration manifest

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
     * @brief Sanitize a pipeline callback/progress message with the shared
     *        prompt-safety policy.
     *
     * The returned string is safe to emit through telemetry, logs, or external
     * callback sinks:
     * - blocked prompt-injection patterns are fail-closed to a constant marker
     * - allowed payloads are returned with control-token redaction applied
     *
     * @param message Raw callback/progress message.
     * @return Sanitized callback message suitable for downstream emission.
     */
    static std::string sanitizeCallbackMessage(const std::string& message);

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
     * 
     * @note Fail-closed guards (QW-40): Validates critical inputs before execution:
     *   - target_collection non-empty (prompt injection prevention)
     *   - drift_threshold in valid range [0, 1] (prevent invalid LLM queries)
     *   - data_selection collection_name non-empty (prevent injection)
     * If any guard fails, returns error_message in stats and avoids LLM calls.
     */
    PipelineStats run(PipelineCallback callback = nullptr);

    /**
     * @brief Run only the auto-labeling stage
        *
        * Callback messages are sanitized via the shared prompt-safety policy
        * before emission.
     */
    LabelingStats runLabeling(LabelingCallback callback = nullptr);

    /**
     * @brief Run only the enrichment stage
        *
        * Callback messages are sanitized via the shared prompt-safety policy
        * before emission.
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
     *                 Emitted messages are sanitized via the shared
     *                 prompt-safety policy before callback invocation.
     * @return Selection result including selected samples and provenance.
     */
    DataSelectionResult runDataSelection(
        SelectionProgressCallback callback = nullptr);

    /**
     * @brief Run only the training stage
        *
        * Callback messages are sanitized via the shared prompt-safety policy
        * before emission.
     */
    TrainingResult runTraining(TrainingCallback callback = nullptr);

    /**
     * @brief Run the confidence calibration stage and persist the result.
     *
     * Executes `ConfidenceCalibrator::calibrate()` on any accumulated
     * per-sample (confidence, correct) pairs and, when
     * `enable_checkpoint_manager` is true, writes a `calibration_manifest.json`
     * to the checkpoint directory via `LoRACheckpointManager`.
     *
     * @return Calibration result with per-category thresholds.
     */
    CalibrationResult runCalibration();

    /**
     * @brief Feed a per-sample validation pair to the internal calibrator.
     *
     * Should be called once per sample after the validation loop in
     * `IncrementalLoRATrainer` to accumulate data for `runCalibration()`.
     *
     * @param category      Legal category of the sample.
     * @param confidence    Model confidence score in [0, 1].
     * @param model_correct Whether the model produced the correct label.
     */
    void addCalibrationSample(const std::string& category,
                              float confidence,
                              bool model_correct);

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

    /**
     * @brief Run an automated hyperparameter search over rank × lr combinations.
     *
     * Generates the Cartesian product of `config.rank_candidates` and
     * `config.lr_candidates`, shuffles the trial list with `config.seed` for
     * deterministic ordering, then executes up to `config.max_trials` trials.
     * Each trial clones the pipeline's trainer config with the trial's rank and
     * learning rate and runs a single-pass training simulation.
     *
     * When `config.budget_seconds > 0` the search stops early once the wall-clock
     * budget is exceeded and returns the best result seen so far.
     *
     * On success, the best (rank, lr) pair is automatically applied to the
     * pipeline's internal trainer configuration so subsequent calls to
     * `run()` or `runTraining()` use the optimised hyperparameters.
     *
     * @param config    Search configuration (candidates, budget, seed).
     * @param callback  Optional per-trial callback.
     * @return Search result with best rank, lr, val_loss, and trial log.
     */
    HyperparamResult runHyperparamSearch(
        const HyperparamSearchConfig& config,
        HyperparamSearchCallback callback = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis

