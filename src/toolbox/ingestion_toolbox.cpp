/**
 * @file ingestion_toolbox.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/ingestion_toolbox.h"
#include "ingestion/builtin_step_factories.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Impl — private implementation
// ─────────────────────────────────────────────────────────────────────────────

/** @brief Impl — private implementation. */
class IngestionToolbox::Impl {
public:
    Impl()
        : workflow_engine_(std::make_shared<ingestion::WorkflowEngine>())
        , text_backend_(std::make_shared<ingestion::NullTextGenerationBackend>())
        , extract_calls_total_(0)
        , extract_errors_total_(0)
        , extract_entities_total_(0)
        , extract_latency_ms_total_(0)
        , extract_empty_results_(0)
        , extract_failures_total_(0)
        , extract_latency_us_bucket_0_100_(0)     ///< 0-100 microseconds
        , extract_latency_us_bucket_100_1000_(0)  ///< 100-1000 microseconds
        , extract_latency_us_bucket_1000_10000_(0) ///< 1-10 milliseconds
        , extract_latency_us_bucket_10000_plus_(0) ///< 10+ milliseconds
    {}

    std::shared_ptr<ingestion::WorkflowEngine>         workflow_engine_;
    std::shared_ptr<ingestion::ITextGenerationBackend> text_backend_;
    mutable std::mutex                                 mutex_;

    // Prometheus counters — lock-free
    std::atomic<uint64_t> extract_calls_total_;
    std::atomic<uint64_t> extract_errors_total_;
    std::atomic<uint64_t> extract_entities_total_;
    std::atomic<uint64_t> extract_latency_ms_total_;
    std::atomic<uint64_t> extract_empty_results_;  ///< Tracks calls with no entities extracted

    // Phase 3: Extended metrics for error handling and diagnostics
    std::atomic<uint64_t> extract_failures_total_;      ///< Total extraction failures (counter)
    std::atomic<uint64_t> extract_latency_us_bucket_0_100_;    ///< Histogram bucket: 0-100 us
    std::atomic<uint64_t> extract_latency_us_bucket_100_1000_; ///< Histogram bucket: 100-1000 us
    std::atomic<uint64_t> extract_latency_us_bucket_1000_10000_;///< Histogram bucket: 1-10 ms
    std::atomic<uint64_t> extract_latency_us_bucket_10000_plus_; ///< Histogram bucket: 10+ ms
};

// ─────────────────────────────────────────────────────────────────────────────
// IngestionToolbox public API
// ─────────────────────────────────────────────────────────────────────────────

IngestionToolbox::IngestionToolbox()
    : impl_(std::make_unique<Impl>())
{}

IngestionToolbox::~IngestionToolbox() = default;

IngestionToolbox::IngestionToolbox(IngestionToolbox&&) noexcept = default;
IngestionToolbox& IngestionToolbox::operator=(IngestionToolbox&&) noexcept = default;

// ── Factory ──────────────────────────────────────────────────────────────────

std::shared_ptr<IngestionToolbox> IngestionToolbox::createDefault() {
    auto toolbox = std::make_shared<IngestionToolbox>();

    // Register all built-in steps into the engine's StepRegistry
    auto& reg = toolbox->stepRegistry();

    (void)reg.registerStep("builtin.ner_de",
        ingestion::builtin::createNerDeStep(toolbox->textBackend()));

    (void)reg.registerStep("builtin.llm_extract",
        ingestion::builtin::createLlmExtractStep(toolbox->textBackend()));

    // Register tensor pipeline steps with null backends.
    // The null backends produce graceful no-ops (skip_when_unavailable=true
    // default); callers that need real TT-cores should replace these steps
    // via stepRegistry().registerStep(...) with a live TensorIngestionBridge
    // and TensorCoreStorageBridge after construction.
    (void)reg.registerStep("builtin.chunk_tt_decompose",
        ingestion::builtin::createChunkTtDecomposeStep(nullptr));

    (void)reg.registerStep("builtin.tensor_core_bridge",
        ingestion::builtin::createTensorCoreBridgeStep(nullptr));

    return toolbox;
}

// ── Dependency injection ──────────────────────────────────────────────────────

void IngestionToolbox::setWorkflowEngine(
    std::shared_ptr<ingestion::WorkflowEngine> engine)
{
    if (!engine) {
        throw std::invalid_argument("IngestionToolbox::setWorkflowEngine: engine must not be null");
    }
    std::lock_guard<std::mutex> lk(impl_->mutex_);
    impl_->workflow_engine_ = std::move(engine);
}

void IngestionToolbox::setTextBackend(
    std::shared_ptr<ingestion::ITextGenerationBackend> backend)
{
    std::lock_guard<std::mutex> lk(impl_->mutex_);
    impl_->text_backend_ = backend
        ? std::move(backend)
        : std::make_shared<ingestion::NullTextGenerationBackend>();
}

// ── Accessors ────────────────────────────────────────────────────────────────

std::shared_ptr<ingestion::WorkflowEngine> IngestionToolbox::workflowEngine() const {
    std::lock_guard<std::mutex> lk(impl_->mutex_);
    return impl_->workflow_engine_;
}

ingestion::StepRegistry& IngestionToolbox::stepRegistry() {
    // WorkflowEngine owns the StepRegistry; no additional lock needed here
    // because WorkflowEngine::stepRegistry() is already thread-safe.
    return impl_->workflow_engine_->stepRegistry();
}

std::shared_ptr<ingestion::ITextGenerationBackend> IngestionToolbox::textBackend() const {
    std::lock_guard<std::mutex> lk(impl_->mutex_);
    return impl_->text_backend_;
}

// ── High-level convenience ────────────────────────────────────────────────────

// Shared helper: build an ExtractionContext and run the workflow.
// Returns the BaseEntitySet on success, empty set on failure.
static ingestion::BaseEntitySet runWorkflow(
    const std::shared_ptr<ingestion::WorkflowEngine>& engine,
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    ingestion::ExtractionContext ctx;
    ctx.manifest.detected_mime  = mime;
    ctx.manifest.filename_stem  = filename;
    ctx.manifest.extension      = "";
    ctx.raw_text                = text;

    auto result = engine->execute(ctx);
    if (!result) {
        return {};
    }
    return std::move(result.value());
}

// Minimum text length (bytes) for which a zero-entity result is considered
// an error rather than a valid "nothing to extract" outcome.  Texts shorter
// than this are trivially empty/whitespace and don't produce workflow output;
// the call is still recorded but not counted as an error.
static constexpr std::size_t kMinTextSizeForValidation = 8;

std::vector<ingestion::BaseEntity> IngestionToolbox::extractEntities(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    if (text.empty()) {
        return {};
    }

    auto t0 = std::chrono::steady_clock::now();
    auto engine = workflowEngine();
    auto entity_set = runWorkflow(engine, text, mime, filename);
    auto t1 = std::chrono::steady_clock::now();

    const uint64_t latency_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
    const bool success = !entity_set.nodes.empty()
                         || text.size() < kMinTextSizeForValidation;
    recordExtraction(entity_set.nodes.size(), latency_ms, success);

    return entity_set.nodes;
}

ingestion::BaseEntitySet IngestionToolbox::extractEntitySet(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    if (text.empty()) {
        return {};
    }

    auto t0 = std::chrono::steady_clock::now();
    auto engine = workflowEngine();
    auto entity_set = runWorkflow(engine, text, mime, filename);
    auto t1 = std::chrono::steady_clock::now();

    const uint64_t latency_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
    const bool success = !entity_set.nodes.empty() || !entity_set.chunks.empty()
                         || text.size() < kMinTextSizeForValidation;
    recordExtraction(entity_set.nodes.size() + entity_set.chunks.size(),
                     latency_ms, success);

    return entity_set;
}

// ── Prometheus metrics ────────────────────────────────────────────────────────

void IngestionToolbox::recordExtraction(std::size_t entity_count,
                                         uint64_t    latency_ms,
                                         bool        success) noexcept
{
    impl_->extract_calls_total_.fetch_add(1, std::memory_order_relaxed);
    if (!success) {
        impl_->extract_errors_total_.fetch_add(1, std::memory_order_relaxed);
        impl_->extract_failures_total_.fetch_add(1, std::memory_order_relaxed);
    }
    if (entity_count == 0) {
        impl_->extract_empty_results_.fetch_add(1, std::memory_order_relaxed);
    }
    impl_->extract_entities_total_.fetch_add(
        static_cast<uint64_t>(entity_count), std::memory_order_relaxed);
    impl_->extract_latency_ms_total_.fetch_add(latency_ms, std::memory_order_relaxed);

    // Phase 3: Populate latency histogram buckets (converting ms to us)
    const uint64_t latency_us = latency_ms * 1000;
    if (latency_us < 100) {
        impl_->extract_latency_us_bucket_0_100_.fetch_add(1, std::memory_order_relaxed);
    } else if (latency_us < 1000) {
        impl_->extract_latency_us_bucket_100_1000_.fetch_add(1, std::memory_order_relaxed);
    } else if (latency_us < 10000) {
        impl_->extract_latency_us_bucket_1000_10000_.fetch_add(1, std::memory_order_relaxed);
    } else {
        impl_->extract_latency_us_bucket_10000_plus_.fetch_add(1, std::memory_order_relaxed);
    }
}

std::string IngestionToolbox::getMetricsText() const {
    const uint64_t calls = impl_->extract_calls_total_.load();
    if (calls == 0) {
      return "";
    }

    const uint64_t errors         = impl_->extract_errors_total_.load();
    const uint64_t empty_results  = impl_->extract_empty_results_.load();
    const uint64_t entities       = impl_->extract_entities_total_.load();
    const uint64_t latency        = impl_->extract_latency_ms_total_.load();
    const uint64_t failures       = impl_->extract_failures_total_.load();
    const uint64_t bucket_0_100   = impl_->extract_latency_us_bucket_0_100_.load();
    const uint64_t bucket_100_1000 = impl_->extract_latency_us_bucket_100_1000_.load();
    const uint64_t bucket_1000_10000 = impl_->extract_latency_us_bucket_1000_10000_.load();
    const uint64_t bucket_10000_plus = impl_->extract_latency_us_bucket_10000_plus_.load();

    std::ostringstream out;

    out << "# HELP toolbox_extract_calls_total Total extractEntities() / extractEntitySet() calls.\n";
    out << "# TYPE toolbox_extract_calls_total counter\n";
    out << "toolbox_extract_calls_total " << calls << "\n";

    out << "# HELP toolbox_extract_errors_total Total extraction errors.\n";
    out << "# TYPE toolbox_extract_errors_total counter\n";
    out << "toolbox_extract_errors_total " << errors << "\n";

    out << "# HELP toolbox_extract_empty_results_total Extractions with zero entities.\n";
    out << "# TYPE toolbox_extract_empty_results_total counter\n";
    out << "toolbox_extract_empty_results_total " << empty_results << "\n";

    out << "# HELP toolbox_extract_entities_total Cumulative number of entities / chunks extracted.\n";
    out << "# TYPE toolbox_extract_entities_total counter\n";
    out << "toolbox_extract_entities_total " << entities << "\n";

    out << "# HELP toolbox_extract_latency_ms_total Cumulative latency (ms) for all extractions.\n";
    out << "# TYPE toolbox_extract_latency_ms_total counter\n";
    out << "toolbox_extract_latency_ms_total " << latency << "\n";

    // Phase 3: Export new metrics
    out << "# HELP toolbox_extraction_failures_total Total extraction failures.\n";
    out << "# TYPE toolbox_extraction_failures_total counter\n";
    out << "toolbox_extraction_failures_total " << failures << "\n";

    out << "# HELP toolbox_extraction_latency_us Histogram of extraction latency in microseconds.\n";
    out << "# TYPE toolbox_extraction_latency_us histogram\n";
    out << "toolbox_extraction_latency_us_bucket{le=\"100\"} " << bucket_0_100 << "\n";
    out << "toolbox_extraction_latency_us_bucket{le=\"1000\"} " 
        << (bucket_0_100 + bucket_100_1000) << "\n";
    out << "toolbox_extraction_latency_us_bucket{le=\"10000\"} " 
        << (bucket_0_100 + bucket_100_1000 + bucket_1000_10000) << "\n";
    out << "toolbox_extraction_latency_us_bucket{le=\"+Inf\"} " 
        << (bucket_0_100 + bucket_100_1000 + bucket_1000_10000 + bucket_10000_plus) << "\n";
    out << "toolbox_extraction_latency_us_count " << calls << "\n";
    out << "toolbox_extraction_latency_us_sum " << (latency * 1000) << "\n";

    return out.str();
}

} // namespace toolbox
} // namespace themis
