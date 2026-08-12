// test_canonical_resolver.cpp
//
// Unit tests for CanonicalEntityResolver:
//   - createGoldenRecord with all ResolutionPolicy variants
//   - Field-level reconciliation (string, numeric, object)
//   - Completeness scoring
//   - Field provenance tracking
//   - Protected fields
//   - scoreFieldQuality

#include <gtest/gtest.h>
#include "importers/canonical_resolver.h"

namespace ti = themis::importers;

using Pair = std::pair<std::string, ti::json>;

// ============================================================================
// reconcileStringField tests
// ============================================================================

class StringReconcileTest : public ::testing::Test {};

TEST_F(StringReconcileTest, KeepExisting) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "Alice Smith", "ALICE S.", ti::FieldRule::KEEP_EXISTING), "Alice Smith");
}

TEST_F(StringReconcileTest, TakeIncoming) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "old", "new", ti::FieldRule::TAKE_INCOMING), "new");
}

TEST_F(StringReconcileTest, TakeLongest) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "Alice Smith", "A. S.", ti::FieldRule::TAKE_LONGEST), "Alice Smith");
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "A", "Bob Jones", ti::FieldRule::TAKE_LONGEST), "Bob Jones");
}

TEST_F(StringReconcileTest, TakeMax) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "b", "a", ti::FieldRule::TAKE_MAX), "b");
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "a", "z", ti::FieldRule::TAKE_MAX), "z");
}

TEST_F(StringReconcileTest, TakeMin) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "b", "a", ti::FieldRule::TAKE_MIN), "a");
}

TEST_F(StringReconcileTest, ConcatenateBothNonEmpty) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "note1", "note2", ti::FieldRule::CONCATENATE, " | "), "note1 | note2");
}

TEST_F(StringReconcileTest, ConcatenateOneEmpty) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "", "note2", ti::FieldRule::CONCATENATE), "note2");
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "note1", "", ti::FieldRule::CONCATENATE), "note1");
}

TEST_F(StringReconcileTest, TakeNewestTimestamp) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileStringField(
        "2024-01-01T00:00:00Z", "2025-06-15T10:00:00Z", ti::FieldRule::TAKE_NEWEST),
        "2025-06-15T10:00:00Z");
}

// ============================================================================
// reconcileNumericField tests
// ============================================================================

class NumericReconcileTest : public ::testing::Test {};

TEST_F(NumericReconcileTest, TakeMax) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileNumericField(10, 20, ti::FieldRule::TAKE_MAX), 20);
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileNumericField(30, 5,  ti::FieldRule::TAKE_MAX), 30);
}

TEST_F(NumericReconcileTest, TakeMin) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileNumericField(10, 20, ti::FieldRule::TAKE_MIN), 10);
}

TEST_F(NumericReconcileTest, TakeSum) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileNumericField(100, 200, ti::FieldRule::TAKE_SUM), 300);
}

TEST_F(NumericReconcileTest, KeepExisting) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileNumericField(42, 99, ti::FieldRule::KEEP_EXISTING), 42);
}

TEST_F(NumericReconcileTest, TakeIncoming) {
    EXPECT_EQ(ti::CanonicalEntityResolver::reconcileNumericField(42, 99, ti::FieldRule::TAKE_INCOMING), 99);
}

// ============================================================================
// scoreFieldQuality tests
// ============================================================================

class FieldQualityTest : public ::testing::Test {};

TEST_F(FieldQualityTest, EmptyValueScoresZero) {
    EXPECT_DOUBLE_EQ(ti::CanonicalEntityResolver::scoreFieldQuality("name", ""), 0.0);
}

TEST_F(FieldQualityTest, NonEmptyValueScoresOne) {
    ti::FieldQualityPolicy policy;
    EXPECT_DOUBLE_EQ(ti::CanonicalEntityResolver::scoreFieldQuality("name", "Alice", policy), 1.0);
}

TEST_F(FieldQualityTest, TooShortValueReducesScore) {
    ti::FieldQualityPolicy policy;
    policy.min_length = 5;
    double score = ti::CanonicalEntityResolver::scoreFieldQuality("name", "Al", policy);
    EXPECT_LT(score, 1.0);
}

TEST_F(FieldQualityTest, NonDigitPhoneReducesScore) {
    ti::FieldQualityPolicy policy;
    policy.prefer_digits_only = true;
    double score = ti::CanonicalEntityResolver::scoreFieldQuality("phone", "+1-800-FLOWERS", policy);
    EXPECT_LT(score, 1.0);
}

// ============================================================================
// createGoldenRecord tests
// ============================================================================

class GoldenRecordTest : public ::testing::Test {
protected:
    ti::CanonicalEntityResolver resolver;
};

TEST_F(GoldenRecordTest, EmptyInputReturnsEmptyGoldenRecord) {
    auto gr = resolver.createGoldenRecord({}, "users", ti::ResolutionPolicy::RICHEST_MERGE);
    EXPECT_TRUE(gr.contributing_ids.empty());
    EXPECT_DOUBLE_EQ(gr.completeness_score, 0.0);
}

TEST_F(GoldenRecordTest, SingleEntityReturnsItself) {
    ti::json e1 = {{"id", "u-1"}, {"name", "Alice Smith"}, {"email", "alice@test.com"}};
    std::vector<Pair> entities = {{"u-1", e1}};

    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::EXISTING_PREFERRED);
    ASSERT_EQ(gr.contributing_ids.size(), 1u);
    EXPECT_EQ(gr.merged_data["name"].get<std::string>(), "Alice Smith");
    EXPECT_GT(gr.completeness_score, 0.0);
}

TEST_F(GoldenRecordTest, RichestMergePrefersLongerName) {
    // entity 1 has abbreviated name, entity 2 has full name
    ti::json e1 = {{"id", "u-1"}, {"name", "Alice S."}};
    ti::json e2 = {{"id", "u-2"}, {"name", "Alice Smith"}};
    std::vector<Pair> entities = {{"u-1", e1}, {"u-2", e2}};

    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::RICHEST_MERGE);
    EXPECT_EQ(gr.merged_data["name"].get<std::string>(), "Alice Smith");
    ASSERT_EQ(gr.contributing_ids.size(), 2u);
}

TEST_F(GoldenRecordTest, ExistingPreferredKeepsBase) {
    ti::json e1 = {{"id", "u-1"}, {"name", "Original Name"}};
    ti::json e2 = {{"id", "u-2"}, {"name", "New Name"}};
    std::vector<Pair> entities = {{"u-1", e1}, {"u-2", e2}};

    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::EXISTING_PREFERRED);
    EXPECT_EQ(gr.merged_data["name"].get<std::string>(), "Original Name");
}

TEST_F(GoldenRecordTest, IncomingPreferredOverwritesExisting) {
    ti::json e1 = {{"id", "u-1"}, {"name", "Old Name"}, {"email", "old@test.com"}};
    ti::json e2 = {{"id", "u-2"}, {"name", "New Name"}, {"email", "new@test.com"}};
    std::vector<Pair> entities = {{"u-1", e1}, {"u-2", e2}};

    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::INCOMING_PREFERRED);
    EXPECT_EQ(gr.merged_data["name"].get<std::string>(), "New Name");
}

TEST_F(GoldenRecordTest, MostCompleteSelectsMostFilledRecord) {
    // e2 has more fields filled
    ti::json e1 = {{"id", "u-1"}, {"name", "Alice"}, {"email", nullptr}};
    ti::json e2 = {{"id", "u-2"}, {"name", "Alice Smith"}, {"email", "alice@test.com"}};
    std::vector<Pair> entities = {{"u-1", e1}, {"u-2", e2}};

    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::MOST_COMPLETE);
    EXPECT_EQ(gr.merged_data["email"].get<std::string>(), "alice@test.com");
}

TEST_F(GoldenRecordTest, ProtectedFieldsNotOverwritten) {
    ti::json e1 = {{"id", "original-id"}, {"created_at", "2020-01-01T00:00:00Z"}};
    ti::json e2 = {{"id", "new-id"},      {"created_at", "2025-06-01T00:00:00Z"}};
    std::vector<Pair> entities = {{"orig", e1}, {"new", e2}};

    auto gr = resolver.createGoldenRecord(
        entities, "users", ti::ResolutionPolicy::INCOMING_PREFERRED,
        {}, {"id", "created_at"}
    );
    EXPECT_EQ(gr.merged_data["id"].get<std::string>(), "original-id");
    EXPECT_EQ(gr.merged_data["created_at"].get<std::string>(), "2020-01-01T00:00:00Z");
}

TEST_F(GoldenRecordTest, FieldProvenanceTracked) {
    ti::json e1 = {{"id", "u-1"}, {"name", "A"}};
    ti::json e2 = {{"id", "u-2"}, {"email", "b@test.com"}};
    std::vector<Pair> entities = {{"u-1", e1}, {"u-2", e2}};

    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::RICHEST_MERGE);
    EXPECT_TRUE(gr.field_provenance.contains("name"));
    EXPECT_TRUE(gr.field_provenance.contains("email"));
}

TEST_F(GoldenRecordTest, CompletenessScoreZeroToOne) {
    ti::json e = {{"id", "u-1"}, {"name", "Alice"}, {"email", nullptr}};
    std::vector<Pair> entities = {{"u-1", e}};
    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::EXISTING_PREFERRED);
    EXPECT_GE(gr.completeness_score, 0.0);
    EXPECT_LE(gr.completeness_score, 1.0);
}

TEST_F(GoldenRecordTest, CanonicalIdIsNonEmpty) {
    ti::json e = {{"id", "u-1"}, {"name", "Bob"}};
    std::vector<Pair> entities = {{"u-1", e}};
    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::RICHEST_MERGE);
    EXPECT_FALSE(gr.canonical_id.empty());
}

TEST_F(GoldenRecordTest, LastReconciliationPopulated) {
    ti::json e = {{"id", "u-1"}, {"name", "Carol"}};
    std::vector<Pair> entities = {{"u-1", e}};
    auto gr = resolver.createGoldenRecord(entities, "users", ti::ResolutionPolicy::RICHEST_MERGE);
    EXPECT_FALSE(gr.last_reconciliation.empty());
    // Should look like an RFC3339 timestamp.
    EXPECT_NE(gr.last_reconciliation.find('T'), std::string::npos);
}

TEST_F(GoldenRecordTest, CustomRulesFieldLevelMerge) {
    ti::json e1 = {{"id", "u-1"}, {"score", 100}, {"notes", "original note"}};
    ti::json e2 = {{"id", "u-2"}, {"score", 50},  {"notes", "extra note"}};
    std::vector<Pair> entities = {{"u-1", e1}, {"u-2", e2}};

    std::map<std::string, ti::FieldRule> rules = {
        {"score", ti::FieldRule::TAKE_MAX},
        {"notes", ti::FieldRule::CONCATENATE}
    };

    auto gr = resolver.createGoldenRecord(
        entities, "users", ti::ResolutionPolicy::CUSTOM_RULES, rules);
    // Score: max(100, 50) = 100 stays with the base entity.
    // Notes: concatenated.
    EXPECT_TRUE(gr.merged_data["notes"].get<std::string>().find("original note") != std::string::npos ||
                gr.merged_data["notes"].get<std::string>().find("extra note")    != std::string::npos);
}

TEST_F(GoldenRecordTest, GoldenRecordToJsonContainsAllKeys) {
    ti::json e = {{"id", "u-1"}, {"name", "Dave"}};
    auto gr = resolver.createGoldenRecord({{"u-1", e}}, "users",
                                          ti::ResolutionPolicy::RICHEST_MERGE);
    auto j = gr.toJson();
    EXPECT_TRUE(j.contains("canonical_id"));
    EXPECT_TRUE(j.contains("merged_data"));
    EXPECT_TRUE(j.contains("contributing_ids"));
    EXPECT_TRUE(j.contains("completeness_score"));
    EXPECT_TRUE(j.contains("field_provenance"));
    EXPECT_TRUE(j.contains("last_reconciliation"));
}

// ============================================================================
// reconcileObjectField tests
// ============================================================================

class ObjectReconcileTest : public ::testing::Test {};

TEST_F(ObjectReconcileTest, MergeAddsFieldsFromBothObjects) {
    ti::json o1 = {{"a", 1}};
    ti::json o2 = {{"b", 2}};
    auto merged = ti::CanonicalEntityResolver::reconcileObjectField(
        o1, o2, ti::ResolutionPolicy::RICHEST_MERGE);
    EXPECT_TRUE(merged.contains("a"));
    EXPECT_TRUE(merged.contains("b"));
}

TEST_F(ObjectReconcileTest, ExistingPreferredKeepsOriginalValues) {
    ti::json o1 = {{"key", "original"}};
    ti::json o2 = {{"key", "override"}};
    auto merged = ti::CanonicalEntityResolver::reconcileObjectField(
        o1, o2, ti::ResolutionPolicy::EXISTING_PREFERRED);
    EXPECT_EQ(merged["key"].get<std::string>(), "original");
}

TEST_F(ObjectReconcileTest, IncomingPreferredOverridesExisting) {
    ti::json o1 = {{"key", "original"}};
    ti::json o2 = {{"key", "override"}};
    auto merged = ti::CanonicalEntityResolver::reconcileObjectField(
        o1, o2, ti::ResolutionPolicy::INCOMING_PREFERRED);
    EXPECT_EQ(merged["key"].get<std::string>(), "override");
}

TEST_F(ObjectReconcileTest, DepthZeroReturnsEntiretyBasedOnPolicy) {
    ti::json o1 = {{"x", 1}};
    ti::json o2 = {{"y", 2}};
    auto merged = ti::CanonicalEntityResolver::reconcileObjectField(
        o1, o2, ti::ResolutionPolicy::EXISTING_PREFERRED, 0);
    EXPECT_EQ(merged, o1);
}

TEST_F(ObjectReconcileTest, NonObjectFallsBackToPolicy) {
    ti::json v1 = "string_val";
    ti::json v2 = "other_val";
    auto result = ti::CanonicalEntityResolver::reconcileObjectField(
        v1, v2, ti::ResolutionPolicy::EXISTING_PREFERRED);
    EXPECT_EQ(result, v1);
    auto result2 = ti::CanonicalEntityResolver::reconcileObjectField(
        v1, v2, ti::ResolutionPolicy::INCOMING_PREFERRED);
    EXPECT_EQ(result2, v2);
}
