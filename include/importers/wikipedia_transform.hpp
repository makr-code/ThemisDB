#pragma once

#include "importers/wikipedia_types.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis::importers {

/**
 * @brief Wikipedia-specific normalization and multi-model transformation helpers.
 */

/**
 * @file wikipedia_transform.hpp
 * @brief Transformation stage for Wikipedia article records.
 *
 * Converts raw WikipediaPage records into ThemisDB document format,
 * performing wikitext stripping, entity extraction, and embedding requests.
 */
class WikipediaTransform {
public:
    [[nodiscard]] static std::string normalizeTitle(std::string_view title);
    [[nodiscard]] static std::string decodeXmlEntities(std::string_view text);
    [[nodiscard]] static std::string canonicalizeText(std::string_view text);
    [[nodiscard]] static std::string checksumHex(std::string_view text);
    [[nodiscard]] static std::vector<WikipediaLinkRecord> extractLinks(
        const WikipediaPageRecord& page,
        const WikipediaRevisionRecord& revision);
    [[nodiscard]] static std::vector<WikipediaCategoryRecord> extractCategories(
        const WikipediaPageRecord& page,
        const WikipediaRevisionRecord& revision);
    [[nodiscard]] static std::optional<WikipediaRedirectRecord> extractRedirect(
        const WikipediaPageRecord& page,
        const WikipediaRevisionRecord& revision);
    [[nodiscard]] static std::vector<WikipediaProcessEvent> buildProcessEvents(
        const WikipediaPageRecord& page,
        const std::vector<WikipediaRevisionRecord>& revisions);
    [[nodiscard]] static std::vector<WikipediaTimeSeriesMetric> buildTimeSeriesMetrics(
        const WikipediaPageRecord& page,
        const std::vector<WikipediaRevisionRecord>& revisions);
    [[nodiscard]] static std::vector<WikipediaVectorRecord> buildVectorRecords(
        const WikipediaPageRecord& page,
        const WikipediaRevisionRecord& revision,
        const std::string& embedding_model,
        bool embedding_enabled);
};

} // namespace themis::importers
