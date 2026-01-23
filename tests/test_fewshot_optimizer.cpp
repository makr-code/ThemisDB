/**
 * @file test_fewshot_optimizer.cpp
 * @brief Unit tests for FewShotOptimizer
 */

#include <gtest/gtest.h>
#include "llm/fewshot_optimizer.h"

using namespace themis::llm;

class FewShotOptimizerTest : public ::testing::Test {
protected:
    FewShotConfig config_;
    
    void SetUp() override {
        config_.max_examples = 5;
        config_.min_examples = 1;
        config_.diversity_weight = 0.4;
        config_.relevance_weight = 0.6;
        config_.enable_caching = true;
        config_.cache_size = 100;
    }
    
    std::vector<FewShotExample> createTestExamples() {
        return {
            {"What is 2+2?", "4", {}, 0.0, 0.0, {}},
            {"What is 3+3?", "6", {}, 0.0, 0.0, {}},
            {"What is the capital of France?", "Paris", {}, 0.0, 0.0, {}},
            {"What is the capital of Germany?", "Berlin", {}, 0.0, 0.0, {}},
            {"Who wrote Hamlet?", "Shakespeare", {}, 0.0, 0.0, {}},
            {"Who painted Mona Lisa?", "Leonardo da Vinci", {}, 0.0, 0.0, {}}
        };
    }
};

TEST_F(FewShotOptimizerTest, BasicSelection) {
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples();
    std::string query = "What is 5+5?";
    
    auto result = optimizer.selectExamples(query, examples, 3);
    
    EXPECT_EQ(result.selected_examples.size(), 3);
    EXPECT_GT(result.avg_relevance, 0.0);
    EXPECT_GT(result.selection_score, 0.0);
}

TEST_F(FewShotOptimizerTest, RelevanceScoring) {
    FewShotExample ex1{"What is 2+2?", "4", {}, 0.0, 0.0, {}};
    FewShotExample ex2{"Who wrote Hamlet?", "Shakespeare", {}, 0.0, 0.0, {}};
    
    std::string query = "What is 3+3?";
    
    double rel1 = FewShotOptimizer::computeRelevance(query, ex1);
    double rel2 = FewShotOptimizer::computeRelevance(query, ex2);
    
    // Math query should be more relevant to math example
    EXPECT_GT(rel1, rel2);
}

TEST_F(FewShotOptimizerTest, DiversityScoring) {
    std::vector<FewShotExample> similar = {
        {"What is 2+2?", "4", {}, 0.0, 0.0, {}},
        {"What is 3+3?", "6", {}, 0.0, 0.0, {}}
    };
    
    std::vector<FewShotExample> diverse = {
        {"What is 2+2?", "4", {}, 0.0, 0.0, {}},
        {"Who wrote Hamlet?", "Shakespeare", {}, 0.0, 0.0, {}}
    };
    
    double div1 = FewShotOptimizer::computeDiversity(similar);
    double div2 = FewShotOptimizer::computeDiversity(diverse);
    
    // Diverse examples should have higher diversity score
    EXPECT_GT(div2, div1);
}

TEST_F(FewShotOptimizerTest, ExampleCaching) {
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples();
    optimizer.cacheExamples(examples);
    
    auto stats = optimizer.getCacheStats();
    EXPECT_EQ(stats["cache_size"].get<size_t>(), examples.size());
    EXPECT_TRUE(stats["caching_enabled"].get<bool>());
}

TEST_F(FewShotOptimizerTest, CachedExampleRetrieval) {
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples();
    optimizer.cacheExamples(examples);
    
    std::string query = "What is addition?";
    auto cached = optimizer.getCachedExamples(query, 2);
    
    EXPECT_LE(cached.size(), 2);
    // Should retrieve math-related examples
}

TEST_F(FewShotOptimizerTest, CacheSizeLimit) {
    config_.cache_size = 3;
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples(); // 6 examples
    optimizer.cacheExamples(examples);
    
    auto stats = optimizer.getCacheStats();
    EXPECT_LE(stats["cache_size"].get<size_t>(), config_.cache_size);
}

TEST_F(FewShotOptimizerTest, ClearCache) {
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples();
    optimizer.cacheExamples(examples);
    
    EXPECT_GT(optimizer.getCacheStats()["cache_size"].get<size_t>(), 0);
    
    optimizer.clearCache();
    EXPECT_EQ(optimizer.getCacheStats()["cache_size"].get<size_t>(), 0);
}

TEST_F(FewShotOptimizerTest, ExampleFormatting) {
    std::vector<FewShotExample> examples = {
        {"input1", "output1", {}, 0.0, 0.0, {}},
        {"input2", "output2", {}, 0.0, 0.0, {}}
    };
    
    std::string formatted = FewShotOptimizer::formatExamples(examples);
    
    EXPECT_NE(formatted.find("input1"), std::string::npos);
    EXPECT_NE(formatted.find("output1"), std::string::npos);
    EXPECT_NE(formatted.find("input2"), std::string::npos);
    EXPECT_NE(formatted.find("output2"), std::string::npos);
}

TEST_F(FewShotOptimizerTest, CustomFormatTemplate) {
    std::vector<FewShotExample> examples = {
        {"Q", "A", {}, 0.0, 0.0, {}}
    };
    
    std::string custom_format = "Question: {input} Answer: {output}\n";
    std::string formatted = FewShotOptimizer::formatExamples(examples, custom_format);
    
    EXPECT_NE(formatted.find("Question: Q"), std::string::npos);
    EXPECT_NE(formatted.find("Answer: A"), std::string::npos);
}

TEST_F(FewShotOptimizerTest, EmptyExamples) {
    FewShotOptimizer optimizer(config_);
    
    std::vector<FewShotExample> empty;
    auto result = optimizer.selectExamples("query", empty);
    
    EXPECT_EQ(result.selected_examples.size(), 0);
    EXPECT_DOUBLE_EQ(result.selection_score, 0.0);
}

TEST_F(FewShotOptimizerTest, SelectionCount) {
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples();
    
    // Request more than available
    auto result1 = optimizer.selectExamples("query", examples, 100);
    EXPECT_EQ(result1.selected_examples.size(), examples.size());
    
    // Request specific count
    auto result2 = optimizer.selectExamples("query", examples, 2);
    EXPECT_EQ(result2.selected_examples.size(), 2);
}

TEST_F(FewShotOptimizerTest, ConfigurationUpdate) {
    FewShotOptimizer optimizer(config_);
    
    EXPECT_EQ(optimizer.getConfig().max_examples, 5);
    
    FewShotConfig new_config;
    new_config.max_examples = 10;
    optimizer.setConfig(new_config);
    
    EXPECT_EQ(optimizer.getConfig().max_examples, 10);
}

TEST_F(FewShotOptimizerTest, MetadataInResult) {
    FewShotOptimizer optimizer(config_);
    
    auto examples = createTestExamples();
    auto result = optimizer.selectExamples("query", examples);
    
    EXPECT_TRUE(result.metadata.contains("num_candidates"));
    EXPECT_TRUE(result.metadata.contains("num_selected"));
    EXPECT_EQ(result.metadata["num_candidates"].get<size_t>(), examples.size());
}
