/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_test_utils.hpp                             ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:22:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     244                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// SPDX-License-Identifier: Apache-2.0 OR MIT
// Copyright (c) 2026 CHIMERA Suite Contributors
//
// Common test utilities for CHIMERA database adapters
// This file provides shared test infrastructure for all adapters

#pragma once

#include "chimera/database_adapter.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace chimera {
namespace testing {

/// Helper function to create a test document
inline Document create_test_document(const std::string& id, 
                                     const std::string& name,
                                     int64_t value) {
    Document doc;
    doc["id"] = id;
    doc["name"] = name;
    doc["value"] = value;
    return doc;
}

/// Helper function to create a document with a vector field
inline Document create_vector_document(const std::string& id,
                                       const std::vector<float>& vector) {
    Document doc;
    doc["id"] = id;
    doc["vector"] = vector;
    return doc;
}

/// Base test fixture for adapter tests
class AdapterTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Subclasses should override this to:
        // 1. Create the adapter instance
        // 2. Connect to the test database
        // 3. Create test collections if needed
    }
    
    void TearDown() override {
        // Clean up test data and disconnect
        if (adapter && adapter->is_connected()) {
            // Drop test collections
            for (const auto& collection : test_collections) {
                adapter->drop_collection(collection);
            }
            adapter->disconnect();
        }
    }
    
    /// Helper to register a test collection for cleanup
    void register_test_collection(const std::string& name) {
        test_collections.push_back(name);
    }
    
    std::unique_ptr<DatabaseAdapter> adapter;
    std::vector<std::string> test_collections;
};

/// Macro to define standard adapter tests
#define CHIMERA_ADAPTER_TEST_SUITE(AdapterClass, TestName) \
    class TestName : public AdapterTestFixture { \
    protected: \
        void SetUp() override; \
    }; \
    \
    TEST_F(TestName, ConnectionTest) { \
        ASSERT_NE(adapter, nullptr); \
        EXPECT_TRUE(adapter->is_connected()); \
        auto status = adapter->ping(); \
        EXPECT_TRUE(status.ok()) << status.message; \
    } \
    \
    TEST_F(TestName, BasicInsert) { \
        register_test_collection("test_insert"); \
        adapter->create_collection("test_insert"); \
        \
        auto doc = create_test_document("test_1", "Test", 42); \
        auto status = adapter->insert("test_insert", doc); \
        EXPECT_TRUE(status.ok()) << status.message; \
    } \
    \
    TEST_F(TestName, FindById) { \
        register_test_collection("test_find"); \
        adapter->create_collection("test_find"); \
        \
        auto doc = create_test_document("test_1", "Test", 42); \
        adapter->insert("test_find", doc); \
        \
        Document result; \
        auto status = adapter->find_by_id("test_find", "test_1", result); \
        EXPECT_TRUE(status.ok()) << status.message; \
        \
        if (status.ok()) { \
            EXPECT_EQ(std::get<std::string>(result.at("name")), "Test"); \
            EXPECT_EQ(std::get<int64_t>(result.at("value")), 42); \
        } \
    } \
    \
    TEST_F(TestName, BatchInsert) { \
        register_test_collection("test_batch"); \
        adapter->create_collection("test_batch"); \
        \
        std::vector<Document> docs; \
        for (int i = 0; i < 10; ++i) { \
            docs.push_back(create_test_document( \
                "test_" + std::to_string(i), \
                "Test" + std::to_string(i), \
                i \
            )); \
        } \
        \
        auto status = adapter->insert_batch("test_batch", docs); \
        EXPECT_TRUE(status.ok()) << status.message; \
        \
        size_t count = 0; \
        status = adapter->count("test_batch", {}, count); \
        EXPECT_TRUE(status.ok()); \
        EXPECT_EQ(count, 10); \
    } \
    \
    TEST_F(TestName, Update) { \
        register_test_collection("test_update"); \
        adapter->create_collection("test_update"); \
        \
        auto doc = create_test_document("test_1", "Original", 42); \
        adapter->insert("test_update", doc); \
        \
        Document filter; \
        filter["id"] = std::string("test_1"); \
        Document update; \
        update["name"] = std::string("Updated"); \
        \
        size_t updated_count = 0; \
        auto status = adapter->update("test_update", filter, update, updated_count); \
        EXPECT_TRUE(status.ok()) << status.message; \
        EXPECT_EQ(updated_count, 1); \
        \
        Document result; \
        adapter->find_by_id("test_update", "test_1", result); \
        EXPECT_EQ(std::get<std::string>(result.at("name")), "Updated"); \
    } \
    \
    TEST_F(TestName, Remove) { \
        register_test_collection("test_remove"); \
        adapter->create_collection("test_remove"); \
        \
        auto doc = create_test_document("test_1", "ToDelete", 42); \
        adapter->insert("test_remove", doc); \
        \
        Document filter; \
        filter["id"] = std::string("test_1"); \
        \
        size_t deleted_count = 0; \
        auto status = adapter->remove("test_remove", filter, deleted_count); \
        EXPECT_TRUE(status.ok()) << status.message; \
        EXPECT_EQ(deleted_count, 1); \
        \
        Document result; \
        status = adapter->find_by_id("test_remove", "test_1", result); \
        EXPECT_EQ(status.code, ErrorCode::NOT_FOUND); \
    } \
    \
    TEST_F(TestName, VectorSearchIfSupported) { \
        if (!adapter->supports_vector_search()) { \
            GTEST_SKIP() << "Vector search not supported"; \
        } \
        \
        register_test_collection("test_vectors"); \
        adapter->create_collection("test_vectors"); \
        \
        std::vector<float> vec1 = {0.1f, 0.2f, 0.3f}; \
        std::vector<float> vec2 = {0.2f, 0.3f, 0.4f}; \
        \
        adapter->insert("test_vectors", create_vector_document("vec1", vec1)); \
        adapter->insert("test_vectors", create_vector_document("vec2", vec2)); \
        \
        VectorSearchParams params; \
        params.query_vector = {0.15f, 0.25f, 0.35f}; \
        params.k = 2; \
        params.metric = "cosine"; \
        \
        ResultSet results; \
        auto status = adapter->vector_search("test_vectors", "vector", params, results); \
        EXPECT_TRUE(status.ok()) << status.message; \
        EXPECT_LE(results.size(), 2); \
    } \
    \
    TEST_F(TestName, TransactionsIfSupported) { \
        if (!adapter->supports_transactions()) { \
            GTEST_SKIP() << "Transactions not supported"; \
        } \
        \
        register_test_collection("test_txn"); \
        adapter->create_collection("test_txn"); \
        \
        std::unique_ptr<Transaction> txn; \
        auto status = adapter->begin_transaction(txn); \
        EXPECT_TRUE(status.ok()) << status.message; \
        ASSERT_NE(txn, nullptr); \
        EXPECT_TRUE(txn->is_active()); \
        \
        status = txn->commit(); \
        EXPECT_TRUE(status.ok()); \
        EXPECT_FALSE(txn->is_active()); \
    } \
    \
    TEST_F(TestName, GetCapabilities) { \
        auto capabilities = adapter->get_capabilities(); \
        EXPECT_FALSE(capabilities.empty()); \
        EXPECT_TRUE(std::find(capabilities.begin(), capabilities.end(), "crud") != capabilities.end()); \
    }

} // namespace testing
} // namespace chimera
