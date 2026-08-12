/**
 * @file object_centric_tracer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB - Process Modeling Module
 *
 * File:    object_centric_tracer.h
 * Module:  include/process/
 * Purpose: Object-Centric Process Mining (OCPM) — OCEL 2.0 log builder,
 *          Directly-Follows Multigraph, and convergence/divergence analysis.
 *          P6 implementation (van der Aalst 2022).
 */

#pragma once

#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace themis {
namespace process {

// ─────────────────────────────────────────────────────────────────────────────
// OcelEvent
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief In-memory representation of a single OCEL 2.0 event.
 *
 * Each @c ProcessAttachment maps to exactly one @c OcelEvent.
 */
struct OcelEvent {
    std::string event_id;                                           ///< "attach:<inst>:<obj>"
    std::string activity;                                           ///< toString(link_type)
    int64_t     timestamp_ms{0};                                    ///< attached_at_ms
    std::map<std::string, std::vector<std::string>> object_refs; ///< {type→[ids]}
    nlohmann::json attributes;                                      ///< Additional fields
};

// ─────────────────────────────────────────────────────────────────────────────
// ObjectCentricTracer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Object-Centric Process Mining façade.
 *
 * Provides three core capabilities:
 *
 * 1. **buildOcelLog()** — Convert process instance attachments into an
 *    OCEL 2.0 compatible JSON event log.
 *
 * 2. **computeDfmg()** — Build the Directly-Follows Multigraph (DFMG) for
 *    a given object type across a process model.
 *
 * 3. **analyze()** — Identify convergence (many→one) and divergence (one→many)
 *    nodes by object type.
 *
 * @par Performance
 * @c computeDfmg() must handle 10,000 events in < 5 s (O(n) frequency map).
 *
 * @see ProcessLinker, ProcessModelManager
 */
class ObjectCentricTracer {
public:
    /**
     * @param linker         ProcessLinker to retrieve attachments.
     * @param model_manager  ProcessModelManager to load model definitions.
     */
    explicit ObjectCentricTracer(
        ProcessLinker&       linker,
        ProcessModelManager& model_manager
    );

    /**
     * @brief Build an OCEL 2.0 compatible JSON log for a process instance.
     *
     * Format:
     * @code{.json}
     * {
     *   "ocel:global-log": {
     *     "ocel:attribute-names": [],
     *     "ocel:object-types": ["documents", "metadata", ...]
     *   },
     *   "ocel:events": [
     *     {
     *       "ocel:id": "...",
     *       "ocel:activity": "HAS_DOCUMENT",
     *       "ocel:timestamp": 1234567890,
     *       "ocel:omap": {"documents": ["doc-1"]},
     *       "ocel:vmap": {}
     *     }, ...
     *   ],
     *   "ocel:objects": {
     *     "doc-1": {"ocel:type": "documents"},
     *     ...
     *   }
     * }
     * @endcode
     *
     * @param instance_id  Process instance identifier.
     * @return OCEL 2.0 JSON object.
     */
    [[nodiscard]] nlohmann::json buildOcelLog(std::string_view instance_id) const;

    /**
     * @brief Compute the Directly-Follows Multigraph (DFMG) for a given
     *        object type within a process model.
     *
     * Loads the process model, iterates all edges, and accumulates arcs with
     * their frequency. Runs in O(n) over the edge list.
     *
     * @param model_id     Process model identifier.
     * @param object_type  Object type (collection name) to trace.
     * @return JSON:
     * @code{.json}
     * {
     *   "object_type": "documents",
     *   "nodes": ["node-A", "node-B", ...],
     *   "arcs": [{"from": "node-A", "to": "node-B", "frequency": 3}, ...]
     * }
     * @endcode
     */
    [[nodiscard]] nlohmann::json computeDfmg(
        std::string_view model_id,
        std::string_view object_type
    ) const;

    /**
     * @brief Find convergence and divergence nodes in a process model.
     *
     * - **Convergence** nodes: in-degree per object type > 1 (many paths merge).
     * - **Divergence**  nodes: out-degree per object type > 1 (one path splits).
     */
    struct ConvergenceDivergenceResult {
        std::vector<std::string> convergence_nodes; ///< Nodes with > 1 incoming object links
        std::vector<std::string> divergence_nodes;  ///< Nodes with > 1 outgoing object links
    };

    /**
     * @brief Analyse a process model for convergence / divergence patterns.
     *
     * @param model_id  Process model identifier.
     * @return @c ConvergenceDivergenceResult with classified nodes.
     */
    [[nodiscard]] ConvergenceDivergenceResult analyze(std::string_view model_id) const;

private:
    ProcessLinker&       linker_;
    ProcessModelManager& model_manager_;
};

} // namespace process
} // namespace themis

