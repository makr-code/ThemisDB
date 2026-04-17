/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            explainability_reason_builder.h                   ║
  Version:         0.1.0                                             ║
  Last Modified:   2026-04-17                                        ║
  Author:          copilot                                           ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ---------------------------------------------------------------------------
// AIDecisionRecord — lightweight input type (no RocksDB dependency)
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight decision record consumed by ExplainabilityReasonBuilder.
 *
 * This struct mirrors the subset of AIDecisionAudit (ai_decision_auditor.h)
 * that is needed to build a causal explanation without pulling in the full
 * RocksDB-backed AIDecisionAuditor.
 */
struct AIDecisionRecord {
    /// Canonical decision type (e.g., "HNSW_PARAMS_UPDATED").
    std::string decision_type;

    /// Unique identifier (optional; used for deduplication in enrichAuditor).
    std::string decision_id;

    /// Arbitrary key/value parameters attached to this decision.
    std::map<std::string, std::string> parameters;

    /// Confidence score in [0.0, 1.0].
    double confidence{0.0};

    /// When this decision was made.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};

    /// True when all guardrails and safety checks passed.
    bool guardrail_passed{true};

    /// Originating shard identifier.
    std::string shard_id;
};

// ---------------------------------------------------------------------------
// ExplainabilityReasonBuilder
// ---------------------------------------------------------------------------

/**
 * @brief Layer-9 LLM Optimization: causal explanation builder.
 *
 * Every autonomous ThemisDB decision must be explainable to the DBA in
 * natural language.  `ExplainabilityReasonBuilder` translates a lightweight
 * `AIDecisionRecord` into a structured `CausalChain` (signal → analysis →
 * decision → confidence → impact → dba_action_required) using a per-type
 * template library.
 *
 * ### Design goals
 * - **Template-based** — no LLM call, deterministic output, zero latency.
 * - **100 % coverage** — every known `decision_type` has a template; unknown
 *   types fall back to a generic template rather than crashing.
 * - **EU AI Act compliant** — each explanation explicitly states whether DBA
 *   action is required and the expected impact.
 *
 * ### Thread safety
 * `ExplainabilityReasonBuilder` is stateless after construction and safe for
 * concurrent use.
 *
 * // STUB/SIMULATION NOTE:
 * // Purpose: Template-based placeholder for future LoRA-adapted NL generation
 * // Activation: Always active (v1.0); LoRA adapter replaces templates post-IMPL-A2
 * // Production Delta: Template text is deterministic; LoRA will produce
 * //                   context-sensitive prose with richer detail
 * // Removal Plan: Replace toNaturalLanguage() internals in IMPL-A2 Loop-1
 */
class ExplainabilityReasonBuilder {
public:
    // ─── CausalChain ──────────────────────────────────────────────────────

    /**
     * @brief Structured causal explanation for a single autonomous decision.
     *
     * The chain follows the SAFE (Signal → Analysis → Fact → Effect) pattern
     * used by ThemisDB's DBA-dialog API.
     */
    struct CausalChain {
        /// What signal triggered the autonomous action.
        std::string signal;

        /// Why the signal is relevant to the system state.
        std::string analysis;

        /// What was decided / changed.
        std::string decision;

        /// How confident the system is in this decision [0.0, 1.0].
        double confidence{0.0};

        /// Expected operational impact of the decision.
        std::string impact;

        /// True when the DBA must take explicit follow-up action.
        bool dba_action_required{false};

        /// Original decision_type tag from the source AIDecisionRecord.
        std::string decision_type;
    };

    // ─── Construction ─────────────────────────────────────────────────────

    ExplainabilityReasonBuilder() = default;
    ~ExplainabilityReasonBuilder() = default;

    ExplainabilityReasonBuilder(const ExplainabilityReasonBuilder&) = default;
    ExplainabilityReasonBuilder& operator=(const ExplainabilityReasonBuilder&) = default;

    // ─── Core API ─────────────────────────────────────────────────────────

    /**
     * @brief Build a causal explanation from an autonomous decision record.
     *
     * Selects the appropriate template from the built-in library, substitutes
     * parameter values, and evaluates whether DBA action is required.
     *
     * @param record  The autonomous decision to explain.
     * @return        A fully-populated CausalChain (never empty).
     */
    CausalChain build(const AIDecisionRecord& record) const;

    /**
     * @brief Render a CausalChain as human-readable natural language.
     *
     * Produces a paragraph of 50–500 words suitable for the DBA REST API
     * or CLI output.
     *
     * @param chain  A CausalChain produced by build().
     * @return       Multi-sentence English explanation.
     */
    std::string toNaturalLanguage(const CausalChain& chain) const;

    /**
     * @brief Batch-enrich a vector of decision records with causal chain text.
     *
     * For each record, calls build() + toNaturalLanguage() and stores the
     * result in `record.parameters["_explanation"]`.
     *
     * @param records  In/out vector of records to enrich.
     * @return         Number of records successfully enriched.
     */
    size_t enrichAuditor(std::vector<AIDecisionRecord>& records) const;

private:
    /// Evaluate whether DBA action is required for the given record.
    static bool requiresDbaAction(const AIDecisionRecord& record);

    /// Retrieve the template entry for a given decision_type.
    struct TemplateEntry {
        std::string signal_tmpl;
        std::string analysis_tmpl;
        std::string decision_tmpl;
        std::string impact_tmpl;
    };
    static const TemplateEntry& getTemplate(const std::string& decision_type);
};

// ---------------------------------------------------------------------------
// FederatedAIDecisionAuditor — cross-shard timeline stub
// ---------------------------------------------------------------------------

/**
 * @brief Stub: cross-shard autonomous-decision timeline merger.
 *
 * Accepts per-shard `AIDecisionRecord` collections and produces a
 * chronologically merged timeline for DBA inspection.
 *
 * // STUB/SIMULATION NOTE:
 * // Purpose: Placeholder until DK-4 (Federated RAG Merge) wires the
 * //          real cross-shard record propagation via FederatedRAGMerger.
 * // Activation: Always active (v1.0); real propagation available post-DK-4.
 * // Production Delta: Real version fetches records from remote shards via
 * //                   GossipProtocol; stub uses in-memory maps only.
 * // Removal Plan: Replace internal shard_records_ with FederatedRAGMerger
 * //               integration in DK-4.
 */
class FederatedAIDecisionAuditor {
public:
    FederatedAIDecisionAuditor() = default;
    ~FederatedAIDecisionAuditor() = default;

    /**
     * @brief Register the decision records of one shard.
     *
     * @param shard_id  Unique shard identifier.
     * @param records   Records from that shard (copied in).
     */
    void addShard(const std::string& shard_id,
                  std::vector<AIDecisionRecord> records);

    /**
     * @brief Return all records from all shards sorted by timestamp.
     *
     * @return Chronologically ordered decision timeline.
     */
    std::vector<AIDecisionRecord> mergeTimeline() const;

    /// Return the number of registered shards.
    size_t shardCount() const noexcept { return shard_records_.size(); }

    /// Return the total number of records across all shards.
    size_t totalRecords() const noexcept;

private:
    std::map<std::string, std::vector<AIDecisionRecord>> shard_records_;
};

} // namespace rag
} // namespace themis
