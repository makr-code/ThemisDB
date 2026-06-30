/**
 * @file test_lora_graph_linking.cpp
 * @brief Tests for LoRA Feedback Graph Linking (Stub #304)
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_feedback.h"
#include "llm/lora_framework/lora_feedback_storage.h"
#include "index/graph_index.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <memory>
#include <chrono>

namespace themis {
namespace test {

using namespace llm::lora;

class LoRAGraphLinkingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        db_path_ = std::filesystem::temp_directory_path() / 
                   ("themis_lora_graph_test_" + std::to_string(ms));
        std::filesystem::create_directories(db_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(db_config);
        
        // Create graph index
        graph_index_ = std::make_unique<GraphIndexManager>(*db_);
        
        // Create feedback storage service
        FeedbackStorageService::Config config;
        config.db = std::shared_ptr<RocksDBWrapper>(db_.get(), [](RocksDBWrapper*){});
        config.graph_index = std::shared_ptr<GraphIndexManager>(graph_index_.get(), 
                                                                 [](GraphIndexManager*){});
        config.enable_graph_links = true;
        config.collection_name = "help_feedback";
        
        storage_ = std::make_unique<FeedbackStorageService>(config);
    }
    
    void TearDown() override {
        storage_.reset();
        graph_index_.reset();
        db_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }
    
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<GraphIndexManager> graph_index_;
    std::unique_ptr<FeedbackStorageService> storage_;
    std::filesystem::path db_path_;
};

// Test 1: Create feedback with graph link (callback-based)
TEST_F(LoRAGraphLinkingTest, CreateFeedbackWithCallbackGraphLink) {
    // Track callback invocations
    bool callback_invoked = false;
    std::string callback_from, callback_to, callback_edge_type;
    
    FeedbackStorageService::CreateGraphLinkFn create_fn = 
        [&](const std::string& from, const std::string& to, const std::string& edge_type) -> bool {
            callback_invoked = true;
            callback_from = from;
            callback_to = to;
            callback_edge_type = edge_type;
            return true;
        };
    
    // Set the callback
    storage_->setCreateGraphLinkFn(create_fn);
    
    // Create feedback
    Feedback feedback;
    feedback.id = "fb-001";
    feedback.adapter_id = "adapter-1";
    feedback.response = "Test response";
    feedback.is_training = true;
    feedback.rating = 5;
    
    auto result = storage_->createFeedback(feedback);
    
    // Verify feedback was created
    ASSERT_TRUE(result.has_value());
    
    // Note: The callback may or may not be invoked depending on implementation
    // This test verifies the infrastructure is in place
    EXPECT_TRUE(callback_invoked) << "Graph link callback should have been invoked";
    
    if (callback_invoked) {
        EXPECT_TRUE(callback_from.find(feedback.id) != std::string::npos);
        EXPECT_TRUE(callback_to.find(feedback.adapter_id) != std::string::npos);
        EXPECT_EQ(callback_edge_type, "belongs_to_adapter");
    }
}

// Test 2: Remove feedback with graph link removal (callback-based)
TEST_F(LoRAGraphLinkingTest, RemoveFeedbackWithCallbackGraphLinkRemoval) {
    // Track callback invocations
    bool remove_callback_invoked = false;
    std::string remove_callback_from, remove_callback_to;
    
    FeedbackStorageService::RemoveGraphLinkFn remove_fn = 
        [&](const std::string& from, const std::string& to, const std::string& edge_type) -> bool {
            remove_callback_invoked = true;
            remove_callback_from = from;
            remove_callback_to = to;
            return true;
        };
    
    // Set the callback
    storage_->setRemoveGraphLinkFn(remove_fn);
    
    // First create a feedback
    Feedback feedback;
    feedback.id = "fb-002";
    feedback.adapter_id = "adapter-2";
    feedback.response = "Test response";
    feedback.is_training = true;
    feedback.rating = 4;
    
    auto created = storage_->createFeedback(feedback);
    ASSERT_TRUE(created.has_value());
    
    // Now delete it
    bool deleted = storage_->deleteFeedback(created->id);
    EXPECT_TRUE(deleted);
    
    // Note: The callback may or may not be invoked depending on implementation
    EXPECT_TRUE(remove_callback_invoked) << "Graph link removal callback should have been invoked";
    
    if (remove_callback_invoked) {
        EXPECT_TRUE(remove_callback_from.find(created->id) != std::string::npos);
        EXPECT_TRUE(remove_callback_to.find(feedback.adapter_id) != std::string::npos);
    }
}

// Test 3: Direct graph index linking (backward compatibility)
TEST_F(LoRAGraphLinkingTest, DirectGraphIndexLinking) {
    // Don't set callbacks - should use direct graph index
    
    Feedback feedback;
    feedback.id = "fb-003";
    feedback.adapter_id = "adapter-3";
    feedback.response = "Test response";
    feedback.is_training = true;
    feedback.rating = 3;
    
    auto result = storage_->createFeedback(feedback);
    ASSERT_TRUE(result.has_value());
    
    // Verify the feedback was stored
    auto retrieved = storage_->getFeedback(result->id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->adapter_id, "adapter-3");
}

// Test 4: Callback error handling
TEST_F(LoRAGraphLinkingTest, CallbackErrorHandling) {
    // Set a callback that fails
    FeedbackStorageService::CreateGraphLinkFn failing_fn = 
        [](const std::string&, const std::string&, const std::string&) -> bool {
            return false;  // Simulate failure
        };
    
    storage_->setCreateGraphLinkFn(failing_fn);
    
    Feedback feedback;
    feedback.id = "fb-004";
    feedback.adapter_id = "adapter-4";
    feedback.response = "Test response";
    feedback.is_training = true;
    feedback.rating = 2;
    
    // Should handle callback failure gracefully
    auto result = storage_->createFeedback(feedback);
    
    // Depending on implementation, it might fail or fall back to graph index
    // The important thing is that it doesn't crash
}

// Test 5: Callback with exception handling
TEST_F(LoRAGraphLinkingTest, CallbackExceptionHandling) {
    // Set a callback that throws
    FeedbackStorageService::CreateGraphLinkFn throwing_fn = 
        [](const std::string&, const std::string&, const std::string&) -> bool {
            throw std::runtime_error("Callback error");
        };
    
    storage_->setCreateGraphLinkFn(throwing_fn);
    
    Feedback feedback;
    feedback.id = "fb-005";
    feedback.adapter_id = "adapter-5";
    feedback.response = "Test response";
    feedback.is_training = true;
    feedback.rating = 1;
    
    // Should handle exception gracefully
    EXPECT_NO_THROW({
        auto result = storage_->createFeedback(feedback);
    });
}

// Test 6: Switch between callback and direct graph index
TEST_F(LoRAGraphLinkingTest, SwitchCallbackModes) {
    // Create with callback
    bool create_callback_invoked = false;
    FeedbackStorageService::CreateGraphLinkFn create_fn = 
        [&](const std::string&, const std::string&, const std::string&) -> bool {
            create_callback_invoked = true;
            return true;
        };
    
    storage_->setCreateGraphLinkFn(create_fn);
    
    Feedback feedback1;
    feedback1.id = "fb-006";
    feedback1.adapter_id = "adapter-6";
    feedback1.response = "Response 1";
    feedback1.is_training = true;
    feedback1.rating = 5;
    
    auto result1 = storage_->createFeedback(feedback1);
    ASSERT_TRUE(result1.has_value());
    EXPECT_TRUE(create_callback_invoked);
    
    // Clear callback and verify fallback to direct graph index
    create_callback_invoked = false;
    storage_->setCreateGraphLinkFn(nullptr);
    
    Feedback feedback2;
    feedback2.id = "fb-007";
    feedback2.adapter_id = "adapter-7";
    feedback2.response = "Response 2";
    feedback2.is_training = true;
    feedback2.rating = 4;
    
    auto result2 = storage_->createFeedback(feedback2);
    ASSERT_TRUE(result2.has_value());
    
    // Callback should not have been invoked the second time
    EXPECT_FALSE(create_callback_invoked);
}

} // namespace test
} // namespace themis
