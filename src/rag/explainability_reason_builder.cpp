/**
 * @file explainability_reason_builder.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B9 / S-8: ExplainabilityReasonBuilder implementation
//

#include "rag/explainability_reason_builder.h"

#include <algorithm>
#include <sstream>

namespace themis {
namespace rag {

// ---------------------------------------------------------------------------
// Template library — one entry per canonical decision_type
// ---------------------------------------------------------------------------

namespace {

using TE = ExplainabilityReasonBuilder::TemplateEntry;

// Helper: look up a parameter with a fallback value
static std::string param(const AIDecisionRecord& rec,
                         const std::string& key,
                         const std::string& fallback = "N/A")
{
    auto it = rec.parameters.find(key);
    return (it != rec.parameters.end()) ? it->second : fallback;
}

// ---------------------------------------------------------------------------
// Static template table (decision_type → TemplateEntry)
// ---------------------------------------------------------------------------
static const std::map<std::string, TE>& templateTable()
{
    static const std::map<std::string, TE> kTable = {
        // ── Layer 1 ───────────────────────────────────────────────────────
        {"HNSW_PARAMS_UPDATED", {
            "HNSW recall@10 dropped below the configured threshold.",
            "Index distribution drift was detected in the embedding space, "
            "indicating that the current ef_construction and M values are "
            "sub-optimal for the current data distribution.",
            "LoRA adapter (Loop 1) retrained; ef_construction and M updated "
            "to restore target recall.",
            "Estimated recall improvement; index build time slightly increased."
        }},
        {"BAO_PLAN_SELECTED", {
            "BaoOptimizer detected a query plan that outperforms the "
            "default planner selection.",
            "The learned cost model predicted a lower execution cost for an "
            "alternative plan using indexed access rather than a full scan.",
            "Alternative query plan selected and recorded for future "
            "reinforcement training.",
            "Expected average query speedup >= +15% for similar query shapes."
        }},
        // ── Layer 5 ───────────────────────────────────────────────────────
        {"TX_SEMANTIC_HINT", {
            "Transaction conflict rate exceeded the advisory threshold.",
            "Semantic analysis of concurrent transaction patterns revealed "
            "preventable retry cycles caused by overlapping write sets.",
            "Isolation-level hint and re-ordering recommendation issued to "
            "the transaction scheduler.",
            "Estimated retry-cycle reduction >= 15%."
        }},
        // ── Layer 6 ───────────────────────────────────────────────────────
        {"SCHEMA_DEAD_WEIGHT_REPORT", {
            "One or more schema fields have not been accessed within the "
            "rolling 180-day analysis window.",
            "Field access time-series shows no reads beyond the seasonality-"
            "adjusted staleness threshold.",
            "Dead-weight field candidates identified and advisory report "
            "generated for DBA review.",
            "Potential schema simplification and storage reclamation."
        }},
        {"SCHEMA_DEAD_WEIGHT", {
            "Schema dead-weight analysis cycle completed.",
            "Fields with zero reads in the rolling window were identified "
            "after filtering GDPR-protected and seasonally accessed fields.",
            "Advisory dead-weight report produced; no DDL executed.",
            "DBA can archive or drop the listed fields to reduce schema bloat."
        }},
        // ── Layer 7 ───────────────────────────────────────────────────────
        {"INTENT_ALERT", {
            "A query with a potentially malicious semantic intent pattern "
            "was detected.",
            "The IntentClassifier identified features consistent with "
            "SQL injection, data exfiltration, or privilege escalation in "
            "the submitted query.",
            "Risk score escalated on the associated ZeroTrust session; "
            "alert propagated to Layer-11 GossipProtocol.",
            "Query blocked or rate-limited pending DBA review."
        }},
        // ── Layer 8 ───────────────────────────────────────────────────────
        {"WORKLOAD_FINGERPRINT", {
            "Workload pattern matching cycle completed.",
            "The WorkloadFingerprintEngine compared the current query "
            "distribution against the registered fingerprint library.",
            "Best-matching workload archetype identified; adaptive "
            "configuration profile applied.",
            "Resource allocation tuned to the detected workload archetype."
        }},
        // ── Layer 10 ──────────────────────────────────────────────────────
        {"LAYOUT_RECOMMENDATION", {
            "Storage layout analysis cycle completed for one or more "
            "collections.",
            "Access pattern analysis indicates that the current row-oriented "
            "layout is sub-optimal for the observed query mix.",
            "Columnar, hybrid, or tiered layout recommended; DBA approval "
            "required before migration.",
            "Expected compression ratio and query speedup improvements as "
            "stated in the recommendation."
        }},
        // ── Layer 11B ─────────────────────────────────────────────────────
        {"FEDERATED_ROUND", {
            "A federated LoRA training round was initiated across multiple "
            "shards.",
            "The LoRAFederationCoordinator aggregated local gradients from "
            "participating shards using FedAvg.",
            "Global adapter delta computed and distributed to all "
            "participating shards.",
            "Expected cross-shard model quality improvement."
        }},
        {"FEDERATED_DELTA_APPLIED", {
            "A global LoRA adapter delta was applied to the local shard.",
            "The IncrementalLoRATrainer received the aggregated gradient "
            "from the federation coordinator and merged it into the local "
            "adapter weights.",
            "Local adapter weights updated; next inference cycle will use "
            "the improved weights.",
            "Gradual improvement in local query optimisation accuracy."
        }},
        // ── LoRA / Adapter ────────────────────────────────────────────────
        {"LORA_ADAPTER_SELECTION", {
            "Query routing required selection of a LoRA adapter.",
            "The LoRARouter evaluated adapter domain scores against the "
            "incoming query domain to select the best-matching adapter.",
            "Adapter selected and loaded for inference.",
            "Query latency within SLA; domain-specific accuracy maintained."
        }},
        {"LORA_RANK_ADJUSTMENT", {
            "Adapter load imbalance detected across available LoRA adapters.",
            "The AdapterLoadBalancer measured per-adapter utilisation and "
            "identified an imbalance exceeding the rebalance threshold.",
            "Adapter rank scores adjusted; traffic redistributed.",
            "Expected reduction in peak adapter latency."
        }},
        {"LOOP_TRIGGER", {
            "A continuous learning orchestrator loop was triggered.",
            "The ContinuousLearningOrchestrator determined that one of "
            "Loops 1–4 should execute based on the registered trigger "
            "conditions (miss rate, profile drift, or new entry count).",
            "Loop executed; model artefacts updated.",
            "Incremental improvement to the targeted optimisation dimension."
        }},
        {"FEDERATED_FEEDBACK", {
            "Cross-shard feedback sync event recorded.",
            "The CrossShardFeedbackSync propagated RLAIF feedback signals "
            "to or from a peer shard via the GossipProtocol.",
            "Feedback delta integrated into the local RLAIF training buffer.",
            "Improved cross-shard query quality alignment."
        }},
    };
    return kTable;
}

// Fallback template for unknown decision types
static const TE& fallbackTemplate()
{
    static const TE kFallback = {
        "An autonomous system decision was recorded.",
        "The decision was triggered by internal ThemisDB monitoring logic.",
        "Action taken as configured; details available in the parameter map.",
        "System state updated; no DBA action required unless flagged."
    };
    return kFallback;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// ExplainabilityReasonBuilder — implementation
// ---------------------------------------------------------------------------

void ExplainabilityReasonBuilder::setNlGeneratorFn(NlGeneratorFn fn)
{
    nl_generator_fn_ = std::move(fn);
}

const ExplainabilityReasonBuilder::TemplateEntry&
ExplainabilityReasonBuilder::getTemplate(const std::string& decision_type)
{
    const auto& tbl = templateTable();
    auto it = tbl.find(decision_type);
    if (it != tbl.end()) {
        return it->second;
    }
    return fallbackTemplate();
}

bool ExplainabilityReasonBuilder::requiresDbaAction(const AIDecisionRecord& rec)
{
    // INTENT_ALERT with high confidence always requires DBA review
    if (rec.decision_type == "INTENT_ALERT" && rec.confidence > 0.9) {
        return true;
    }

    // SCHEMA_DEAD_WEIGHT_REPORT always requires DBA to review candidates
    if (rec.decision_type == "SCHEMA_DEAD_WEIGHT_REPORT") {
        return true;
    }

    // LAYOUT_RECOMMENDATION requires DBA approval before migration
    if (rec.decision_type == "LAYOUT_RECOMMENDATION") {
        return true;
    }

    // If any guardrail failed the DBA must review
    if (!rec.guardrail_passed) {
        return true;
    }

    return false;
}

ExplainabilityReasonBuilder::CausalChain
ExplainabilityReasonBuilder::build(const AIDecisionRecord& record) const
{
    const TemplateEntry& tmpl = getTemplate(record.decision_type);

    CausalChain chain;
    chain.decision_type       = record.decision_type;
    chain.confidence          = record.confidence;
    chain.dba_action_required = requiresDbaAction(record);

    // Build signal — append shard and key parameters when available
    chain.signal = tmpl.signal_tmpl;
    if (!record.shard_id.empty()) {
        chain.signal += " [shard=" + record.shard_id + "]";
    }

    // Build analysis — substitute confidence
    {
        std::ostringstream ss = {};
        ss << tmpl.analysis_tmpl;
        if (record.confidence > 0.0) {
            ss << " (confidence=" << record.confidence << ")";
        }
        chain.analysis = ss.str();
    }

    // Build decision — include key parameters when present
    chain.decision = tmpl.decision_tmpl;
    if (!record.parameters.empty()) {
        std::ostringstream pss = {};
        pss << " [";
        bool first = true;
        for (const auto& kv : record.parameters) {
            if (kv.first.rfind('_', 0) == 0) continue; // skip internal "_"-keys
            if (!first) pss << ", ";
            pss << kv.first << "=" << kv.second;
            first = false;
        }
        pss << "]";
        chain.decision += pss.str();
    }

    chain.impact = tmpl.impact_tmpl;

    return chain;
}

std::string
ExplainabilityReasonBuilder::toNaturalLanguage(const CausalChain& chain) const
{
    // Delegate to the injected LoRA-adapted generator when available.
    if (nl_generator_fn_) {
        auto result = nl_generator_fn_(chain);
        if (!result.empty()) {
            return result;
        }
    }

    std::ostringstream out = {};

    out << "Decision type: " << chain.decision_type << ".\n\n";

    out << "Signal: " << chain.signal << "\n\n";

    out << "Analysis: " << chain.analysis << "\n\n";

    out << "Decision taken: " << chain.decision << "\n\n";

    out << "Expected impact: " << chain.impact << "\n\n";

    out << "Confidence: " << chain.confidence << ". ";

    if (chain.dba_action_required) {
        out << "DBA action is REQUIRED: please review this decision in the "
               "ThemisDB admin console and confirm or override before the next "
               "scheduled maintenance window.";
    } else {
        out << "DBA action is NOT required: this decision was applied "
               "automatically within the configured guardrail bounds.";
    }

    return out.str();
}

size_t
ExplainabilityReasonBuilder::enrichAuditor(
    std::vector<AIDecisionRecord>& records) const
{
    size_t count = 0;
    for (auto& rec : records) {
        auto chain = build(rec);
        rec.parameters["_explanation"] = toNaturalLanguage(chain);
        ++count;
    }
    return count;
}

// ---------------------------------------------------------------------------
// FederatedAIDecisionAuditor — implementation
// ---------------------------------------------------------------------------

void FederatedAIDecisionAuditor::setShardRecordFetcher(ShardRecordFetcher fn)
{
    shard_fetcher_ = std::move(fn);
}

void FederatedAIDecisionAuditor::addShard(
    const std::string& shard_id,
    std::vector<AIDecisionRecord> records)
{
    shard_records_[shard_id] = std::move(records);
}

std::vector<AIDecisionRecord>
FederatedAIDecisionAuditor::mergeTimeline() const
{
    std::vector<AIDecisionRecord> merged = {};

    for (const auto& [shard_id, records] : shard_records_) {
        for (auto rec : records) {
            rec.shard_id = shard_id; // ensure shard_id is stamped
            merged.push_back(std::move(rec));
        }
        // If a fetcher is wired, supplement local records with remote ones.
        if (shard_fetcher_) {
            auto remote = shard_fetcher_(shard_id);
            for (auto& rec : remote) {
                rec.shard_id = shard_id;
                merged.push_back(std::move(rec));
            }
        }
    }
    // Sort by timestamp (ascending — oldest first)
    std::sort(merged.begin(), merged.end(),
              [](const AIDecisionRecord& a, const AIDecisionRecord& b) {
                  return a.timestamp < b.timestamp;
              });
    return merged;
}

size_t FederatedAIDecisionAuditor::totalRecords() const noexcept
{
    size_t n = 0;
    for (const auto& [id, recs] : shard_records_) {
        n += recs.size();
    }
    return n;
}

} // namespace rag
} // namespace themis
