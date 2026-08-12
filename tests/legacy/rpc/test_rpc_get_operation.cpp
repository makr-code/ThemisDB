/**
 * @file test_rpc_get_operation.cpp
 * @brief Unit tests for the RPC GET operation with a real RocksDB backend.
 *
 * Covers:
 *  - Basic GET round-trip (document found)
 *  - GET for a non-existent document (not found)
 *  - GET with missing / invalid parameters
 *  - GET with null storage pointer
 *  - GET with various entity field types (string, int, float, bool, array, object)
 *  - Batch GET with a mix of found and missing documents
 */

#include <gtest/gtest.h>
#include "server/rpc_service_impl.h"
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace themis;
using namespace themis::server::rpc;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

/// Key format used by the RPC service: collection:model:uuid
static std::string makeEntityKey(const std::string& collection,
                                 const std::string& model,
                                 const std::string& uuid) {
    return collection + ":" + model + ":" + uuid;
}

class RPCGetOperationTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "test_rpc_get_op_db";
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        cfg.memtable_size_mb = 16;
        cfg.block_cache_size_mb = 16;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open RocksDB test instance";

        svc_ = std::make_unique<ThemisRPCService>(db_.get());
    }

    void TearDown() override {
        svc_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }

    /// Helper: write an entity directly into RocksDB using the RPC key schema.
    void putEntity(const std::string& collection, const std::string& model,
                   const std::string& uuid, const json& entity) {
        std::string key = makeEntityKey(collection, model, uuid);
        ASSERT_TRUE(db_->put(key, entity.dump()))
            << "Direct RocksDB put failed for key: " << key;
    }

    std::string db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<ThemisRPCService> svc_;
};

// ---------------------------------------------------------------------------
// Basic round-trip: PUT via storage, GET via RPC service
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, GetFoundDocument) {
    json entity = {
        {"uuid", "doc-1"},
        {"_collection", "users"},
        {"_model", "User"},
        {"_version", 1},
        {"name", "Alice"},
        {"age", 30}
    };
    putEntity("users", "User", "doc-1", entity);

    json params = {{"collection", "users"}, {"model", "User"}, {"uuid", "doc-1"}};
    json resp = svc_->handleGet(params);

    ASSERT_TRUE(resp.contains("result")) << "Response must have 'result'";
    const auto& result = resp["result"];
    EXPECT_TRUE(result.value("found", false)) << "Document should be found";
    ASSERT_TRUE(result.contains("entity")) << "Result must contain 'entity'";
    EXPECT_EQ(result["entity"]["name"], "Alice");
    EXPECT_EQ(result["entity"]["age"], 30);
}

// ---------------------------------------------------------------------------
// GET for a document that does not exist
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, GetNotFoundDocument) {
    json params = {
        {"collection", "users"}, {"model", "User"}, {"uuid", "nonexistent-uuid"}
    };
    json resp = svc_->handleGet(params);

    ASSERT_TRUE(resp.contains("result")) << "Response must have 'result'";
    EXPECT_FALSE(resp["result"].value("found", true)) << "Document should not be found";
}

// ---------------------------------------------------------------------------
// GET with missing required parameters
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, GetMissingCollection) {
    json params = {{"model", "User"}, {"uuid", "doc-1"}};
    json resp = svc_->handleGet(params);
    ASSERT_TRUE(resp.contains("error")) << "Missing 'collection' should return error";
}

TEST_F(RPCGetOperationTest, GetMissingModel) {
    json params = {{"collection", "users"}, {"uuid", "doc-1"}};
    json resp = svc_->handleGet(params);
    ASSERT_TRUE(resp.contains("error")) << "Missing 'model' should return error";
}

TEST_F(RPCGetOperationTest, GetMissingUuid) {
    json params = {{"collection", "users"}, {"model", "User"}};
    json resp = svc_->handleGet(params);
    ASSERT_TRUE(resp.contains("error")) << "Missing 'uuid' should return error";
}

TEST_F(RPCGetOperationTest, GetEmptyParams) {
    json resp = svc_->handleGet(json::object());
    ASSERT_TRUE(resp.contains("error")) << "Empty params should return error";
}

// ---------------------------------------------------------------------------
// GET with null storage (service initialised without a db pointer)
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, GetNullStorage) {
    ThemisRPCService null_svc(nullptr);
    json params = {{"collection", "c"}, {"model", "M"}, {"uuid", "u"}};
    json resp = null_svc.handleGet(params);
    ASSERT_TRUE(resp.contains("error")) << "Null storage should return error";
}

// ---------------------------------------------------------------------------
// GET with various entity field types (all supported JSON types)
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, GetEntityWithAllFieldTypes) {
    json entity = {
        {"uuid", "typed-doc"},
        {"_collection", "typed"},
        {"_model", "TypedModel"},
        {"_version", 2},
        // string field
        {"str_field", "hello"},
        // integer field
        {"int_field", 42},
        // float field
        {"float_field", 3.14},
        // boolean field
        {"bool_field", true},
        // null field
        {"null_field", nullptr},
        // array field
        {"array_field", json::array({1, 2, 3})},
        // nested object field
        {"obj_field", {{"nested_key", "nested_value"}, {"nested_int", 99}}}
    };
    putEntity("typed", "TypedModel", "typed-doc", entity);

    json params = {{"collection", "typed"}, {"model", "TypedModel"}, {"uuid", "typed-doc"}};
    json resp = svc_->handleGet(params);

    ASSERT_TRUE(resp.contains("result"));
    const auto& result = resp["result"];
    ASSERT_TRUE(result.value("found", false)) << "Typed document should be found";
    const auto& ent = result["entity"];

    EXPECT_EQ(ent["str_field"],   "hello");
    EXPECT_EQ(ent["int_field"],   42);
    EXPECT_NEAR(ent["float_field"].get<double>(), 3.14, 1e-9);
    EXPECT_EQ(ent["bool_field"],  true);
    EXPECT_TRUE(ent["null_field"].is_null());
    EXPECT_EQ(ent["array_field"], json::array({1, 2, 3}));
    EXPECT_EQ(ent["obj_field"]["nested_key"], "nested_value");
    EXPECT_EQ(ent["obj_field"]["nested_int"], 99);
}

// ---------------------------------------------------------------------------
// Version and timestamp metadata are preserved
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, GetPreservesVersionAndTimestamp) {
    constexpr int64_t kTestTimestampNs = 1234567890LL;
    json entity = {
        {"uuid", "versioned-doc"},
        {"_collection", "col"},
        {"_model", "Mod"},
        {"_version", 5},
        {"_timestamp_ns", kTestTimestampNs}
    };
    putEntity("col", "Mod", "versioned-doc", entity);

    json params = {{"collection", "col"}, {"model", "Mod"}, {"uuid", "versioned-doc"}};
    json resp = svc_->handleGet(params);

    ASSERT_TRUE(resp.contains("result"));
    const auto& result = resp["result"];
    EXPECT_EQ(result.value("version", -1), 5);
    EXPECT_EQ(result.value("timestamp_ns", -1LL), kTestTimestampNs);
}

// ---------------------------------------------------------------------------
// Batch GET: mix of found and missing documents
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, BatchGetFoundAndMissing) {
    // Insert two documents
    putEntity("batch", "Item", "item-0",
              json{{"uuid","item-0"},{"_collection","batch"},{"_model","Item"},{"_version",1},{"val",10}});
    putEntity("batch", "Item", "item-2",
              json{{"uuid","item-2"},{"_collection","batch"},{"_model","Item"},{"_version",1},{"val",30}});

    json params = {
        {"keys", json::array({
            {{"collection","batch"},{"model","Item"},{"uuid","item-0"}},
            {{"collection","batch"},{"model","Item"},{"uuid","item-1"}},  // missing
            {{"collection","batch"},{"model","Item"},{"uuid","item-2"}}
        })}
    };
    json resp = svc_->handleBatchGet(params);

    ASSERT_TRUE(resp.contains("result")) << "Batch GET must return 'result'";
    const auto& result = resp["result"];
    ASSERT_TRUE(result.contains("results"));
    ASSERT_EQ(result["results"].size(), 3u);

    EXPECT_TRUE(result["results"][0].value("found", false))  << "item-0 should be found";
    EXPECT_FALSE(result["results"][1].value("found", true))  << "item-1 should be missing";
    EXPECT_TRUE(result["results"][2].value("found", false))  << "item-2 should be found";

    EXPECT_EQ(result["results"][0]["entity"]["val"], 10);
    EXPECT_EQ(result["results"][2]["entity"]["val"], 30);
}

// ---------------------------------------------------------------------------
// Batch GET with missing required key fields returns error
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, BatchGetInvalidKeyFormat) {
    // Each key entry must have collection, model, uuid
    json params = {
        {"keys", json::array({
            {{"collection","c"}}  // missing model and uuid
        })}
    };
    json resp = svc_->handleBatchGet(params);
    ASSERT_TRUE(resp.contains("error")) << "Incomplete key should return error";
}

// ---------------------------------------------------------------------------
// Batch GET with missing 'keys' parameter
// ---------------------------------------------------------------------------

TEST_F(RPCGetOperationTest, BatchGetMissingKeysParam) {
    json resp = svc_->handleBatchGet(json::object());
    ASSERT_TRUE(resp.contains("error")) << "Missing 'keys' should return error";
}
