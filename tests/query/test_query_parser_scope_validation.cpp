/**
 * @file test_query_parser_scope_validation.cpp
 * @brief Contract tests for parser scope-validation integration.
 */

#include <gtest/gtest.h>

#include "query/aql_parser.h"

using namespace themis;
using namespace themis::query;

TEST(QueryParserScopeValidationContract, ParseSimpleQuery) {
    AQLParser parser;
    auto result = parser.parse("FOR doc IN users RETURN doc");
    EXPECT_TRUE(result.has_value());
}

TEST(QueryParserScopeValidationContract, RejectMalformedQuery) {
    AQLParser parser;
    auto result = parser.parse("FOR doc users RETURN");
    EXPECT_FALSE(result.has_value());
}

TEST(QueryParserScopeValidationContract, RejectUnterminatedString) {
    AQLParser parser;
    auto result = parser.parse("FOR doc IN users FILTER doc.name == \"oops RETURN doc");
    EXPECT_FALSE(result.has_value());
}
