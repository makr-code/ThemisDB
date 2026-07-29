/**
 * @file wikipedia_project_process.cpp
 * @brief Process coordination helpers for the Wikipedia import project.
 *
 * Utility functions for subprocess spawning, IPC, and pipeline
 * orchestration used by the Wikipedia import subsystem.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>

namespace themis::importers {

WikipediaProjectionSummary WikipediaIngestionPipeline::projectProcessDirtyPages() {
    WikipediaProjectionSummary summary;
    summary.relational_rows = relationalRowCount();
    if (snapshot_.dirty_pages.empty()) {
        summary.process_events = snapshot_.process_events.size();
        return summary;
    }

    snapshot_.process_events.erase(
        std::remove_if(snapshot_.process_events.begin(), snapshot_.process_events.end(),
            [this](const WikipediaProcessEvent& event) {
                return snapshot_.dirty_pages.count(event.page_id) > 0;
            }),
        snapshot_.process_events.end());

    for (const auto& [page_id, reason] : snapshot_.dirty_pages) {
        (void)reason;
        const auto page_it = snapshot_.pages.find(page_id);
        if (page_it == snapshot_.pages.end()) {
            continue;
        }
        auto revisions = revisionsForPage(page_id);
        auto events = WikipediaTransform::buildProcessEvents(page_it->second, revisions);
        snapshot_.process_events.insert(
            snapshot_.process_events.end(), events.begin(), events.end());
    }

    summary.process_events = snapshot_.process_events.size();
    return summary;
}

} // namespace themis::importers
