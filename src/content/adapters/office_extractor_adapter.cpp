/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            office_extractor_adapter.cpp                       ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-04-16                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "content/adapters/format_extractor_adapters.h"

#ifdef THEMIS_ENABLE_OFFICE
#include "content/office_processor.h"
#include "content/content_type.h"

namespace themis {
namespace content {
namespace adapters {

namespace {

class OfficeExtractorAdapter : public ingestion::IFormatExtractor {
public:
    OfficeExtractorAdapter() = default;

    ingestion::FormatExtractResult extract(
        std::span<const std::byte> data,
        const std::string& mime_type,
        const std::string& /*filename_hint*/) override
    {
        ingestion::FormatExtractResult out;
        try {
            std::string blob(reinterpret_cast<const char*>(data.data()), data.size());

            ContentType ct;
            ct.mime_type = mime_type;
            ct.category  = ContentCategory::TEXT;
            ct.supports_text_extraction = true;

            auto result = processor_.extract(blob, ct);
            if (!result.ok) {
                out.error = result.error_message;
                return out;
            }

            out.raw_text      = std::move(result.text);
            out.metadata      = std::move(result.metadata);
            out.detected_lang = result.metadata.value("language", "");
            out.ok = true;
        } catch (const std::exception& ex) {
            out.error = std::string("OfficeExtractorAdapter: exception: ") + ex.what();
        }
        return out;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return {
            "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
            "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
            "application/vnd.openxmlformats-officedocument.presentationml.presentation",
            "application/msword",
            "application/vnd.ms-excel",
            "application/vnd.ms-powerpoint",
            "application/vnd.oasis.opendocument.text",
            "application/vnd.oasis.opendocument.spreadsheet",
        };
    }

    const char* name() const noexcept override {
        return "OfficeExtractorAdapter";
    }

private:
    OfficeProcessor processor_;
};

} // anonymous namespace

std::shared_ptr<ingestion::IFormatExtractor> createOfficeExtractorAdapter() {
    return std::make_shared<OfficeExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis

#else // !THEMIS_ENABLE_OFFICE

namespace themis { namespace content { namespace adapters {
std::shared_ptr<ingestion::IFormatExtractor> createOfficeExtractorAdapter() {
    return nullptr;
}
} } }

#endif // THEMIS_ENABLE_OFFICE
