/**
 * @file test_negative_keyword_filter.cpp
 * @brief Unit tests for NegativeKeywordFilter (v2.2.0, Issue #2003)
 *
 * Coverage:
 *  - parseQuery(): positive-only, minus-prefix syntax, NOT-keyword syntax,
 *    mixed syntax, multiple negatives, NOT at end of query (dangling NOT),
 *    lone minus, empty query, case-insensitive NOT
 *  - Construction: null index is allowed
 *  - filter(): empty negative_terms passes through unchanged,
 *    null index returns error, empty candidate_pks returns empty,
 *    null index preserves original candidate list on error
 */

#include <gtest/gtest.h>
#include "search/negative_keyword_filter.h"
#include <algorithm>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// parseQuery() — positive-only queries
// ============================================================================

TEST(NegativeKeywordFilterParse, PurePositiveQuery) {
    auto pq = NegativeKeywordFilter::parseQuery("machine learning");
    EXPECT_EQ(pq.positive_query, "machine learning");
    EXPECT_TRUE(pq.negative_terms.empty());
}

TEST(NegativeKeywordFilterParse, SingleWord) {
    auto pq = NegativeKeywordFilter::parseQuery("database");
    EXPECT_EQ(pq.positive_query, "database");
    EXPECT_TRUE(pq.negative_terms.empty());
}

TEST(NegativeKeywordFilterParse, EmptyQuery) {
    auto pq = NegativeKeywordFilter::parseQuery("");
    EXPECT_EQ(pq.positive_query, "");
    EXPECT_TRUE(pq.negative_terms.empty());
}

// ============================================================================
// parseQuery() — minus-prefix syntax
// ============================================================================

TEST(NegativeKeywordFilterParse, MinusPrefixSingleNegative) {
    auto pq = NegativeKeywordFilter::parseQuery("machine learning -neural");
    EXPECT_EQ(pq.positive_query, "machine learning");
    ASSERT_EQ(pq.negative_terms.size(), 1u);
    EXPECT_EQ(pq.negative_terms[0], "neural");
}

TEST(NegativeKeywordFilterParse, MinusPrefixMultipleNegatives) {
    auto pq = NegativeKeywordFilter::parseQuery("database -slow -crash");
    EXPECT_EQ(pq.positive_query, "database");
    ASSERT_EQ(pq.negative_terms.size(), 2u);
    EXPECT_EQ(pq.negative_terms[0], "slow");
    EXPECT_EQ(pq.negative_terms[1], "crash");
}

TEST(NegativeKeywordFilterParse, MinusPrefixNegativeTermIsLowerCased) {
    auto pq = NegativeKeywordFilter::parseQuery("search -SPAM");
    EXPECT_EQ(pq.positive_query, "search");
    ASSERT_EQ(pq.negative_terms.size(), 1u);
    EXPECT_EQ(pq.negative_terms[0], "spam");
}

TEST(NegativeKeywordFilterParse, LoneMinusIsPositiveToken) {
    // A single "-" with no following characters is not a negative term
    auto pq = NegativeKeywordFilter::parseQuery("hello - world");
    EXPECT_NE(pq.positive_query.find("hello"), std::string::npos);
    EXPECT_NE(pq.positive_query.find("-"), std::string::npos);
    EXPECT_NE(pq.positive_query.find("world"), std::string::npos);
    EXPECT_TRUE(pq.negative_terms.empty());
}

// ============================================================================
// parseQuery() — NOT keyword syntax
// ============================================================================

TEST(NegativeKeywordFilterParse, NotKeywordSingleNegative) {
    auto pq = NegativeKeywordFilter::parseQuery("machine learning NOT neural");
    EXPECT_EQ(pq.positive_query, "machine learning");
    ASSERT_EQ(pq.negative_terms.size(), 1u);
    EXPECT_EQ(pq.negative_terms[0], "neural");
}

TEST(NegativeKeywordFilterParse, NotKeywordMultipleNegatives) {
    auto pq = NegativeKeywordFilter::parseQuery("database NOT crash NOT slow");
    EXPECT_EQ(pq.positive_query, "database");
    ASSERT_EQ(pq.negative_terms.size(), 2u);
    EXPECT_EQ(pq.negative_terms[0], "crash");
    EXPECT_EQ(pq.negative_terms[1], "slow");
}

TEST(NegativeKeywordFilterParse, NotKeywordCaseInsensitive) {
    auto pq1 = NegativeKeywordFilter::parseQuery("query not spam");
    auto pq2 = NegativeKeywordFilter::parseQuery("query NOT spam");
    auto pq3 = NegativeKeywordFilter::parseQuery("query Not spam");
    EXPECT_EQ(pq1.negative_terms, pq2.negative_terms);
    EXPECT_EQ(pq2.negative_terms, pq3.negative_terms);
    EXPECT_EQ(pq1.negative_terms.size(), 1u);
    EXPECT_EQ(pq1.negative_terms[0], "spam");
}

TEST(NegativeKeywordFilterParse, NotKeywordNegativeTermIsLowerCased) {
    auto pq = NegativeKeywordFilter::parseQuery("search NOT SPAM");
    ASSERT_EQ(pq.negative_terms.size(), 1u);
    EXPECT_EQ(pq.negative_terms[0], "spam");
}

TEST(NegativeKeywordFilterParse, DanglingNotAtEndIgnored) {
    // A trailing NOT with no following token is silently absorbed
    auto pq = NegativeKeywordFilter::parseQuery("search query NOT");
    EXPECT_EQ(pq.positive_query, "search query");
    EXPECT_TRUE(pq.negative_terms.empty());
}

// ============================================================================
// parseQuery() — mixed syntax
// ============================================================================

TEST(NegativeKeywordFilterParse, MixedMinusAndNot) {
    auto pq = NegativeKeywordFilter::parseQuery("search -engine NOT index");
    EXPECT_EQ(pq.positive_query, "search");
    ASSERT_EQ(pq.negative_terms.size(), 2u);
    EXPECT_EQ(pq.negative_terms[0], "engine");
    EXPECT_EQ(pq.negative_terms[1], "index");
}

TEST(NegativeKeywordFilterParse, OnlyNegativeTerms) {
    // Query with nothing positive
    auto pq = NegativeKeywordFilter::parseQuery("-spam NOT ads");
    EXPECT_EQ(pq.positive_query, "");
    ASSERT_EQ(pq.negative_terms.size(), 2u);
}

// ============================================================================
// Construction
// ============================================================================

TEST(NegativeKeywordFilterConstruct, NullIndexIsAllowed) {
    EXPECT_NO_THROW(NegativeKeywordFilter{nullptr});
}

TEST(NegativeKeywordFilterConstruct, DefaultConstructIsAllowed) {
    EXPECT_NO_THROW(NegativeKeywordFilter{});
}

TEST(NegativeKeywordFilterConstruct, StoresIndex) {
    NegativeKeywordFilter f{nullptr};
    EXPECT_EQ(f.getIndex(), nullptr);
}

TEST(NegativeKeywordFilterConstruct, ConfigDefaultMaxExcludeScan) {
    NegativeKeywordFilter f{nullptr};
    EXPECT_EQ(f.getConfig().max_exclude_scan, 100'000u);
}

TEST(NegativeKeywordFilterConstruct, ConfigCustomMaxExcludeScan) {
    NegativeKeywordFilter::Config cfg;
    cfg.max_exclude_scan = 500;
    NegativeKeywordFilter f{nullptr, cfg};
    EXPECT_EQ(f.getConfig().max_exclude_scan, 500u);
}

TEST(NegativeKeywordFilterConstruct, ConfigZeroMeansNoLimit) {
    NegativeKeywordFilter::Config cfg;
    cfg.max_exclude_scan = 0;
    NegativeKeywordFilter f{nullptr, cfg};
    EXPECT_EQ(f.getConfig().max_exclude_scan, 0u);
}

// ============================================================================
// filter() — null index
// ============================================================================

TEST(NegativeKeywordFilterFilter, NullIndexReturnsError) {
    NegativeKeywordFilter f{nullptr};
    auto [status, result] = f.filter(
        "docs", "content", {"pk1", "pk2"}, {"spam"});
    EXPECT_FALSE(status.ok);
    // Original candidates are returned so callers can decide what to do
    ASSERT_EQ(result.size(), 2u);
}

// ============================================================================
// filter() — empty inputs
// ============================================================================

TEST(NegativeKeywordFilterFilter, EmptyNegativeTermsPassesThrough) {
    NegativeKeywordFilter f{nullptr};
    std::vector<std::string> pks = {"pk1", "pk2", "pk3"};
    auto [status, result] = f.filter("docs", "content", pks, {});
    EXPECT_TRUE(status.ok);
    EXPECT_EQ(result, pks);
}

TEST(NegativeKeywordFilterFilter, EmptyCandidatePksReturnsEmpty) {
    NegativeKeywordFilter f{nullptr};
    // Even with null index, empty candidates → empty result (without error)
    auto [status, result] = f.filter("docs", "content", {}, {"spam"});
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(result.empty());
}

TEST(NegativeKeywordFilterFilter, EmptyQueryAndEmptyCandidates) {
    NegativeKeywordFilter f{nullptr};
    auto [status, result] = f.filter("docs", "content", {}, {});
    EXPECT_TRUE(status.ok);
    EXPECT_TRUE(result.empty());
}
