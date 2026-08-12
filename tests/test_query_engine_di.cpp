/**
 * @file test_query_engine_di.cpp
 * @brief Unit tests for QueryEngineBuilder (Dependency Injection)
 *
 * Tests the Builder pattern for constructing QueryEngine instances
 * using the IStorageEngine / IIndexManager interfaces.
 */

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/query_engine_builder.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include "utils/expected.h"

using namespace themis;
using namespace themis::errors;

// ============================================================================
// Minimal mock implementations of the DI interfaces
// ============================================================================

class MockStorageEngine : public IStorageEngine {
public:
    Result<void> open(const std::string&) override { return Ok(); }
    void close() override {}
    Result<void> put(const std::string&, const std::string&) override { return Ok(); }
    Result<std::string> get(const std::string& key) override {
        return Err<std::string>(ErrorCode::ERR_NOT_FOUND);
    }
    Result<void> del(const std::string&) override { return Ok(); }
};

class MockIndexManager : public IIndexManager {
public:
    Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view, std::string_view, const std::string&) override {
        return Err<ISecondaryIndex*>(ErrorCode::ERR_INDEX_CREATION_FAILED);
    }
    Result<IVectorIndex*> createVectorIndex(
        std::string_view, uint32_t, const std::string&) override {
        return Err<IVectorIndex*>(ErrorCode::ERR_INDEX_CREATION_FAILED);
    }
    Result<IGraphIndex*> createGraphIndex(
        std::string_view, const std::string&) override {
        return Err<IGraphIndex*>(ErrorCode::ERR_INDEX_CREATION_FAILED);
    }
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view) const override {
        return Err<ISecondaryIndex*>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
    Result<IVectorIndex*> getVectorIndex(std::string_view) const override {
        return Err<IVectorIndex*>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
    Result<IGraphIndex*> getGraphIndex(std::string_view) const override {
        return Err<IGraphIndex*>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
    Result<void> dropIndex(std::string_view) override { return Ok(); }
    std::vector<std::string> listIndexes() const override { return {}; }
    Result<IndexType> getIndexType(std::string_view) const override {
        return Err<IndexType>(ErrorCode::ERR_INDEX_NOT_FOUND);
    }
};

// ============================================================================
// QueryEngineBuilder tests
// ============================================================================

class QueryEngineBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        storage_   = std::make_shared<MockStorageEngine>();
        index_mgr_ = std::make_shared<MockIndexManager>();
    }
    std::shared_ptr<MockStorageEngine> storage_;
    std::shared_ptr<MockIndexManager>  index_mgr_;
};

// build() with both storage and index manager → succeeds
TEST_F(QueryEngineBuilderTest, BuildWithAllDeps) {
    auto engine = QueryEngineBuilder()
        .withStorage(storage_)
        .withIndexManager(index_mgr_)
        .build();
    EXPECT_NE(engine, nullptr);
}

// build() with nullptr storage (late binding) + index manager → succeeds
TEST_F(QueryEngineBuilderTest, BuildWithNullStorage) {
    auto engine = QueryEngineBuilder()
        .withStorage(nullptr)
        .withIndexManager(index_mgr_)
        .build();
    EXPECT_NE(engine, nullptr);
}

// build() without index manager → throws std::runtime_error
TEST_F(QueryEngineBuilderTest, BuildWithoutIndexManagerThrows) {
    EXPECT_THROW(
        QueryEngineBuilder().withStorage(storage_).build(),
        std::runtime_error
    );
}

// build() with no deps at all → throws std::runtime_error
TEST_F(QueryEngineBuilderTest, BuildWithNoDepsThrows) {
    EXPECT_THROW(QueryEngineBuilder().build(), std::runtime_error);
}

// standard() factory returns an empty builder that still requires deps
TEST_F(QueryEngineBuilderTest, StandardFactoryRequiresDeps) {
    EXPECT_THROW(QueryEngineBuilder::standard().build(), std::runtime_error);
}

// standard() factory + withIndexManager → succeeds
TEST_F(QueryEngineBuilderTest, StandardFactoryWithIndexManager) {
    auto engine = QueryEngineBuilder::standard()
        .withIndexManager(index_mgr_)
        .build();
    EXPECT_NE(engine, nullptr);
}

// Builder supports method chaining (fluent API)
TEST_F(QueryEngineBuilderTest, FluentChaining) {
    QueryEngineBuilder builder;
    auto& ref1 = builder.withStorage(storage_);
    auto& ref2 = ref1.withIndexManager(index_mgr_);
    // Returns reference to self → same object
    EXPECT_EQ(&ref1, &ref2);
}

// setStorage() on built engine allows late binding
TEST_F(QueryEngineBuilderTest, LateBindingViaSetStorage) {
    auto engine = QueryEngineBuilder()
        .withStorage(nullptr)
        .withIndexManager(index_mgr_)
        .build();
    ASSERT_NE(engine, nullptr);
    // setStorage() should not throw
    EXPECT_NO_THROW(engine->setStorage(storage_));
}
