/**
 * @file test_search_future_interfaces.cpp
 * @brief Unit tests for search module Phase 5 interfaces (v2.4.0):
 *        ConversationalSearch, FederatedSearch, SearchResultStream.
 *
 * All tests operate without live index infrastructure: null HybridSearch
 * pointers are used to validate config, API surface, and stateless helpers.
 * Integration with live indices is exercised in test_hybrid_search_integration.cpp.
 */

#include <gtest/gtest.h>
#include "search/conversational_search.h"
#include "search/federated_search.h"
#include "search/search_result_stream.h"
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// ConversationalSearch — Config validation
// ============================================================================

TEST(ConversationalSearchConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(ConversationalSearch(nullptr));
}

TEST(ConversationalSearchConfig, ZeroMaxHistoryThrows) {
    ConversationalSearch::Config cfg;
    cfg.max_history = 0;
    EXPECT_THROW(ConversationalSearch(nullptr, cfg), std::invalid_argument);
}

TEST(ConversationalSearchConfig, ZeroContextWindowIsValid) {
    ConversationalSearch::Config cfg;
    cfg.context_window = 0;
    EXPECT_NO_THROW(ConversationalSearch(nullptr, cfg));
}

TEST(ConversationalSearchConfig, ConfigRoundtrip) {
    ConversationalSearch::Config cfg;
    cfg.context_window    = 5;
    cfg.max_history       = 100;
    cfg.context_separator = " | ";
    ConversationalSearch cs(nullptr, cfg);
    EXPECT_EQ(cs.getConfig().context_window,    5u);
    EXPECT_EQ(cs.getConfig().max_history,       100u);
    EXPECT_EQ(cs.getConfig().context_separator, " | ");
}

TEST(ConversationalSearchConfig, SetConfigZeroMaxHistoryThrows) {
    ConversationalSearch cs(nullptr);
    ConversationalSearch::Config bad;
    bad.max_history = 0;
    EXPECT_THROW(cs.setConfig(bad), std::invalid_argument);
}

// ============================================================================
// ConversationalSearch — Struct defaults
// ============================================================================

TEST(ConversationalSearchTurn, DefaultTurnIsEmpty) {
    ConversationalSearch::Turn t;
    EXPECT_TRUE(t.query.empty());
    EXPECT_TRUE(t.reformulated_query.empty());
    EXPECT_TRUE(t.results.empty());
}

// ============================================================================
// ConversationalSearch — Null-engine safety
// ============================================================================

TEST(ConversationalSearchNullEngine, SearchReturnsEmpty) {
    ConversationalSearch cs(nullptr);
    auto r = cs.search("hello");
    EXPECT_TRUE(r.empty());
}

TEST(ConversationalSearchNullEngine, EmptyQueryReturnsEmpty) {
    ConversationalSearch cs(nullptr);
    auto r = cs.search("");
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(cs.historySize(), 0u); // empty query: not added to history
}

// ============================================================================
// ConversationalSearch — reformulate
// ============================================================================

TEST(ConversationalSearchReformulate, EmptyHistoryReturnsQueryUnchanged) {
    ConversationalSearch cs(nullptr);
    EXPECT_EQ(cs.reformulate("foo"), "foo");
}

TEST(ConversationalSearchReformulate, ZeroWindowReturnsQueryUnchanged) {
    ConversationalSearch::Config cfg;
    cfg.context_window = 0;
    ConversationalSearch cs(nullptr, cfg);
    cs.search("first");
    EXPECT_EQ(cs.reformulate("second"), "second");
}

TEST(ConversationalSearchReformulate, SinglePriorTurnPrepended) {
    ConversationalSearch::Config cfg;
    cfg.context_window = 2;
    ConversationalSearch cs(nullptr, cfg);
    cs.search("machine learning");
    const std::string r = cs.reformulate("overfitting");
    EXPECT_EQ(r, "machine learning overfitting");
}

TEST(ConversationalSearchReformulate, WindowLimitedToContextWindow) {
    ConversationalSearch::Config cfg;
    cfg.context_window = 1;
    ConversationalSearch cs(nullptr, cfg);
    cs.search("first");
    cs.search("second");
    // Only "second" should be in context (window=1)
    const std::string r = cs.reformulate("third");
    EXPECT_EQ(r, "second third");
    EXPECT_EQ(r.find("first"), std::string::npos);
}

TEST(ConversationalSearchReformulate, MultipleHistoryTurns) {
    ConversationalSearch::Config cfg;
    cfg.context_window    = 3;
    cfg.context_separator = " ";
    ConversationalSearch cs(nullptr, cfg);
    cs.search("a");
    cs.search("b");
    cs.search("c");
    const std::string r = cs.reformulate("d");
    EXPECT_EQ(r, "a b c d");
}

// ============================================================================
// ConversationalSearch — History management
// ============================================================================

TEST(ConversationalSearchHistory, HistoryGrowsWithSearchCalls) {
    ConversationalSearch cs(nullptr);
    cs.search("q1");
    cs.search("q2");
    cs.search("q3");
    EXPECT_EQ(cs.historySize(), 3u);
}

TEST(ConversationalSearchHistory, ClearHistoryResetsToZero) {
    ConversationalSearch cs(nullptr);
    cs.search("q1");
    cs.search("q2");
    cs.clearHistory();
    EXPECT_EQ(cs.historySize(), 0u);
}

TEST(ConversationalSearchHistory, MaxHistoryEvictsOldest) {
    ConversationalSearch::Config cfg;
    cfg.max_history = 2;
    ConversationalSearch cs(nullptr, cfg);
    cs.search("a");
    cs.search("b");
    cs.search("c"); // evicts "a"
    EXPECT_EQ(cs.historySize(), 2u);
    EXPECT_EQ(cs.getHistory().front().query, "b");
    EXPECT_EQ(cs.getHistory().back().query,  "c");
}

TEST(ConversationalSearchHistory, TurnStoresOriginalAndReformulated) {
    ConversationalSearch::Config cfg;
    cfg.context_window = 1;
    ConversationalSearch cs(nullptr, cfg);
    cs.search("first");
    cs.search("second");
    const auto& last = cs.getHistory().back();
    EXPECT_EQ(last.query, "second");
    EXPECT_EQ(last.reformulated_query, "first second");
}

// ============================================================================
// FederatedSearch — Config validation
// ============================================================================

TEST(FederatedSearchConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(FederatedSearch());
}

TEST(FederatedSearchConfig, ZeroKThrows) {
    FederatedSearch::Config cfg;
    cfg.k = 0;
    EXPECT_THROW(FederatedSearch(cfg), std::invalid_argument);
}

TEST(FederatedSearchConfig, ZeroRrfKThrows) {
    FederatedSearch::Config cfg;
    cfg.rrf_k = 0.0;
    EXPECT_THROW(FederatedSearch(cfg), std::invalid_argument);
}

TEST(FederatedSearchConfig, NegativeRrfKThrows) {
    FederatedSearch::Config cfg;
    cfg.rrf_k = -1.0;
    EXPECT_THROW(FederatedSearch(cfg), std::invalid_argument);
}

TEST(FederatedSearchConfig, SetConfigZeroKThrows) {
    FederatedSearch fs;
    FederatedSearch::Config bad;
    bad.k = 0;
    EXPECT_THROW(fs.setConfig(bad), std::invalid_argument);
}

TEST(FederatedSearchConfig, ConfigRoundtrip) {
    FederatedSearch::Config cfg;
    cfg.k     = 20;
    cfg.rrf_k = 30.0;
    FederatedSearch fs(cfg);
    EXPECT_EQ(fs.getConfig().k, 20u);
    EXPECT_DOUBLE_EQ(fs.getConfig().rrf_k, 30.0);
}

// ============================================================================
// FederatedSearch — Struct defaults
// ============================================================================

TEST(FederatedSearchResult, DefaultResultIsZero) {
    FederatedSearch::Result r;
    EXPECT_TRUE(r.document_id.empty());
    EXPECT_TRUE(r.tenant_id.empty());
    EXPECT_DOUBLE_EQ(r.score, 0.0);
    EXPECT_DOUBLE_EQ(r.bm25_score, 0.0);
    EXPECT_DOUBLE_EQ(r.vector_score, 0.0);
}

TEST(FederatedSearchStats, DefaultStatsIsZero) {
    FederatedSearch::TenantStats s;
    EXPECT_TRUE(s.tenant_id.empty());
    EXPECT_EQ(s.results_count, 0u);
    EXPECT_FALSE(s.skipped);
}

// ============================================================================
// FederatedSearch — Tenant management
// ============================================================================

TEST(FederatedSearchTenants, InitiallyEmpty) {
    FederatedSearch fs;
    EXPECT_EQ(fs.tenantCount(), 0u);
}

TEST(FederatedSearchTenants, RegisterIncreasesCount) {
    FederatedSearch fs;
    fs.registerTenant("A", nullptr);
    fs.registerTenant("B", nullptr);
    EXPECT_EQ(fs.tenantCount(), 2u);
}

TEST(FederatedSearchTenants, RemoveDecreasesCount) {
    FederatedSearch fs;
    fs.registerTenant("A", nullptr);
    fs.registerTenant("B", nullptr);
    fs.removeTenant("A");
    EXPECT_EQ(fs.tenantCount(), 1u);
}

TEST(FederatedSearchTenants, RemoveUnknownTenantIsNoOp) {
    FederatedSearch fs;
    EXPECT_NO_THROW(fs.removeTenant("non_existent"));
    EXPECT_EQ(fs.tenantCount(), 0u);
}

TEST(FederatedSearchTenants, DefaultWeightIsOne) {
    FederatedSearch fs;
    fs.registerTenant("A", nullptr);
    EXPECT_DOUBLE_EQ(fs.getTenantWeight("A"), 1.0);
}

TEST(FederatedSearchTenants, SetWeightClampedToUnitInterval) {
    FederatedSearch fs;
    fs.registerTenant("A", nullptr);
    fs.setTenantWeight("A", -0.5);
    EXPECT_DOUBLE_EQ(fs.getTenantWeight("A"), 0.0);
    fs.setTenantWeight("A", 2.0);
    EXPECT_DOUBLE_EQ(fs.getTenantWeight("A"), 1.0);
}

// ============================================================================
// FederatedSearch — Null/empty safety
// ============================================================================

TEST(FederatedSearchNullSafety, SearchWithNoTenantsReturnsEmpty) {
    FederatedSearch fs;
    auto r = fs.search("hello");
    EXPECT_TRUE(r.empty());
}

TEST(FederatedSearchNullSafety, SearchWithNullTenantReturnsEmpty) {
    FederatedSearch fs;
    fs.registerTenant("A", nullptr);
    auto r = fs.search("hello");
    EXPECT_TRUE(r.empty());
}

// ============================================================================
// FederatedSearch — mergeTenantResults (public unit-testable helper)
// ============================================================================

static HybridSearch::Result makeHSResult(const std::string& id,
                                          double hybrid = 0.5,
                                          double bm25   = 0.3,
                                          double vec    = 0.2) {
    HybridSearch::Result r;
    r.document_id  = id;
    r.hybrid_score = hybrid;
    r.bm25_score   = bm25;
    r.vector_score = vec;
    return r;
}

TEST(FederatedSearchMerge, EmptyInputReturnsEmpty) {
    FederatedSearch fs;
    auto r = fs.mergeTenantResults({});
    EXPECT_TRUE(r.empty());
}

TEST(FederatedSearchMerge, SingleTenantPreservesOrder) {
    FederatedSearch::Config cfg;
    cfg.k = 10;
    FederatedSearch fs(cfg);
    fs.registerTenant("t1", nullptr);

    std::unordered_map<std::string, std::vector<HybridSearch::Result>> input;
    input["t1"] = {makeHSResult("d1"), makeHSResult("d2"), makeHSResult("d3")};

    auto results = fs.mergeTenantResults(input);
    EXPECT_EQ(results.size(), 3u);
    // All results must come from tenant "t1"
    for (const auto& r : results) {
        EXPECT_EQ(r.tenant_id, "t1");
    }
    // Results are sorted by score descending
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].score, results[i].score);
    }
}

TEST(FederatedSearchMerge, MultiTenantResultsLabeledCorrectly) {
    FederatedSearch::Config cfg;
    cfg.k = 10;
    FederatedSearch fs(cfg);
    fs.registerTenant("tA", nullptr);
    fs.registerTenant("tB", nullptr);

    std::unordered_map<std::string, std::vector<HybridSearch::Result>> input;
    input["tA"] = {makeHSResult("doc_a1"), makeHSResult("doc_a2")};
    input["tB"] = {makeHSResult("doc_b1")};

    auto results = fs.mergeTenantResults(input);
    EXPECT_EQ(results.size(), 3u);

    // Every result must have a non-empty tenant_id
    for (const auto& r : results) {
        EXPECT_FALSE(r.tenant_id.empty());
        EXPECT_FALSE(r.document_id.empty());
    }
}

TEST(FederatedSearchMerge, KLimitsResultCount) {
    FederatedSearch::Config cfg;
    cfg.k = 2;
    FederatedSearch fs(cfg);
    fs.registerTenant("t1", nullptr);

    std::unordered_map<std::string, std::vector<HybridSearch::Result>> input;
    input["t1"] = {makeHSResult("d1"), makeHSResult("d2"), makeHSResult("d3"),
                   makeHSResult("d4"), makeHSResult("d5")};

    auto results = fs.mergeTenantResults(input);
    EXPECT_EQ(results.size(), 2u);
}

// ============================================================================
// SearchResultStream — Config validation
// ============================================================================

TEST(SearchResultStreamConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(SearchResultStream(nullptr));
}

TEST(SearchResultStreamConfig, ZeroTotalKThrows) {
    SearchResultStream::Config cfg;
    cfg.total_k = 0;
    EXPECT_THROW(SearchResultStream(nullptr, cfg), std::invalid_argument);
}

TEST(SearchResultStreamConfig, ZeroPageSizeThrows) {
    SearchResultStream::Config cfg;
    cfg.page_size = 0;
    EXPECT_THROW(SearchResultStream(nullptr, cfg), std::invalid_argument);
}

TEST(SearchResultStreamConfig, SetConfigZeroTotalKThrows) {
    SearchResultStream ss(nullptr);
    SearchResultStream::Config bad;
    bad.total_k = 0;
    EXPECT_THROW(ss.setConfig(bad), std::invalid_argument);
}

TEST(SearchResultStreamConfig, ConfigRoundtrip) {
    SearchResultStream::Config cfg;
    cfg.total_k   = 5000;
    cfg.page_size = 200;
    SearchResultStream ss(nullptr, cfg);
    EXPECT_EQ(ss.getConfig().total_k,   5000u);
    EXPECT_EQ(ss.getConfig().page_size, 200u);
}

// ============================================================================
// SearchResultStream — Null-engine safety
// ============================================================================

TEST(SearchResultStreamNullEngine, InitiallyHasNoMore) {
    SearchResultStream ss(nullptr);
    EXPECT_FALSE(ss.hasMore());
}

TEST(SearchResultStreamNullEngine, OpenReturnsEmptyStream) {
    SearchResultStream ss(nullptr);
    ss.open("machine learning");
    EXPECT_FALSE(ss.hasMore());
    EXPECT_EQ(ss.totalResults(), 0u);
}

TEST(SearchResultStreamNullEngine, NextPageOnEmptyReturnsEmpty) {
    SearchResultStream ss(nullptr);
    ss.open("query");
    auto page = ss.nextPage();
    EXPECT_TRUE(page.empty());
}

// ============================================================================
// SearchResultStream — Cursor / pagination logic
// ============================================================================

TEST(SearchResultStreamPagination, InitialCursorIsZero) {
    SearchResultStream ss(nullptr);
    EXPECT_EQ(ss.cursorPosition(), 0u);
}

TEST(SearchResultStreamPagination, CloseResetsStateCompletely) {
    SearchResultStream ss(nullptr);
    ss.close();
    EXPECT_EQ(ss.totalResults(), 0u);
    EXPECT_EQ(ss.cursorPosition(), 0u);
    EXPECT_FALSE(ss.hasMore());
}

TEST(SearchResultStreamPagination, ResetRewoundsCursorToZero) {
    SearchResultStream ss(nullptr);
    // After open with null engine, stream is empty; reset should not throw
    ss.open("hello");
    ss.reset();
    EXPECT_EQ(ss.cursorPosition(), 0u);
}

// ============================================================================
// SearchResultStream — forEachResult with empty stream
// ============================================================================

TEST(SearchResultStreamForEach, EmptyStreamCallbackNeverInvoked) {
    SearchResultStream ss(nullptr);
    int count = 0;
    ss.forEachResult([&](const HybridSearch::Result&) {
        ++count;
        return true;
    });
    EXPECT_EQ(count, 0);
}

TEST(SearchResultStreamForEach, NullCallbackIsNoOp) {
    SearchResultStream ss(nullptr);
    EXPECT_NO_THROW(ss.forEachResult(nullptr));
}

// ============================================================================
// SearchResultStream — Open with empty query
// ============================================================================

TEST(SearchResultStreamOpen, EmptyQueryProducesEmptyStream) {
    SearchResultStream ss(nullptr);
    ss.open("");
    EXPECT_FALSE(ss.hasMore());
    EXPECT_EQ(ss.totalResults(), 0u);
}

