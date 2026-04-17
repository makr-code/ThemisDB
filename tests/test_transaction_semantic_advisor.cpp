// Copyright 2026 ThemisDB — Licensed under MIT License
// IMPL-B5 / S-6: TransactionSemanticAdvisor unit tests
//
// Tests:
//   TSA-01  analyzeBatch() returns empty hints for fully disjoint entities
//   TSA-02  analyzeBatch() groups transactions with identical entity keys
//   TSA-03  conflict_probability < 0.1 for disjoint entities
//   TSA-04  conflict_probability > 0.7 for same entity + competing WRITEs
//   TSA-05  analyzeBatch() handles a mixed batch (affine + disjoint)
//   TSA-06  suggestDeferral() returns 0 ms when no write conflict exists
//   TSA-07  suggestDeferral() returns non-zero deferral when write conflict detected
//   TSA-08  advisory is non-blocking: analyzeBatch(100 txs) completes within 10 ms

#include <gtest/gtest.h>
#include "transaction/transaction_semantic_advisor.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::transaction;
using Op = TransactionContext::OperationType;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static TransactionContext makeTx(
    const std::string& id,
    std::map<std::string, std::string> entities,
    Op op = Op::WRITE)
{
    TransactionContext tx;
    tx.tx_id          = id;
    tx.entity_map     = std::move(entities);
    tx.operation_type = op;
    return tx;
}

// ---------------------------------------------------------------------------
// TSA-01  Disjoint entities → no hints
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, DisjointEntitiesNoHints)
{
    TransactionSemanticAdvisor advisor;

    std::vector<TransactionContext> txs = {
        makeTx("tx-A", {{"user", "1"}}),
        makeTx("tx-B", {{"inventory", "7"}}),
        makeTx("tx-C", {{"order", "99"}}),
    };

    auto hints = advisor.analyzeBatch(txs);
    EXPECT_TRUE(hints.empty());
}

// ---------------------------------------------------------------------------
// TSA-02  Same entity keys → grouped into a hint
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, SameEntityGrouped)
{
    TransactionSemanticAdvisor advisor;

    std::vector<TransactionContext> txs = {
        makeTx("tx-A", {{"user", "42"}}),
        makeTx("tx-B", {{"user", "42"}}),
    };

    auto hints = advisor.analyzeBatch(txs);
    ASSERT_EQ(hints.size(), 1u);
    EXPECT_EQ(hints[0].primary_tx_id, "tx-A");
    ASSERT_EQ(hints[0].affine_tx_ids.size(), 1u);
    EXPECT_EQ(hints[0].affine_tx_ids[0], "tx-B");
}

// ---------------------------------------------------------------------------
// TSA-03  Disjoint entity maps → conflict_probability < 0.1
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, DisjointConflictProbabilityLow)
{
    TransactionSemanticAdvisor advisor;

    // Two transactions that share no entities
    std::vector<TransactionContext> txs = {
        makeTx("tx-X", {{"user", "1"}}),
        makeTx("tx-Y", {{"inventory", "2"}}),
    };

    auto hints = advisor.analyzeBatch(txs);
    // No affinity → no hints → implicitly conflict_probability == 0 for all pairs
    for (const auto& h : hints) {
        EXPECT_LT(h.conflict_probability, 0.1);
    }
}

// ---------------------------------------------------------------------------
// TSA-04  Same entity + competing WRITEs → conflict_probability > 0.7
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, CompetingWritesHighConflict)
{
    TransactionSemanticAdvisor advisor;

    std::vector<TransactionContext> txs = {
        makeTx("tx-W1", {{"inventory", "7"}}, Op::WRITE),
        makeTx("tx-W2", {{"inventory", "7"}}, Op::WRITE),
    };

    auto hints = advisor.analyzeBatch(txs);
    ASSERT_FALSE(hints.empty());
    EXPECT_GT(hints[0].conflict_probability, 0.7);
    EXPECT_EQ(hints[0].reason, "same_entity_competing_writes");
}

// ---------------------------------------------------------------------------
// TSA-05  Mixed batch: some affine, some disjoint
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, MixedBatch)
{
    TransactionSemanticAdvisor advisor;

    // tx-A and tx-B share "user:42"; tx-C is fully disjoint
    std::vector<TransactionContext> txs = {
        makeTx("tx-A", {{"user", "42"}}, Op::READ),
        makeTx("tx-B", {{"user", "42"}}, Op::READ),
        makeTx("tx-C", {{"product", "99"}}, Op::WRITE),
    };

    auto hints = advisor.analyzeBatch(txs);
    // tx-C should not appear in any hint because it shares no entity with A or B
    bool c_in_hint = false;
    for (const auto& h : hints) {
        if (h.primary_tx_id == "tx-C") c_in_hint = true;
        for (const auto& id : h.affine_tx_ids) {
            if (id == "tx-C") c_in_hint = true;
        }
    }
    EXPECT_FALSE(c_in_hint);

    // tx-A and tx-B should appear together
    bool ab_grouped = false;
    for (const auto& h : hints) {
        if (h.primary_tx_id == "tx-A") {
            for (const auto& id : h.affine_tx_ids) {
                if (id == "tx-B") ab_grouped = true;
            }
        }
    }
    EXPECT_TRUE(ab_grouped);
}

// ---------------------------------------------------------------------------
// TSA-06  suggestDeferral() → 0 ms when no write conflict
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, SuggestDeferralNoConflict)
{
    TransactionSemanticAdvisor advisor;

    TransactionContext tx  = makeTx("tx-me",    {{"user", "1"}}, Op::WRITE);
    TransactionContext other = makeTx("tx-other", {{"user", "2"}}, Op::WRITE); // diff entity id

    auto deferral = advisor.suggestDeferral(tx, {other});
    EXPECT_EQ(deferral.count(), 0);
}

// ---------------------------------------------------------------------------
// TSA-07  suggestDeferral() → non-zero when write conflict detected
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, SuggestDeferralWithConflict)
{
    TransactionSemanticAdvisor advisor;

    TransactionContext tx    = makeTx("tx-me",    {{"inventory", "7"}}, Op::WRITE);
    TransactionContext other = makeTx("tx-other", {{"inventory", "7"}}, Op::WRITE);

    auto deferral = advisor.suggestDeferral(tx, {other});
    EXPECT_GT(deferral.count(), 0);
}

// ---------------------------------------------------------------------------
// TSA-08  Non-blocking: analyzeBatch(100 txs) ≤ 10 ms
// ---------------------------------------------------------------------------
TEST(TransactionSemanticAdvisorTest, AnalyzeBatchNonBlocking)
{
    TransactionSemanticAdvisor advisor;

    // Build 100 transactions with diverse entities to avoid trivial fast-exit
    std::vector<TransactionContext> txs;
    txs.reserve(100);
    for (int i = 0; i < 50; ++i) {
        // Half share entity "user:42"
        txs.push_back(makeTx("tx-" + std::to_string(i),
                             {{"user", "42"}}, Op::WRITE));
    }
    for (int i = 50; i < 100; ++i) {
        txs.push_back(makeTx("tx-" + std::to_string(i),
                             {{"product", std::to_string(i)}}, Op::READ));
    }

    auto t0 = std::chrono::steady_clock::now();
    auto hints = advisor.analyzeBatch(txs);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count();

    (void)hints;
    EXPECT_LE(elapsed, 10) << "analyzeBatch took " << elapsed << " ms (limit: 10 ms)";
}
