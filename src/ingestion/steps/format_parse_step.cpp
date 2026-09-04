/**
 * @file format_parse_step.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ─────────────────────────────────────────────────────────────────────────────
// Format-aware parse steps for the ingestion pipeline.
//
// Five builtin steps are defined in this single translation unit to keep the
// per-step boilerplate to a minimum:
//
//   builtin.parse_pdf      — PDF text and metadata via IFormatExtractor
//   builtin.parse_office   — DOCX / XLSX / PPTX / ODF
//   builtin.parse_image    — Image metadata + optional OCR text
//   builtin.parse_archive  — ZIP / TAR → ctx.extracted_file_paths
//   builtin.parse_audio    — STT transcription for audio files
//
// Each step receives an IFormatExtractor via constructor injection.  When the
// extractor is nullptr (e.g. THEMIS_ENABLE_CONTENT=OFF), the step's
// canHandle() returns false so it is silently skipped.
//
// Factory functions are declared in include/ingestion/builtin_step_factories.h
// (added in the same commit).
// ─────────────────────────────────────────────────────────────────────────────

#include "ingestion/ingestion_step.h"
#include "ingestion/format_extractor.h"
#include "utils/error_registry.h"

#include <fstream>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

namespace themis {
namespace ingestion {
namespace builtin {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helper — FormatParseStepBase
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief CRTP-free base for all format-specific parse steps.
 *
 * Subclasses provide `pluginName()` and optionally override
 * `postProcess(ctx, result)` to do type-specific context enrichment
 * (e.g. archive: populate extracted_file_paths).
 */
class FormatParseStepBase : public IIngestionStep {
public:
    explicit FormatParseStepBase(std::shared_ptr<IFormatExtractor> extractor,
                                 const char* plugin_name)
        : extractor_(std::move(extractor))
        , plugin_name_(plugin_name)
    {}

    // IThemisPlugin boilerplate
    const char* getName()    const override { return plugin_name_; }
    const char* getVersion() const override { return "0.1.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool  initialize(cons[[maybe_unused]] t cha[[maybe_unused]] r*) override { return true; }
    void  shutdown()              override {}
    void* getInstance()           override { return this; }

    // canHandle: delegate to extractor's MIME list
    bool canHandle(cons[[maybe_unused]] t ExtractionContext& [[maybe_unused]] ctx) const override {
        if (!extractor_) {
          return false;
        }
        const auto& mimes = extractor_->supportedMimeTypes();
        if (mimes.empty()) {
          return true;
        }
        for (const auto& m : mimes) {
            if (m == ctx.manifest.detected_mime) {
              return true;
            }
        }
        return false;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return extractor_ ? extractor_->supportedMimeTypes()
                          : std::vector<std::string>{};
    }

    Result<void> execute(ExtractionContext& ctx,
                         const StepConfig& /*cfg*/) override
    {
        if (!extractor_) {
            // No extractor available (e.g. THEMIS_ENABLE_CONTENT=OFF)
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      std::string(plugin_name_) + ": no extractor available"});
        }

        if (!ctx.raw_text.empty()) {
            return {};  // already populated by an earlier step
        }

        // Load the raw bytes from the manifest path
        const std::string& path = ctx.manifest.original_path;
        if (path.empty()) {
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      std::string(plugin_name_) + ": manifest.original_path is empty"});
        }

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      std::string(plugin_name_) + ": cannot open '" + path + "'"});
        }
        std::ostringstream oss;
        oss << file.rdbuf();
        const std::string raw_bytes = oss.str();

        // Convert to span and call extractor
        const auto* byte_ptr = reinterpret_cast<const std::byte*>(raw_bytes.data());
        std::span<const std::byte> span{byte_ptr, raw_bytes.size()};

        FormatExtractResult result = extractor_->extract(
            span,
            ctx.manifest.detected_mime,
            ctx.manifest.filename_stem + ctx.manifest.extension);

        if (!result.ok) {
            return tl::make_unexpected(
                Error{errors::ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                      std::string(plugin_name_) + ": " + result.error});
        }

        // Populate context
        ctx.raw_text = std::move(result.raw_text);
        if (!result.detected_lang.empty()) {
            ctx.text_language = std::move(result.detected_lang);
        }
        if (!result.metadata.empty()) {
            // Merge metadata into ctx.extra using "format.*" namespace
            for (auto& [k, v] : result.metadata.items()) {
                if (v.is_string()) {
                    ctx.extra["format." + k] = v.get<std::string>();
                } else {
                    ctx.extra["format." + k] = v.dump();
                }
            }
        }

        // Type-specific enrichment (overridden by ArchiveParseStep)
        postProcess(ctx, result);

        return {};
    }

protected:
    /**
     * @brief Hook for subclass-specific context enrichment after extraction.
     *
     * Default: no-op.  ArchiveParseStep overrides this to populate
     * ctx.extracted_file_paths.
     */
    virtual void postProcess(ExtractionContext& /*ctx*/,
                             FormatExtractResult& /*result*/) {}

    std::shared_ptr<IFormatExtractor> extractor_;
    const char* plugin_name_;
};

// ─────────────────────────────────────────────────────────────────────────────
// builtin.parse_pdf
// ─────────────────────────────────────────────────────────────────────────────

/** @brief builtin.parse_pdf. */
class ParsePdfStep final : public FormatParseStepBase {
public:
    explicit ParsePdfStep(std::shared_ptr<IFormatExtractor> e)
        : FormatParseStepBase(std::move(e), "builtin.parse_pdf") {}
};

// ─────────────────────────────────────────────────────────────────────────────
// builtin.parse_office
// ─────────────────────────────────────────────────────────────────────────────

/** @brief builtin.parse_office. */
class ParseOfficeStep final : public FormatParseStepBase {
public:
    explicit ParseOfficeStep(std::shared_ptr<IFormatExtractor> e)
        : FormatParseStepBase(std::move(e), "builtin.parse_office") {}
};

// ─────────────────────────────────────────────────────────────────────────────
// builtin.parse_image
// ─────────────────────────────────────────────────────────────────────────────

/** @brief builtin.parse_image. */
class ParseImageStep final : public FormatParseStepBase {
public:
    explicit ParseImageStep(std::shared_ptr<IFormatExtractor> e)
        : FormatParseStepBase(std::move(e), "builtin.parse_image") {}
};

// ─────────────────────────────────────────────────────────────────────────────
// builtin.parse_archive
// ─────────────────────────────────────────────────────────────────────────────

/** @brief builtin.parse_archive. */
class ParseArchiveStep final : public FormatParseStepBase {
public:
    explicit ParseArchiveStep(std::shared_ptr<IFormatExtractor> e)
        : FormatParseStepBase(std::move(e), "builtin.parse_archive") {}

protected:
    void postProcess(ExtractionContext& ctx,
                     FormatExtractResult& result) override {
        // Archive members → ctx.extracted_file_paths for recursive ingestion
        if (!result.child_paths.empty()) {
            ctx.extracted_file_paths.insert(
                ctx.extracted_file_paths.end(),
                std::make_move_iterator(result.child_paths.begin()),
                std::make_move_iterator(result.child_paths.end()));
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// builtin.parse_audio
// ─────────────────────────────────────────────────────────────────────────────

/** @brief builtin.parse_audio. */
class ParseAudioStep final : public FormatParseStepBase {
public:
    explicit ParseAudioStep(std::shared_ptr<IFormatExtractor> e)
        : FormatParseStepBase(std::move(e), "builtin.parse_audio") {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory functions (declared in builtin_step_factories.h)
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IIngestionStep> createParsePdfStep(
    std::shared_ptr<IFormatExtractor> extractor)
{
    return std::make_shared<ParsePdfStep>(std::move(extractor));
}

std::shared_ptr<IIngestionStep> createParseOfficeStep(
    std::shared_ptr<IFormatExtractor> extractor)
{
    return std::make_shared<ParseOfficeStep>(std::move(extractor));
}

std::shared_ptr<IIngestionStep> createParseImageStep(
    std::shared_ptr<IFormatExtractor> extractor)
{
    return std::make_shared<ParseImageStep>(std::move(extractor));
}

std::shared_ptr<IIngestionStep> createParseArchiveStep(
    std::shared_ptr<IFormatExtractor> extractor)
{
    return std::make_shared<ParseArchiveStep>(std::move(extractor));
}

std::shared_ptr<IIngestionStep> createParseAudioStep(
    std::shared_ptr<IFormatExtractor> extractor)
{
    return std::make_shared<ParseAudioStep>(std::move(extractor));
}

} // namespace builtin
} // namespace ingestion
} // namespace themis
