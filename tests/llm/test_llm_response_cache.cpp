#include <gtest/gtest.h>
#include "llm/llm_response_cache.h"
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis::llm;

class LLMResponseCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.similarity_threshold = 0.90f;
        config_.ttl_seconds = 3600;  // 1 hour
        config_.max_entries = 100;
        // Unique cache dir per test to avoid cross-test interference
        auto tmp = std::filesystem::temp_directory_path() / std::filesystem::path("llm_cache_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        cache_dir_ = tmp.string();
        config_.cache_dir = cache_dir_;
    }

    void TearDown() override {
        if (!cache_dir_.empty()) {
            std::error_code ec = {};
            std::filesystem::remove_all(cache_dir_, ec);
        }
    }

    InferenceResponse createResponse(const std::string& text) {
        InferenceResponse response;
        response.text = text;
        response.tokens_generated = 50;
        response.inference_time_ms = 150;
        return response;
    }

    LLMResponseCache::Config config_;
    std::string cache_dir_;
};

TEST_F(LLMResponseCacheTest, BasicPutAndGet) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("ThemisDB is a distributed graph database.");
    cache.put("What is ThemisDB?", response);
    
    auto cached = cache.get("What is ThemisDB?");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, response.text);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 0);
    EXPECT_EQ(stats.total_entries, 1);
}

TEST_F(LLMResponseCacheTest, SemanticSimilarityMatching) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("ThemisDB is a distributed graph database.");
    cache.put("What is ThemisDB?", response);
    
    // Similar prompt should match
    auto cached = cache.get("Can you explain what ThemisDB is?");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, response.text);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 1);
}

TEST_F(LLMResponseCacheTest, ExactMatchPriority) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response1 = createResponse("Exact match response");
    auto response2 = createResponse("Similar match response");
    
    cache.put("What is ThemisDB?", response1);
    cache.put("What is ThemisDB exactly?", response2);
    
    // Exact match should be returned, not semantic match
    auto cached = cache.get("What is ThemisDB?");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, response1.text);
}

TEST_F(LLMResponseCacheTest, NoMatchBelowThreshold) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("ThemisDB is a database.");
    cache.put("What is ThemisDB?", response);
    
    // Very different prompt - should not match
    auto cached = cache.get("How do I install Python?");
    EXPECT_FALSE(cached.has_value());
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.misses, 1);
}

TEST_F(LLMResponseCacheTest, TTLExpiration) {
    config_.ttl_seconds = 1;  // 1 second TTL
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("Response text");
    cache.put("Test prompt", response);
    
    // Should be cached immediately
    auto cached1 = cache.get("Test prompt");
    ASSERT_TRUE(cached1.has_value());
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    auto cached2 = cache.get("Test prompt");
    EXPECT_FALSE(cached2.has_value());
}

TEST_F(LLMResponseCacheTest, UpdateExistingResponse) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response1 = createResponse("Old response");
    auto response2 = createResponse("New response");
    
    cache.put("Test prompt", response1);
    cache.put("Test prompt", response2);  // Update
    
    auto cached = cache.get("Test prompt");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text, response2.text);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_entries, 1);  // Still only 1 entry
}

TEST_F(LLMResponseCacheTest, CacheStatistics) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("Response");
    cache.put("Prompt 1", response);
    cache.put("Prompt 2", response);
    
    cache.get("Prompt 1");  // Hit
    cache.get("Prompt 2");  // Hit
    cache.get("Prompt 3");  // Miss
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 2);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_EQ(stats.total_entries, 2);
    EXPECT_NEAR(stats.getHitRate(), 0.666, 0.01);
    EXPECT_GT(stats.avg_lookup_time_ms, 0.0);
}

TEST_F(LLMResponseCacheTest, MultipleEntriesSameSimilarity) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response1 = createResponse("Response 1");
    auto response2 = createResponse("Response 2");
    
    cache.put("What is ThemisDB?", response1);
    cache.put("What is ThemisDB exactly?", response2);
    
    // Should return best semantic match
    auto cached = cache.get("Can you explain ThemisDB?");
    ASSERT_TRUE(cached.has_value());
    // Will match one of them based on similarity
}

TEST_F(LLMResponseCacheTest, LargeResponse) {
    LLMResponseCache cache("test_cache", config_);
    
    std::string large_text(10000, 'x');  // 10KB response
    auto response = createResponse(large_text);
    
    cache.put("Large prompt", response);
    
    auto cached = cache.get("Large prompt");
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->text.size(), 10000);
}

TEST_F(LLMResponseCacheTest, EmptyPrompt) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("Response");
    cache.put("", response);
    
    auto cached = cache.get("");
    ASSERT_TRUE(cached.has_value());
}

TEST_F(LLMResponseCacheTest, HighConcurrency) {
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("Response");
    cache.put("Shared prompt", response);
    
    std::vector<std::thread> threads;
    std::atomic<int> hits{0};
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&cache, &hits]() {
            for (int j = 0; j < 100; ++j) {
                auto cached = cache.get("Shared prompt");
                if (cached.has_value()) {
                  hits++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
      t.join();
    }
    
    EXPECT_EQ(hits, 1000);  // All should hit
}

TEST_F(LLMResponseCacheTest, CachePersistence) {
    // Note: This test assumes RocksDB integration
    // For stub implementation, entries are only in-memory
    LLMResponseCache cache("test_cache", config_);
    
    auto response = createResponse("Persistent response");
    cache.put("Persistent prompt", response);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_entries, 1);
}

TEST_F(LLMResponseCacheTest, InvalidateByPattern) {
    LLMResponseCache cache("test_cache", config_);
    
    cache.put("What is ThemisDB?", createResponse("Response 1"));
    cache.put("How to use ThemisDB?", createResponse("Response 2"));
    cache.put("What is Python?", createResponse("Response 3"));
    
    // Invalidate all "ThemisDB" prompts
    size_t invalidated = cache.invalidate(".*ThemisDB.*");
    EXPECT_EQ(invalidated, 2);
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_entries, 1);
}

TEST_F(LLMResponseCacheTest, RealisticWorkflow) {
    LLMResponseCache cache("test_cache", config_);
    
    // Simulate customer support bot
    struct Query {
        std::string prompt;
        std::string expected_response;
    };
    
    std::vector<Query> queries = {
        {"How do I reset my password?", "To reset your password, go to..."},
        {"What is the password reset process?", ""},  // Should hit cache (similar)
        {"I forgot my password", ""},  // Should hit cache (similar)
        {"How do I contact support?", "You can contact support at..."},
        {"What are your support hours?", "Our support hours are..."},
    };
    
    // First query - cache miss
    cache.put(queries[0].prompt, createResponse(queries[0].expected_response));
    
    // Second query - semantic hit
    auto cached2 = cache.get(queries[1].prompt);
    ASSERT_TRUE(cached2.has_value());
    
    // Third query - semantic hit
    auto cached3 = cache.get(queries[2].prompt);
    ASSERT_TRUE(cached3.has_value());
    
    // Fourth query - cache miss (different topic)
    auto cached4 = cache.get(queries[3].prompt);
    EXPECT_FALSE(cached4.has_value());
    
    cache.put(queries[3].prompt, createResponse(queries[3].expected_response));
    
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits, 2);  // queries[1] and queries[2]
    EXPECT_EQ(stats.misses, 1);  // observed: queries[3]
    EXPECT_NEAR(stats.getHitRate(), 0.5, 0.2);
}

// Thread-Safety Tests (FIND-018)

TEST_F(LLMResponseCacheTest, ConcurrentPutAndGet) {
    LLMResponseCache cache("test_cache", config_);
    
    std::atomic<int> put_count{0};
    std::atomic<int> get_count{0};
    std::vector<std::thread> threads;
    
    // Spawn threads that concurrently put and get
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&cache, &put_count, i]() {
            for (int j = 0; j < 20; ++j) {
                std::string prompt = "Prompt " + std::to_string(i * 20 + j);
                auto response = InferenceResponse();
                response.text = "Response " + std::to_string(i * 20 + j);
                cache.put(prompt, response);
                put_count++;
            }
        });
    }
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&cache, &get_count, i]() {
            for (int j = 0; j < 20; ++j) {
                std::string prompt = "Prompt " + std::to_string(i * 20 + j);
                cache.get(prompt);  // May hit or miss
                get_count++;
            }
        });
    }
    
    for (auto& t : threads) {
      t.join();
    }
    
    EXPECT_EQ(put_count, 100);
    EXPECT_EQ(get_count, 100);
    
    // Statistics should be consistent (no race conditions)
    auto stats = cache.getStatistics();
    EXPECT_GE(stats.hits.load() + stats.misses.load(), 100);
}

TEST_F(LLMResponseCacheTest, ConcurrentStatisticsUpdate) {
    LLMResponseCache cache("test_cache", config_);
    
    // Pre-populate cache
    for (int i = 0; i < 10; ++i) {
        std::string prompt = "Prompt " + std::to_string(i);
        auto response = InferenceResponse();
        response.text = "Response " + std::to_string(i);
        cache.put(prompt, response);
    }
    
    std::vector<std::thread> threads;
    
    // Spawn many threads doing concurrent get operations
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&cache, i]() {
            for (int j = 0; j < 100; ++j) {
                std::string prompt = "Prompt " + std::to_string(j % 10);
                cache.get(prompt);
            }
        });
    }
    
    for (auto& t : threads) {
      t.join();
    }
    
    // Verify statistics are consistent (no lost updates)
    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.hits.load() + stats.misses.load(), 1000);
    EXPECT_GT(stats.hits.load(), 900);  // Most should be hits
    EXPECT_GE(stats.avg_lookup_time_ms.load(), 0.0);
}

TEST_F(LLMResponseCacheTest, ConcurrentClearAndAccess) {
    LLMResponseCache cache("test_cache", config_);
    
    std::atomic<bool> should_stop{false};
    std::vector<std::thread> threads;
    
    // Thread that continuously adds entries
    threads.emplace_back([&cache, &should_stop]() {
        int count = 0;
        while (!should_stop) {
            std::string prompt = "Prompt " + std::to_string(count++);
            auto response = InferenceResponse();
            response.text = "Response";
            cache.put(prompt, response);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // Thread that continuously reads entries
    threads.emplace_back([&cache, &should_stop]() {
        int count = 0;
        while (!should_stop) {
            std::string prompt = "Prompt " + std::to_string(count++);
            cache.get(prompt);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // Thread that periodically clears cache
    threads.emplace_back([&cache, &should_stop]() {
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            cache.clear();
        }
        should_stop = true;
    });
    
    for (auto& t : threads) {
      t.join();
    }
    
    // No crashes = success
    SUCCEED();
}

TEST_F(LLMResponseCacheTest, ConcurrentInvalidate) {
    LLMResponseCache cache("test_cache", config_);
    
    // Pre-populate cache
    for (int i = 0; i < 50; ++i) {
        std::string prompt = "Prompt " + std::to_string(i);
        auto response = InferenceResponse();
        response.text = "Response " + std::to_string(i);
        cache.put(prompt, response);
    }
    
    std::vector<std::thread> threads;
    
    // Multiple threads invalidating different patterns
    threads.emplace_back([&cache]() {
        cache.invalidate(".*[0-4]$");  // Invalidate ending in 0-4
    });
    
    threads.emplace_back([&cache]() {
        cache.invalidate(".*[5-9]$");  // Invalidate ending in 5-9
    });
    
    // Thread reading during invalidation
    threads.emplace_back([&cache]() {
        for (int i = 0; i < 50; ++i) {
            std::string prompt = "Prompt " + std::to_string(i);
            cache.get(prompt);
        }
    });
    
    for (auto& t : threads) {
      t.join();
    }
    
    // Some entries should be invalidated
    auto stats = cache.getStatistics();
    EXPECT_LT(stats.total_entries.load(), 50);
}

// No custom main; gtest_main provides the entry point
