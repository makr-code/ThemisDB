/**
 * @file test_llm_feedback.cpp
 * @brief Unit tests for LLM feedback metadata system
 */

#include <gtest/gtest.h>
#include "llm/llm_interaction_store.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <memory>

namespace themis {
namespace test {

class LLMFeedbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory with unique identifier
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        db_path_ = std::filesystem::temp_directory_path() / 
                   ("themis_llm_feedback_test_" + std::to_string(ms));
        std::filesystem::create_directories(db_path_);
        
        // Initialize RocksDB
        db_ = std::make_unique<RocksDBWrapper>(db_path_.string());
        
        // Create LLM interaction store
        llm_store_ = std::make_unique<LLMInteractionStore>(db_->db(), nullptr);
    }
    
    void TearDown() override {
        llm_store_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }
    
    std::filesystem::path db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<LLMInteractionStore> llm_store_;
};

// Test: Create interaction and add feedback metadata
TEST_F(LLMFeedbackTest, AddFeedbackToInteraction) {
    // Create an interaction
    LLMInteractionStore::Interaction interaction;
    interaction.prompt = "What is the capital of France?";
    interaction.response = "The capital of France is Paris.";
    interaction.model_version = "gpt-4o-mini";
    interaction.token_count = 25;
    interaction.latency_ms = 200;
    
    auto stored = llm_store_->createInteraction(interaction);
    ASSERT_FALSE(stored.id.empty());
    
    // Add feedback via metadata update
    nlohmann::json feedback;
    feedback["feedback"]["rating"] = 5;
    feedback["feedback"]["feedback_text"] = "Excellent response";
    feedback["feedback"]["user_id"] = "test_user";
    feedback["feedback"]["flagged_for_training"] = true;
    feedback["feedback"]["training_category"] = "positive";
    
    bool updated = llm_store_->updateMetadata(stored.id, feedback);
    ASSERT_TRUE(updated);
    
    // Retrieve and verify
    auto retrieved = llm_store_->getInteraction(stored.id);
    ASSERT_TRUE(retrieved.has_value());
    ASSERT_TRUE(retrieved->metadata.contains("feedback"));
    
    auto& fb = retrieved->metadata["feedback"];
    EXPECT_EQ(fb["rating"].get<int>(), 5);
    EXPECT_EQ(fb["feedback_text"].get<std::string>(), "Excellent response");
    EXPECT_EQ(fb["user_id"].get<std::string>(), "test_user");
    EXPECT_EQ(fb["flagged_for_training"].get<bool>(), true);
    EXPECT_EQ(fb["training_category"].get<std::string>(), "positive");
}

// Test: Update metadata for non-existent interaction
TEST_F(LLMFeedbackTest, UpdateMetadataNotFound) {
    nlohmann::json metadata;
    metadata["feedback"]["rating"] = 3;
    
    bool updated = llm_store_->updateMetadata("non-existent-id", metadata);
    EXPECT_FALSE(updated);
}

// Test: Multiple metadata updates
TEST_F(LLMFeedbackTest, MultipleMetadataUpdates) {
    // Create interaction
    LLMInteractionStore::Interaction interaction;
    interaction.prompt = "Test prompt";
    interaction.response = "Test response";
    
    auto stored = llm_store_->createInteraction(interaction);
    
    // First update: Add feedback
    nlohmann::json update1;
    update1["feedback"]["rating"] = 3;
    llm_store_->updateMetadata(stored.id, update1);
    
    // Second update: Add custom field
    nlohmann::json update2;
    update2["custom_field"] = "custom_value";
    llm_store_->updateMetadata(stored.id, update2);
    
    // Third update: Update feedback rating
    nlohmann::json update3;
    update3["feedback"]["rating"] = 5;
    update3["feedback"]["comment"] = "Updated to 5 stars";
    llm_store_->updateMetadata(stored.id, update3);
    
    // Verify all updates are present
    auto retrieved = llm_store_->getInteraction(stored.id);
    ASSERT_TRUE(retrieved.has_value());
    
    EXPECT_EQ(retrieved->metadata["feedback"]["rating"].get<int>(), 5);
    EXPECT_EQ(retrieved->metadata["feedback"]["comment"].get<std::string>(), "Updated to 5 stars");
    EXPECT_EQ(retrieved->metadata["custom_field"].get<std::string>(), "custom_value");
}

// Test: Metadata for LoRa training collection
TEST_F(LLMFeedbackTest, LoRaTrainingDataCollection) {
    // Create multiple interactions with various feedback
    constexpr int NUM_TEST_INTERACTIONS = 5;
    std::vector<std::string> interaction_ids;
    
    for (int i = 0; i < NUM_TEST_INTERACTIONS; i++) {
        LLMInteractionStore::Interaction interaction;
        interaction.prompt = "Test prompt " + std::to_string(i);
        interaction.response = "Test response " + std::to_string(i);
        
        auto stored = llm_store_->createInteraction(interaction);
        interaction_ids.push_back(stored.id);
        
        // Add feedback with training flag
        nlohmann::json feedback;
        feedback["feedback"]["rating"] = (i % 2 == 0) ? 5 : 2;
        feedback["feedback"]["flagged_for_training"] = (i < 3); // Flag first 3 for training
        feedback["feedback"]["training_category"] = (i % 2 == 0) ? "positive" : "negative";
        
        llm_store_->updateMetadata(stored.id, feedback);
    }
    
    // List all interactions and count training-flagged ones
    auto all_interactions = llm_store_->listInteractions();
    ASSERT_EQ(all_interactions.size(), NUM_TEST_INTERACTIONS);
    
    int training_count = 0;
    for (const auto& interaction : all_interactions) {
        if (interaction.metadata.contains("feedback") &&
            interaction.metadata["feedback"].contains("flagged_for_training") &&
            interaction.metadata["feedback"]["flagged_for_training"].get<bool>()) {
            training_count++;
        }
    }
    
    EXPECT_EQ(training_count, 3);
}

} // namespace test
} // namespace themis
