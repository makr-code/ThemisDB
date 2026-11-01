#include <gtest/gtest.h>

#include "query/aql_parser.h"

using namespace themis::query;

TEST(AqlFtsPhraseProximityTest, ParsesPhrasePredicateIntoSearchClause) {
    AQLParser parser;
    const auto result = parser.parse(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hello world") RETURN doc)");

    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);
    ASSERT_NE(result.value()->search_clause, nullptr);
    ASSERT_EQ(result.value()->search_clause->predicates.size(), 1U);

    const auto& predicate = result.value()->search_clause->predicates.front();
    EXPECT_EQ(predicate.pred_type, FtsPredType::PHRASE);
    EXPECT_EQ(predicate.field, "doc");
    EXPECT_EQ(predicate.term, "hello world");
}

TEST(AqlFtsPhraseProximityTest, ParsesNearDistanceIntoProximityPredicate) {
    AQLParser parser;
    const auto result = parser.parse(
        R"(FOR doc IN articles SEARCH NEAR[5](doc, "graph") RETURN doc)");

    ASSERT_TRUE(result.has_value());
    ASSERT_NE(result.value(), nullptr);
    ASSERT_NE(result.value()->search_clause, nullptr);
    ASSERT_EQ(result.value()->search_clause->predicates.size(), 1U);

    const auto& predicate = result.value()->search_clause->predicates.front();
    EXPECT_EQ(predicate.pred_type, FtsPredType::PROXIMITY);
    EXPECT_EQ(predicate.field, "doc");
    EXPECT_EQ(predicate.term, "graph");
    EXPECT_EQ(predicate.proximity_distance, 5U);
}

TEST(AqlFtsPhraseProximityTest, RejectsMalformedPhrasePredicate) {
    AQLParser parser;
    const auto result = parser.parse(
        R"(FOR doc IN articles SEARCH PHRASE(doc "hello world") RETURN doc)");

    EXPECT_FALSE(result.has_value());
}
