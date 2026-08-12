/**
 * @file format_extractor_adapters.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// This header is ONLY to be included from content/ or from build wiring code.
// It MUST NOT be included from ingestion/ or toolbox/ to preserve the
// dependency direction: ingestion/ and toolbox/ only know IFormatExtractor.

#include "ingestion/format_extractor.h"
#include <memory>

namespace themis {
namespace content {
namespace adapters {

// ─────────────────────────────────────────────────────────────────────────────
// Factory functions — each returns a heap-allocated adapter that wraps the
// corresponding content/ processor.  Guards mirror the CMake build options.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Create a PDF extractor adapter wrapping `content::PDFProcessor`.
 *
 * Available when `THEMIS_ENABLE_CONTENT` is ON and libpoppler is present.
 * Returns nullptr when `THEMIS_ENABLE_CONTENT` is OFF (compile-time guard).
 */
std::shared_ptr<ingestion::IFormatExtractor> createPdfExtractorAdapter();

/**
 * @brief Create an Office extractor adapter wrapping `content::OfficeProcessor`.
 *
 * Handles application/vnd.openxmlformats-officedocument.* (DOCX, XLSX, PPTX)
 * and application/msword / application/vnd.ms-excel via libzip + pugixml.
 * Requires `THEMIS_ENABLE_CONTENT && THEMIS_ENABLE_OFFICE`.
 */
std::shared_ptr<ingestion::IFormatExtractor> createOfficeExtractorAdapter();

/**
 * @brief Create an Image extractor adapter wrapping `content::ImageProcessor`
 *        and `content::OCRProcessor` (when OCR is enabled).
 *
 * Extracts EXIF metadata and (optionally) OCR text.
 * Requires `THEMIS_ENABLE_CONTENT`.
 */
std::shared_ptr<ingestion::IFormatExtractor> createImageExtractorAdapter();

/**
 * @brief Create an Archive extractor adapter wrapping `content::ArchiveProcessor`.
 *
 * Handles application/zip, application/x-tar, application/gzip, etc.
 * Extracts members to a temporary directory and returns their paths in
 * `FormatExtractResult::child_paths`.
 * Requires `THEMIS_ENABLE_CONTENT`.
 */
std::shared_ptr<ingestion::IFormatExtractor> createArchiveExtractorAdapter();

/**
 * @brief Create an Audio extractor adapter wrapping `content::STTProcessor`.
 *
 * Transcribes audio to text via Whisper/FFmpeg.
 * Available when `THEMIS_ENABLE_CONTENT && THEMIS_ENABLE_VOICE_ASSISTANT` is ON.
 */
std::shared_ptr<ingestion::IFormatExtractor> createAudioExtractorAdapter();

/**
 * @brief Create a plain-text / HTML / Markdown extractor adapter.
 *
 * Handles text/plain, text/html, text/markdown.  No heavy dependencies;
 * always available when `THEMIS_ENABLE_CONTENT` is ON.
 */
std::shared_ptr<ingestion::IFormatExtractor> createTextExtractorAdapter();

// ─────────────────────────────────────────────────────────────────────────────
// FormatExtractorFactory — concrete IFormatExtractorFactory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Concrete factory that maps MIME types to content/ processor adapters.
 *
 * Constructed by `createDefaultFormatExtractorFactory()`.  All available
 * adapters are registered based on compile-time feature flags.
 */
class FormatExtractorFactory : public ingestion::IFormatExtractorFactory {
public:
    FormatExtractorFactory();
    ~FormatExtractorFactory() override;

    std::shared_ptr<ingestion::IFormatExtractor> extractorFor(
        const std::string& mime_type) const override;

    void registerExtractor(
        std::shared_ptr<ingestion::IFormatExtractor> extractor) override;

    std::vector<std::string> registeredMimeTypes() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Create a `FormatExtractorFactory` pre-populated with all adapters
 *        available at compile time.
 *
 * This is the primary entry point for server bootstrap and `ToolboxBuilder`.
 *
 * Equivalent to:
 * @code
 * auto f = std::make_shared<FormatExtractorFactory>();
 * f->registerExtractor(createPdfExtractorAdapter());     // if available
 * f->registerExtractor(createOfficeExtractorAdapter());  // if available
 * // ...
 * return f;
 * @endcode
 */
std::shared_ptr<FormatExtractorFactory> createDefaultFormatExtractorFactory();

} // namespace adapters
} // namespace content
} // namespace themis
