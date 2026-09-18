/**
 * @file content_classifier.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace themis {
namespace content {

// ---------------------------------------------------------------------------
// ContentCategory — a single taxonomy label with confidence score
// ---------------------------------------------------------------------------

struct ContentCategory {
    std::string category_id;
    std::string label;           ///< Human-readable label (e.g., "Finance/Banking").
    float       confidence = 0.0f; ///< Classification confidence [0.0, 1.0].
    std::string taxonomy;        ///< Taxonomy identifier.
};

// ---------------------------------------------------------------------------
// ContentClassificationRequest — input descriptor for a classify() call
// ---------------------------------------------------------------------------

struct ContentClassificationRequest {
    std::string content_id;
    std::string text;                              ///< Plain text to classify.
    std::string language = "en";                   ///< ISO 639-1 language code.
    std::vector<std::string> taxonomy_filters;     ///< Restrict output to these taxonomies.
    int         max_categories = 5;                ///< Maximum categories to return.
};

// ---------------------------------------------------------------------------
// ContentClassificationResult — output from a classify() call
// ---------------------------------------------------------------------------

struct ContentClassificationResult {
    std::string content_id;
    std::vector<ContentCategory> categories;
    bool        is_sensitive      = false;
    std::string primary_language;
    double      processing_ms     = 0.0;
};

// ---------------------------------------------------------------------------
// IContentClassifier — automated content classification interface
// ---------------------------------------------------------------------------

class IContentClassifier {
public:
    /**
     * @brief IContent Classifier.
     * @return Return value.
     */
    virtual ~IContentClassifier() = default;

    /**
     * @brief Classify the semantic intent of a query.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    virtual ContentClassificationResult classify(
        const ContentClassificationRequest& req
    ) = 0;

    /**
     * @brief Classify Batch.
     * @param[in] requests Input parameter.
     * @return Return value.
     */
    virtual std::vector<ContentClassificationResult> classifyBatch(
        const std::vector<ContentClassificationRequest>& requests
    ) = 0;

    [[nodiscard]] virtual std::vector<std::string> supportedTaxonomies() const = 0;

    [[nodiscard]] virtual std::vector<std::string> supportedLanguages() const = 0;

    [[nodiscard]] virtual bool isAvailable() const = 0;
};

} // namespace content
} // namespace themis
