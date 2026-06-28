#include <gtest/gtest.h>

#include "index/index_manager.h"
#include "query/query_engine.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "storage/base_entity.h"
#include "storage/storage_engine.h"
#include "themis/edition.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

using namespace themis;
using namespace themis::query;

namespace {

[[nodiscard]] std::string makeTempDbPath() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_storage_query_index_explicit_di_" + std::to_string(static_cast<long long>(now))))
        .string();
}

} // namespace

class StorageQueryIndexExplicitDITest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = makeTempDbPath();
        fs::remove_all(db_path_);

        key_provider_ = std::make_shared<MockKeyProvider>();
        key_provider_->createKey("default", 1);

        encryption_ = std::make_shared<FieldEncryption>(key_provider_);
        index_manager_ = std::make_shared<IndexManager>();

        evaluator_engine_ = std::make_unique<QueryEngine>(nullptr, index_manager_);
        const auto evaluator = evaluator_engine_->get_expression_evaluator();

        storage_ = std::make_shared<StorageEngine>(
            evaluator,
            std::static_pointer_cast<IFieldEncryption>(encryption_),
            std::static_pointer_cast<IKeyProvider>(key_provider_),
            std::static_pointer_cast<IIndexManager>(index_manager_));

        ASSERT_TRUE(storage_->open(db_path_).has_value()) << "Failed to open explicit-DI StorageEngine";

        auto db_alias = std::shared_ptr<RocksDBWrapper>(storage_, storage_->rawDB());
        index_manager_->setStorage(storage_);
        index_manager_->setRocksDB(db_alias);
        evaluator_engine_->setStorage(storage_);

        auto secondary_manager = index_manager_->getSecondaryIndexManager();
        ASSERT_NE(secondary_manager, nullptr) << "SecondaryIndexManager was not initialized";

        query_engine_ = std::make_unique<QueryEngine>(*storage_->rawDB(), *secondary_manager);
    }

    void TearDown() override {
        query_engine_.reset();
        evaluator_engine_.reset();
        if (storage_) {
            storage_->close();
        }
        storage_.reset();
        encryption_.reset();
        key_provider_.reset();
        index_manager_.reset();
        fs::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<MockKeyProvider> key_provider_;
    std::shared_ptr<FieldEncryption> encryption_;
    std::shared_ptr<IndexManager> index_manager_;
    std::shared_ptr<StorageEngine> storage_;
    std::unique_ptr<QueryEngine> evaluator_engine_;
    std::unique_ptr<QueryEngine> query_engine_;
};

TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageRoundTripUsesRealRocksDB) {
    ASSERT_TRUE(storage_->put("doc:users:u1", R"({"status":"active","age":30})").has_value());

    const auto get_result = storage_->get("doc:users:u1");
    ASSERT_TRUE(get_result.has_value()) << get_result.error().message();
    EXPECT_NE(get_result->find("\"status\":\"active\""), std::string::npos);
}

TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageUsesQueryEvaluatorForFilters) {
    QueryEngine::EvaluationContext ctx;
    ctx.bind("doc", nlohmann::json{{"status", "active"}, {"age", 30}});

    EXPECT_TRUE(storage_->apply_filter("doc.status == \"active\"", &ctx));
    EXPECT_FALSE(storage_->apply_filter("doc.status == \"inactive\"", &ctx));
}

TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageIndexAndQueryComposeWithoutDefaultShims) {
    auto create_index_result = index_manager_->createSecondaryIndex("users", "age");
    ASSERT_TRUE(create_index_result.has_value()) << create_index_result.error().message();

    auto secondary_manager = index_manager_->getSecondaryIndexManager();
    ASSERT_NE(secondary_manager, nullptr);

    BaseEntity::FieldMap user1{{"name", std::string("Alice")}, {"age", int64_t(30)}, {"city", std::string("Berlin")}};
    BaseEntity::FieldMap user2{{"name", std::string("Bob")}, {"age", int64_t(31)}, {"city", std::string("Berlin")}};

    ASSERT_TRUE(secondary_manager->put("users", BaseEntity::fromFields("u1", user1)).ok);
    ASSERT_TRUE(secondary_manager->put("users", BaseEntity::fromFields("u2", user2)).ok);

    ConjunctiveQuery query{"users", {{"age", "30"}}};
    const auto result = query_engine_->executeAndKeys(query);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ((*result)[0], "u1");
}

TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageEncryptsAndDecryptsWithConfiguredProvider) {
    const std::vector<uint8_t> plaintext{0x10, 0x20, 0x30, 0x40};

    if (!themis::edition::IsFeatureEnabled("field_encryption")) {
        EXPECT_THROW(
            {
                (void)storage_->encrypt_field("sensitive_field", plaintext);
            },
            std::runtime_error);
        return;
    }

    const auto ciphertext = storage_->encrypt_field("sensitive_field", plaintext);
    EXPECT_NE(ciphertext, plaintext);

    const auto roundtrip = storage_->decrypt_field("sensitive_field", ciphertext);
    EXPECT_EQ(roundtrip, plaintext);
}