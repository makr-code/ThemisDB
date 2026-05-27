/*
 * ThemisDB | File: test_rpc_batch_operations.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include "server/rpc_service_impl.h"
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <string>
#include <limits>

using namespace themis;
using namespace themis::server::rpc;
using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

// ============================================================================
// Fixture
// ============================================================================

class RPCBatchOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "test_rpc_batch_ops_db";
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test RocksDB instance";
        service_ = std::make_unique<ThemisRPCService>(db_.get());
    }

    void TearDown() override {
        service_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    // Build a key-spec object used by batch_get and batch_delete
    static json KeySpec(const std::string& collection,
                        const std::string& model,
                        const std::string& uuid) {
        return {{"collection", collection}, {"model", model}, {"uuid", uuid}};
    }

    // Build an entity-spec object used by batch_put
    static json EntitySpec(const std::string& collection,
                            const std::string& model,
                            const std::string& uuid,
                            const json& payload = {}) {
        json e = payload;
        e["id"] = uuid;
        return {{"collection", collection},
                {"model",      model},
                {"uuid",       uuid},
                {"entity",     e}};
    }

    // Insert a single entity directly (bypass RPC) for test setup
    void DirectPut(const std::string& collection,
                   const std::string& model,
                   const std::string& uuid,
                   const json& payload = {}) {
        json e = payload;
        e["_collection"] = collection;
        e["_model"] = model;
        e["uuid"] = uuid;
        e["_version"] = 1;
        std::string key = collection + ":" + model + ":" + uuid;
        ASSERT_TRUE(db_->put(key, e.dump()));
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<ThemisRPCService> service_;
};

// ============================================================================
// handleBatchGet – read path
// ============================================================================

TEST_F(RPCBatchOperationsTest, BatchGetMissingKeysParam) {
    json params = {{"wrong_key", json::array()}};
    auto resp = service_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS));
}

TEST_F(RPCBatchOperationsTest, BatchGetEmptyKeys) {
    json params = {{"keys", json::array()}};
    auto resp = service_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), 0);
    EXPECT_TRUE(resp["result"]["results"].is_array());
    EXPECT_TRUE(resp["result"]["results"].empty());
}

TEST_F(RPCBatchOperationsTest, BatchGetMalformedKeyEntry) {
    json params = {{"keys", json::array({json{{"collection", "c"}}})}};  // missing model/uuid
    auto resp = service_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS));
}

TEST_F(RPCBatchOperationsTest, BatchGetAllMissing) {
    json params = {{"keys", json::array({
        KeySpec("col", "M", "u1"),
        KeySpec("col", "M", "u2"),
    })}};
    auto resp = service_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("result"));
    auto& results = resp["result"]["results"];
    ASSERT_EQ(results.size(), 2u);
    for (const auto& r : results) {
        EXPECT_FALSE(r.value("found", true));
    }
}

TEST_F(RPCBatchOperationsTest, BatchGetPartialHit) {
    DirectPut("col", "M", "u1", {{"v", 10}});

    json params = {{"keys", json::array({
        KeySpec("col", "M", "u1"),
        KeySpec("col", "M", "u2"),  // does not exist
    })}};
    auto resp = service_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("result"));
    auto& results = resp["result"]["results"];
    ASSERT_EQ(results.size(), 2u);
    EXPECT_TRUE(results[0].value("found", false));
    EXPECT_EQ(results[0]["entity"]["v"].get<int>(), 10);
    EXPECT_FALSE(results[1].value("found", true));
}

TEST_F(RPCBatchOperationsTest, BatchGetAllFound) {
    const int n = 10;
    for (int i = 0; i < n; ++i) {
        DirectPut("data", "Item", "item-" + std::to_string(i), {{"idx", i}});
    }

    json keys = json::array();
    for (int i = 0; i < n; ++i) {
        keys.push_back(KeySpec("data", "Item", "item-" + std::to_string(i)));
    }
    json params = {{"keys", keys}};

    auto resp = service_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), n);
    auto& results = resp["result"]["results"];
    ASSERT_EQ(static_cast<int>(results.size()), n);
    for (int i = 0; i < n; ++i) {
        EXPECT_TRUE(results[i].value("found", false))
            << "item-" << i << " should be found";
    }
}

// ============================================================================
// handleBatchPut – write path
// ============================================================================

TEST_F(RPCBatchOperationsTest, BatchPutMissingEntitiesParam) {
    json params = {{"wrong", json::array()}};
    auto resp = service_->handleBatchPut(params);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS));
}

TEST_F(RPCBatchOperationsTest, BatchPutEmptyArray) {
    json params = {{"entities", json::array()}};
    auto resp = service_->handleBatchPut(params);
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), 0);
}

TEST_F(RPCBatchOperationsTest, BatchPutMalformedEntry) {
    json params = {{"entities", json::array({json{{"collection", "c"}}})}};
    auto resp = service_->handleBatchPut(params);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS));
}

TEST_F(RPCBatchOperationsTest, BatchPutAndReadBack) {
    const int n = 5;
    json entities = json::array();
    for (int i = 0; i < n; ++i) {
        entities.push_back(EntitySpec("tbl", "Row", "row-" + std::to_string(i), {{"val", i * 3}}));
    }
    json put_params = {{"entities", entities}};
    auto put_resp = service_->handleBatchPut(put_params);
    ASSERT_TRUE(put_resp.contains("result"));
    EXPECT_EQ(put_resp["result"]["count"].get<int>(), n);

    // Read back with batch get
    json keys = json::array();
    for (int i = 0; i < n; ++i) {
        keys.push_back(KeySpec("tbl", "Row", "row-" + std::to_string(i)));
    }
    auto get_resp = service_->handleBatchGet({{"keys", keys}});
    ASSERT_TRUE(get_resp.contains("result"));
    auto& results = get_resp["result"]["results"];
    ASSERT_EQ(static_cast<int>(results.size()), n);
    for (int i = 0; i < n; ++i) {
        EXPECT_TRUE(results[i].value("found", false)) << "row-" << i;
        EXPECT_EQ(results[i]["entity"]["val"].get<int>(), i * 3) << "row-" << i;
    }
}

TEST_F(RPCBatchOperationsTest, BatchPutIsAtomicOnCommit) {
    // Insert 20 items; all should be written or none (atomicity via WriteBatch)
    const int n = 20;
    json entities = json::array();
    for (int i = 0; i < n; ++i) {
        entities.push_back(EntitySpec("atomic", "Obj", "obj-" + std::to_string(i)));
    }
    auto resp = service_->handleBatchPut({{"entities", entities}});
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), n);

    // All items must exist after commit
    for (int i = 0; i < n; ++i) {
        std::string key = "atomic:Obj:obj-" + std::to_string(i);
        std::string val;
        EXPECT_TRUE(db_->get(key, val)) << "Key " << key << " should exist";
    }
}

// ============================================================================
// handleBatchDelete – delete path
// ============================================================================

TEST_F(RPCBatchOperationsTest, BatchDeleteMissingKeysParam) {
    json params = {{"wrong", json::array()}};
    auto resp = service_->handleBatchDelete(params);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS));
}

TEST_F(RPCBatchOperationsTest, BatchDeleteEmptyKeys) {
    json params = {{"keys", json::array()}};
    auto resp = service_->handleBatchDelete(params);
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), 0);
}

TEST_F(RPCBatchOperationsTest, BatchDeleteMalformedKeyEntry) {
    json params = {{"keys", json::array({json{{"collection", "c"}}})}};
    auto resp = service_->handleBatchDelete(params);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::INVALID_PARAMETERS));
}

TEST_F(RPCBatchOperationsTest, BatchDeleteExistingItems) {
    const int n = 5;
    for (int i = 0; i < n; ++i) {
        DirectPut("del_test", "Doc", "doc-" + std::to_string(i));
    }

    json keys = json::array();
    for (int i = 0; i < n; ++i) {
        keys.push_back(KeySpec("del_test", "Doc", "doc-" + std::to_string(i)));
    }
    auto del_resp = service_->handleBatchDelete({{"keys", keys}});
    ASSERT_TRUE(del_resp.contains("result"));
    EXPECT_EQ(del_resp["result"]["count"].get<int>(), n);

    // Items must no longer exist
    auto get_resp = service_->handleBatchGet({{"keys", keys}});
    ASSERT_TRUE(get_resp.contains("result"));
    for (const auto& r : get_resp["result"]["results"]) {
        EXPECT_FALSE(r.value("found", true)) << "Deleted item should not be found";
    }
}

TEST_F(RPCBatchOperationsTest, BatchDeleteNonExistentItemsIsIdempotent) {
    // Deleting keys that do not exist must succeed (idempotent)
    json keys = json::array({
        KeySpec("ghost", "G", "missing-1"),
        KeySpec("ghost", "G", "missing-2"),
    });
    auto resp = service_->handleBatchDelete({{"keys", keys}});
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_TRUE(resp["result"]["success"].get<bool>());
    EXPECT_EQ(resp["result"]["count"].get<int>(), 2);
}

TEST_F(RPCBatchOperationsTest, BatchDeletePartialSetLeavesRemainder) {
    for (int i = 0; i < 4; ++i) {
        DirectPut("mixed", "E", "e-" + std::to_string(i));
    }

    // Delete only the even-indexed items
    json del_keys = json::array({KeySpec("mixed", "E", "e-0"), KeySpec("mixed", "E", "e-2")});
    auto del_resp = service_->handleBatchDelete({{"keys", del_keys}});
    ASSERT_TRUE(del_resp.contains("result"));
    EXPECT_EQ(del_resp["result"]["count"].get<int>(), 2);

    // Odd-indexed items must still exist
    for (int i : {1, 3}) {
        std::string key = "mixed:E:e-" + std::to_string(i);
        std::string val;
        EXPECT_TRUE(db_->get(key, val)) << key << " should still exist";
    }

    // Even-indexed items must be gone
    for (int i : {0, 2}) {
        std::string key = "mixed:E:e-" + std::to_string(i);
        std::string val;
        EXPECT_FALSE(db_->get(key, val)) << key << " should have been deleted";
    }
}

// ============================================================================
// handleBatchUpdate – partial-update path
// ============================================================================

TEST_F(RPCBatchOperationsTest, BatchUpdateNonExistentItemsSkipped) {
    json updates = json::array({
        {{"collection", "u"}, {"model", "M"}, {"uuid", "no-such"}, {"updates", {{"x", 99}}}},
    });
    auto resp = service_->handleBatchUpdate({{"updates", updates}});
    ASSERT_TRUE(resp.contains("result"));
    // Non-existent items are skipped; count == 0
    EXPECT_EQ(resp["result"]["count"].get<int>(), 0);
}

TEST_F(RPCBatchOperationsTest, BatchUpdateMergesFields) {
    DirectPut("upd", "T", "t1", {{"a", 1}, {"b", 2}});

    json updates = json::array({
        {{"collection", "upd"}, {"model", "T"}, {"uuid", "t1"}, {"updates", {{"b", 99}, {"c", 3}}}},
    });
    auto resp = service_->handleBatchUpdate({{"updates", updates}});
    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), 1);

    // Read back and verify merge
    std::string raw;
    ASSERT_TRUE(db_->get("upd:T:t1", raw));
    auto entity = json::parse(raw);
    EXPECT_EQ(entity["a"].get<int>(), 1);    // unchanged
    EXPECT_EQ(entity["b"].get<int>(), 99);   // updated
    EXPECT_EQ(entity["c"].get<int>(), 3);    // new field
}

// ============================================================================
// dispatch() – batch methods routed via the method dispatcher
// ============================================================================

TEST_F(RPCBatchOperationsTest, DispatchBatchGetRoutes) {
    themis::plugins::rpc::RPCRequestContext ctx;
    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    // Returns result (not an error about method not found)
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchBatchPutRoutes) {
    themis::plugins::rpc::RPCRequestContext ctx;
    json entities = json::array({EntitySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_put", {{"entities", entities}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchBatchDeleteRoutes) {
    themis::plugins::rpc::RPCRequestContext ctx;
    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_delete", {{"keys", keys}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchBatchUpdateRoutes) {
    themis::plugins::rpc::RPCRequestContext ctx;
    DirectPut("d", "M", "k1");
    json updates = json::array({
        {{"collection", "d"}, {"model", "M"}, {"uuid", "k1"}, {"updates", {{"x", 1}}}},
    });
    auto resp = service_->dispatch("batch_update", {{"updates", updates}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchReturnsTimeoutForExpiredGrpcDeadline) {
    themis::plugins::rpc::RPCRequestContext ctx;
    const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    ctx.timestamp_ms = now_ms - 250;
    ctx.metadata["grpc-timeout"] = "100m";

    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT));
}

TEST_F(RPCBatchOperationsTest, DispatchAllowsRequestWithinGrpcDeadline) {
    themis::plugins::rpc::RPCRequestContext ctx;
    const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    ctx.timestamp_ms = now_ms;
    ctx.metadata["grpc-timeout"] = "5S";

    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchTreatsNegativeMsTimeoutAsExpiredDeadline) {
    themis::plugins::rpc::RPCRequestContext ctx;
    const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    ctx.timestamp_ms = now_ms;
    ctx.metadata["x-timeout-ms"] = "-1";

    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    ASSERT_TRUE(resp.contains("error"));
    EXPECT_EQ(resp["error"]["code"].get<int>(),
              static_cast<int>(themis::plugins::rpc::RPCErrorCode::QUERY_TIMEOUT));
}

TEST_F(RPCBatchOperationsTest, DispatchDoesNotOverflowDeadlineArithmeticForFutureTimestamps) {
    themis::plugins::rpc::RPCRequestContext ctx;
    ctx.timestamp_ms = std::numeric_limits<uint64_t>::max() - 5;
    ctx.metadata["request-timeout-ms"] = "10";

    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchIgnoresMalformedGrpcTimeoutWithNonNumericValue) {
    themis::plugins::rpc::RPCRequestContext ctx;
    const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    ctx.timestamp_ms = now_ms - 5000;
    ctx.metadata["grpc-timeout"] = "1xS";

    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

TEST_F(RPCBatchOperationsTest, DispatchIgnoresMalformedMsTimeoutWithNonNumericSuffix) {
    themis::plugins::rpc::RPCRequestContext ctx;
    const auto now_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
    ctx.timestamp_ms = now_ms - 5000;
    ctx.metadata["x-timeout-ms"] = "10abc";

    json keys = json::array({KeySpec("d", "M", "k1")});
    auto resp = service_->dispatch("batch_get", {{"keys", keys}}, ctx);
    EXPECT_TRUE(resp.contains("result"));
}

// ============================================================================
// Performance – batch vs. individual operations
// ============================================================================

TEST_F(RPCBatchOperationsTest, BatchPutCompletesInReasonableTime) {
    const int n = 100;
    json entities = json::array();
    for (int i = 0; i < n; ++i) {
        entities.push_back(EntitySpec("perf", "B", "bat-" + std::to_string(i), {{"v", i}}));
    }

    auto t0 = Clock::now();
    auto resp = service_->handleBatchPut({{"entities", entities}});
    double elapsed_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), n);
    // 100 batch puts must complete within 1 second on any CI machine
    // (typical hardware should complete in < 100 ms; 1 s accounts for slow CI)
    EXPECT_LT(elapsed_ms, 1000.0)
        << "Batch put of " << n << " items took " << elapsed_ms << " ms";
}

TEST_F(RPCBatchOperationsTest, BatchGetCompletesInReasonableTime) {
    const int n = 100;
    for (int i = 0; i < n; ++i) {
        DirectPut("perf2", "R", "r-" + std::to_string(i), {{"v", i}});
    }

    json keys = json::array();
    for (int i = 0; i < n; ++i) {
        keys.push_back(KeySpec("perf2", "R", "r-" + std::to_string(i)));
    }

    auto t0 = Clock::now();
    auto resp = service_->handleBatchGet({{"keys", keys}});
    double elapsed_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), n);
    // 100 batch reads must complete within 500 ms on any CI machine
    // (typical hardware should complete in < 50 ms)
    EXPECT_LT(elapsed_ms, 500.0)
        << "Batch get of " << n << " items took " << elapsed_ms << " ms";
}

TEST_F(RPCBatchOperationsTest, BatchDeleteCompletesInReasonableTime) {
    const int n = 100;
    for (int i = 0; i < n; ++i) {
        DirectPut("del_perf", "D", "d-" + std::to_string(i));
    }

    json keys = json::array();
    for (int i = 0; i < n; ++i) {
        keys.push_back(KeySpec("del_perf", "D", "d-" + std::to_string(i)));
    }

    auto t0 = Clock::now();
    auto resp = service_->handleBatchDelete({{"keys", keys}});
    double elapsed_ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    ASSERT_TRUE(resp.contains("result"));
    EXPECT_EQ(resp["result"]["count"].get<int>(), n);
    // 100 batch deletes should complete well within 500 ms on any reasonable hardware/CI
    EXPECT_LT(elapsed_ms, 500.0) << "Batch delete took " << elapsed_ms << " ms";
}
