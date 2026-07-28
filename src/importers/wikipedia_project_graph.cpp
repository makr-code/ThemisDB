/**
 * @file wikipedia_project_graph.cpp
 * @brief Graph-model projection for Wikipedia article relationships.
 *
 * Builds a ThemisDB graph structure from Wikipedia link and category
 * relationships extracted during the import pipeline.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>
#include <map>

namespace themis::importers {

WikipediaProjectionSummary WikipediaIngestionPipeline::projectGraphDirtyPages() {
    WikipediaProjectionSummary summary;
    summary.relational_rows = relationalRowCount();
    if (snapshot_.dirty_pages.empty()) {
        summary.graph_edges = snapshot_.graph_edges.size();
        return summary;
    }

    snapshot_.graph_edges.erase(
        std::remove_if(snapshot_.graph_edges.begin(), snapshot_.graph_edges.end(),
            [this](const WikipediaGraphEdge& edge) {
                return snapshot_.dirty_pages.count(edge.from_page_id) > 0;
            }),
        snapshot_.graph_edges.end());

    std::map<std::string, uint64_t> title_index;
    for (const auto& [page_id, page] : snapshot_.pages) {
        title_index[WikipediaTransform::normalizeTitle(page.title)] = page_id;
    }

    for (const auto& link : snapshot_.links) {
        if (snapshot_.dirty_pages.count(link.from_page_id) == 0) {
            continue;
        }
        WikipediaGraphEdge edge;
        edge.from_page_id = link.from_page_id;
        edge.target_title = link.target_title;
        edge.edge_type = link.link_type;
        const auto title_it = title_index.find(WikipediaTransform::normalizeTitle(link.target_title));
        if (title_it != title_index.end()) {
            edge.to_page_id = title_it->second;
        }
        snapshot_.graph_edges.push_back(edge);
    }

    for (const auto& category : snapshot_.categories) {
        if (snapshot_.dirty_pages.count(category.page_id) == 0) {
            continue;
        }
        WikipediaGraphEdge edge;
        edge.from_page_id = category.page_id;
        edge.target_title = category.category_title;
        edge.edge_type = "IN_CATEGORY";
        snapshot_.graph_edges.push_back(edge);
    }

    for (const auto& redirect : snapshot_.redirects) {
        if (snapshot_.dirty_pages.count(redirect.from_page_id) == 0) {
            continue;
        }
        WikipediaGraphEdge edge;
        edge.from_page_id = redirect.from_page_id;
        edge.target_title = redirect.target_title;
        edge.edge_type = "REDIRECTS_TO";
        const auto title_it = title_index.find(WikipediaTransform::normalizeTitle(redirect.target_title));
        if (title_it != title_index.end()) {
            edge.to_page_id = title_it->second;
        }
        snapshot_.graph_edges.push_back(edge);
    }

    summary.graph_edges = snapshot_.graph_edges.size();
    return summary;
}

} // namespace themis::importers
