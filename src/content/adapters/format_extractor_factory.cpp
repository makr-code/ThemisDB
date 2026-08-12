/**
 * @file format_extractor_factory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <mutex>
#include <unordered_map>

#include "content/adapters/format_extractor_adapters.h"

namespace themis {
namespace content {
namespace adapters {

// ─────────────────────────────────────────────────────────────────────────────
// FormatExtractorFactory::Impl
// ─────────────────────────────────────────────────────────────────────────────

/** @brief FormatExtractorFactory::Impl. */
class FormatExtractorFactory::Impl {
  public:
    std::unordered_map<std::string, std::shared_ptr<ingestion::IFormatExtractor>> registry;
    mutable std::mutex mutex;
};

// ─────────────────────────────────────────────────────────────────────────────
// FormatExtractorFactory public API
// ─────────────────────────────────────────────────────────────────────────────

FormatExtractorFactory::FormatExtractorFactory() : impl_(std::make_unique<Impl>()) {}

FormatExtractorFactory::~FormatExtractorFactory() = default;

std::shared_ptr<ingestion::IFormatExtractor> FormatExtractorFactory::extractorFor(const std::string &mime_type) const {
    std::lock_guard<std::mutex> lk(impl_->mutex);
    auto it = impl_->registry.find(mime_type);
    if (it != impl_->registry.end()) {
        return it->second;
    }
    return nullptr;
}

void FormatExtractorFactory::registerExtractor(std::shared_ptr<ingestion::IFormatExtractor> extractor) {
    if (!extractor) {
        return;
    }
    std::lock_guard<std::mutex> lk(impl_->mutex);
    for (const auto &mime : extractor->supportedMimeTypes()) {
        impl_->registry.emplace(mime, extractor);
    }
}

std::vector<std::string> FormatExtractorFactory::registeredMimeTypes() const {
    std::lock_guard<std::mutex> lk(impl_->mutex);
    std::vector<std::string> result;
    result.reserve(impl_->registry.size());
    for (const auto &[mime, _] : impl_->registry) {
        result.push_back(mime);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// createDefaultFormatExtractorFactory
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<FormatExtractorFactory> createDefaultFormatExtractorFactory() {
    auto factory = std::make_shared<FormatExtractorFactory>();

    // Text (always available when THEMIS_ENABLE_CONTENT is ON)
    if (auto e = createTextExtractorAdapter()) {
        factory->registerExtractor(e);
    }

    // PDF
    if (auto e = createPdfExtractorAdapter()) {
        factory->registerExtractor(e);
    }

    // Office (guarded by THEMIS_ENABLE_OFFICE inside its own adapter)
    if (auto e = createOfficeExtractorAdapter()) {
        factory->registerExtractor(e);
    }

    // Image
    if (auto e = createImageExtractorAdapter()) {
        factory->registerExtractor(e);
    }

    // Archive
    if (auto e = createArchiveExtractorAdapter()) {
        factory->registerExtractor(e);
    }

    // Audio / STT (guarded by THEMIS_ENABLE_VOICE_ASSISTANT)
    if (auto e = createAudioExtractorAdapter()) {
        factory->registerExtractor(e);
    }

    return factory;
}

} // namespace adapters
} // namespace content
} // namespace themis
