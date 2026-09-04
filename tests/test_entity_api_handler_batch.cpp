/**
 * ThemisDB Entity API Handler Batch Operations Unit Tests
 *
 * Tests for the POST /entities/batch endpoint (handleBatch) which supports
 * atomic bulk inserts and deletes via the EntityApiHandler.
 */

#include <gtest/gtest.h>
#include "server/entity_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "transaction/transaction_manager.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "server/auth_middleware.h"
#include "storage/key_schema.h"
#include <memory>
#include <string>
#include <filesystem>
#include <ctime>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

using namespace themis;
using namespace themis::server;
using json = nlohmann::json;

class EntityApiBatchTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable entity batch API tests on Windows";
#endif
        test_db_path_ = std::filesystem::temp_directory_path() /
                       ("themis_batch_test_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(getpid()));
        std::filesystem::create_directories(test_db_path_);

        RocksDBWrapper::Config config;
        config.db_path = test_db_path_.string();
        storage_ = std::make_shared<RocksDBWrapper>(config);
        storage_->open();

        secondary_index_ = std::make_shared<SecondaryIndexManager>(*storage_);
        graph_index_     = std::make_shared<GraphIndexManager>(*storage_);
        vector_index_    = std::make_shared<VectorIndexManager>(*storage_);

        tx_manager_ = std::make_shared<TransactionManager>(
            *storage_, *secondary_index_, *graph_index_, *vector_index_);

        key_provider_     = std::make_shared<themis::MockKeyProvider>();
        field_encryption_ = std::make_shared<FieldEncryption>(key_provider_);
        auth_             = std::make_shared<themis::AuthMiddleware>();
    }

    void TearDown() override {
        secondary_index_.reset();
        graph_index_.reset();
        vector_index_.reset();
        tx_manager_.reset();
        storage_.reset();
        std::filesystem::remove_all(test_db_path_);
    }

    EntityApiHandler makeHandler() {
        EntityApiConfig cfg = {};
        return EntityApiHandler(
            storage_, secondary_index_, graph_index_, tx_manager_,
            field_encryption_, key_provider_, auth_, cfg);
    }

    boost::beast::http::request<boost::beast::http::string_body> makeBatchRequest(
        const std::string& body)
    {
        namespace http = boost::beast::http;
        http::request<http::string_body> req;
        req.method(http::verb::post);
        req.target("/entities/batch");
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();
        return req;
    }

    std::filesystem::path                       test_db_path_;
    std::shared_ptr<RocksDBWrapper>             storage_;
    std::shared_ptr<SecondaryIndexManager>      secondary_index_;
    std::shared_ptr<GraphIndexManager>          graph_index_;
    std::shared_ptr<VectorIndexManager>         vector_index_;
    std::shared_ptr<TransactionManager>         tx_manager_;
    std::shared_ptr<FieldEncryption>            field_encryption_;
    std::shared_ptr<themis::KeyProvider>        key_provider_;
    std::shared_ptr<themis::AuthMiddleware>     auth_;
};

// ---------------------------------------------------------------------------
// Batch insert: two PUT operations → both entities must be readable afterwards
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, BatchInsertTwoEntities) {
    auto handler = makeHandler();

    json body = {
        {"operations", json::array({
            {{"op","put"}, {"key","users:alice"}, {"blob",R"({"name":"Alice","age":30})"}},
            {{"op","put"}, {"key","users:bob"},   {"blob",R"({"name":"Bob","age":25})"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    EXPECT_TRUE(resp_json["success"].get<bool>());
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 2);
    EXPECT_EQ(resp_json["failed"].get<int>(), 0);

    EXPECT_TRUE(storage_->get(KeySchema::makeRelationalKey("users", "alice")).has_value());
    EXPECT_TRUE(storage_->get(KeySchema::makeRelationalKey("users", "bob")).has_value());
}

// ---------------------------------------------------------------------------
// Batch delete: insert an entity first, then delete it via batch
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, BatchDeleteEntity) {
    // Pre-insert an entity using PUT so that there is something to delete
    auto handler = makeHandler();

    {
        namespace http = boost::beast::http;
        http::request<http::string_body> put_req;
        put_req.method(http::verb::put);
        put_req.target("/entities/orders:o1");
        put_req.set(http::field::content_type, "application/json");
        put_req.body() = R"({"key":"orders:o1","blob":"{\"item\":\"widget\"}"})";
        put_req.prepare_payload();
        auto put_resp = handler.handlePut(put_req);
        ASSERT_EQ(put_resp.result(), boost::beast::http::status::created);
    }

    ASSERT_TRUE(storage_->get(KeySchema::makeRelationalKey("orders", "o1")).has_value());

    json body = {
        {"operations", json::array({
            {{"op","delete"}, {"key","orders:o1"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    EXPECT_TRUE(resp_json["success"].get<bool>());
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 1);
    EXPECT_EQ(resp_json["failed"].get<int>(), 0);

    EXPECT_FALSE(storage_->get(KeySchema::makeRelationalKey("orders", "o1")).has_value());
}

// ---------------------------------------------------------------------------
// Mixed batch: one insert + one delete succeed atomically
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, MixedBatchPutAndDelete) {
    auto handler = makeHandler();

    // Pre-insert entity to delete
    {
        namespace http = boost::beast::http;
        http::request<http::string_body> put_req;
        put_req.method(http::verb::put);
        put_req.target("/entities/products:old");
        put_req.set(http::field::content_type, "application/json");
        put_req.body() = R"({"key":"products:old","blob":"{\"name\":\"old\"}"})";
        put_req.prepare_payload();
        handler.handlePut(put_req);
    }

    json body = {
        {"operations", json::array({
            {{"op","put"},    {"key","products:new"}, {"blob",R"({"name":"new"})"}},
            {{"op","delete"}, {"key","products:old"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 2);
    EXPECT_EQ(resp_json["failed"].get<int>(), 0);

    EXPECT_TRUE(storage_->get(KeySchema::makeRelationalKey("products", "new")).has_value());
    EXPECT_FALSE(storage_->get(KeySchema::makeRelationalKey("products", "old")).has_value());
}

// ---------------------------------------------------------------------------
// Validation: empty operations array is rejected with 400
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, EmptyOperationsArrayRejected) {
    auto handler = makeHandler();

    json body = {{"operations", json::array()}};
    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Validation: missing 'operations' field is rejected with 400
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, MissingOperationsFieldRejected) {
    auto handler = makeHandler();

    auto resp = handler.handleBatch(makeBatchRequest(R"({"data":[]})"));

    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

// ---------------------------------------------------------------------------
// Validation: invalid key format (missing colon) reported as per-item error,
//             valid operations in the same batch are still committed
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, InvalidKeyFormatReportedAsError) {
    auto handler = makeHandler();

    json body = {
        {"operations", json::array({
            {{"op","put"}, {"key","nocolon"}, {"blob",R"({"x":1})"}},
            {{"op","put"}, {"key","valid:k1"}, {"blob",R"({"x":2})"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    // One valid, one failed
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 1);
    EXPECT_EQ(resp_json["failed"].get<int>(), 1);
    EXPECT_TRUE(resp_json.contains("errors"));
}

TEST_F(EntityApiBatchTest, InvalidKeyTraversalReportedAsError) {
    auto handler = makeHandler();

    json body = {
        {"operations", json::array({
            {{"op","put"}, {"key","../users:alice"}, {"blob",R"({"x":1})"}},
            {{"op","put"}, {"key","valid:k1"}, {"blob",R"({"x":2})"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 1);
    EXPECT_EQ(resp_json["failed"].get<int>(), 1);
    EXPECT_TRUE(resp_json.contains("errors"));
}

TEST_F(EntityApiBatchTest, InvalidKeyHeaderInjectionReportedAsError) {
    auto handler = makeHandler();

    json body = {
        {"operations", json::array({
            {{"op","delete"}, {"key","users:alice\r\nbad"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 0);
    EXPECT_EQ(resp_json["failed"].get<int>(), 1);
    EXPECT_TRUE(resp_json.contains("errors"));
}

// ---------------------------------------------------------------------------
// Validation: unknown op type is reported as per-item error
// ---------------------------------------------------------------------------
TEST_F(EntityApiBatchTest, UnknownOpTypeReportedAsError) {
    auto handler = makeHandler();

    json body = {
        {"operations", json::array({
            {{"op","update"}, {"key","tbl:k"}, {"blob",R"({})"}}
        })}
    };

    auto resp = handler.handleBatch(makeBatchRequest(body.dump()));

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);

    auto resp_json = json::parse(resp.body());
    EXPECT_EQ(resp_json["succeeded"].get<int>(), 0);
    EXPECT_EQ(resp_json["failed"].get<int>(), 1);
    EXPECT_TRUE(resp_json.contains("errors"));
}
