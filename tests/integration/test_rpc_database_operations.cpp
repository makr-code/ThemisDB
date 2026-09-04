/**
 * @file test_rpc_database_operations.cpp
 * @brief Integration tests for RPC database operations (aligned with current RocksDBWrapper API)
 */

#include "test_fixture.h"
#include "test_data_generator.h"
#include "storage/rocksdb_wrapper.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <string>

using namespace themis;
using namespace themis::test;
using json = nlohmann::json;

class RPCDatabaseOperationsTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        db_path_ = CreateTestDbPath("rpc_test_db");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string();
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open()) << "Failed to open RocksDB test instance";
        data_gen_ = std::make_unique<TestDataGenerator>();
    }

    void TearDown() override {
        storage_.reset();
        IntegrationTestFixture::TearDown();
    }

    json CreateTestEntity(const std::string& collection, const std::string& model,
                          const std::string& uuid, const json& data = {}) {
        json entity = data;
        entity["_collection"] = collection;
        entity["_model"] = model;
        entity["uuid"] = uuid;
        entity["_timestamp_ns"] = std::chrono::system_clock::now().time_since_epoch().count();
        entity["_version"] = 1;
        return entity;
    }

    std::string MakeKey(const std::string& collection, const std::string& model,
                        const std::string& uuid) {
        return collection + ":" + model + ":" + uuid;
    }

    std::filesystem::path db_path_;
    std::shared_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<TestDataGenerator> data_gen_;
};

// Basic PUT/GET round-trip using convenience string APIs
TEST_F(RPCDatabaseOperationsTest, HandlePutAndGet) {
    json entity = CreateTestEntity("users", "User", "user-123", {
        {"name", "Alice"},
        {"email", "alice@example.com"},
        {"age", 30}
    });

    const std::string key = MakeKey("users", "User", "user-123");
    ASSERT_TRUE(storage_->put(key, entity.dump()));

    std::string retrieved = {};
    ASSERT_TRUE(storage_->get(key, retrieved));

    auto retrieved_entity = json::parse(retrieved);
    EXPECT_EQ(retrieved_entity["name"], "Alice");
    EXPECT_EQ(retrieved_entity["email"], "alice@example.com");
    EXPECT_EQ(retrieved_entity["age"], 30);
}

// Transaction commit should persist writes
TEST_F(RPCDatabaseOperationsTest, TransactionCommitPersistsData) {
    const std::string key = MakeKey("accounts", "Account", "acct-commit");
    json entity = CreateTestEntity("accounts", "Account", "acct-commit", {{"balance", 100.0}});

    auto tx = storage_->beginTransaction();
    ASSERT_NE(tx, nullptr);

    auto entity_str = entity.dump();
    std::vector<uint8_t> entity_vec(entity_str.begin(), entity_str.end());
    ASSERT_TRUE(tx->put(key, entity_vec));
    ASSERT_TRUE(tx->commit());

    std::string value = {};
    ASSERT_TRUE(storage_->get(key, value));
    auto retrieved = json::parse(value);
    EXPECT_EQ(retrieved["balance"], 100.0);
}

// Transaction rollback should discard writes
TEST_F(RPCDatabaseOperationsTest, TransactionRollbackDiscardsData) {
    const std::string key = MakeKey("accounts", "Account", "acct-rollback");
    json entity = CreateTestEntity("accounts", "Account", "acct-rollback", {{"balance", 200.0}});

    auto tx = storage_->beginTransaction();
    ASSERT_NE(tx, nullptr);

    auto entity_str = entity.dump();
    std::vector<uint8_t> entity_vec(entity_str.begin(), entity_str.end());
    ASSERT_TRUE(tx->put(key, entity_vec));
    tx->rollback();

    std::string value = {};
    EXPECT_FALSE(storage_->get(key, value));
}

// get() optional API should return binary payload
TEST_F(RPCDatabaseOperationsTest, OptionalGetReturnsData) {
    const std::string key = MakeKey("blob", "Blob", "blob-1");
    const std::string payload = "binary-data";
    ASSERT_TRUE(storage_->put(key, payload));

    auto result = storage_->get(key);
    ASSERT_TRUE(result.has_value());
    std::string restored(result->begin(), result->end());
    EXPECT_EQ(restored, payload);
}

// Smoke test for write batch wrapper
TEST_F(RPCDatabaseOperationsTest, WriteBatchCommit) {
    auto batch = storage_->createWriteBatch();
    ASSERT_NE(batch, nullptr);

    for (int i = 0; i < 5; ++i) {
        auto key = MakeKey("batch", "Item", "item-" + std::to_string(i));
        auto entity = CreateTestEntity("batch", "Item", "item-" + std::to_string(i), {{"index", i}});
        auto entity_str = entity.dump();
        std::vector<uint8_t> entity_vec(entity_str.begin(), entity_str.end());
        batch->put(key, entity_vec);
    }

    ASSERT_TRUE(batch->commit());
}
