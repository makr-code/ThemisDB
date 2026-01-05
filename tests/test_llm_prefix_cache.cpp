#include <gtest/gtest.h>
#include "llm/llm_prefix_cache.h"
#include <thread>
#include <chrono>

using namespace themis::llm;

class LLMPrefixCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.similarity_threshold = 0.95;
        config_.max_entries = 10;
        config_.min_prefix_length = 10;
        config_.ttl_seconds = 2;
        config_.enable_kv_caching = true;
    }
    
    LLMPrefixCache::Config config_;
};

TEST_F(LLMPrefixCacheTest, BasicPutAndGet) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "You are a helpful assistant.";
    std::vector<int> tokens = {1, 2, 3, 4, 5};
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
    
    cache.put(prefix, tokens, embedding);
    
    auto result = cache.get(prefix, embedding);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->prefix, prefix);
    EXPECT_EQ(result->token_ids, tokens);
    EXPECT_EQ(result->usage_count, 2);  // 1 from put, 1 from get
}

TEST_F(LLMPrefixCacheTest, SimilarityMatching) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "You are a helpful assistant.";
    std::vector<int> tokens = {1, 2, 3, 4, 5};
    std::vector<float> embedding1 = {1.0f, 0.0f, 0.0f, 0.0f};
    
    cache.put(prefix, tokens, embedding1);
    
    // Very similar embedding (96% cosine similarity)
    std::vector<float> embedding2 = {0.98f, 0.2f, 0.0f, 0.0f};
    auto result = cache.get("You are a helpful AI.", embedding2);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->prefix, prefix);
}

TEST_F(LLMPrefixCacheTest, NoMatchBelowThreshold) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "You are a helpful assistant.";
    std::vector<int> tokens = {1, 2, 3, 4, 5};
    std::vector<float> embedding1 = {1.0f, 0.0f, 0.0f, 0.0f};
    
    cache.put(prefix, tokens, embedding1);
    
    // Very different embedding
    std::vector<float> embedding2 = {0.0f, 1.0f, 0.0f, 0.0f};
    auto result = cache.get("Completely different text", embedding2);
    
    EXPECT_FALSE(result.has_value());
}

TEST_F(LLMPrefixCacheTest, LongestPrefixMatch) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix1 = "You are a helpful";
    std::string prefix2 = "You are a helpful assistant";
    std::vector<int> tokens1 = {1, 2, 3, 4};
    std::vector<int> tokens2 = {1, 2, 3, 4, 5};
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f, 0.4f};
    
    cache.put(prefix1, tokens1, embedding);
    cache.put(prefix2, tokens2, embedding);
    
    std::string full_text = "You are a helpful assistant. Please answer my question.";
    auto result = cache.getLongestMatch(full_text, embedding);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->prefix, prefix2);  // Longer prefix
    EXPECT_EQ(result->token_ids.size(), 5);
}

TEST_F(LLMPrefixCacheTest, PrecomputedKVCache) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "System prompt with context";
    std::vector<int> tokens = {1, 2, 3};
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    std::vector<float> kv_cache = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    
    cache.put(prefix, tokens, embedding, kv_cache);
    
    auto result = cache.get(prefix, embedding);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->has_precomputed_kv);
    EXPECT_EQ(result->precomputed_kv, kv_cache);
}

TEST_F(LLMPrefixCacheTest, MinPrefixLengthFilter) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string short_prefix = "Short";  // Less than min_prefix_length (10)
    std::vector<int> tokens = {1, 2};
    std::vector<float> embedding = {0.1f, 0.2f};
    
    cache.put(short_prefix, tokens, embedding);
    
    auto result = cache.get(short_prefix, embedding);
    EXPECT_FALSE(result.has_value());  // Not cached due to length
}

TEST_F(LLMPrefixCacheTest, TTLExpiration) {
    config_.ttl_seconds = 1;  // 1 second TTL
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "Expiring prefix text here";
    std::vector<int> tokens = {1, 2, 3};
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put(prefix, tokens, embedding);
    
    // Should be cached
    auto result1 = cache.get(prefix, embedding);
    EXPECT_TRUE(result1.has_value());
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    auto result2 = cache.get(prefix, embedding);
    EXPECT_FALSE(result2.has_value());
}

TEST_F(LLMPrefixCacheTest, LRUEviction) {
    config_.max_entries = 3;
    LLMPrefixCache cache("test_prefix", config_);
    
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put("Prefix 1 long enough", {1}, embedding);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache.put("Prefix 2 long enough", {2}, embedding);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cache.put("Prefix 3 long enough", {3}, embedding);
    
    // This should evict "Prefix 1" (oldest)
    cache.put("Prefix 4 long enough", {4}, embedding);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_entries, 3);
    
    // Prefix 1 should be evicted
    auto result = cache.get("Prefix 1 long enough", embedding);
    EXPECT_FALSE(result.has_value());
}

TEST_F(LLMPrefixCacheTest, TouchUpdatesUsage) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "Touchable prefix text";
    std::vector<int> tokens = {1, 2, 3};
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put(prefix, tokens, embedding);
    
    auto result1 = cache.get(prefix, embedding);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1->usage_count, 2);
    
    cache.touch(prefix);
    
    auto result2 = cache.get(prefix, embedding);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2->usage_count, 4);  // 1 (put) + 1 (get) + 1 (touch) + 1 (get)
}

TEST_F(LLMPrefixCacheTest, InvalidateByPattern) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put("System: You are helpful", {1}, embedding);
    cache.put("System: You are smart", {2}, embedding);
    cache.put("User input goes here", {3}, embedding);
    
    cache.invalidateByPattern("^System:");
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_entries, 1);  // Only "User input" remains
}

TEST_F(LLMPrefixCacheTest, CacheStatistics) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put("Prefix for stats test", {1, 2, 3}, embedding);
    
    // Hit
    auto result1 = cache.get("Prefix for stats test", embedding);
    EXPECT_TRUE(result1.has_value());
    
    // Miss
    std::vector<float> different_embedding = {0.9f, 0.8f, 0.7f};
    auto result2 = cache.get("Different prefix", different_embedding);
    EXPECT_FALSE(result2.has_value());
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.getHitRate(), 0.5);
    EXPECT_EQ(stats.total_entries, 1);
}

TEST_F(LLMPrefixCacheTest, ClearCache) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put("Prefix 1 long text", {1}, embedding);
    cache.put("Prefix 2 long text", {2}, embedding);
    cache.put("Prefix 3 long text", {3}, embedding);
    
    auto stats1 = cache.getStatistics();
    EXPECT_EQ(stats1.total_entries, 3);
    
    cache.clear();
    
    auto stats2 = cache.getStatistics();
    EXPECT_EQ(stats2.total_entries, 0);
    EXPECT_EQ(stats2.hits, 0);
    EXPECT_EQ(stats2.misses, 0);
}

TEST_F(LLMPrefixCacheTest, ConcurrentAccess) {
    LLMPrefixCache cache("test_prefix", config_);
    
    const int num_threads = 10;
    const int iterations = 100;
    
    auto worker = [&cache](int thread_id) {
        std::vector<float> embedding = {
            static_cast<float>(thread_id) / 10.0f,
            0.2f, 0.3f, 0.4f
        };
        
        for (int i = 0; i < iterations; ++i) {
            std::string prefix = "Thread " + std::to_string(thread_id) + " prefix text";
            std::vector<int> tokens = {thread_id, i};
            
            cache.put(prefix, tokens, embedding);
            auto result = cache.get(prefix, embedding);
            EXPECT_TRUE(result.has_value());
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto stats = cache.getStatistics();
    EXPECT_GT(stats.hits, 0);
    EXPECT_GT(stats.total_entries, 0);
}

TEST_F(LLMPrefixCacheTest, RealisticWorkflow) {
    LLMPrefixCache cache("test_prefix", config_);
    
    // System prompt (common across many requests)
    std::string system_prompt = "You are a helpful legal assistant specialized in contract law.";
    std::vector<int> system_tokens = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<float> system_embedding = {0.8f, 0.1f, 0.05f, 0.05f};
    std::vector<float> system_kv = {/* precomputed KV cache data */};
    
    cache.put(system_prompt, system_tokens, system_embedding, system_kv);
    
    // RAG context (common for similar questions)
    std::string rag_context = "Contract clause 3.4 states: The parties agree to...";
    std::vector<int> rag_tokens = {10, 11, 12, 13, 14, 15};
    std::vector<float> rag_embedding = {0.7f, 0.2f, 0.05f, 0.05f};
    
    cache.put(rag_context, rag_tokens, rag_embedding);
    
    // User queries with similar prefixes
    std::string query1 = "You are a helpful legal assistant specialized in contract law. What is clause 3.4?";
    auto match1 = cache.getLongestMatch(query1, system_embedding);
    ASSERT_TRUE(match1.has_value());
    EXPECT_EQ(match1->prefix, system_prompt);
    
    std::string query2 = "Contract clause 3.4 states: The parties agree to... Can you explain?";
    auto match2 = cache.getLongestMatch(query2, rag_embedding);
    ASSERT_TRUE(match2.has_value());
    EXPECT_EQ(match2->prefix, rag_context);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 2);
    EXPECT_GT(stats.total_tokens_saved, 0);
}

TEST_F(LLMPrefixCacheTest, TokensSavedStatistics) {
    LLMPrefixCache cache("test_prefix", config_);
    
    std::string prefix = "Common system prompt text here";
    std::vector<int> tokens(50);  // 50 tokens
    std::vector<float> embedding = {0.1f, 0.2f, 0.3f};
    
    cache.put(prefix, tokens, embedding);
    
    // First match
    auto match1 = cache.getLongestMatch(prefix + " extra text", embedding);
    EXPECT_TRUE(match1.has_value());
    
    // Second match
    auto match2 = cache.getLongestMatch(prefix + " more extra", embedding);
    EXPECT_TRUE(match2.has_value());
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_tokens_saved, 100);  // 50 tokens × 2 hits
}

TEST_F(LLMPrefixCacheTest, HNSWIntegrationTest) {
    // Test that HNSW-based similarity search works correctly
    LLMPrefixCache cache("test_hnsw", config_);
    
    // Add multiple prefixes with different embeddings
    std::string prefix1 = "You are a helpful assistant specialized in programming.";
    std::vector<int> tokens1 = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<float> embedding1 = {0.8f, 0.1f, 0.05f, 0.05f};
    
    std::string prefix2 = "You are a helpful assistant specialized in mathematics.";
    std::vector<int> tokens2 = {10, 11, 12, 13, 14, 15, 16};
    std::vector<float> embedding2 = {0.7f, 0.2f, 0.05f, 0.05f};
    
    std::string prefix3 = "Please analyze this document carefully.";
    std::vector<int> tokens3 = {20, 21, 22, 23, 24};
    std::vector<float> embedding3 = {0.1f, 0.1f, 0.7f, 0.1f};
    
    cache.put(prefix1, tokens1, embedding1);
    cache.put(prefix2, tokens2, embedding2);
    cache.put(prefix3, tokens3, embedding3);
    
    // Query with embedding similar to prefix1
    std::vector<float> query_embedding = {0.79f, 0.11f, 0.05f, 0.05f};
    auto result = cache.get("You are a helpful assistant in coding.", query_embedding);
    
    // Should match prefix1 or prefix2 due to high similarity
    ASSERT_TRUE(result.has_value());
    // Either prefix1 or prefix2 is acceptable since both are similar
    bool matched_expected = (result->prefix == prefix1 || result->prefix == prefix2);
    EXPECT_TRUE(matched_expected);
    
    // Query with very different embedding
    std::vector<float> different_embedding = {0.05f, 0.05f, 0.8f, 0.1f};
    auto result2 = cache.get("Analyze this document.", different_embedding);
    
    // Should match prefix3 due to high similarity
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2->prefix, prefix3);
    
    // Verify cache statistics
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 2);
    EXPECT_EQ(stats.total_entries, 3);
    EXPECT_GT(stats.avg_similarity, 0.9);  // Should have high average similarity
}
