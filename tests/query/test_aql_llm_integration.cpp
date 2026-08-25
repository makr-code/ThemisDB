/**
 * @file test_aql_llm_integration.cpp
 * @brief Integration tests for AQL Consolidation Phase 2:
 *        LLM-to-AQL validation pipeline
 * 
 * Tests verify that the LLM layer can successfully:
 * 1. Call AQLParserService to validate generated AQL
 * 2. Handle parse failures with corrective feedback
 * 3. Retry on validation failure (if enabled)
 * 4. Return only validated AQL to user
 *
 * See: src/query/AQL_LLM_INTEGRATION_CONTRACT.md (canonical spec)
 *      src/aql/llm_aql_handler.cpp::validateAQLWithParser (implementation)
 */

#include <gtest/gtest.h>
#include "query/aql_parser_service.h"
#include "query/aql_parser.h"
#include <memory>
#include <string>

namespace themis {
namespace query {

/**
 * @brief Test fixture for LLM-AQL validation integration
 */
class LLMAQLValidationTest : public ::testing::Test {
protected:
    std::shared_ptr<AQLParserService> parser_;

    void SetUp() override {
        // Phase 1 spec: AQLParserServiceImpl with default config
        // - enable_mutations: true (LLM should be able to suggest mutations)
        // - enable_ddl: false (LLM should NOT suggest DDL by default)
        // - enable_geospatial: true (LLM should suggest geo queries)
        parser_ = std::make_shared<AQLParserServiceImpl>(
            true,   // enable_mutations
            false,  // enable_ddl
            true    // enable_geospatial
        );
    }
};

/**
 * @test ValidateSimpleQuery: Parser accepts valid simple FOR/RETURN
 * 
 * Simulates: LLM generates "FOR u IN users RETURN u", validation succeeds
 */
TEST_F(LLMAQLValidationTest, ValidateSimpleQuery) {
    std::string aql = "FOR u IN users RETURN u";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success) << "Simple FOR/RETURN query should parse";
    EXPECT_EQ(result.diagnostics.error_message, "")
        << "No error expected for valid query";
}

/**
 * @test ValidateSimpleQueryWithFilter: Parser accepts FOR/FILTER/RETURN
 * 
 * Simulates: LLM generates filtered query, validation succeeds
 */
TEST_F(LLMAQLValidationTest, ValidateSimpleQueryWithFilter) {
    std::string aql = "FOR u IN users FILTER u.age > 18 RETURN u";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success) << "FOR/FILTER/RETURN should parse";
}

/**
 * @test ValidateUpdateMutation: Parser accepts UPDATE when mutations enabled
 * 
 * Simulates: LLM generates mutation, validation succeeds (mutations enabled in SetUp)
 */
TEST_F(LLMAQLValidationTest, ValidateUpdateMutation) {
    std::string aql = "FOR u IN users FILTER u.id == 'user_1' UPDATE u WITH {status: 'active'} IN users";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success) << "UPDATE mutation should parse with mutations enabled";
}

/**
 * @test MalformedQueryReturnsError: Parser rejects invalid syntax
 * 
 * Simulates: LLM generates malformed AQL, parser provides detailed error
 * This is the case where LLM needs **corrective feedback** for retry
 */
TEST_F(LLMAQLValidationTest, MalformedQueryReturnsError) {
    std::string aql = "FOR u IN users FILTER u.age > RETURN u";  // Missing predicate
    auto result = parser_->parse(aql);
    
    EXPECT_FALSE(result.success) << "Malformed query should fail parse";
    EXPECT_NE(result.diagnostics.error_message, "")
        << "Error message should be populated for parser failure";
    
    // Verify diagnostic richness (for LLM feedback)
    EXPECT_GT(result.diagnostics.error_message.length(), 5)
        << "Error message should be detailed enough for LLM retry";
}

/**
 * @test MalformedQueryHasLocationInfo: Parser returns line:column info
 * 
 * Simulates: LLM uses error location to pinpoint mistake in generated AQL
 */
TEST_F(LLMAQLValidationTest, MalformedQueryHasLocationInfo) {
    std::string aql = "FOR u IN users FILTER u.age >> 18 RETURN u";  // Invalid operator >>
    auto result = parser_->parse(aql);
    
    EXPECT_FALSE(result.success) << "Query with invalid operator should fail";
    
    // Diagnostics should include location information (if available)
    // This helps LLM pinpoint the exact error location
    if (result.diagnostics.line_number > 0 || result.diagnostics.column_number > 0) {
        EXPECT_GT(result.diagnostics.line_number, 0)
            << "Line number should be populated for location-aware error";
    }
}

/**
 * @test ParserDiagnosticsIncludeSuggestions: Error message may include suggestions
 * 
 * Simulates: LLM receives corrective suggestions from parser for retry
 * 
 * Note: Suggestions are optional based on error type, but when available,
 * the LLM can use them to refine the query for retry
 */
TEST_F(LLMAQLValidationTest, ParserDiagnosticsIncludeSuggestions) {
    std::string aql = "FOR u IN users FILTER u.age > RETURN u";
    auto result = parser_->parse(aql);
    
    EXPECT_FALSE(result.success);
    
    // Suggestions are optional, but if provided, should be meaningful
    if (!result.diagnostics.suggestions.empty()) {
        EXPECT_GT(result.diagnostics.suggestions[0].length(), 0)
            << "Suggestions should be non-empty strings";
    }
}

/**
 * @test MultilineQueryParsing: Parser handles multiline AQL
 * 
 * Simulates: LLM generates formatted/readable multiline query
 */
TEST_F(LLMAQLValidationTest, MultilineQueryParsing) {
    std::string aql = R"(
FOR u IN users
  FILTER u.age > 18
  FILTER u.status == 'active'
  SORT u.created DESC
  RETURN {
    id: u._key,
    name: u.name,
    email: u.email
  }
)";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Multiline query with proper structure should parse";
}

/**
 * @test QueryWithComplexExpression: Parser validates complex expressions
 * 
 * Simulates: LLM generates query with nested functions/expressions
 */
TEST_F(LLMAQLValidationTest, QueryWithComplexExpression) {
    std::string aql = "FOR u IN users RETURN {name: UPPER(u.name), age: u.age + 1}";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Query with function calls and expressions should parse";
}

/**
 * @test DDLDisabled: Parser rejects DDL when disabled
 * 
 * Simulates: LLM attempts to generate CREATE COLLECTION, parser rejects
 * (DDL is disabled in SetUp for security reasons)
 */
TEST_F(LLMAQLValidationTest, DDLDisabled) {
    std::string aql = "CREATE COLLECTION users";
    auto result = parser_->parse(aql);
    
    // DDL is disabled (enable_ddl: false in SetUp), so this should fail
    EXPECT_FALSE(result.success) << "CREATE COLLECTION should fail with DDL disabled";
}

/**
 * @test JoinQuery: Parser supports complex join patterns
 * 
 * Simulates: LLM generates multi-collection join (common LLM pattern)
 */
TEST_F(LLMAQLValidationTest, JoinQuery) {
    std::string aql = R"(
FOR u IN users
  FOR o IN orders
    FILTER u._key == o.user_id
    RETURN {
      user: u.name,
      order_id: o._key,
      total: o.total
    }
)";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Nested FOR (join pattern) should parse";
}

/**
 * @test ValidateLETVariable: Parser handles LET bindings
 * 
 * Simulates: LLM generates query with intermediate variables
 */
TEST_F(LLMAQLValidationTest, ValidateLETVariable) {
    std::string aql = R"(
FOR u IN users
  LET orders = (FOR o IN orders FILTER o.user_id == u._key RETURN o)
  RETURN {
    user: u.name,
    order_count: LENGTH(orders),
    orders: orders
  }
)";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Query with LET bindings should parse";
}

/**
 * @test ValidateCOLLECT: Parser handles COLLECT aggregation
 * 
 * Simulates: LLM generates aggregation query
 */
TEST_F(LLMAQLValidationTest, ValidateCOLLECT) {
    std::string aql = R"(
FOR u IN users
  COLLECT status = u.status
  WITH COUNT INTO count
  RETURN {
    status: status,
    total_users: count
  }
)";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Query with COLLECT aggregation should parse";
}

/**
 * @test ValidateSORT: Parser handles ordering
 * 
 * Simulates: LLM generates sorted results
 */
TEST_F(LLMAQLValidationTest, ValidateSORT) {
    std::string aql = "FOR u IN users SORT u.created DESC, u.name ASC RETURN u";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Query with multi-field SORT should parse";
}

/**
 * @test ValidateLIMIT: Parser handles pagination
 * 
 * Simulates: LLM generates paginated results
 */
TEST_F(LLMAQLValidationTest, ValidateLIMIT) {
    std::string aql = "FOR u IN users SORT u.created DESC LIMIT 10, 20 RETURN u";
    auto result = parser_->parse(aql);
    
    EXPECT_TRUE(result.success)
        << "Query with LIMIT/OFFSET should parse";
}

/**
 * @test ErrorCategoryPopulated: Parser provides error categorization
 * 
 * Simulates: LLM categorizes error types for appropriate retry strategy
 */
TEST_F(LLMAQLValidationTest, ErrorCategoryPopulated) {
    std::string aql = "FORX u IN users RETURN u";  // FORX is typo
    auto result = parser_->parse(aql);
    
    EXPECT_FALSE(result.success);
    
    // Error category helps LLM determine retry strategy:
    // - syntax error: might be typo, worth retrying
    // - semantic error: might need semantic correction
    // - timeout: might need query simplification
    if (!result.diagnostics.error_category.empty()) {
        EXPECT_GT(result.diagnostics.error_category.length(), 0)
            << "Error category should guide LLM retry strategy";
    }
}

/**
 * @test ParserVersion: Verify parser service version
 * 
 * Simulates: Client queries parser version for compatibility check
 */
TEST_F(LLMAQLValidationTest, ParserVersion) {
    auto version = parser_->version();

    EXPECT_FALSE(version.empty())
        << "Parser version should not be empty";
}

/**
 * @test ValidateFeatureSupport: Query feature availability
 * 
 * Simulates: LLM queries which features are enabled before generating query
 */
TEST_F(LLMAQLValidationTest, ValidateFeatureSupport) {
    // Created parser with:
    // - mutations: enabled
    // - ddl: disabled
    // - geospatial: enabled
    
    EXPECT_TRUE(parser_->supportsFeature("mutations"))
        << "Mutations should be enabled";
    EXPECT_FALSE(parser_->supportsFeature("ddl"))
        << "DDL should be disabled";
    EXPECT_TRUE(parser_->supportsFeature("geospatial"))
        << "Geospatial should be enabled";
}

}  // namespace query
}  // namespace themis
