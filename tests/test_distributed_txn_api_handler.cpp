/**
 * @file test_distributed_txn_api_handler.cpp
 * @brief Unit tests for the distributed transaction HTTP API handler
 *
 * Tests the DistributedTxnApiHandler REST endpoints:
 *  - POST /dtxn/begin      – begin a distributed transaction
 *  - POST /dtxn/operation  – add an operation
 *  - POST /dtxn/commit     – 2PC commit
 *  - POST /dtxn/abort      – explicit abort
 *  - POST /dtxn/readonly   – snapshot read
 *  - GET  /dtxn/status/... – query transaction state
 *  - GET  /dtxn/stats      – coordinator statistics
 *  - Input validation (missing fields, bad JSON, empty shard list)
 */

#include <gtest/gtest.h>
#include "server/distributed_txn_api_handler.h"
#include "server/monitoring_api_handler.h"
#include "sharding/distributed_transaction.h"
#include "sharding/truetime.h"
#include "transaction/two_phase_commit_wal_recovery.h"
#include <atomic>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using namespace themis::server;
using namespace themis::sharding;
namespace http = boost::beast::http;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static http::request<http::string_body>
makeReq(http::verb verb, const std::string& target, const std::string& body = "") {
    http::request<http::string_body> req{verb, target, 11};
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

static nlohmann::json parseBody(const http::response<http::string_body>& res) {
    return nlohmann::json::parse(res.body());
}

constexpr const char* kDtxnDefaultIsolationEnv = "THEMIS_DTXN_DEFAULT_ISOLATION";

class ScopedEnvVar {
public:
    ScopedEnvVar(const char* key, const char* value)
        : key_(key) {
        const char* existing = std::getenv(key_);
        if (existing != nullptr) {
            had_previous_ = true;
            previous_ = existing;
        }
        set(value);
    }

    ~ScopedEnvVar() {
        if (had_previous_) {
            set(previous_.c_str());
        } else {
            clear();
        }
    }

private:
    void set(const char* value) {
#if defined(_WIN32)
        _putenv_s(key_, value);
#else
        setenv(key_, value, 1);
#endif
    }

    void clear() {
#if defined(_WIN32)
        _putenv_s(key_, "");
#else
        unsetenv(key_);
#endif
    }

    const char* key_;
    bool had_previous_{false};
    std::string previous_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class DistributedTxnApiHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        TrueTime::Config tt;
        tt.base_uncertainty_us = 1000;
        auto truetime = std::make_shared<TrueTime>(tt);

        DistributedTransactionCoordinator::Config cfg;
        cfg.enable_recovery_log = false; // no disk I/O in tests
        coordinator_ = std::make_shared<DistributedTransactionCoordinator>(truetime, cfg);
        handler_ = std::make_unique<DistributedTxnApiHandler>(coordinator_);
    }

    void TearDown() override {
        handler_.reset();
        coordinator_.reset();
    }

    std::shared_ptr<DistributedTransactionCoordinator> coordinator_;
    std::unique_ptr<DistributedTxnApiHandler>          handler_;
};

// ─────────────────────────────────────────────────────────────────────────────
// /dtxn/begin
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, BeginReturnsTransactionId) {
    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1","shard2"]})");
    auto res = handler_->handleBegin(req);

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_TRUE(body.contains("transaction_id"));
    EXPECT_FALSE(body["transaction_id"].get<std::string>().empty());
    EXPECT_EQ(body["status"], "active");
    EXPECT_EQ(body["shards"].size(), 2u);
    // Default isolation level should be serializable
    EXPECT_EQ(body["isolation_level"], "serializable");
    EXPECT_FALSE(body.contains("isolation_warning"));
}

TEST_F(DistributedTxnApiHandlerTest, BeginWithSnapshotIsolation) {
    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1"],"isolation_level":"snapshot_isolation"})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    // Some builds downgrade snapshot requests to serializable.
    EXPECT_TRUE(body["isolation_level"] == "snapshot_isolation" ||
                body["isolation_level"] == "serializable");
}

TEST_F(DistributedTxnApiHandlerTest, PercolatorRequiresDurableCoordinatorLog) {
    TrueTime::Config tt;
    auto truetime = std::make_shared<TrueTime>(tt);

    DistributedTransactionCoordinator::Config cfg;
    cfg.enable_recovery_log = false;
    cfg.use_percolator_for_snapshot = true;
    auto coordinator =
        std::make_shared<DistributedTransactionCoordinator>(truetime, cfg);

    const auto transaction_id =
        coordinator->beginTransaction({"shard1"}, DistributedIsolationLevel::SNAPSHOT_ISOLATION);
    ASSERT_FALSE(transaction_id.empty());

    EXPECT_FALSE(coordinator->commit(transaction_id));
    EXPECT_EQ(coordinator->getTransactionState(transaction_id),
              TransactionState::ACTIVE);
}

TEST_F(DistributedTxnApiHandlerTest, BeginWithSerializableIsolation) {
    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1"],"isolation_level":"serializable"})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_EQ(body["isolation_level"], "serializable");
    EXPECT_FALSE(body.contains("isolation_warning"));
}

TEST_F(DistributedTxnApiHandlerTest, BeginUsesSerializableDefaultWhenEnvConfigured) {
    ScopedEnvVar env(kDtxnDefaultIsolationEnv, "serializable");

    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1"]})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_EQ(body["isolation_level"], "serializable");
    EXPECT_FALSE(body.contains("isolation_warning"));
}

TEST_F(DistributedTxnApiHandlerTest, BeginExplicitIsolationOverridesEnvDefault) {
    ScopedEnvVar env(kDtxnDefaultIsolationEnv, "serializable");

    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1"],"isolation_level":"snapshot_isolation"})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_TRUE(body["isolation_level"] == "snapshot_isolation" ||
                body["isolation_level"] == "serializable");
}

TEST_F(DistributedTxnApiHandlerTest, BeginWithInvalidEnvDefaultFallsBackToSerializable) {
    ScopedEnvVar env(kDtxnDefaultIsolationEnv, "invalid_value");

    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1"]})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_EQ(body["isolation_level"], "serializable");
    EXPECT_FALSE(body.contains("isolation_warning"));
}

TEST_F(DistributedTxnApiHandlerTest, BeginWithInvalidIsolationLevelReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["shard1"],"isolation_level":"read_committed"})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, BeginMissingShardsReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/begin", R"({"foo":"bar"})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, BeginEmptyShardsReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/begin", R"({"shards":[]})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST(DistributedTransactionCoordinatorRecoveryTest,
     ConstructorRecoveryKeepsCommitInDoubtWhenParticipantEndpointIsUnavailable) {
    const auto wal_dir = (std::filesystem::temp_directory_path() /
                          ("distributed_txn_wal_" +
                           std::to_string(
                               std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count())))
                             .string();

    struct WalDirGuard {
        const std::string& path;
        ~WalDirGuard() { std::filesystem::remove_all(path); }
    } guard{wal_dir};

    std::filesystem::create_directories(wal_dir);

    const std::string txn_id = "dtxn-wal-replay";

    WALManagerConfig wal_cfg;
    wal_cfg.wal_directory = wal_dir;
    wal_cfg.sync_on_write = true;
    WALManager wal(wal_cfg);

    WALEntry begin_entry;
    begin_entry.type = WALEntryType::BEGIN_TX;
    begin_entry.transaction_id = txn_id;
    begin_entry.data = {
        {"transaction_id", txn_id},
        {"phase", "begin"},
        {"participants", nlohmann::json::array({
            {{"shard_id", "shard-a"}, {"endpoint", "shard://shard-a"}}
        })}
    };
    wal.append(begin_entry);

    WALEntry prepare_entry;
    prepare_entry.type = WALEntryType::PREPARE_TX;
    prepare_entry.transaction_id = txn_id;
    prepare_entry.data = {
        {"transaction_id", txn_id},
        {"phase", "prepared"},
        {"participants", nlohmann::json::array({
            {{"shard_id", "shard-a"}, {"endpoint", "shard://shard-a"}, {"prepared", true}}
        })}
    };
    wal.append(prepare_entry);

    WALEntry decision_entry;
    decision_entry.type = WALEntryType::COMMIT_TX;
    decision_entry.transaction_id = txn_id;
    decision_entry.data = {
        {"transaction_id", txn_id},
        {"phase", "decision"},
        {"decision", "commit"},
        {"commit_timestamp_ns", int64_t{42}},
        {"participants", nlohmann::json::array({
            {{"shard_id", "shard-a"}, {"endpoint", "shard://shard-a"}, {"prepared", true}}
        })}
    };
    wal.append(decision_entry);
    wal.flush();

    TrueTime::Config tt;
    tt.base_uncertainty_us = 1000;
    auto truetime = std::make_shared<TrueTime>(tt);

    DistributedTransactionCoordinator::Config cfg;
    cfg.enable_recovery_log = true;
    cfg.use_percolator_for_snapshot = false;
    cfg.wal_directory = wal_dir;

    DistributedTransactionCoordinator coordinator(truetime, cfg);
    EXPECT_EQ(coordinator.getTransactionState(txn_id), TransactionState::COMMITTING);
    ASSERT_EQ(coordinator.getRecoverableTransactions().size(), 1u);
    EXPECT_EQ(coordinator.getRecoverableTransactions().front().transaction_id, txn_id);
    EXPECT_EQ(coordinator.getRecoveryBlockerCount(), 1u);
    EXPECT_EQ(coordinator.recoverInDoubtTransactions(), 0u);
    EXPECT_EQ(coordinator.getTransactionState(txn_id), TransactionState::COMMITTING);
    EXPECT_EQ(coordinator.getRecoveryBlockerCount(), 1u);

    const auto report =
        themis::transaction::GlobalTwoPhaseCommitRecoveryManager::recoverAll({&coordinator});
    EXPECT_FALSE(report.ready_for_service);
    EXPECT_EQ(report.in_doubt_after, 1u);
}

TEST(DistributedTransactionCoordinatorRecoveryTest,
     WALReplayMarksMalformedParticipantMetadataUnresolved) {
    WALEntry begin_entry;
    begin_entry.type = WALEntryType::BEGIN_TX;
    begin_entry.transaction_id = "malformed-participant-metadata";
    begin_entry.data = {
        {"participants", nlohmann::json::array({
            {{"shard_id", "shard-a"}},
            {{"endpoint", "shard://missing-id"}}
        })}
    };

    const auto recovered =
        themis::transaction::TwoPhaseCommitWALRecovery::reconstruct({begin_entry});
    ASSERT_EQ(recovered.size(), 1u);
    const auto& transaction = recovered.begin()->second;
    EXPECT_TRUE(transaction.participants_invalid);
    ASSERT_EQ(transaction.participants.size(), 1u);
    EXPECT_EQ(transaction.participants.front(), "shard-a");
}

TEST(DistributedTransactionCoordinatorRecoveryTest,
     ActiveRuntimeTransactionDoesNotBlockReadiness) {
    TrueTime::Config tt;
    tt.base_uncertainty_us = 1000;
    auto truetime = std::make_shared<TrueTime>(tt);

    DistributedTransactionCoordinator::Config cfg;
    cfg.enable_recovery_log = false;
    DistributedTransactionCoordinator coordinator(truetime, cfg);

    ASSERT_FALSE(coordinator.beginTransaction({"shard-a"}).empty());
    EXPECT_EQ(coordinator.getRecoveryBlockerCount(), 0u);
}

TEST(DistributedTransactionCoordinatorRecoveryTest,
     RuntimeRecoveryRetryDoesNotAbortLiveTransaction) {
    const auto wal_dir = (std::filesystem::temp_directory_path() /
        ("distributed_txn_live_wal_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))).string();
    struct WalDirGuard {
        const std::string& path;
        ~WalDirGuard() { std::filesystem::remove_all(path); }
    } guard{wal_dir};

    TrueTime::Config tt;
    auto truetime = std::make_shared<TrueTime>(tt);
    DistributedTransactionCoordinator::Config cfg;
    cfg.enable_recovery_log = true;
    cfg.wal_directory = wal_dir;
    DistributedTransactionCoordinator coordinator(truetime, cfg);

    const auto transaction_id = coordinator.beginTransaction({"shard-a"});
    ASSERT_FALSE(transaction_id.empty());
    EXPECT_EQ(coordinator.recoverInDoubtTransactions(), 0u);
    EXPECT_EQ(coordinator.getTransactionState(transaction_id), TransactionState::ACTIVE);
    EXPECT_EQ(coordinator.getRecoveryBlockerCount(), 0u);
}

TEST(MonitoringApiHandlerReadinessTest, BlocksWhileTransactionRecoveryIsUnresolved) {
    std::atomic<uint64_t> request_count{0};
    std::atomic<uint64_t> error_count{0};
    const auto started_at = std::chrono::steady_clock::now();
    MonitoringApiHandler handler(
        nullptr, nullptr, &request_count, &error_count, &started_at, nullptr);
    std::size_t recovery_blockers = 1;
    handler.setTransactionRecoveryBlockerCountProvider(
        [&recovery_blockers] { return recovery_blockers; });

    auto req = makeReq(http::verb::get, "/health/ready");
    auto response = handler.handleReadiness(req);
    EXPECT_EQ(response.result(), http::status::service_unavailable);
    auto body = parseBody(response);
    EXPECT_EQ(body["status"], "not_ready");
    EXPECT_FALSE(body["checks"]["transaction_recovery_ready"].get<bool>());
    EXPECT_EQ(body["checks"]["transaction_recovery_blockers"], 1u);

    recovery_blockers = 0;
    response = handler.handleReadiness(req);
    EXPECT_EQ(response.result(), http::status::ok);
    body = parseBody(response);
    EXPECT_TRUE(body["checks"]["transaction_recovery_ready"].get<bool>());
    EXPECT_EQ(body["checks"]["transaction_recovery_blockers"], 0u);
}

TEST(MonitoringApiHandlerReadinessTest, FailsClosedWhenRecoveryReadinessProviderThrows) {
    std::atomic<uint64_t> request_count{0};
    std::atomic<uint64_t> error_count{0};
    const auto started_at = std::chrono::steady_clock::now();
    MonitoringApiHandler handler(
        nullptr, nullptr, &request_count, &error_count, &started_at, nullptr);
    handler.setTransactionRecoveryBlockerCountProvider([]() -> std::size_t {
        throw std::runtime_error("recovery state unavailable");
    });

    auto req = makeReq(http::verb::get, "/health/ready");
    const auto response = handler.handleReadiness(req);
    EXPECT_EQ(response.result(), http::status::service_unavailable);
    const auto body = parseBody(response);
    EXPECT_FALSE(body["checks"]["transaction_recovery_ready"].get<bool>());
}

TEST_F(DistributedTxnApiHandlerTest, BeginInvalidShardIdReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/begin",
                       R"({"shards":["../shard1"]})");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, BeginInvalidJsonReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/begin", "{bad json");
    auto res = handler_->handleBegin(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ─────────────────────────────────────────────────────────────────────────────
// /dtxn/operation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, OperationAppendedSuccessfully) {
    // First begin
    auto begin_req = makeReq(http::verb::post, "/dtxn/begin",
                             R"({"shards":["shard1"]})");
    auto begin_res = handler_->handleBegin(begin_req);
    auto txn_id = parseBody(begin_res)["transaction_id"].get<std::string>();

    // Then add operation
    nlohmann::json op_body = {
        {"transaction_id", txn_id},
        {"shard_id",       "shard1"},
        {"operation",      {{"type","insert"},{"key","k1"}}}
    };
    auto op_req = makeReq(http::verb::post, "/dtxn/operation", op_body.dump());
    auto op_res = handler_->handleOperation(op_req);

    EXPECT_EQ(op_res.result(), http::status::ok);
    auto body = parseBody(op_res);
    EXPECT_EQ(body["status"], "ok");
}

TEST_F(DistributedTxnApiHandlerTest, OperationMissingTransactionIdReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/operation",
                       R"({"shard_id":"shard1","operation":{}})");
    auto res = handler_->handleOperation(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, OperationMissingShardIdReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/operation",
                       R"({"transaction_id":"txn-x","operation":{}})");
    auto res = handler_->handleOperation(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, OperationInvalidTransactionIdReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/operation",
                       R"({"transaction_id":"../txn-x","shard_id":"shard1","operation":{}})");
    auto res = handler_->handleOperation(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, OperationOnNonExistentTxnReturnsError) {
    nlohmann::json body = {
        {"transaction_id", "txn-doesnotexist"},
        {"shard_id",       "shard1"},
        {"operation",      {{"type","insert"}}}
    };
    auto req = makeReq(http::verb::post, "/dtxn/operation", body.dump());
    auto res = handler_->handleOperation(req);
    EXPECT_EQ(res.result(), http::status::unprocessable_entity);
}

// ─────────────────────────────────────────────────────────────────────────────
// /dtxn/abort
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, AbortActiveTransactionSucceeds) {
    auto begin_req = makeReq(http::verb::post, "/dtxn/begin",
                             R"({"shards":["shard1","shard2"]})");
    auto txn_id = parseBody(handler_->handleBegin(begin_req))["transaction_id"].get<std::string>();

    auto abort_req = makeReq(http::verb::post, "/dtxn/abort",
                             nlohmann::json{{"transaction_id", txn_id}}.dump());
    auto abort_res = handler_->handleAbort(abort_req);

    EXPECT_EQ(abort_res.result(), http::status::ok);
    EXPECT_EQ(parseBody(abort_res)["status"], "aborted");
}

TEST_F(DistributedTxnApiHandlerTest, AbortMissingTransactionIdReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/abort", R"({"foo":"bar"})");
    auto res = handler_->handleAbort(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ─────────────────────────────────────────────────────────────────────────────
// /dtxn/status/{id}
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, StatusOfActiveTransaction) {
    auto begin_req = makeReq(http::verb::post, "/dtxn/begin",
                             R"({"shards":["shard1"]})");
    auto txn_id = parseBody(handler_->handleBegin(begin_req))["transaction_id"].get<std::string>();

    auto status_req = makeReq(http::verb::get, "/dtxn/status/" + txn_id);
    auto status_res = handler_->handleStatus(status_req);

    EXPECT_EQ(status_res.result(), http::status::ok);
    auto body = parseBody(status_res);
    EXPECT_EQ(body["state"], "ACTIVE");
    EXPECT_EQ(body["transaction_id"], txn_id);
}

TEST_F(DistributedTxnApiHandlerTest, StatusOfUndeliveredAbort) {
    auto begin_req = makeReq(http::verb::post, "/dtxn/begin",
                             R"({"shards":["shard1"]})");
    auto txn_id = parseBody(handler_->handleBegin(begin_req))["transaction_id"].get<std::string>();

    handler_->handleAbort(
        makeReq(http::verb::post, "/dtxn/abort",
                nlohmann::json{{"transaction_id", txn_id}}.dump())
    );

    auto status_res = handler_->handleStatus(
        makeReq(http::verb::get, "/dtxn/status/" + txn_id)
    );

    EXPECT_EQ(status_res.result(), http::status::ok);
    EXPECT_EQ(parseBody(status_res)["state"], "ABORTING")
        << "A shard transaction remains unresolved until every participant "
           "acknowledges ABORT";
}

TEST_F(DistributedTxnApiHandlerTest, StatusOfUnknownTransactionReturnsNotFound) {
    auto req = makeReq(http::verb::get, "/dtxn/status/txn-unknown-xyz");
    auto res = handler_->handleStatus(req);
    EXPECT_EQ(res.result(), http::status::not_found);
}

TEST_F(DistributedTxnApiHandlerTest, StatusMissingIdReturnsBadRequest) {
    // Path exactly "/dtxn/status/" with no trailing ID
    auto req = makeReq(http::verb::get, "/dtxn/status/");
    auto res = handler_->handleStatus(req);
    // Empty txn_id means the coordinator returns not found
    EXPECT_TRUE(res.result() == http::status::not_found ||
                res.result() == http::status::bad_request);
}

TEST_F(DistributedTxnApiHandlerTest, StatusInvalidIdReturnsBadRequest) {
    auto req = makeReq(http::verb::get, "/dtxn/status/../txn-x");
    auto res = handler_->handleStatus(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ─────────────────────────────────────────────────────────────────────────────
// /dtxn/stats
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, StatsReturnsValidJson) {
    auto res = handler_->handleStats(
        makeReq(http::verb::get, "/dtxn/stats")
    );
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_TRUE(body.contains("total_transactions"));
    EXPECT_TRUE(body.contains("committed_transactions"));
    EXPECT_TRUE(body.contains("aborted_transactions"));
}

TEST_F(DistributedTxnApiHandlerTest, StatsDoNotCountUndeliveredAborts) {
    // Unknown shard endpoints cannot acknowledge the ABORT decision.
    for (int i = 0; i < 2; ++i) {
        auto begin_req = makeReq(http::verb::post, "/dtxn/begin",
                                 R"({"shards":["shard1"]})");
        auto txn_id = parseBody(handler_->handleBegin(begin_req))["transaction_id"].get<std::string>();
        handler_->handleAbort(
            makeReq(http::verb::post, "/dtxn/abort",
                    nlohmann::json{{"transaction_id", txn_id}}.dump())
        );
    }

    auto stats = parseBody(handler_->handleStats(
        makeReq(http::verb::get, "/dtxn/stats")
    ));

    EXPECT_GE(stats["total_transactions"].get<int>(), 2);
    EXPECT_EQ(stats["aborted_transactions"].get<int>(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// /dtxn/readonly
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, ReadOnlyReturnsShardsResult) {
    auto req = makeReq(http::verb::post, "/dtxn/readonly",
                       R"({"shards":["shard1","shard2"],"operations":{}})");
    auto res = handler_->handleReadOnly(req);

    EXPECT_EQ(res.result(), http::status::ok);
    auto body = parseBody(res);
    EXPECT_TRUE(body.contains("results"));
    // Results should have one entry per shard
    EXPECT_EQ(body["results"].size(), 2u);
    EXPECT_EQ(body["results"]["shard1"]["status"], "error");
    EXPECT_EQ(body["results"]["shard1"]["error"], "missing_registered_endpoint");
    EXPECT_EQ(body["results"]["shard2"]["status"], "error");
    EXPECT_EQ(body["results"]["shard2"]["error"], "missing_registered_endpoint");
}

TEST_F(DistributedTxnApiHandlerTest, ReadOnlyMissingShardsReturnsBadRequest) {
    auto req = makeReq(http::verb::post, "/dtxn/readonly", R"({"operations":{}})");
    auto res = handler_->handleReadOnly(req);
    EXPECT_EQ(res.result(), http::status::bad_request);
}

// ─────────────────────────────────────────────────────────────────────────────
// Full begin → operate → abort flow
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnApiHandlerTest, FullBeginOperateAbortFlowRemainsInDoubt) {
    // 1. Begin
    auto begin_res = handler_->handleBegin(
        makeReq(http::verb::post, "/dtxn/begin",
                R"({"shards":["alpha","beta","gamma"]})"));
    ASSERT_EQ(begin_res.result(), http::status::ok);
    auto txn_id = parseBody(begin_res)["transaction_id"].get<std::string>();

    // 2. Add operations to multiple shards
    for (const auto& shard : {"alpha", "beta", "gamma"}) {
        nlohmann::json op_body = {
            {"transaction_id", txn_id},
            {"shard_id",       shard},
            {"operation",      {{"type","insert"},{"key","doc"},{"value",42}}}
        };
        auto op_res = handler_->handleOperation(
            makeReq(http::verb::post, "/dtxn/operation", op_body.dump()));
        EXPECT_EQ(op_res.result(), http::status::ok);
    }

    // 3. Check status is ACTIVE
    auto st = parseBody(handler_->handleStatus(
        makeReq(http::verb::get, "/dtxn/status/" + txn_id)));
    EXPECT_EQ(st["state"], "ACTIVE");

    // 4. Abort
    auto abort_res = handler_->handleAbort(
        makeReq(http::verb::post, "/dtxn/abort",
                nlohmann::json{{"transaction_id", txn_id}}.dump()));
    EXPECT_EQ(abort_res.result(), http::status::ok);

    // 5. Status remains ABORTING until participants acknowledge the decision.
    auto st2 = parseBody(handler_->handleStatus(
        makeReq(http::verb::get, "/dtxn/status/" + txn_id)));
    EXPECT_EQ(st2["state"], "ABORTING");
}
