#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis::importers {

using json = nlohmann::json;

/**
 * @brief Projection targets that can be rebuilt from the canonical Wikipedia core.
 */

/**
 * @file wikipedia_types.hpp
 * @brief Core data types for the Wikipedia import module.
 *
 * Defines WikipediaPage, WikipediaSection, WikipediaLink, and related
 * structures used throughout the Wikipedia import pipeline.
 */
enum class WikipediaProjectionModel {
    RELATIONAL_CORE,
    GRAPH,
    VECTOR,
    PROCESS,
    TIMESERIES
};

/**
 * @brief Logical description of a Wikimedia dump source.
 */
struct WikipediaDumpSource {
    std::string source_path;
    std::string source_id;
    std::string language = "en";
    std::string dump_date;
    std::string producer_hint;

    [[nodiscard]] json toJson() const {
        return json{
            {"source_path", source_path},
            {"source_id", source_id},
            {"language", language},
            {"dump_date", dump_date},
            {"producer_hint", producer_hint}
        };
    }
};

/**
 * @brief Canonical page row used as relational source of truth.
 */
struct WikipediaPageRecord {
    uint64_t page_id = 0;
    int ns = 0;
    std::string title;
    uint64_t latest_revision_id = 0;
    bool is_redirect = false;
    std::string redirect_title;
    std::string touched_at;
    std::string language = "en";
    std::string checksum;

    [[nodiscard]] json toJson() const {
        return json{
            {"page_id", page_id},
            {"ns", ns},
            {"title", title},
            {"latest_revision_id", latest_revision_id},
            {"is_redirect", is_redirect},
            {"redirect_title", redirect_title},
            {"touched_at", touched_at},
            {"language", language},
            {"checksum", checksum}
        };
    }
};

/**
 * @brief Canonical revision row.
 */
struct WikipediaRevisionRecord {
    uint64_t revision_id = 0;
    uint64_t page_id = 0;
    std::string timestamp;
    std::string sha1;
    std::string text;
    std::string comment;

    [[nodiscard]] json toJson() const {
        return json{
            {"revision_id", revision_id},
            {"page_id", page_id},
            {"timestamp", timestamp},
            {"sha1", sha1},
            {"text", text},
            {"comment", comment}
        };
    }
};

/**
 * @brief Canonical outgoing page-link row.
 */
struct WikipediaLinkRecord {
    uint64_t from_page_id = 0;
    std::string target_title;
    uint64_t target_page_id = 0;
    std::string link_type = "LINKS_TO";

    [[nodiscard]] json toJson() const {
        return json{
            {"from_page_id", from_page_id},
            {"target_title", target_title},
            {"target_page_id", target_page_id},
            {"link_type", link_type}
        };
    }
};

/**
 * @brief Canonical category membership row.
 */
struct WikipediaCategoryRecord {
    uint64_t page_id = 0;
    std::string category_title = {};

    [[nodiscard]] json toJson() const {
        return json{{"page_id", page_id}, {"category_title", category_title}};
    }
};

/**
 * @brief Canonical redirect row.
 */
struct WikipediaRedirectRecord {
    uint64_t from_page_id = 0;
    std::string target_title = {};

    [[nodiscard]] json toJson() const {
        return json{{"from_page_id", from_page_id}, {"target_title", target_title}};
    }
};

/**
 * @brief Dirty-page work queue entry for incremental projection refresh.
 */
struct WikipediaDirtyPageRecord {
    uint64_t page_id = 0;
    std::string reason = {};

    [[nodiscard]] json toJson() const {
        return json{{"page_id", page_id}, {"reason", reason}};
    }
};

/**
 * @brief Graph projection edge emitted from the canonical core.
 */
struct WikipediaGraphEdge {
    uint64_t from_page_id = 0;
    uint64_t to_page_id = 0;
    std::string target_title;
    std::string edge_type;

    [[nodiscard]] json toJson() const {
        return json{
            {"from_page_id", from_page_id},
            {"to_page_id", to_page_id},
            {"target_title", target_title},
            {"edge_type", edge_type}
        };
    }
};

/**
 * @brief Vector projection row that keeps embedding hooks vendor-neutral.
 */
struct WikipediaVectorRecord {
    uint64_t page_id = 0;
    std::string content;
    std::string embedding_model;
    bool pending_embedding = true;

    [[nodiscard]] json toJson() const {
        return json{
            {"page_id", page_id},
            {"content", content},
            {"embedding_model", embedding_model},
            {"pending_embedding", pending_embedding}
        };
    }
};

/**
 * @brief Process/event-log row for page and revision lifecycle activity.
 */
struct WikipediaProcessEvent {
    uint64_t page_id = 0;
    std::string event_type;
    std::string timestamp;
    json payload = json::object();

    [[nodiscard]] json toJson() const {
        return json{
            {"page_id", page_id},
            {"event_type", event_type},
            {"timestamp", timestamp},
            {"payload", payload}
        };
    }
};

/**
 * @brief Time-series projection row keyed by metric and time bucket.
 */
struct WikipediaTimeSeriesMetric {
    uint64_t page_id = 0;
    std::string metric;
    std::string bucket;
    double value = 0.0;

    [[nodiscard]] json toJson() const {
        return json{
            {"page_id", page_id},
            {"metric", metric},
            {"bucket", bucket},
            {"value", value}
        };
    }
};

/**
 * @brief Dead-letter/error-sink row retained for best-effort ingestion mode.
 */
struct WikipediaDeadLetterRecord {
    std::string source;
    std::string reason;
    std::string raw_snippet;

    [[nodiscard]] json toJson() const {
        return json{
            {"source", source},
            {"reason", reason},
            {"raw_snippet", raw_snippet}
        };
    }
};

/**
 * @brief Parsed page payload handed from the streaming reader to the pipeline.
 */
struct WikipediaParsedPage {
    WikipediaPageRecord page;
    WikipediaRevisionRecord revision;
    std::string raw_xml;
};

/**
 * @brief In-memory, portable representation of the canonical core and all projections.
 */
struct WikipediaDatasetSnapshot {
    std::map<uint64_t, WikipediaPageRecord> pages;
    std::map<uint64_t, WikipediaRevisionRecord> revisions;
    std::vector<WikipediaLinkRecord> links;
    std::vector<WikipediaCategoryRecord> categories;
    std::vector<WikipediaRedirectRecord> redirects;
    std::vector<WikipediaDeadLetterRecord> dead_letters;
    std::vector<WikipediaGraphEdge> graph_edges;
    std::vector<WikipediaVectorRecord> vector_records;
    std::vector<WikipediaProcessEvent> process_events;
    std::vector<WikipediaTimeSeriesMetric> timeseries_metrics;
    std::map<uint64_t, std::string> dirty_pages;

    [[nodiscard]] json toJson() const {
        json pages_json = json::array();
        for (const auto& [_, row] : pages) {
            pages_json.push_back(row.toJson());
        }

        json revisions_json = json::array();
        for (const auto& [_, row] : revisions) {
            revisions_json.push_back(row.toJson());
        }

        json links_json = json::array();
        for (const auto& row : links) {
            links_json.push_back(row.toJson());
        }

        json categories_json = json::array();
        for (const auto& row : categories) {
            categories_json.push_back(row.toJson());
        }

        json redirects_json = json::array();
        for (const auto& row : redirects) {
            redirects_json.push_back(row.toJson());
        }

        json dead_letters_json = json::array();
        for (const auto& row : dead_letters) {
            dead_letters_json.push_back(row.toJson());
        }

        json graph_json = json::array();
        for (const auto& row : graph_edges) {
            graph_json.push_back(row.toJson());
        }

        json vector_json = json::array();
        for (const auto& row : vector_records) {
            vector_json.push_back(row.toJson());
        }

        json process_json = json::array();
        for (const auto& row : process_events) {
            process_json.push_back(row.toJson());
        }

        json timeseries_json = json::array();
        for (const auto& row : timeseries_metrics) {
            timeseries_json.push_back(row.toJson());
        }

        json dirty_json = json::array();
        for (const auto& [page_id, reason] : dirty_pages) {
            dirty_json.push_back(WikipediaDirtyPageRecord{page_id, reason}.toJson());
        }

        return json{
            {"pages", pages_json},
            {"revisions", revisions_json},
            {"links", links_json},
            {"categories", categories_json},
            {"redirects", redirects_json},
            {"dead_letters", dead_letters_json},
            {"graph_edges", graph_json},
            {"vector_records", vector_json},
            {"process_events", process_json},
            {"timeseries_metrics", timeseries_json},
            {"dirty_pages", dirty_json}
        };
    }
};

/**
 * @brief Summary counters returned after a projection refresh.
 */
struct WikipediaProjectionSummary {
    size_t relational_rows = 0;
    size_t graph_edges = 0;
    size_t vector_records = 0;
    size_t process_events = 0;
    size_t timeseries_points = 0;
    size_t dirty_pages_cleared = 0;

    [[nodiscard]] json toJson() const {
        return json{
            {"relational_rows", relational_rows},
            {"graph_edges", graph_edges},
            {"vector_records", vector_records},
            {"process_events", process_events},
            {"timeseries_points", timeseries_points},
            {"dirty_pages_cleared", dirty_pages_cleared}
        };
    }
};

/**
 * @brief Verification result for the portable Wikipedia database artifact.
 */
struct WikipediaValidationReport {
    bool success = true;
    size_t orphan_revisions = 0;
    size_t dangling_redirects = 0;
    size_t dead_letters = 0;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    [[nodiscard]] json toJson() const {
        return json{
            {"success", success},
            {"orphan_revisions", orphan_revisions},
            {"dangling_redirects", dangling_redirects},
            {"dead_letters", dead_letters},
            {"warnings", warnings},
            {"errors", errors}
        };
    }
};

/**
 * @brief Portable export manifest written next to the wikipedia.db artifact.
 */
struct WikipediaManifest {
    std::string dump_source;
    std::string importer_version;
    std::string generated_at;
    std::map<std::string, size_t> row_counts;
    std::map<std::string, std::string> checksums;
    std::vector<std::string> external_tool_references;

    [[nodiscard]] json toJson() const {
        return json{
            {"dump_source", dump_source},
            {"importer_version", importer_version},
            {"generated_at", generated_at},
            {"row_counts", row_counts},
            {"checksums", checksums},
            {"external_tool_references", external_tool_references}
        };
    }
};

} // namespace themis::importers
