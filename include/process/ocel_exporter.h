/**
 * @file ocel_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB – Process Modeling Module
 *
 * File:    ocel_exporter.h
 * Module:  include/process/
 * Purpose: OCEL 2.0 (Object-Centric Event Log) export of process
 *          instances for external process mining tools (PM4Py, Celonis,
 *          ProM).
 *
 * Scientific basis: Berti, A. et al. (2023). *OCEL 2.0 Specification.*
 * Process Mining Group, RWTH Aachen. doi:10.5281/zenodo.8428111.
 *
 * OCEL 2.0 JSON schema overview:
 * {
 *   "objectTypes": [{ "name": "...", "attributes": [...] }],
 *   "eventTypes":  [{ "name": "...", "attributes": [...] }],
 *   "objects":     [{ "id": "...", "type": "...", "attributes": [...] }],
 *   "events":      [{ "id": "...", "type": "...", "time": "...",
 *                     "attributes": [...], "relationships": [...] }]
 * }
 */

#pragma once

#include "index/process_graph.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// OcelExporter
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Exports ThemisDB process instances as OCEL 2.0 JSON event logs.
 *
 * Enables import into external process mining tools (PM4Py, Celonis, ProM)
 * for advanced analytics: object-centric discovery, conformance checking,
 * and process enhancement.
 *
 * Object types are derived from @c ProcessLinker attachment collections.
 * Events are derived from @c ProcessToken::visited_nodes and their
 * timestamps stored in @c ProcessToken::visit_timestamps.
 *
 * @code
 * OcelExporter exp(db, engine, models, linker);
 * auto log = exp.exportInstance("bauantrag-inst-001");
 * // log is OCEL 2.0 JSON — ready for PM4Py::ocel.read_ocel2_json()
 * @endcode
 */
class OcelExporter {
public:
    OcelExporter(
        RocksDBWrapper&      db,
        ProcessGraphManager& engine,
        ProcessModelManager& models,
        ProcessLinker&       linker
    );

    // ── Export API ────────────────────────────────────────────────────────

    /**
     * @brief Export a single process instance as an OCEL 2.0 JSON log.
     *
     * The returned document contains all events derived from token
     * traversal history and all attached objects as OCEL objects.
     *
     * @param instance_id  Process instance identifier.
     * @return             OCEL 2.0 JSON object; empty @c {} on error.
     */
    [[nodiscard]] nlohmann::json exportInstance(
        std::string_view instance_id
    ) const;

    /**
     * @brief Export all instances of a process model as a combined
     *        OCEL 2.0 JSON log.
     *
     * All instances that reference @p model_id are included in a single
     * event log, enabling cross-case analytics.
     *
     * @param model_id  Process model / definition identifier.
     * @return          OCEL 2.0 JSON; empty @c {} if no instances found.
     */
    [[nodiscard]] nlohmann::json exportModel(
        std::string_view model_id
    ) const;

    /**
     * @brief Export instances of a process model within a time window.
     *
     * @param model_id  Process model identifier.
     * @param from_ms   Start of time window (epoch milliseconds, inclusive).
     * @param to_ms     End of time window (epoch milliseconds, inclusive).
     * @return          OCEL 2.0 JSON; empty @c {} if no matching instances.
     */
    [[nodiscard]] nlohmann::json exportFiltered(
        std::string_view model_id,
        int64_t          from_ms,
        int64_t          to_ms
    ) const;

private:
    RocksDBWrapper&      db_;
    ProcessGraphManager& engine_;
    ProcessModelManager& models_;
    ProcessLinker&       linker_;

    /// Build the OCEL 2.0 event list from a process instance.
    [[nodiscard]] nlohmann::json buildEvents_(
        const ProcessInstance& inst
    ) const;

    /// Build the OCEL 2.0 object list from instance attachments.
    [[nodiscard]] nlohmann::json buildObjects_(
        std::string_view instance_id
    ) const;

    /// Derive the set of objectTypes from all objects.
    [[nodiscard]] static nlohmann::json deriveObjectTypes_(
        const nlohmann::json& objects
    );

    /// Derive the set of eventTypes from all events.
    [[nodiscard]] static nlohmann::json deriveEventTypes_(
        const nlohmann::json& events
    );

    /// Convert epoch ms to ISO-8601 timestamp string.
    [[nodiscard]] static std::string toIso8601_(int64_t epoch_ms);
};

} // namespace process
} // namespace themis
