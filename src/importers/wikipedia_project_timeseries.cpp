/**
 * @file wikipedia_project_timeseries.cpp
 * @brief Time-series projection for Wikipedia edit history.
 *
 * Extracts revision timestamps and edit-frequency metrics from the
 * Wikipedia dump and stores them as ThemisDB time-series records.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>

namespace themis::importers {

WikipediaProjectionSummary WikipediaIngestionPipeline::projectTimeSeriesDirtyPages() {
    WikipediaProjectionSummary summary;
    summary.relational_rows = relationalRowCount();
    if (snapshot_.dirty_pages.empty()) {
        summary.timeseries_points = snapshot_.timeseries_metrics.size();
        return summary;
    }

    snapshot_.timeseries_metrics.erase(
        std::remove_if(snapshot_.timeseries_metrics.begin(), snapshot_.timeseries_metrics.end(),
            [this](const WikipediaTimeSeriesMetric& metric) {
                return snapshot_.dirty_pages.count(metric.page_id) > 0;
            }),
        snapshot_.timeseries_metrics.end());

    for (const auto& [page_id, reason] : snapshot_.dirty_pages) {
        (void)reason;
        const auto page_it = snapshot_.pages.find(page_id);
        if (page_it == snapshot_.pages.end()) {
            continue;
        }
        auto revisions = revisionsForPage(page_id);
        auto metrics = WikipediaTransform::buildTimeSeriesMetrics(page_it->second, revisions);
        snapshot_.timeseries_metrics.insert(
            snapshot_.timeseries_metrics.end(), metrics.begin(), metrics.end());
    }

    summary.timeseries_points = snapshot_.timeseries_metrics.size();
    return summary;
}

} // namespace themis::importers
