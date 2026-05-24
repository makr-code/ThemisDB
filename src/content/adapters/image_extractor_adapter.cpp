/*
 * ThemisDB | File: image_extractor_adapter.cpp | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "content/adapters/format_extractor_adapters.h"
#include "content/image_processor.h"
#include "content/content_plugin_interface.h"

namespace themis {
namespace content {
namespace adapters {

namespace {

class ImageExtractorAdapter : public ingestion::IFormatExtractor {
public:
    ImageExtractorAdapter() {
        PluginConfig cfg;
        processor_.initialize(cfg);
    }

    ingestion::FormatExtractResult extract(
        std::span<const std::byte> data,
        const std::string& mime_type,
        const std::string& /*filename_hint*/) override
    {
        ingestion::FormatExtractResult out;
        try {
            std::vector<uint8_t> blob(
                reinterpret_cast<const uint8_t*>(data.data()),
                reinterpret_cast<const uint8_t*>(data.data()) + data.size());

            ExtractionOptions opts;
            auto result = processor_.extract(blob, mime_type, opts);
            if (!result.success) {
                out.error = result.error_message;
                return out;
            }

            out.raw_text      = std::move(result.text);
            out.metadata      = std::move(result.metadata);
            out.detected_lang = "";  // images don't have a language per se
            out.ok = true;
        } catch (const std::exception& ex) {
            out.error = std::string("ImageExtractorAdapter: exception: ") + ex.what();
        }
        return out;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return {
            "image/jpeg",
            "image/png",
            "image/gif",
            "image/webp",
            "image/tiff",
            "image/bmp",
            "image/svg+xml",
        };
    }

    const char* name() const noexcept override {
        return "ImageExtractorAdapter";
    }

private:
    ImageProcessor processor_;
};

} // anonymous namespace

std::shared_ptr<ingestion::IFormatExtractor> createImageExtractorAdapter() {
    return std::make_shared<ImageExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis
