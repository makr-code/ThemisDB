/**
 * @file format_extractor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// FormatExtractResult — output of IFormatExtractor::extract()
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result produced by an `IFormatExtractor` for a single binary blob.
 *
 * The result carries everything the ingestion pipeline needs to proceed with
 * chunking, NER, embedding, and graph construction:
 *
 *  - `raw_text`    — full UTF-8 plain text (may be empty for binary-only types)
 *  - `detected_lang` — BCP-47 language code detected during extraction, e.g.
 *                      "de", "en" (empty when unknown)
 *  - `metadata`    — structured metadata (EXIF fields, PDF document properties,
 *                      audio duration, page count, …)  as a flat JSON object
 *  - `child_paths` — for archive types (ZIP, TAR): absolute paths of the files
 *                      extracted to a temporary directory so the engine can
 *                      recursively ingest them
 *  - `ok`          — true when extraction succeeded (even if `raw_text` is empty)
 *  - `error`       — human-readable error description when `ok == false`
 */
struct FormatExtractResult {
    std::string raw_text;                    ///< Extracted plain text (UTF-8)
    std::string detected_lang;               ///< BCP-47 language code ("de", "en", …)
    nlohmann::json metadata;                 ///< Structured key/value metadata
    std::vector<std::string> child_paths;    ///< Unpacked archive members (absolute paths)
    bool ok{false};                          ///< True on success
    std::string error;                       ///< Error description when ok == false
};

// ─────────────────────────────────────────────────────────────────────────────
// IFormatExtractor — decoupled format-extraction abstraction
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Format-specific text and metadata extractor interface.
 *
 * ## Motivation
 *
 * The `content/` module ships high-quality format processors (PDF, DOCX,
 * images with OCR, audio STT, archives) that have heavy dependencies on
 * `storage/`, `cache/`, and `security/`.  The ingestion pipeline and the
 * `toolbox/` module cannot depend on those subsystems directly.
 *
 * `IFormatExtractor` is a thin, dependency-free boundary: adapters inside the
 * `content/` module implement this interface and delegate internally to the
 * existing processors.  Builtin ingestion steps receive an `IFormatExtractor`
 * via constructor injection and never include `content/` headers.
 *
 * ## Dependency direction
 * @code
 *   ingestion/ defines IFormatExtractor   (no content/ include)
 *   content/   implements IFormatExtractor via adapters
 *   toolbox/   consumes via IFormatExtractor*  (no content/ include needed)
 * @endcode
 *
 * ## Thread-safety
 * Implementations MUST be thread-safe.  The `WorkflowEngine` may call
 * `extract()` from multiple threads concurrently.
 *
 * ## Error handling
 * Return `FormatExtractResult{.ok = false, .error = "…"}` on failure; do NOT
 * throw.  Exceptions from third-party libraries must be caught internally.
 */
class IFormatExtractor {
public:
    virtual ~IFormatExtractor() = default;

    /**
     * @brief Extract text and metadata from a raw binary blob.
     *
     * @param data           Raw bytes of the document/media file.
     * @param mime_type      MIME type string, e.g. "application/pdf".
     *                       The adapter uses this for routing and may override
     *                       it with a more precise type after magic-byte sniffing.
     * @param filename_hint  Original filename (without path), e.g. "report.pdf".
     *                       Used for format disambiguation and archive member
     *                       naming.  May be empty.
     * @return FormatExtractResult with extracted text, metadata, and optional
     *         child paths.
     */
    [[nodiscard]] virtual FormatExtractResult extract(
        std::span<const std::byte> data,
        const std::string& mime_type,
        const std::string& filename_hint) = 0;

    /**
     * @brief MIME types this extractor can handle.
     *
     * Used by `FormatExtractorFactory` to route a file to the right extractor
     * and by `IIngestionStep::canHandle()` default implementation.
     *
     * An empty vector means "any MIME type" (fall-through / generic extractor).
     *
     * @return Immutable list of supported MIME type strings.
     */
    [[nodiscard]] virtual std::vector<std::string> supportedMimeTypes() const = 0;

    /**
     * @brief Human-readable name of this extractor for logging and diagnostics.
     *
     * Example: "PdfExtractorAdapter", "OfficeExtractorAdapter".
     */
    [[nodiscard]] virtual const char* name() const noexcept = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// IFormatExtractorFactory — obtains the right extractor for a MIME type
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Factory that maps MIME types to `IFormatExtractor` implementations.
 *
 * The concrete factory lives in `content/adapters/format_extractor_factory.cpp`
 * (guarded by `THEMIS_ENABLE_CONTENT`) and wires together all available
 * processor adapters.  When `THEMIS_ENABLE_CONTENT` is OFF, the factory
 * returns `nullptr` for all types (plain text fall-through is still handled
 * by `builtin.parse_text`).
 *
 * Usage by builtin steps:
 * @code
 * auto extractor = factory_->extractorFor("application/pdf");
 * if (!extractor) { return skip_or_error; }
 * auto result = extractor->extract(data, mime, hint);
 * @endcode
 */
class IFormatExtractorFactory {
public:
    virtual ~IFormatExtractorFactory() = default;

    /**
     * @brief Return the extractor registered for @p mime_type, or nullptr.
     *
     * @param mime_type  MIME type string, e.g. "application/pdf".
     * @return Shared pointer to the registered extractor, or `nullptr` when
     *         no extractor is registered for that type.
     */
    [[nodiscard]] virtual std::shared_ptr<IFormatExtractor> extractorFor(
        const std::string& mime_type) const = 0;

    /**
     * @brief Register an extractor for one or more MIME types.
     *
     * Calling this with an extractor whose `supportedMimeTypes()` returns
     * `{"application/pdf"}` registers it for exactly that one MIME type.
     *
     * @param extractor  Extractor instance to register.
     */
    virtual void registerExtractor(
        std::shared_ptr<IFormatExtractor> extractor) = 0;

    /**
     * @brief List all MIME types for which an extractor is registered.
     */
    [[nodiscard]] virtual std::vector<std::string> registeredMimeTypes() const = 0;
};

} // namespace ingestion
} // namespace themis
