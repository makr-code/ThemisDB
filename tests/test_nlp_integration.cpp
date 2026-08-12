/**
 * @file test_nlp_integration.cpp
 * @brief Integration tests for NLP Text Analyzer in AQL Query Pipeline
 * 
 * Tests NLP integration with AQL Parser, Query Optimizer, and Query Engine
 * as part of PR #317 Phase 1 implementation.
 */

#include <gtest/gtest.h>
#include "analytics/nlp_text_analyzer.h"
#include "query/query_optimizer.h"
#include <string>
#include <iostream>

using namespace themis;
using namespace themis::analytics;
using namespace themis::query;

class NLPIntegrationTest : public ::testing::Test {
protected:
    NlpTextAnalyzer nlp;
    
    void SetUp() override {
        // NLP analyzer is ready to use
    }
};

/**
 * @test Verify NLP metadata is correctly added to QueryOptimizer::Plan
 */
TEST_F(NLPIntegrationTest, QueryPlanHasNLPMetadata) {
    QueryOptimizer::Plan plan;
    
    // Verify NLP fields exist and have default values
    EXPECT_EQ(plan.nlp_complexity, 0.0);
    EXPECT_TRUE(plan.nlp_suggested_indexes.empty());
    EXPECT_TRUE(plan.nlp_hints.empty());
    
    // Simulate setting NLP metadata
    plan.nlp_complexity = 0.75;
    plan.nlp_suggested_indexes = {"btree", "hash"};
    plan.nlp_hints["aggregation"] = "detected";
    plan.nlp_hints["join_type"] = "inner";
    
    // Verify metadata was set correctly
    EXPECT_DOUBLE_EQ(plan.nlp_complexity, 0.75);
    EXPECT_EQ(plan.nlp_suggested_indexes.size(), 2u);
    EXPECT_EQ(plan.nlp_suggested_indexes[0], "btree");
    EXPECT_EQ(plan.nlp_suggested_indexes[1], "hash");
    EXPECT_EQ(plan.nlp_hints["aggregation"], "detected");
    EXPECT_EQ(plan.nlp_hints["join_type"], "inner");
}

/**
 * @test Test NLP query analysis for simple query
 */
TEST_F(NLPIntegrationTest, SimpleQueryAnalysis) {
    std::string query = "FOR u IN users FILTER u.age > 18 RETURN u";
    
    // Analyze query
    double complexity = nlp.estimateQueryComplexity(query);
    auto hints = nlp.extractQueryHints(query);
    auto indexes = nlp.suggestIndexes(query);
    
    // Verify results
    EXPECT_GE(complexity, 0.0);
    EXPECT_LE(complexity, 1.0);
    EXPECT_FALSE(indexes.empty());
    
    std::cout << "Query: " << query << "\n";
    std::cout << "Complexity: " << complexity << "\n";
    std::cout << "Suggested indexes: ";
    for (const auto& idx : indexes) {
        std::cout << idx << " ";
    }
    std::cout << "\n";
}

/**
 * @test Test NLP query analysis for complex query
 */
TEST_F(NLPIntegrationTest, ComplexQueryAnalysis) {
    std::string query = 
        "FOR u IN users "
        "FOR o IN orders "
        "FILTER u.id == o.user_id "
        "FILTER o.amount > 100 "
        "COLLECT city = u.city AGGREGATE total = SUM(o.amount) "
        "SORT total DESC "
        "LIMIT 10 "
        "RETURN {city, total}";
    
    // Analyze query
    double complexity = nlp.estimateQueryComplexity(query);
    auto hints = nlp.extractQueryHints(query);
    auto indexes = nlp.suggestIndexes(query);
    
    // Complex query should have higher complexity
    EXPECT_GT(complexity, 0.4);
    
    // Should detect aggregation
    EXPECT_TRUE(hints.count("aggregation") > 0 || hints.count("function_aggregate") > 0);
    
    // Should suggest indexes
    EXPECT_FALSE(indexes.empty());
    
    std::cout << "Complex Query Complexity: " << complexity << "\n";
    std::cout << "Hints detected: " << hints.size() << "\n";
    for (const auto& [key, value] : hints) {
        std::cout << "  " << key << ": " << value << "\n";
    }
}

/**
 * @test Test query normalization for caching
 */
TEST_F(NLPIntegrationTest, QueryNormalization) {
    std::string query1 = "FOR u IN users FILTER u.status == 'active' RETURN u";
    std::string query2 = "FOR   u   IN   users   FILTER   u.status == 'active'   RETURN   u";
    
    // Normalize both queries
    std::string norm1 = nlp.normalizeQuery(query1);
    std::string norm2 = nlp.normalizeQuery(query2);
    
    // Normalized forms should be similar (whitespace collapsed)
    EXPECT_FALSE(norm1.empty());
    EXPECT_FALSE(norm2.empty());
    
    std::cout << "Original 1: " << query1 << "\n";
    std::cout << "Normalized 1: " << norm1 << "\n";
    std::cout << "Original 2: " << query2 << "\n";
    std::cout << "Normalized 2: " << norm2 << "\n";
}

/**
 * @test Test NLP with fulltext query
 */
TEST_F(NLPIntegrationTest, FulltextQueryAnalysis) {
    std::string query = 
        "FOR doc IN documents "
        "FILTER MATCH(doc.content, 'database optimization performance') "
        "SORT BM25(doc) DESC "
        "LIMIT 20 "
        "RETURN doc";
    
    // Analyze query
    double complexity = nlp.estimateQueryComplexity(query);
    auto hints = nlp.extractQueryHints(query);
    auto indexes = nlp.suggestIndexes(query);
    
    // Should detect fulltext patterns
    EXPECT_GT(complexity, 0.0);
    
    // Should suggest fulltext index
    bool has_fulltext = false;
    for (const auto& idx : indexes) {
        if (idx == "fulltext" || idx == "inverted") {
            has_fulltext = true;
            break;
        }
    }
    EXPECT_TRUE(has_fulltext);
    
    std::cout << "Fulltext query complexity: " << complexity << "\n";
    std::cout << "Suggested indexes: ";
    for (const auto& idx : indexes) {
        std::cout << idx << " ";
    }
    std::cout << "\n";
}

/**
 * @test Verify NLP analysis has acceptable performance
 */
TEST_F(NLPIntegrationTest, PerformanceBenchmark) {
    std::string query = 
        "FOR u IN users "
        "FILTER u.age > 18 AND u.status == 'active' "
        "SORT u.created_at DESC "
        "LIMIT 100 "
        "RETURN u";
    
    // Measure NLP analysis time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        nlp.normalizeQuery(query);
        nlp.estimateQueryComplexity(query);
        nlp.extractQueryHints(query);
        nlp.suggestIndexes(query);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    double avg_ms = duration / 100.0;
    
    std::cout << "Average NLP analysis time: " << avg_ms << " ms\n";
    
    // Should complete in reasonable time (< 5ms per query)
    EXPECT_LT(avg_ms, 5.0);
}

/**
 * @test Test integration with multiple query types
 */
TEST_F(NLPIntegrationTest, MultipleQueryTypes) {
    std::vector<std::string> queries = {
        "FOR u IN users RETURN u",  // Simple scan
        "FOR u IN users FILTER u.id == '123' RETURN u",  // Equality filter
        "FOR u IN users FILTER u.age > 18 RETURN u",  // Range filter
        "FOR u IN users SORT u.name RETURN u",  // Sort
        "FOR u IN users LIMIT 10 RETURN u",  // Limit
    };
    
    for (const auto& query : queries) {
        auto complexity = nlp.estimateQueryComplexity(query);
        auto hints = nlp.extractQueryHints(query);
        auto indexes = nlp.suggestIndexes(query);
        
        // All queries should produce valid results
        EXPECT_GE(complexity, 0.0);
        EXPECT_LE(complexity, 1.0);
        // Hints and indexes may be empty for simple queries
        
        std::cout << "Query: " << query.substr(0, 50) << "...\n";
        std::cout << "  Complexity: " << complexity << "\n";
        std::cout << "  Hints: " << hints.size() << "\n";
        std::cout << "  Indexes: " << indexes.size() << "\n";
    }
}


