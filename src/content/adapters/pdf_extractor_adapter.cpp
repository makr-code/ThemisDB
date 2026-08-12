/**
 * @file pdf_extractor_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "content/adapters/format_extractor_adapters.h"
#include "content/pdf_processor.h"
#include "content/content_type.h"
#include <cstring>

namespace themis {
namespace content {
namespace adapters {

namespace {

class PdfExtractorAdapter : public ingestion::IFormatExtractor {
public:
    PdfExtractorAdapter() = default;

    ingestion::FormatExtractResult extract(
        std::span<const std::byte> data,
        const std::string& /*mime_type*/,
        [[maybe_unused]] const std::string& filename_hint) override
    {
        ingestion::FormatExtractResult out;
        try {
            // Convert span to string blob (PDFProcessor expects std::string)
            std::string blob(reinterpret_cast<const char*>(data.data()), data.size());

            ContentType ct;
            ct.mime_type = "application/pdf";
            ct.category  = ContentCategory::TEXT;
            ct.supports_text_extraction = true;

            auto result = processor_.extract(blob, ct);
            if (!result.ok) {
                out.error = result.error_message;
                return out;
            }

            out.raw_text     = std::move(result.text);
            out.metadata     = std::move(result.metadata);
            out.detected_lang = result.metadata.value("language", "");
            out.ok = true;
        } catch (const std::exception& ex) {
            out.error = std::string("PdfExtractorAdapter: exception: ") + ex.what();
        }
        return out;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return {"application/pdf"};
    }

    const char* name() const noexcept override {
        return "PdfExtractorAdapter";
    }

private:
    PDFProcessor processor_;
};

} // anonymous namespace

std::shared_ptr<ingestion::IFormatExtractor> createPdfExtractorAdapter() {
    return std::make_shared<PdfExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis
