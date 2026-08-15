/**
 * @file archive_extractor_adapter.cpp
 * @brief Archive format extractor adapter for plugin architecture (ZIP, TAR, 7Z, RAR).
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 91/100 (Batch 5 verified; no scope issues detected)
 * @note Gap Status: Batches 1-4 complete; adapter pattern RAII-compliant, safe lifetime management
 * @note Batch Tracking: CMT-7503 (scope verification complete), CMT-7505 (test coverage 95%)
 * @note Status: Production Ready; Safe plugin adapter with proper resource cleanup
 * @note This block is auto-generated and will be overwritten.
 */


#include "content/adapters/format_extractor_adapters.h"
#include "content/archive_processor.h"
#include "content/content_type.h"

namespace themis {
namespace content {
namespace adapters {

namespace {

class ArchiveExtractorAdapter : public ingestion::IFormatExtractor {
public:
    ArchiveExtractorAdapter() = default;

    ingestion::FormatExtractResult extract(
        std::span<const std::byte> data,
        const std::string& mime_type,
        const std::string& filename_hint) override
    {
        ingestion::FormatExtractResult out;
        try {
            std::string blob(reinterpret_cast<const char*>(data.data()), data.size());

            // Detect the specific archive sub-format using the static helper
            ArchiveFormat fmt = ArchiveProcessor::detectFormat(blob, filename_hint);

            // extractToTemp unpacks the archive into a temp dir and returns
            // the paths of all extracted members.
            ArchiveExtractionResult arch = processor_.extractToTemp(blob, fmt);
            if (!arch.success) {
                out.error = arch.error_message;
                return out;
            }

            // Populate child_paths so the WorkflowEngine can recurse
            out.child_paths = std::move(arch.extracted_files);

            // Also run the IContentProcessor::extract() path to collect
            // the archive-level metadata (member list, format info, etc.)
            ContentType ct;
            ct.mime_type = mime_type;
            ct.category  = ContentCategory::ARCHIVE;
            ct.supports_text_extraction = false;

            auto meta_result = processor_.extract(blob, ct);
            if (meta_result.ok) {
                out.metadata = std::move(meta_result.metadata);
            }

            out.ok = true;
        } catch (const std::exception& ex) {
            out.error = std::string("ArchiveExtractorAdapter: exception: ") + ex.what();
        }
        return out;
    }

    std::vector<std::string> supportedMimeTypes() const override {
        return {
            "application/zip",
            "application/x-zip-compressed",
            "application/x-tar",
            "application/gzip",
            "application/x-gzip",
            "application/x-bzip2",
            "application/x-xz",
            "application/x-7z-compressed",
            "application/x-rar-compressed",
        };
    }

    const char* name() const noexcept override {
        return "ArchiveExtractorAdapter";
    }

private:
    ArchiveProcessor processor_;
};

} // anonymous namespace

std::shared_ptr<ingestion::IFormatExtractor> createArchiveExtractorAdapter() {
    return std::make_shared<ArchiveExtractorAdapter>();
}

} // namespace adapters
} // namespace content
} // namespace themis
