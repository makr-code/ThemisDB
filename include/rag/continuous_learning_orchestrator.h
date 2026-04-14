/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_learning_orchestrator.h                 ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:55:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     259                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8f2d385c0e  2026-03-01  feat(rag): implement online learning from evaluation feed... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file continuous_learning_orchestrator.h
 * @brief Central orchestrator for continuous RAG system improvement
 *
 * Coordinates automatic optimization of LoRA adapters, prompts, and retrieval
 * parameters through A/B testing and statistical validation.
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "ab_testing_framework.h"
#include "bayesian_optimizer.h"
#include "learning_metrics.h"
#include "training/lora_data_selection.h"

namespace themis::rag::learning {

/**
 * @brief Configuration for continuous learning
 */
struct ContinuousLearningConfig {
    // Trigger thresholds
    size_t min_feedback_samples = 100;
    double min_accuracy_drop    = 0.05; ///< 5% drop triggers retraining
    std::chrono::hours retraining_interval{24};

    // Learning rates
    double prompt_learning_rate    = 0.01;
    double retrieval_learning_rate = 0.1;
    double lora_learning_rate      = 3e-4;

    // A/B Testing
    bool enable_ab_testing       = true;
    double ab_test_traffic_split = 0.1; ///< 10% traffic for new models
    size_t min_ab_samples        = 1000;

    // Rollback safety
    double min_improvement_threshold = 0.02; ///< 2% minimum improvement
    bool enable_auto_rollback        = true;

    // Persistence
    std::string metrics_db_path     = "data/learning_metrics.db";
    std::string model_registry_path = "data/model_registry/";

    // Learning loop
    std::chrono::seconds learning_loop_interval{3600}; ///< Check every hour

    // ---- Automated Data Selection integration ----
    /// Configuration for the data selection pipeline executed before retraining.
    themis::training::LoRADataSelectionConfig data_selection_config;

    /// Adaptive self-improvement rules applied after each selection run.
    themis::training::SelfImprovementConfig   self_improvement_config;

    /// Path to LoRATrainerConfig.yaml for live-reload (empty = use defaults).
    std::string lora_trainer_config_path;

    /// Path to SelfImprovementModule.yaml for live-reload (empty = use defaults).
    std::string self_improvement_config_path;
};

/**
 * @brief Continuous Learning Orchestrator
 *
 * Main class that coordinates automatic learning across all RAG components.
 * Implements trigger-based retraining, prompt optimization, retrieval tuning,
 * and A/B testing with statistical validation.
 */
class ContinuousLearningOrchestrator {
  public:
    explicit ContinuousLearningOrchestrator(const ContinuousLearningConfig &config);
    ~ContinuousLearningOrchestrator();

    // Main API

    /**
     * @brief Start the background learning loop
     *
     * Launches a background thread that periodically checks for learning
     * opportunities and triggers optimization as needed.
     */
    void startLearningLoop();

    /**
     * @brief Stop the background learning loop
     */
    void stopLearningLoop();

    /**
     * @brief Trigger a learning iteration manually
     *
     * Forces an immediate check and execution of learning strategies.
     */
    void triggerLearningIteration();

    // Component registration
    // Note: These are registration stubs - actual implementation would require
    // the full component classes which may not be available yet

    /**
     * @brief Register a LoRA adapter for automatic retraining
     * @param adapter_id Unique identifier for the adapter
     * @param adapter_info Metadata about the adapter
     */
    void registerLoRAAdapter(const std::string &adapter_id, const std::string &adapter_info);

    /**
     * @brief Register retrieval system for parameter tuning
     * @param system_id Unique identifier
     */
    void registerRetrievalSystem(const std::string &system_id);

    /**
     * @brief Register prompt system for optimization
     * @param system_id Unique identifier
     */
    void registerPromptSystem(const std::string &system_id);

    /**
     * @brief Register knowledge gap detector for metrics
     * @param detector_id Unique identifier
     */
    void registerKnowledgeGapDetector(const std::string &detector_id);

    // Feedback logging

    /**
     * @brief Log a single RAG interaction
     * @param interaction Complete interaction record
     */
    void logInteraction(const Interaction &interaction);

    /**
     * @brief Log multiple interactions in batch
     * @param interactions Vector of interactions
     */
    void logInteractionBatch(const std::vector<Interaction> &interactions);

    // Metrics & monitoring

    /**
     * @brief Get current learning statistics
     */
    LearningStats getStats() const;

    /**
     * @brief Get performance history over time
     * @param lookback_period How far back to look
     * @return Vector of performance snapshots
     */
    std::vector<PerformanceSnapshot> getPerformanceHistory(std::chrono::hours lookback_period) const;

    /**
     * @brief Check if system is improving over time
     */
    bool isSystemImproving() const;

    // ---- Data selection ----

    /**
     * @brief Run the automated data selection pipeline for a specific adapter.
     *
     * Loads (or reloads) configuration from `lora_trainer_config_path` if set,
     * executes all five data-selection stages on the provided candidate samples,
     * applies adaptive self-improvement rules to the config for the next run,
     * and returns the selection result including the JSONL audit entry.
     *
     * This is called automatically during `runLoRARetraining()` and can also be
     * invoked directly for manual testing or scheduled jobs.
     *
     * @param adapter_id        Adapter being retrained (used for logging).
     * @param candidate_samples Raw samples to run the pipeline on.
     * @param current_metrics   Monitoring metrics for adaptive threshold adjustment.
     * @return Selection result (selected samples + audit provenance).
     */
    themis::training::DataSelectionResult runDataSelectionForAdapter(
        const std::string& adapter_id,
        const std::vector<themis::training::DataSample>& candidate_samples,
        const themis::training::DataSelectionMetrics& current_metrics = {});

    /**
     * @brief Get the data selection configuration currently in use.
     */
    const themis::training::LoRADataSelectionConfig& getDataSelectionConfig() const;

    /**
     * @brief Update the data selection configuration (live reload).
     */
    void setDataSelectionConfig(const themis::training::LoRADataSelectionConfig& cfg);

    // ---- Adaptive retrieval ----

    /**
     * @brief Get the current optimized retrieval parameters.
     *
     * Returns the retrieval parameters most recently updated by the adaptive
     * optimization loop.  Callers should apply these parameters when issuing
     * retrieval requests so that the system benefits from online learning.
     *
     * The parameters are updated every time `triggerLearningIteration()` runs
     * and sufficient feedback (both user signals and evaluation confidence
     * scores) has been collected.  Until the first optimization cycle the
     * method returns the default values from `RetrievalParams`.
     *
     * @return Current optimized retrieval parameters (thread-safe read).
     */
    RetrievalParams getOptimizedRetrievalParams() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Learning strategies
    void runPromptOptimization();
    void runRetrievalOptimization();
    void runLoRARetraining();

    // A/B Testing
    void deployABTest(const std::string &model_id);
    void promoteOrRollback(const ABTestResult &result);

    // Persistence
    void saveMetrics();
    void loadMetrics();
    void saveModelCheckpoint(const std::string &model_id);

    // Background thread
    void learningLoopThread();
};

} // namespace themis::rag::learning
