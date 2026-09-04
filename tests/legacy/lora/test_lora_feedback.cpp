/**
 * @file test_lora_feedback.cpp
 * @brief Unit tests for LoRA feedback system
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_feedback.h"
#include "llm/lora_framework/feedback_plugin.h"
#include "llm/lora_framework/lora_feedback_storage.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <memory>

namespace themis {
namespace test {

using namespace llm::lora;

class LoRAFeedbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        db_path_ = std::filesystem::temp_directory_path() / 
                   ("themis_lora_feedback_test_" + std::to_string(ms));
        std::filesystem::create_directories(db_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_ = std::make_unique<RocksDBWrapper>(db_config);
        
        // Create feedback storage service
        FeedbackStorageService::Config config;
        config.db = std::shared_ptr<RocksDBWrapper>(db_.get(), [](RocksDBWrapper*){});
        config.enable_graph_links = false; // Disable for unit tests
        
        storage_ = std::make_unique<FeedbackStorageService>(config);
    }
    
    void TearDown() override {
        storage_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }
    
    std::filesystem::path db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<FeedbackStorageService> storage_;
};

// ═══════════════════════════════════════════════════════════
// Basic Feedback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFeedbackTest, CreateFeedback) {
    Feedback feedback;
    feedback.adapter_id = "test_adapter";
    feedback.user_id = "user123";
    feedback.rating = 5;
    feedback.feedback_text = "Excellent response!";
    feedback.prompt = "What is ThemisDB?";
    feedback.response = "ThemisDB is a multi-model database...";
    
    auto created = storage_->createFeedback(feedback);
    ASSERT_TRUE(created.has_value());
    EXPECT_FALSE(created->id.empty());
    EXPECT_EQ(created->adapter_id, "test_adapter");
    EXPECT_EQ(created->rating, 5);
}

TEST_F(LoRAFeedbackTest, GetFeedback) {
    // Create feedback
    Feedback feedback;
    feedback.adapter_id = "test_adapter";
    feedback.user_id = "user123";
    feedback.rating = 4;
    
    auto created = storage_->createFeedback(feedback);
    ASSERT_TRUE(created.has_value());
    
    // Retrieve feedback
    auto retrieved = storage_->getFeedback(created->id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, created->id);
    EXPECT_EQ(retrieved->adapter_id, "test_adapter");
    EXPECT_EQ(retrieved->rating, 4);
}

TEST_F(LoRAFeedbackTest, UpdateFeedback) {
    // Create feedback
    Feedback feedback;
    feedback.adapter_id = "test_adapter";
    feedback.user_id = "user123";
    feedback.rating = 3;
    
    auto created = storage_->createFeedback(feedback);
    ASSERT_TRUE(created.has_value());
    
    // Update feedback
    Feedback updated = *created;
    updated.rating = 5;
    updated.feedback_text = "Updated feedback";
    
    bool success = storage_->updateFeedback(created->id, updated);
    ASSERT_TRUE(success);
    
    // Verify update
    auto retrieved = storage_->getFeedback(created->id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->rating, 5);
    EXPECT_EQ(retrieved->feedback_text, "Updated feedback");
}

TEST_F(LoRAFeedbackTest, DeleteFeedback) {
    // Create feedback
    Feedback feedback;
    feedback.adapter_id = "test_adapter";
    feedback.user_id = "user123";
    feedback.rating = 4;
    
    auto created = storage_->createFeedback(feedback);
    ASSERT_TRUE(created.has_value());
    
    // Delete feedback
    bool success = storage_->deleteFeedback(created->id);
    ASSERT_TRUE(success);
    
    // Verify deletion
    auto retrieved = storage_->getFeedback(created->id);
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(LoRAFeedbackTest, GraphLinkCreatedAndRemovedWithFeedbackLifecycle) {
    auto graph_index = std::make_shared<GraphIndexManager>(*db_);

    FeedbackStorageService::Config graph_cfg;
    graph_cfg.db = std::shared_ptr<RocksDBWrapper>(db_.get(), [](RocksDBWrapper*){});
    graph_cfg.graph_index = graph_index;
    graph_cfg.enable_graph_links = true;

    FeedbackStorageService graph_storage(graph_cfg);

    Feedback feedback;
    feedback.adapter_id = "adapter_graph";
    feedback.user_id = "graph_user";
    feedback.rating = 5;

    auto created = graph_storage.createFeedback(feedback);
    ASSERT_TRUE(created.has_value());

    const std::string from = "help_feedback:" + created->id;
    auto out = graph_index->outNeighbors(from);
    ASSERT_TRUE(out.first.ok) << out.first.message;
    ASSERT_EQ(out.second.size(), 1u);
    EXPECT_EQ(out.second.front(), "lora_adapters:adapter_graph");

    ASSERT_TRUE(graph_storage.deleteFeedback(created->id));

    out = graph_index->outNeighbors(from);
    ASSERT_TRUE(out.first.ok) << out.first.message;
    EXPECT_TRUE(out.second.empty());
}

TEST_F(LoRAFeedbackTest, ListFeedback) {
    // Create multiple feedback entries
    for (int i = 0; i < 5; i++) {
        Feedback feedback;
        feedback.adapter_id = "test_adapter";
        feedback.user_id = "user" + std::to_string(i);
        feedback.rating = i + 1;
        storage_->createFeedback(feedback);
    }
    
    // List all feedback
    auto feedback_list = storage_->listFeedback();
    EXPECT_GE(feedback_list.size(), 5);
}

TEST_F(LoRAFeedbackTest, FilterByAdapter) {
    // Create feedback for different adapters
    Feedback fb1;
    fb1.adapter_id = "adapter1";
    fb1.user_id = "user1";
    fb1.rating = 5;
    storage_->createFeedback(fb1);
    
    Feedback fb2;
    fb2.adapter_id = "adapter2";
    fb2.user_id = "user2";
    fb2.rating = 4;
    storage_->createFeedback(fb2);
    
    // Filter by adapter
    FeedbackFilter filter;
    filter.adapter_id = "adapter1";
    auto feedback_list = storage_->listFeedback(filter);
    
    EXPECT_GE(feedback_list.size(), 1);
    for (const auto& fb : feedback_list) {
        EXPECT_EQ(fb.adapter_id, "adapter1");
    }
}

TEST_F(LoRAFeedbackTest, FilterByRating) {
    // Create feedback with different ratings
    for (int i = 1; i <= 5; i++) {
        Feedback feedback;
        feedback.adapter_id = "test_adapter";
        feedback.user_id = "user" + std::to_string(i);
        feedback.rating = i;
        storage_->createFeedback(feedback);
    }
    
    // Filter by minimum rating
    FeedbackFilter filter;
    filter.adapter_id = "test_adapter";
    filter.min_rating = 4;
    auto feedback_list = storage_->listFeedback(filter);
    
    EXPECT_GE(feedback_list.size(), 2);
    for (const auto& fb : feedback_list) {
        EXPECT_GE(fb.rating, 4);
    }
}

// ═══════════════════════════════════════════════════════════
// Plugin Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFeedbackTest, BasePluginValidation) {
    BaseFeedbackPlugin plugin;
    
    // Valid feedback
    Feedback valid_fb;
    valid_fb.adapter_id = "test_adapter";
    valid_fb.user_id = "user123";
    valid_fb.rating = 5;
    EXPECT_TRUE(plugin.validate(valid_fb));
    
    // Invalid: missing adapter_id
    Feedback invalid_fb1;
    invalid_fb1.user_id = "user123";
    invalid_fb1.rating = 5;
    EXPECT_FALSE(plugin.validate(invalid_fb1));
    
    // Invalid: missing user_id
    Feedback invalid_fb2;
    invalid_fb2.adapter_id = "test_adapter";
    invalid_fb2.rating = 5;
    EXPECT_FALSE(plugin.validate(invalid_fb2));
    
    // Invalid: rating out of range
    Feedback invalid_fb3;
    invalid_fb3.adapter_id = "test_adapter";
    invalid_fb3.user_id = "user123";
    invalid_fb3.rating = 6;
    EXPECT_FALSE(plugin.validate(invalid_fb3));
}

TEST_F(LoRAFeedbackTest, PrivacyFilterPlugin) {
    PrivacyFilterPlugin plugin;
    
    // Feedback with PII
    Feedback feedback;
    feedback.adapter_id = "test_adapter";
    feedback.user_id = "user123";
    feedback.rating = 5;
    feedback.feedback_text = "Contact me at test@example.com or call 555-123-4567";
    
    // Process feedback
    plugin.process(feedback);
    
    // Verify PII is removed
    EXPECT_EQ(feedback.feedback_text.find("test@example.com"), std::string::npos);
    EXPECT_NE(feedback.feedback_text.find("[EMAIL]"), std::string::npos);
    EXPECT_NE(feedback.feedback_text.find("[PHONE]"), std::string::npos);
}

TEST_F(LoRAFeedbackTest, ContentValidationPlugin) {
    ContentValidationPlugin plugin;
    
    // Valid feedback
    Feedback valid_fb;
    valid_fb.adapter_id = "test_adapter";
    valid_fb.user_id = "user123";
    valid_fb.rating = 5;
    valid_fb.feedback_text = "This is good feedback";
    EXPECT_TRUE(plugin.validate(valid_fb));
    
    // Invalid: too short
    Feedback invalid_fb1;
    invalid_fb1.adapter_id = "test_adapter";
    invalid_fb1.user_id = "user123";
    invalid_fb1.rating = 5;
    invalid_fb1.feedback_text = "OK";
    EXPECT_FALSE(plugin.validate(invalid_fb1));
    
    // Invalid: spam (excessive repetition)
    Feedback invalid_fb2;
    invalid_fb2.adapter_id = "test_adapter";
    invalid_fb2.user_id = "user123";
    invalid_fb2.rating = 5;
    invalid_fb2.feedback_text = "aaaaaaaaaaaaaaaaaaaaaaa";
    EXPECT_FALSE(plugin.validate(invalid_fb2));
}

TEST_F(LoRAFeedbackTest, TrainingTriggerPlugin) {
    TrainingTriggerPlugin::Config config;
    config.min_batch_size = 10;
    config.max_batch_size = 20;
    TrainingTriggerPlugin plugin(config);
    
    // Small batch - should not trigger
    std::vector<Feedback> small_batch = {};

    for (int i = 0; i < 5; i++) {
        Feedback fb;
        fb.rating = 4;
        small_batch.push_back(fb);
    }
    EXPECT_FALSE(plugin.onTrainingTrigger(small_batch));
    
    // Medium batch - should trigger
    std::vector<Feedback> medium_batch = {};

    for (int i = 0; i < 15; i++) {
        Feedback fb;
        fb.rating = 4;
        medium_batch.push_back(fb);
    }
    EXPECT_TRUE(plugin.onTrainingTrigger(medium_batch));
    
    // Large batch - should definitely trigger
    std::vector<Feedback> large_batch = {};

    for (int i = 0; i < 25; i++) {
        Feedback fb;
        fb.rating = 4;
        large_batch.push_back(fb);
    }
    EXPECT_TRUE(plugin.onTrainingTrigger(large_batch));
}

TEST_F(LoRAFeedbackTest, PluginRegistration) {
    // Register plugins
    auto base_plugin = std::make_shared<BaseFeedbackPlugin>();
    auto privacy_plugin = std::make_shared<PrivacyFilterPlugin>();
    
    storage_->registerPlugin(base_plugin);
    storage_->registerPlugin(privacy_plugin);
    
    // Create feedback with PII
    Feedback feedback;
    feedback.adapter_id = "test_adapter";
    feedback.user_id = "user123";
    feedback.rating = 5;
    feedback.feedback_text = "Email me at test@example.com";
    
    auto created = storage_->createFeedback(feedback);
    ASSERT_TRUE(created.has_value());
    
    // Verify PII was filtered
    EXPECT_EQ(created->feedback_text.find("test@example.com"), std::string::npos);
    EXPECT_NE(created->feedback_text.find("[EMAIL]"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════
// Serialization Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFeedbackTest, FeedbackSerialization) {
    Feedback original;
    original.adapter_id = "test_adapter";
    original.user_id = "user123";
    original.rating = 5;
    original.feedback_text = "Great response";
    original.prompt = "Test prompt";
    original.response = "Test response";
    original.flagged_for_training = true;
    original.training_category = "positive";
    original.custom_metadata = {{"key", "value"}};
    
    // Serialize to JSON
    auto j = original.toJSON();
    
    // Deserialize from JSON
    auto deserialized = Feedback::fromJSON(j);
    
    // Verify
    EXPECT_EQ(deserialized.adapter_id, original.adapter_id);
    EXPECT_EQ(deserialized.user_id, original.user_id);
    EXPECT_EQ(deserialized.rating, original.rating);
    EXPECT_EQ(deserialized.feedback_text, original.feedback_text);
    EXPECT_EQ(deserialized.prompt, original.prompt);
    EXPECT_EQ(deserialized.response, original.response);
    EXPECT_EQ(deserialized.flagged_for_training, original.flagged_for_training);
    EXPECT_EQ(deserialized.training_category, original.training_category);
    EXPECT_EQ(deserialized.custom_metadata, original.custom_metadata);
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFeedbackTest, GetStatistics) {
    // Create feedback with various ratings
    for (int i = 1; i <= 5; i++) {
        Feedback feedback;
        feedback.adapter_id = "test_adapter";
        feedback.user_id = "user" + std::to_string(i);
        feedback.rating = i;
        feedback.flagged_for_training = (i >= 4);
        feedback.training_category = (i >= 4) ? "positive" : "negative";
        storage_->createFeedback(feedback);
    }
    
    // Get statistics
    auto stats = storage_->getStatistics("test_adapter");
    
    EXPECT_EQ(stats["total_count"], 5);
    EXPECT_EQ(stats["avg_rating"], 3.0);
    EXPECT_EQ(stats["flagged_for_training"], 2);
}

} // namespace test
} // namespace themis
