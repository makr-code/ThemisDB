/**
 * @file test_aql_migration_assistant.cpp
 * @brief Unit tests for AQLMigrationAssistant – ArangoDB AQL → ThemisDB AQL migration.
 */

#include <gtest/gtest.h>
#include "aql/aql_migration_assistant.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace themis::aql;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool hasIssueWithSeverity(const std::vector<MigrationIssue>& issues,
                                  MigrationIssue::Severity sev) {
    return std::any_of(issues.begin(), issues.end(),
        [sev](const MigrationIssue& i) { return i.severity == sev; });
}

static bool hasIssueContaining(const std::vector<MigrationIssue>& issues,
                                const std::string& substr) {
    return std::any_of(issues.begin(), issues.end(),
        [&substr](const MigrationIssue& i) {
            return i.message.find(substr) != std::string::npos ||
                   i.suggestion.find(substr) != std::string::npos;
        });
}

/// Case-insensitive search helper for test assertions.
static std::size_t findCI(const std::string& haystack, const std::string& needle,
                           std::size_t pos = 0) {
    if (needle.empty()) {
      return pos;
    }
    auto it = std::search(
        haystack.begin() + static_cast<std::string::difference_type>(pos),
        haystack.end(),
        needle.begin(), needle.end(),
        [](unsigned char a, unsigned char b) {
            return std::tolower(a) == std::tolower(b);
        });
    if (it == haystack.end()) {
      return std::string::npos;
    }
    return static_cast<std::size_t>(it - haystack.begin());
}

// ---------------------------------------------------------------------------
// MigrationResult::summary() tests
// ---------------------------------------------------------------------------

TEST(MigrationResultTest, EmptySummaryIsOK) {
    MigrationResult r;
    r.is_fully_automatable = true;
    EXPECT_EQ(r.summary(), "OK");
}

TEST(MigrationResultTest, SummaryWithError) {
    MigrationResult r;
    r.issues.push_back({MigrationIssue::Severity::ERROR, "e", ""});
    EXPECT_NE(r.summary().find("1 error"), std::string::npos);
}

TEST(MigrationResultTest, SummaryWithWarning) {
    MigrationResult r;
    r.issues.push_back({MigrationIssue::Severity::WARNING, "w", ""});
    EXPECT_NE(r.summary().find("1 warning"), std::string::npos);
}

TEST(MigrationResultTest, SummaryWithInfo) {
    MigrationResult r;
    r.issues.push_back({MigrationIssue::Severity::INFO, "i", ""});
    EXPECT_NE(r.summary().find("1 info"), std::string::npos);
}

TEST(MigrationResultTest, SummaryMultipleIssues) {
    MigrationResult r;
    r.issues.push_back({MigrationIssue::Severity::ERROR,   "e", ""});
    r.issues.push_back({MigrationIssue::Severity::WARNING, "w", ""});
    r.issues.push_back({MigrationIssue::Severity::INFO,    "i", ""});
    std::string s = r.summary();
    EXPECT_NE(s.find("1 error"),   std::string::npos);
    EXPECT_NE(s.find("1 warning"), std::string::npos);
    EXPECT_NE(s.find("1 info"),    std::string::npos);
}

// ---------------------------------------------------------------------------
// Empty / trivial input
// ---------------------------------------------------------------------------

class AQLMigrationAssistantTest : public ::testing::Test {
protected:
    AQLMigrationAssistant assistant;
};

TEST_F(AQLMigrationAssistantTest, EmptyQueryReturnsEmptyWithNoIssues) {
    auto result = assistant.migrate("");
    EXPECT_EQ(result.migrated_query, "");
    EXPECT_TRUE(result.issues.empty());
    EXPECT_TRUE(result.is_fully_automatable);
}

TEST_F(AQLMigrationAssistantTest, CompatibleQueryPassesThroughUnchanged) {
    const std::string aql =
        "FOR u IN users FILTER u.age > 18 SORT u.name ASC LIMIT 10 RETURN u";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(result.migrated_query, aql);
    EXPECT_TRUE(result.issues.empty());
    EXPECT_TRUE(result.is_fully_automatable);
}

// ---------------------------------------------------------------------------
// @@collection → @collection
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, DoubleAtBindRewritten) {
    const std::string aql = "FOR doc IN @@myCollection RETURN doc";
    auto result = assistant.migrate(aql);

    EXPECT_EQ(result.migrated_query.find("@@"), std::string::npos)
        << "All @@ should be replaced";
    EXPECT_NE(result.migrated_query.find("@myCollection"), std::string::npos);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::INFO));
}

TEST_F(AQLMigrationAssistantTest, MultipleDoubleAtBindsAllRewritten) {
    const std::string aql = "FOR a IN @@col1 FOR b IN @@col2 RETURN {a,b}";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(result.migrated_query.find("@@"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("@col1"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("@col2"), std::string::npos);
}

TEST_F(AQLMigrationAssistantTest, SingleAtBindNotModified) {
    const std::string aql = "FOR doc IN users FILTER doc.name == @name RETURN doc";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(result.migrated_query, aql);
}

// ---------------------------------------------------------------------------
// NEAR() → ST_DISTANCE-based sub-query
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, NearRewrittenToSTDistance) {
    const std::string aql =
        "FOR doc IN NEAR(restaurants, 52.5, 13.4, 10) RETURN doc";
    auto result = assistant.migrate(aql);

    EXPECT_EQ(findCI(result.migrated_query, "NEAR("), std::string::npos)
        << "NEAR() call should be gone";
    EXPECT_NE(result.migrated_query.find("ST_DISTANCE"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("restaurants"), std::string::npos);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::WARNING));
    EXPECT_TRUE(hasIssueContaining(result.issues, "NEAR"));
}

TEST_F(AQLMigrationAssistantTest, NearRewritePreservesLimit) {
    const std::string aql = "FOR doc IN NEAR(places, 48.8, 2.3, 5) RETURN doc";
    auto result = assistant.migrate(aql);
    EXPECT_NE(result.migrated_query.find("LIMIT 5"), std::string::npos);
}

TEST_F(AQLMigrationAssistantTest, NearRewriteIsFullyAutomatable) {
    const std::string aql = "FOR doc IN NEAR(shops, 0.0, 0.0, 3) RETURN doc";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(result.is_fully_automatable);
}

// ---------------------------------------------------------------------------
// WITHIN() → ST_DISTANCE-based FILTER
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, WithinRewrittenToSTDistanceFilter) {
    const std::string aql =
        "FOR doc IN WITHIN(parks, 40.7, -74.0, 1000) RETURN doc";
    auto result = assistant.migrate(aql);

    EXPECT_EQ(findCI(result.migrated_query, "WITHIN("), std::string::npos)
        << "WITHIN() call should be gone";
    EXPECT_NE(result.migrated_query.find("ST_DISTANCE"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("parks"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("<= 1000"), std::string::npos);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::WARNING));
    EXPECT_TRUE(hasIssueContaining(result.issues, "WITHIN"));
}

// ---------------------------------------------------------------------------
// FULLTEXT() → SIMILARITY()-based FILTER
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, FulltextRewrittenToSimilarity) {
    const std::string aql =
        "FOR doc IN FULLTEXT(articles, 'body', 'database') RETURN doc";
    auto result = assistant.migrate(aql);

    EXPECT_EQ(findCI(result.migrated_query, "FULLTEXT("), std::string::npos)
        << "FULLTEXT() call should be gone";
    EXPECT_NE(result.migrated_query.find("SIMILARITY"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("articles"), std::string::npos);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::WARNING));
    EXPECT_TRUE(hasIssueContaining(result.issues, "FULLTEXT"));
}

TEST_F(AQLMigrationAssistantTest, FulltextStripQuotedAttribute) {
    const std::string aql =
        "FOR doc IN FULLTEXT(docs, \"title\", @q) RETURN doc";
    auto result = assistant.migrate(aql);
    // The attribute 'title' should appear without surrounding quotes in the output
    EXPECT_NE(result.migrated_query.find("_ft_doc.title"), std::string::npos);
}

// ---------------------------------------------------------------------------
// DOCUMENT() → inline sub-query
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, DocumentTwoArgRewritten) {
    const std::string aql = "LET d = DOCUMENT(users, @key) RETURN d";
    auto result = assistant.migrate(aql);

    EXPECT_EQ(findCI(result.migrated_query, "DOCUMENT("), std::string::npos)
        << "DOCUMENT() call should be gone";
    EXPECT_NE(result.migrated_query.find("_doc._key"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("users"), std::string::npos);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::WARNING));
    EXPECT_TRUE(hasIssueContaining(result.issues, "DOCUMENT"));
}

TEST_F(AQLMigrationAssistantTest, DocumentOneArgEmitsWarningAndKeepsQuery) {
    const std::string aql = "LET d = DOCUMENT(@id) RETURN d";
    auto result = assistant.migrate(aql);

    // Single-arg form: cannot fully rewrite; query kept as-is, warning issued
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::WARNING));
    // is_fully_automatable is true (no ERROR), but the query may still contain DOCUMENT
}

// ---------------------------------------------------------------------------
// V8() → ERROR
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, V8DetectedAsError) {
    const std::string aql = "RETURN V8(require('os').cpus().length)";
    auto result = assistant.migrate(aql);

    EXPECT_FALSE(result.is_fully_automatable);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::ERROR));
    EXPECT_TRUE(hasIssueContaining(result.issues, "V8"));
}

TEST_F(AQLMigrationAssistantTest, V8QueryNotModified) {
    const std::string aql = "RETURN V8(1 + 1)";
    auto result = assistant.migrate(aql);
    // V8 is only flagged, not rewritten
    EXPECT_NE(result.migrated_query.find("V8("), std::string::npos);
}

// ---------------------------------------------------------------------------
// IS_STRING / IS_NUMBER / IS_BOOL / etc. → INFO
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, IsStringEmitsInfo) {
    const std::string aql = "FOR d IN col FILTER IS_STRING(d.name) RETURN d";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::INFO));
    EXPECT_TRUE(hasIssueContaining(result.issues, "IS_STRING"));
}

TEST_F(AQLMigrationAssistantTest, IsNumberEmitsInfo) {
    const std::string aql = "FOR d IN col FILTER IS_NUMBER(d.age) RETURN d";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(hasIssueContaining(result.issues, "IS_NUMBER"));
}

TEST_F(AQLMigrationAssistantTest, IsNullEmitsInfo) {
    const std::string aql = "FOR d IN col FILTER IS_NULL(d.field) RETURN d";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(hasIssueContaining(result.issues, "IS_NULL"));
}

TEST_F(AQLMigrationAssistantTest, TypeCheckDoesNotMarkErrorAutomatable) {
    const std::string aql = "FOR d IN col FILTER IS_BOOL(d.flag) RETURN d";
    auto result = assistant.migrate(aql);
    // INFO-only issues → still fully automatable
    EXPECT_TRUE(result.is_fully_automatable);
}

// ---------------------------------------------------------------------------
// HASH() → INFO
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, HashFunctionEmitsInfo) {
    const std::string aql = "RETURN HASH(\"hello\")";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::INFO));
    EXPECT_TRUE(hasIssueContaining(result.issues, "HASH"));
}

TEST_F(AQLMigrationAssistantTest, HashQueryRemainsAutomatable) {
    const std::string aql = "RETURN HASH(42)";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(result.is_fully_automatable);
}

// ---------------------------------------------------------------------------
// ATTRIBUTES() → INFO
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, AttributesFunctionEmitsInfo) {
    const std::string aql = "RETURN ATTRIBUTES(doc)";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::INFO));
    EXPECT_TRUE(hasIssueContaining(result.issues, "ATTRIBUTES"));
}

// ---------------------------------------------------------------------------
// TRANSLATE() → INFO
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, TranslateFunctionEmitsInfo) {
    const std::string aql =
        "FOR d IN col RETURN TRANSLATE(d.status, {active: 'yes', inactive: 'no'})";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::INFO));
    EXPECT_TRUE(hasIssueContaining(result.issues, "TRANSLATE"));
}

// ---------------------------------------------------------------------------
// is_fully_automatable flag
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, FullyAutomatableWithOnlyWarnings) {
    const std::string aql = "FOR doc IN NEAR(col, 0.0, 0.0, 1) RETURN doc";
    auto result = assistant.migrate(aql);
    // Warnings don't block automation
    EXPECT_TRUE(result.is_fully_automatable);
}

TEST_F(AQLMigrationAssistantTest, NotFullyAutomatableWithError) {
    const std::string aql = "RETURN V8(process.version)";
    auto result = assistant.migrate(aql);
    EXPECT_FALSE(result.is_fully_automatable);
}

// ---------------------------------------------------------------------------
// Combined migration (multiple rewrites in one query)
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, CombinedDoubleAtAndFulltext) {
    const std::string aql =
        "FOR doc IN FULLTEXT(@@articles, 'title', 'db') RETURN doc";
    auto result = assistant.migrate(aql);

    // Both @@ and FULLTEXT should be handled
    EXPECT_EQ(result.migrated_query.find("@@"), std::string::npos);
    EXPECT_EQ(findCI(result.migrated_query, "FULLTEXT("), std::string::npos);
    EXPECT_NE(result.migrated_query.find("SIMILARITY"), std::string::npos);
}

TEST_F(AQLMigrationAssistantTest, MultipleInfoIssuesAccumulate) {
    const std::string aql =
        "FOR d IN col FILTER IS_STRING(d.a) && IS_NUMBER(d.b) RETURN d";
    auto result = assistant.migrate(aql);
    long info_count = std::count_if(result.issues.begin(), result.issues.end(),
        [](const MigrationIssue& i) {
            return i.severity == MigrationIssue::Severity::INFO;
        });
    EXPECT_GE(info_count, 2);
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, IdentifierWithNEARSubstringNotTouched) {
    // "NEAREST" should not be rewritten as it is not "NEAR("
    const std::string aql = "LET nearest = 5 RETURN nearest";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(result.migrated_query, aql);
    EXPECT_TRUE(result.issues.empty());
}

TEST_F(AQLMigrationAssistantTest, IdentifierWithV8SubstringNotTouched) {
    // A field named "v8_engine" should not trigger the V8 detector
    const std::string aql = "FOR d IN engines FILTER d.v8_engine == true RETURN d";
    auto result = assistant.migrate(aql);
    EXPECT_TRUE(result.is_fully_automatable);
    EXPECT_FALSE(hasIssueWithSeverity(result.issues, MigrationIssue::Severity::ERROR));
}

// ---------------------------------------------------------------------------
// Multi-occurrence rewrites (regression for first-only bug)
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, TwoNearCallsBothRewritten) {
    const std::string aql =
        "FOR x IN NEAR(col1, 1.0, 2.0, 5) "
        "FOR y IN NEAR(col2, 3.0, 4.0, 3) "
        "RETURN {x, y}";
    auto result = assistant.migrate(aql);
    // Neither NEAR() should remain
    EXPECT_EQ(findCI(result.migrated_query, "NEAR("), std::string::npos);
    EXPECT_NE(result.migrated_query.find("col1"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("col2"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("LIMIT 5"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("LIMIT 3"), std::string::npos);
}

TEST_F(AQLMigrationAssistantTest, TwoFulltextCallsBothRewritten) {
    const std::string aql =
        "LET a = FULLTEXT(col1, 'title', 'foo') "
        "LET b = FULLTEXT(col2, 'body', 'bar') "
        "RETURN {a, b}";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(findCI(result.migrated_query, "FULLTEXT("), std::string::npos);
    EXPECT_NE(result.migrated_query.find("col1"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("col2"), std::string::npos);
}

TEST_F(AQLMigrationAssistantTest, TwoDocumentCallsBothRewritten) {
    const std::string aql =
        "LET u = DOCUMENT(users, @uk) "
        "LET o = DOCUMENT(orders, @ok) "
        "RETURN {u, o}";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(findCI(result.migrated_query, "DOCUMENT("), std::string::npos);
    EXPECT_NE(result.migrated_query.find("users"), std::string::npos);
    EXPECT_NE(result.migrated_query.find("orders"), std::string::npos);
}

// ---------------------------------------------------------------------------
// False-early-exit regression: identifier matching keyword before actual call
// ---------------------------------------------------------------------------

TEST_F(AQLMigrationAssistantTest, NearestIdentifierBeforeNearCallIsHandled) {
    // "NEAREST" (not a function call) appears before an actual "NEAR()" call.
    // The old first-only implementation would match "NEAR" inside "NEAREST",
    // then exit because no '(' followed; the actual NEAR() was never rewritten.
    const std::string aql =
        "LET NEAREST = NEAR(col, 0.0, 0.0, 1) RETURN NEAREST";
    auto result = assistant.migrate(aql);
    // The NEAR() call must be rewritten
    EXPECT_EQ(findCI(result.migrated_query, "NEAR("), std::string::npos)
        << "NEAR() should be rewritten even when preceded by 'NEAREST' identifier";
    EXPECT_NE(result.migrated_query.find("ST_DISTANCE"), std::string::npos);
}

TEST_F(AQLMigrationAssistantTest, WithinIdentifierBeforeWithinCallIsHandled) {
    const std::string aql =
        "LET WITHIN_RADIUS = WITHIN(parks, 0.0, 0.0, 500) RETURN WITHIN_RADIUS";
    auto result = assistant.migrate(aql);
    EXPECT_EQ(findCI(result.migrated_query, "WITHIN("), std::string::npos)
        << "WITHIN() should be rewritten even when preceded by 'WITHIN_RADIUS' identifier";
    EXPECT_NE(result.migrated_query.find("ST_DISTANCE"), std::string::npos);
}
