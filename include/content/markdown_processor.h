/**
 * @file markdown_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "content/content_processor.h"
#include <string>
#include <vector>

namespace themis {
namespace content {

class MarkdownProcessor : public IContentProcessor {
public:
    struct Config {
        bool parse_frontmatter = true;
        bool preserve_heading_markers = false;
        bool strip_code_blocks = false;
        size_t max_text_length = 0;
    };

    MarkdownProcessor();
    /**
     * @brief Markdown Processor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MarkdownProcessor(Config config);
    ~MarkdownProcessor() override = default;

    ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) override;

    std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) override;

    std::vector<float> generateEmbedding(const std::string& chunk_data) override;

    std::string getName() const override { return "MarkdownProcessor"; }

    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};
    }


    /**
     * @brief Parse Frontmatter.
     * @param[in] markdown Input parameter.
     * @param[in,out] body_out Input/output parameter.
     * @return Return value.
     */
    static json parseFrontmatter(const std::string& markdown,
                                  std::string& body_out);

    static std::string stripMarkdown(const std::string& markdown,
                                      bool preserve_headings = false,
                                      bool strip_code = false);

private:
    Config config_;

    /**
     * @brief Normalize Whitespace.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string normalizeWhitespace(const std::string& text);

    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static int countTokens(const std::string& text);
};

/**
 * @brief Create Markdown Processor.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createMarkdownProcessor();

/**
 * @brief Create Markdown Processor.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createMarkdownProcessor(MarkdownProcessor::Config config);

} // namespace content
} // namespace themis
