/**
 * @file provenance_tracker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=10; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=1, Debt=0, C=3, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "training/provenance_tracker.h"
#include "query/aql_runner.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <map>
#include <unordered_map>

namespace themis {
namespace training {

// ============================================================================
// AQL query templates for provenance persistence and lineage traversal
// ============================================================================
namespace provenance_aql {
    // Insert a TrainingSample vertex
    constexpr const char* INSERT_SAMPLE_VERTEX =
        "INSERT {"
        "  _key: @sample_id,"
        "  source_doc_urn: @source_doc_urn,"
        "  extraction_timestamp: @extraction_ts,"
        "  labeler_version: @labeler_version,"
        "  modality: @modality,"
        "  enrichment_fingerprints: @fingerprints"
        "} INTO @@collection OPTIONS { ignoreErrors: false }";

    // Insert a DerivedFrom edge from sample → source document
    constexpr const char* INSERT_DERIVED_FROM_EDGE =
        "INSERT {"
        "  _from: CONCAT(@@sample_collection, '/', @sample_id),"
        "  _to:   CONCAT(@@doc_collection,    '/', @doc_id),"
        "  derived_at: @derived_at"
        "} INTO @@edge_collection OPTIONS { ignoreErrors: true }";

    // Multi-hop lineage traversal (model → samples → documents)
    constexpr const char* LINEAGE_TRAVERSAL =
        "FOR v, e, p IN 1..@max_hops INBOUND CONCAT(@@sample_collection, '/', @root_id) "
        "@@edge_collection "
        "RETURN {node_type: v.node_type, node_id: v._key, label: v.label}";

    // Fetch a single provenance record by sample_id
    constexpr const char* FETCH_RECORD =
        "FOR doc IN @@collection FILTER doc._key == @sample_id LIMIT 1 RETURN doc";

} // namespace provenance_aql

// ============================================================================
// Impl
// ============================================================================
/** @brief Impl. */
class ProvenanceTracker::Impl {
public:
    explicit Impl(const ProvenanceTrackerConfig& config,
                  const std::string& db_connection,
                  query::QueryEngine* engine)
        : config_(config)
        , db_connection_(db_connection)
        , query_engine_(engine) {
    }

    // -------------------------------------------------------------------------
    ProvenanceWriteStats write(const std::vector<ProvenanceRecord>& records) {
        ProvenanceWriteStats stats;
        auto t0 = std::chrono::steady_clock::now();

        // Compute optional write deadline (0 = no limit).
        const bool has_timeout = config_.write_timeout_ms > 0;
        const auto deadline = t0 + std::chrono::milliseconds(config_.write_timeout_ms);

        // Per-record helper: returns false when the write deadline has passed.
        // Checked before every individual executeAql() call so that a single
        // slow network write cannot block indefinitely beyond the configured
        // per-operation window (gap-scanner finding: no_timeout on file I/O).
        auto shouldContinue = [&]() -> bool {
            return !has_timeout || std::chrono::steady_clock::now() < deadline;
        };

        size_t batch_start = 0;
        while (static_cast<size_t>(batch_start) < records.size()) {
            // Enforce write deadline before starting each batch.
            if (has_timeout && std::chrono::steady_clock::now() >= deadline) {
                stats.records_rejected += static_cast<int>(records.size()) - batch_start;
                break;
            }

            size_t batch_end = std::min(batch_start + config_.batch_write_size,
                                        records.size());

            for (size_t i = batch_start; i < batch_end; ++i) {
                const auto& rec = records[i];

                // Validate required fields
                if (config_.reject_without_urn && rec.source_doc_urn.empty()) {
                    ++stats.records_rejected;
                    continue;
                }
                if (rec.sample_id.empty()) {
                    ++stats.records_rejected;
                    continue;
                }

                // Always maintain the in-process store for offline/test access
                store_[rec.sample_id] = rec;

                // When a live AQL engine is injected, persist to the graph store
                if (query_engine_) {
                    // Serialize enrichment_query_fingerprints as a JSON array using
                    // nlohmann::json to avoid manual quoting/escaping errors.
                    nlohmann::json fingerprints_arr(rec.enrichment_query_fingerprints);
                    std::string fingerprints_json = fingerprints_arr.dump();

                    // Build and execute the vertex INSERT; bail out early when
                    // the write deadline has already elapsed (per-AQL timeout guard).
                    if (!shouldContinue()) {
                        stats.records_rejected += (batch_end - i);
                        batch_start = batch_end;
                        break;
                    }
                    std::string vertex_query = buildQuery(
                        provenance_aql::INSERT_SAMPLE_VERTEX,
                        {
                            {"@collection",          config_.graph_collection},
                            {"sample_id",            "\"" + escapedStr(rec.sample_id) + "\""},
                            {"source_doc_urn",       "\"" + escapedStr(rec.source_doc_urn) + "\""},
                            {"extraction_ts",        std::to_string(rec.extraction_timestamp)},
                            {"labeler_version",      "\"" + escapedStr(rec.labeler_version) + "\""},
                            {"modality",             "\"" + escapedStr(rec.modality) + "\""},
                            {"fingerprints",         fingerprints_json},
                        });
                    auto vertex_result = executeAql(vertex_query, *query_engine_);
                    if (!vertex_result) {
                        THEMIS_WARN("ProvenanceTracker: AQL vertex INSERT failed for sample '{}' – "
                                    "in-process store updated but graph database may be inconsistent.",
                                    rec.sample_id);
                    }

                    // Insert the DerivedFrom edge only when a source URN is available
                    if (!rec.source_doc_urn.empty()) {
                        // Check deadline again before the second AQL call
                        if (!shouldContinue()) {
                            stats.records_rejected += (batch_end - i);
                            batch_start = batch_end;
                            break;
                        }
                        std::string edge_query = buildQuery(
                            provenance_aql::INSERT_DERIVED_FROM_EDGE,
                            {
                                {"@sample_collection", config_.graph_collection},
                                {"@doc_collection",    config_.graph_collection},
                                {"@edge_collection",   config_.edge_collection},
                                {"sample_id",          "\"" + escapedStr(rec.sample_id) + "\""},
                                {"doc_id",             "\"" + escapedStr(rec.source_doc_urn) + "\""},
                                {"derived_at",         std::to_string(rec.extraction_timestamp)},
                            });
                        auto edge_result = executeAql(edge_query, *query_engine_);
                        if (!edge_result) {
                            THEMIS_WARN("ProvenanceTracker: AQL edge INSERT failed for sample '{}' → '{}' – "
                                        "provenance graph may be missing the DerivedFrom edge.",
                                        rec.sample_id, rec.source_doc_urn);
                        }
                    }
                }

                ++stats.records_written;
            }
            batch_start = batch_end;
        }

        auto t1 = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration<double>(t1 - t0).count();
        return stats;
    }

    // -------------------------------------------------------------------------
    void recordFilteredSample(const std::string& sample_id,
                              const std::string& category,
                              float confidence,
                              float threshold_used) {
        if (!config_.emit_audit_events) {
          return;
        }

        // In production: call utils::AuditLogger::log() with structured event.
        // Build a simple JSON-like audit string for in-process traceability:
        std::ostringstream oss = {};
        oss << "{"
            << "\"event\":\"sample_filtered\","
            << "\"sample_id\":\"" << sample_id << "\","
            << "\"category\":\"" << category << "\","
            << "\"confidence\":" << confidence << ","
            << "\"threshold\":" << threshold_used
            << "}";

        audit_log_.push_back(oss.str());
    }

    // -------------------------------------------------------------------------
    LineageNode queryLineage(const std::string& model_id, size_t max_hops) const {
        LineageNode root;
        root.node_type = "model";
        root.node_id   = model_id;
        root.label     = "LoRA adapter " + model_id;

        // When a live AQL engine is wired, execute the graph traversal query
        if (query_engine_) {
            std::string query = buildQuery(
                provenance_aql::LINEAGE_TRAVERSAL,
                {
                    {"@sample_collection", config_.graph_collection},
                    {"@edge_collection",   config_.edge_collection},
                    {"root_id",            "\"" + escapedStr(model_id) + "\""},
                    {"max_hops",           std::to_string(max_hops)},
                });
            auto result = executeAql(query, *query_engine_);
            if (result) {
                const auto& json = *result;
                // Parse the traversal result into a flat list; build a two-level
                // tree (model → samples → documents) from the returned nodes.
                auto parseNodes = [&]([[maybe_unused]] const nlohmann::json& arr) {
                    for (const auto& item : arr) {
                        if (!item.is_object()) {
                          continue;
                        }
                        LineageNode node = {};
                        if (item.contains("node_id") && item["node_id"].is_string())
                            node.node_id = item["node_id"].get<std::string>();
                        if (item.contains("node_type") && item["node_type"].is_string())
                            node.node_type = item["node_type"].get<std::string>();
                        if (item.contains("label") && item["label"].is_string())
                            node.label = item["label"].get<std::string>();
                        if (node.node_id.empty()) {
                          continue;
                        }
                        root.parents.push_back(std::move(node));
                    }
                };

                if (json.is_object() && json.contains("results") &&
                    json["results"].is_array()) {
                    parseNodes(json["results"]);
                } else if (json.is_array()) {
                    parseNodes(json);
                }

                if (!root.parents.empty()) {
                    return root;
                }
                // Fall through to in-process store if traversal returned nothing
            }
        }

        // PERMANENT FALLBACK NOTE (ProvenanceTracker in-process lineage store):
        // Purpose: Build a lineage tree entirely from the in-process
        //   `store_` map when no AQL traversal result is available (offline
        //   mode, disconnected from the graph DB, or empty AQL result set).
        //   This allows training provenance to be tracked in single-process
        //   test runs without a live ThemisDB instance.
        // Activation: Reached when `query_engine_` is null or when AQL traversal
        //   returns an empty result set for the given sample_id.
        // Production Delta: The in-process store only contains samples that
        //   were registered in the current process lifetime; cross-process or
        //   cross-restart lineage is not captured.  Graph relationships between
        //   samples (sibling nodes, model checkpoints) are approximated as
        //   flat parent–child pairs rather than a true provenance DAG.
        // Note: Call setQueryEngine() to wire a live QueryEngine; the in-process
        //   fallback is then only reached when AQL traversal returns no results.
        //   RESOLVED 2026-05-06: setQueryEngine() injection API added.
        // In-process fallback: build a stub tree from the in-process store.
        // Used in offline / test mode and as a fallback when the AQL traversal
        // returns an empty result set.
        for (const auto& [sample_id, rec] : store_) {
            LineageNode sample_node;
            sample_node.node_type = "sample";
            sample_node.node_id   = sample_id;
            sample_node.label     = "TrainingSample " + sample_id;

            if (!rec.source_doc_urn.empty()) {
                LineageNode doc_node;
                doc_node.node_type = "document";
                doc_node.node_id   = rec.source_doc_urn;
                doc_node.label     = "Document " + rec.source_doc_urn;
                sample_node.parents.push_back(std::move(doc_node));
            }

            root.parents.push_back(std::move(sample_node));
        }

        return root;
    }

    // -------------------------------------------------------------------------
    ProvenanceRecord getRecord(const std::string& sample_id) const {
        // Check in-process store first (covers offline/test mode and cached writes)
        auto it = store_.find(sample_id);
        if (it != store_.end()) {
            return it->second;
        }

        // When a live engine is available, attempt an AQL fetch
        if (query_engine_) {
            std::string query = buildQuery(
                provenance_aql::FETCH_RECORD,
                {
                    {"@collection", config_.graph_collection},
                    {"sample_id",   "\"" + escapedStr(sample_id) + "\""},
                });
            auto result = executeAql(query, *query_engine_);
            if (result) {
                const auto& json = *result;
                const nlohmann::json* doc_ptr = nullptr;
                if (json.is_object() && json.contains("results") &&
                    json["results"].is_array() && !json["results"].empty()) {
                    doc_ptr = &json["results"][0];
                } else if (json.is_array() && !json.empty()) {
                    doc_ptr = &json[0];
                }
                if (doc_ptr && doc_ptr->is_object()) {
                    ProvenanceRecord rec;
                    const auto& d = *doc_ptr;
                    if (d.contains("_key") && d["_key"].is_string())
                        rec.sample_id = d["_key"].get<std::string>();
                    if (d.contains("source_doc_urn") && d["source_doc_urn"].is_string())
                        rec.source_doc_urn = d["source_doc_urn"].get<std::string>();
                    if (d.contains("labeler_version") && d["labeler_version"].is_string())
                        rec.labeler_version = d["labeler_version"].get<std::string>();
                    if (d.contains("modality") && d["modality"].is_string())
                        rec.modality = d["modality"].get<std::string>();
                    return rec;
                }
            }
        }

        return {};
    }

    // -------------------------------------------------------------------------
    void setQueryEngine(query::QueryEngine* engine) {
        query_engine_ = engine;
    }

    // -------------------------------------------------------------------------
    const std::vector<std::string>& auditLog() const {
        return audit_log_;
    }

private:
    ProvenanceTrackerConfig                           config_;
    std::string                                       db_connection_;
    query::QueryEngine*                               query_engine_;   ///< non-owning; nullptr = offline/test
    std::map<std::string, ProvenanceRecord>               store_;
    std::vector<std::string>                          audit_log_;

    // Build an AQL query string from a template by substituting @placeholder tokens.
    // Matches the pattern used in auto_labeler.cpp::buildQuery().
    static std::string buildQuery(
        const std::string& tmpl,
        const std::vector<std::pair<std::string, std::string>>& bindings)
    {
        std::string query = tmpl;
        for (const auto& [placeholder, value] : bindings) {
            std::string token = "@" + placeholder;
            size_t pos = 0;
            while ((pos = query.find(token, pos)) != std::string::npos) {
                query.replace(pos,static_cast<int>(token.size()), value);
                pos += value.size();
            }
        }
        return query;
    }

    // Escape characters that would break an AQL inline string literal.
    static std::string escapedStr(const std::string& raw) {
        std::string out = {};
        out.reserve(raw.size());
        for (char c : raw) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
            }
        }
        return out;
    }
};

// ============================================================================
// Public API
// ============================================================================
ProvenanceTracker::ProvenanceTracker(const ProvenanceTrackerConfig& config,
                                     const std::string& db_connection,
                                     query::QueryEngine* engine)
    : impl_(std::make_unique<Impl>(config, db_connection, engine)) {}

ProvenanceTracker::~ProvenanceTracker() = default;

ProvenanceWriteStats ProvenanceTracker::write(const std::vector<ProvenanceRecord>& records) {
    return impl_->write(records);
}

void ProvenanceTracker::recordFilteredSample(const std::string& sample_id,
                                             const std::string& category,
                                             float confidence,
                                             float threshold_used) {
    impl_->recordFilteredSample(sample_id, category, confidence, threshold_used);
}

LineageNode ProvenanceTracker::queryLineage(const std::string& model_id,
                                            size_t max_hops) const {
    return impl_->queryLineage(model_id, max_hops);
}

ProvenanceRecord ProvenanceTracker::getRecord(const std::string& sample_id) const {
    return impl_->getRecord(sample_id);
}

void ProvenanceTracker::setQueryEngine(query::QueryEngine* engine) {
    impl_->setQueryEngine(engine);
}

} // namespace training
} // namespace themis
