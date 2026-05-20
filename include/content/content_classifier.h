/*
 * ThemisDB | File: content_classifier.h | Version: 0.1.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 129
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file content_classifier.h
 * @brief Automated category tagging interface for content items.
 *
 * IContentClassifier assigns taxonomy categories and sensitivity flags to
 * text content, supporting multiple classification taxonomies (IAB, NAICS,
 * custom) and ISO 639-1 language filtering.
 *
 * Typical use: classify content before indexing to enrich search metadata
 * and apply data-governance policies.
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

/**
 * @brief A single classification label assigned to a piece of content.
 *
 * `confidence` is a probability in [0.0, 1.0].  `taxonomy` identifies the
 * classification scheme used (e.g., "IAB-QAG", "NAICS-2022", "custom").
 */
struct ContentCategory {
    std::string category_id;
    std::string label;           ///< Human-readable label (e.g., "Finance/Banking").
    float       confidence = 0.0f; ///< Classification confidence [0.0, 1.0].
    std::string taxonomy;        ///< Taxonomy identifier.
};

// ---------------------------------------------------------------------------
// ContentClassificationRequest — input descriptor for a classify() call
// ---------------------------------------------------------------------------

/**
 * @brief Request descriptor for a single content classification.
 */
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

/**
 * @brief Result of classifying a single content item.
 *
 * `is_sensitive` is set when any returned category carries a sensitivity flag
 * (e.g., medical, legal, adult) or when `confidence` exceeds a classifier-
 * specific threshold.
 */
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

/**
 * @brief Pure-virtual interface for content category classification.
 *
 * Implementations wrap ML inference engines (e.g., FastText, transformer
 * classifiers, cloud NLP APIs).
 *
 * ### Thread safety
 * All methods must be safe to call concurrently from multiple threads.
 *
 * ### Batch semantics
 * `classifyBatch()` may process requests in parallel; result order matches
 * input order.
 */
class IContentClassifier {
public:
    virtual ~IContentClassifier() = default;

    /**
     * @brief Classify a single content item.
     */
    virtual ContentClassificationResult classify(
        const ContentClassificationRequest& req
    ) = 0;

    /**
     * @brief Classify multiple content items.
     *
     * Result vector has the same size and order as @p requests.
     */
    virtual std::vector<ContentClassificationResult> classifyBatch(
        const std::vector<ContentClassificationRequest>& requests
    ) = 0;

    /// Return the taxonomy identifiers supported by this classifier.
    [[nodiscard]] virtual std::vector<std::string> supportedTaxonomies() const = 0;

    /// Return the ISO 639-1 language codes supported by this classifier.
    [[nodiscard]] virtual std::vector<std::string> supportedLanguages() const = 0;

    /// Return `false` if the underlying model or service is unavailable.
    [[nodiscard]] virtual bool isAvailable() const = 0;
};

} // namespace content
} // namespace themis
