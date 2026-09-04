/**
 * @file toolbox_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "toolbox/toolbox_builder.h"
#include "aql/aql_ingestion_bridge.h"
#include "rag/rag_ingestion_bridge.h"
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
    std::vector<std::string>                                       profile_paths;
    std::shared_ptr<ingestion::WorkflowEngine>                     engine;
    std::shared_ptr<ingestion::ITextGenerationBackend>             text_backend;
    std::shared_ptr<ingestion::IGraphWriter>                       graph_writer;
    std::shared_ptr<ingestion::IVectorWriter>                      vector_writer;
    std::vector<std::shared_ptr<ingestion::IFormatExtractor>>      format_extractors;
    std::shared_ptr<ingestion::ITensorDecompositionBackend>        tensor_decomp_backend;
    std::shared_ptr<ingestion::ITensorCoreBridge>                  tensor_core_bridge;
    bool                                                           built{false};
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

ToolboxBuilder& ToolboxBuilder::withVectorWriter(
    std::shared_ptr<ingestion::IVectorWriter> writer)
{
    impl_->vector_writer = std::move(writer);
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withTextBackend(
    std::shared_ptr<ingestion::ITextGenerationBackend> backend)
{
    impl_->text_backend = std::move(backend);
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withTensorDecompositionBackend(
    std::shared_ptr<ingestion::ITensorDecompositionBackend> backend)
{
    impl_->tensor_decomp_backend = std::move(backend);
    return *this;
}

ToolboxBuilder& ToolboxBuilder::withTensorCoreSink(
    std::shared_ptr<ingestion::ITensorCoreBridge> sink)
{
    impl_->tensor_core_bridge = std::move(sink);
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
    std::set<ingestion::IFormatExtractor*> seen = {};

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

    // ── Phase 2.1: Validate profile paths early (fail-fast for invalid input)
    for (const auto& path : impl_->profile_paths) {
        if (path.empty()) {
            throw std::invalid_argument("ToolboxBuilder::build(): empty profile path found in paths list");
        }
    }

    // ── 1. Create toolbox (uses createDefault() which registers NER + LLM steps)
    auto toolbox = IngestionToolbox::createDefault();
    if (!toolbox) {
        throw std::logic_error("ToolboxBuilder::build(): failed to create default IngestionToolbox");
    }

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
            (void)reg.registerStep("builtin.parse_pdf", step);
        } else if (primary.find("vnd.openxmlformats") != std::string::npos ||
                   primary == "application/msword" ||
                   primary == "application/vnd.ms-excel" ||
                   primary == "application/vnd.ms-powerpoint" ||
                   primary.find("opendocument") != std::string::npos) {
            step = ingestion::builtin::createParseOfficeStep(ext);
            (void)reg.registerStep("builtin.parse_office", step);
        } else if (primary.rfind("image/", 0) == 0) {
            step = ingestion::builtin::createParseImageStep(ext);
            (void)reg.registerStep("builtin.parse_image", step);
        } else if (primary == "application/zip" ||
                   primary == "application/x-tar" ||
                   primary == "application/gzip" ||
                   primary == "application/x-7z-compressed") {
            step = ingestion::builtin::createParseArchiveStep(ext);
            (void)reg.registerStep("builtin.parse_archive", step);
        } else if (primary.rfind("audio/", 0) == 0) {
            step = ingestion::builtin::createParseAudioStep(ext);
            (void)reg.registerStep("builtin.parse_audio", step);
        } else {
            // Generic text extractor → registers under builtin.parse_format_<name>
            const std::string step_name =
                std::string("builtin.parse_format.") + ext->name();
            step = ingestion::builtin::createParsePdfStep(ext); // reuse base
            (void)reg.registerStep(step_name, step);
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

    // ── 4b. Re-register tensor steps with real backends (if provided)
    if (impl_->tensor_decomp_backend) {
        (void)reg.registerStep("builtin.chunk_tt_decompose",
            ingestion::builtin::createChunkTtDecomposeStep(impl_->tensor_decomp_backend));
    }
    if (impl_->tensor_core_bridge) {
        (void)reg.registerStep("builtin.tensor_core_bridge",
            ingestion::builtin::createTensorCoreBridgeStep(impl_->tensor_core_bridge));
    }

    // ── 5. Load workflow profiles
    auto engine = toolbox->workflowEngine();
    for (const auto& path : impl_->profile_paths) {
        auto res = engine->loadProfile(path);
        if (!res) {
            // Non-fatal: log and continue
            THEMIS_WARN("ToolboxBuilder: failed to load profile '{}': {}",
                        path, res.error().message());
        }
    }

    return toolbox;
}

// ── Accessors ─────────────────────────────────────────────────────────────────

std::shared_ptr<ingestion::IGraphWriter> ToolboxBuilder::graphWriter() const {
    return impl_->graph_writer;
}

std::shared_ptr<ingestion::IVectorWriter> ToolboxBuilder::vectorWriter() const {
    return impl_->vector_writer;
}

std::size_t ToolboxBuilder::profileCount() const noexcept {
    return static_cast<bool>(impl_- < static_cast<int>(profile_paths.size()));
}

// ── BuiltToolbox special members ─────────────────────────────────────────────

ToolboxBuilder::BuiltToolbox::BuiltToolbox()  = default;
ToolboxBuilder::BuiltToolbox::~BuiltToolbox() = default;
ToolboxBuilder::BuiltToolbox::BuiltToolbox(BuiltToolbox&&) noexcept            = default;
ToolboxBuilder::BuiltToolbox& ToolboxBuilder::BuiltToolbox::operator=(BuiltToolbox&&) noexcept = default;

// ── buildWithBridges() ────────────────────────────────────────────────────────

ToolboxBuilder::BuiltToolbox ToolboxBuilder::buildWithBridges() {
    // Guard runs before build() — build() will set impl_->built = true, so any
    // subsequent call to buildWithBridges() or build() will throw here or inside
    // build() respectively, preventing double-initialisation.
    if (impl_->built) {
        throw std::logic_error(
            "ToolboxBuilder::buildWithBridges() called after build() or buildWithBridges()");
    }

    // Phase 2.2: Validate bridge requirements before construction
    // Soft-fail behavior: missing optional writers are acceptable
    // Required: toolbox must be created by build()
    
    auto toolbox = this->build();  // delegates to the existing build() path

    if (!toolbox) {
        throw std::logic_error(
            "ToolboxBuilder::buildWithBridges(): build() returned null toolbox");
    }

    BuiltToolbox out;
    out.toolbox = toolbox;

    // Phase 2.3: Wire bridges from optional sinks
    // graph_writer and vector_writer are optional; soft-fail if missing
    if (impl_->graph_writer) {
        try {
            out.aql_bridge = std::make_shared<aql::AQLIngestionBridge>(
                toolbox, impl_->graph_writer);
        } catch (const std::exception& e) {
            THEMIS_WARN("ToolboxBuilder::buildWithBridges(): AQLIngestionBridge construction failed: {}",
                        e.what());
            // Soft fail: continue without AQL bridge
        }
    }

    if (impl_->vector_writer || impl_->graph_writer) {
        try {
            out.rag_bridge = std::make_shared<rag::RAGIngestionBridge>(
                toolbox, impl_->vector_writer, impl_->graph_writer);
        } catch (const std::exception& e) {
            THEMIS_WARN("ToolboxBuilder::buildWithBridges(): RAGIngestionBridge construction failed: {}",
                        e.what());
            // Soft fail: continue without RAG bridge
        }
    }

    return out;
}

} // namespace toolbox
} // namespace themis

