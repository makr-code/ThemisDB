/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            toolbox_builder.cpp                                ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-16                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "toolbox/toolbox_builder.h"
#include "ingestion/builtin_step_factories.h"
#include "utils/logger.h"

#include <stdexcept>
#include <algorithm>
#include <set>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxBuilder::Impl
// ─────────────────────────────────────────────────────────────────────────────

struct ToolboxBuilder::Impl {
    std::vector<std::string>                                     profile_paths;
    std::shared_ptr<ingestion::WorkflowEngine>                   engine;
    std::shared_ptr<ingestion::ITextGenerationBackend>           text_backend;
    std::shared_ptr<ingestion::IGraphWriter>                     graph_writer;
    std::vector<std::shared_ptr<ingestion::IFormatExtractor>>    format_extractors;
    bool                                                         built{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// ToolboxBuilder public API
// ─────────────────────────────────────────────────────────────────────────────

ToolboxBuilder::ToolboxBuilder()  : impl_(std::make_unique<Impl>()) {}
ToolboxBuilder::~ToolboxBuilder() = default;

ToolboxBuilder::ToolboxBuilder(ToolboxBuilder&&) noexcept = default;
ToolboxBuilder& ToolboxBuilder::operator=(ToolboxBuilder&&) noexcept = default;

ToolboxBuilder& ToolboxBuilder::withWorkflowProfile(std::string profile_path) {
    if (profile_path.empty()) {
        throw std::invalid_argument("ToolboxBuilder::withWorkflowProfile: path must not be empty");
    }
    impl_->profile_paths.push_back(std::move(profile_path));
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withGraphWriter(
    std::shared_ptr<ingestion::IGraphWriter> writer)
{
    impl_->graph_writer = std::move(writer);
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withTextBackend(
    std::shared_ptr<ingestion::ITextGenerationBackend> backend)
{
    impl_->text_backend = std::move(backend);
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withWorkflowEngine(
    std::shared_ptr<ingestion::WorkflowEngine> engine)
{
    if (!engine) {
        throw std::invalid_argument("ToolboxBuilder::withWorkflowEngine: engine must not be null");
    }
    impl_->engine = std::move(engine);
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withFormatExtractor(
    std::shared_ptr<ingestion::IFormatExtractor> extractor)
{
    if (!extractor) {
        throw std::invalid_argument("ToolboxBuilder::withFormatExtractor: extractor must not be null");
    }
    impl_->format_extractors.push_back(std::move(extractor));
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withFormatExtractorFactory(
    std::shared_ptr<ingestion::IFormatExtractorFactory> factory)
{
    if (!factory) {
        throw std::invalid_argument("ToolboxBuilder::withFormatExtractorFactory: factory must not be null");
    }

    // Collect one extractor per MIME type, de-duplicating by pointer identity
    std::set<ingestion::IFormatExtractor*> seen;
    for (const auto& mime : factory->registeredMimeTypes()) {
        auto ext = factory->extractorFor(mime);
        if (ext && seen.find(ext.get()) == seen.end()) {
            seen.insert(ext.get());
            impl_->format_extractors.push_back(std::move(ext));
        }
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// build()
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IngestionToolbox> ToolboxBuilder::build() {
    if (impl_->built) {
        throw std::logic_error("ToolboxBuilder::build() called more than once");
    }
    impl_->built = true;

    // ── 1. Create toolbox (uses createDefault() which registers NER + LLM steps)
    auto toolbox = IngestionToolbox::createDefault();

    // ── 2. Inject custom WorkflowEngine if provided
    if (impl_->engine) {
        toolbox->setWorkflowEngine(impl_->engine);
    }

    // ── 3. Register format-extractor backed builtin steps
    auto& reg = toolbox->stepRegistry();

    // Helper: determine which category an extractor belongs to based on its
    // supportedMimeTypes() and register the correct builtin step.
    auto registerFormatStep = [&](std::shared_ptr<ingestion::IFormatExtractor> ext) {
        const auto& mimes = ext->supportedMimeTypes();
        if (mimes.empty()) return;

        // Select step type based on the first supported MIME type
        const std::string& primary = mimes.front();

        std::shared_ptr<ingestion::IIngestionStep> step;

        if (primary == "application/pdf") {
            step = ingestion::builtin::createParsePdfStep(ext);
            reg.registerStep("builtin.parse_pdf", step);
        } else if (primary.find("vnd.openxmlformats") != std::string::npos ||
                   primary == "application/msword" ||
                   primary == "application/vnd.ms-excel" ||
                   primary == "application/vnd.ms-powerpoint" ||
                   primary.find("opendocument") != std::string::npos) {
            step = ingestion::builtin::createParseOfficeStep(ext);
            reg.registerStep("builtin.parse_office", step);
        } else if (primary.rfind("image/", 0) == 0) {
            step = ingestion::builtin::createParseImageStep(ext);
            reg.registerStep("builtin.parse_image", step);
        } else if (primary == "application/zip" ||
                   primary == "application/x-tar" ||
                   primary == "application/gzip" ||
                   primary == "application/x-7z-compressed") {
            step = ingestion::builtin::createParseArchiveStep(ext);
            reg.registerStep("builtin.parse_archive", step);
        } else if (primary.rfind("audio/", 0) == 0) {
            step = ingestion::builtin::createParseAudioStep(ext);
            reg.registerStep("builtin.parse_audio", step);
        } else {
            // Generic text extractor → registers under builtin.parse_format_<name>
            const std::string step_name =
                std::string("builtin.parse_format.") + ext->name();
            step = ingestion::builtin::createParsePdfStep(ext); // reuse base
            reg.registerStep(step_name, step);
        }
    };

    for (auto& ext : impl_->format_extractors) {
        if (ext) {
            registerFormatStep(ext);
        }
    }

    // ── 4. Inject text-generation backend
    if (impl_->text_backend) {
        toolbox->setTextBackend(impl_->text_backend);
    }

    // ── 5. Load workflow profiles
    auto engine = toolbox->workflowEngine();
    for (const auto& path : impl_->profile_paths) {
        auto res = engine->loadProfile(path);
        if (!res) {
            // Non-fatal: log and continue
            Logger::warn("ToolboxBuilder: failed to load profile '{}': {}",
                         path, res.error().message());
        }
    }

    return toolbox;
}

// ── Accessors ─────────────────────────────────────────────────────────────────

std::shared_ptr<ingestion::IGraphWriter> ToolboxBuilder::graphWriter() const {
    return impl_->graph_writer;
}

std::size_t ToolboxBuilder::profileCount() const noexcept {
    return impl_->profile_paths.size();
}

} // namespace toolbox
} // namespace themis
