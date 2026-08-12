#include <gtest/gtest.h>
#include "api/graphql.h"
#include <string>

using namespace themis::graphql;

// ============================================================================
// Query Size Limit Tests
// ============================================================================

TEST(GraphQLLimits, QuerySizeExceedsLimit) {
    QueryLimits limits;
    limits.max_query_size_bytes = 100;  // Very small limit for testing
    
    std::string largeQuery(200, 'a');  // Create a 200-byte query
    largeQuery = "{ " + largeQuery + " }";
    
    auto result = Parser::parse(largeQuery, limits);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].message.find("Query size exceeds maximum") != std::string::npos);
}

TEST(GraphQLLimits, QuerySizeWithinLimit) {
    QueryLimits limits;
    limits.max_query_size_bytes = 1000;
    
    std::string query = "{ user { id name } }";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query should parse successfully within size limit";
    EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// Depth Limit Tests
// ============================================================================

TEST(GraphQLLimits, DepthExceedsLimit) {
    QueryLimits limits;
    limits.max_depth = 3;
    
    // Create a deeply nested query (depth 5)
    std::string query = R"(
        {
            user {
                posts {
                    comments {
                        author {
                            profile {
                                bio
                            }
                        }
                    }
                }
            }
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].message.find("depth") != std::string::npos || 
                result.errors[0].message.find("Depth") != std::string::npos);
}

TEST(GraphQLLimits, DepthWithinLimit) {
    QueryLimits limits;
    limits.max_depth = 5;
    
    // Create a query with depth 3
    std::string query = R"(
        {
            user {
                posts {
                    title
                    content
                }
            }
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query should parse successfully within depth limit";
    EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// Field Count Limit Tests
// ============================================================================

TEST(GraphQLLimits, FieldCountExceedsLimit) {
    QueryLimits limits;
    limits.max_fields = 5;
    
    // Create a query with 8 fields
    std::string query = R"(
        {
            field1
            field2
            field3
            field4
            field5
            field6
            field7
            field8
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].message.find("field") != std::string::npos || 
                result.errors[0].message.find("Field") != std::string::npos);
}

TEST(GraphQLLimits, FieldCountWithinLimit) {
    QueryLimits limits;
    limits.max_fields = 10;
    
    std::string query = R"(
        {
            field1
            field2
            field3
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query should parse successfully within field limit";
    EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// AST Node Limit Tests
// ============================================================================

TEST(GraphQLLimits, ASTNodeCountExceedsLimit) {
    QueryLimits limits;
    limits.max_ast_nodes = 10;
    
    // Create a complex query with many AST nodes
    std::string query = R"(
        query GetUser($id: ID!) {
            user(id: $id) {
                id
                name
                email
                posts {
                    id
                    title
                    comments {
                        id
                        text
                    }
                }
            }
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].message.find("AST") != std::string::npos || 
                result.errors[0].message.find("node") != std::string::npos);
}

TEST(GraphQLLimits, ASTNodeCountWithinLimit) {
    QueryLimits limits;
    limits.max_ast_nodes = 100;
    
    std::string query = "{ user { id name } }";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query should parse successfully within AST node limit";
    EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// Combined Limits Tests
// ============================================================================

TEST(GraphQLLimits, DefaultLimitsAllowReasonableQueries) {
    QueryLimits limits = QueryLimits::defaults();
    
    std::string query = R"(
        query GetUserData {
            user(id: "123") {
                id
                name
                email
                posts(limit: 10) {
                    id
                    title
                    content
                    createdAt
                    author {
                        id
                        name
                    }
                }
            }
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query should parse successfully with default limits";
    EXPECT_TRUE(result.errors.empty());
}

TEST(GraphQLLimits, PermissiveLimitsAllowComplexQueries) {
    QueryLimits limits = QueryLimits::permissive();
    
    // Create a moderately complex query
    std::string query = R"(
        query GetComplexData {
            users {
                id
                name
                email
                profile {
                    bio
                    avatar
                    settings {
                        theme
                        notifications {
                            email
                            push
                        }
                    }
                }
                posts {
                    id
                    title
                    content
                    tags {
                        name
                    }
                }
            }
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query should parse successfully with permissive limits";
    EXPECT_TRUE(result.errors.empty());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(GraphQLLimits, EmptyQueryHandling) {
    QueryLimits limits = QueryLimits::defaults();
    std::string query = "";
    
    auto result = Parser::parse(query, limits);
    // Empty query should fail parsing but not crash
    EXPECT_FALSE(result.success);
}

TEST(GraphQLLimits, MinimalValidQuery) {
    QueryLimits limits = QueryLimits::defaults();
    std::string query = "{ __typename }";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success);
}

TEST(GraphQLLimits, NestedFieldsCountedCorrectly) {
    QueryLimits limits;
    limits.max_fields = 5;
    
    // This has exactly 5 fields: user, posts, id, title, author
    std::string query = R"(
        {
            user {
                posts {
                    id
                    title
                    author
                }
            }
        }
    )";
    
    auto result = Parser::parse(query, limits);
    EXPECT_TRUE(result.success) << "Query with exactly max_fields should succeed";
}


// ============================================================================
// Introspection Limits Tests
// ============================================================================

TEST(GraphQLLimits, IntrospectionAllowedByDefault) {
    QueryLimits limits;  // allow_introspection = true by default

    // __schema is allowed
    auto r1 = Parser::parse("{ __schema { queryType { name } } }", limits);
    EXPECT_TRUE(r1.success) << "Introspection should succeed when allow_introspection=true";

    // __type is allowed
    auto r2 = Parser::parse("{ __type(name: \"Query\") { name } }", limits);
    EXPECT_TRUE(r2.success) << "__type should succeed when allow_introspection=true";

    // __typename is allowed
    auto r3 = Parser::parse("{ user { __typename id } }", limits);
    EXPECT_TRUE(r3.success) << "__typename should succeed when allow_introspection=true";
}

TEST(GraphQLLimits, IntrospectionBlockedWhenDisabled) {
    QueryLimits limits;
    limits.allow_introspection = false;

    // __schema rejected
    auto r1 = Parser::parse("{ __schema { queryType { name } } }", limits);
    EXPECT_FALSE(r1.success) << "__schema should be rejected when allow_introspection=false";
    ASSERT_FALSE(r1.errors.empty());
    EXPECT_NE(r1.errors[0].message.find("introspection"), std::string::npos)
        << "Error message should mention introspection";

    // __type rejected
    auto r2 = Parser::parse("{ __type(name: \"Query\") { name } }", limits);
    EXPECT_FALSE(r2.success) << "__type should be rejected when allow_introspection=false";

    // __typename rejected
    auto r3 = Parser::parse("{ user { __typename id } }", limits);
    EXPECT_FALSE(r3.success) << "__typename should be rejected when allow_introspection=false";
}

TEST(GraphQLLimits, NonIntrospectionQueryAllowedWhenIntrospectionDisabled) {
    QueryLimits limits;
    limits.allow_introspection = false;

    auto result = Parser::parse("{ user { id name email } }", limits);
    EXPECT_TRUE(result.success) << "Regular query should still succeed even when introspection is disabled";
    EXPECT_TRUE(result.errors.empty());
}

TEST(GraphQLLimits, ProductionLimitsDisableIntrospection) {
    auto limits = QueryLimits::production();
    EXPECT_FALSE(limits.allow_introspection)
        << "production() limits must disable introspection";

    // Verify __schema is rejected
    auto result = Parser::parse("{ __schema { types { name } } }", limits);
    EXPECT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_NE(result.errors[0].message.find("introspection"), std::string::npos);
}

TEST(GraphQLLimits, DefaultsAllowIntrospection) {
    auto limits = QueryLimits::defaults();
    EXPECT_TRUE(limits.allow_introspection)
        << "defaults() should allow introspection (development context)";
}

TEST(GraphQLLimits, PermissiveAllowsIntrospection) {
    auto limits = QueryLimits::permissive();
    EXPECT_TRUE(limits.allow_introspection)
        << "permissive() should allow introspection (trusted context)";
}

TEST(GraphQLLimits, NestedIntrospectionBlockedWhenDisabled) {
    QueryLimits limits;
    limits.allow_introspection = false;

    // Introspection nested inside a mutation
    auto result = Parser::parse(
        "mutation { createUser(name: \"alice\") { id __typename } }", limits);
    EXPECT_FALSE(result.success) << "Nested __typename should be rejected when disabled";
}

TEST(GraphQLLimits, IntrospectionErrorMessageContainsFieldName) {
    QueryLimits limits;
    limits.allow_introspection = false;

    auto result = Parser::parse("{ __schema { types { name } } }", limits);
    ASSERT_FALSE(result.success);
    ASSERT_FALSE(result.errors.empty());
    // Error must identify the offending field name
    EXPECT_NE(result.errors[0].message.find("__schema"), std::string::npos)
        << "Error message should contain the offending field name '__schema'";
}
