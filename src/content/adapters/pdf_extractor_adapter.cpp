/** @file pdf_extractor_adapter.cpp @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata. @author makr-code @version 0.0.1 @date 2026-09-18 21:43:08 @note Maturity: 🟢 PRODUCTION-READY @note Score: 100/100 @note Lines: 74 @note Module Context: src/content @note Ownership Scope: production-code @note Primary Symbols: PdfExtractorAdapter, createPdfExtractorAdapter @note PR History (last 5): none @note Governance: BranchModel=develop-first; CanonicalBranches=develop,community,military @note Release Context: GateModel=WaveA→B→C→D on develop @note Status: Production Ready @note Generator: .github/scripts/code_maturity_header_writer.py @note This block is auto-generated and will be overwritten. */

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
        const std::string& filename_hint) override
    {
        ingestion::FormatExtractResult out;
        try {
            // Convert span to string blob (PDFProcessor expects std::string)
            std::string blob(reinterpret_cast<const char*>(data.data()),data.size());

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

/**
 * @brief Create Pdf Extractor Adapter.
 * @return Return value.
 * @details Implements createPdfExtractorAdapter without additional internal calls.
 */
std::shared_ptr<ingestion::IFormatExtractor> createPdfExtractorAdapter() {
    return std::make_shared<PdfExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis
