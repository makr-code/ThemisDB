/**
 * @file test_episodic_compaction.cpp
 * @brief Unit tests for L2 episodic memory compression (P2-D03 gate validation).
 * @version 0.1.0-beta
 */

#include <gtest/gtest.h>
#include "aql/llm_extractive_compressor.h"
#include "aql/llm_aql_handler.h"
#include "llm/llm_interaction_store.h"
#include "llm/prompt_manager.h"

#include <memory>
#include <vector>
#include <string>
#include <cmath>

namespace themis {
namespace aql {
namespace tests {

class MockLLMAQLHandler : public LLMAQLHandler {
public:
    std::string executeChat(const std::vector<std::pair<std::string, std::string>>& history,
                           const std::string& schema_context = "") override {
        return "mocked_response";
    }

    std::string executeInfer(const std::string& query, const std::string& context = "") override {
        return "mocked_inference_result";
    }

    bool isInitialized() const override { return true; }
    void reset() override {}
};

class EpisodicCompressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_handler_ = std::make_unique<MockLLMAQLHandler>();
        compressor_ = std::make_unique<LLMExtractiveCompressor>(*mock_handler_);
    }

    void TearDown() override {
        compressor_.reset();
        mock_handler_.reset();
    }

    std::vector<std::pair<std::string, std::string>> createSampleHistory() {
        return {
            {"system", "You are a helpful AQL query assistant. You have access to collections: users, orders, products."},
            {"user", "Find all users in Berlin"},
            {"assistant", "FOR u IN users FILTER u.city == \"Berlin\" RETURN u"},
            {"user", "Also show their email addresses"},
            {"assistant", "FOR u IN users FILTER u.city == \"Berlin\" RETURN {name: u.name, email: u.email}"},
            {"user", "And add a limit of 10 results"},
            {"assistant", "FOR u IN users FILTER u.city == \"Berlin\" RETURN {name: u.name, email: u.email} LIMIT 10"},
            {"user", "Sort by name ascending"},
            {"assistant", "FOR u IN users FILTER u.city == \"Berlin\" RETURN {name: u.name, email: u.email} LIMIT 10 SORT u.name ASC"},
            {"user", "Now also include their age"},
            {"assistant", "FOR u IN users FILTER u.city == \"Berlin\" RETURN {name: u.name, email: u.email, age: u.age} LIMIT 10 SORT u.name ASC"},
        };
    }

    std::unique_ptr<MockLLMAQLHandler> mock_handler_;
    std::unique_ptr<LLMExtractiveCompressor> compressor_;
};

// --- Test Cases ---

TEST_F(EpisodicCompressionTest, AvailabilityCheck) {
    EXPECT_TRUE(compressor_->isAvailable());
}

TEST_F(EpisodicCompressionTest, AvailabilityReflectsLLMClientReadiness) {
    LLMAQLHandler real_handler;
    LLMExtractiveCompressor real_compressor(real_handler);
    EXPECT_TRUE(real_compressor.isAvailable());

    real_handler.setLLMClient(nullptr);
    EXPECT_FALSE(real_compressor.isAvailable());
}

TEST_F(EpisodicCompressionTest, BasicCompressionRoundTrip) {
    auto history = createSampleHistory();
    
    auto result = compressor_->compressHistory(history, 512, 0.85f);
    
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->episode_id.empty());
    EXPECT_GT(result->original_token_count, 0);
    EXPECT_GT(result->compressed_token_count, 0);
    EXPECT_LT(result->compressed_token_count, result->original_token_count);
    EXPECT_FALSE(result->summary.empty());
}

TEST_F(EpisodicCompressionTest, SemanticSimilarityPreservation) {
    auto history = createSampleHistory();
    
    auto result = compressor_->compressHistory(history, 512, 0.85f);
    
    ASSERT_NE(result, nullptr);
    // P2-GATE-03: semantic similarity must be >= 0.85
    EXPECT_GE(result->semantic_similarity, 0.85f);
}

TEST_F(EpisodicCompressionTest, TokenBudgetRespected) {
    auto history = createSampleHistory();
    int32_t max_tokens = 256;
    
    auto result = compressor_->compressHistory(history, max_tokens, 0.70f);
    
    if (result) {
        EXPECT_LE(result->compressed_token_count, max_tokens * 1.1f);  // Allow 10% overage
    }
}

TEST_F(EpisodicCompressionTest, SystemMessagePreserved) {
    auto history = createSampleHistory();
    std::string expected_system = history[0].second;
    
    auto result = compressor_->compressHistory(history, 512, 0.85f);
    
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->summary.find(expected_system) != std::string::npos);
}

TEST_F(EpisodicCompressionTest, InvalidInputHandling) {
    std::vector<std::pair<std::string, std::string>> empty_history;
    
    auto result = compressor_->compressHistory(empty_history, 256);
    
    EXPECT_EQ(result, nullptr);
}

TEST_F(EpisodicCompressionTest, InvalidParametersThrow) {
    auto history = createSampleHistory();
    
    // Test too small max_tokens
    EXPECT_THROW(
        compressor_->compressHistory(history, 64, 0.85f),
        std::invalid_argument
    );
    
    // Test invalid min_similarity
    EXPECT_THROW(
        compressor_->compressHistory(history, 256, 0.4f),
        std::invalid_argument
    );
}

TEST_F(EpisodicCompressionTest, CompressionRatio) {
    auto history = createSampleHistory();
    
    auto result = compressor_->compressHistory(history, 512, 0.85f);
    
    ASSERT_NE(result, nullptr);
    float ratio = static_cast<float>(result->original_token_count) / 
                 result->compressed_token_count;
    EXPECT_GT(ratio, 1.0f);  // Compression should reduce size
    EXPECT_LT(ratio, 10.0f);  // But not excessively
}

TEST_F(EpisodicCompressionTest, ConsistentEpisodeIds) {
    auto history = createSampleHistory();
    
    auto result1 = compressor_->compressHistory(history, 512, 0.85f);
    auto result2 = compressor_->compressHistory(history, 512, 0.85f);
    
    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    // Episode IDs should be unique (generated UUIDs)
    EXPECT_NE(result1->episode_id, result2->episode_id);
}

TEST_F(EpisodicCompressionTest, TimestampRecorded) {
    auto history = createSampleHistory();
    auto before_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    
    auto result = compressor_->compressHistory(history, 512, 0.85f);
    
    auto after_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    
    ASSERT_NE(result, nullptr);
    EXPECT_GE(result->timestamp_ms, before_ms);
    EXPECT_LE(result->timestamp_ms, after_ms);
}

TEST_F(EpisodicCompressionTest, SelectedIndicesTracked) {
    auto history = createSampleHistory();
    
    auto result = compressor_->compressHistory(history, 512, 0.85f);
    
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->selected_indices.empty());
    
    // Verify indices are valid and sorted
    for (int32_t idx : result->selected_indices) {
        EXPECT_GE(idx, 0);
        EXPECT_LT(idx, static_cast<int32_t>(history.size()));
    }
    
    // Indices should be sorted for readability
    for (size_t i = 1; i < result->selected_indices.size(); ++i) {
        EXPECT_LE(result->selected_indices[i-1], result->selected_indices[i]);
    }
}

TEST_F(EpisodicCompressionTest, StatisticsTracking) {
    auto history = createSampleHistory();
    
    compressor_->compressHistory(history, 512, 0.85f);
    compressor_->compressHistory(history, 512, 0.85f);
    
    std::string stats = compressor_->getStatistics();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("total_compressions"), std::string::npos);
}

TEST_F(EpisodicCompressionTest, LargeBudgetNoCompression) {
    auto history = createSampleHistory();
    int32_t very_large_budget = 100000;
    
    auto result = compressor_->compressHistory(history, very_large_budget, 0.85f);
    
    // With a very large budget, compressed should be close to original
    ASSERT_NE(result, nullptr);
    float ratio = static_cast<float>(result->original_token_count) / 
                 result->compressed_token_count;
    EXPECT_LT(ratio, 1.5f);  // Less aggressive compression
}

TEST_F(EpisodicCompressionTest, SmallBudgetCompression) {
    auto history = createSampleHistory();
    int32_t tiny_budget = 256;
    
    auto result = compressor_->compressHistory(history, tiny_budget, 0.70f);
    
    // With tiny budget, more aggressive compression
    if (result) {
        float ratio = static_cast<float>(result->original_token_count) / 
                     result->compressed_token_count;
        EXPECT_GT(ratio, 2.0f);  // Should compress significantly
    }
}

// P2-GATE-03 Compliance Test
TEST_F(EpisodicCompressionTest, P2GATE03SemanticSimilarity) {
    // Test multiple compression scenarios
    auto history = createSampleHistory();
    
    for (int32_t budget = 256; budget <= 512; budget += 128) {
        auto result = compressor_->compressHistory(history, budget, 0.85f);
        
        if (result) {
            // P2-GATE-03: must preserve semantic similarity >= 0.85
            EXPECT_GE(result->semantic_similarity, 0.85f) 
                << "Failed for budget=" << budget;
        }
    }
}

} // namespace tests
} // namespace aql
} // namespace themis
