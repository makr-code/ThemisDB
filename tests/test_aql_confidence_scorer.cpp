/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_aql_confidence_scorer.cpp                     ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 16:53:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     241                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_aql_confidence_scorer.cpp
 * @brief Unit tests for AQLConfidenceScorer
 */

#include <gtest/gtest.h>
#include "aql/aql_confidence_scorer.h"

using namespace themis::aql;

class AQLConfidenceScorerTest : public ::testing::Test {
protected:
    AQLConfidenceScorer scorer;
};

// ============================================================================
// Empty / trivial input
// ============================================================================

TEST_F(AQLConfidenceScorerTest, EmptyQueryReturnsZeroConfidence) {
    auto result = scorer.score("");
    EXPECT_FLOAT_EQ(result.overall_confidence, 0.0f);
    EXPECT_FLOAT_EQ(result.structural_score, 0.0f);
    EXPECT_FALSE(result.has_required_keywords);
    EXPECT_FALSE(result.reasoning.empty());
}

// ============================================================================
// Structural score
// ============================================================================

TEST_F(AQLConfidenceScorerTest, MinimalValidQueryScoresHigh) {
    // Minimal valid AQL: FOR + IN + RETURN
    const std::string aql = "FOR u IN users RETURN u";
    auto result = scorer.score(aql);

    EXPECT_GT(result.overall_confidence, 0.5f);
    EXPECT_FLOAT_EQ(result.structural_score, 1.0f);
    EXPECT_TRUE(result.has_required_keywords);
}

TEST_F(AQLConfidenceScorerTest, QueryWithoutReturnHasLowStructuralScore) {
    const std::string aql = "FOR u IN users";
    auto result = scorer.score(aql);

    EXPECT_LT(result.structural_score, 0.5f);
    EXPECT_FALSE(result.has_required_keywords);
}

TEST_F(AQLConfidenceScorerTest, QueryWithoutForHasLowStructuralScore) {
    const std::string aql = "RETURN 42";
    auto result = scorer.score(aql);

    EXPECT_LT(result.structural_score, 0.5f);
    EXPECT_FALSE(result.has_required_keywords);
}

TEST_F(AQLConfidenceScorerTest, ForReturnWithoutInHasReducedStructuralScore) {
    // FOR + RETURN but no IN → slightly lower than 1.0 but still good
    const std::string aql = "FOR u RETURN u";
    auto result = scorer.score(aql);

    EXPECT_GT(result.structural_score, 0.5f);
    EXPECT_LT(result.structural_score, 1.0f);
}

// ============================================================================
// Completeness score
// ============================================================================

TEST_F(AQLConfidenceScorerTest, QueryWithFilterScoresHigherCompleteness) {
    const std::string simple = "FOR u IN users RETURN u";
    const std::string filtered = "FOR u IN users FILTER u.city == 'Seattle' RETURN u";

    auto s1 = scorer.score(simple);
    auto s2 = scorer.score(filtered);

    EXPECT_GT(s2.completeness_score, s1.completeness_score);
}

TEST_F(AQLConfidenceScorerTest, QueryWithSortAndLimitScoresHigherCompleteness) {
    const std::string base = "FOR u IN users RETURN u";
    const std::string paged = "FOR u IN users SORT u.name ASC LIMIT 10 RETURN u";

    auto s1 = scorer.score(base);
    auto s2 = scorer.score(paged);

    EXPECT_GT(s2.completeness_score, s1.completeness_score);
}

TEST_F(AQLConfidenceScorerTest, CompletenessIsCapAtOne) {
    // Even a very keyword-rich query must not exceed 1.0
    const std::string aql =
        "FOR u IN users "
        "FILTER u.age > 18 "
        "LET posts = (FOR p IN posts FILTER p.author == u._id RETURN p) "
        "SORT u.name ASC "
        "LIMIT 100 "
        "COLLECT city = u.city "
        "RETURN {city, count: LENGTH(posts)}";
    auto result = scorer.score(aql);
    EXPECT_LE(result.completeness_score, 1.0f);
}

// ============================================================================
// Schema match score
// ============================================================================

TEST_F(AQLConfidenceScorerTest, NoSchemaGivesNeutralSchemaScore) {
    const std::string aql = "FOR u IN users RETURN u";
    auto result = scorer.score(aql, "", "");
    EXPECT_FLOAT_EQ(result.schema_match_score, 0.5f);
}

TEST_F(AQLConfidenceScorerTest, MatchingCollectionGivesHighSchemaScore) {
    const std::string schema = R"(
Collections:
- users: {name, email, city}
- posts: {title, content}
)";
    const std::string aql = "FOR u IN users RETURN u";
    auto result = scorer.score(aql, "", schema);

    EXPECT_GT(result.schema_match_score, 0.5f);
}

TEST_F(AQLConfidenceScorerTest, NoCollectionMatchGivesLowSchemaScore) {
    const std::string schema = R"(
Collections:
- users: {name, email}
- posts: {title}
)";
    const std::string aql = "FOR x IN unknown_collection RETURN x";
    auto result = scorer.score(aql, "", schema);

    EXPECT_LT(result.schema_match_score, 0.5f);
}

TEST_F(AQLConfidenceScorerTest, AllCollectionsMatchedGivesFullSchemaScore) {
    const std::string schema = R"(
Collections:
- users: {name}
- posts: {title}
)";
    const std::string aql =
        "FOR u IN users "
        "LET p = (FOR post IN posts FILTER post.author == u._id RETURN post) "
        "RETURN {u, p}";
    auto result = scorer.score(aql, "", schema);

    EXPECT_FLOAT_EQ(result.schema_match_score, 1.0f);
}

// ============================================================================
// Overall confidence
// ============================================================================

TEST_F(AQLConfidenceScorerTest, OverallConfidenceIsBoundedBetweenZeroAndOne) {
    const std::string aql = "FOR u IN users FILTER u.city == 'NYC' RETURN u";
    auto result = scorer.score(aql);
    EXPECT_GE(result.overall_confidence, 0.0f);
    EXPECT_LE(result.overall_confidence, 1.0f);
}

TEST_F(AQLConfidenceScorerTest, WellFormedQueryWithSchemaHasHighOverallConfidence) {
    const std::string schema = R"(
Collections:
- orders: {customer_id, total}
- customers: {name, email}
)";
    const std::string aql =
        "FOR o IN orders "
        "LET c = DOCUMENT('customers', o.customer_id) "
        "FILTER o.total > 100 "
        "SORT o.total DESC "
        "LIMIT 50 "
        "RETURN {order: o, customer: c}";
    auto result = scorer.score(aql, "Find large orders with customer info", schema);

    EXPECT_GT(result.overall_confidence, 0.7f);
    EXPECT_TRUE(result.has_required_keywords);
}

// ============================================================================
// Reasoning field
// ============================================================================

TEST_F(AQLConfidenceScorerTest, ReasoningIsNonEmptyForValidQuery) {
    const std::string aql = "FOR u IN users RETURN u";
    auto result = scorer.score(aql);
    EXPECT_FALSE(result.reasoning.empty());
}

TEST_F(AQLConfidenceScorerTest, MissingRequiredKeywordsWarningInReasoning) {
    const std::string aql = "SOME RANDOM TEXT";
    auto result = scorer.score(aql);
    // Reasoning should contain warning about missing keywords
    EXPECT_NE(result.reasoning.find("FOR"), std::string::npos);
}

// ============================================================================
// Case insensitivity
// ============================================================================

TEST_F(AQLConfidenceScorerTest, ScoringIsCaseInsensitive) {
    const std::string upper = "FOR u IN users RETURN u";
    const std::string lower = "for u in users return u";

    auto r1 = scorer.score(upper);
    auto r2 = scorer.score(lower);

    EXPECT_FLOAT_EQ(r1.overall_confidence, r2.overall_confidence);
    EXPECT_FLOAT_EQ(r1.structural_score,   r2.structural_score);
    EXPECT_EQ(r1.has_required_keywords,    r2.has_required_keywords);
}
