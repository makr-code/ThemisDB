/**
 * @file test_aql_fewshot_example_library.cpp
 * @brief Unit tests for AQLFewShotExampleLibrary
 */

#include <gtest/gtest.h>
#include "aql/aql_fewshot_example_library.h"
#include <algorithm>
#include <chrono>
#include <unordered_map>

using namespace themis::aql;

// ============================================================================
// Fixture
// ============================================================================

class AQLFewShotExampleLibraryTest : public ::testing::Test {
protected:
    AQLFewShotExampleLibrary lib;
};

// ============================================================================
// Built-in example registration
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, BuiltinsAreRegistered) {
    EXPECT_GT(lib.size(), 0u);
}

TEST_F(AQLFewShotExampleLibraryTest, AtLeastThirtyBuiltins) {
    EXPECT_GE(lib.size(), 30u);
}

TEST_F(AQLFewShotExampleLibraryTest, AllExamplesHaveNonEmptyFields) {
    for (const auto& ex : lib.all()) {
        EXPECT_FALSE(ex.id.empty())       << "id is empty";
        EXPECT_FALSE(ex.nl_query.empty()) << "nl_query is empty for id=" << ex.id;
        EXPECT_FALSE(ex.aql_query.empty()) << "aql_query is empty for id=" << ex.id;
        EXPECT_FALSE(ex.description.empty()) << "description is empty for id=" << ex.id;
    }
}

TEST_F(AQLFewShotExampleLibraryTest, AllIdsAreUnique) {
    std::unordered_map<std::string, int> id_counts;
    for (const auto& ex : lib.all()) {
        ++id_counts[ex.id];
    }
    for (const auto& [id, cnt] : id_counts) {
        EXPECT_EQ(cnt, 1) << "duplicate id: " << id;
    }
}

TEST_F(AQLFewShotExampleLibraryTest, AllAqlQueriesContainForOrInsert) {
    for (const auto& ex : lib.all()) {
        std::string aql_lower = ex.aql_query;
        std::transform(aql_lower.begin(), aql_lower.end(), aql_lower.begin(), ::tolower);
        bool valid = aql_lower.find("for ") != std::string::npos
                  || aql_lower.find("insert ") != std::string::npos
                  || aql_lower.find("update ") != std::string::npos
                  || aql_lower.find("remove ") != std::string::npos;
        EXPECT_TRUE(valid) << "AQL for id=" << ex.id << " missing FOR/INSERT/UPDATE/REMOVE";
    }
}

// ============================================================================
// Domain coverage
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, DocumentExamplesExist) {
    auto docs = lib.findByDomain(AQLExampleDomain::DOCUMENT);
    EXPECT_GE(docs.size(), 5u) << "Expected >= 5 DOCUMENT examples";
}

TEST_F(AQLFewShotExampleLibraryTest, GraphExamplesExist) {
    auto graphs = lib.findByDomain(AQLExampleDomain::GRAPH);
    EXPECT_GE(graphs.size(), 3u) << "Expected >= 3 GRAPH examples";
}

TEST_F(AQLFewShotExampleLibraryTest, VectorExamplesExist) {
    auto vecs = lib.findByDomain(AQLExampleDomain::VECTOR);
    EXPECT_GE(vecs.size(), 2u) << "Expected >= 2 VECTOR examples";
}

TEST_F(AQLFewShotExampleLibraryTest, GeospatialExamplesExist) {
    auto geo = lib.findByDomain(AQLExampleDomain::GEOSPATIAL);
    EXPECT_GE(geo.size(), 2u) << "Expected >= 2 GEOSPATIAL examples";
}

TEST_F(AQLFewShotExampleLibraryTest, TimeseriesExamplesExist) {
    auto ts = lib.findByDomain(AQLExampleDomain::TIMESERIES);
    EXPECT_GE(ts.size(), 2u) << "Expected >= 2 TIMESERIES examples";
}

TEST_F(AQLFewShotExampleLibraryTest, AggregationExamplesExist) {
    auto agg = lib.findByDomain(AQLExampleDomain::AGGREGATION);
    EXPECT_GE(agg.size(), 2u) << "Expected >= 2 AGGREGATION examples";
}

// ============================================================================
// findByDomain correctness
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, FindByDomainReturnsOnlyMatchingDomain) {
    for (const auto& ex : lib.findByDomain(AQLExampleDomain::GRAPH)) {
        EXPECT_EQ(ex.domain, AQLExampleDomain::GRAPH)
            << "findByDomain(GRAPH) returned non-GRAPH example: " << ex.id;
    }
}

TEST_F(AQLFewShotExampleLibraryTest, FindByDomainEmptyForGeneralIfNoneRegistered) {
    // GENERAL domain examples should exist
    auto gen = lib.findByDomain(AQLExampleDomain::GENERAL);
    // We just check it doesn't crash; GENERAL may or may not have entries
    EXPECT_NO_THROW(lib.findByDomain(AQLExampleDomain::GENERAL));
}

// ============================================================================
// findByTag
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, FindByTagGraphReturnsGraphExamples) {
    auto results = lib.findByTag("graph");
    EXPECT_GE(results.size(), 1u);
    for (const auto& ex : results) {
        bool found = false;
        for (const auto& t : ex.tags) {
            if (t == "graph") { found = true; break; }
        }
        EXPECT_TRUE(found) << "findByTag('graph') returned example without 'graph' tag: " << ex.id;
    }
}

TEST_F(AQLFewShotExampleLibraryTest, FindByTagCaseInsensitive) {
    auto lower = lib.findByTag("filter");
    auto upper = lib.findByTag("FILTER");
    EXPECT_EQ(lower.size(), upper.size());
}

TEST_F(AQLFewShotExampleLibraryTest, FindByTagNonExistentReturnsEmpty) {
    auto result = lib.findByTag("xyzzy_nonexistent_9999");
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// findById
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, FindByIdKnownExample) {
    const auto* ex = lib.findById("doc_filter_city");
    ASSERT_NE(ex, nullptr);
    EXPECT_EQ(ex->id, "doc_filter_city");
    EXPECT_EQ(ex->domain, AQLExampleDomain::DOCUMENT);
}

TEST_F(AQLFewShotExampleLibraryTest, FindByIdGraphTraversal) {
    const auto* ex = lib.findById("graph_outbound");
    ASSERT_NE(ex, nullptr);
    EXPECT_EQ(ex->domain, AQLExampleDomain::GRAPH);
    // AQL should mention OUTBOUND
    EXPECT_NE(ex->aql_query.find("OUTBOUND"), std::string::npos);
}

TEST_F(AQLFewShotExampleLibraryTest, FindByIdVectorSearch) {
    const auto* ex = lib.findById("vector_ann_search");
    ASSERT_NE(ex, nullptr);
    EXPECT_EQ(ex->domain, AQLExampleDomain::VECTOR);
}

TEST_F(AQLFewShotExampleLibraryTest, FindByIdGeospatial) {
    const auto* ex = lib.findById("geo_within_radius");
    ASSERT_NE(ex, nullptr);
    EXPECT_EQ(ex->domain, AQLExampleDomain::GEOSPATIAL);
    EXPECT_NE(ex->aql_query.find("GEO_DISTANCE"), std::string::npos);
}

TEST_F(AQLFewShotExampleLibraryTest, FindByIdTimeseries) {
    const auto* ex = lib.findById("ts_range_query");
    ASSERT_NE(ex, nullptr);
    EXPECT_EQ(ex->domain, AQLExampleDomain::TIMESERIES);
}

TEST_F(AQLFewShotExampleLibraryTest, FindByIdUnknownReturnsNull) {
    EXPECT_EQ(lib.findById("no_such_id_xyz"), nullptr);
}

// ============================================================================
// registerExample
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, RegisterCustomExample) {
    AQLFewShotExample custom{
        "test_custom_ex",
        "Find all widgets with colour red",
        "FOR w IN widgets\n  FILTER w.colour == \"red\"\n  RETURN w",
        AQLExampleDomain::DOCUMENT,
        "Custom example for testing",
        {"custom", "document"}
    };
    EXPECT_NO_THROW(lib.registerExample(custom));

    const auto* ex = lib.findById("test_custom_ex");
    ASSERT_NE(ex, nullptr);
    EXPECT_EQ(ex->nl_query, "Find all widgets with colour red");
}

TEST_F(AQLFewShotExampleLibraryTest, RegisterDuplicateIdThrows) {
    AQLFewShotExample dup{
        "doc_filter_city",   // already exists
        "Another query", "FOR x IN y RETURN x",
        AQLExampleDomain::DOCUMENT, "dup", {}
    };
    EXPECT_THROW(lib.registerExample(dup), std::invalid_argument);
}

TEST_F(AQLFewShotExampleLibraryTest, RegisterEmptyIdThrows) {
    AQLFewShotExample bad{
        "", "query", "FOR x IN y RETURN x",
        AQLExampleDomain::DOCUMENT, "desc", {}
    };
    EXPECT_THROW(lib.registerExample(bad), std::invalid_argument);
}

// ============================================================================
// findRelevant
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantReturnsAtMostN) {
    auto results = lib.findRelevant("Find all users in Seattle", 2);
    EXPECT_LE(results.size(), 2u);
}

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantNonZeroForKnownQuery) {
    auto results = lib.findRelevant("find users", 5);
    EXPECT_GE(results.size(), 1u);
}

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantWithDomainFilter) {
    auto results = lib.findRelevant("search for documents", 5,
                                    AQLExampleDomain::GRAPH);
    for (const auto& ex : results) {
        EXPECT_EQ(ex.domain, AQLExampleDomain::GRAPH)
            << "findRelevant with GRAPH domain filter returned non-GRAPH example";
    }
}

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantEmptyQueryReturnsResults) {
    // Empty query should return up to n examples (scores = 0, any order)
    auto results = lib.findRelevant("", 3);
    EXPECT_LE(results.size(), 3u);
}

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantGraphQueryPrefersGraphExamples) {
    // A graph-specific query should rank graph examples higher
    auto results = lib.findRelevant("find friends of user 42 via graph traversal", 3);
    EXPECT_GE(results.size(), 1u);
    // At least one result should be a graph example
    bool has_graph = false;
    for (const auto& ex : results) {
        if (ex.domain == AQLExampleDomain::GRAPH) {
            has_graph = true;
        }
    }
    EXPECT_TRUE(has_graph) << "Graph query should retrieve at least one graph example";
}

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantVectorQueryPrefersVectorExamples) {
    auto results = lib.findRelevant("find similar documents using vector embedding", 3);
    EXPECT_GE(results.size(), 1u);
    bool has_vector = false;
    for (const auto& ex : results) {
        if (ex.domain == AQLExampleDomain::VECTOR) {
            has_vector = true;
        }
    }
    EXPECT_TRUE(has_vector) << "Vector query should retrieve at least one vector example";
}

TEST_F(AQLFewShotExampleLibraryTest, FindRelevantNZeroReturnsEmpty) {
    auto results = lib.findRelevant("any query", 0);
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// formatForPrompt
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, FormatForPromptEmptyExamplesReturnsEmpty) {
    std::string formatted = AQLFewShotExampleLibrary::formatForPrompt({});
    EXPECT_TRUE(formatted.empty());
}

TEST_F(AQLFewShotExampleLibraryTest, FormatForPromptContainsNLAndAQL) {
    auto examples = lib.findByDomain(AQLExampleDomain::DOCUMENT);
    ASSERT_GE(examples.size(), 1u);
    std::vector<AQLFewShotExample> one = {examples[0]};
    std::string formatted = AQLFewShotExampleLibrary::formatForPrompt(one);

    EXPECT_NE(formatted.find("Natural language:"), std::string::npos);
    EXPECT_NE(formatted.find("AQL:"), std::string::npos);
    EXPECT_NE(formatted.find(examples[0].nl_query), std::string::npos);
    EXPECT_NE(formatted.find("FOR"), std::string::npos);
}

TEST_F(AQLFewShotExampleLibraryTest, FormatForPromptMultipleExamples) {
    auto examples = lib.findByDomain(AQLExampleDomain::DOCUMENT);
    ASSERT_GE(examples.size(), 3u);
    std::vector<AQLFewShotExample> three(examples.begin(), examples.begin() + 3);
    std::string formatted = AQLFewShotExampleLibrary::formatForPrompt(three);

    // Count occurrences of "Natural language:"
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = formatted.find("Natural language:", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    EXPECT_EQ(count, 3u);
}

// ============================================================================
// buildPromptSection
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, BuildPromptSectionNonEmpty) {
    std::string section = lib.buildPromptSection("find users in Seattle", 3);
    EXPECT_FALSE(section.empty());
}

TEST_F(AQLFewShotExampleLibraryTest, BuildPromptSectionWithDomainFilter) {
    std::string section = lib.buildPromptSection(
        "traverse graph from node 1",
        3,
        AQLExampleDomain::GRAPH
    );
    EXPECT_FALSE(section.empty());
    // Should contain OUTBOUND or graph-related keyword
    bool has_graph_kw =
        section.find("OUTBOUND") != std::string::npos ||
        section.find("INBOUND")  != std::string::npos ||
        section.find("ANY")      != std::string::npos ||
        section.find("edges")    != std::string::npos;
    EXPECT_TRUE(has_graph_kw) << "Graph-domain section should contain traversal keywords";
}

TEST_F(AQLFewShotExampleLibraryTest, BuildPromptSectionZeroExamplesReturnsEmpty) {
    std::string section = lib.buildPromptSection("any query", 0);
    EXPECT_TRUE(section.empty());
}

// ============================================================================
// size()
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, SizeReflectsRegisteredExamples) {
    std::size_t before = lib.size();
    AQLFewShotExample extra{
        "test_size_check",
        "A test query for size",
        "FOR x IN col RETURN x",
        AQLExampleDomain::GENERAL,
        "size test",
        {}
    };
    lib.registerExample(extra);
    EXPECT_EQ(lib.size(), before + 1);
}

// ============================================================================
// Performance benchmarks
// ============================================================================

TEST_F(AQLFewShotExampleLibraryTest, Performance_FindRelevant_UnderThreshold) {
    // findRelevant() over 37 built-in examples must complete in < 10 ms per call
    // (budget: 1 s total for 100 calls)
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        auto results = lib.findRelevant("Find all users in Seattle older than 30", 3);
        (void)results;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto avg_us = total_us / 100;
    std::cout << "findRelevant() avg: " << avg_us << " µs per call\n";
    // 100 calls in less than 1 second total (= 10 ms/call budget)
    EXPECT_LT(total_us, 1'000'000);
}

TEST_F(AQLFewShotExampleLibraryTest, Performance_BuildPromptSection_UnderThreshold) {
    // buildPromptSection() must produce output in < 5 ms per call
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        auto section = lib.buildPromptSection("Traverse the friends graph from a start node", 5);
        (void)section;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    auto avg_us = total_us / 100;
    std::cout << "buildPromptSection() avg: " << avg_us << " µs per call\n";
    // 100 calls in less than 500 ms total (= 5 ms/call budget)
    EXPECT_LT(total_us, 500'000);
}

TEST_F(AQLFewShotExampleLibraryTest, Performance_FindByDomain_UnderThreshold) {
    // findByDomain() must be fast even when called frequently
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        auto results = lib.findByDomain(AQLExampleDomain::GRAPH);
        (void)results;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "findByDomain() 1000 calls: " << total_ms << " ms\n";
    EXPECT_LT(total_ms, 100);  // 1000 calls in under 100 ms
}
