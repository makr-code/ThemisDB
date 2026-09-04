/**
 * @file transaction_semantic_advisor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B5 / S-6: TransactionSemanticAdvisor implementation
//

#include "transaction/transaction_semantic_advisor.h"

#include <algorithm>
#include <chrono>
#include <unordered_set>

namespace themis {
namespace transaction {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

TransactionSemanticAdvisor::TransactionSemanticAdvisor()
    : TransactionSemanticAdvisor(Config{}) {}

TransactionSemanticAdvisor::TransactionSemanticAdvisor(Config config)
    : config_(std::move(config)) {}

void TransactionSemanticAdvisor::setDecisionRecordProcessor(
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor)
{
    dr_processor_ = std::move(processor);
}

// ---------------------------------------------------------------------------
// Primary interface
// ---------------------------------------------------------------------------

std::vector<TransactionSemanticAdvisor::BatchAffinityHint>
TransactionSemanticAdvisor::analyzeBatch(
    const std::vector<TransactionContext>& pending_txs) const
{
    std::vector<BatchAffinityHint> hints = {};

    if (pending_txs.empty()) {
        emitDecisionRecord(0, 0);
        return hints;
    }

    // Union-Find style grouping: pair each transaction with the first one it
    // is affine to.  This produces one hint per unique "primary" transaction.
    const size_t n = pending_txs.size();

    // assigned[i] = index of the primary for tx i (-1 = unassigned)
    std::vector<int> assigned(n, -1);

    // Map primary index → position in hints vector
    std::vector<int> hint_index(n, -1);

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            if (entityOverlap(pending_txs[i], pending_txs[j]) >= config_.affinity_threshold
                || entityOverlap(pending_txs[i], pending_txs[j]) > 0.0)
            {
                // i and j are batch-affine
                int primary = (assigned[i] == -1) ? static_cast<int>(i) : assigned[i];

                if (assigned[i] == -1) {
                    // Create a new hint with i as primary
                    BatchAffinityHint hint;
                    hint.primary_tx_id = pending_txs[i].tx_id;
                    hint.reason        = "same_entity_different_action";

                    if (hasWriteConflict(pending_txs[i], pending_txs[j])) {
                        hint.conflict_probability = config_.write_conflict_probability;
                        hint.reason               = "same_entity_competing_writes";
                    }

                    hint_index[i] = static_cast<int>(hints.size());
                    hints.push_back(std::move(hint));
                    assigned[i] = static_cast<int>(i);
                }

                if (assigned[j] == -1) {
                    hints[static_cast<size_t>(hint_index[primary])].affine_tx_ids
                        .push_back(pending_txs[j].tx_id);
                    assigned[j] = primary;

                    // Upgrade conflict probability if this pair competes
                    if (hasWriteConflict(pending_txs[primary], pending_txs[j])) {
                        auto& h = hints[static_cast<size_t>(hint_index[primary])];
                        h.conflict_probability = std::max(h.conflict_probability,
                                                          config_.write_conflict_probability);
                        h.reason = "same_entity_competing_writes";
                    }
                }
            }
        }
    }

    emitDecisionRecord(hints.size(), pending_txs.size());
    return hints;
}

std::chrono::milliseconds TransactionSemanticAdvisor::suggestDeferral(
    const TransactionContext& tx,
    const std::vector<TransactionContext>& concurrent_txs) const
{
    for (const auto& other : concurrent_txs) {
        if (other.tx_id == tx.tx_id) {
            continue;
        }
        if (hasWriteConflict(tx, other)) {
            return config_.default_deferral_ms;
        }
    }
    return std::chrono::milliseconds{0};
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

double TransactionSemanticAdvisor::entityOverlap(
    const TransactionContext& a,
    const TransactionContext& b)
{
    if (a.entity_map.empty() && b.entity_map.empty()) {
        return 0.0;
    }

    size_t shared = 0;
    for (const auto& [type, id] : a.entity_map) {
        auto it = b.entity_map.find(type);
        if (it != b.entity_map.end() && it->second == id) {
            ++shared;
        }
    }

    if (shared == 0) {
        return 0.0;
    }

    // Jaccard: shared / union
    size_t union_size = a.entity_map.size() + b.entity_map.size() - shared;
    return static_cast<double>(shared) / static_cast<double>(union_size);
}

bool TransactionSemanticAdvisor::hasWriteConflict(
    const TransactionContext& a,
    const TransactionContext& b)
{
    using Op = TransactionContext::OperationType;
    if (a.operation_type != Op::WRITE && a.operation_type != Op::DELETE) {
        return false;
    }
    if (b.operation_type != Op::WRITE && b.operation_type != Op::DELETE) {
        return false;
    }

    for (const auto& [type, id] : a.entity_map) {
        auto it = b.entity_map.find(type);
        if (it != b.entity_map.end() && it->second == id) {
            return true;
        }
    }
    return false;
}

void TransactionSemanticAdvisor::emitDecisionRecord(
    size_t hint_count, size_t tx_count) const
{
    if (!dr_processor_) {
        return;
    }

    themis::llm::DecisionRecord rec;
    rec.decision_type = "TX_SEMANTIC_HINT";
    rec.component     = "TransactionSemanticAdvisor";
    rec.outcome       = "SUCCESS";

    rec.parameters["hint_count"] = std::to_string(hint_count);
    rec.parameters["tx_count"]   = std::to_string(tx_count);

    dr_processor_->submit(std::move(rec));
}

} // namespace transaction
} // namespace themis

