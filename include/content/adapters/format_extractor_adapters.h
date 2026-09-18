/**
 * @file format_extractor_adapters.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
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

/**
 * @brief ───────────────────────────────────────────────────────────────────────────── Factory functions — each returns a heap-allocated adapter that wraps the corresponding content/ processor.
 * @return Return value.
 * @details Guards mirror the CMake build options. ─────────────────────────────────────────────────────────────────────────────
 */

std::shared_ptr<ingestion::IFormatExtractor> createPdfExtractorAdapter();

/**
 * @brief Create Office Extractor Adapter.
 * @return Return value.
 */
std::shared_ptr<ingestion::IFormatExtractor> createOfficeExtractorAdapter();

/**
 * @brief Create Image Extractor Adapter.
 * @return Return value.
 */
std::shared_ptr<ingestion::IFormatExtractor> createImageExtractorAdapter();

/**
 * @brief Create Archive Extractor Adapter.
 * @return Return value.
 */
std::shared_ptr<ingestion::IFormatExtractor> createArchiveExtractorAdapter();

/**
 * @brief Create Audio Extractor Adapter.
 * @return Return value.
 */
std::shared_ptr<ingestion::IFormatExtractor> createAudioExtractorAdapter();

/**
 * @brief Create Text Extractor Adapter.
 * @return Return value.
 */
std::shared_ptr<ingestion::IFormatExtractor> createTextExtractorAdapter();

// ─────────────────────────────────────────────────────────────────────────────
// FormatExtractorFactory — concrete IFormatExtractorFactory
// ─────────────────────────────────────────────────────────────────────────────

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
 * @brief Create Default Format Extractor Factory.
 * @return Return value.
 */
std::shared_ptr<FormatExtractorFactory> createDefaultFormatExtractorFactory();

} // namespace adapters
} // namespace content
} // namespace themis
