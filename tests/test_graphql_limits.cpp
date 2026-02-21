/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_graphql_limits.cpp                            ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:35:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
