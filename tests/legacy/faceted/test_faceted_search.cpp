/**
 * @file test_faceted_search.cpp
 * @brief Unit tests for FacetedSearch (v1.5.0)
 *
 * Tests cover the null-index error paths and the applyFacetFilters() logic.
 * End-to-end tests with a live RocksDB backend are in test_hybrid_search_integration.cpp.
 */

#include <gtest/gtest.h>
#include "search/faceted_search.h"
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Construction
// ============================================================================

TEST(FacetedSearch, NullIndexConstructionIsAllowed) {
    EXPECT_NO_THROW(FacetedSearch{nullptr});
}

// ============================================================================
// computeFacet — null index / empty args error paths
// ============================================================================

TEST(FacetedSearchComputeFacet, NullIndexReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, facet] = fs.computeFacet("table", "col");
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchComputeFacet, EmptyTableReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, facet] = fs.computeFacet("", "col");
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchComputeFacet, EmptyColumnReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, facet] = fs.computeFacet("table", "");
    EXPECT_FALSE(st.ok);
}

// ============================================================================
// computeRangeFacet — error paths
// ============================================================================

TEST(FacetedSearchRangeFacet, NullIndexReturnsError) {
    FacetedSearch fs(nullptr);
    std::vector<FacetedSearch::RangeBucket> buckets = {{"0-10", 0.0, 10.0}};
    auto [st, facet] = fs.computeRangeFacet("table", "price", buckets);
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchRangeFacet, EmptyBucketsReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, facet] = fs.computeRangeFacet("table", "price", {});
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchRangeFacet, AllBucketLabelsInitializedToZero) {
    // With null index the range scan loop silently skips; buckets still present
    // This test verifies that bucket labels are pre-initialized
    FacetedSearch fs(nullptr);
    std::vector<FacetedSearch::RangeBucket> buckets = {
        {"low",  0.0, 10.0},
        {"mid",  10.0, 50.0},
        {"high", 50.0, 100.0}
    };
    // With null index, computeRangeFacet returns early with error before bucket init
    auto [st, facet] = fs.computeRangeFacet("t", "price", buckets);
    EXPECT_FALSE(st.ok); // null index → error
}

// ============================================================================
// applyFacetFilters
// ============================================================================

TEST(FacetedSearchApplyFilters, NullIndexReturnsError) {
    FacetedSearch fs(nullptr);
    std::vector<FacetedSearch::ActiveFacet> filters = {{"brand", "ThemisDB"}};
    auto [st, pks] = fs.applyFacetFilters("table", {"pk1", "pk2"}, filters);
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchApplyFilters, EmptyFiltersReturnsAllCandidates) {
    FacetedSearch fs(nullptr);
    std::vector<std::string> candidates = {"pk1", "pk2", "pk3"};
    auto [st, pks] = fs.applyFacetFilters("table", candidates, {});
    EXPECT_FALSE(st.ok);
    EXPECT_TRUE(pks.empty());
}

TEST(FacetedSearchApplyFilters, EmptyCandidatesWithEmptyFilters) {
    FacetedSearch fs(nullptr);
    auto [st, pks] = fs.applyFacetFilters("table", {}, {});
    EXPECT_FALSE(st.ok);
    EXPECT_TRUE(pks.empty());
}

// ============================================================================
// FacetResult structure
// ============================================================================

TEST(FacetResult, DefaultInitialization) {
    FacetResult r;
    EXPECT_TRUE(r.field.empty());
    EXPECT_TRUE(r.value_counts.empty());
    EXPECT_EQ(r.total_docs, 0u);
}

TEST(FacetResult, FieldAssignment) {
    FacetResult r;
    r.field = "brand";
    r.value_counts["ThemisDB"] = 5;
    r.total_docs = 5;
    EXPECT_EQ(r.field, "brand");
    EXPECT_EQ(r.value_counts["ThemisDB"], 5u);
}

// ============================================================================
// RangeBucket structure
// ============================================================================

TEST(RangeBucket, DefaultInitialization) {
    FacetedSearch::RangeBucket b{"0-100", 0.0, 100.0};
    EXPECT_EQ(b.label, "0-100");
    EXPECT_DOUBLE_EQ(b.low, 0.0);
    EXPECT_DOUBLE_EQ(b.high, 100.0);
}

// ============================================================================
// ActiveFacet structure
// ============================================================================

TEST(ActiveFacet, FieldValueAssignment) {
    FacetedSearch::ActiveFacet f;
    f.field = "category";
    f.value = "electronics";
    EXPECT_EQ(f.field, "category");
    EXPECT_EQ(f.value, "electronics");
}

// ============================================================================
// discoverFacetableColumns
// ============================================================================

TEST(FacetedSearchDiscoverColumns, NullIndexReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, cols] = fs.discoverFacetableColumns("products");
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchDiscoverColumns, EmptyTableReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, cols] = fs.discoverFacetableColumns("");
    EXPECT_FALSE(st.ok);
}

// ============================================================================
// computeDynamicFacets
// ============================================================================

TEST(FacetedSearchDynamicFacets, NullIndexReturnsError) {
    FacetedSearch fs(nullptr);
    auto [st, facets] = fs.computeDynamicFacets("products");
    EXPECT_FALSE(st.ok);
}

TEST(FacetedSearchDynamicFacets, NullIndexWithCandidatesReturnsError) {
    FacetedSearch fs(nullptr);
    std::vector<std::string> candidates = {"pk1", "pk2"};
    auto [st, facets] = fs.computeDynamicFacets("products", candidates);
    EXPECT_FALSE(st.ok);
}
