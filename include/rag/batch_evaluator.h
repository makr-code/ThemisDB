/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            batch_evaluator.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:12:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     275                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file batch_evaluator.h
 * @brief Batch evaluation pipeline for RAG Judge
 * 
 * Phase 6: Implements batch processing, parallel execution, and async evaluation
 */

#pragma once

#include "rag/rag_judge.h"
#include <vector>
#include <future>
#include <functional>
#include <queue>
#include <thread>
#include <atomic>

namespace themis::rag::judge {

/**
 * @brief Batch evaluation progress
 */
struct BatchProgress {
    size_t total_items;
    size_t completed_items;
    size_t failed_items;
    double progress_percentage;
    std::chrono::milliseconds elapsed_time;
    std::chrono::milliseconds estimated_time_remaining;
};

/**
 * @brief Batch evaluation result
 */
struct BatchEvaluationResult {
    std::vector<EvaluationResult> results;
    BatchProgress progress;
    
    // Aggregated statistics
    double average_faithfulness;
    double average_relevance;
    double average_completeness;
    double average_coherence;
    double average_overall_score;
    
    size_t passed_quality_threshold;
    size_t failed_quality_threshold;
    
    std::chrono::milliseconds total_time;
};

/**
 * @brief Configuration for batch evaluator
 */
struct BatchEvaluatorConfig {
    size_t batch_size = 8;                  ///< Number of items per batch
    size_t num_workers = 4;                 ///< Number of parallel worker threads
    bool enable_progress_tracking = true;   ///< Track and report progress
    bool fail_fast = false;                 ///< Stop on first failure
    
    std::chrono::seconds timeout_per_item = std::chrono::seconds(30);
    
    // Callback for progress updates
    std::function<void(const BatchProgress&)> progress_callback;
};

/**
 * @brief Async evaluation handle
 */
class AsyncEvaluationHandle {
public:
    /**
     * @brief Check if evaluation is complete
     * @return true if done
     */
    bool isDone() const;
    
    /**
     * @brief Wait for evaluation to complete
     * @param timeout Maximum time to wait
     * @return true if completed within timeout
     */
    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    
    /**
     * @brief Get evaluation result (blocking)
     * @return Evaluation result
     * @throws std::runtime_error if evaluation failed
     */
    EvaluationResult get();
    
    /**
     * @brief Cancel evaluation
     */
    void cancel();
    
private:
    friend class BatchEvaluator;
    std::future<EvaluationResult> future_;
    std::atomic<bool> cancelled_;
};

/**
 * @brief Batch evaluator for efficient parallel processing
 * 
 * Features:
 * - Parallel batch processing with configurable worker threads
 * - Async evaluation with futures/promises
 * - Progress tracking and reporting
 * - Queue-based processing with backpressure handling
 * - Timeout and cancellation support
 * - Comprehensive batch statistics
 */
class BatchEvaluator {
public:
    /**
     * @brief Construct batch evaluator
     * @param judge Judge instance to use
     * @param config Batch evaluator configuration
     */
    explicit BatchEvaluator(std::shared_ptr<RAGJudge> judge);
    BatchEvaluator(std::shared_ptr<RAGJudge> judge, const BatchEvaluatorConfig& config);
    
    /**
     * @brief Destructor - stops worker threads
     */
    ~BatchEvaluator();
    
    /**
     * @brief Evaluate batch of test cases synchronously
     * 
     * Processes all test cases and returns aggregated results.
     * 
     * @param test_cases Vector of test cases to evaluate
     * @return Batch evaluation result with all results
     */
    BatchEvaluationResult evaluateBatch(const std::vector<RAGTestCase>& test_cases);
    
    /**
     * @brief Evaluate batch of inputs synchronously
     * @param inputs Vector of evaluation inputs
     * @return Batch evaluation result
     */
    BatchEvaluationResult evaluateBatch(const std::vector<EvaluationInput>& inputs);
    
    /**
     * @brief Evaluate single input asynchronously
     * 
     * Returns immediately with a handle to check status and retrieve result.
     * 
     * @param input Evaluation input
     * @return Async evaluation handle
     */
    std::shared_ptr<AsyncEvaluationHandle> evaluateAsync(const EvaluationInput& input);
    
    /**
     * @brief Evaluate multiple inputs asynchronously
     * @param inputs Vector of evaluation inputs
     * @return Vector of async handles
     */
    std::vector<std::shared_ptr<AsyncEvaluationHandle>> evaluateAsync(
        const std::vector<EvaluationInput>& inputs
    );
    
    /**
     * @brief Submit evaluation to queue
     * 
     * Adds evaluation to processing queue without blocking.
     * Use progress callback to track completion.
     * 
     * @param input Evaluation input
     * @param callback Callback when evaluation completes
     */
    void submit(
        const EvaluationInput& input,
        std::function<void(const EvaluationResult&)> callback
    );
    
    /**
     * @brief Get current queue size
     * @return Number of pending evaluations
     */
    size_t getQueueSize() const;
    
    /**
     * @brief Wait for all queued evaluations to complete
     * @param timeout Maximum time to wait
     * @return true if all completed within timeout
     */
    bool waitForAll(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());
    
    /**
     * @brief Stop processing and clear queue
     */
    void stop();
    
    /**
     * @brief Resume processing after stop
     */
    void resume();
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const BatchEvaluatorConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    BatchEvaluatorConfig getConfig() const;

private:
    std::shared_ptr<RAGJudge> judge_;
    BatchEvaluatorConfig config_;
    
    // Worker thread pool
    std::vector<std::thread> workers_;
    std::atomic<bool> stop_requested_;
    std::atomic<bool> paused_;
    
    // Queue for async processing
    struct QueuedEvaluation {
        EvaluationInput input;
        std::promise<EvaluationResult> promise;
        bool has_promise = false;
        std::function<void(const EvaluationResult&)> callback;
    };
    
    std::queue<QueuedEvaluation> eval_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // Statistics
    std::atomic<size_t> total_processed_;
    std::atomic<size_t> total_failed_;
    
    // Worker thread function
    void workerThread();
    
    // Helper to process single evaluation
    EvaluationResult processEvaluation(const EvaluationInput& input);
    
    // Calculate batch statistics
    BatchEvaluationResult aggregateResults(
        const std::vector<EvaluationResult>& results,
        std::chrono::milliseconds total_time
    );
};

} // namespace themis::rag::judge
