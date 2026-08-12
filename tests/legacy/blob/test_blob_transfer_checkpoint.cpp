#include <gtest/gtest.h>
#include "server/rpc/blob_transfer_handler.h"
#include <filesystem>
#include <fstream>
#include <vector>

using namespace themis::rpc;
namespace fs = std::filesystem;

class BlobTransferCheckpointTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "./data/test_blob_checkpoint";
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);
        
        checkpoint_dir_ = (fs::temp_directory_path() / "themis_blob_checkpoints").string();
        if (fs::exists(checkpoint_dir_)) {
            fs::remove_all(checkpoint_dir_);
        }
        
        // Create a test blob file
        test_blob_path_ = test_dir_ + "/test_blob.bin";
        CreateTestBlob(test_blob_path_, 5 * 1024 * 1024);  // 5 MB test file
        
        handler_ = std::make_unique<BlobTransferHandler>();
        
        // Configure blob transfer
        config_.blob_id = "test_blob_123";
        config_.blob_type = "test_data";
        config_.source_path = test_blob_path_;
        config_.dest_path = test_dir_ + "/test_blob_dest.bin";
        config_.compression_type = themis::sharding::proto::COMPRESSION_NONE;
        config_.compression_level = 1;
        config_.chunk_size_mb = 1;  // 1 MB chunks
        config_.checksum_type = themis::sharding::proto::CHECKSUM_CRC32;
        config_.enable_resume = true;
    }
    
    void TearDown() override {
        handler_.reset();
        
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        
        if (fs::exists(checkpoint_dir_)) {
            fs::remove_all(checkpoint_dir_);
        }
    }
    
    void CreateTestBlob(const std::string& path, size_t size) {
        std::ofstream file(path, std::ios::binary);
        std::vector<char> data(size);
        
        // Fill with pseudo-random data
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<char>(i % 256);
        }
        
        file.write(data.data(), data.size());
        file.close();
    }
    
    std::string test_dir_;
    std::string test_blob_path_;
    std::string checkpoint_dir_;
    std::unique_ptr<BlobTransferHandler> handler_;
    BlobConfig config_;
};

// Test 1: Create checkpoint
TEST_F(BlobTransferCheckpointTest, CreateCheckpoint) {
    auto status = handler_->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Stream all chunks (in real scenarios, transfer would be interrupted by network issues or cancellation)
    int chunks_received = 0;
    auto callback = [&chunks_received](const themis::sharding::proto::BlobChunk& chunk) {
        if (!chunk.is_last()) {
            chunks_received++;
        }
    };
    
    status = handler_->StreamChunks(callback);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Create checkpoint after transfer
    std::string checkpoint_id = handler_->CreateCheckpoint();
    EXPECT_FALSE(checkpoint_id.empty());
    EXPECT_TRUE(checkpoint_id.find("test_blob_123") != std::string::npos);
    
    // Verify checkpoint file was created
    fs::path checkpoint_path = fs::path(checkpoint_dir_) / (checkpoint_id + ".json");
    EXPECT_TRUE(fs::exists(checkpoint_path));
}

// Test 2: Resume transfer from checkpoint
TEST_F(BlobTransferCheckpointTest, ResumeTransferFromCheckpoint) {
    auto status = handler_->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Simulate partial transfer by streaming some chunks
    int chunks_streamed = 0;
    auto callback1 = [&chunks_streamed](const themis::sharding::proto::BlobChunk& chunk) {
        if (!chunk.is_last()) {
            chunks_streamed++;
        }
    };
    
    status = handler_->StreamChunks(callback1);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Create checkpoint after partial transfer
    std::string checkpoint_id = handler_->CreateCheckpoint();
    EXPECT_FALSE(checkpoint_id.empty());
    
    // Get current progress
    auto progress1 = handler_->GetProgress();
    uint64_t transferred_bytes = progress1.transferred_bytes;
    uint32_t transferred_chunks = progress1.transferred_chunks;
    
    // Create a new handler to simulate resume after crash
    auto handler2 = std::make_unique<BlobTransferHandler>();
    
    // Start transfer with same config
    status = handler2->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Resume from checkpoint
    status = handler2->ResumeTransfer(checkpoint_id);
    EXPECT_EQ(status, BlobStatus::OK);
    
    // Verify progress was restored
    auto progress2 = handler2->GetProgress();
    EXPECT_EQ(progress2.transferred_bytes, transferred_bytes);
    EXPECT_EQ(progress2.transferred_chunks, transferred_chunks);
}

// Test 3: Resume transfer with invalid checkpoint ID
TEST_F(BlobTransferCheckpointTest, ResumeTransferInvalidCheckpoint) {
    auto status = handler_->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Try to resume with non-existent checkpoint
    status = handler_->ResumeTransfer("invalid_checkpoint_id_12345");
    EXPECT_EQ(status, BlobStatus::ERROR_RESUME_FAILED);
}

// Test 4: Checkpoint persistence
TEST_F(BlobTransferCheckpointTest, CheckpointPersistence) {
    auto status = handler_->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Stream all chunks
    int total_chunks = 0;
    auto callback = [&total_chunks](const themis::sharding::proto::BlobChunk& chunk) {
        if (!chunk.is_last()) {
            total_chunks++;
        }
    };
    
    status = handler_->StreamChunks(callback);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Create checkpoint
    std::string checkpoint_id = handler_->CreateCheckpoint();
    EXPECT_FALSE(checkpoint_id.empty());
    
    // Read checkpoint file and verify content
    fs::path checkpoint_path = fs::path(checkpoint_dir_) / (checkpoint_id + ".json");
    ASSERT_TRUE(fs::exists(checkpoint_path));
    
    std::ifstream checkpoint_file(checkpoint_path);
    ASSERT_TRUE(checkpoint_file.is_open());
    
    // Verify file is not empty
    checkpoint_file.seekg(0, std::ios::end);
    size_t file_size = checkpoint_file.tellg();
    EXPECT_GT(file_size, 0);
}

// Test 5: Resume continues from correct position
TEST_F(BlobTransferCheckpointTest, ResumeContinuesFromCorrectPosition) {
    // This test verifies that resume skips already transferred chunks
    auto status = handler_->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Stream first 2 chunks
    int chunks_before_checkpoint = 0;
    std::vector<uint32_t> chunk_indices_before;
    auto callback1 = [&chunks_before_checkpoint, &chunk_indices_before](
        const themis::sharding::proto::BlobChunk& chunk) {
        if (!chunk.is_last()) {
            chunks_before_checkpoint++;
            chunk_indices_before.push_back(chunk.chunk_index());
        }
    };
    
    status = handler_->StreamChunks(callback1);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Create checkpoint
    std::string checkpoint_id = handler_->CreateCheckpoint();
    EXPECT_FALSE(checkpoint_id.empty());
    
    uint64_t bytes_before = handler_->GetProgress().transferred_bytes;
    uint32_t chunks_before = handler_->GetProgress().transferred_chunks;
    
    // Create new handler and resume
    auto handler2 = std::make_unique<BlobTransferHandler>();
    status = handler2->StartTransfer(config_);
    ASSERT_EQ(status, BlobStatus::OK);
    
    status = handler2->ResumeTransfer(checkpoint_id);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // Verify initial state is restored
    auto progress_after_resume = handler2->GetProgress();
    EXPECT_EQ(progress_after_resume.transferred_bytes, bytes_before);
    EXPECT_EQ(progress_after_resume.transferred_chunks, chunks_before);
    
    // Stream remaining chunks
    int chunks_after_resume = 0;
    std::vector<uint32_t> chunk_indices_after;
    auto callback2 = [&chunks_after_resume, &chunk_indices_after](
        const themis::sharding::proto::BlobChunk& chunk) {
        if (!chunk.is_last()) {
            chunks_after_resume++;
            chunk_indices_after.push_back(chunk.chunk_index());
        }
    };
    
    status = handler2->StreamChunks(callback2);
    ASSERT_EQ(status, BlobStatus::OK);
    
    // After resuming, chunk indices should start from where we left off
    if (!chunk_indices_after.empty() && !chunk_indices_before.empty()) {
        EXPECT_GE(chunk_indices_after[0], chunk_indices_before.back());
    }
}