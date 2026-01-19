/// @file test_query_engine_di.cpp
/// @brief Unit tests for QueryEngine with Dependency Injection
/// 
/// Tests the new DI constructors, late binding, and builder pattern.
/// Uses mock implementations to validate that QueryEngine can work
/// independently of concrete Storage and Index implementations.

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "query/query_engine.h"
#include "core/query_engine_builder.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "utils/expected.h"

using namespace themis;

// ============================================================================
// Mock Implementations for Testing
// ============================================================================

/// @brief Mock storage engine for testing
class MockStorageEngine : public IStorageEngine {
public:
    bool put(std::string_view key, std::string_view value) override {
        return true;
    }
    
    std::optional<std::string> get(std::string_view key) const override {
        return std::nullopt;
    }
    
    bool del(std::string_view key) override {
        return true;
    }
    
    bool exists(std::string_view key) const override {
        return false;
    }
    
    bool executeBatch(
        const std::vector<std::pair<std::string, std::string>>& puts,
        const std::vector<std::string>& deletes) override {
        return true;
    }
    
    void scanPrefix(std::string_view prefix, ScanCallback callback) const override {
        // Empty scan
    }
    
    void scanRange(std::string_view start_key, std::string_view end_key,
                   ScanCallback callback) const override {
        // Empty scan
    }
    
    std::unique_ptr<ITransaction> beginTransaction() override {
        return nullptr;
    }
    
    void flush() override {}
    
    void compact(std::optional<std::string_view> start_key,
                std::optional<std::string_view> end_key) override {}
    
    uint64_t getApproximateSize() const override {
        return 0;
    }
    
    std::string getStatistics() const override {
        return "{}";
    }
};

/// @brief Mock secondary index for testing
class MockSecondaryIndex : public ISecondaryIndex {
public:
    bool insert(std::string_view indexed_value,
               std::string_view primary_key) override {
        return true;
    }
    
    bool remove(std::string_view indexed_value,
               std::string_view primary_key) override {
        return true;
    }
    
    std::vector<std::string> lookup(std::string_view value) const override {
        return {};
    }
    
    std::vector<std::string> rangeScan(
        std::string_view start_value,
        std::string_view end_value,
        ScanOrder order) const override {
        return {};
    }
    
    std::string getName() const override {
        return "mock_index";
    }
    
    std::string getFieldName() const override {
        return "mock_field";
    }
    
    std::string getStatistics() const override {
        return "{}";
    }
};

/// @brief Mock index manager for testing
class MockIndexManager : public IIndexManager {
public:
    Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view name,
        std::string_view field_name,
        const std::string& config) override {
        return Ok<ISecondaryIndex*>(nullptr);
    }
    
    Result<IVectorIndex*> createVectorIndex(
        std::string_view name,
        uint32_t dimension,
        const std::string& config) override {
        return Ok<IVectorIndex*>(nullptr);
    }
    
    Result<IGraphIndex*> createGraphIndex(
        std::string_view name,
        const std::string& config) override {
        return Ok<IGraphIndex*>(nullptr);
    }
    
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view name) const override {
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND, 
                                       fmt::format("Index '{}' not found (mock)", name));
    }
    
    Result<IVectorIndex*> getVectorIndex(std::string_view name) const override {
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    fmt::format("Index '{}' not found (mock)", name));
    }
    
    Result<IGraphIndex*> getGraphIndex(std::string_view name) const override {
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                   fmt::format("Index '{}' not found (mock)", name));
    }
    
    Result<void> dropIndex(std::string_view name) override {
        return OkVoid();
    }
    
    std::vector<std::string> listIndexes() const override {
        return {};
    }
    
    std::optional<IndexType> getIndexType(std::string_view name) const override {
        return std::nullopt;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

/// @brief Test basic DI constructor
TEST(QueryEngineDITest, ConstructorWithInterfaces) {
    auto storage = std::make_shared<MockStorageEngine>();
    auto index_mgr = std::make_shared<MockIndexManager>();
    
    // Should construct successfully with both dependencies
    EXPECT_NO_THROW({
        auto engine = std::make_shared<QueryEngine>(storage, index_mgr);
        EXPECT_NE(engine, nullptr);
    });
}

/// @brief Test late binding via setStorage
TEST(QueryEngineDITest, LateBindingWithSetStorage) {
    auto index_mgr = std::make_shared<MockIndexManager>();
    
    // Construct with nullptr storage (late binding)
    auto engine = std::make_shared<QueryEngine>(nullptr, index_mgr);
    EXPECT_NE(engine, nullptr);
    
    // Later, inject storage
    auto storage = std::make_shared<MockStorageEngine>();
    EXPECT_NO_THROW(engine->setStorage(storage));
}

/// @brief Test constructor validation
TEST(QueryEngineDITest, ThrowsOnMissingIndexManager) {
    auto storage = std::make_shared<MockStorageEngine>();
    
    // Should throw when index_manager is nullptr
    EXPECT_THROW({
        auto engine = std::make_shared<QueryEngine>(storage, nullptr);
    }, std::invalid_argument);
}

/// @brief Test expression evaluator creation
TEST(QueryEngineDITest, ProvidesExpressionEvaluator) {
    auto storage = std::make_shared<MockStorageEngine>();
    auto index_mgr = std::make_shared<MockIndexManager>();
    auto engine = std::make_shared<QueryEngine>(storage, index_mgr);
    
    auto evaluator = engine->get_expression_evaluator();
    EXPECT_NE(evaluator, nullptr);
    
    // Test canEvaluate
    // Note: Phase 3 stub returns false - will return true in Phase 4 when implemented
    EXPECT_FALSE(evaluator->canEvaluate("doc.age > 18"));
    EXPECT_FALSE(evaluator->canEvaluate(""));
}

/// @brief Test builder pattern
TEST(QueryEngineDITest, BuilderPattern) {
    auto storage = std::make_shared<MockStorageEngine>();
    auto index_mgr = std::make_shared<MockIndexManager>();
    
    // Build with explicit dependencies
    auto engine = QueryEngineBuilder()
        .withStorage(storage)
        .withIndexManager(index_mgr)
        .build();
    
    EXPECT_NE(engine, nullptr);
}

/// @brief Test builder validation
TEST(QueryEngineDITest, BuilderThrowsOnMissingIndexManager) {
    auto storage = std::make_shared<MockStorageEngine>();
    
    // Should throw when trying to build without index manager
    EXPECT_THROW({
        auto engine = QueryEngineBuilder()
            .withStorage(storage)
            .build();  // Missing index manager
    }, std::runtime_error);
}

/// @brief Test builder allows nullptr storage (for late binding)
TEST(QueryEngineDITest, BuilderAllowsNullptrStorage) {
    auto index_mgr = std::make_shared<MockIndexManager>();
    
    // Should succeed with nullptr storage (late binding scenario)
    EXPECT_NO_THROW({
        auto engine = QueryEngineBuilder()
            .withStorage(nullptr)
            .withIndexManager(index_mgr)
            .build();
        
        EXPECT_NE(engine, nullptr);
    });
}

/// @brief Test standard builder factory
TEST(QueryEngineDITest, StandardBuilderFactory) {
    // Standard builder returns empty builder in Phase 3 (no default implementations yet)
    auto builder = QueryEngineBuilder::standard();
    
    // Must still provide dependencies explicitly
    EXPECT_THROW({
        builder.build();  // Should fail - no dependencies provided
    }, std::runtime_error);
    
    // Can be configured with explicit dependencies
    auto storage = std::make_shared<MockStorageEngine>();
    auto index_mgr = std::make_shared<MockIndexManager>();
    
    EXPECT_NO_THROW({
        auto engine = builder
            .withStorage(storage)
            .withIndexManager(index_mgr)
            .build();
        EXPECT_NE(engine, nullptr);
    });
}
