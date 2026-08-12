/**
 * @file llm_metrics_collector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=8, H=13, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/llm_metrics_collector.h"

#include <spdlog/spdlog.h>

namespace themis {
namespace aql {

LLMMetricsCollector::LLMMetricsCollector() : exporter_(std::make_shared<llm::monitoring::PrometheusExporter>()) {}

void LLMMetricsCollector::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }

    registerMetrics();
    initialized_ = true;
    spdlog::info("LLM metrics collector initialized");
}

void LLMMetricsCollector::registerMetrics() {
    using namespace llm::monitoring;

    // Latency histogram
    exporter_->registerMetric({"llm_operation_latency_ms",
                               "Latency of LLM operations in milliseconds",
                               PrometheusExporter::MetricType::HISTOGRAM,
                               {"operation", "model", "lora"}});

    // Token throughput counters
    exporter_->registerMetric({
        "llm_tokens_processed_total",
        "Total number of tokens processed",
        PrometheusExporter::MetricType::COUNTER,
        {"operation", "model", "direction"} // direction: input/output
    });

    // Error counters
    exporter_->registerMetric({"llm_errors_total",
                               "Total number of errors by type",
                               PrometheusExporter::MetricType::COUNTER,
                               {"operation", "model", "error_code"}});

    // Success/failure counters
    exporter_->registerMetric({
        "llm_operations_total",
        "Total number of operations",
        PrometheusExporter::MetricType::COUNTER,
        {"operation", "model", "status"} // status: success/failure
    });

    // Cache hit rate
    exporter_->registerMetric({
        "llm_cache_access_total",
        "Total cache accesses",
        PrometheusExporter::MetricType::COUNTER,
        {"cache_type", "result"} // result: hit/miss
    });

    // Model memory usage
    exporter_->registerMetric({"llm_model_memory_bytes",
                               "Memory usage per model in bytes",
                               PrometheusExporter::MetricType::GAUGE,
                               {"model"}});

    // Circuit breaker state
    exporter_->registerMetric({"llm_circuit_breaker_state",
                               "Circuit breaker state (0=closed, 1=open, 2=half_open)",
                               PrometheusExporter::MetricType::GAUGE,
                               {"operation"}});

    // RAG-specific metrics
    exporter_->registerMetric({"llm_rag_documents_retrieved",
                               "Number of documents retrieved for RAG",
                               PrometheusExporter::MetricType::HISTOGRAM,
                               {"collection", "lora"}});
}

void LLMMetricsCollector::recordInference(const std::string &model_id, const std::string &lora_id,
                                          std::chrono::milliseconds latency, size_t input_tokens, size_t output_tokens,
                                          bool success, const std::string &error_code) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Record latency
    exporter_->observeHistogram("llm_operation_latency_ms", static_cast<double>(latency.count()),
                                {{"operation", "infer"}, {"model", model_id}, {"lora", lora_id}});

    // Record token counts
    exporter_->incrementCounter("llm_tokens_processed_total",
                                {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
                                static_cast<double>(input_tokens));

    exporter_->incrementCounter("llm_tokens_processed_total",
                                {{"operation", "infer"}, {"model", model_id}, {"direction", "output"}},
                                static_cast<double>(output_tokens));

    // Record operation status
    exporter_->incrementCounter(
        "llm_operations_total",
        {{"operation", "infer"}, {"model", model_id}, {"status", success ? "success" : "failure"}});

    // Record error if applicable
    if (!success && !error_code.empty()) {
        exporter_->incrementCounter("llm_errors_total",
                                    {{"operation", "infer"}, {"model", model_id}, {"error_code", error_code}});
    }
}

void LLMMetricsCollector::recordRAG(const std::string &collection, const std::string &lora_id,
                                    std::chrono::milliseconds latency, size_t retrieved_docs, size_t input_tokens,
                                    size_t output_tokens, bool success, const std::string &error_code) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Record latency
    exporter_->observeHistogram("llm_operation_latency_ms", static_cast<double>(latency.count()),
                                {{"operation", "rag"}, {"model", collection}, {"lora", lora_id}});

    // Record retrieved documents
    exporter_->observeHistogram("llm_rag_documents_retrieved", static_cast<double>(retrieved_docs),
                                {{"collection", collection}, {"lora", lora_id}});

    // Record token counts
    exporter_->incrementCounter("llm_tokens_processed_total",
                                {{"operation", "rag"}, {"model", collection}, {"direction", "input"}},
                                static_cast<double>(input_tokens));

    exporter_->incrementCounter("llm_tokens_processed_total",
                                {{"operation", "rag"}, {"model", collection}, {"direction", "output"}},
                                static_cast<double>(output_tokens));

    // Record operation status
    exporter_->incrementCounter(
        "llm_operations_total",
        {{"operation", "rag"}, {"model", collection}, {"status", success ? "success" : "failure"}});

    // Record error if applicable
    if (!success && !error_code.empty()) {
        exporter_->incrementCounter("llm_errors_total",
                                    {{"operation", "rag"}, {"model", collection}, {"error_code", error_code}});
    }
}

void LLMMetricsCollector::recordEmbedding(const std::string &model_id, std::chrono::milliseconds latency,
                                          size_t input_tokens, bool success, const std::string &error_code) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Record latency
    exporter_->observeHistogram("llm_operation_latency_ms", static_cast<double>(latency.count()),
                                {{"operation", "embed"}, {"model", model_id}, {"lora", ""}});

    // Record token count
    exporter_->incrementCounter("llm_tokens_processed_total",
                                {{"operation", "embed"}, {"model", model_id}, {"direction", "input"}},
                                static_cast<double>(input_tokens));

    // Record operation status
    exporter_->incrementCounter(
        "llm_operations_total",
        {{"operation", "embed"}, {"model", model_id}, {"status", success ? "success" : "failure"}});

    // Record error if applicable
    if (!success && !error_code.empty()) {
        exporter_->incrementCounter("llm_errors_total",
                                    {{"operation", "embed"}, {"model", model_id}, {"error_code", error_code}});
    }
}

void LLMMetricsCollector::recordCacheAccess(const std::string &cache_type, bool hit) {
    std::lock_guard<std::mutex> lock(mutex_);

    exporter_->incrementCounter("llm_cache_access_total",
                                {{"cache_type", cache_type}, {"result", hit ? "hit" : "miss"}});
}

void LLMMetricsCollector::updateModelMemory(const std::string &model_id, size_t memory_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    exporter_->setGauge("llm_model_memory_bytes", static_cast<double>(memory_bytes), {{"model", model_id}});
}

void LLMMetricsCollector::recordCircuitBreakerState(const std::string &operation, const std::string &state) {
    std::lock_guard<std::mutex> lock(mutex_);

    double state_value = 0.0;
    if (state == "closed") {
        state_value = 0.0;
    } else if (state == "open") {
        state_value = 1.0;
    } else if (state == "half_open") {
        state_value = 2.0;
    }

    exporter_->setGauge("llm_circuit_breaker_state", state_value, {{"operation", operation}});
}

void LLMMetricsCollector::recordAQLValidation(
    bool success,
    std::chrono::milliseconds duration,
    const std::string& error_reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Record validation latency
    exporter_->observeHistogram(
        "aql_validation_duration_seconds",
        duration.count() / 1000.0,
        {{"status", success ? "success" : "failed"}});

    // Record validation result
    std::string status = success ? "success" : "parse_error";
    if (!error_reason.empty()) {
        status = error_reason;  // Use specific error reason if provided
    }

    exporter_->incrementCounter(
        "aql_validation_total",
        {{"status", status}});
}

void LLMMetricsCollector::recordAQLGenerationAttempt(
    bool success,
    int attempt_number,
    std::chrono::milliseconds duration,
    const std::string& outcome) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Record generation latency
    exporter_->observeHistogram(
        "aql_generation_duration_seconds",
        duration.count() / 1000.0,
        {{"attempt", std::to_string(attempt_number)},
         {"status", success ? "success" : "failed"}});

    // Record generation attempt
    std::string result_status = success ? "success" : "parse_error";
    if (!outcome.empty()) {
        result_status = outcome;  // Use specific outcome if provided
    }

    exporter_->incrementCounter(
        "aql_generation_attempts_total",
        {{"attempt_number", std::to_string(attempt_number)},
         {"status", result_status}});

    // Track retries
    if (!success && attempt_number > 1) {
        exporter_->incrementCounter(
            "aql_generation_retries_total",
            {{"attempt", std::to_string(attempt_number)},
             {"outcome", result_status}});
    }
}

void LLMMetricsCollector::recordValidationRetry(
    bool retry_succeeded,
    int attempt_number) {
    std::lock_guard<std::mutex> lock(mutex_);

    exporter_->incrementCounter(
        "aql_validation_retries_total",
        {{"attempt", std::to_string(attempt_number)},
         {"outcome", retry_succeeded ? "success" : "failed"}});
}

LLMMetricsCollector &LLMMetricsCollector::instance() {
    static LLMMetricsCollector instance;
    return instance;
}

std::unordered_map<std::string, std::string>
LLMMetricsCollector::makeLabels(const std::string &operation, const std::string &model, const std::string &status) {
    std::unordered_map<std::string, std::string> labels;
    labels["operation"] = operation;
    if (!model.empty()) {
        labels["model"] = model;
    }
    if (!status.empty()) {
        labels["status"] = status;
    }
    return labels;
}

} // namespace aql
} // namespace themis
