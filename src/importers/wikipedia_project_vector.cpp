/**
 * @file wikipedia_project_vector.cpp
 * @brief Vector embedding projection for Wikipedia articles.
 *
 * Orchestrates embedding inference for Wikipedia article text segments
 * and writes the resulting vectors to the configured ThemisDB vector index.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>

namespace themis::importers {

WikipediaProjectionSummary WikipediaIngestionPipeline::projectVectorDirtyPages() {
    WikipediaProjectionSummary summary;
    summary.relational_rows = relationalRowCount();
    if (snapshot_.dirty_pages.empty()) {
        summary.vector_records = snapshot_.vector_records.size();
        return summary;
    }

    snapshot_.vector_records.erase(
        std::remove_if(snapshot_.vector_records.begin(), snapshot_.vector_records.end(),
            [this](const WikipediaVectorRecord& record) {
                return snapshot_.dirty_pages.count(record.page_id) > 0;
            }),
        snapshot_.vector_records.end());

    for (const auto& [page_id, reason] : snapshot_.dirty_pages) {
        (void)reason;
        const auto page_it = snapshot_.pages.find(page_id);
        if (page_it == snapshot_.pages.end()) {
            continue;
        }
        const auto revision_it = snapshot_.revisions.find(page_it->second.latest_revision_id);
        if (revision_it == snapshot_.revisions.end()) {
            continue;
        }
        auto records = WikipediaTransform::buildVectorRecords(
            page_it->second,
            revision_it->second,
            config_.embedding.model,
            config_.embedding.enabled);
        snapshot_.vector_records.insert(
            snapshot_.vector_records.end(), records.begin(), records.end());
    }

    summary.vector_records = snapshot_.vector_records.size();
    return summary;
}

} // namespace themis::importers
