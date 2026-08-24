/**
 * @file test_transaction_semantic_advisor_llm_focused.cpp
 * @brief Group TS — TransactionSemanticAdvisor LLM-path (analyzeBatch, suggestDeferral) tests.
 */

#include <gtest/gtest.h>
#include "transaction/transaction_semantic_advisor.h"

#include <string>
#include <vector>
#include <chrono>

using namespace themis::transaction;

// ── TS1: Default construct does not throw ────────────────────────────────────
TEST(TransactionSemanticAdvisorLlmFocused, TS1_DefaultConstruct_NoThrow) {
    EXPECT_NO_THROW({ TransactionSemanticAdvisor advisor; });
}

// ── TS2: analyzeBatch on empty input returns empty hints ─────────────────────
TEST(TransactionSemanticAdvisorLlmFocused, TS2_EmptyBatch_ReturnsNoHints) {
    TransactionSemanticAdvisor advisor;
    auto hints = advisor.analyzeBatch({});
    EXPECT_TRUE(hints.empty());
}

// ── TS3: Single transaction → no batch-affinity hints ────────────────────────
TEST(TransactionSemanticAdvisorLlmFocused, TS3_SingleTx_NoAffinityHints) {
    TransactionSemanticAdvisor advisor;

    TransactionContext tx;
    tx.tx_id = "tx-1";
    tx.entity_map = {{"user", "42"}};
    tx.operation_type = TransactionContext::OperationType::WRITE;

    auto hints = advisor.analyzeBatch({tx});
    EXPECT_TRUE(hints.empty());
}

// ── TS4: Two disjoint transactions → no hint (conflict_probability < threshold) ─
TEST(TransactionSemanticAdvisorLlmFocused, TS4_DisjointTxs_NoHints) {
    TransactionSemanticAdvisor advisor;

    TransactionContext tx1;
    tx1.tx_id = "tx-A";
    tx1.entity_map = {{"product", "10"}};
    tx1.operation_type = TransactionContext::OperationType::WRITE;

    TransactionContext tx2;
    tx2.tx_id = "tx-B";
    tx2.entity_map = {{"product", "99"}};
    tx2.operation_type = TransactionContext::OperationType::WRITE;

    auto hints = advisor.analyzeBatch({tx1, tx2});
    // No shared entity → no affinity group expected
    EXPECT_TRUE(hints.empty());
}

// ── TS5: Two transactions sharing entity → hint produced ──────────────────────
TEST(TransactionSemanticAdvisorLlmFocused, TS5_SharedEntityWrites_HintProduced) {
    TransactionSemanticAdvisor advisor;

    TransactionContext tx1;
    tx1.tx_id = "tx-A";
    tx1.entity_map = {{"user", "42"}};
    tx1.operation_type = TransactionContext::OperationType::WRITE;

    TransactionContext tx2;
    tx2.tx_id = "tx-B";
    tx2.entity_map = {{"user", "42"}};  // same entity → conflict
    tx2.operation_type = TransactionContext::OperationType::WRITE;

    auto hints = advisor.analyzeBatch({tx1, tx2});
    ASSERT_FALSE(hints.empty());
    EXPECT_GE(hints[0].conflict_probability, 0.0);
    EXPECT_LE(hints[0].conflict_probability, 1.0);
    EXPECT_FALSE(hints[0].reason.empty());
}

// ── TS6: competing WRITEs on same entity → conflict_probability > 0 ──────────
TEST(TransactionSemanticAdvisorLlmFocused, TS6_CompetingWrites_HighConflictProbability) {
    TransactionSemanticAdvisor::Config cfg;
    cfg.write_conflict_probability = 0.75;
    TransactionSemanticAdvisor advisor(cfg);

    TransactionContext tx1;
    tx1.tx_id = "tx-conflict-A";
    tx1.entity_map = {{"order", "77"}};
    tx1.operation_type = TransactionContext::OperationType::WRITE;

    TransactionContext tx2;
    tx2.tx_id = "tx-conflict-B";
    tx2.entity_map = {{"order", "77"}};
    tx2.operation_type = TransactionContext::OperationType::WRITE;

    auto hints = advisor.analyzeBatch({tx1, tx2});
    ASSERT_FALSE(hints.empty());
    EXPECT_GE(hints[0].conflict_probability, 0.5);
}

// ── TS7: suggestDeferral with no concurrent conflict → 0 ms ──────────────────
TEST(TransactionSemanticAdvisorLlmFocused, TS7_NoConcurrentConflict_ZeroDeferral) {
    TransactionSemanticAdvisor advisor;

    TransactionContext tx;
    tx.tx_id = "tx-main";
    tx.entity_map = {{"item", "5"}};
    tx.operation_type = TransactionContext::OperationType::WRITE;

    TransactionContext concurrent;
    concurrent.tx_id = "tx-other";
    concurrent.entity_map = {{"item", "6"}};  // different entity
    concurrent.operation_type = TransactionContext::OperationType::WRITE;

    auto deferral = advisor.suggestDeferral(tx, {concurrent});
    EXPECT_EQ(deferral.count(), 0);
}

// ── TS8: setDecisionRecordProcessor with null does not crash ──────────────────
TEST(TransactionSemanticAdvisorLlmFocused, TS8_SetNullProcessor_NoThrow) {
    TransactionSemanticAdvisor advisor;
    EXPECT_NO_THROW({ advisor.setDecisionRecordProcessor(nullptr); });
}
