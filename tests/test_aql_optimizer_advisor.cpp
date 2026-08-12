/**
 * @file test_aql_optimizer_advisor.cpp
 * @brief Unit tests for AQLOptimizerAdvisor — cost-aware AQL query suggestions
 */

#include <gtest/gtest.h>
#include "aql/aql_optimizer_advisor.h"

using namespace themis::aql;

// ============================================================================
// Helper
// ============================================================================

static bool hasSuggestionInClause(
    const std::vector<ValidationIssue>& issues,
    ValidationIssue::Severity sev,
    const std::string& clause)
{
    for (const auto& issue : issues) {
        if (issue.severity == sev && issue.clause == clause) {
            return true;
        }
    }
    return false;
}

static bool anySuggestionContains(
    const std::vector<ValidationIssue>& issues,
    const std::string& substring)
{
    for (const auto& issue : issues) {
        if (issue.message.find(substring) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Basic behaviour
// ============================================================================

class AQLOptimizerAdvisorTest : public ::testing::Test {
protected:
    AQLOptimizerAdvisor advisor;
};

TEST_F(AQLOptimizerAdvisorTest, EmptyQueryReturnsNoSuggestions) {
    auto suggestions = advisor.suggest("");
    EXPECT_TRUE(suggestions.empty());
}

TEST_F(AQLOptimizerAdvisorTest, SimpleQueryNoSuggestionsOrInfoOnly) {
    // A trivial query should not produce WARNING-level suggestions
    auto suggestions = advisor.suggest("FOR doc IN users RETURN doc");
    for (const auto& s : suggestions) {
        EXPECT_NE(s.severity, ValidationIssue::Severity::ERROR)
            << "Advisor must never produce ERROR issues";
        EXPECT_NE(s.severity, ValidationIssue::Severity::WARNING)
            << "Simple query should not trigger any WARNINGs";
    }
}

TEST_F(AQLOptimizerAdvisorTest, DoesNotProduceErrorIssues) {
    // No matter what query is provided, the advisor must not issue ERRORs
    std::vector<std::string> queries = {
        "",
        "FOR x IN col RETURN x",
        "FOR x IN col FILTER x.a == 1 RETURN x",
        "INVALID QUERY TEXT @@@",
    };
    for (const auto& q : queries) {
        for (const auto& s : advisor.suggest(q)) {
            EXPECT_NE(s.severity, ValidationIssue::Severity::ERROR)
                << "Advisor issued ERROR for: " << q;
        }
    }
}

// ============================================================================
// Index suggestions
// ============================================================================

TEST_F(AQLOptimizerAdvisorTest, VectorQuerySuggestsHNSWIndex) {
    const std::string q =
        "FOR doc IN items "
        "LET score = SIMILARITY(doc.embedding, @vec, 10) "
        "SORT score DESC LIMIT 5 RETURN doc";
    auto suggestions = advisor.suggest(q);
    bool found = anySuggestionContains(suggestions, "HNSW") ||
                 anySuggestionContains(suggestions, "hnsw") ||
                 anySuggestionContains(suggestions, "vector");
    EXPECT_TRUE(found) << "Expected an HNSW/vector index suggestion";
}

TEST_F(AQLOptimizerAdvisorTest, SpatialQuerySuggestsSpatialIndex) {
    const std::string q =
        "FOR doc IN locations "
        "FILTER PROXIMITY(doc.coords, [13.4, 52.5]) < 1000 "
        "RETURN doc";
    auto suggestions = advisor.suggest(q);
    bool found = anySuggestionContains(suggestions, "spatial") ||
                 anySuggestionContains(suggestions, "geo");
    EXPECT_TRUE(found) << "Expected a spatial/geo index suggestion";
}

// ============================================================================
// Hybrid query plan ordering
// ============================================================================

TEST_F(AQLOptimizerAdvisorTest, VectorGeoHybridYieldsOptimizerHint) {
    const std::string q =
        "FOR doc IN places "
        "FILTER ST_WITHIN(doc.loc, @bbox) "
        "LET score = SIMILARITY(doc.vec, @qvec, 5) "
        "SORT score DESC LIMIT 10 RETURN doc";
    auto suggestions = advisor.suggest(q);
    bool found = hasSuggestionInClause(
        suggestions, ValidationIssue::Severity::INFO, "OPTIMIZER");
    EXPECT_TRUE(found) << "Expected an OPTIMIZER hint for vector+geo hybrid";
}

TEST_F(AQLOptimizerAdvisorTest, FulltextGeoHybridYieldsOptimizerHint) {
    const std::string q =
        "FOR doc IN articles "
        "FILTER FULLTEXT(doc.body, 'database') "
        "FILTER ST_WITHIN(doc.location, @region) "
        "RETURN doc";
    auto suggestions = advisor.suggest(q);
    bool found = hasSuggestionInClause(
        suggestions, ValidationIssue::Severity::INFO, "OPTIMIZER");
    EXPECT_TRUE(found) << "Expected an OPTIMIZER hint for fulltext+geo hybrid";
}

// ============================================================================
// Graph traversal
// ============================================================================

TEST_F(AQLOptimizerAdvisorTest, DeepTraversalTriggersWarning) {
    // depth 1..15 → estimated expansion = 4^15 > 50 000 → WARNING
    const std::string q =
        "FOR v, e, p IN 1..15 OUTBOUND @start GRAPH 'social' RETURN v";
    auto suggestions = advisor.suggest(q);
    bool found = hasSuggestionInClause(
        suggestions, ValidationIssue::Severity::WARNING, "OPTIMIZER");
    EXPECT_TRUE(found) << "Expected a WARNING for deep graph traversal";
}

TEST_F(AQLOptimizerAdvisorTest, ShallowTraversalNoWarning) {
    // depth 1..2 → estimated expansion = 4^2 = 16 → no WARNING
    const std::string q =
        "FOR v, e, p IN 1..2 OUTBOUND @start GRAPH 'social' RETURN v";
    auto suggestions = advisor.suggest(q);
    bool found = hasSuggestionInClause(
        suggestions, ValidationIssue::Severity::WARNING, "OPTIMIZER");
    EXPECT_FALSE(found) << "Shallow traversal should not trigger a WARNING";
}

// ============================================================================
// Return types
// ============================================================================

TEST_F(AQLOptimizerAdvisorTest, AllReturnedIssuesHaveNonEmptyMessage) {
    const std::string q =
        "FOR v IN 1..10 OUTBOUND @s GRAPH 'g' "
        "FILTER ST_WITHIN(v.loc, @bbox) "
        "LET score = SIMILARITY(v.vec, @qvec, 5) "
        "RETURN v";
    for (const auto& s : advisor.suggest(q)) {
        EXPECT_FALSE(s.message.empty()) << "Suggestion has empty message";
        EXPECT_FALSE(s.clause.empty())  << "Suggestion has empty clause";
    }
}

TEST_F(AQLOptimizerAdvisorTest, SuggestDoesNotThrow) {
    EXPECT_NO_THROW({
        advisor.suggest("FOR x IN col RETURN x");
        advisor.suggest("");
        advisor.suggest("INVALID @@@");
    });
}
