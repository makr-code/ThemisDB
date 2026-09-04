#include <gtest/gtest.h>
#include "llm/kv_cache_buffer.h"
#include <thread>
#include <vector>
#include <chrono>
#include <limits>

using namespace themis::llm;

class KVCacheBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_tokens_per_batch = 256;
        config_.embedding_dim = 128;
        config_.num_layers = 4;
        config_.enable_auto_flush = true;
        config_.flush_interval = std::chrono::milliseconds(100);
    }

    KVCacheBuffer::Config config_;
};

TEST_F(KVCacheBufferTest, BasicAppendToken) {
    KVCacheBuffer buffer(config_);
    
    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);
    
    bool flushed = buffer.appendToken(1, key.data(), value.data());
    EXPECT_FALSE(flushed);  // First token shouldn't trigger flush
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_appends, 1);
    EXPECT_EQ(stats.current_batch_size, 1);
}

TEST_F(KVCacheBufferTest, AppendTokenRejectsNullPointers) {
    KVCacheBuffer buffer(config_);

    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);

    EXPECT_FALSE(buffer.appendToken(1, nullptr, value.data()));
    EXPECT_FALSE(buffer.appendToken(1, key.data(), nullptr));

    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_appends, 0);
    EXPECT_EQ(stats.current_batch_size, 0);
}

TEST_F(KVCacheBufferTest, AppendMultipleTokens) {
    KVCacheBuffer buffer(config_);
    
    size_t n_tokens = 10;
    std::vector<float> keys(n_tokens * config_.embedding_dim, 1.0f);
    std::vector<float> values(n_tokens * config_.embedding_dim, 2.0f);
    
    bool flushed = buffer.appendTokens(1, keys, values, n_tokens);
    EXPECT_FALSE(flushed);
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_appends, n_tokens);
    EXPECT_EQ(stats.current_batch_size, n_tokens);
}

TEST_F(KVCacheBufferTest, AppendTokensRejectsPositiveTokensWhenEmbeddingDimZero) {
    config_.embedding_dim = 0;
    KVCacheBuffer buffer(config_);

    std::vector<float> empty;
    EXPECT_FALSE(buffer.appendTokens(1, empty, empty, 1));

    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_appends, 0);
    EXPECT_EQ(stats.current_batch_size, 0);
}

TEST_F(KVCacheBufferTest, AppendTokensRejectsSizeComputationOverflow) {
    config_.embedding_dim = 2;
    KVCacheBuffer buffer(config_);

    std::vector<float> empty;
    const size_t overflowing_tokens = (std::numeric_limits<size_t>::max() / config_.embedding_dim) + 1;
    EXPECT_THROW(buffer.appendTokens(1, empty, empty, overflowing_tokens), std::invalid_argument);

    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_appends, 0);
    EXPECT_EQ(stats.current_batch_size, 0);
}

TEST_F(KVCacheBufferTest, AutoFlushOnBatchSizeThreshold) {
    KVCacheBuffer buffer(config_);
    
    int flush_count = 0;
    buffer.setFlushCallback([&](const std::vector<KVCacheBuffer::KVCache>& batch) {
        flush_count++;
        EXPECT_GT(batch.size(), 0);
    });
    
    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);
    
    // Append tokens until flush
    for (size_t i = 0; i < config_.max_tokens_per_batch + 1; ++i) {
        buffer.appendToken(1, key.data(), value.data());
    }
    
    EXPECT_EQ(flush_count, 1);
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_flushes, 1);
}

TEST_F(KVCacheBufferTest, ManualFlush) {
    KVCacheBuffer buffer(config_);
    
    int flush_count = 0;
    buffer.setFlushCallback([&](const std::vector<KVCacheBuffer::KVCache>& batch) {
        flush_count++;
    });
    
    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);
    
    buffer.appendToken(1, key.data(), value.data());
    buffer.flush();
    
    EXPECT_EQ(flush_count, 1);
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.current_batch_size, 0);  // Should be cleared after flush
}

TEST_F(KVCacheBufferTest, MultipleSequences) {
    KVCacheBuffer buffer(config_);
    
    bool flushed = false;
    buffer.setFlushCallback([&](const std::vector<KVCacheBuffer::KVCache>& batch) {
        EXPECT_EQ(batch.size(), 3);  // 3 sequences
        
        // Check sequence IDs
        std::set<int> seq_ids = {};

        for (const auto& cache : batch) {
            seq_ids.insert(cache.sequence_id);
        }
        EXPECT_EQ(seq_ids.size(), 3);
        EXPECT_TRUE(seq_ids.count(1));
        EXPECT_TRUE(seq_ids.count(2));
        EXPECT_TRUE(seq_ids.count(3));
        
        flushed = true;
    });
    
    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);
    
    // Append to 3 different sequences
    buffer.appendToken(1, key.data(), value.data());
    buffer.appendToken(2, key.data(), value.data());
    buffer.appendToken(3, key.data(), value.data());
    
    buffer.flush();
    EXPECT_TRUE(flushed);
}

TEST_F(KVCacheBufferTest, ClearBuffer) {
    KVCacheBuffer buffer(config_);
    
    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);
    
    buffer.appendToken(1, key.data(), value.data());
    buffer.clear();
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.current_batch_size, 0);
}

TEST_F(KVCacheBufferTest, Statistics) {
    KVCacheBuffer buffer(config_);
    
    std::vector<float> key(config_.embedding_dim, 1.0f);
    std::vector<float> value(config_.embedding_dim, 2.0f);
    
    // Add some tokens
    for (int i = 0; i < 100; ++i) {
        buffer.appendToken(1, key.data(), value.data());
    }
    
    buffer.flush();
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_appends, 100);
    EXPECT_EQ(stats.total_flushes, 1);
    EXPECT_EQ(stats.total_tokens_cached, 100);
    EXPECT_GT(stats.avg_batch_utilization, 0.0);
    EXPECT_LE(stats.avg_batch_utilization, 1.0);
}

// KVCacheBufferPool Tests

class KVCacheBufferPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool_config_.num_buffers = 4;
        pool_config_.buffer_config.max_tokens_per_batch = 256;
        pool_config_.buffer_config.embedding_dim = 128;
    }

    KVCacheBufferPool::Config pool_config_;
};

TEST_F(KVCacheBufferPoolTest, AcquireAndRelease) {
    KVCacheBufferPool pool(pool_config_);
    
    auto buffer = pool.acquireBuffer();
    EXPECT_NE(buffer, nullptr);
    
    auto stats = pool.getPoolStats();
    EXPECT_EQ(stats.total_buffers, 4);
    EXPECT_EQ(stats.available_buffers, 3);
    EXPECT_EQ(stats.acquired_buffers, 1);
    
    pool.releaseBuffer(buffer);
    
    stats = pool.getPoolStats();
    EXPECT_EQ(stats.available_buffers, 4);
    EXPECT_EQ(stats.acquired_buffers, 0);
}

TEST_F(KVCacheBufferPoolTest, AcquireMultiple) {
    KVCacheBufferPool pool(pool_config_);
    
    std::vector<std::shared_ptr<KVCacheBuffer>> buffers;
    
    // Acquire all buffers
    for (int i = 0; i < 4; ++i) {
        buffers.push_back(pool.acquireBuffer());
    }
    
    auto stats = pool.getPoolStats();
    EXPECT_EQ(stats.available_buffers, 0);
    EXPECT_EQ(stats.acquired_buffers, 4);
    
    // Acquire one more (should create temporary)
    auto extra = pool.acquireBuffer();
    EXPECT_NE(extra, nullptr);
    
    // Release all
    for (auto& buf : buffers) {
        pool.releaseBuffer(buf);
    }
    
    stats = pool.getPoolStats();
    EXPECT_EQ(stats.available_buffers, 4);
}

TEST_F(KVCacheBufferPoolTest, ConcurrentAccess) {
    KVCacheBufferPool pool(pool_config_);
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_acquires{0};
    
    // Multiple threads trying to acquire buffers
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            auto buffer = pool.acquireBuffer();
            if (buffer) {
                successful_acquires++;
                
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
                pool.releaseBuffer(buffer);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(successful_acquires, 10);
    
    auto stats = pool.getPoolStats();
    EXPECT_EQ(stats.available_buffers, 4);  // All returned
}

TEST_F(KVCacheBufferPoolTest, BufferReuse) {
    KVCacheBufferPool pool(pool_config_);
    
    auto buffer1 = pool.acquireBuffer();
    
    // Add some data
    std::vector<float> key(pool_config_.buffer_config.embedding_dim, 1.0f);
    std::vector<float> value(pool_config_.buffer_config.embedding_dim, 2.0f);
    buffer1->appendToken(1, key.data(), value.data());
    
    // Release (should clear)
    pool.releaseBuffer(buffer1);
    
    // Acquire again (should be cleared)
    auto buffer2 = pool.acquireBuffer();
    auto stats = buffer2->getStats();
    EXPECT_EQ(stats.current_batch_size, 0);  // Should be cleared
}

// Integration Test

TEST(KVCacheBufferIntegrationTest, RealisticWorkflow) {
    KVCacheBuffer::Config config;
    config.max_tokens_per_batch = 512;
    config.embedding_dim = 4096;
    config.num_layers = 32;
    config.enable_auto_flush = true;
    
    KVCacheBuffer buffer(config);
    
    int total_flushes = 0;
    size_t total_sequences = 0;
    
    buffer.setFlushCallback([&](const std::vector<KVCacheBuffer::KVCache>& batch) {
        total_flushes++;
        total_sequences += batch.size();
        
        // Verify batch contents
        for (const auto& cache : batch) {
            EXPECT_GT(cache.n_tokens, 0);
            EXPECT_EQ(cache.keys.size(), cache.n_tokens * config.embedding_dim);
            EXPECT_EQ(cache.values.size(), cache.n_tokens * config.embedding_dim);
        }
    });
    
    // Simulate multiple sequences with varying lengths
    std::vector<float> key(config.embedding_dim);
    std::vector<float> value(config.embedding_dim);
    
    // Sequence 1: 100 tokens
    for (int i = 0; i < 100; ++i) {
        std::fill(key.begin(), key.end(), static_cast<float>(i));
        std::fill(value.begin(), value.end(), static_cast<float>(i * 2));
        buffer.appendToken(1, key.data(), value.data());
    }
    
    // Sequence 2: 200 tokens
    for (int i = 0; i < 200; ++i) {
        std::fill(key.begin(), key.end(), static_cast<float>(i + 100));
        std::fill(value.begin(), value.end(), static_cast<float>((i + 100) * 2));
        buffer.appendToken(2, key.data(), value.data());
    }
    
    // Sequence 3: 300 tokens (should trigger auto-flush)
    for (int i = 0; i < 300; ++i) {
        std::fill(key.begin(), key.end(), static_cast<float>(i + 300));
        std::fill(value.begin(), value.end(), static_cast<float>((i + 300) * 2));
        buffer.appendToken(3, key.data(), value.data());
    }
    
    // Final flush
    buffer.flush();
    
    EXPECT_GT(total_flushes, 0);
    EXPECT_GT(total_sequences, 0);
    
    auto stats = buffer.getStats();
    EXPECT_EQ(stats.total_tokens_cached, 600);  // 100 + 200 + 300
}
