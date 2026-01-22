#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "storage/storage_engine.h"
#include "core/storage_initialization.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/security_interface.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"

using namespace themis;
using ::testing::Return;
using ::testing::_;

// Mock implementations for testing

class MockExpressionEvaluator : public IExpressionEvaluator {
public:
    MOCK_METHOD(bool, evaluate, (const std::string&, const void*), (override));
    MOCK_METHOD(std::string, get_expression_type, (), (const, override));
};

class MockFieldEncryption : public IFieldEncryption {
public:
    MOCK_METHOD(std::vector<uint8_t>, encrypt_field,
        (const std::string&, const std::vector<uint8_t>&), (override));
    MOCK_METHOD(std::vector<uint8_t>, decrypt_field,
        (const std::string&, const std::vector<uint8_t>&), (override));
    MOCK_METHOD(bool, should_encrypt, (const std::string&), (const, override));
};

class MockKeyProvider : public IKeyProvider {
public:
    MOCK_METHOD(std::optional<std::vector<uint8_t>>, getKey, (std::string_view, uint32_t), (const, override));
    MOCK_METHOD(uint32_t, getLatestKeyVersion, (std::string_view), (const, override));
    MOCK_METHOD(uint32_t, rotateKey, (std::string_view), (override));
    MOCK_METHOD(bool, hasKey, (std::string_view), (const, override));
    MOCK_METHOD(std::vector<std::string>, listKeys, (), (const, override));
};

class MockIndexManager : public IIndexManager {
public:
    MOCK_METHOD(ISecondaryIndex*, createSecondaryIndex, (std::string_view, std::string_view, const std::string&), (override));
    MOCK_METHOD(IVectorIndex*, createVectorIndex, (std::string_view, uint32_t, const std::string&), (override));
    MOCK_METHOD(IGraphIndex*, createGraphIndex, (std::string_view, const std::string&), (override));
    MOCK_METHOD(ISecondaryIndex*, getSecondaryIndex, (std::string_view), (const, override));
    MOCK_METHOD(IVectorIndex*, getVectorIndex, (std::string_view), (const, override));
    MOCK_METHOD(IGraphIndex*, getGraphIndex, (std::string_view), (const, override));
    MOCK_METHOD(bool, dropIndex, (std::string_view), (override));
    MOCK_METHOD(std::vector<std::string>, listIndexes, (), (const, override));
    MOCK_METHOD(std::optional<IndexType>, getIndexType, (std::string_view), (const, override));
};

// Test fixture

class StorageEngineWithDITest : public ::testing::Test {
protected:
    std::shared_ptr<MockExpressionEvaluator> mock_evaluator_;
    std::shared_ptr<MockFieldEncryption> mock_encryption_;
    std::shared_ptr<MockKeyProvider> mock_key_provider_;
    std::shared_ptr<MockIndexManager> mock_index_manager_;
    std::shared_ptr<StorageEngine> storage_;
    
    void SetUp() override {
        mock_evaluator_ = std::make_shared<MockExpressionEvaluator>();
        mock_encryption_ = std::make_shared<MockFieldEncryption>();
        mock_key_provider_ = std::make_shared<MockKeyProvider>();
        mock_index_manager_ = std::make_shared<MockIndexManager>();
        
        storage_ = std::make_shared<StorageEngine>(
            mock_evaluator_,
            mock_encryption_,
            mock_key_provider_,
            mock_index_manager_
        );
    }
};

// Tests

TEST_F(StorageEngineWithDITest, ConstructorAcceptsDependencies) {
    // Test passes if setUp() succeeds without throwing
    EXPECT_NE(storage_, nullptr);
}

TEST_F(StorageEngineWithDITest, ThrowsOnNullEvaluator) {
    EXPECT_THROW(
        StorageEngine(nullptr, mock_encryption_, mock_key_provider_, nullptr),
        std::invalid_argument
    );
}

TEST_F(StorageEngineWithDITest, ThrowsOnNullEncryption) {
    EXPECT_THROW(
        StorageEngine(mock_evaluator_, nullptr, mock_key_provider_, nullptr),
        std::invalid_argument
    );
}

TEST_F(StorageEngineWithDITest, ThrowsOnNullKeyProvider) {
    EXPECT_THROW(
        StorageEngine(mock_evaluator_, mock_encryption_, nullptr, nullptr),
        std::invalid_argument
    );
}

TEST_F(StorageEngineWithDITest, IndexManagerIsOptional) {
    // Should not throw when index_manager is null
    EXPECT_NO_THROW(
        StorageEngine(mock_evaluator_, mock_encryption_, mock_key_provider_, nullptr)
    );
}

TEST_F(StorageEngineWithDITest, UsesInjectedEvaluator) {
    EXPECT_CALL(*mock_evaluator_, evaluate(_, _))
        .WillOnce(Return(true));
    
    bool result = storage_->apply_filter("test_filter", nullptr);
    
    EXPECT_TRUE(result);
    ::testing::Mock::VerifyAndClearExpectations(mock_evaluator_.get());
}

TEST_F(StorageEngineWithDITest, UsesInjectedEncryption) {
    std::vector<uint8_t> plaintext = {1, 2, 3, 4};
    std::vector<uint8_t> encrypted = {5, 6, 7, 8};
    
    EXPECT_CALL(*mock_encryption_, encrypt_field("field1", plaintext))
        .WillOnce(Return(encrypted));
    
    auto result = storage_->encrypt_field("field1", plaintext);
    
    EXPECT_EQ(result, encrypted);
    ::testing::Mock::VerifyAndClearExpectations(mock_encryption_.get());
}

TEST_F(StorageEngineWithDITest, UsesInjectedDecryption) {
    std::vector<uint8_t> ciphertext = {5, 6, 7, 8};
    std::vector<uint8_t> decrypted = {1, 2, 3, 4};
    
    EXPECT_CALL(*mock_encryption_, decrypt_field("field1", ciphertext))
        .WillOnce(Return(decrypted));
    
    auto result = storage_->decrypt_field("field1", ciphertext);
    
    EXPECT_EQ(result, decrypted);
    ::testing::Mock::VerifyAndClearExpectations(mock_encryption_.get());
}

TEST_F(StorageEngineWithDITest, CreateDefaultFactory) {
    auto engine = StorageEngine::createDefault();
    
    EXPECT_NE(engine, nullptr);
}

TEST_F(StorageEngineWithDITest, DefaultFactoryEngineWorks) {
    auto engine = StorageEngine::createDefault();
    
    // Should be able to open and use default engine
    EXPECT_TRUE(engine->open("/tmp/test_db"));
    EXPECT_TRUE(engine->put("key1", "value1"));
    engine->close();
}

// Builder pattern tests

TEST(StorageEngineBuilderTest, BuilderRequiresEvaluator) {
    StorageEngineBuilder builder;
    auto encryption = std::make_shared<MockFieldEncryption>();
    auto key_provider = std::make_shared<MockKeyProvider>();
    
    builder.withEncryption(encryption)
           .withKeyProvider(key_provider);
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST(StorageEngineBuilderTest, BuilderRequiresEncryption) {
    StorageEngineBuilder builder;
    auto evaluator = std::make_shared<MockExpressionEvaluator>();
    auto key_provider = std::make_shared<MockKeyProvider>();
    
    builder.withEvaluator(evaluator)
           .withKeyProvider(key_provider);
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST(StorageEngineBuilderTest, BuilderRequiresKeyProvider) {
    StorageEngineBuilder builder;
    auto evaluator = std::make_shared<MockExpressionEvaluator>();
    auto encryption = std::make_shared<MockFieldEncryption>();
    
    builder.withEvaluator(evaluator)
           .withEncryption(encryption);
    
    EXPECT_THROW(builder.build(), std::runtime_error);
}

TEST(StorageEngineBuilderTest, BuilderSucceedsWithAllRequired) {
    StorageEngineBuilder builder;
    auto evaluator = std::make_shared<MockExpressionEvaluator>();
    auto encryption = std::make_shared<MockFieldEncryption>();
    auto key_provider = std::make_shared<MockKeyProvider>();
    
    auto engine = builder.withEvaluator(evaluator)
                         .withEncryption(encryption)
                         .withKeyProvider(key_provider)
                         .build();
    
    EXPECT_NE(engine, nullptr);
}

TEST(StorageEngineBuilderTest, BuilderAcceptsOptionalIndexManager) {
    StorageEngineBuilder builder;
    auto evaluator = std::make_shared<MockExpressionEvaluator>();
    auto encryption = std::make_shared<MockFieldEncryption>();
    auto key_provider = std::make_shared<MockKeyProvider>();
    auto index_manager = std::make_shared<MockIndexManager>();
    
    auto engine = builder.withEvaluator(evaluator)
                         .withEncryption(encryption)
                         .withKeyProvider(key_provider)
                         .withIndexManager(index_manager)
                         .build();
    
    EXPECT_NE(engine, nullptr);
}

TEST(StorageEngineBuilderTest, StandardFactoryCreatesBuilder) {
    auto builder = StorageEngineBuilder::standard();
    
    // Builder should be populated with defaults and ready to build
    auto engine = builder.build();
    EXPECT_NE(engine, nullptr);
}
