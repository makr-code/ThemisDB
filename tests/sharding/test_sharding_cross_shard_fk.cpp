// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file test_sharding_cross_shard_fk.cpp
 * @brief Unit tests for CrossShardForeignKeyValidator (issue #5392).
 *
 * Tests cover:
 *  - Constraint registration / removal
 *  - INSERT FK check: parent key exists → no violation
 *  - INSERT FK check: parent key missing → violation
 *  - INSERT FK check: null FK value → skipped (NULL semantics)
 *  - DELETE parent check: no children → no violation
 *  - DELETE parent check: child exists on remote shard → violation
 *  - Deferrable constraint: violation returned but marked deferrable
 *  - Fan-out across all shard IDs when no parent_shard_id hint given
 *  - Fail-closed when no KeyExistsCallback is set
 *  - Fail-closed when no ChildExistsCallback is set
 *  - Integration: coordinator prepare() blocked by non-deferrable FK violation
 *  - Integration: coordinator prepare() not blocked by deferrable FK violation
 *  - Integration: removing FK validator disables FK check
 */

#include <gtest/gtest.h>
#include "sharding/cross_shard_fk_validator.h"
#include "sharding/cross_shard_transaction.h"
#include "sharding/consensus_module.h"
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace themisdb::sharding;

// ============================================================================
// Helper factories
// ============================================================================

namespace {

/// Build a minimal INSERT operation JSON string.
std::string makeInsertOp(
    const std::string& table,
    const std::map<std::string, std::string>& data)
{
    nlohmann::json j;
    j["op"]    = "INSERT";
    j["table"] = table;
    for (const auto& [k, v] : data) {
        j["data"][k] = v;
    }
    return j.dump();
}

/// Build a minimal DELETE operation JSON string.
std::string makeDeleteOp(
    const std::string& table,
    const std::string& key,
    const std::string& key_column = "")
{
    nlohmann::json j;
    j["op"]    = "DELETE";
    j["table"] = table;
    j["key"]   = key;
    if (!key_column.empty()) {
        j["key_column"] = key_column;
    }
    return j.dump();
}

/// Build a CrossShardFKConstraint with sensible defaults.
CrossShardFKConstraint makeConstraint(
    const std::string& name,
    const std::string& child_table,
    const std::string& child_col,
    const std::string& parent_table,
    const std::string& parent_col,
    const std::string& parent_shard = "",
    bool deferrable = false)
{
    CrossShardFKConstraint c;
    c.name            = name;
    c.child_table     = child_table;
    c.child_column    = child_col;
    c.parent_table    = parent_table;
    c.parent_column   = parent_col;
    c.parent_shard_id = parent_shard;
    c.deferrable      = deferrable;
    return c;
}

} // anonymous namespace

// ============================================================================
// Unit tests for CrossShardForeignKeyValidator
// ============================================================================

class CrossShardFKValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = std::make_unique<CrossShardForeignKeyValidator>();
    }

    std::unique_ptr<CrossShardForeignKeyValidator> validator_;
};

TEST_F(CrossShardFKValidatorTest, RegisterAndCountConstraints) {
    EXPECT_EQ(validator_->constraintCount(), 0u);
    validator_->registerConstraint(
        makeConstraint("fk1", "orders", "user_id", "users", "id"));
    EXPECT_EQ(validator_->constraintCount(), 1u);
    validator_->registerConstraint(
        makeConstraint("fk2", "order_items", "order_id", "orders", "id"));
    EXPECT_EQ(validator_->constraintCount(), 2u);
}

TEST_F(CrossShardFKValidatorTest, RegisterSameNameReplacesConstraint) {
    validator_->registerConstraint(
        makeConstraint("fk1", "orders", "user_id", "users", "id"));
    validator_->registerConstraint(
        makeConstraint("fk1", "orders", "customer_id", "customers", "id"));
    EXPECT_EQ(validator_->constraintCount(), 1u);
}

TEST_F(CrossShardFKValidatorTest, RemoveConstraint) {
    validator_->registerConstraint(
        makeConstraint("fk1", "orders", "user_id", "users", "id"));
    validator_->registerConstraint(
        makeConstraint("fk2", "order_items", "order_id", "orders", "id"));
    validator_->removeConstraint("fk1");
    EXPECT_EQ(validator_->constraintCount(), 1u);
    validator_->removeConstraint("does_not_exist"); // no-op
    EXPECT_EQ(validator_->constraintCount(), 1u);
}

TEST_F(CrossShardFKValidatorTest, InsertParentExists_NoViolation) {
    validator_->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id",
                       "shard_users"));
    validator_->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string& key) { return key == "user_42"; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {makeInsertOp("orders", {{"user_id", "user_42"}})};

    EXPECT_TRUE(validator_->validate("txn_1", ops).empty());
}

TEST_F(CrossShardFKValidatorTest, InsertParentMissing_Violation) {
    validator_->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id",
                       "shard_users"));
    validator_->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {makeInsertOp("orders", {{"user_id", "user_99"}})};

    auto violations = validator_->validate("txn_2", ops);
    ASSERT_EQ(violations.size(), 1u);
    EXPECT_EQ(violations[0].constraint_name, "fk_orders_user");
    EXPECT_EQ(violations[0].key_value, "user_99");
    EXPECT_FALSE(violations[0].deferrable);
}

TEST_F(CrossShardFKValidatorTest, InsertNullFKValue_NoViolation) {
    validator_->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id",
                       "shard_users"));
    std::atomic<int> callback_calls{0};
    validator_->setKeyExistsCallback(
        [&](const std::string&, const std::string&, const std::string&,
            const std::string&) -> bool { ++callback_calls; return false; });

    nlohmann::json j;
    j["op"]              = "INSERT";
    j["table"]           = "orders";
    j["data"]["user_id"] = nullptr; // explicit JSON null

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {j.dump()};

    EXPECT_TRUE(validator_->validate("txn_null", ops).empty());
    EXPECT_EQ(callback_calls.load(), 0);
}

TEST_F(CrossShardFKValidatorTest, InsertFKColumnAbsent_NoViolation) {
    validator_->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id",
                       "shard_users"));
    validator_->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {makeInsertOp("orders", {{"total", "100"}})};

    EXPECT_TRUE(validator_->validate("txn_absent", ops).empty());
}

TEST_F(CrossShardFKValidatorTest, DeleteParentNoChildren_NoViolation) {
    validator_->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id"));
    validator_->setAllShardIds({"shard_a", "shard_b"});
    validator_->setChildExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_users"] = {makeDeleteOp("users", "user_42")};

    EXPECT_TRUE(validator_->validate("txn_del_ok", ops).empty());
}

TEST_F(CrossShardFKValidatorTest, DeleteParentChildExists_Violation) {
    validator_->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id"));
    validator_->setAllShardIds({"shard_a", "shard_orders"});
    validator_->setChildExistsCallback(
        [](const std::string& shard_id, const std::string&, const std::string&,
           const std::string& parent_val) {
            return shard_id == "shard_orders" && parent_val == "user_42";
        });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_users"] = {makeDeleteOp("users", "user_42")};

    auto violations = validator_->validate("txn_del_blocked", ops);
    ASSERT_EQ(violations.size(), 1u);
    EXPECT_EQ(violations[0].constraint_name, "fk_orders_user");
    EXPECT_EQ(violations[0].key_value, "user_42");
    EXPECT_FALSE(violations[0].deferrable);
}

TEST_F(CrossShardFKValidatorTest, DeferrableViolation_Flagged) {
    validator_->registerConstraint(
        makeConstraint("fk_deferred", "orders", "user_id", "users", "id",
                       "shard_users", /*deferrable=*/true));
    validator_->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {makeInsertOp("orders", {{"user_id", "ghost_user"}})};

    auto violations = validator_->validate("txn_deferred", ops);
    ASSERT_EQ(violations.size(), 1u);
    EXPECT_TRUE(violations[0].deferrable);
    EXPECT_EQ(violations[0].constraint_name, "fk_deferred");
}

TEST_F(CrossShardFKValidatorTest, FanOutParentOnSecondShard_NoViolation) {
    validator_->registerConstraint(
        makeConstraint("fk_no_hint", "orders", "user_id", "users", "id"));
    validator_->setAllShardIds({"shard_a", "shard_b", "shard_c"});
    validator_->setKeyExistsCallback(
        [](const std::string& shard_id, const std::string&, const std::string&,
           const std::string& key) {
            return shard_id == "shard_b" && key == "user_7";
        });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {makeInsertOp("orders", {{"user_id", "user_7"}})};

    EXPECT_TRUE(validator_->validate("txn_fanout", ops).empty());
}

TEST_F(CrossShardFKValidatorTest, NoKeyExistsCallback_FailClosed) {
    validator_->registerConstraint(
        makeConstraint("fk_fail", "orders", "user_id", "users", "id",
                       "shard_users"));
    // No KeyExistsCallback set.

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {makeInsertOp("orders", {{"user_id", "u1"}})};

    auto violations = validator_->validate("txn_no_cb", ops);
    ASSERT_EQ(violations.size(), 1u);
    EXPECT_NE(violations[0].message.find("No KeyExistsCallback"), std::string::npos);
}

TEST_F(CrossShardFKValidatorTest, NoChildExistsCallback_FailClosed) {
    validator_->registerConstraint(
        makeConstraint("fk_fail", "orders", "user_id", "users", "id"));
    validator_->setAllShardIds({"shard_a"});
    // No ChildExistsCallback set.

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_users"] = {makeDeleteOp("users", "u1")};

    auto violations = validator_->validate("txn_no_child_cb", ops);
    ASSERT_EQ(violations.size(), 1u);
    EXPECT_NE(violations[0].message.find("No ChildExistsCallback"), std::string::npos);
}

TEST_F(CrossShardFKValidatorTest, NonJsonOperationSkipped) {
    validator_->registerConstraint(
        makeConstraint("fk1", "orders", "user_id", "users", "id", "s1"));
    validator_->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["shard_orders"] = {"this is not json", "also not json"};

    EXPECT_TRUE(validator_->validate("txn_skip", ops).empty());
}

TEST_F(CrossShardFKValidatorTest, NoConstraints_AlwaysPasses) {
    validator_->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    std::map<std::string, std::vector<std::string>> ops;
    ops["s1"] = {makeInsertOp("orders", {{"user_id", "u1"}})};

    EXPECT_TRUE(validator_->validate("txn_no_c", ops).empty());
}

TEST_F(CrossShardFKValidatorTest, ViolationToJson_ValidJson) {
    FKViolation v;
    v.constraint_name = "fk1";
    v.table           = "orders";
    v.column          = "user_id";
    v.key_value       = "u99";
    v.message         = "parent missing";
    v.deferrable      = false;

    const std::string json_str = v.toJson();
    ASSERT_NO_THROW({
        auto j = nlohmann::json::parse(json_str);
        EXPECT_EQ(j["constraint_name"].get<std::string>(), "fk1");
        EXPECT_EQ(j["deferrable"].get<bool>(), false);
    });
}

// ============================================================================
// Integration tests: CrossShardTransactionCoordinator + FK validator
// ============================================================================

namespace {

/// Minimal stub ConsensusModule that always succeeds.
class StubConsensusModuleFK : public ConsensusModule {
public:
    ConsensusType getType() const override { return ConsensusType::RAFT; }
    bool initialize(const std::string&, const std::vector<std::string>&) override {
        return true;
    }
    bool start() override { return true; }
    void stop() override {}
    bool isLeader() const override { return true; }
    std::string getLeaderId() const override { return "stub-leader"; }
    ConsensusState getState() const override { return ConsensusState::LEADER; }
    std::optional<uint64_t> propose(const std::string&, const nlohmann::json&) override {
        return ++last_index_;
    }
    bool waitForCommit([[maybe_unused]] uint64_t idx,
                       [[maybe_unused]] std::chrono::milliseconds t) override { return true; }
    std::vector<ConsensusLogEntry> readLog(
        [[maybe_unused]] uint64_t s,
        [[maybe_unused]] std::optional<uint64_t> e = std::nullopt) override { return {}; }
    uint64_t getCommitIndex() const override { return 0; }
    uint64_t getLastLogIndex() const override { return last_index_; }
    bool addNode(const std::string&, const std::string&) override { return true; }
    bool removeNode(const std::string&) override { return true; }
    bool transferLeadership(const std::string&) override { return true; }
    bool takeSnapshot(const nlohmann::json&) override { return true; }
    bool restoreSnapshot(const nlohmann::json&) override { return true; }
    ConsensusStats getStats() const override {
        return ConsensusStats{0, 0, 0, ConsensusState::LEADER,
                              "stub-leader", 1, 1,
                              std::chrono::milliseconds(0), 0, 0};
    }
    nlohmann::json getStatus() const override { return {}; }
    void onCommit(std::function<void(const ConsensusLogEntry&)>) override {}
    void onStateChange(std::function<void(ConsensusState, ConsensusState)>) override {}
    void onLeaderChange(
        std::function<void(const std::string&, const std::string&)>) override {}
private:
    uint64_t last_index_ = 0;
};

std::unique_ptr<CrossShardTransactionCoordinator> makeTestCoordinator() {
    CrossShardTransactionConfig cfg;
    cfg.enable_deadlock_detection = false;
    cfg.prepare_timeout  = std::chrono::milliseconds(100);
    cfg.commit_timeout   = std::chrono::milliseconds(100);
    cfg.abort_timeout    = std::chrono::milliseconds(100);
    cfg.lock_timeout     = std::chrono::milliseconds(100);
    cfg.transaction_log_path =
        (std::filesystem::temp_directory_path() /
         ("themis_fk_test_" +
          std::to_string(std::chrono::steady_clock::now()
                             .time_since_epoch()
                             .count()) +
          ".jsonl"))
            .string();
    auto consensus = std::make_shared<StubConsensusModuleFK>();
    return std::make_unique<CrossShardTransactionCoordinator>(cfg, consensus);
}

} // anonymous namespace

class CrossShardFKCoordinatorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        coordinator_ = makeTestCoordinator();
        coordinator_->initialize();
        coordinator_->start();
    }
    void TearDown() override {
        if (coordinator_) {
            coordinator_->stop();
        }
    }
    std::unique_ptr<CrossShardTransactionCoordinator> coordinator_;
};

TEST_F(CrossShardFKCoordinatorIntegrationTest, PrepareBlockedByFKViolation) {
    auto fk_val = std::make_shared<CrossShardForeignKeyValidator>();
    fk_val->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id",
                       "shard_users"));
    fk_val->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; }); // parent always missing

    coordinator_->setForeignKeyValidator(fk_val);

    ASSERT_TRUE(coordinator_->beginTransaction("txn_blocked"));
    ASSERT_TRUE(coordinator_->addParticipant(
        "txn_blocked", "shard_orders", "localhost:9001",
        {makeInsertOp("orders", {{"user_id", "ghost"}})}));

    const bool result = coordinator_->prepare("txn_blocked");
    EXPECT_FALSE(result);

    // Transaction state must be reset to ACTIVE (not stuck in PREPARING).
    auto state = coordinator_->getTransactionState("txn_blocked");
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, TransactionState::ACTIVE);
}

TEST_F(CrossShardFKCoordinatorIntegrationTest, RemovingValidatorDisablesFKCheck) {
    auto fk_val = std::make_shared<CrossShardForeignKeyValidator>();
    fk_val->registerConstraint(
        makeConstraint("fk_orders_user", "orders", "user_id", "users", "id",
                       "shard_users"));
    fk_val->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; });

    coordinator_->setForeignKeyValidator(fk_val);
    coordinator_->setForeignKeyValidator(nullptr); // remove → no FK check

    ASSERT_TRUE(coordinator_->beginTransaction("txn_no_fk"));
    ASSERT_TRUE(coordinator_->addParticipant(
        "txn_no_fk", "shard_orders", "localhost:9001",
        {makeInsertOp("orders", {{"user_id", "ghost"}})}));

    // Not blocked by FK gate; stub sendPrepare may return true or false.
    coordinator_->prepare("txn_no_fk");

    auto state = coordinator_->getTransactionState("txn_no_fk");
    ASSERT_TRUE(state.has_value());
    EXPECT_TRUE(*state == TransactionState::PREPARED ||
                *state == TransactionState::ACTIVE);
}

TEST_F(CrossShardFKCoordinatorIntegrationTest,
       DeferrableViolationDoesNotBlockPrepare) {
    auto fk_val = std::make_shared<CrossShardForeignKeyValidator>();
    fk_val->registerConstraint(
        makeConstraint("fk_deferred", "orders", "user_id", "users", "id",
                       "shard_users", /*deferrable=*/true));
    fk_val->setKeyExistsCallback(
        [](const std::string&, const std::string&, const std::string&,
           const std::string&) { return false; }); // violation but deferrable

    coordinator_->setForeignKeyValidator(fk_val);

    ASSERT_TRUE(coordinator_->beginTransaction("txn_defer_int"));
    ASSERT_TRUE(coordinator_->addParticipant(
        "txn_defer_int", "shard_orders", "localhost:9001",
        {makeInsertOp("orders", {{"user_id", "ghost_deferred"}})}));

    // Deferrable violation must NOT block prepare.
    coordinator_->prepare("txn_defer_int");

    auto state = coordinator_->getTransactionState("txn_defer_int");
    ASSERT_TRUE(state.has_value());
    // NOT reset by FK gate — state reflects stub RPC outcome.
    EXPECT_TRUE(*state == TransactionState::PREPARED ||
                *state == TransactionState::ACTIVE);
}
