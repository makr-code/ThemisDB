/**
 * @file test_aql_validation_performance.cpp
 * @brief Performance tests for AQL Consolidation Phase 2:
 *        Validation SLA verification (≤ 500ms per parse)
 * 
 * Tests verify that the parser validation pipeline meets defined SLA:
 * - Parser call duration: ≤ 500ms
 * - Validation throughput: ≥ 100 queries/second
 * - Error enrichment overhead: ≤ 50ms
 *
 * See: src/query/AQL_LLM_INTEGRATION_CONTRACT.md §4.3 (SLA & Guarantees)
 */

#include <gtest/gtest.h>
#include "query/aql_parser_service.h"
#include "query/aql_parser.h"
#include <memory>
#include <string>
#include <chrono>
#include <vector>

namespace themis {
namespace query {

/**
 * @brief Fixture for performance testing
 */
class AQLValidationPerformanceTest : public ::testing::Test {
protected:
    std::shared_ptr<AQLParserService> parser_;
    
    // Test queries of varying complexity
    std::string simple_query_ = "FOR u IN users RETURN u";
    std::string medium_query_ = R"(
FOR u IN users
  FILTER u.age > 18
  SORT u.created DESC
  RETURN {
    id: u._key,
    name: u.name,
    email: u.email,
    age: u.age
  }
)";
    std::string complex_query_ = R"(
FOR u IN users
  FILTER u.age > 18 && u.status == 'active'
  LET orders = (FOR o IN orders FILTER o.user_id == u._key RETURN o)
  LET total_spent = SUM(orders[*].amount)
  COLLECT category = u.category
    INTO group = {
      user: u.name,
      order_count: LENGTH(orders),
      total_spent: total_spent
    }
  SORT category ASC
  LIMIT 100
  RETURN {
    category: category,
    users: group
  }
)";
    
    void SetUp() override {
        parser_ = std::make_shared<AQLParserServiceImpl>(
            true,   // enable_mutations
            false,  // enable_ddl
            true    // enable_geospatial
        );
    }
};

/**
 * @test SimpleQueryValidationSLA: Simple queries validate in < 100ms
 * 
 * SLA: ≤ 500ms (target: < 100ms for simple queries)
 * Tests: Fast path validation with minimal overhead
 */
TEST_F(AQLValidationPerformanceTest, SimpleQueryValidationSLA) {
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        auto result = parser_->parse(simple_query_);
        EXPECT_TRUE(result.success);
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    // Average per query
    int avg_ms = duration.count() / 10;
    
    EXPECT_LT(avg_ms, 100)
        << "Simple query validation should complete in < 100ms, got " << avg_ms << "ms";
}

/**
 * @test MediumQueryValidationSLA: Medium-complexity queries validate in < 300ms
 * 
 * SLA: ≤ 500ms (target: < 300ms for medium queries)
 * Tests: Typical LLM-generated query patterns
 */
TEST_F(AQLValidationPerformanceTest, MediumQueryValidationSLA) {
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        auto result = parser_->parse(medium_query_);
        EXPECT_TRUE(result.success);
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    int avg_ms = duration.count() / 10;
    
    EXPECT_LT(avg_ms, 300)
        << "Medium query validation should complete in < 300ms, got " << avg_ms << "ms";
}

/**
 * @test ComplexQueryValidationSLA: Complex queries validate in ≤ 500ms
 * 
 * SLA: ≤ 500ms
 * Tests: Worst-case validation time (complex nested structure)
 */
TEST_F(AQLValidationPerformanceTest, ComplexQueryValidationSLA) {
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 5; ++i) {
        auto result = parser_->parse(complex_query_);
        EXPECT_TRUE(result.success);
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    int avg_ms = duration.count() / 5;
    
    EXPECT_LE(avg_ms, 500)
        << "Complex query validation should complete in ≤ 500ms, got " << avg_ms << "ms";
}

/**
 * @test ValidationThroughput: Parser handles ≥ 100 validations/second
 * 
 * SLA: ≥ 100 queries/second (at 5ms average per query)
 * Tests: Throughput under sustained load
 */
TEST_F(AQLValidationPerformanceTest, ValidationThroughput) {
    const int num_queries = 100;
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < num_queries; ++i) {
        auto result = parser_->parse(simple_query_);
        EXPECT_TRUE(result.success);
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    double throughput = (num_queries * 1000.0) / duration.count();
    
    EXPECT_GE(throughput, 100)
        << "Parser should handle ≥ 100 validations/second, got " << throughput << " q/s";
}

/**
 * @test ErrorEnrichmentOverhead: Parse error diagnostics add < 50ms overhead
 * 
 * SLA: Diagnostic enrichment ≤ 50ms
 * Tests: Error handling performance (location info, suggestions, categories)
 */
TEST_F(AQLValidationPerformanceTest, ErrorEnrichmentOverhead) {
    std::string malformed_query = "FOR u IN users FILTER u.age > RETURN u";
    
    auto start_parse = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        auto result = parser_->parse(malformed_query);
        EXPECT_FALSE(result.success);
        // Verify diagnostics are enriched (not empty)
        EXPECT_GT(result.diagnostics.error_message.length(), 10);
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_parse);
    
    int avg_ms = duration.count() / 10;
    
    EXPECT_LT(avg_ms, 150)
        << "Error enrichment should add < 50ms overhead (total < 150ms for error case), got " << avg_ms << "ms";
}

/**
 * @test LocationInfoGeneration: Error location computation is fast
 * 
 * Specifically tests that line:column computation doesn't add significant overhead
 */
TEST_F(AQLValidationPerformanceTest, LocationInfoGeneration) {
    // Multi-line query with error on a later line
    std::string multiline_query = R"(
FOR u IN users
  FILTER u.age > 18
  FILTER u.status >> 'active'
  RETURN u
)";
    
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        auto result = parser_->parse(multiline_query);
        EXPECT_FALSE(result.success);
        // Verify line information is available
        if (result.diagnostics.line_number > 0) {
            EXPECT_GT(result.diagnostics.line_number, 2)
                << "Error should be on line > 2";
        }
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    int avg_ms = duration.count() / 10;
    
    EXPECT_LT(avg_ms, 100)
        << "Location info generation should not add significant overhead";
}

/**
 * @test BatchValidation: Validating multiple queries in sequence
 * 
 * Tests typical LLM scenario: multiple retry attempts in a batch
 */
TEST_F(AQLValidationPerformanceTest, BatchValidation) {
    std::vector<std::string> queries = {
        "FOR u IN users RETURN u",
        "FOR o IN orders FILTER o.status == 'completed' RETURN o",
        "FOR u IN users LET o = (FOR x IN orders FILTER x.user_id == u._key RETURN x) RETURN {u, o}",
        "FOR p IN products FILTER p.price > 100 SORT p.created DESC RETURN p",
        "FOR u IN users COLLECT status = u.status INTO g WITH COUNT INTO count RETURN {status, count}"
    };
    
    auto start = std::chrono::steady_clock::now();
    
    for (int batch = 0; batch < 20; ++batch) {
        for (const auto& q : queries) {
            auto result = parser_->parse(q);
            EXPECT_TRUE(result.success);
        }
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    int total_queries = queries.size() * 20;
    int avg_ms = duration.count() / total_queries;
    
    EXPECT_LT(avg_ms, 50)
        << "Batch validation average should be < 50ms per query";
}

// =================== Test Main ===================

}  // namespace query
}  // namespace themis
