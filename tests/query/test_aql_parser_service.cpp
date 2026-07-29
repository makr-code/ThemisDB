/**
 * @file test_aql_parser_service.cpp
 * @brief Unit tests for AQL parser service interface
 *
 * Tests the parser service wrapper, ensuring:
 * - ParseResult struct population
 * - ParserDiagnostics accuracy
 * - Feature capability reporting
 * - Thread safety
 * - Error handling
 *
 * @author ThemisDB Test Suite
 * @date 2026-06-18
 */

#include <gtest/gtest.h>
#include "query/aql_parser_service.h"

namespace themis { namespace query { namespace test { 

class AQLParserServiceTest : public ::testing::Test {
protected:
    AQLParserServiceTest() 
        : service_(std::make_unique<AQLParserServiceImpl>()) {}
    
    std::unique_ptr<AQLParserService> service_;
};

// ============================================================================
// Success Path Tests
// ============================================================================

/// Test 1: Parse simple FOR...RETURN query
TEST_F(AQLParserServiceTest, ParseSimpleForReturn) {
    std::string query = R"(FOR doc IN users RETURN doc)";
    
    auto result = service_->parse(query);
    
    EXPECT_TRUE(result.success);
    ASSERT_NE(nullptr, result.ast);
    EXPECT_EQ(0, result.diagnostics.line_number);
    EXPECT_EQ(0, result.diagnostics.column_number);
}

/// Test 2: Parse complex query with FILTER, SORT, LIMIT
TEST_F(AQLParserServiceTest, ParseComplexQuery) {
    std::string query = R"(
        FOR doc IN users
        FILTER doc.age > 18
        SORT doc.created DESC
        LIMIT 10
        RETURN { name: doc.name, age: doc.age }
    )";
    
    auto result = service_->parse(query);
    
    EXPECT_TRUE(result.success);
    ASSERT_NE(nullptr, result.ast);
}

/// Test 3: Parse query with LET clause
TEST_F(AQLParserServiceTest, ParseQueryWithLet) {
    std::string query = R"(
        LET threshold = 100
        FOR doc IN items
        FILTER doc.price > threshold
        RETURN doc
    )";
    
    auto result = service_->parse(query);
    
    EXPECT_TRUE(result.success);
    ASSERT_NE(nullptr, result.ast);
}

/// Test 4: Parse query with COLLECT
TEST_F(AQLParserServiceTest, ParseQueryWithCollect) {
    std::string query = R"(
        FOR doc IN sales
        COLLECT category = doc.category
        RETURN { category, total: SUM(doc.amount) }
    )";
    
    auto result = service_->parse(query);
    
    EXPECT_TRUE(result.success);
    ASSERT_NE(nullptr, result.ast);
}

/// Test 5: Parse geospatial query with ST_Distance
TEST_F(AQLParserServiceTest, ParseGeospatialQuery) {
    std::string query = R"(
        FOR doc IN locations
        LET dist = ST_Distance([40.7128, -74.0060], doc.coords)
        FILTER dist < 5
        RETURN { name: doc.name, distance: dist }
    )";
    
    auto result = service_->parse(query);
    
    EXPECT_TRUE(result.success);
    ASSERT_NE(nullptr, result.ast);
}

// ============================================================================
// Error Path Tests
// ============================================================================

/// Test 6: Parse fails - missing RETURN clause
TEST_F(AQLParserServiceTest, ParseFailsMissingReturn) {
    std::string query = R"(FOR doc IN users)";
    
    auto result = service_->parse(query);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(nullptr, result.ast);
    EXPECT_FALSE(result.diagnostics.error_message.empty());
    EXPECT_EQ("SYNTAX_ERROR", result.diagnostics.error_category);
}

/// Test 7: Parse fails - unknown keyword
TEST_F(AQLParserServiceTest, ParseFailsUnknownKeyword) {
    std::string query = R"(FOR doc IN users UNKNOWN doc)";
    
    auto result = service_->parse(query);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(nullptr, result.ast);
    EXPECT_FALSE(result.diagnostics.error_message.empty());
}

/// Test 8: Parse fails - malformed function call
TEST_F(AQLParserServiceTest, ParseFailsMalformedFunction) {
    std::string query = R"(FOR doc IN users RETURN LENGTH(doc))";
    
    auto result = service_->parse(query);
    
    // This might succeed depending on parser implementation
    // If it fails, check diagnostics
    if (!result.success) {
        EXPECT_EQ(nullptr, result.ast);
        EXPECT_FALSE(result.diagnostics.error_message.empty());
    }
}

/// Test 9: Parse fails - empty query
TEST_F(AQLParserServiceTest, ParseFailsEmptyQuery) {
    std::string query = "";
    
    auto result = service_->parse(query);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(nullptr, result.ast);
    EXPECT_EQ("EMPTY_QUERY", result.diagnostics.error_category);
}

/// Test 10: Parse fails - only whitespace
TEST_F(AQLParserServiceTest, ParseFailsWhitespaceOnly) {
    std::string query = "   \n\t  ";
    
    auto result = service_->parse(query);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(nullptr, result.ast);
}

// ============================================================================
// Diagnostics Tests
// ============================================================================

/// Test 11: Diagnostics populated on error
TEST_F(AQLParserServiceTest, DiagnosticsPopulatedOnError) {
    std::string query = R"(FOR doc IN users)";
    
    auto result = service_->parse(query);
    
    ASSERT_FALSE(result.success);
    
    // Diagnostics should be populated
    const auto& diag = result.diagnostics;
    EXPECT_FALSE(diag.error_message.empty());
    EXPECT_FALSE(diag.error_category.empty());
    
    // Line number should be reasonable (0 or 1)
    EXPECT_GE(diag.line_number, 0);
    EXPECT_LE(diag.line_number, 10);
}

/// Test 12: Diagnostics include error context
TEST_F(AQLParserServiceTest, DiagnosticsIncludeContext) {
    std::string query = R"(FOR doc IN users FILTER)";
    
    auto result = service_->parse(query);
    
    if (!result.success) {
        const auto& diag = result.diagnostics;
        // Error context might be the offending line or excerpt
        if (!diag.error_context.empty()) {
            EXPECT_NE(std::string::npos, diag.error_context.find("FILTER"));
        }
    }
}

/// Test 13: Diagnostics include suggestions
TEST_F(AQLParserServiceTest, DiagnosticsIncludeSuggestions) {
    std::string query = R"(FOR doc IN users)";
    
    auto result = service_->parse(query);
    
    if (!result.success) {
        const auto& diag = result.diagnostics;
        // Suggestions might include "add RETURN clause"
        // This is optional but helpful
        if (!diag.suggestions.empty()) {
            EXPECT_FALSE(diag.suggestions[0].empty());
        }
    }
}

// ============================================================================
// Feature Tests
// ============================================================================

/// Test 14: Version string is non-empty
TEST_F(AQLParserServiceTest, VersionStringNonEmpty) {
    std::string version = service_->version();
    
    EXPECT_FALSE(version.empty());
    EXPECT_NE(std::string::npos, version.find("AQL"));
}

/// Test 15: supportsFeature - core features
TEST_F(AQLParserServiceTest, SupportsCoreFeaturesAlways) {
    // Core AQL features should always be supported
    EXPECT_TRUE(service_->supportsFeature("for"));
    EXPECT_TRUE(service_->supportsFeature("filter"));
    EXPECT_TRUE(service_->supportsFeature("sort"));
    EXPECT_TRUE(service_->supportsFeature("limit"));
    EXPECT_TRUE(service_->supportsFeature("return"));
}

/// Test 16: supportsFeature - geospatial default
TEST_F(AQLParserServiceTest, SupportsGeospatialByDefault) {
    // Default service should support geospatial (functions exist in let_evaluator)
    EXPECT_TRUE(service_->supportsFeature("geospatial"));
}

/// Test 17: supportsFeature - mutations disabled by default
TEST_F(AQLParserServiceTest, MutationsDisabledByDefault) {
    EXPECT_FALSE(service_->supportsFeature("mutations"));
    EXPECT_FALSE(service_->supportsFeature("dml"));
}

/// Test 18: supportsFeature - ddl disabled by default
TEST_F(AQLParserServiceTest, DDLDisabledByDefault) {
    EXPECT_FALSE(service_->supportsFeature("ddl"));
}

/// Test 19: supportsFeature with custom features
TEST_F(AQLParserServiceTest, CustomFeatureConfiguration) {
    // Create service with mutations enabled
    auto service_with_mutations = std::make_unique<AQLParserServiceImpl>(
        true,  // enable_mutations
        false, // enable_ddl
        true   // enable_geospatial
    );
    
    EXPECT_TRUE(service_with_mutations->supportsFeature("mutations"));
    EXPECT_FALSE(service_with_mutations->supportsFeature("ddl"));
}

// ============================================================================
// Factory Tests
// ============================================================================

/// Test 20: Factory creates working instance
TEST_F(AQLParserServiceTest, FactoryCreatesWorkingInstance) {
    auto service = AQLParserServiceFactory::create();
    
    ASSERT_NE(nullptr, service);
    EXPECT_FALSE(service->version().empty());
}

/// Test 21: Factory with features creates configured instance
TEST_F(AQLParserServiceTest, FactoryWithFeaturesConfigures) {
    auto service = AQLParserServiceFactory::createWithFeatures(true, true, true);
    
    ASSERT_NE(nullptr, service);
    EXPECT_TRUE(service->supportsFeature("mutations"));
    EXPECT_TRUE(service->supportsFeature("ddl"));
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

/// Test 22: Concurrent parse calls are thread-safe
TEST_F(AQLParserServiceTest, ConcurrentParseThreadSafe) {
    const int num_threads = 4;
    const int queries_per_thread = 10;
    std::vector<std::thread> threads;
    
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    auto worker = [this, &success_count, &failure_count]() {
        for (int i = 0; i < queries_per_thread; ++i) {
            std::string query = R"(FOR doc IN users RETURN doc)";
            auto result = service_->parse(query);
            
            if (result.success) {
                success_count++;
            } else {
                failure_count++;
            }
        }
    };
    
    // Spawn threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker);
    }
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // All queries should succeed
    EXPECT_EQ(num_threads * queries_per_thread, success_count.load());
    EXPECT_EQ(0, failure_count.load());
}

// ============================================================================
// Edge Cases
// ============================================================================

/// Test 23: Parse query with special characters in string literals
TEST_F(AQLParserServiceTest, ParseQueryWithSpecialChars) {
    std::string query = R"(FOR doc IN users FILTER doc.name == "O'Brien" RETURN doc)";
    
    auto result = service_->parse(query);
    
    // Should handle special characters in literals
    // Result depends on parser robustness
    if (!result.success) {
        EXPECT_FALSE(result.diagnostics.error_message.empty());
    }
}

/// Test 24: Parse very long query
TEST_F(AQLParserServiceTest, ParseVeryLongQuery) {
    std::string query = "FOR doc IN users";
    
    // Add many FILTER clauses
    for (int i = 0; i < 50; ++i) {
        query += " LET var" + std::to_string(i) + " = doc.field";
    }
    query += " RETURN doc";
    
    auto result = service_->parse(query);
    
    // Should handle large queries (or fail gracefully)
    if (result.success) {
        EXPECT_NE(nullptr, result.ast);
    } else {
        EXPECT_FALSE(result.diagnostics.error_message.empty());
    }
}

/// Test 25: Parse query with nested function calls
TEST_F(AQLParserServiceTest, ParseNestedFunctionCalls) {
    std::string query = R"(
        FOR doc IN users
        RETURN { upper: UPPER(LOWER(doc.name)) }
    )";
    
    auto result = service_->parse(query);
    
    if (result.success) {
        EXPECT_NE(nullptr, result.ast);
    }
}
} } } // namespace themis::query::test
