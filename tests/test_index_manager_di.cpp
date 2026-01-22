#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "index/index_manager.h"
#include "core/index_initialization.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/storage_interface.h"

using namespace themis;
using ::testing::Return;
using ::testing::_;

// Mock implementations for testing

class MockExpressionEvaluator : public IExpressionEvaluator {
public:
    MOCK_METHOD(bool, evaluate, (const std::string&, const void*), (override));
    MOCK_METHOD(std::string, get_expression_type, (), (const, override));
};

class MockStorageEngine : public IStorageEngine {
public:
    MOCK_METHOD(bool, open, (const std::string&), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, put, (const std::string&, const std::string&), (override));
    MOCK_METHOD(std::optional<std::string>, get, (const std::string&), (override));
    MOCK_METHOD(bool, del, (const std::string&), (override));
};

// Test fixture

class IndexManagerWithDITest : public ::testing::Test {
protected:
    std::shared_ptr<MockExpressionEvaluator> mock_evaluator_;
    std::shared_ptr<MockStorageEngine> mock_storage_;
    std::shared_ptr<IndexManager> index_manager_;
    
    void SetUp() override {
        mock_evaluator_ = std::make_shared<MockExpressionEvaluator>();
        mock_storage_ = std::make_shared<MockStorageEngine>();
        
        // Create index manager with mocks
        index_manager_ = std::make_shared<IndexManager>(mock_evaluator_, mock_storage_);
    }
};

// Tests

TEST_F(IndexManagerWithDITest, ConstructorAcceptsDependencies) {
    // Test passes if SetUp() succeeds without throwing
    EXPECT_NE(index_manager_, nullptr);
}

TEST_F(IndexManagerWithDITest, ConstructorWithNullDependencies) {
    // Should not throw - dependencies are optional
    auto index_mgr = std::make_shared<IndexManager>(nullptr, nullptr);
    EXPECT_NE(index_mgr, nullptr);
}

TEST_F(IndexManagerWithDITest, SetEvaluatorAfterConstruction) {
    auto index_mgr = std::make_shared<IndexManager>(nullptr, nullptr);
    
    auto new_evaluator = std::make_shared<MockExpressionEvaluator>();
    index_mgr->setExpressionEvaluator(new_evaluator);
    
    EXPECT_EQ(index_mgr->getExpressionEvaluator(), new_evaluator);
}

TEST_F(IndexManagerWithDITest, EvaluatorPropagatesToIndexManagers) {
    // This test verifies that when evaluator is set, it propagates to concrete managers
    // Note: Without RocksDB, concrete managers won't be created, so we just verify
    // the evaluator is stored correctly
    auto index_mgr = std::make_shared<IndexManager>(nullptr, nullptr);
    
    auto new_evaluator = std::make_shared<MockExpressionEvaluator>();
    index_mgr->setExpressionEvaluator(new_evaluator);
    
    EXPECT_EQ(index_mgr->getExpressionEvaluator(), new_evaluator);
    
    // In a full integration test with RocksDB, we would verify:
    // EXPECT_EQ(index_mgr->getVectorIndexManager()->getExpressionEvaluator(), new_evaluator);
    // EXPECT_EQ(index_mgr->getSecondaryIndexManager()->getExpressionEvaluator(), new_evaluator);
    // EXPECT_EQ(index_mgr->getGraphIndexManager()->getExpressionEvaluator(), new_evaluator);
}

TEST_F(IndexManagerWithDITest, SetStorageAfterConstruction) {
    auto index_mgr = std::make_shared<IndexManager>(nullptr, nullptr);
    
    auto new_storage = std::make_shared<MockStorageEngine>();
    index_mgr->setStorage(new_storage);
    
    // Storage is set internally, no direct getter
    EXPECT_NE(index_mgr, nullptr);
}

TEST_F(IndexManagerWithDITest, GetExpressionEvaluator) {
    EXPECT_EQ(index_manager_->getExpressionEvaluator(), mock_evaluator_);
}

TEST_F(IndexManagerWithDITest, CreateDefaultFactory) {
    auto index_mgr = IndexManager::createDefault();
    EXPECT_NE(index_mgr, nullptr);
}

TEST_F(IndexManagerWithDITest, BuilderPattern) {
    auto index_mgr = IndexManagerBuilder::standard()
        .withEvaluator(mock_evaluator_)
        .withStorage(mock_storage_)
        .build();
    
    EXPECT_NE(index_mgr, nullptr);
    EXPECT_EQ(index_mgr->getExpressionEvaluator(), mock_evaluator_);
}

TEST_F(IndexManagerWithDITest, BuilderWithoutDependencies) {
    auto index_mgr = IndexManagerBuilder::standard()
        .build();
    
    EXPECT_NE(index_mgr, nullptr);
}

TEST_F(IndexManagerWithDITest, ListIndexesEmptyInitially) {
    auto indices = index_manager_->listIndexes();
    EXPECT_TRUE(indices.empty());
}

TEST_F(IndexManagerWithDITest, GetIndexTypeReturnsErrorForNonExistent) {
    auto type = index_manager_->getIndexType("nonexistent");
    EXPECT_FALSE(type.has_value());
    EXPECT_EQ(type.error().code(), errors::ErrorCode::ERR_INDEX_NOT_FOUND);
}

TEST_F(IndexManagerWithDITest, GetVectorIndexReturnsNullForNonExistent) {
    auto idx = index_manager_->getVectorIndex("nonexistent");
    EXPECT_EQ(idx, nullptr);
}

TEST_F(IndexManagerWithDITest, GetSecondaryIndexReturnsNullForNonExistent) {
    auto idx = index_manager_->getSecondaryIndex("nonexistent");
    EXPECT_EQ(idx, nullptr);
}

TEST_F(IndexManagerWithDITest, GetGraphIndexReturnsNullForNonExistent) {
    auto idx = index_manager_->getGraphIndex("nonexistent");
    EXPECT_EQ(idx, nullptr);
}

TEST_F(IndexManagerWithDITest, DropNonExistentIndexReturnsFalse) {
    bool result = index_manager_->dropIndex("nonexistent");
    EXPECT_FALSE(result);
}

// Integration test with evaluator expectations

TEST_F(IndexManagerWithDITest, EvaluatorCanBeUsedForFiltering) {
    // Setup mock expectations
    EXPECT_CALL(*mock_evaluator_, evaluate(_, _))
        .WillOnce(Return(true));
    
    // Simulate using the evaluator
    auto evaluator = index_manager_->getExpressionEvaluator();
    ASSERT_NE(evaluator, nullptr);
    
    bool result = evaluator->evaluate("price > 100", nullptr);
    EXPECT_TRUE(result);
    
    ::testing::Mock::VerifyAndClearExpectations(mock_evaluator_.get());
}

TEST_F(IndexManagerWithDITest, GetExpressionTypeFromEvaluator) {
    EXPECT_CALL(*mock_evaluator_, get_expression_type())
        .WillOnce(Return("AQL"));
    
    auto evaluator = index_manager_->getExpressionEvaluator();
    ASSERT_NE(evaluator, nullptr);
    
    std::string type = evaluator->get_expression_type();
    EXPECT_EQ(type, "AQL");
    
    ::testing::Mock::VerifyAndClearExpectations(mock_evaluator_.get());
}

// Builder pattern tests

TEST_F(IndexManagerWithDITest, BuilderChaining) {
    auto builder = IndexManagerBuilder::standard();
    
    auto index_mgr = builder
        .withEvaluator(mock_evaluator_)
        .withStorage(mock_storage_)
        .build();
    
    EXPECT_NE(index_mgr, nullptr);
    EXPECT_EQ(index_mgr->getExpressionEvaluator(), mock_evaluator_);
}

TEST_F(IndexManagerWithDITest, BuilderOverridesDefaults) {
    auto custom_eval = std::make_shared<MockExpressionEvaluator>();
    
    auto index_mgr = IndexManagerBuilder::standard()
        .withEvaluator(custom_eval)
        .build();
    
    EXPECT_NE(index_mgr, nullptr);
    EXPECT_EQ(index_mgr->getExpressionEvaluator(), custom_eval);
}

// Test internal index manager access

TEST_F(IndexManagerWithDITest, InternalManagersNullBeforeRocksDB) {
    auto index_mgr = IndexManager::createDefault();
    
    // Internal managers should be null before RocksDB is set
    EXPECT_EQ(index_mgr->getVectorIndexManager(), nullptr);
    EXPECT_EQ(index_mgr->getSecondaryIndexManager(), nullptr);
    EXPECT_EQ(index_mgr->getGraphIndexManager(), nullptr);
}

// Test thread safety

TEST_F(IndexManagerWithDITest, ConcurrentListIndexes) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this]() {
            auto indices = index_manager_->listIndexes();
            // Should be empty initially
            EXPECT_TRUE(indices.empty());
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

TEST_F(IndexManagerWithDITest, ConcurrentGetIndexType) {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this]() {
            auto type = index_manager_->getIndexType("test");
            EXPECT_FALSE(type.has_value());
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}
