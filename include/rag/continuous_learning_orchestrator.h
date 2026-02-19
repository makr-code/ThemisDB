/**
 * @file continuous_learning_orchestrator.h
 * @brief Central orchestrator for continuous RAG system improvement
 * 
 * Coordinates automatic optimization of LoRA adapters, prompts, and retrieval
 * parameters through A/B testing and statistical validation.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include "learning_metrics.h"
#include "ab_testing_framework.h"
#include "bayesian_optimizer.h"

namespace themis::rag::learning {

/**
 * @brief Configuration for continuous learning
 */
struct ContinuousLearningConfig {
    // Trigger thresholds
    size_t min_feedback_samples = 100;
    double min_accuracy_drop = 0.05;              ///< 5% drop triggers retraining
    std::chrono::hours retraining_interval{24};
    
    // Learning rates
    double prompt_learning_rate = 0.01;
    double retrieval_learning_rate = 0.1;
    double lora_learning_rate = 3e-4;
    
    // A/B Testing
    bool enable_ab_testing = true;
    double ab_test_traffic_split = 0.1;           ///< 10% traffic for new models
    size_t min_ab_samples = 1000;
    
    // Rollback safety
    double min_improvement_threshold = 0.02;      ///< 2% minimum improvement
    bool enable_auto_rollback = true;
    
    // Persistence
    std::string metrics_db_path = "data/learning_metrics.db";
    std::string model_registry_path = "data/model_registry/";
    
    // Learning loop
    std::chrono::seconds learning_loop_interval{3600};  ///< Check every hour
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
    explicit ContinuousLearningOrchestrator(const ContinuousLearningConfig& config);
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
    void registerLoRAAdapter(
        const std::string& adapter_id,
        const std::string& adapter_info
    );
    
    /**
     * @brief Register retrieval system for parameter tuning
     * @param system_id Unique identifier
     */
    void registerRetrievalSystem(const std::string& system_id);
    
    /**
     * @brief Register prompt system for optimization
     * @param system_id Unique identifier
     */
    void registerPromptSystem(const std::string& system_id);
    
    /**
     * @brief Register knowledge gap detector for metrics
     * @param detector_id Unique identifier
     */
    void registerKnowledgeGapDetector(const std::string& detector_id);
    
    // Feedback logging
    
    /**
     * @brief Log a single RAG interaction
     * @param interaction Complete interaction record
     */
    void logInteraction(const Interaction& interaction);
    
    /**
     * @brief Log multiple interactions in batch
     * @param interactions Vector of interactions
     */
    void logInteractionBatch(const std::vector<Interaction>& interactions);
    
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
    std::vector<PerformanceSnapshot> getPerformanceHistory(
        std::chrono::hours lookback_period
    ) const;
    
    /**
     * @brief Check if system is improving over time
     */
    bool isSystemImproving() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Learning strategies
    void runPromptOptimization();
    void runRetrievalOptimization();
    void runLoRARetraining();
    
    // A/B Testing
    void deployABTest(const std::string& model_id);
    void promoteOrRollback(const ABTestResult& result);
    
    // Persistence
    void saveMetrics();
    void loadMetrics();
    void saveModelCheckpoint(const std::string& model_id);
    
    // Background thread
    void learningLoopThread();
};

} // namespace themis::rag::learning
