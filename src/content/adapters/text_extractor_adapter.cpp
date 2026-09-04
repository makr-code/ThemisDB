/**
 * @file text_extractor_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "content/adapters/format_extractor_adapters.h"
#include "content/html_processor.h"
#include "content/markdown_processor.h"
#include "content/content_processor.h"
#include "content/content_type.h"

namespace themis {
namespace content {
namespace adapters {

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// TextExtractorAdapter — handles text/plain, text/html, text/markdown
// ─────────────────────────────────────────────────────────────────────────────

class TextExtractorAdapter : public ingestion::IFormatExtractor {
public:
    TextExtractorAdapter() = default;

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

            ExtractionResult result = {};
            if (mime_type == "text/html" || mime_type == "application/xhtml+xml") {
                result = html_processor_.extract(blob, ct);
            } else if (mime_type == "text/markdown" || mime_type == "text/x-markdown") {
                result = md_processor_.extract(blob, ct);
            } else {
                // Plain text — use TextProcessor
                result = text_processor_.extract(blob, ct);
            }

            if (!result.ok) {
                out.error = result.error_message;
                return out;
            }

            out.raw_text      = std::move(result.text);
            out.metadata      = std::move(result.metadata);
            out.detected_lang = result.metadata.value("language", "");
            out.ok = true;
        } catch (const std::exception& ex) {
            out.error = std::string("TextExtractorAdapter: exception: ") + ex.what();
        }
        return out;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return {
            "text/plain",
            "text/html",
            "application/xhtml+xml",
            "text/markdown",
            "text/x-markdown",
            "text/xml",
            "application/xml",
            "application/json",
            "text/csv",
        };
    }

    const char* name() const noexcept override {
        return "TextExtractorAdapter";
    }

private:
    TextProcessor     text_processor_;
    HtmlProcessor     html_processor_;
    MarkdownProcessor md_processor_;
};

} // anonymous namespace

std::shared_ptr<ingestion::IFormatExtractor> createTextExtractorAdapter() {
    return std::make_shared<TextExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis
