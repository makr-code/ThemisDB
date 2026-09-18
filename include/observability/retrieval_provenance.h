#pragma once

/**
 * @file retrieval_provenance.h
 * @brief Unified provenance record for one end-to-end retrieval decision chain.
 *
 * Captures the full ANN → Tensor → Graph → Final-Layer chain in a single
 * exportable, audit-ready record. Populated by TensorRAGPipeline::step()
 * and emitted as a structured JSON log entry alongside the per-layer
 * layer_handoff_decision events.
 *
 * ## Fields per stage
 * - **trigger_stage**: which gate(s) fired (FLARE / TARG / BOTH / NONE)
 * - **tensor_stage**: number of adapter candidates from tensor summary
 * - **graph_stage**: number of graph-truth evidences assembled
 * - **final_stage**: whether a package/adapter was resolved and which
 * - **chain_complete**: true when retrieval triggered and all wired stages ran
 */

#include "observability/reason_codes.h"
#include "observability/telemetry_keys.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace themis::observability {

/** @brief Compact provenance record for one retrieval decision chain. */
struct RetrievalProvenanceRecord {
    // ── identity ────────────────────────────────────────────────────────────
    std::string correlation_id;
    std::string confidence_policy_version;

    // ── trigger stage ───────────────────────────────────────────────────────
    std::string trigger_reason_code;   ///< e.g. TENSOR_RAG_TRIGGER_FLARE
    bool        flare_fired  = false;
    bool        targ_fired   = false;

    // ── tensor stage ────────────────────────────────────────────────────────
    std::size_t tensor_candidate_count = 0;
    std::string tensor_routing_reason;

    // ── graph stage ─────────────────────────────────────────────────────────
    std::size_t graph_evidence_count   = 0;
    std::string graph_routing_reason_code;

    // ── final-layer stage ───────────────────────────────────────────────────
    bool        final_resolved = false;
    std::string final_package_id;
    std::string final_adapter_id;
    std::string final_routing_reason_code;

    // ── chain summary ───────────────────────────────────────────────────────
    std::string fallback_mode;
    std::string fallback_reason_code;

    /**
     * @brief True when at least one gate triggered and every wired stage
     *        produced a result without a fail-closed fallback.
     */
    bool chain_complete = false;
};

/**
 * @brief Emits one structured provenance log entry for an end-to-end chain.
 *
 * Uses the same THEMIS_INFO sink as emitLayerDecisionLog. Each call emits
 * exactly one JSON line with event type "retrieval_provenance_record".
 *
 * @param r  Populated provenance record.
 */
inline void emitProvenanceLog(const RetrievalProvenanceRecord& r) {
    nlohmann::json j = nlohmann::json::object();
    j["event"]                    = "retrieval_provenance_record";
    j[std::string(telemetry::fields::kCorrelationId)]          = r.correlation_id;
    j[std::string(telemetry::fields::kConfidencePolicyVersion)] = r.confidence_policy_version;
    j["trigger_reason_code"]      = r.trigger_reason_code;
    j["flare_fired"]              = r.flare_fired;
    j["targ_fired"]               = r.targ_fired;
    j["tensor_candidate_count"]   = r.tensor_candidate_count;
    j["tensor_routing_reason"]    = r.tensor_routing_reason;
    j["graph_evidence_count"]     = r.graph_evidence_count;
    j["graph_routing_reason_code"]= r.graph_routing_reason_code;
    j["final_resolved"]           = r.final_resolved;
    j["final_package_id"]         = r.final_package_id;
    j["final_adapter_id"]         = r.final_adapter_id;
    j["final_routing_reason_code"]= r.final_routing_reason_code;
    j[std::string(telemetry::fields::kFallbackMode)]        = r.fallback_mode;
    j[std::string(telemetry::fields::kFallbackReasonCode)]  = r.fallback_reason_code;
    j["chain_complete"]           = r.chain_complete;

    THEMIS_INFO("{}", j.dump());
}

} // namespace themis::observability
