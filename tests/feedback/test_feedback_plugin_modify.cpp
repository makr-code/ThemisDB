/**
 * @file test_feedback_plugin_modify.cpp
 * @brief Tests for Feedback Plugin MODIFY Action (Stub #297 remediation)
 * 
 * This test suite verifies that the FeedbackStore correctly applies
 * modifications from plugins when FeedbackValidationResult::MODIFY is returned.
 */

#include <gtest/gtest.h>
#include "llm/feedback_store.h"
#include "llm/i_feedback_plugin.h"
#include "storage/rocksdb_wrapper.h"
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>
#include <filesystem>
#include <memory>
#include <chrono>

namespace themis {
namespace llm {
namespace test {

// Mock plugin that modifies feedback (e.g., PII redaction)
class ModifyingMockPlugin : public IFeedbackPlugin {
public:
    std::string getName() const override { return "modifying_mock"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "Mock plugin that modifies feedback"; }
    
    bool initialize(const nlohmann::json& /*config*/) override { return true; }
    void shutdown() override {}
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        ValidationResponse response;
        response.result = FeedbackValidationResult::MODIFY;
        
        // Simulate PII redaction
        std::string redacted_comment = feedback.comment;
        // Replace common PII patterns
        if (redacted_comment.find("email@") != std::string::npos) {
            redacted_comment = "This comment contained PII and was redacted.";
        }
        
        // Simulate adding plugin metadata
        nlohmann::json modified_metadata = feedback.metadata;
        modified_metadata["pii_check"] = true;
        modified_metadata["plugin_name"] = "modifying_mock";
        
        response.modified_comment = redacted_comment;
        response.modified_metadata = modified_metadata;
        response.confidence_score = 0.95f;
        
        return response;
    }
};

// Plugin that accepts but modifies specific fields
class CommentModifyingPlugin : public IFeedbackPlugin {
public:
    std::string getName() const override { return "comment_modifier"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "Plugin that modifies comments"; }
    
    bool initialize(const nlohmann::json& /*config*/) override { return true; }
    void shutdown() override {}
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        ValidationResponse response;
        response.result = FeedbackValidationResult::MODIFY;
        
        // Normalize comment to uppercase
        std::string normalized_comment = feedback.comment;
        for (auto& c : normalized_comment) {
            c = std::toupper(c);
        }
        response.modified_comment = normalized_comment;
        response.confidence_score = 0.9f;
        
        return response;
    }
};

// Plugin that only modifies metadata
class MetadataModifyingPlugin : public IFeedbackPlugin {
public:
    std::string getName() const override { return "metadata_modifier"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "Plugin that modifies metadata"; }
    
    bool initialize(const nlohmann::json& /*config*/) override { return true; }
    void shutdown() override {}
    
    ValidationResponse validate(const FeedbackData& feedback) override {
        ValidationResponse response;
        response.result = FeedbackValidationResult::MODIFY;
        
        nlohmann::json modified_metadata = feedback.metadata;
        modified_metadata["quality_score"] = 8.5;
        modified_metadata["categorized_as"] = "technical_feedback";
        modified_metadata["plugin_version"] = "1.0.0";
        
        response.modified_metadata = modified_metadata;
        response.confidence_score = 0.88f;
        
        return response;
    }
};

class FeedbackPluginModifyTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        db_path_ = std::filesystem::temp_directory_path() / 
                   ("themis_feedback_modify_test_" + std::to_string(ms));
        std::filesystem::create_directories(db_path_);
        
        // Create RocksDB with unique path
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions txn_opts;
        rocksdb::Status s = rocksdb::TransactionDB::Open(options, txn_opts, db_path_.string(), &db_);
        if (!s.ok() || db_ == nullptr) {
            throw std::runtime_error("Failed to create RocksDB instance: " + s.ToString());
        }
        
        feedback_store_ = std::make_unique<FeedbackStore>(db_, nullptr);
    }
    
    void TearDown() override {
        feedback_store_.reset();
        if (db_) {
            delete db_;
            db_ = nullptr;
        }
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }
    
    std::filesystem::path db_path_;
    rocksdb::TransactionDB* db_ = nullptr;
    std::unique_ptr<FeedbackStore> feedback_store_;
};

// Test 1: Verify MODIFY action applies modified comment
TEST_F(FeedbackPluginModifyTest, ModifyActionAppliesToComment) {
    auto plugin = std::make_shared<CommentModifyingPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user123";
    feedback.question = "How do I optimize performance?";
    feedback.answer = "Use proper indexing.";
    feedback.comment = "This was helpful";
    feedback.model_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Verify comment was modified (uppercase)
    EXPECT_EQ(stored.comment, "THIS WAS HELPFUL");
    EXPECT_EQ(stored.validation_status, ValidationStatus::APPROVED);
}

// Test 2: Verify MODIFY action applies modified metadata
TEST_F(FeedbackPluginModifyTest, ModifyActionAppliesToMetadata) {
    auto plugin = std::make_shared<MetadataModifyingPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::NEGATIVE;
    feedback.user_id = "user456";
    feedback.question = "Question?";
    feedback.answer = "Answer.";
    feedback.metadata = nlohmann::json::object();
    feedback.model_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Verify metadata was modified
    EXPECT_TRUE(stored.metadata.contains("quality_score"));
    EXPECT_EQ(stored.metadata["quality_score"], 8.5);
    EXPECT_EQ(stored.metadata["categorized_as"], "technical_feedback");
    EXPECT_EQ(stored.metadata["plugin_version"], "1.0.0");
    EXPECT_EQ(stored.validation_status, ValidationStatus::APPROVED);
}

// Test 3: Verify both comment and metadata modifications are applied
TEST_F(FeedbackPluginModifyTest, ModifyActionAppliesToBothCommentAndMetadata) {
    auto plugin = std::make_shared<ModifyingMockPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user789";
    feedback.question = "How to configure?";
    feedback.answer = "Set the config value.";
    feedback.comment = "Found email@example.com in the docs";
    feedback.metadata = nlohmann::json::object();
    feedback.model_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Verify both modifications were applied
    EXPECT_EQ(stored.comment, "This comment contained PII and was redacted.");
    EXPECT_TRUE(stored.metadata.contains("pii_check"));
    EXPECT_EQ(stored.metadata["pii_check"], true);
    EXPECT_EQ(stored.metadata["plugin_name"], "modifying_mock");
    EXPECT_EQ(stored.validation_status, ValidationStatus::APPROVED);
}

// Test 4: Verify modifications are persisted in storage
TEST_F(FeedbackPluginModifyTest, ModificationsArePersisted) {
    auto plugin = std::make_shared<CommentModifyingPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user_persist";
    feedback.question = "Q?";
    feedback.answer = "A.";
    feedback.comment = "original comment";
    feedback.model_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    std::string stored_id = stored.id;
    
    // Verify stored comment is modified
    EXPECT_EQ(stored.comment, "ORIGINAL COMMENT");
    
    // Retrieve from storage and verify modification is persisted
    auto retrieved = feedback_store_->getFeedback(stored_id);
    ASSERT_TRUE(retrieved);
    EXPECT_EQ(retrieved->comment, "ORIGINAL COMMENT");
    EXPECT_NE(retrieved->comment, "original comment");
}

// Test 5: Verify count includes modified feedback
TEST_F(FeedbackPluginModifyTest, ModifiedFeedbackCountedInStats) {
    auto plugin = std::make_shared<MetadataModifyingPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    // Create several feedback entries
    for (int i = 0; i < 3; ++i) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = (i % 2 == 0) ? FeedbackType::POSITIVE : FeedbackType::NEGATIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Q" + std::to_string(i);
        feedback.answer = "A" + std::to_string(i);
        feedback.model_version = "v1.0";
        
        feedback_store_->createFeedback(feedback);
    }
    
    auto stats = feedback_store_->getStats();
    EXPECT_EQ(stats.total_feedback, 3);
    EXPECT_EQ(stats.approved_count, 3);  // All should be approved after modification
    EXPECT_EQ(stats.positive_count, 2);
    EXPECT_EQ(stats.negative_count, 1);
}

// Test 6: Verify optional modifications (only comment without metadata)
TEST_F(FeedbackPluginModifyTest, PartialModification_OnlyComment) {
    class PartialCommentPlugin : public IFeedbackPlugin {
    public:
        std::string getName() const override { return "partial"; }
        std::string getVersion() const override { return "1.0.0"; }
        std::string getDescription() const override { return "Partial modifier"; }
        bool initialize(const nlohmann::json& /*config*/) override { return true; }
        void shutdown() override {}
        
        ValidationResponse validate(const FeedbackData& feedback) override {
            ValidationResponse response;
            response.result = FeedbackValidationResult::MODIFY;
            response.modified_comment = "Modified comment only";
            // No modified_metadata
            return response;
        }
    };
    
    auto plugin = std::make_shared<PartialCommentPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user_partial";
    feedback.question = "Q?";
    feedback.answer = "A.";
    feedback.comment = "Original";
    feedback.metadata["key"] = "original_value";
    feedback.model_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Comment should be modified
    EXPECT_EQ(stored.comment, "Modified comment only");
    // Metadata should remain unchanged
    EXPECT_EQ(stored.metadata["key"], "original_value");
}

// Test 7: Verify optional modifications (only metadata without comment)
TEST_F(FeedbackPluginModifyTest, PartialModification_OnlyMetadata) {
    class PartialMetadataPlugin : public IFeedbackPlugin {
    public:
        std::string getName() const override { return "partial"; }
        std::string getVersion() const override { return "1.0.0"; }
        std::string getDescription() const override { return "Partial modifier"; }
        bool initialize(const nlohmann::json& /*config*/) override { return true; }
        void shutdown() override {}
        
        ValidationResponse validate(const FeedbackData& feedback) override {
            ValidationResponse response;
            response.result = FeedbackValidationResult::MODIFY;
            nlohmann::json meta = feedback.metadata;
            meta["modified"] = true;
            response.modified_metadata = meta;
            // No modified_comment
            return response;
        }
    };
    
    auto plugin = std::make_shared<PartialMetadataPlugin>();
    feedback_store_->setValidationPlugin(plugin);
    
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::NEGATIVE;
    feedback.user_id = "user_partial2";
    feedback.question = "Q?";
    feedback.answer = "A.";
    feedback.comment = "Original comment";
    feedback.metadata = nlohmann::json::object();
    feedback.model_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Comment should remain unchanged
    EXPECT_EQ(stored.comment, "Original comment");
    // Metadata should be modified
    EXPECT_TRUE(stored.metadata.contains("modified"));
    EXPECT_EQ(stored.metadata["modified"], true);
}

} // namespace test
} // namespace llm
} // namespace themis
