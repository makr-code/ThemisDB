/**
 * @file transaction_semantic_advisor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct TransactionContext {
    std::string tx_id;

    std::map<std::string, std::string> entity_map;

    enum class OperationType { READ, WRITE, DELETE };
    OperationType operation_type{OperationType::WRITE};

    std::chrono::milliseconds estimated_duration_ms{0};
};

// ---------------------------------------------------------------------------
// TransactionSemanticAdvisor
// ---------------------------------------------------------------------------

class TransactionSemanticAdvisor {
public:
    // ─── Config ────────────────────────────────────────────────────────────

    struct Config {
        double affinity_threshold{0.8};

        double write_conflict_probability{0.75};

        std::chrono::milliseconds default_deferral_ms{50};
    };

    // ─── BatchAffinityHint ─────────────────────────────────────────────────

    struct BatchAffinityHint {
        std::string primary_tx_id;

        std::vector<std::string> affine_tx_ids;

        double conflict_probability{0.0};

        std::string reason;
    };

    // ─── Lifecycle ─────────────────────────────────────────────────────────

    TransactionSemanticAdvisor();
    /**
     * @brief Transaction Semantic Advisor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit TransactionSemanticAdvisor(Config config);
    ~TransactionSemanticAdvisor() = default;

    TransactionSemanticAdvisor(const TransactionSemanticAdvisor&) = delete;
    TransactionSemanticAdvisor& operator=(const TransactionSemanticAdvisor&) = delete;
    TransactionSemanticAdvisor(TransactionSemanticAdvisor&&) noexcept = default;
    TransactionSemanticAdvisor& operator=(TransactionSemanticAdvisor&&) noexcept = default;


    /**
     * @brief Set Decision Record Processor.
     * @param[in] processor Input parameter.
     */
    void setDecisionRecordProcessor(
        std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor);


    /**
     * @brief Analyze Batch.
     * @param[in] pending_txs Input parameter.
     * @return Return value.
     */
    std::vector<BatchAffinityHint> analyzeBatch(
        const std::vector<TransactionContext>& pending_txs) const;

    /**
     * @brief Suggest Deferral.
     * @param[in] tx Input parameter.
     * @param[in] concurrent_txs Input parameter.
     * @return Return value.
     */
    std::chrono::milliseconds suggestDeferral(
        const TransactionContext& tx,
        const std::vector<TransactionContext>& concurrent_txs) const;

private:
    Config config_;
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor_;

    /**
     * @brief Entity Overlap.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static double entityOverlap(const TransactionContext& a,
                                const TransactionContext& b);

    /**
     * @brief Has Write Conflict.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return True when the operation succeeds.
     */
    static bool hasWriteConflict(const TransactionContext& a,
                                 const TransactionContext& b);

    /**
     * @brief Emit Decision Record.
     * @param[in] hint_count Input parameter.
     * @param[in] tx_count Input parameter.
     */
    void emitDecisionRecord(size_t hint_count, size_t tx_count) const;
};

} // namespace transaction
} // namespace themis
