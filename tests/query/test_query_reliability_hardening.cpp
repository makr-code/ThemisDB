/**
 * @file test_query_reliability_hardening.cpp
 * @brief Contract tests for parser reliability hardening invariants.
 */

#include <gtest/gtest.h>

#include "query/aql_parser.h"

using namespace themis;
using namespace themis::query;

class QueryReliabilityContractTest : public ::testing::Test {
protected:
    AQLParser parser_;
};

TEST_F(QueryReliabilityContractTest, ValidQueryParsesSuccessfully) {
    auto result = parser_.parse("FOR doc IN documents RETURN doc");
    EXPECT_TRUE(result.has_value());
}

TEST_F(QueryReliabilityContractTest, InvalidQueryReturnsError) {
    auto result = parser_.parse("RETURN 1 WHERE x > 5");
    EXPECT_FALSE(result.has_value());
}

TEST_F(QueryReliabilityContractTest, UnterminatedStringReturnsError) {
    auto result = parser_.parse("FOR doc IN documents FILTER doc.name == \"unterminated RETURN doc");
    EXPECT_FALSE(result.has_value());
}
