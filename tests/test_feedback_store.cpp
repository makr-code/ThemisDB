/**
 * @file test_feedback_store.cpp
 * @brief Unit tests for FeedbackStore - LoRA continuous learning feedback system
 */

#include <gtest/gtest.h>
#include "llm/feedback_store.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <memory>
#include <thread>
#include <chrono>

namespace themis {
namespace llm {
namespace test {

class FeedbackStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory with unique identifier
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        db_path_ = std::filesystem::temp_directory_path() / 
                   ("themis_feedback_store_test_" + std::to_string(ms));
        std::filesystem::create_directories(db_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config config;
        config.db_path = db_path_.string();
        db_ = std::make_unique<RocksDBWrapper>(config);
        
        // Create FeedbackStore
        feedback_store_ = std::make_unique<FeedbackStore>(db_->db(), nullptr);
    }
    
    void TearDown() override {
        feedback_store_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }
    
    std::filesystem::path db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<FeedbackStore> feedback_store_;
};

// Test: Create positive feedback
TEST_F(FeedbackStoreTest, CreatePositiveFeedback) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user123";
    feedback.question = "How do I enable sharding in ThemisDB?";
    feedback.answer = "To enable sharding, use the SHARD BY clause in your CREATE COLLECTION statement.";
    feedback.model_version = "llama-2-7b";
    feedback.adapter_id = "themis_help_lora";
    feedback.adapter_version = "v1.0";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    ASSERT_FALSE(stored.id.empty());
    EXPECT_EQ(stored.type, FeedbackType::POSITIVE);
    EXPECT_EQ(stored.user_id, "user123");
    EXPECT_EQ(stored.question, feedback.question);
    EXPECT_EQ(stored.answer, feedback.answer);
    EXPECT_GT(stored.timestamp_ms, 0);
    EXPECT_EQ(stored.validation_status, ValidationStatus::APPROVED);
    EXPECT_FALSE(stored.used_for_training);
    EXPECT_EQ(stored.training_batch_id, 0);
}

// Test: Create negative feedback with correction
TEST_F(FeedbackStoreTest, CreateNegativeFeedbackWithCorrection) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::NEGATIVE;
    feedback.user_id = "user456";
    feedback.question = "What is the default replication factor?";
    feedback.answer = "The default replication factor is 1.";
    feedback.correction = "The default replication factor is 3 for production environments.";
    feedback.model_version = "llama-2-7b";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    ASSERT_FALSE(stored.id.empty());
    EXPECT_EQ(stored.type, FeedbackType::NEGATIVE);
    EXPECT_EQ(stored.correction, feedback.correction);
    EXPECT_EQ(stored.validation_status, ValidationStatus::APPROVED);
}

// Test: Retrieve feedback by ID
TEST_F(FeedbackStoreTest, GetFeedback) {
    // Create feedback
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user789";
    feedback.question = "Test question";
    feedback.answer = "Test answer";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Retrieve by ID
    auto retrieved = feedback_store_->getFeedback(stored.id);
    
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->id, stored.id);
    EXPECT_EQ(retrieved->user_id, "user789");
    EXPECT_EQ(retrieved->question, "Test question");
    EXPECT_EQ(retrieved->answer, "Test answer");
}

// Test: Get non-existent feedback
TEST_F(FeedbackStoreTest, GetNonExistentFeedback) {
    auto retrieved = feedback_store_->getFeedback("non-existent-id");
    EXPECT_FALSE(retrieved.has_value());
}

// Test: List all feedback
TEST_F(FeedbackStoreTest, ListFeedback) {
    // Create multiple feedback entries
    for (int i = 0; i < 5; i++) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = (i % 2 == 0) ? FeedbackType::POSITIVE : FeedbackType::NEGATIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Question " + std::to_string(i);
        feedback.answer = "Answer " + std::to_string(i);
        if (feedback.type == FeedbackType::NEGATIVE) {
            feedback.correction = "Correction " + std::to_string(i);
        }
        
        feedback_store_->createFeedback(feedback);
    }
    
    // List all feedback
    auto all_feedback = feedback_store_->listFeedback();
    
    EXPECT_EQ(all_feedback.size(), 5);
}

// Test: List feedback with filters
TEST_F(FeedbackStoreTest, ListFeedbackWithFilters) {
    // Create feedback entries with different types
    for (int i = 0; i < 10; i++) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = (i < 5) ? FeedbackType::POSITIVE : FeedbackType::NEGATIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Question " + std::to_string(i);
        feedback.answer = "Answer " + std::to_string(i);
        if (feedback.type == FeedbackType::NEGATIVE) {
            feedback.correction = "Correction " + std::to_string(i);
        }
        
        feedback_store_->createFeedback(feedback);
    }
    
    // Filter by type - positive only
    FeedbackStore::ListOptions options;
    options.filter_type = FeedbackType::POSITIVE;
    
    auto positive_feedback = feedback_store_->listFeedback(options);
    
    EXPECT_EQ(positive_feedback.size(), 5);
    for (const auto& fb : positive_feedback) {
        EXPECT_EQ(fb.type, FeedbackType::POSITIVE);
    }
    
    // Filter by type - negative only
    options.filter_type = FeedbackType::NEGATIVE;
    auto negative_feedback = feedback_store_->listFeedback(options);
    
    EXPECT_EQ(negative_feedback.size(), 5);
    for (const auto& fb : negative_feedback) {
        EXPECT_EQ(fb.type, FeedbackType::NEGATIVE);
    }
}

// Test: Update validation status
TEST_F(FeedbackStoreTest, UpdateValidationStatus) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user001";
    feedback.question = "Test question";
    feedback.answer = "Test answer";
    
    auto stored = feedback_store_->createFeedback(feedback);
    EXPECT_EQ(stored.validation_status, ValidationStatus::APPROVED);
    
    // Update status to REJECTED
    bool updated = feedback_store_->updateValidationStatus(stored.id, ValidationStatus::REJECTED);
    ASSERT_TRUE(updated);
    
    // Verify update
    auto retrieved = feedback_store_->getFeedback(stored.id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->validation_status, ValidationStatus::REJECTED);
}

// Test: Mark feedback as used for training
TEST_F(FeedbackStoreTest, MarkUsedForTraining) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user002";
    feedback.question = "Training question";
    feedback.answer = "Training answer";
    
    auto stored = feedback_store_->createFeedback(feedback);
    EXPECT_FALSE(stored.used_for_training);
    EXPECT_EQ(stored.training_batch_id, 0);
    
    // Mark as used
    bool marked = feedback_store_->markUsedForTraining(stored.id, 42);
    ASSERT_TRUE(marked);
    
    // Verify update
    auto retrieved = feedback_store_->getFeedback(stored.id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_TRUE(retrieved->used_for_training);
    EXPECT_EQ(retrieved->training_batch_id, 42);
}

// Test: Get statistics
TEST_F(FeedbackStoreTest, GetStats) {
    // Create mix of feedback
    for (int i = 0; i < 10; i++) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = (i < 6) ? FeedbackType::POSITIVE : FeedbackType::NEGATIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Question " + std::to_string(i);
        feedback.answer = "Answer " + std::to_string(i);
        if (feedback.type == FeedbackType::NEGATIVE) {
            feedback.correction = "Correction " + std::to_string(i);
        }
        
        auto stored = feedback_store_->createFeedback(feedback);
        
        // Mark some as used for training
        if (i < 3) {
            feedback_store_->markUsedForTraining(stored.id, 1);
        }
    }
    
    auto stats = feedback_store_->getStats();
    
    EXPECT_EQ(stats.total_feedback, 10);
    EXPECT_EQ(stats.positive_count, 6);
    EXPECT_EQ(stats.negative_count, 4);
    EXPECT_EQ(stats.used_for_training, 3);
    EXPECT_EQ(stats.unused_for_training, 7);
    EXPECT_NEAR(stats.positive_ratio, 0.6, 0.01);
}

// Test: Delete feedback
TEST_F(FeedbackStoreTest, DeleteFeedback) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "user003";
    feedback.question = "Delete test";
    feedback.answer = "Delete test answer";
    
    auto stored = feedback_store_->createFeedback(feedback);
    
    // Verify it exists
    auto retrieved = feedback_store_->getFeedback(stored.id);
    ASSERT_TRUE(retrieved.has_value());
    
    // Delete it
    bool deleted = feedback_store_->deleteFeedback(stored.id);
    ASSERT_TRUE(deleted);
    
    // Verify it's gone
    auto after_delete = feedback_store_->getFeedback(stored.id);
    EXPECT_FALSE(after_delete.has_value());
    
    // Try to delete again (should return false)
    bool deleted_again = feedback_store_->deleteFeedback(stored.id);
    EXPECT_FALSE(deleted_again);
}

// Test: Clear all feedback
TEST_F(FeedbackStoreTest, ClearAllFeedback) {
    // Create some feedback
    for (int i = 0; i < 5; i++) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = FeedbackType::POSITIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Question " + std::to_string(i);
        feedback.answer = "Answer " + std::to_string(i);
        feedback_store_->createFeedback(feedback);
    }
    
    // Verify feedback exists
    auto before_clear = feedback_store_->listFeedback();
    EXPECT_EQ(before_clear.size(), 5);
    
    // Clear all
    feedback_store_->clear();
    
    // Verify all cleared
    auto after_clear = feedback_store_->listFeedback();
    EXPECT_EQ(after_clear.size(), 0);
}

// Test: Spam detection - too short
TEST_F(FeedbackStoreTest, SpamDetectionTooShort) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "spammer";
    feedback.question = "ab"; // Too short
    feedback.answer = "Test answer";
    
    auto validation = FeedbackStore::validateFeedback(feedback);
    EXPECT_EQ(validation, ValidationStatus::REJECTED);
}

// Test: Spam detection - excessive repetition
TEST_F(FeedbackStoreTest, SpamDetectionRepetition) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::POSITIVE;
    feedback.user_id = "spammer";
    feedback.question = "aaaaaaaaaaaaaaaaaaa"; // Excessive repetition
    feedback.answer = "Test answer";
    
    auto validation = FeedbackStore::validateFeedback(feedback);
    EXPECT_EQ(validation, ValidationStatus::REJECTED);
}

// Test: Negative feedback without correction should be flagged
TEST_F(FeedbackStoreTest, NegativeFeedbackWithoutCorrection) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::NEGATIVE;
    feedback.user_id = "user004";
    feedback.question = "Valid question";
    feedback.answer = "Invalid answer";
    feedback.correction = ""; // No correction
    feedback.comment = ""; // No comment
    
    auto validation = FeedbackStore::validateFeedback(feedback);
    EXPECT_EQ(validation, ValidationStatus::FLAGGED);
}

// Test: Valid negative feedback with correction
TEST_F(FeedbackStoreTest, ValidNegativeFeedbackWithCorrection) {
    FeedbackStore::FeedbackEntry feedback;
    feedback.type = FeedbackType::NEGATIVE;
    feedback.user_id = "user005";
    feedback.question = "Valid question";
    feedback.answer = "Invalid answer";
    feedback.correction = "Correct answer here";
    
    auto validation = FeedbackStore::validateFeedback(feedback);
    EXPECT_EQ(validation, ValidationStatus::APPROVED);
}

// Test: List unused feedback for training
TEST_F(FeedbackStoreTest, ListUnusedForTraining) {
    // Create feedback and mark some as used
    for (int i = 0; i < 10; i++) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = FeedbackType::POSITIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Question " + std::to_string(i);
        feedback.answer = "Answer " + std::to_string(i);
        
        auto stored = feedback_store_->createFeedback(feedback);
        
        // Mark first 4 as used
        if (i < 4) {
            feedback_store_->markUsedForTraining(stored.id, 1);
        }
    }
    
    // List only unused
    FeedbackStore::ListOptions options;
    options.unused_for_training = true;
    
    auto unused = feedback_store_->listFeedback(options);
    
    EXPECT_EQ(unused.size(), 6);
    for (const auto& fb : unused) {
        EXPECT_FALSE(fb.used_for_training);
    }
}

// Test: Pagination with limit
TEST_F(FeedbackStoreTest, PaginationWithLimit) {
    // Create 20 feedback entries
    for (int i = 0; i < 20; i++) {
        FeedbackStore::FeedbackEntry feedback;
        feedback.type = FeedbackType::POSITIVE;
        feedback.user_id = "user" + std::to_string(i);
        feedback.question = "Question " + std::to_string(i);
        feedback.answer = "Answer " + std::to_string(i);
        feedback_store_->createFeedback(feedback);
        
        // Small delay to ensure different timestamps
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // List with limit of 5
    FeedbackStore::ListOptions options;
    options.limit = 5;
    
    auto limited = feedback_store_->listFeedback(options);
    
    EXPECT_EQ(limited.size(), 5);
}

} // namespace test
} // namespace llm
} // namespace themis
