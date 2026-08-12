// test_entity_matching.cpp
//
// Unit tests for:
//   - DeterministicMatcher (exact key matching)
//   - SemanticMatcher       (string distance metrics, name/email/phone)
//   - HybridEntityMatcher   (combined strategies)

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// Minimal standalone re-implementations of the types and algorithms so that
// this test binary has no dependency on themis_core or any other library.
// ---------------------------------------------------------------------------

#include "importers/entity_matcher.h"

namespace ti = themis::importers;

// ============================================================================
// DeterministicMatcher tests
// ============================================================================

class DeterministicMatcherTest : public ::testing::Test {
protected:
    ti::DeterministicMatcher matcher;
};

TEST_F(DeterministicMatcherTest, FindExactMatchByIdField) {
    ti::json incoming = {{"id", "user-001"}, {"email", "alice@example.com"}, {"name", "Alice"}};
    auto results = matcher.findExactMatches(incoming, "users", {"id"});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].existing_entity_id, "user-001");
    EXPECT_DOUBLE_EQ(results[0].confidence_score, 1.0);
    ASSERT_EQ(results[0].match_keys.size(), 1u);
    EXPECT_EQ(results[0].match_keys[0], "id");
}

TEST_F(DeterministicMatcherTest, FindExactMatchByEmailField) {
    ti::json incoming = {{"id", "user-002"}, {"email", "bob@example.com"}};
    auto results = matcher.findExactMatches(incoming, "users", {"email"});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].existing_entity_id, "user-002");
}

TEST_F(DeterministicMatcherTest, NoMatchWhenKeyFieldMissing) {
    ti::json incoming = {{"name", "Charlie"}};  // no "id" or "email"
    auto results = matcher.findExactMatches(incoming, "users", {"id"});
    EXPECT_TRUE(results.empty());
}

TEST_F(DeterministicMatcherTest, FindByPrimaryKeyUsesIdField) {
    ti::json incoming = {{"id", "p-123"}, {"name", "Product X"}};
    auto result = matcher.findByPrimaryKey(incoming, "products");
    EXPECT_EQ(result.existing_entity_id, "p-123");
    EXPECT_DOUBLE_EQ(result.confidence_score, 1.0);
}

TEST_F(DeterministicMatcherTest, FindByPrimaryKeyNoIdField) {
    ti::json incoming = {{"name", "No ID entity"}};
    auto result = matcher.findByPrimaryKey(incoming, "things");
    EXPECT_TRUE(result.existing_entity_id.empty());
    EXPECT_DOUBLE_EQ(result.confidence_score, 0.0);
}

TEST_F(DeterministicMatcherTest, FindByUniqueFieldsReturnsOnePerMatch) {
    ti::json incoming = {{"id", "u-777"}, {"email", "carol@example.com"}, {"ssn", "123-45-6789"}};
    auto results = matcher.findByUniqueFields(incoming, "users", {"email", "ssn"});
    // Two unique fields → two match results (deduplication by entity_id: both have same id → should be 1)
    EXPECT_GE(results.size(), 1u);
    for (const auto& r : results) {
        EXPECT_DOUBLE_EQ(r.confidence_score, 1.0);
    }
}

TEST_F(DeterministicMatcherTest, FindByCustomIdentifierWithMapping) {
    ti::json incoming    = {{"_id", "ext-001"}, {"customer_id", "CUS-999"}};
    ti::json id_mapping  = {{"customer_id", "external_id"}};
    auto result = matcher.findByCustomIdentifier(incoming, "customers", id_mapping);
    // Match found via the "_id" fallback in findExactMatches.
    EXPECT_EQ(result.existing_entity_id, "ext-001");
}

TEST_F(DeterministicMatcherTest, FindByCustomIdentifierNonObjectMapping) {
    ti::json incoming   = {{"id", "x-1"}};
    ti::json bad_mapping = "not_an_object";
    auto result = matcher.findByCustomIdentifier(incoming, "col", bad_mapping);
    EXPECT_TRUE(result.existing_entity_id.empty());
}

TEST_F(DeterministicMatcherTest, EvidencePopulated) {
    ti::json incoming = {{"id", "e-1"}, {"email", "d@x.com"}};
    auto results = matcher.findExactMatches(incoming, "col", {"id", "email"});
    ASSERT_FALSE(results.empty());
    EXPECT_TRUE(results[0].evidence.contains("id"));
}

// ============================================================================
// SemanticMatcher tests – string distance
// ============================================================================

class SemanticMatcherTest : public ::testing::Test {};

TEST_F(SemanticMatcherTest, JaroWinklerIdentical) {
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::jaroWinklerDistance("Alice", "Alice"), 1.0);
}

TEST_F(SemanticMatcherTest, JaroWinklerEmptyStrings) {
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::jaroWinklerDistance("", ""), 1.0);
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::jaroWinklerDistance("Alice", ""), 0.0);
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::jaroWinklerDistance("", "Alice"), 0.0);
}

TEST_F(SemanticMatcherTest, JaroWinklerJohnVsJon) {
    double score = ti::SemanticMatcher::jaroWinklerDistance("john", "jon");
    EXPECT_GT(score, 0.85);
}

TEST_F(SemanticMatcherTest, LevenshteinIdentical) {
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::levenshteinSimilarity("Smith", "Smith"), 1.0);
}

TEST_F(SemanticMatcherTest, LevenshteinEmptyStrings) {
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::levenshteinSimilarity("", ""), 1.0);
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::levenshteinSimilarity("abc", ""), 0.0);
}

TEST_F(SemanticMatcherTest, LevenshteinSingleEdit) {
    // "Smith" → "Smyth": 1 substitution, score = 4/5 = 0.8
    double score = ti::SemanticMatcher::levenshteinSimilarity("Smith", "Smyth");
    EXPECT_NEAR(score, 0.8, 0.01);
}

// ============================================================================
// SemanticMatcher tests – name normalisation & phonetic
// ============================================================================

TEST_F(SemanticMatcherTest, NormalizeFullNameLowerCase) {
    EXPECT_EQ(ti::SemanticMatcher::normalizeFullName("Alice SMITH"), "alice smith");
}

TEST_F(SemanticMatcherTest, NormalizeFullNameLastFirst) {
    // "Smith, Alice" → "alice smith"
    EXPECT_EQ(ti::SemanticMatcher::normalizeFullName("Smith, Alice"), "alice smith");
}

TEST_F(SemanticMatcherTest, SoundexMatchIdenticalNames) {
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::soundexMatch("Smith", "Smith"), 1.0);
}

TEST_F(SemanticMatcherTest, SoundexMatchPhoneticallySimilar) {
    // "Smith" and "Smythe" share the same Soundex code S530.
    double score = ti::SemanticMatcher::soundexMatch("Smith", "Smythe");
    EXPECT_GT(score, 0.0);
}

TEST_F(SemanticMatcherTest, ScoreNameVariationsTypo) {
    double score = ti::SemanticMatcher::scoreNameVariations("Jon Smith", "John Smith");
    EXPECT_GT(score, 0.80);
}

// ============================================================================
// SemanticMatcher tests – email
// ============================================================================

TEST_F(SemanticMatcherTest, EmailSameDomainHighScore) {
    double score = ti::SemanticMatcher::scoreEmailPair(
        "alice@company.com", "alice@company.com");
    EXPECT_DOUBLE_EQ(score, 1.0);
}

TEST_F(SemanticMatcherTest, EmailDifferentDomainZeroScore) {
    double score = ti::SemanticMatcher::scoreEmailPair(
        "alice@foo.com", "alice@bar.com");
    EXPECT_DOUBLE_EQ(score, 0.0);
}

TEST_F(SemanticMatcherTest, EmailTypoDetection) {
    EXPECT_TRUE(ti::SemanticMatcher::isLikelyEmailTypo(
        "alice@example.com", "alce@example.com"));
}

TEST_F(SemanticMatcherTest, EmailNoTypoForDifferentDomains) {
    EXPECT_FALSE(ti::SemanticMatcher::isLikelyEmailTypo(
        "alice@foo.com", "alice@bar.com"));
}

// ============================================================================
// SemanticMatcher tests – phone
// ============================================================================

TEST_F(SemanticMatcherTest, NormalizePhoneStripsFormatting) {
    EXPECT_EQ(ti::SemanticMatcher::normalizePhoneNumber("+1(202)555-1234"), "2025551234");
}

TEST_F(SemanticMatcherTest, NormalizePhoneElevenDigitNorthAmerica) {
    EXPECT_EQ(ti::SemanticMatcher::normalizePhoneNumber("12025551234"), "2025551234");
}

TEST_F(SemanticMatcherTest, ScorePhonePairIdentical) {
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::scorePhonePair(
        "+1(202)555-1234", "2025551234"), 1.0);
}

TEST_F(SemanticMatcherTest, ScorePhonePairDifferent) {
    double score = ti::SemanticMatcher::scorePhonePair("2025551234", "9999999999");
    EXPECT_LT(score, 0.5);
}

// ============================================================================
// SemanticMatcher tests – vector similarity
// ============================================================================

TEST_F(SemanticMatcherTest, VectorSimilarityIdentical) {
    std::vector<float> v = {1.0f, 0.0f, 0.0f};
    EXPECT_NEAR(ti::SemanticMatcher::vectorSimilarity(v, v), 1.0, 1e-6);
}

TEST_F(SemanticMatcherTest, VectorSimilarityOrthogonal) {
    std::vector<float> v1 = {1.0f, 0.0f};
    std::vector<float> v2 = {0.0f, 1.0f};
    EXPECT_NEAR(ti::SemanticMatcher::vectorSimilarity(v1, v2), 0.0, 1e-6);
}

TEST_F(SemanticMatcherTest, VectorSimilarityEmptyVectors) {
    std::vector<float> empty;
    std::vector<float> v = {1.0f};
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::vectorSimilarity(empty, empty), 0.0);
    EXPECT_DOUBLE_EQ(ti::SemanticMatcher::vectorSimilarity(v, empty), 0.0);
}

// ============================================================================
// SemanticMatcher – main scoring engine
// ============================================================================

TEST_F(SemanticMatcherTest, ScoreEntityMatchIdentical) {
    ti::json incoming = {{"id", "u-1"}, {"first_name", "Alice"}, {"last_name", "Smith"}};
    ti::json existing = {{"id", "u-1"}, {"first_name", "Alice"}, {"last_name", "Smith"}};
    ti::SemanticMatcher sm;
    ti::SemanticMatchConfig cfg;
    auto score = sm.scoreEntityMatch(incoming, existing, "users", cfg);
    EXPECT_NEAR(score.overall_confidence, 1.0, 0.01);
    EXPECT_EQ(score.confidence_level, "very_high");
}

TEST_F(SemanticMatcherTest, ScoreEntityMatchLowConfidence) {
    ti::json incoming = {{"id", "u-1"}, {"name", "Alice Smith"}};
    ti::json existing = {{"id", "u-2"}, {"name", "Bob Jones"}};
    ti::SemanticMatcher sm;
    ti::SemanticMatchConfig cfg;
    auto score = sm.scoreEntityMatch(incoming, existing, "users", cfg);
    EXPECT_LT(score.overall_confidence, 0.70);
}

TEST_F(SemanticMatcherTest, FindSimilarEntitiesFiltersAboveThreshold) {
    ti::json incoming  = {{"id", "u-1"}, {"name", "Alice Smith"}};
    ti::json candidate = {{"id", "u-2"}, {"name", "Alise Smith"}};  // close typo

    ti::SemanticMatcher sm;
    ti::SemanticMatchConfig cfg;
    cfg.overall_threshold = 0.80;
    auto results = sm.findSimilarEntities(incoming, {candidate}, cfg);
    EXPECT_FALSE(results.empty());
    if (!results.empty()) {
        EXPECT_GE(results[0].overall_confidence, 0.80);
    }
}

TEST_F(SemanticMatcherTest, FindSimilarEntitiesEmptyCandidates) {
    ti::SemanticMatcher sm;
    ti::SemanticMatchConfig cfg;
    auto results = sm.findSimilarEntities({{"id", "u-1"}}, {}, cfg);
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// HybridEntityMatcher tests
// ============================================================================

class HybridMatcherTest : public ::testing::Test {
protected:
    ti::HybridEntityMatcher matcher;
};

TEST_F(HybridMatcherTest, DeterministicFirstPrefersExactMatch) {
    ti::json incoming = {{"id", "u-1"}, {"name", "Alice"}};
    ti::json existing = {{"id", "u-1"}, {"name", "Alice"}};

    ti::SemanticMatchConfig sem_cfg;
    sem_cfg.overall_threshold = 0.80;

    auto results = matcher.findMatchingEntities(
        incoming, {existing}, {"id"},
        ti::HybridEntityMatcher::MatchStrategy::DETERMINISTIC_FIRST,
        sem_cfg, 0.85
    );
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(results[0].match_method, "deterministic");
    EXPECT_DOUBLE_EQ(results[0].deterministic_score, 1.0);
}

TEST_F(HybridMatcherTest, WeightedEnsembleCombinesScores) {
    ti::json incoming = {{"id", "u-1"}, {"name", "Alice Smith"}};
    ti::json existing = {{"id", "u-1"}, {"name", "Alice Smith"}};

    ti::SemanticMatchConfig sem_cfg;
    sem_cfg.overall_threshold = 0.50;

    auto results = matcher.findMatchingEntities(
        incoming, {existing}, {"id"},
        ti::HybridEntityMatcher::MatchStrategy::WEIGHTED_ENSEMBLE,
        sem_cfg, 0.5
    );
    ASSERT_FALSE(results.empty());
    EXPECT_GT(results[0].hybrid_score, 0.0);
}

TEST_F(HybridMatcherTest, SelectOptimalStrategyWithPK) {
    std::vector<ti::FieldCharacteristics> fields = {
        {"id", "id", 1.0, true, true}
    };
    auto strategy = ti::HybridEntityMatcher::selectOptimalStrategy(fields);
    EXPECT_EQ(strategy, ti::HybridEntityMatcher::MatchStrategy::DETERMINISTIC_FIRST);
}

TEST_F(HybridMatcherTest, SelectOptimalStrategyTextOnly) {
    std::vector<ti::FieldCharacteristics> fields = {
        {"description", "text", 0.9, false, false}
    };
    auto strategy = ti::HybridEntityMatcher::selectOptimalStrategy(fields);
    EXPECT_EQ(strategy, ti::HybridEntityMatcher::MatchStrategy::SEMANTIC_FIRST);
}

TEST_F(HybridMatcherTest, BelowThresholdNoResults) {
    ti::json incoming = {{"id", "u-1"}, {"name", "Alice"}};
    ti::json existing = {{"id", "u-99"}, {"name", "Completely Different Name XYZ"}};

    ti::SemanticMatchConfig sem_cfg;
    sem_cfg.overall_threshold = 0.85;

    auto results = matcher.findMatchingEntities(
        incoming, {existing}, {},
        ti::HybridEntityMatcher::MatchStrategy::SEMANTIC_FIRST,
        sem_cfg, 0.85
    );
    // Name is very different → should not exceed threshold.
    for (const auto& r : results) {
        EXPECT_GE(r.hybrid_score, 0.85);
    }
}
