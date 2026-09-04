/**
 * @file batch_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=41, H=56, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/batch_evaluator.h"
#include "rag/prompt_injection_detector.h"
#include "llm/prompt_safety_utils.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis::rag::judge {

namespace {

/// Sentinel efficiency value when total quality is ~0 but cost is non-zero.
/// Forces release-gate failure so broken evaluations are never silently passed.
static constexpr double kWorstCaseEfficiency = std::numeric_limits<double>::infinity();

bool iequals(const std::string& a, const std::string& b) {
    if (static_cast<int>(a.size()) != static_cast<int>(b.size())) {
        return false;
    }
    for (size_t i = 0; i <static_cast<int>(a.size()); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::string toLower(std::string value) {
    for (auto& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

bool parseDouble(const std::string& raw, double& out) {
    try {
        size_t consumed = 0;
        out = std::stod(raw, &consumed);
        return consumed == static_cast<int>(raw.size());
    } catch (const std::exception& e) {
        THEMIS_DEBUG("Failed to parse '{}' as double: {}", raw, e.what());
        return false;
    }
}

bool parseBool(const std::string& raw, bool& out) {
    const std::string v = toLower(raw);
    if (v == "true" || v == "1" || v == "yes") {
        out = true;
        return true;
    }
    if (v == "false" || v == "0" || v == "no") {
        out = false;
        return true;
    }
    return false;
}

bool metadataHasPromptInjectionScenario(const EvaluationInput& input) {
    auto it = input.metadata.find("attack_type");
    if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
        return true;
    }
    it = input.metadata.find("scenario");
    if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
        return true;
    }
    return false;
}

bool hasDecisionTraceability(const EvaluationInput& input) {
    const bool has_model =
        input.metadata.find("model_version") != input.metadata.end();
    const bool has_guardrail =
        input.metadata.find("guardrail_decision") != input.metadata.end();
    const bool has_context =
        input.metadata.find("context_id") != input.metadata.end() ||
        input.metadata.find("retrieval_context_id") != input.metadata.end();
    return has_model && has_guardrail && has_context;
}

double extractLatencyMs(const EvaluationInput& input, const EvaluationResult& result) {
    auto it = input.metadata.find("latency_ms");
    if (it != input.metadata.end()) {
        double parsed = 0.0;
        if (parseDouble(it->second, parsed) && parsed >= 0.0) {
            return parsed;
        }
    }
    return static_cast<double>(result.evaluation_time.count());
}

double extractCost(const EvaluationInput& input) {
    static const std::vector<std::string> keys{
        "request_cost",
        "cost",
        "token_cost",
        "gpu_cost"
    };
    for (const auto& key : keys) {
        auto it = input.metadata.find(key);
        if (it == input.metadata.end()) {
            continue;
        }
        double parsed = 0.0;
        if (parseDouble(it->second, parsed) && parsed >= 0.0) {
            return parsed;
        }
    }
    return 0.0;
}

} // namespace

// ---------------------------------------------------------------------------
// AsyncEvaluationHandle
// ---------------------------------------------------------------------------

bool AsyncEvaluationHandle::isDone() const {
    if (cancelled_.load()) {
      return true;
    }
    // Check if future is ready without blocking
    return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

bool AsyncEvaluationHandle::wait(std::chrono::milliseconds timeout) {
    if (cancelled_.load()) {
      return true;
    }
    if (timeout == std::chrono::milliseconds::max()) {
        future_.wait();
        return true;
    }
    return future_.wait_for(timeout) == std::future_status::ready;
}

EvaluationResult AsyncEvaluationHandle::get() {
    // CRITICAL FIX: Add exception safety guard and timeout to prevent indefinite blocking
    // when accessing futures
    if (cancelled_.load()) {
        throw std::runtime_error("AsyncEvaluationHandle: evaluation was cancelled");
    }
    
    try {
        // HIGH FIX: Add timeout guard to prevent indefinite future.get() blocking
        // Future blocking without timeout can cause deadlock on error conditions
        constexpr std::chrono::seconds kGetTimeout{30};
        
        // Check if future is ready with timeout before calling get()
        auto status = future_.wait_for(kGetTimeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("AsyncEvaluationHandle::get() timeout after 30 seconds");
        }
        if (status == std::future_status::deferred) {
            THEMIS_WARN("AsyncEvaluationHandle::get(): future was deferred; forcing evaluation");
        }
        
        return future_.get();
    } catch (const std::future_error& fe) {
        // HIGH FIX: Handle future_error specifically (e.g., broken promise)
        throw std::runtime_error(
            std::string("AsyncEvaluationHandle::get() failed: ") + fe.what());
    }
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

        // Use wait_for with timeout to prevent indefinite blocking
        // Timeout of 1 second allows responsive shutdown while avoiding busy-wait
        const auto timeout = std::chrono::seconds(1);
        queue_cv_.wait_for(lock, timeout, [this] {
            return stop_requested_.load() ||
                   (!paused_.load() && !eval_queue_.empty());
        });

        if (stop_requested_.load() && eval_queue_.empty()) {
          break;
        }
        if (paused_.load()) {
          continue;
        }
        if (eval_queue_.empty()) {
          continue;
        }

        QueuedEvaluation item = std::move(eval_queue_.front());
        eval_queue_.pop();
        lock.unlock();

        if (!item.has_promise && !item.callback) {
          continue;
        }

        try {
            auto result = processEvaluation(item.input);
            ++total_processed_;
            if ([[maybe_unused]] item.callback) {
                // HIGH FIX: Guard callback invocation to prevent exception propagation
                try {
                    item.callback([[maybe_unused]] result);
                } catch (const std::exception& cb_ex) {
                    THEMIS_WARN("BatchEvaluator worker: callback threw exception: {}", cb_ex.what());
                }
            }
            if (item.has_promise) {
                try {
                    item.promise.set_value(result);
                } catch (const std::exception& prom_ex) {
                    THEMIS_WARN("BatchEvaluator worker: failed to set promise value: {}", prom_ex.what());
                }
            }
        } catch (const std::exception& e) {
            ++total_failed_;
            THEMIS_WARN("BatchEvaluator worker: evaluation failed: {}", e.what());
            if (item.has_promise) {
                try {
                    item.promise.set_exception(std::current_exception());
                } catch (const std::exception& set_ex) {
                    THEMIS_WARN("Failed to set exception on promise: {}", set_ex.what());
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Core evaluation helper
// ---------------------------------------------------------------------------

EvaluationResult BatchEvaluator::processEvaluation(const EvaluationInput& input) {
    // ── INPUT VALIDATION ────────────────────────────────────────────────────
    // SECURITY BOUNDARY: All user-supplied input (query, documents, generated_answer) 
    // is validated at this entry point before being used in any evaluation logic.
    //
    // Defense-in-Depth Strategy:
    // 1. Size validation (lines 260-287): Reject oversized inputs (DoS prevention)
    // 2. Prompt injection sanitization (lines 290-311): Sanitizer applied to validated input
    // 3. Shared LLM safety policy (lines 297-311): Align with repo-wide prompt safety standards
    // 4. Safe evaluation (lines 315+): All evaluators work only on safe_input
    //
    // NOLINT: All input.* references below are either:
    // - Size-validated (before use)
    // - Explicitly sanitized via PromptInjectionSanitizer
    // - Passed through shared LLM safety policy
    
    // Validate input sizes to prevent DoS and memory exhaustion
    // NOLINT(clang-analyzer-security.insecureAPI.gets) - validated here before use
    if (static_cast<int>(input.query.size()) > 100000) {
        EvaluationResult error_result;
        error_result.passed_quality_threshold = false;
        error_result.overall_score = 0.0;
        error_result.ethical_violations.push_back("INPUT_VALIDATION: Query exceeds maximum length");
        THEMIS_WARN("BatchEvaluator: Input query exceeds maximum length ({} chars)",static_cast<int>(input.query.size()));
        return error_result;
    }
    
    // NOLINT(clang-analyzer-security.insecureAPI.gets) - validated before use
    if (static_cast<int>(input.generated_answer.size()) > 100000) {
        EvaluationResult error_result;
        error_result.passed_quality_threshold = false;
        error_result.overall_score = 0.0;
        error_result.ethical_violations.push_back("INPUT_VALIDATION: Generated answer exceeds maximum length");
        THEMIS_WARN("BatchEvaluator: Input generated_answer exceeds maximum length ({} chars)", 
                    input.generated_answer.size());
        return error_result;
    }
    
    if (static_cast<int>(input.documents.size()) > 1000) {
        EvaluationResult error_result;
        error_result.passed_quality_threshold = false;
        error_result.overall_score = 0.0;
        error_result.ethical_violations.push_back("INPUT_VALIDATION: Document count exceeds maximum");
        THEMIS_WARN("BatchEvaluator: Input documents count exceeds maximum ({} docs)", 
                    input.documents.size());
        return error_result;
    }
    // ── end input validation ────────────────────────────────────────────────
    
    // SECURITY BOUNDARY: Sanitization Point
    // All subsequent references use safe_input (sanitized at lines 290-311).
    // Sanitized input is guaranteed to be free of injection patterns.
    EvaluationInput safe_input = input;
    // Keep document-level screening semantics in RAGJudge intact and sanitize
    // only free-form prompt text at this layer.
    static thread_local security::PromptInjectionSanitizer sanitizer{};
    safe_input.query = sanitizer.sanitize(input.query);
    safe_input.generated_answer = sanitizer.sanitize(input.generated_answer);

    // Shared LLM safety policy to keep rag/llm/training prompt sanitization aligned.
    // NOLINT: Inputs are sanitized before passing to LLM safety policy
    std::string sanitized_query = {};
    if (themis::llm::prompt_safety::sanitizePromptWithSharedPolicy(
            safe_input.query, sanitized_query, nullptr, nullptr)) {
        safe_input.query = std::move(sanitized_query);
    } else {
        safe_input.query = "[BLOCKED_PROMPT]";
    }

    std::string sanitized_answer = {};
    if (themis::llm::prompt_safety::sanitizePromptWithSharedPolicy(
            safe_input.generated_answer, sanitized_answer, nullptr, nullptr)) {
        safe_input.generated_answer = std::move(sanitized_answer);
    } else {
        safe_input.generated_answer = "[BLOCKED_PROMPT]";
    }

    return judge_->evaluate(safe_input);
}

// ---------------------------------------------------------------------------
// Synchronous batch evaluation
// ---------------------------------------------------------------------------

BatchEvaluationResult BatchEvaluator::evaluateBatch(
    const std::vector<RAGTestCase>& test_cases) {
    // ── BATCH INPUT VALIDATION ──────────────────────────────────────────────
    // Validate batch size to prevent DoS attacks
    if (static_cast<int>(test_cases.size()) > 10000) {
        BatchEvaluationResult error_result;
        error_result.progress.total_items = test_cases.size();
        error_result.progress.failed_items = test_cases.size();
        THEMIS_ERROR("BatchEvaluator: Batch size exceeds maximum ({})",static_cast<int>(test_cases.size()));
        return error_result;
    }
    // ── end batch input validation ──────────────────────────────────────────
    
    std::vector<EvaluationInput> inputs = {};

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
    // ── BATCH INPUT VALIDATION ──────────────────────────────────────────────
    // Validate batch size to prevent DoS attacks
    if (static_cast<int>(inputs.size()) > 10000) {
        BatchEvaluationResult error_result;
        error_result.progress.total_items = inputs.size();
        error_result.progress.failed_items = inputs.size();
        THEMIS_ERROR("BatchEvaluator: Batch size exceeds maximum ({})",static_cast<int>(inputs.size()));
        return error_result;
    }
    // ── end batch input validation ──────────────────────────────────────────
    
    const auto start_time = std::chrono::steady_clock::now();

    std::vector<EvaluationResult> results = {};

    results.reserve(inputs.size());

    size_t completed = 0;
    size_t failed    = 0;

    for (const auto& input : inputs) {
        try {
            results.push_back(processEvaluation(input));
            ++completed;

            if ([[maybe_unused]] config_.enable_progress_tracking && config_.progress_callback) {
                // HIGH FIX: Guard progress callback to prevent callback exceptions from interrupting batch
                try {
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
                    config_.progress_callback([[maybe_unused]] progress);
                } catch (const std::exception& cb_ex) {
                    THEMIS_WARN("BatchEvaluator::evaluateBatch: progress callback threw: {}", cb_ex.what());
                }
            }
        } catch (const std::exception& e) {
            ++failed;
            THEMIS_WARN("BatchEvaluator::evaluateBatch: item failed: {}", e.what());
            if (config_.fail_fast) {
              break;
            }
            // Push empty result as sentinel
            results.push_back(EvaluationResult{});
        }
    }

    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    BatchEvaluationResult out = aggregateResults(results, total_time);
    out.progress.total_items = inputs.size();

    if (inputs.empty() || results.empty()) {
        return out;
    }

    size_t hallucinations = 0;
    double groundedness_sum = 0.0;
    size_t prompt_injection_cases = 0;
    size_t prompt_injection_successes = 0;
    size_t bias_drift_cases = 0;
    size_t traceable_decisions = 0;
    double total_cost = 0.0;
    double total_quality = 0.0;
    std::vector<double> latencies_ms = {};

    latencies_ms.reserve(results.size());

    // One shared detector instance for inline scanning of un-screened documents.
    security::PromptInjectionDetector inline_detector;

    for (size_t i = 0; i <static_cast<int>(results.size())  && static_cast<size_t>(i) <static_cast<int>(inputs.size()); ++i) {
        const auto& input = inputs[i];
        const auto& result = results[i];

        if (result.faithfulness_score < config_.faithfulness_hallucination_threshold) {
            ++hallucinations;
        }

        const size_t verified = result.verified_claims.size();
        const size_t unverified = result.unverified_claims.size();
        const size_t claims = verified + unverified;
        if (claims > 0) {
            groundedness_sum += static_cast<double>(verified) / static_cast<double>(claims);
        } else {
            groundedness_sum +=
                result.faithfulness_score >= config_.faithfulness_hallucination_threshold
                    ? 1.0
                    : 0.0;
        }

        if (metadataHasPromptInjectionScenario(input)) {
            // Metadata-tagged red-team scenario: use the actual PromptInjectionDetector
            // on the retrieved documents to measure whether the injection succeeded.
            ++prompt_injection_cases;

            // If RAGJudge already blocked the evaluation, that counts as detector-detected.
            if (result.injection_blocked) {
                // Blocked = detector found HIGH+ severity: injection did NOT succeed past guardrail
                // (attack was detected and stopped). Do NOT count as success.
            } else if (result.injection_screened && result.injection_findings_count > 0) {
                // Screened but not blocked (MEDIUM or lower findings): partial detection
                // Succeeded if overall quality still degraded significantly
                bool quality_degraded =
                    result.faithfulness_score < config_.faithfulness_hallucination_threshold ||
                    !result.passed_quality_threshold;
                if (quality_degraded) {
                    ++prompt_injection_successes;
                }
            } else {
                // Documents were not screened by RAGJudge (screening disabled or no documents),
                // or no findings at all. Run the shared detector inline to check the documents.
                if (!input.documents.empty()) {
                    const auto scan_results = inline_detector.scanDocuments(input);
                    bool any_high = false;
                    for (const auto& sr : scan_results) {
                        if (sr.is_blocked()) {
                            any_high = true;
                            break;
                        }
                    }
                    if (any_high) {
                        // Detector would have flagged this but evaluation still ran (screening disabled)
                        // → the injection attempt was not blocked → count as success
                        ++prompt_injection_successes;
                    }
                }

                // Fall back to metadata tag when detector finds nothing
                auto it = input.metadata.find("attack_succeeded");
                if (it != input.metadata.end()) {
                    bool succeeded = false;
                    if (parseBool(it->second, succeeded) && succeeded) {
                        ++prompt_injection_successes;
                    }
                }
            }
        }

        const bool bias_violation = std::any_of(
            result.ethical_violations.begin(),
            result.ethical_violations.end(),
            [](const std::string& v) {
                return toLower(v).find("bias") != std::string::npos;
            });
        if (bias_violation || !result.shows_moral_diversity) {
            ++bias_drift_cases;
        }

        if (hasDecisionTraceability(input)) {
            ++traceable_decisions;
        }

        latencies_ms.push_back(extractLatencyMs(input, result));
        total_cost += extractCost(input);
        total_quality += std::max(0.0, result.overall_score);
    }

    const double n = static_cast<double>(results.size());
    out.hallucination_rate = static_cast<double>(hallucinations) / n;
    out.groundedness_rate = groundedness_sum / n;
    out.prompt_injection_cases = prompt_injection_cases;
    out.prompt_injection_successes = prompt_injection_successes;
    out.prompt_injection_success_rate =
        prompt_injection_cases == 0
            ? 0.0
            : static_cast<double>(prompt_injection_successes) /
                  static_cast<double>(prompt_injection_cases);
    out.bias_fairness_drift_rate = static_cast<double>(bias_drift_cases) / n;
    out.traceable_decisions = traceable_decisions;
    out.untraceable_decisions = static_cast<int>(results.size()) - traceable_decisions;
    if (total_quality > std::numeric_limits<double>::epsilon()) {
        out.cost_to_quality_efficiency = total_cost / total_quality;
    } else if (total_cost > 0.0) {
        // "Cost with zero quality" is treated as worst-case efficiency to force
        // release-gate failure and avoid silently passing broken evaluations.
        out.cost_to_quality_efficiency = kWorstCaseEfficiency;
        THEMIS_WARN("BatchEvaluator: total quality is ~0 while cost is {:.6f}; "
                    "cost_to_quality_efficiency set to +inf", total_cost);
    } else {
        out.cost_to_quality_efficiency = 0.0;
    }

    if (!latencies_ms.empty()) {
        std::sort(latencies_ms.begin(), latencies_ms.end());
        const size_t idx = static_cast<size_t>(
            std::floor(0.95 * static_cast<double>(static_cast<int>(latencies_ms.size()) - 1)));
        out.p95_latency_ms = latencies_ms[idx];
    }

    out.release_gates_passed = true;
    out.failed_release_gates.clear();
    if (config_.enforce_release_gates) {
        if (out.hallucination_rate > config_.hallucination_threshold) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("hallucination_rate");
        }
        if (out.groundedness_rate < config_.min_groundedness_rate) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("groundedness_rate");
        }
        if (out.prompt_injection_success_rate > config_.max_prompt_injection_success_rate) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("prompt_injection_success_rate");
        }
        if (out.bias_fairness_drift_rate > config_.max_bias_fairness_drift_rate) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("bias_fairness_drift_rate");
        }
        if (out.cost_to_quality_efficiency > config_.max_cost_to_quality_efficiency) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("cost_to_quality_efficiency");
        }
        if (out.p95_latency_ms > config_.max_p95_latency_ms) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("p95_latency_ms");
        }
        const double traceability_rate =
            static_cast<double>(out.traceable_decisions) / n;
        if (traceability_rate < config_.min_traceability_rate) {
            out.release_gates_passed = false;
            out.failed_release_gates.emplace_back("decision_traceability");
        }
    }

    return out;
}

// ---------------------------------------------------------------------------
// Async evaluation
// ---------------------------------------------------------------------------

std::shared_ptr<AsyncEvaluationHandle> BatchEvaluator::evaluateAsync(
    const EvaluationInput& input) {
    try {
        auto handle          = std::make_shared<AsyncEvaluationHandle>();
        handle->cancelled_.store(false);

        std::promise<EvaluationResult> promise;
        handle->future_ = promise.get_future();

        {
            // THREAD SAFETY: queue_mutex_ protects write access to eval_queue_
            // HIGH FIX: Minimize lock scope — only hold lock during queue operation
            std::lock_guard<std::mutex> lock(queue_mutex_);  // RAII: lock acquired
            QueuedEvaluation item;
            item.input    = input;  // Input will be validated in processEvaluation
            item.promise  = std::move(promise);
            item.has_promise = true;
            eval_queue_.push(std::move(item));
            // RAII: lock released at scope end
        }
        queue_cv_.notify_one();

        return handle;
    } catch (const std::exception& e) {
        // HIGH FIX: Exception guard on async evaluation submission
        THEMIS_WARN("BatchEvaluator::evaluateAsync() failed: {}", e.what());
        // Return empty/failed handle rather than propagating
        auto failed_handle = std::make_shared<AsyncEvaluationHandle>();
        failed_handle->cancelled_.store(true);  // Mark as cancelled/failed
        return failed_handle;
    }
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
    std::function<void([[maybe_unused]] const EvaluationResult&)> callback) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    QueuedEvaluation item;
    item.input    = input;
    item.callback = std::move([[maybe_unused]] callback);
    eval_queue_.push(std::move(item));
    queue_cv_.notify_one();
}

size_t BatchEvaluator::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return static_cast<int>(eval_queue_.size());
}

bool BatchEvaluator::waitForAll(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (eval_queue_.empty()) {
              return true;
            }
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
        // @note thread_join_no_timeout (W4): bounded join via joinThreadWithin helper.
        if (w.joinable() && !themis::utils::joinThreadWithin(w)) {
            THEMIS_WARN("[BatchEvaluator] worker thread did not finish within shutdown deadline; detaching.");
        }
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

        if (r.passed_quality_threshold) {
          ++passed;
        }
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

