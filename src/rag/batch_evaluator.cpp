/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            batch_evaluator.cpp                                ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:18:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     382                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • 6efaebce20  2026-03-09  feat(rag): implement BatchEvaluator, CalibrationManager, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file batch_evaluator.cpp
 * @brief Batch evaluation pipeline for RAG Judge
 *
 * Implements parallel batch processing with configurable worker threads,
 * async evaluation via std::future/std::promise, progress tracking,
 * and aggregated statistics.
 */

#include "rag/batch_evaluator.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace themis::rag::judge {

// ---------------------------------------------------------------------------
// AsyncEvaluationHandle
// ---------------------------------------------------------------------------

bool AsyncEvaluationHandle::isDone() const {
    if (cancelled_.load()) return true;
    // Check if future is ready without blocking
    return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

bool AsyncEvaluationHandle::wait(std::chrono::milliseconds timeout) {
    if (cancelled_.load()) return true;
    if (timeout == std::chrono::milliseconds::max()) {
        future_.wait();
        return true;
    }
    return future_.wait_for(timeout) == std::future_status::ready;
}

EvaluationResult AsyncEvaluationHandle::get() {
    if (cancelled_.load()) {
        throw std::runtime_error("AsyncEvaluationHandle: evaluation was cancelled");
    }
    return future_.get();
}

void AsyncEvaluationHandle::cancel() {
    cancelled_.store(true);
}

// ---------------------------------------------------------------------------
// BatchEvaluator – construction / destruction
// ---------------------------------------------------------------------------

BatchEvaluator::BatchEvaluator(std::shared_ptr<RAGJudge> judge)
    : judge_(std::move(judge))
    , config_{}
    , stop_requested_(false)
    , paused_(false)
    , total_processed_(0)
    , total_failed_(0) {
    // Start worker threads
    workers_.reserve(config_.num_workers);
    for (size_t i = 0; i < config_.num_workers; ++i) {
        workers_.emplace_back(&BatchEvaluator::workerThread, this);
    }
}

BatchEvaluator::BatchEvaluator(std::shared_ptr<RAGJudge> judge,
                               const BatchEvaluatorConfig& config)
    : judge_(std::move(judge))
    , config_(config)
    , stop_requested_(false)
    , paused_(false)
    , total_processed_(0)
    , total_failed_(0) {
    workers_.reserve(config_.num_workers);
    for (size_t i = 0; i < config_.num_workers; ++i) {
        workers_.emplace_back(&BatchEvaluator::workerThread, this);
    }
}

BatchEvaluator::~BatchEvaluator() {
    stop();
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------

void BatchEvaluator::workerThread() {
    while (true) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        queue_cv_.wait(lock, [this] {
            return stop_requested_.load() ||
                   (!paused_.load() && !eval_queue_.empty());
        });

        if (stop_requested_.load() && eval_queue_.empty()) break;
        if (paused_.load()) continue;
        if (eval_queue_.empty()) continue;

        QueuedEvaluation item = std::move(eval_queue_.front());
        eval_queue_.pop();
        lock.unlock();

        if (!item.has_promise && !item.callback) continue;

        try {
            auto result = processEvaluation(item.input);
            ++total_processed_;
            if (item.callback) {
                item.callback(result);
            }
            if (item.has_promise) {
                item.promise.set_value(result);
            }
        } catch (const std::exception& e) {
            ++total_failed_;
            THEMIS_WARN("BatchEvaluator worker: evaluation failed: {}", e.what());
            if (item.has_promise) {
                try {
                    item.promise.set_exception(std::current_exception());
                } catch (...) {}
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Core evaluation helper
// ---------------------------------------------------------------------------

EvaluationResult BatchEvaluator::processEvaluation(const EvaluationInput& input) {
    return judge_->evaluate(input);
}

// ---------------------------------------------------------------------------
// Synchronous batch evaluation
// ---------------------------------------------------------------------------

BatchEvaluationResult BatchEvaluator::evaluateBatch(
    const std::vector<RAGTestCase>& test_cases) {
    std::vector<EvaluationInput> inputs;
    inputs.reserve(test_cases.size());
    for (const auto& tc : test_cases) {
        EvaluationInput in;
        in.query            = tc.query;
        in.documents        = tc.documents;
        in.generated_answer = tc.generated_answer;
        inputs.push_back(std::move(in));
    }
    return evaluateBatch(inputs);
}

BatchEvaluationResult BatchEvaluator::evaluateBatch(
    const std::vector<EvaluationInput>& inputs) {
    const auto start_time = std::chrono::steady_clock::now();

    std::vector<EvaluationResult> results;
    results.reserve(inputs.size());

    size_t completed = 0;
    size_t failed    = 0;

    for (const auto& input : inputs) {
        try {
            results.push_back(processEvaluation(input));
            ++completed;

            if (config_.enable_progress_tracking && config_.progress_callback) {
                BatchProgress progress;
                progress.total_items     = inputs.size();
                progress.completed_items = completed;
                progress.failed_items    = failed;
                progress.progress_percentage =
                    100.0 * static_cast<double>(completed) /
                    static_cast<double>(inputs.size());
                progress.elapsed_time =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start_time);
                config_.progress_callback(progress);
            }
        } catch (const std::exception& e) {
            ++failed;
            THEMIS_WARN("BatchEvaluator::evaluateBatch: item failed: {}", e.what());
            if (config_.fail_fast) break;
            // Push empty result as sentinel
            results.push_back(EvaluationResult{});
        }
    }

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    return aggregateResults(results, total_time);
}

// ---------------------------------------------------------------------------
// Async evaluation
// ---------------------------------------------------------------------------

std::shared_ptr<AsyncEvaluationHandle> BatchEvaluator::evaluateAsync(
    const EvaluationInput& input) {
    auto handle          = std::make_shared<AsyncEvaluationHandle>();
    handle->cancelled_.store(false);

    std::promise<EvaluationResult> promise;
    handle->future_ = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        QueuedEvaluation item;
        item.input    = input;
        item.promise  = std::move(promise);
        item.has_promise = true;
        eval_queue_.push(std::move(item));
    }
    queue_cv_.notify_one();

    return handle;
}

std::vector<std::shared_ptr<AsyncEvaluationHandle>> BatchEvaluator::evaluateAsync(
    const std::vector<EvaluationInput>& inputs) {
    std::vector<std::shared_ptr<AsyncEvaluationHandle>> handles;
    handles.reserve(inputs.size());
    for (const auto& input : inputs) {
        handles.push_back(evaluateAsync(input));
    }
    return handles;
}

// ---------------------------------------------------------------------------
// Queue-based submit
// ---------------------------------------------------------------------------

void BatchEvaluator::submit(
    const EvaluationInput& input,
    std::function<void(const EvaluationResult&)> callback) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    QueuedEvaluation item;
    item.input    = input;
    item.callback = std::move(callback);
    eval_queue_.push(std::move(item));
    queue_cv_.notify_one();
}

size_t BatchEvaluator::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return eval_queue_.size();
}

bool BatchEvaluator::waitForAll(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (eval_queue_.empty()) return true;
        }
        if (timeout != std::chrono::milliseconds::max() &&
            std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void BatchEvaluator::stop() {
    stop_requested_.store(true);
    queue_cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
    workers_.clear();
}

void BatchEvaluator::resume() {
    if (!stop_requested_.load()) {
        paused_.store(false);
        queue_cv_.notify_all();
        return;
    }
    // Re-start workers after stop
    stop_requested_.store(false);
    paused_.store(false);
    workers_.reserve(config_.num_workers);
    for (size_t i = 0; i < config_.num_workers; ++i) {
        workers_.emplace_back(&BatchEvaluator::workerThread, this);
    }
}

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

void BatchEvaluator::setConfig(const BatchEvaluatorConfig& config) {
    config_ = config;
}

BatchEvaluatorConfig BatchEvaluator::getConfig() const {
    return config_;
}

// ---------------------------------------------------------------------------
// Aggregation helper
// ---------------------------------------------------------------------------

BatchEvaluationResult BatchEvaluator::aggregateResults(
    const std::vector<EvaluationResult>& results,
    std::chrono::milliseconds total_time) {
    BatchEvaluationResult out;
    out.results     = results;
    out.total_time  = total_time;

    if (results.empty()) {
        return out;
    }

    double sum_faith = 0.0, sum_rel = 0.0, sum_comp = 0.0;
    double sum_coh = 0.0, sum_overall = 0.0;
    size_t passed = 0, failed_q = 0;

    for (const auto& r : results) {
        sum_faith   += r.faithfulness_score;
        sum_rel     += r.relevance_score;
        sum_comp    += r.completeness_score;
        sum_coh     += r.coherence_score;
        sum_overall += r.overall_score;

        if (r.passed_quality_threshold) ++passed;
        else ++failed_q;
    }

    const double n = static_cast<double>(results.size());
    out.average_faithfulness  = sum_faith   / n;
    out.average_relevance     = sum_rel     / n;
    out.average_completeness  = sum_comp    / n;
    out.average_coherence     = sum_coh     / n;
    out.average_overall_score = sum_overall / n;
    out.passed_quality_threshold = passed;
    out.failed_quality_threshold = failed_q;

    out.progress.total_items     = results.size();
    out.progress.completed_items = results.size();
    out.progress.failed_items    = total_failed_.load();
    out.progress.progress_percentage = 100.0;
    out.progress.elapsed_time    = total_time;

    THEMIS_INFO("BatchEvaluator: {} items, avg_overall={:.3f}, passed={}, failed={}",
                results.size(), out.average_overall_score, passed, failed_q);

    return out;
}

} // namespace themis::rag::judge
