/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingestion_toolbox.cpp                              ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-15                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "toolbox/ingestion_toolbox.h"
#include "ingestion/builtin_step_factories.h"

#include <mutex>
#include <stdexcept>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// Impl — private implementation
// ─────────────────────────────────────────────────────────────────────────────

class IngestionToolbox::Impl {
public:
    Impl()
        : workflow_engine_(std::make_shared<ingestion::WorkflowEngine>())
        , text_backend_(std::make_shared<ingestion::NullTextGenerationBackend>())
    {}

    std::shared_ptr<ingestion::WorkflowEngine>         workflow_engine_;
    std::shared_ptr<ingestion::ITextGenerationBackend> text_backend_;
    mutable std::mutex                                 mutex_;
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

    reg.registerStep("builtin.ner_de",
        ingestion::builtin::createNerDeStep(toolbox->textBackend()));

    reg.registerStep("builtin.llm_extract",
        ingestion::builtin::createLlmExtractStep(toolbox->textBackend()));

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

std::vector<ingestion::BaseEntity> IngestionToolbox::extractEntities(
    const std::string& text,
    const std::string& mime,
    const std::string& filename)
{
    if (text.empty()) {
        return {};
    }

    // Build a minimal ExtractionContext from the supplied text
    ingestion::ExtractionContext ctx;
    ctx.manifest.detected_mime  = mime;
    ctx.manifest.filename_stem  = filename;
    ctx.manifest.extension      = "";
    ctx.raw_text                = text;

    auto engine = workflowEngine();
    auto result = engine->execute(ctx);
    if (!result) {
        // Failure is non-fatal; callers can still function with an empty set
        return {};
    }

    return result.value().nodes;
}

} // namespace toolbox
} // namespace themis
