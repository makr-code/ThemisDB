/**
 * @file test_query_parser_scope_validation.cpp
 * @brief Parser scope validation tests (Phase 2 Agent 1)
 *
 * Tests the scope validation logic in the query parser to ensure:
 * - Collection names are properly registered and validated
 * - Scope boundaries are enforced
 * - Cross-collection access is prevented
 * - Nested scopes are properly handled
 */

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "utils/error_registry.h"

namespace themis {
namespace query {

// ============================================================================
// Test Fixture
// ============================================================================

class ParserScopeValidationTest : public ::testing::Test {
protected:
    ParserScopeContext scope_context_;
    AQLParser parser_;

    void SetUp() override {
        scope_context_.clear();
    }

    void TearDown() override {
        scope_context_.clear();
    }
};

// ============================================================================
// Unit Tests for ParserScopeContext
// ============================================================================

/**
 * Test: Registration of collections in scope
 */
TEST_F(ParserScopeValidationTest, RegisterCollectionSucceeds) {
    scope_context_.registerCollection("users");
    EXPECT_TRUE(scope_context_.isCollectionInScope("users"));
}

/**
 * Test: Multiple collections can be registered
 */
TEST_F(ParserScopeValidationTest, RegisterMultipleCollections) {
    scope_context_.registerCollection("users");
    scope_context_.registerCollection("posts");
    scope_context_.registerCollection("comments");

    EXPECT_TRUE(scope_context_.isCollectionInScope("users"));
    EXPECT_TRUE(scope_context_.isCollectionInScope("posts"));
    EXPECT_TRUE(scope_context_.isCollectionInScope("comments"));
}

/**
 * Test: Empty collection names are rejected
 */
TEST_F(ParserScopeValidationTest, EmptyCollectionNameRejected) {
    scope_context_.registerCollection("");
    EXPECT_FALSE(scope_context_.isCollectionInScope(""));
}

/**
 * Test: Unregistered collection is not in scope
 */
TEST_F(ParserScopeValidationTest, UnregisteredCollectionNotInScope) {
    scope_context_.registerCollection("users");
    EXPECT_FALSE(scope_context_.isCollectionInScope("posts"));
}

/**
 * Test: Graph synthetic collection name is always in scope
 */
TEST_F(ParserScopeValidationTest, GraphSyntheticCollectionAlwaysInScope) {
    // No registration needed for "graph"
    EXPECT_TRUE(scope_context_.isCollectionInScope("graph"));
}

/**
 * Test: validateCollectionAccess returns Ok for valid collection
 */
TEST_F(ParserScopeValidationTest, ValidateCollectionAccessSuccess) {
    scope_context_.registerCollection("users");
    auto result = scope_context_.validateCollectionAccess("users", "FOR");
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.ok_value(), true);
}

/**
 * Test: validateCollectionAccess returns error for invalid collection
 */
TEST_F(ParserScopeValidationTest, ValidateCollectionAccessFailure) {
    scope_context_.registerCollection("users");
    auto result = scope_context_.validateCollectionAccess("posts", "FOR");
    EXPECT_FALSE(result.is_ok());
    EXPECT_EQ(result.err_code(), errors::ErrorCode::ERR_QUERY_ACCESS_DENIED);
}

/**
 * Test: validateCollectionAccess error message includes context
 */
TEST_F(ParserScopeValidationTest, ValidateCollectionAccessErrorIncludesContext) {
    scope_context_.registerCollection("users");
    auto result = scope_context_.validateCollectionAccess("posts", "INSERT");
    EXPECT_FALSE(result.is_ok());
    // Error message should mention the context
    auto err_msg = result.err_message();
    EXPECT_NE(err_msg.find("INSERT"), std::string::npos);
}

/**
 * Test: Clear removes all registered collections
 */
TEST_F(ParserScopeValidationTest, ClearRemovesCollections) {
    scope_context_.registerCollection("users");
    scope_context_.registerCollection("posts");
    EXPECT_TRUE(scope_context_.isCollectionInScope("users"));
    EXPECT_TRUE(scope_context_.isCollectionInScope("posts"));

    scope_context_.clear();
    EXPECT_FALSE(scope_context_.isCollectionInScope("users"));
    EXPECT_FALSE(scope_context_.isCollectionInScope("posts"));
}

/**
 * Test: Push and pop scope operations
 */
TEST_F(ParserScopeValidationTest, PushPopScope) {
    scope_context_.registerCollection("users");
    EXPECT_TRUE(scope_context_.isCollectionInScope("users"));

    // Push new scope
    scope_context_.pushScope();
    scope_context_.registerCollection("posts");
    EXPECT_TRUE(scope_context_.isCollectionInScope("users"));
    EXPECT_TRUE(scope_context_.isCollectionInScope("posts"));

    // Pop scope
    scope_context_.popScope();
    EXPECT_TRUE(scope_context_.isCollectionInScope("users"));
    // posts was registered only in the nested scope
    EXPECT_FALSE(scope_context_.isCollectionInScope("posts"));
}

/**
 * Test: Nested scopes are properly isolated
 */
TEST_F(ParserScopeValidationTest, NestedScopesIsolated) {
    scope_context_.registerCollection("outer_collection");

    scope_context_.pushScope();
    {
        scope_context_.registerCollection("inner_collection");
        EXPECT_TRUE(scope_context_.isCollectionInScope("outer_collection"));
        EXPECT_TRUE(scope_context_.isCollectionInScope("inner_collection"));
    }
    scope_context_.popScope();

    // After pop, inner_collection should not be accessible
    EXPECT_TRUE(scope_context_.isCollectionInScope("outer_collection"));
    EXPECT_FALSE(scope_context_.isCollectionInScope("inner_collection"));
}

/**
 * Test: getRegisteredCollections returns all registered collections
 */
TEST_F(ParserScopeValidationTest, GetRegisteredCollectionsReturnsAll) {
    scope_context_.registerCollection("users");
    scope_context_.registerCollection("posts");
    scope_context_.registerCollection("comments");

    const auto& collections = scope_context_.getRegisteredCollections();
    EXPECT_EQ(collections.size(), 3);
    EXPECT_NE(collections.find("users"), collections.end());
    EXPECT_NE(collections.find("posts"), collections.end());
    EXPECT_NE(collections.find("comments"), collections.end());
}

// ============================================================================
// Integration Tests with AQLParser
// ============================================================================

/**
 * Test: Valid FOR clause with single collection
 */
TEST_F(ParserScopeValidationTest, ParseValidForClause) {
    std::string aql = "FOR doc IN users RETURN doc";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
    auto query = result.ok_value();
    EXPECT_NE(query, nullptr);
}

/**
 * Test: Valid FOR clause with multiple collections
 */
TEST_F(ParserScopeValidationTest, ParseMultipleForClauses) {
    std::string aql = "FOR u IN users FOR p IN posts FILTER u._key == p.user_id RETURN {u, p}";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Valid INSERT statement with collection name
 */
TEST_F(ParserScopeValidationTest, ParseValidInsertStatement) {
    std::string aql = "INSERT {name: 'John'} INTO users";
    auto result = parser_.parseMutation(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Valid UPDATE statement with collection name
 */
TEST_F(ParserScopeValidationTest, ParseValidUpdateStatement) {
    std::string aql = "FOR doc IN users UPDATE doc SET doc.status = 'active' IN users";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Valid REMOVE statement with collection name
 */
TEST_F(ParserScopeValidationTest, ParseValidRemoveStatement) {
    std::string aql = "FOR doc IN users REMOVE doc IN users";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Valid DELETE statement (SQL-style alias for REMOVE)
 */
TEST_F(ParserScopeValidationTest, ParseValidDeleteStatement) {
    std::string aql = "DELETE FROM users WHERE status == 'inactive'";
    auto result = parser_.parseMutation(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Valid graph traversal uses synthetic "graph" collection
 */
TEST_F(ParserScopeValidationTest, ParseValidGraphTraversal) {
    std::string aql = "FOR v, e, p IN 1..3 OUTBOUND 'users/1' GRAPH 'social' RETURN v";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Collection name with valid identifier characters
 */
TEST_F(ParserScopeValidationTest, CollectionNameWithValidCharacters) {
    std::string aql = "FOR doc IN user_profiles_v2 RETURN doc";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Collection name can start with underscore
 */
TEST_F(ParserScopeValidationTest, CollectionNameStartingWithUnderscore) {
    std::string aql = "FOR doc IN _internal_users RETURN doc";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Collection name can be uppercase
 */
TEST_F(ParserScopeValidationTest, CollectionNameWithUppercase) {
    std::string aql = "FOR doc IN USERS RETURN doc";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Collection name can contain numbers
 */
TEST_F(ParserScopeValidationTest, CollectionNameWithNumbers) {
    std::string aql = "FOR doc IN users2023 RETURN doc";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Multiple scopes in nested FOR statements
 */
TEST_F(ParserScopeValidationTest, NestedForScopesMultipleCollections) {
    std::string aql = "FOR u IN users "
                     "FOR p IN posts "
                     "FOR c IN comments "
                     "FILTER u._key == p.user_id AND p._key == c.post_id "
                     "RETURN {u, p, c}";
    auto result = parser_.parse(aql);
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

/**
 * Test: Contextual keywords as collection names
 */
TEST_F(ParserScopeValidationTest, ContextualKeywordAsCollectionName) {
    // Keywords like INSERT, UPDATE, DELETE can be collection names in some contexts
    std::string aql = "FOR doc IN INSERT RETURN doc";
    auto result = parser_.parse(aql);
    // This should parse successfully since INSERT is allowed as collection name
    EXPECT_TRUE(result.is_ok()) << "Parse failed: " << result.err_message();
}

// ============================================================================
// Continuous Query Planner Scope Tests
// ============================================================================

/**
 * Test: ContinuousQueryPlanner validates source_collection name
 */
TEST_F(ParserScopeValidationTest, ContinuousQueryValidSourceCollection) {
    GTEST_SKIP() << "Placeholder: requires ContinuousQueryPlanner API to be testable";
}

/**
 * Test: ContinuousQueryPlanner rejects empty source_collection
 */
TEST_F(ParserScopeValidationTest, ContinuousQueryRejectsEmptyCollection) {
    GTEST_SKIP() << "Placeholder: requires ContinuousQueryPlanner API to be testable";
}

/**
 * Test: ContinuousQueryPlanner validates collection name format
 */
TEST_F(ParserScopeValidationTest, ContinuousQueryValidatesCollectionFormat) {
    GTEST_SKIP() << "Placeholder: requires ContinuousQueryPlanner API to be testable";
}

// ============================================================================
// Property-Based Tests for Scope Enforcement
// ============================================================================

/**
 * Test: Registered collections are always valid
 */
TEST_F(ParserScopeValidationTest, PropertyRegisteredCollectionsValid) {
    const std::vector<std::string> collections = {
        "users", "posts", "comments", "likes",
        "_internal", "user_profile_v2", "DATA_2023"
    };

    for (const auto& coll : collections) {
        scope_context_.registerCollection(coll);
    }

    for (const auto& coll : collections) {
        auto result = scope_context_.validateCollectionAccess(coll, "TEST");
        EXPECT_TRUE(result.is_ok())
            << "Collection '" << coll << "' should be valid after registration";
    }
}

/**
 * Test: Unregistered collections are always invalid
 */
TEST_F(ParserScopeValidationTest, PropertyUnregisteredCollectionsInvalid) {
    scope_context_.registerCollection("users");

    const std::vector<std::string> unregistered = {
        "posts", "comments", "likes", "unknown"
    };

    for (const auto& coll : unregistered) {
        auto result = scope_context_.validateCollectionAccess(coll, "TEST");
        EXPECT_FALSE(result.is_ok())
            << "Unregistered collection '" << coll << "' should be invalid";
    }
}

/**
 * Test: Scope isolation prevents collection leakage between levels
 */
TEST_F(ParserScopeValidationTest, PropertyScopeIsolation) {
    scope_context_.registerCollection("outer");
    scope_context_.pushScope();
    scope_context_.registerCollection("inner");
    scope_context_.popScope();

    // After pop, inner should not be accessible
    auto result = scope_context_.validateCollectionAccess("inner", "TEST");
    EXPECT_FALSE(result.is_ok())
        << "Inner scope collection should not be accessible after pop";

    // Outer should still be accessible
    result = scope_context_.validateCollectionAccess("outer", "TEST");
    EXPECT_TRUE(result.is_ok())
        << "Outer scope collection should still be accessible after pop";
}

/**
 * Test: Scope idempotency - multiple registrations have same effect as one
 */
TEST_F(ParserScopeValidationTest, PropertyScopeIdempotency) {
    scope_context_.registerCollection("users");
    scope_context_.registerCollection("users");
    scope_context_.registerCollection("users");

    auto result = scope_context_.validateCollectionAccess("users", "TEST");
    EXPECT_TRUE(result.is_ok())
        << "Multiple registrations should result in valid collection";
}

} // namespace query
} // namespace themis
