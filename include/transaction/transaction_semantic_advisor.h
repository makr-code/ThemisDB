/**
 * @file transaction_semantic_advisor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/decision_record_yaml_processor.h"

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace transaction {

// ---------------------------------------------------------------------------
// TransactionContext
// ---------------------------------------------------------------------------

/// @brief Describes a single pending (or recently submitted) transaction.
struct TransactionContext {
    /// Unique transaction identifier.
    std::string tx_id;

    /// Entity map: entity-type → entity-id.
    /// Example: { "user": "42", "inventory": "7" }
    std::map<std::string, std::string> entity_map;

    enum class OperationType { READ, WRITE, DELETE };
    OperationType operation_type{OperationType::WRITE};

    /// Estimated execution duration (hint, may be 0).
    std::chrono::milliseconds estimated_duration_ms{0};
};

// ---------------------------------------------------------------------------
// TransactionSemanticAdvisor
// ---------------------------------------------------------------------------

/**
 * @brief Layer-5 LLM Optimization: semantic batch-affinity advisor.
 *
 * The `TransactionSemanticAdvisor` complements `DeadlockPredictor` by
 * analysing the *semantic content* of a set of pending transactions to:
 *
 *  - identify pairs / groups that share entity keys (batch affinity),
 *  - estimate the conflict probability for competing writes on the same entity,
 *  - suggest an optimal deferral for a transaction whose entities are currently
 *    contended.
 *
 * Advisory only: the advisor **never blocks** a transaction; it returns its
 * hints synchronously in ≤ 10 ms.
 *
 * ### Similarity model
 * Two transactions are batch-affine when they share at least one (entity-type,
 * entity-id) pair.  Conflict probability rises to > 0.7 when both transactions
 * contain a competing WRITE on the same entity; it stays < 0.1 for disjoint
 * entity maps.
 *
 * ### Decision records
 * When a `DecisionRecordYamlProcessor` is injected the advisor emits one
 * `DecisionRecord{decision_type="TX_SEMANTIC_HINT"}` per `analyzeBatch()` call.
 */
class TransactionSemanticAdvisor {
public:
    // ─── Config ────────────────────────────────────────────────────────────

    struct Config {
        /// Entity-similarity threshold: pairs with shared entity keys ≥ this
        /// fraction are considered batch-affine.  Range [0.0, 1.0].
        double affinity_threshold{0.8};

        /// Conflict probability assigned when two transactions compete with
        /// WRITE operations on the exact same entity.
        double write_conflict_probability{0.75};

        /// Suggested deferral when a competing WRITE is detected.
        std::chrono::milliseconds default_deferral_ms{50};
    };

    // ─── BatchAffinityHint ─────────────────────────────────────────────────

    /// @brief Advisory hint returned for a group of batch-affine transactions.
    struct BatchAffinityHint {
        /// The "anchor" transaction of the affine group.
        std::string primary_tx_id;

        /// Other transactions in the same affine group (recommended to co-schedule).
        std::vector<std::string> affine_tx_ids;

        /// Estimated probability [0.0, 1.0] that scheduling these transactions
        /// together (or in the suggested order) leads to a conflict.
        double conflict_probability{0.0};

        /// Human-readable explanation.
        std::string reason;
    };

    // ─── Lifecycle ─────────────────────────────────────────────────────────

    TransactionSemanticAdvisor();
    explicit TransactionSemanticAdvisor(Config config);
    ~TransactionSemanticAdvisor() = default;

    TransactionSemanticAdvisor(const TransactionSemanticAdvisor&) = delete;
    TransactionSemanticAdvisor& operator=(const TransactionSemanticAdvisor&) = delete;
    TransactionSemanticAdvisor(TransactionSemanticAdvisor&&) = default;
    TransactionSemanticAdvisor& operator=(TransactionSemanticAdvisor&&) = default;

    // ─── Dependency injection ──────────────────────────────────────────────

    /// Inject an optional decision-record writer.  Thread-safe.
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);

    // ─── Primary interface ─────────────────────────────────────────────────

    /**
     * @brief Analyse a set of pending transactions and return batch-affinity hints.
     *
     * Transactions that share entity keys are grouped together.  Within a group
     * the conflict probability reflects competing WRITE operations on the same
     * entity.
     *
     * Complexity: O(N² × E) where N = number of transactions, E = avg entity-map
     * size.  For N ≤ 100 this is well within the 10 ms budget.
     *
     * @param pending_txs  Snapshot of pending transactions to analyse.
     * @return             One hint per affine group (primary + affine partners).
     *                     Returns an empty vector when all transactions are
     *                     mutually disjoint.
     */
    std::vector<BatchAffinityHint> analyzeBatch(
        const std::vector<TransactionContext>& pending_txs) const;

    /**
     * @brief Suggest an optimal deferral for a single transaction.
     *
     * Returns 0 ms when no conflicting transaction is detected among
     * `concurrent_txs`.  Returns `config_.default_deferral_ms` when a
     * competing WRITE on a shared entity is detected.
     *
     * @param tx             The transaction to advise on.
     * @param concurrent_txs Snapshot of concurrently executing transactions.
     */
    std::chrono::milliseconds suggestDeferral(
        const TransactionContext& tx,
        const std::vector<TransactionContext>& concurrent_txs) const;

private:
    Config config_;
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    /// Compute the Jaccard-style entity overlap ratio in [0.0, 1.0].
    static double entityOverlap(const TransactionContext& a,
                                const TransactionContext& b);

    /// True when a and b share at least one entity key with competing WRITEs.
    static bool hasWriteConflict(const TransactionContext& a,
                                 const TransactionContext& b);

    void emitDecisionRecord(size_t hint_count, size_t tx_count) const;
};

} // namespace transaction
} // namespace themis
