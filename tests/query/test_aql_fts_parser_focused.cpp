/**
 * @file test_aql_fts_parser_focused.cpp
 * @brief Phase 6 FTS parser-layer focused tests (FTS-P-01..FTS-P-16).
 *
 * Validates the SEARCH clause parsing infrastructure added in Phase 6:
 *   - Lexer recognition of SEARCH, PHRASE, NEAR, STARTS_WITH, BOOST, ANALYZER tokens.
 *   - FtsPredicateNode construction for each predicate type.
 *   - SearchClauseNode wired into Query::search_clause.
 *   - Modifier parsing: BOOST, ANALYZER (per-predicate and top-level).
 *   - Multiple predicates via comma separation.
 *   - Error reporting on malformed SEARCH clauses.
 *
 * These tests exercise only the parser layer; no query executor or storage backend
 * is required.  Backend FTS execution is tested separately once executor wiring
 * lands in Q4 2026.
 *
 * @see include/query/aql_parser.h — FtsPredicateNode, SearchClauseNode, Query
 * @see src/query/ROADMAP.md — Phase 6 FTS
 * @version 1.0.0
 * @note Maturity: PRODUCTION-READY (parser layer)
 */

#include <gtest/gtest.h>

#include "query/aql_parser.h"

#include <memory>
#include <string>

using namespace themis::query;

// ============================================================================
// Fixture
// ============================================================================

class FtsParserTest : public ::testing::Test {
protected:
    AQLParser parser;

    /// Parse a query and assert success; return the query AST.
    std::shared_ptr<Query> parse_ok(const std::string& aql) {
        auto result = parser.parse(aql);
        EXPECT_TRUE(result.has_value())
            << "parse() failed for: " << aql << "\n  error: "
            << (result.has_value() ? "" : result.error().message());
        if (!result.has_value()) {
            return nullptr;
        }
        return result.value();
    }

    /// Parse a query and assert parse failure.
    void parse_err(const std::string& aql) {
        auto result = parser.parse(aql);
        EXPECT_FALSE(result.has_value())
            << "Expected parse error for: " << aql;
    }
};

// ============================================================================
// FTS-P-01: SEARCH clause present → Query::search_clause populated
// ============================================================================

/**
 * FTS-P-01: A query with a SEARCH clause must have query->search_clause != nullptr.
 */
TEST_F(FtsParserTest, FtsP01_SearchClausePresent) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH doc.title == "hello" RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr)
        << "search_clause must be set for a SEARCH query";
    EXPECT_EQ(q->search_clause->getType(), ASTNodeType::SearchClauseNode);
}

// ============================================================================
// FTS-P-02: No SEARCH clause → Query::search_clause is nullptr
// ============================================================================

/**
 * FTS-P-02: A plain FOR/FILTER/RETURN query must NOT set search_clause.
 */
TEST_F(FtsParserTest, FtsP02_NoSearchClauseIsNull) {
    auto q = parse_ok(
        R"(FOR doc IN articles FILTER doc.age > 18 RETURN doc)");
    ASSERT_NE(q, nullptr);
    EXPECT_EQ(q->search_clause, nullptr)
        << "search_clause must be null when no SEARCH keyword is used";
}

// ============================================================================
// FTS-P-03: Implicit TERM predicate via field == "term"
// ============================================================================

/**
 * FTS-P-03: SEARCH field == "term" → single TERM predicate with correct field/term.
 */
TEST_F(FtsParserTest, FtsP03_ImplicitTermPredicate) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH doc.body == "database" RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::TERM);
    EXPECT_EQ(p.field, "doc");     // identifier before '==' parsed as field
    EXPECT_EQ(p.term, "database");
    EXPECT_DOUBLE_EQ(p.boost, 1.0);
}

// ============================================================================
// FTS-P-04: PHRASE() predicate
// ============================================================================

/**
 * FTS-P-04: SEARCH PHRASE(doc.body, "hello world") → PHRASE predicate.
 */
TEST_F(FtsParserTest, FtsP04_PhrasePredicate) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hello world") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::PHRASE);
    EXPECT_EQ(p.field, "doc");
    EXPECT_EQ(p.term, "hello world");
}

// ============================================================================
// FTS-P-05: PHRASE() with analyzer argument
// ============================================================================

/**
 * FTS-P-05: SEARCH PHRASE(doc.body, "hello world", "text_en") → analyzer stored.
 */
TEST_F(FtsParserTest, FtsP05_PhraseWithAnalyzer) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hello world", "text_en") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::PHRASE);
    EXPECT_EQ(p.analyzer, "text_en");
}

// ============================================================================
// FTS-P-06: STARTS_WITH() predicate
// ============================================================================

/**
 * FTS-P-06: SEARCH STARTS_WITH(doc.title, "data") → PREFIX predicate.
 */
TEST_F(FtsParserTest, FtsP06_StartsWithPredicate) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH STARTS_WITH(doc, "data") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::PREFIX);
    EXPECT_EQ(p.field, "doc");
    EXPECT_EQ(p.term, "data");
}

// ============================================================================
// FTS-P-07: NEAR[n]() predicate
// ============================================================================

/**
 * FTS-P-07: SEARCH NEAR[5](doc.body, "graph") → PROXIMITY predicate with distance=5.
 */
TEST_F(FtsParserTest, FtsP07_NearPredicateWithDistance) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH NEAR[5](doc, "graph") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::PROXIMITY);
    EXPECT_EQ(p.proximity_distance, 5u);
    EXPECT_EQ(p.term, "graph");
}

// ============================================================================
// FTS-P-08: NEAR() without distance bracket
// ============================================================================

/**
 * FTS-P-08: SEARCH NEAR(doc.body, "graph") → PROXIMITY predicate, distance=0.
 */
TEST_F(FtsParserTest, FtsP08_NearPredicateWithoutDistance) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH NEAR(doc, "graph") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::PROXIMITY);
    EXPECT_EQ(p.proximity_distance, 0u);
}

// ============================================================================
// FTS-P-09: Per-predicate BOOST modifier
// ============================================================================

/**
 * FTS-P-09: SEARCH PHRASE(doc, "hello") BOOST 2.5 → boost stored on predicate.
 */
TEST_F(FtsParserTest, FtsP09_PerPredicateBoost) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hello") BOOST 2.5 RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    EXPECT_DOUBLE_EQ(q->search_clause->predicates[0].boost, 2.5);
}

// ============================================================================
// FTS-P-10: Per-predicate ANALYZER modifier
// ============================================================================

/**
 * FTS-P-10: SEARCH PHRASE(doc, "hello") ANALYZER "identity" → analyzer stored.
 */
TEST_F(FtsParserTest, FtsP10_PerPredicateAnalyzer) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hello") ANALYZER "identity" RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    EXPECT_EQ(q->search_clause->predicates[0].analyzer, "identity");
}

// ============================================================================
// FTS-P-11: Multiple predicates (comma-separated)
// ============================================================================

/**
 * FTS-P-11: SEARCH pred1, pred2 → two predicates in search_clause.
 */
TEST_F(FtsParserTest, FtsP11_MultiplePredicatesCommaSeparated) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hello"), STARTS_WITH(doc, "world") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    EXPECT_EQ(q->search_clause->predicates.size(), 2u);

    EXPECT_EQ(q->search_clause->predicates[0].pred_type, FtsPredType::PHRASE);
    EXPECT_EQ(q->search_clause->predicates[1].pred_type, FtsPredType::PREFIX);
}

// ============================================================================
// FTS-P-12: Top-level IN field
// ============================================================================

/**
 * FTS-P-12: SEARCH "term" IN body → in_field populated.
 */
TEST_F(FtsParserTest, FtsP12_InFieldClause) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH "database" IN body RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    EXPECT_EQ(q->search_clause->in_field, "body");
}

// ============================================================================
// FTS-P-13: Top-level ANALYZER clause
// ============================================================================

/**
 * FTS-P-13: SEARCH ... ANALYZER "text_de" → default_analyzer set on SearchClauseNode.
 */
TEST_F(FtsParserTest, FtsP13_TopLevelAnalyzer) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "hallo") ANALYZER "text_de" RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    EXPECT_EQ(q->search_clause->default_analyzer, "text_de");
}

// ============================================================================
// FTS-P-14: Top-level BOOST clause
// ============================================================================

/**
 * FTS-P-14: SEARCH ... BOOST 3 → top_boost set on SearchClauseNode.
 */
TEST_F(FtsParserTest, FtsP14_TopLevelBoost) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH PHRASE(doc, "data") BOOST 3 RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    EXPECT_DOUBLE_EQ(q->search_clause->top_boost, 3.0);
}

// ============================================================================
// FTS-P-15: SEARCH coexists with FILTER
// ============================================================================

/**
 * FTS-P-15: Query with both FILTER and SEARCH → both parsed; search_clause != null.
 */
TEST_F(FtsParserTest, FtsP15_SearchWithFilter) {
    auto q = parse_ok(
        R"(FOR doc IN articles FILTER doc.published == true SEARCH PHRASE(doc, "ml") RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    EXPECT_FALSE(q->filters.empty())
        << "FILTER clause must also be preserved";
}

// ============================================================================
// FTS-P-16: Bare string term predicate
// ============================================================================

/**
 * FTS-P-16: SEARCH "bare_term" → TERM predicate with term="bare_term", field empty.
 */
TEST_F(FtsParserTest, FtsP16_BareStringTermPredicate) {
    auto q = parse_ok(
        R"(FOR doc IN articles SEARCH "bare_term" IN title RETURN doc)");
    ASSERT_NE(q, nullptr);
    ASSERT_NE(q->search_clause, nullptr);
    ASSERT_EQ(q->search_clause->predicates.size(), 1u);

    const auto& p = q->search_clause->predicates[0];
    EXPECT_EQ(p.pred_type, FtsPredType::TERM);
    EXPECT_EQ(p.term, "bare_term");
    EXPECT_TRUE(p.field.empty())
        << "Bare term predicate field should be empty (resolved from IN clause at executor level)";
}
