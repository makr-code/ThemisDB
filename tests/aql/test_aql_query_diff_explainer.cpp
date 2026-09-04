/**
 * @file test_aql_query_diff_explainer.cpp
 * @brief Unit tests for AQLQueryDiffExplainer (DIFF-01..18)
 */

#include <gtest/gtest.h>
#include "aql/aql_query_diff_explainer.h"

#include <algorithm>
#include <string>

using namespace themis::aql;

// Helpers
static bool hasDiffKind(const QueryDiffResult& r, QueryDiffEntry::Kind k) {
    return std::any_of(r.diffs.begin(), r.diffs.end(),
        [k](const QueryDiffEntry& e) { return e.kind == k; });
}

static bool explanationContains(const QueryDiffResult& r, const std::string& substr) {
    for (const auto& d : r.diffs) {
        if (d.explanation.find(substr) != std::string::npos) {
          return true;
        }
    }
    return r.summary.find(substr) != std::string::npos;
}

class AQLQueryDiffExplainerTest : public ::testing::Test {
protected:
    AQLQueryDiffExplainer explainer;
};

// DIFF-01: Identical queries → is_equivalent = true, no diffs
TEST_F(AQLQueryDiffExplainerTest, DIFF01_IdenticalQueries) {
    const std::string q = "FOR d IN docs FILTER d.age > 18 RETURN d";
    auto r = explainer.explain(q, q);
    EXPECT_TRUE(r.is_equivalent);
    EXPECT_TRUE(r.diffs.empty());
}

// DIFF-02: Whitespace-only differences → equivalent
TEST_F(AQLQueryDiffExplainerTest, DIFF02_WhitespaceDiff) {
    const std::string a = "FOR d IN docs FILTER d.age > 18 RETURN d";
    const std::string b = "FOR  d  IN  docs  FILTER  d.age  >  18  RETURN  d";
    auto r = explainer.explain(a, b);
    EXPECT_TRUE(r.is_equivalent);
}

// DIFF-03: FILTER clause added
TEST_F(AQLQueryDiffExplainerTest, DIFF03_FilterAdded) {
    const std::string a = "FOR d IN docs RETURN d";
    const std::string b = "FOR d IN docs FILTER d.active == true RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::CLAUSE_ADDED));
}

// DIFF-04: FILTER clause removed
TEST_F(AQLQueryDiffExplainerTest, DIFF04_FilterRemoved) {
    const std::string a = "FOR d IN docs FILTER d.active == true RETURN d";
    const std::string b = "FOR d IN docs RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::CLAUSE_REMOVED));
}

// DIFF-05: SORT clause added
TEST_F(AQLQueryDiffExplainerTest, DIFF05_SortAdded) {
    const std::string a = "FOR d IN docs FILTER d.age > 0 RETURN d";
    const std::string b = "FOR d IN docs FILTER d.age > 0 SORT d.name ASC RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::CLAUSE_ADDED));
}

// DIFF-06: LIMIT changed
TEST_F(AQLQueryDiffExplainerTest, DIFF06_LimitChanged) {
    const std::string a = "FOR d IN docs LIMIT 10 RETURN d";
    const std::string b = "FOR d IN docs LIMIT 100 RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::LIMIT_CHANGED));
}

// DIFF-07: RETURN clause changed
TEST_F(AQLQueryDiffExplainerTest, DIFF07_ReturnChanged) {
    const std::string a = "FOR d IN docs RETURN d";
    const std::string b = "FOR d IN docs RETURN d.name";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::RETURN_CHANGED));
}

// DIFF-08: Function added (KNN appears in B but not A)
TEST_F(AQLQueryDiffExplainerTest, DIFF08_FunctionAdded) {
    const std::string a = "FOR d IN docs FILTER d.score > 0.5 RETURN d";
    const std::string b = "FOR d IN docs FILTER KNN(d.vec, @q) < 0.1 RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::FUNCTION_ADDED));
}

// DIFF-09: Function removed
TEST_F(AQLQueryDiffExplainerTest, DIFF09_FunctionRemoved) {
    const std::string a = "FOR d IN docs FILTER BM25(d, @q) > 0.5 RETURN d";
    const std::string b = "FOR d IN docs FILTER d.text LIKE @q RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_TRUE(hasDiffKind(r, QueryDiffEntry::Kind::FUNCTION_REMOVED));
}

// DIFF-10: Both queries empty → equivalent
TEST_F(AQLQueryDiffExplainerTest, DIFF10_BothEmpty) {
    auto r = explainer.explain("", "");
    EXPECT_TRUE(r.is_equivalent);
}

// DIFF-11: A empty, B non-empty
TEST_F(AQLQueryDiffExplainerTest, DIFF11_AEmptyBNonEmpty) {
    auto r = explainer.explain("", "FOR d IN docs RETURN d");
    EXPECT_FALSE(r.is_equivalent);
}

// DIFF-12: LET clause added
TEST_F(AQLQueryDiffExplainerTest, DIFF12_LetAdded) {
    const std::string a = "FOR d IN docs RETURN d";
    const std::string b = "FOR d IN docs LET x = d.value RETURN x";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_FALSE(r.diffs.empty());
}

// DIFF-13: Summary is not empty on non-equivalent queries
TEST_F(AQLQueryDiffExplainerTest, DIFF13_SummaryNonEmpty) {
    auto r = explainer.explain(
        "FOR d IN a RETURN d",
        "FOR d IN b RETURN d.name");
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_FALSE(r.summary.empty());
}

// DIFF-14: count() helper
TEST_F(AQLQueryDiffExplainerTest, DIFF14_CountHelper) {
    auto r = explainer.explain(
        "FOR d IN docs RETURN d",
        "FOR d IN docs LIMIT 10 RETURN d.name");
    int added   = r.count(QueryDiffEntry::Kind::CLAUSE_ADDED);
    int changed = r.count(QueryDiffEntry::Kind::RETURN_CHANGED);
    EXPECT_GE(added + changed, 1);
}

// DIFF-15: Both queries have INSERT (different collections)
TEST_F(AQLQueryDiffExplainerTest, DIFF15_InsertCollectionChanged) {
    const std::string a = "FOR d IN src INSERT d INTO dest1";
    const std::string b = "FOR d IN src INSERT d INTO dest2";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
}

// DIFF-16: COLLECT clause added
TEST_F(AQLQueryDiffExplainerTest, DIFF16_CollectAdded) {
    const std::string a = "FOR d IN docs RETURN d";
    const std::string b = "FOR d IN docs COLLECT city = d.city RETURN city";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
    EXPECT_FALSE(r.diffs.empty());
}

// DIFF-17: Different functions same structure
TEST_F(AQLQueryDiffExplainerTest, DIFF17_DifferentFunctions) {
    const std::string a = "FOR d IN docs FILTER BM25(d, @q) > 0.5 RETURN d";
    const std::string b = "FOR d IN docs FILTER KNN(d.vec, @q) < 0.2 RETURN d";
    auto r = explainer.explain(a, b);
    EXPECT_FALSE(r.is_equivalent);
}

// DIFF-18: IAQLQueryDiffExplainer interface is accessible via polymorphism
TEST_F(AQLQueryDiffExplainerTest, DIFF18_PolymorphicAccess) {
    std::unique_ptr<IAQLQueryDiffExplainer> iface =
        std::make_unique<AQLQueryDiffExplainer>();
    auto r = iface->explain("FOR d IN a RETURN d", "FOR d IN a RETURN d.x");
    EXPECT_FALSE(r.is_equivalent);
}
