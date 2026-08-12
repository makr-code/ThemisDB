// Copyright 2026 ThemisDB — Licensed under MIT License

/**
 * @file test_federation_admin.cpp
 * @brief DK-7: Unit tests for FederationAdminHandler, GDPR policy,
 *        audit records, and SphincsPlus signing hooks.
 *
 * Coverage:
 *  - ADM-FED-01: getStats() returns JSON with `current_round`
 *  - ADM-FED-02: triggerRound() returns `delta_version` in JSON
 *  - ADM-FED-03: triggerRound() throws when DP budget exhausted → maps to 403
 *  - AUD-FED-01: Audit callback receives `decision_type="FEDERATED_ROUND"`
 *  - AUD-FED-02: Signing callback is called; signature stored in audit record
 *  - GDPR-FED-01: CrossBorderTransferPolicy blocks → triggerAggregation() throws
 */

#include <gtest/gtest.h>

#include "api/federation_admin_handler.h"
#include "distributed_knowledge/lora_federation_coordinator.h"
#include "governance/cross_border_transfer.h"

#include <memory>
#include <stdexcept>
#include <string>

using namespace themis::distributed_knowledge;
using namespace themis::api;
using namespace themis::governance;

namespace {

/// Shared helper: build a simple gradient for testing.
EncryptedGradient makeGrad(const std::string& shard_id, uint64_t round) {
    EncryptedGradient g;
    g.shard_id    = shard_id;
    g.round       = round;
    g.sample_count = 100;
    g.data        = {{"w", 0.1}, {"b", 0.01}};
    return g;
}

/// Create a coordinator with 2-participant minimum, unlimited rounds.
std::shared_ptr<LoRAFederationCoordinator> makeCoord(size_t max_rounds = 0) {
    FederationConfig cfg;
    cfg.min_participants = 2;
    cfg.dp_epsilon       = 0.1;
    cfg.dp_delta         = 1e-5;
    cfg.max_rounds       = max_rounds;
    return std::make_shared<LoRAFederationCoordinator>(cfg);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Admin endpoint tests
// ─────────────────────────────────────────────────────────────────────────────

// ADM-FED-01: getStats() returns JSON with `current_round` field
TEST(FederationAdminTest, ADM_FED_01_GetStats_ContainsCurrentRound) {
    auto coord   = makeCoord();
    FederationAdminHandler handler(coord);

    const auto stats = handler.getStats();

    ASSERT_TRUE(stats.is_object()) << "getStats() must return a JSON object";
    EXPECT_TRUE(stats.contains("current_round"))
        << "stats JSON must contain 'current_round'";
    EXPECT_GE(stats["current_round"].get<uint64_t>(), 1u);
    EXPECT_TRUE(stats.contains("privacy_budget_remaining"));
    EXPECT_TRUE(stats.contains("budget_verified"));
}

// ADM-FED-02: triggerRound() returns delta_version in JSON
TEST(FederationAdminTest, ADM_FED_02_TriggerRound_ReturnsDeltaVersion) {
    auto coord = makeCoord();
    coord->submitGradient(makeGrad("s0", 1));
    coord->submitGradient(makeGrad("s1", 1));

    FederationAdminHandler handler(coord);

    nlohmann::json result;
    ASSERT_NO_THROW(result = handler.triggerRound());

    EXPECT_TRUE(result.contains("delta_version"))
        << "triggerRound() must return 'delta_version'";
    EXPECT_FALSE(result["delta_version"].get<std::string>().empty())
        << "delta_version must not be empty";
    EXPECT_EQ(result.value("status", ""), "success");
    EXPECT_EQ(result.value("participants", 0u), 2u);
}

// ADM-FED-03: triggerRound() throws when DP budget exhausted → maps to 403
TEST(FederationAdminTest, ADM_FED_03_TriggerRound_ThrowsWhenBudgetExhausted) {
    auto coord = makeCoord(/*max_rounds=*/1);

    // Round 1: exhausts the budget
    coord->submitGradient(makeGrad("s0", 1));
    coord->submitGradient(makeGrad("s1", 1));
    coord->triggerAggregation();

    // Round 2: budget exhausted
    coord->submitGradient(makeGrad("s0", 2));
    coord->submitGradient(makeGrad("s1", 2));

    FederationAdminHandler handler(coord);

    EXPECT_THROW({
        handler.triggerRound();
    }, std::runtime_error) << "triggerRound() must throw when DP budget is exhausted";
}

// ─────────────────────────────────────────────────────────────────────────────
// Audit record tests
// ─────────────────────────────────────────────────────────────────────────────

// AUD-FED-01: Audit callback receives record with decision_type="FEDERATED_ROUND"
TEST(FederationAdminTest, AUD_FED_01_AuditCallback_ReceivesFederatedRoundRecord) {
    auto coord = makeCoord();

    nlohmann::json captured_record;
    bool callback_fired = false;

    coord->setAuditRecordCallback([&](const nlohmann::json& rec) {
        captured_record = rec;
        callback_fired  = true;
    });

    coord->submitGradient(makeGrad("s0", 1));
    coord->submitGradient(makeGrad("s1", 1));
    coord->triggerAggregation();

    ASSERT_TRUE(callback_fired) << "Audit callback must be called after aggregation";
    EXPECT_EQ(captured_record.value("decision_type", ""), "FEDERATED_ROUND")
        << "Audit record must have decision_type='FEDERATED_ROUND'";
    EXPECT_TRUE(captured_record.contains("round"));
    EXPECT_TRUE(captured_record.contains("participants"));
    EXPECT_TRUE(captured_record.contains("epsilon_spent"));
}

// AUD-FED-02: Signing callback is invoked; signature stored in audit record
TEST(FederationAdminTest, AUD_FED_02_SigningCallback_SignatureStoredInAuditRecord) {
    auto coord = makeCoord();

    const std::string mock_signature = "MOCK-SPHINCS-SIGNATURE-0x1234ABCD";
    bool signing_called = false;
    nlohmann::json captured_record;

    coord->setSigningCallback([&](const nlohmann::json& /*rec*/) -> std::string {
        signing_called = true;
        return mock_signature;
    });
    coord->setAuditRecordCallback([&](const nlohmann::json& rec) {
        captured_record = rec;
    });

    coord->submitGradient(makeGrad("s0", 1));
    coord->submitGradient(makeGrad("s1", 1));
    coord->triggerAggregation();

    ASSERT_TRUE(signing_called) << "Signing callback must be invoked after aggregation";
    ASSERT_TRUE(captured_record.contains("sphincs_signature"))
        << "Audit record must contain 'sphincs_signature' when signing callback is set";
    EXPECT_EQ(captured_record["sphincs_signature"].get<std::string>(), mock_signature);
}

// ─────────────────────────────────────────────────────────────────────────────
// GDPR cross-border policy test
// ─────────────────────────────────────────────────────────────────────────────

// GDPR-FED-01: CrossBorderTransferPolicy blocks → triggerAggregation() throws
TEST(FederationAdminTest, GDPR_FED_01_CrossBorderPolicyBlocks_TriggerThrows) {
    auto coord = makeCoord();

    // Create a policy that blocks all non-EU regions
    auto policy = std::make_shared<CrossBorderTransferPolicy>();
    // CN is not in the default adequacy list → PROHIBITED
    // EU shards have no explicit entry but default to "EU" in the resolver

    coord->setCrossBorderPolicy(policy);

    // Register shard-cn in a PROHIBITED region
    coord->setShardLocations({{"s0", "EU"}, {"s1", "CN"}});

    coord->submitGradient(makeGrad("s0", 1));
    coord->submitGradient(makeGrad("s1", 1));

    EXPECT_THROW({
        coord->triggerAggregation();
    }, std::runtime_error)
        << "triggerAggregation() must throw when a participant shard is in a PROHIBITED region";
}
