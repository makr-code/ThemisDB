/**
 * @file html_processor.h
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

class HtmlProcessor : public IContentProcessor {
public:
    struct Config {
        bool remove_boilerplate = true;
        bool remove_scripts_styles = true;
        bool decode_entities = true;
        bool preserve_heading_markers = false;
        size_t max_text_length = 0;
    };

    HtmlProcessor();
    /**
     * @brief Html Processor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit HtmlProcessor(Config config);
    ~HtmlProcessor() override = default;

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

    std::string getName() const override { return "HtmlProcessor"; }

    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};
    }


    /**
     * @brief Remove Boilerplate.
     * @param[in] html Input parameter.
     * @return Return value.
     */
    static std::string removeBoilerplate(const std::string& html);

    /**
     * @brief Remove Scripts And Styles.
     * @param[in] html Input parameter.
     * @return Return value.
     */
    static std::string removeScriptsAndStyles(const std::string& html);

    static std::string stripTags(const std::string& html,
                                  bool preserve_headings = false);

    /**
     * @brief Decode Entities.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    static std::string decodeEntities(const std::string& text);

    /**
     * @brief Extract Meta Tags.
     * @param[in] html Input parameter.
     * @return Return value.
     */
    static json extractMetaTags(const std::string& html);

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

    /**
     * @brief Remove Element.
     * @param[in] html Input parameter.
     * @param[in] tag Input parameter.
     * @return Return value.
     */
    static std::string removeElement(const std::string& html, const std::string& tag);
};

/**
 * @brief Create Html Processor.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createHtmlProcessor();

/**
 * @brief Create Html Processor.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<IContentProcessor> createHtmlProcessor(HtmlProcessor::Config config);

} // namespace content
} // namespace themis

