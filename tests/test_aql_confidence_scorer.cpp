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

// ============================================================================
// Config injection (runtime-configurable weights)
// ============================================================================

TEST(AQLConfidenceScorerConfigTest, DefaultConfigMatchesHardCodedBehaviour) {
    AQLConfidenceScorer default_scorer;
    AQLConfidenceScorer config_scorer{AQLConfidenceScorer::Config{}};

    const std::string aql = "FOR u IN users FILTER u.age > 18 RETURN u";
    auto r1 = default_scorer.score(aql);
    auto r2 = config_scorer.score(aql);

    EXPECT_FLOAT_EQ(r1.overall_confidence, r2.overall_confidence);
    EXPECT_FLOAT_EQ(r1.structural_score,   r2.structural_score);
    EXPECT_FLOAT_EQ(r1.completeness_score, r2.completeness_score);
    EXPECT_FLOAT_EQ(r1.schema_match_score, r2.schema_match_score);
}

TEST(AQLConfidenceScorerConfigTest, CustomWeightsAreApplied) {
    AQLConfidenceScorer::Config cfg;
    cfg.structural_weight   = 1.0f;
    cfg.completeness_weight = 0.0f;
    cfg.schema_match_weight = 0.0f;

    AQLConfidenceScorer scorer{cfg};
    const std::string aql = "FOR u IN users RETURN u";
    auto result = scorer.score(aql);

    // With structural_weight=1, the overall score must equal structural_score
    EXPECT_FLOAT_EQ(result.overall_confidence, result.structural_score);
}

TEST(AQLConfidenceScorerConfigTest, CustomNoSchemaNeutralIsUsed) {
    AQLConfidenceScorer::Config cfg;
    cfg.no_schema_neutral = 0.8f;
    AQLConfidenceScorer scorer{cfg};

    auto result = scorer.score("FOR u IN users RETURN u", "", "");
    EXPECT_FLOAT_EQ(result.schema_match_score, 0.8f);
}

TEST(AQLConfidenceScorerConfigTest, CustomZeroMatchFloorIsUsed) {
    AQLConfidenceScorer::Config cfg;
    cfg.zero_match_floor = 0.05f;
    AQLConfidenceScorer scorer{cfg};

    const std::string schema = "- known_col: {field}";
    auto result = scorer.score("FOR x IN unknown RETURN x", "", schema);
    EXPECT_FLOAT_EQ(result.schema_match_score, 0.05f);
}

TEST(AQLConfidenceScorerConfigTest, CustomKeywordBonusesAreUsed) {
    AQLConfidenceScorer::Config cfg;
    cfg.keyword_bonuses = {{"filter", 0.50f}};
    AQLConfidenceScorer scorer{cfg};

    auto without_filter = scorer.score("FOR u IN users RETURN u");
    auto with_filter    = scorer.score("FOR u IN users FILTER u.x == 1 RETURN u");

    EXPECT_GT(with_filter.completeness_score, without_filter.completeness_score);
}

TEST(AQLConfidenceScorerConfigTest, ConfigAccessorReturnsCurrentConfig) {
    AQLConfidenceScorer::Config cfg;
    cfg.structural_weight = 0.7f;
    AQLConfidenceScorer scorer{cfg};

    EXPECT_FLOAT_EQ(scorer.config().structural_weight, 0.7f);
}

// ============================================================================
// Whole-word keyword matching (substring false-positive fix)
// ============================================================================

TEST(AQLConfidenceScorerKeywordTest, UpsertDoesNotTriggerInsertBonus) {
    // "upsert" contains "insert" as a substring; it must NOT trigger the
    // "insert" bonus.  Only "upsert" should be credited.
    AQLConfidenceScorer::Config cfg;
    cfg.keyword_bonuses = {{"insert", 0.10f}, {"upsert", 0.10f}};
    AQLConfidenceScorer scorer{cfg};

    // Query contains "upsert" but NOT standalone "insert"
    const std::string upsert_query = "UPSERT {_key: '1'} INSERT {_key: '1', x: 1} UPDATE {x: 2} IN col";
    // The above actually contains both standalone INSERT and UPSERT; use a
    // simpler synthetic string that only has "upsert".
    const std::string only_upsert  = "for x in col upsert x return x";
    auto result = scorer.score(only_upsert);

    // "insert" is NOT present as a whole word → its bonus must not be added
    // Base (0.40) + upsert bonus (0.10) = 0.50
    EXPECT_FLOAT_EQ(result.completeness_score, std::min(0.40f + 0.10f, 1.0f));
}

TEST(AQLConfidenceScorerKeywordTest, StandaloneInsertIsMatched) {
    AQLConfidenceScorer::Config cfg;
    cfg.keyword_bonuses = {{"insert", 0.10f}};
    AQLConfidenceScorer scorer{cfg};

    const std::string query = "insert {_key: '1', name: 'Alice'} into users";
    auto result = scorer.score(query);
    // Base (0.40) + insert (0.10)
    EXPECT_FLOAT_EQ(result.completeness_score, std::min(0.40f + 0.10f, 1.0f));
}

// ============================================================================
// Acceptance-criteria tests (as per issue)
// ============================================================================

TEST(AQLConfidenceScorerAcceptanceTest, EmptyQueryReturnsExactlyZero) {
    AQLConfidenceScorer scorer;
    auto result = scorer.score("");
    EXPECT_FLOAT_EQ(result.overall_confidence, 0.0f);
}

TEST(AQLConfidenceScorerAcceptanceTest, CompleteQueryReturnsAbove0Point7) {
    AQLConfidenceScorer scorer;
    // Canonical complete query from issue acceptance criteria
    auto result = scorer.score("FOR x IN c FILTER x.a == 1 RETURN x");
    EXPECT_GT(result.overall_confidence, 0.7f);
}

// ============================================================================
// calibrate() method
// ============================================================================

TEST(AQLConfidenceScorerCalibrateTest, CalibrateDoesNotCrashOnEmptyInput) {
    AQLConfidenceScorer scorer;
    EXPECT_NO_THROW(scorer.calibrate({}));
}

TEST(AQLConfidenceScorerCalibrateTest, CalibrateIgnoresEmptyQueryPairs) {
    AQLConfidenceScorer scorer;
    std::vector<std::pair<std::string, float>> pairs = {{"", 0.9f}, {"", 0.5f}};
    auto before = scorer.config().structural_weight;
    scorer.calibrate(pairs);
    EXPECT_FLOAT_EQ(scorer.config().structural_weight, before);
}

TEST(AQLConfidenceScorerCalibrateTest, CalibrateWeightsSumToOne) {
    AQLConfidenceScorer scorer;

    std::vector<std::pair<std::string, float>> pairs = {
        {"FOR x IN c FILTER x.a == 1 RETURN x",          0.95f},
        {"FOR u IN users RETURN u",                       0.80f},
        {"FOR u IN users SORT u.name LIMIT 10 RETURN u",  0.85f},
        {"RETURN 42",                                     0.10f},
        {"FOR x IN c RETURN x",                          0.75f},
    };

    scorer.calibrate(pairs);

    float sum = scorer.config().structural_weight
              + scorer.config().completeness_weight
              + scorer.config().schema_match_weight;
    EXPECT_NEAR(sum, 1.0f, 1e-5f);
}

TEST(AQLConfidenceScorerCalibrateTest, CalibrateWeightsAreInZeroOneRange) {
    AQLConfidenceScorer scorer;

    std::vector<std::pair<std::string, float>> pairs = {
        {"FOR x IN c FILTER x.a == 1 RETURN x",          0.95f},
        {"FOR u IN users RETURN u",                       0.80f},
        {"FOR u IN users SORT u.name LIMIT 10 RETURN u",  0.85f},
    };

    scorer.calibrate(pairs);

    EXPECT_GE(scorer.config().structural_weight,   0.0f);
    EXPECT_LE(scorer.config().structural_weight,   1.0f);
    EXPECT_GE(scorer.config().completeness_weight, 0.0f);
    EXPECT_LE(scorer.config().completeness_weight, 1.0f);
    EXPECT_GE(scorer.config().schema_match_weight, 0.0f);
    EXPECT_LE(scorer.config().schema_match_weight, 1.0f);
}
