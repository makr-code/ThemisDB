/**
 * @file provenance_tracker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "training/auto_labeler.h"
#include "training/training_error_codes.h"
#include "training/training_exceptions.h"

#include <string>
#include <vector>
#include <memory>
#include <ctime>

namespace themis {

// Forward declaration – keeps the training header free of heavy query dependencies.
namespace query {
class QueryEngine;
}

namespace training {

/**
 * @brief Full provenance record attached to every accepted training sample.
 *
 * Carries the traceability chain required for compliance and audit: which
 * source document the sample came from, when it was extracted, which labeler
 * version produced it, its content modality, and fingerprints of the
 * enrichment AQL queries that added graph context.
 */
struct ProvenanceRecord {
    std::string source_doc_urn;       ///< URN of the originating source document
    std::time_t extraction_timestamp = 0; ///< Unix timestamp of extraction
    std::string labeler_version;      ///< auto_labeler build hash / version string
    std::string modality;             ///< Content modality ("text", "table", "citation", "ocr")
    std::vector<std::string> enrichment_query_fingerprints; ///< Hashes of AQL queries used
    std::string sample_id;            ///< Stable ID of the TrainingSample this record belongs to

    ProvenanceRecord() = default;
};

/**
 * @brief Lineage node returned when tracing a model back to its sources.
 */
struct LineageNode {
    std::string node_type;   ///< "model" | "sample" | "document"
    std::string node_id;     ///< Key / ID
    std::string label;       ///< Human-readable description
    std::vector<LineageNode> parents; ///< Upstream nodes (recursive)

    LineageNode() = default;
};

/**
 * @brief Write statistics for a provenance batch.
 */
struct ProvenanceWriteStats {
    size_t records_written    = 0;
    size_t records_rejected   = 0;  ///< Missing required fields
    double elapsed_seconds    = 0.0;

    ProvenanceWriteStats() = default;
};

/**
 * @brief Configuration for the provenance tracker.
 */
struct ProvenanceTrackerConfig {
    std::string graph_collection      = "TrainingSamples"; ///< AQL vertex collection
    std::string edge_collection       = "DerivedFrom";     ///< AQL edge collection
    size_t      batch_write_size      = 200;               ///< AQL bulk-insert batch size
    bool        reject_without_urn    = true;              ///< Reject samples without source URN
    bool        emit_audit_events     = true;              ///< Write to utils/audit_logger
    /// Maximum total wall-clock time allowed for a single write() call (milliseconds).
    /// 0 means no limit (unbounded).  When the deadline is exceeded the call
    /// returns early with whatever records were written so far.
    uint32_t    write_timeout_ms      = 0;

    ProvenanceTrackerConfig() = default;
};

/**
 * @brief End-to-end provenance and lineage tracker for training samples.
 *
 * Writes `TrainingSample` vertices and `DerivedFrom` edges to the AQL graph
 * so that every accepted sample is traceable back to its source document.
 * Integrates with `utils/audit_logger.cpp` to emit structured audit events
 * for samples filtered out by the confidence threshold.
 *
 * Example usage:
 * @code
 * ProvenanceTrackerConfig cfg;
 * cfg.graph_collection = "TrainingSamples";
 * cfg.emit_audit_events = true;
 *
 * ProvenanceTracker tracker(cfg, "arango://localhost:8529");
 * auto stats = tracker.write(records);
 * std::cout << stats.records_written << " provenance records written\n";
 *
 * // Trace a model prediction back to source documents
 * auto lineage = tracker.queryLineage("model_legal_v1", 10);
 * @endcode
 */
class ProvenanceTracker {
public:
    /**
     * @brief Construct the tracker.
     * @param config        Tracker configuration.
     * @param db_connection Database connection string (ArangoDB endpoint).
     * @param engine        Optional AQL query engine.  When non-null, write()
     *                      persists vertices and edges via AQL INSERT statements
     *                      and queryLineage() traverses the live graph via AQL.
     *                      Pass nullptr (the default) to operate in offline /
     *                      test mode, where the in-process store is used.
     */
    explicit ProvenanceTracker(const ProvenanceTrackerConfig& config,
                               const std::string& db_connection,
                               query::QueryEngine* engine = nullptr);

    ~ProvenanceTracker();

    // Non-copyable
    ProvenanceTracker(const ProvenanceTracker&)            = delete;
    ProvenanceTracker& operator=(const ProvenanceTracker&) = delete;

    /**
     * @brief Persist provenance records to the AQL graph.
     *
     * Writes a `TrainingSample` vertex and a `DerivedFrom` edge for each
     * record. Records without `source_doc_urn` are rejected when
     * `reject_without_urn` is true.
     *
     * @param records Provenance records to persist.
     * @return Write statistics.
     */
    ProvenanceWriteStats write(const std::vector<ProvenanceRecord>& records);

    /**
     * @brief Record an audit event for a sample filtered out by the confidence threshold.
     *
     * Emits a structured event to `utils/audit_logger` containing the sample
     * ID, category, confidence score, and applied threshold.
     *
     * @param sample_id       ID of the rejected sample.
     * @param category        Legal category of the sample.
     * @param confidence      Confidence score that caused rejection.
     * @param threshold_used  Category threshold applied at the time of rejection.
     */
    void recordFilteredSample(const std::string& sample_id,
                              const std::string& category,
                              float confidence,
                              float threshold_used);

    /**
     * @brief Query the lineage graph, tracing a model back to source documents.
     *
     * Traverses `DerivedFrom` edges up to @p max_hops hops starting from the
     * given model node, returning a recursive lineage tree.
     *
     * @param model_id  Model or adapter version to start the traversal from.
     * @param max_hops  Maximum number of edge hops (default 10).
     * @return Root lineage node for the model with populated parent chain.
     */
    LineageNode queryLineage(const std::string& model_id,
                             size_t max_hops = 10) const;

    /**
     * @brief Return the provenance record for a specific sample (if persisted).
     * @param sample_id Sample key.
     * @return Provenance record, or empty record if not found.
     */
    ProvenanceRecord getRecord(const std::string& sample_id) const;

    /**
     * @brief Inject or replace the AQL query engine after construction.
     *
     * Allows server bootstrap code to wire a live `QueryEngine` into an
     * already-constructed `ProvenanceTracker` without recreating it.
     * When @p engine is non-null, subsequent `write()` calls persist
     * vertices and edges via AQL INSERT, and `queryLineage()` traverses
     * the live graph via AQL.  Pass `nullptr` to revert to offline/test
     * mode (in-process store only).
     *
     * Thread safety: not thread-safe with respect to concurrent `write()`
     * or `queryLineage()` calls; call this method before first use or
     * while no other threads are accessing the tracker.
     *
     * @param engine Non-owning pointer to the AQL query engine; may be null.
     */
    void setQueryEngine(query::QueryEngine* engine);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace training
} // namespace themis
