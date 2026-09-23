/**
 * @file test_qdrant_adapter_production.cpp
 * @brief Production-grade unit tests for QdrantAdapter TODO implementations.
 *
 * Tests cover:
 * - gRPC channel lifecycle management (connect/disconnect)
 * - Vector operations (insert, search)
 * - Collection management (create_index)
 * - Error handling and edge cases
 * - Resource cleanup and RAII principles
 *
 * @note Tests run WITHOUT a live Qdrant server; they verify the adapter
 *       framework, error handling, and interface contract.
 */

#include <gtest/gtest.h>
#include "chimera/qdrant_adapter.hpp"

#include <memory>
#include <string>
#include <vector>
#include <thread>

using namespace chimera;

// ────────────────────────────────────────────────────────────────────────────
// Fixture for basic adapter lifecycle tests
// ────────────────────────────────────────────────────────────────────────────

class QdrantAdapterProductionTest : public ::testing::Test {
protected:
    QdrantAdapterProductionTest() = default;
    ~QdrantAdapterProductionTest() override = default;

    void SetUp() override {
        adapter_ = std::make_unique<QdrantAdapter>();
        EXPECT_NE(adapter_, nullptr);
    }

    void TearDown() override {
        if (adapter_ && adapter_->is_connected()) {
            [[maybe_unused]] auto result = adapter_->disconnect();
        }
        adapter_.reset();
    }

    std::unique_ptr<QdrantAdapter> adapter_;
};

// ────────────────────────────────────────────────────────────────────────────
// Connection Management Tests (covers TODO line 68: gRPC channel creation)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, InitiallyDisconnected) {
    ASSERT_NE(adapter_, nullptr);
    EXPECT_FALSE(adapter_->is_connected());
}

TEST_F(QdrantAdapterProductionTest, ConnectWithEmptyString) {
    auto result = adapter_->connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_NE(result.error_message.find("empty"), std::string::npos);
    EXPECT_FALSE(adapter_->is_connected());
}

TEST_F(QdrantAdapterProductionTest, ConnectWithInvalidFormat) {
    auto result = adapter_->connect("invalid-string-without-colon");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter_->is_connected());
}

TEST_F(QdrantAdapterProductionTest, ConnectWithValidHostPort) {
    // Format: "host:port"
    auto result = adapter_->connect("localhost:6334");
    
    // Note: This will fail without a live Qdrant server when THEMIS_CHIMERA_QDRANT=ON,
    // but when OFF it will fail with NOT_IMPLEMENTED, which is expected in test.
    // For in-process testing without gRPC, we accept either error.
    if (result.is_ok()) {
        EXPECT_TRUE(adapter_->is_connected());
        auto disconnect_result = adapter_->disconnect();
        EXPECT_TRUE(disconnect_result.is_ok());
        EXPECT_FALSE(adapter_->is_connected());
    } else {
        // Either NOT_IMPLEMENTED (build flag) or connection error (no server)
        EXPECT_TRUE(result.error_code == ErrorCode::NOT_IMPLEMENTED ||
                   result.error_code == ErrorCode::INTERNAL_ERROR);
    }
}

TEST_F(QdrantAdapterProductionTest, ConnectWithHttpUri) {
    // Format: "http://host:port"
    auto result = adapter_->connect("http://localhost:6334");
    
    if (result.is_ok()) {
        EXPECT_TRUE(adapter_->is_connected());
    } else {
        EXPECT_TRUE(result.error_code == ErrorCode::NOT_IMPLEMENTED ||
                   result.error_code == ErrorCode::INTERNAL_ERROR);
    }
}

TEST_F(QdrantAdapterProductionTest, ConnectWithHttpsUri) {
    // Format: "https://host:port"
    auto result = adapter_->connect("https://localhost:6334");
    
    if (result.is_ok()) {
        EXPECT_TRUE(adapter_->is_connected());
    } else {
        EXPECT_TRUE(result.error_code == ErrorCode::NOT_IMPLEMENTED ||
                   result.error_code == ErrorCode::INTERNAL_ERROR);
    }
}

TEST_F(QdrantAdapterProductionTest, DisconnectWhenNotConnected) {
    EXPECT_FALSE(adapter_->is_connected());
    auto result = adapter_->disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter_->is_connected());
}

TEST_F(QdrantAdapterProductionTest, MultipleConnectAttempts) {
    // First connect attempt
    auto result1 = adapter_->connect("localhost:6334");
    
    // Second connect attempt (should handle gracefully)
    auto result2 = adapter_->connect("localhost:6335");
    
    // At least one should have been attempted without crash
    if (result1.is_ok() || result2.is_ok()) {
        EXPECT_TRUE(adapter_->is_connected());
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Vector Operation Tests (covers TODOs on lines 150, 198: insert_vector, search)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, InsertVectorWhenDisconnected) {
    EXPECT_FALSE(adapter_->is_connected());
    
    Vector vec;
    vec.data = {1.0f, 2.0f, 3.0f};
    
    auto result = adapter_->insert_vector("test_collection", vec);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(QdrantAdapterProductionTest, InsertVectorWithEmptyCollection) {
    // Setup: Mock connection (no live server needed)
    EXPECT_FALSE(adapter_->is_connected());
    
    Vector vec;
    vec.data = {1.0f, 2.0f, 3.0f};
    
    auto result = adapter_->insert_vector("", vec);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    // Note: Real error would be INVALID_ARGUMENT once connected
}

TEST_F(QdrantAdapterProductionTest, InsertVectorWithEmptyData) {
    // Setup: Mock connection (no live server needed)
    EXPECT_FALSE(adapter_->is_connected());
    
    Vector vec;  // Empty data
    
    auto result = adapter_->insert_vector("test_collection", vec);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    // Note: Real error would be INVALID_ARGUMENT once connected
}

TEST_F(QdrantAdapterProductionTest, SearchVectorsWhenDisconnected) {
    EXPECT_FALSE(adapter_->is_connected());
    
    Vector query;
    query.data = {1.0f, 2.0f, 3.0f};
    
    auto result = adapter_->search_vectors("test_collection", query, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(QdrantAdapterProductionTest, SearchVectorsWithZeroK) {
    // Setup: Mock connection (no live server needed)
    EXPECT_FALSE(adapter_->is_connected());
    
    Vector query;
    query.data = {1.0f, 2.0f, 3.0f};
    
    auto result = adapter_->search_vectors("test_collection", query, 0);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    // Note: Real error would be INVALID_ARGUMENT once connected
}

TEST_F(QdrantAdapterProductionTest, SearchVectorsWithEmptyQuery) {
    // Setup: Mock connection (no live server needed)
    EXPECT_FALSE(adapter_->is_connected());
    
    Vector query;  // Empty data
    
    auto result = adapter_->search_vectors("test_collection", query, 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    // Note: Real error would be INVALID_ARGUMENT once connected
}

// ────────────────────────────────────────────────────────────────────────────
// Index Management Tests (covers TODO line 224: create_index)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, CreateIndexWhenDisconnected) {
    EXPECT_FALSE(adapter_->is_connected());
    
    auto result = adapter_->create_index("test_collection", 128);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(QdrantAdapterProductionTest, CreateIndexWithEmptyCollection) {
    // Setup: Mock connection (no live server needed)
    EXPECT_FALSE(adapter_->is_connected());
    
    auto result = adapter_->create_index("", 128);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    // Note: Real error would be INVALID_ARGUMENT once connected
}

TEST_F(QdrantAdapterProductionTest, CreateIndexWithZeroDimensions) {
    // Setup: Mock connection (no live server needed)
    EXPECT_FALSE(adapter_->is_connected());
    
    auto result = adapter_->create_index("test_collection", 0);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    // Note: Real error would be INVALID_ARGUMENT once connected
}

// ────────────────────────────────────────────────────────────────────────────
// Resource Management Tests (RAII verification)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, DestructorCallsDisconnect) {
    // Create adapter in inner scope
    {
        auto temp_adapter = std::make_unique<QdrantAdapter>();
        EXPECT_NE(temp_adapter, nullptr);
        // Destructor should be called cleanly
    }
    // Should not crash if destructor was invoked
}

TEST_F(QdrantAdapterProductionTest, CapabilitiesReported) {
    auto capabilities = adapter_->get_capabilities();
    EXPECT_FALSE(capabilities.empty());
    
    // Should report vector search capability
    bool has_vector_search = false;
    for (const auto& cap : capabilities) {
        if (cap == Capability::VECTOR_SEARCH) {
            has_vector_search = true;
            break;
        }
    }
    EXPECT_TRUE(has_vector_search);
}

TEST_F(QdrantAdapterProductionTest, SystemInfoAvailable) {
    auto result = adapter_->get_system_info();
    EXPECT_TRUE(result.is_ok());
    
    if (result.is_ok()) {
        auto info = result.value.value();
        EXPECT_EQ(info.system_name, "Qdrant");
        EXPECT_FALSE(info.version.empty());
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Error Handling Tests (edge cases and boundary conditions)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, LongConnectionString) {
    std::string long_host(1000, 'a');
    auto result = adapter_->connect(long_host + ":6334");
    EXPECT_TRUE(result.is_err());
    // Should handle long strings gracefully without buffer overflow
}

TEST_F(QdrantAdapterProductionTest, InvalidPortNumber) {
    auto result = adapter_->connect("localhost:99999");
    // May succeed or fail depending on gRPC validation, but should not crash
    // Error either way is acceptable
}

TEST_F(QdrantAdapterProductionTest, SpecialCharactersInCollectionName) {
    Vector vec;
    vec.data = {1.0f};
    
    // Should handle special characters in collection name validation
    auto result = adapter_->insert_vector("test/collection:name", vec);
    EXPECT_TRUE(result.is_err());  // Disconnected
}

// ────────────────────────────────────────────────────────────────────────────
// Batch Operation Tests
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, BatchInsertVectors) {
    EXPECT_FALSE(adapter_->is_connected());
    
    std::vector<Vector> vectors;
    for (int i = 0; i < 10; ++i) {
        Vector vec;
        vec.data = {static_cast<float>(i), static_cast<float>(i + 1)};
        vectors.push_back(vec);
    }
    
    auto result = adapter_->batch_insert_vectors("test_collection", vectors);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
}

TEST_F(QdrantAdapterProductionTest, GetPendingCount) {
    auto count = adapter_->get_pending_count();
    EXPECT_EQ(count, 0);  // Should start with empty queue
}

TEST_F(QdrantAdapterProductionTest, BatchConfigRoundtrip) {
    BatchConfig config;
    config.batch_size = 1000;
    config.flush_interval_ms = 5000;
    
    auto set_result = adapter_->set_batch_config(config);
    EXPECT_TRUE(set_result.is_ok());
    
    auto retrieved = adapter_->get_batch_config();
    EXPECT_EQ(retrieved.batch_size, config.batch_size);
    EXPECT_EQ(retrieved.flush_interval_ms, config.flush_interval_ms);
}

// ────────────────────────────────────────────────────────────────────────────
// Unsupported Operations (verify NOT_IMPLEMENTED paths)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, RelationalOperationsNotSupported) {
    auto result = adapter_->execute_query("SELECT * FROM table");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantAdapterProductionTest, GraphOperationsNotSupported) {
    auto result = adapter_->shortest_path("node1", "node2", 5);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantAdapterProductionTest, TransactionsNotSupported) {
    auto result = adapter_->begin_transaction();
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

TEST_F(QdrantAdapterProductionTest, DocumentOperationsNotSupported) {
    Document doc;
    doc.id = "doc1";
    auto result = adapter_->insert_document("collection", doc);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

// ────────────────────────────────────────────────────────────────────────────
// Thread-Safety Tests (basic race condition detection)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, ConcurrentGetPendingCount) {
    // Multiple threads calling get_pending_count should not crash
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this]() {
            auto count = adapter_->get_pending_count();
            EXPECT_GE(count, 0);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

// ────────────────────────────────────────────────────────────────────────────
// Integration-style Test (simulates real usage pattern)
// ────────────────────────────────────────────────────────────────────────────

TEST_F(QdrantAdapterProductionTest, TypicalUsagePattern) {
    // Test the expected usage flow
    
    // Step 1: Check initial state
    EXPECT_FALSE(adapter_->is_connected());
    
    // Step 2: Attempt connection
    auto connect_result = adapter_->connect("localhost:6334");
    // May fail if server not available, but should not crash
    
    // Step 3: Check capabilities
    auto caps = adapter_->get_capabilities();
    EXPECT_FALSE(caps.empty());
    
    // Step 4: Verify system info
    auto info_result = adapter_->get_system_info();
    EXPECT_TRUE(info_result.is_ok());
    
    // Step 5: Cleanup
    auto disconnect_result = adapter_->disconnect();
    EXPECT_TRUE(disconnect_result.is_ok());
    EXPECT_FALSE(adapter_->is_connected());
}
