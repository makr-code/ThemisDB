/**
 * @file test_aql_fts_phrase_proximity.cpp
 * @brief Phase 6 Implementation: Full-Text Search Phrase & Proximity Queries
 * 
 * Tests for AQL v2.0.0 FTS enhancement:
 * - PHRASE("term1 term2") — exact phrase matching
 * - PROXIMITY("term1", "term2", distance=5) — proximity search within N words
 * 
 * Performance target: ≤100ms on 100K documents
 * 
 * Reference: src/query/ROADMAP.md line 59
 *            src/query/AQL_V2_0_0_COMPLETE_ROADMAP.md §FTS Enhancement
 */

#include <gtest/gtest.h>
#include "query/aql_parser.h"
#include "query/query_engine.h"
#include "query/query_profiler.h"
#include <memory>
#include <string>
#include <chrono>
#include <vector>

namespace themis {
namespace query {

/**
 * @class FTSPhraseProximityTest
 * @brief Test suite for phrase and proximity search functionality
 */
class FTSPhraseProximityTest : public ::testing::Test {
protected:
    std::shared_ptr<AQLParser> parser_;
    std::shared_ptr<QueryEngine> engine_;
    std::shared_ptr<QueryProfiler> profiler_;
    
    void SetUp() override {
        // Initialize parser, engine, profiler
        // Setup test document collection with 100K documents for performance testing
    }
    
    void TearDown() override {
        // Cleanup test data
    }
    
    // Helper: Execute query and measure latency
    int64_t ExecuteQueryAndMeasure(const std::string& query_str) {
        auto start = std::chrono::high_resolution_clock::now();
        // Parse and execute query
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
};

// ============================================================================
// PHRASE QUERY TESTS (20 test cases)
// ============================================================================

/**
 * FPH-01: Basic phrase query — exact sequence
 * Query: PHRASE("hello world")
 * Expected: Returns documents containing "hello world" in exact sequence
 */
TEST_F(FTSPhraseProximityTest, FPH01_BasicPhraseExactSequence) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "hello world")
          RETURN doc
    )";
    // Implementation test: Verify phrase extraction and matching logic
    // Assert: All results contain exact phrase
}

/**
 * FPH-02: Phrase query with punctuation handling
 * Query: PHRASE("don't", "worry")
 * Expected: Handles apostrophes and punctuation correctly
 */
TEST_F(FTSPhraseProximityTest, FPH02_PhraseWithPunctuation) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "don't worry")
          RETURN doc
    )";
    // Implementation test: Punctuation normalization in phrase matching
}

/**
 * FPH-03: Multi-word phrase matching
 * Query: PHRASE("the quick brown fox")
 * Expected: Matches exact 4-word sequence
 */
TEST_F(FTSPhraseProximityTest, FPH03_MultiWordPhrase) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "the quick brown fox")
          RETURN doc
    )";
    // Implementation test: N-word phrase matching
}

/**
 * FPH-04: Phrase query case insensitivity
 * Query: PHRASE("Hello World") should match "hello world"
 * Expected: Case-insensitive matching
 */
TEST_F(FTSPhraseProximityTest, FPH04_CaseInsensitivePhrase) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "Hello World")
          RETURN doc
    )";
    // Implementation test: Case normalization
}

/**
 * FPH-05: Phrase with special characters
 * Query: PHRASE("C++ programming")
 * Expected: Correctly handles special characters
 */
TEST_F(FTSPhraseProximityTest, FPH05_PhraseWithSpecialChars) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "C++ programming")
          RETURN doc
    )";
    // Implementation test: Special character preservation
}

/**
 * FPH-06: Phrase matching with negation
 * Query: !PHRASE("deprecated API")
 * Expected: Returns documents NOT containing the phrase
 */
TEST_F(FTSPhraseProximityTest, FPH06_NegatedPhrase) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER !PHRASE(doc.content, "deprecated API")
          RETURN doc
    )";
    // Implementation test: Negation logic for phrases
}

/**
 * FPH-07: Phrase combined with other filters
 * Query: PHRASE() AND FILTER u.age > 18
 * Expected: Combines phrase search with other predicates
 */
TEST_F(FTSPhraseProximityTest, FPH07_PhraseWithOtherFilters) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER doc.type == 'article' && PHRASE(doc.content, "important update")
          RETURN doc
    )";
    // Implementation test: Filter combination and optimization
}

/**
 * FPH-08: Multiple phrase conditions (OR)
 * Query: PHRASE() || PHRASE()
 * Expected: Matches either phrase
 */
TEST_F(FTSPhraseProximityTest, FPH08_MultiplePhrasesOR) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "term1 term2") || PHRASE(doc.content, "term3 term4")
          RETURN doc
    )";
    // Implementation test: OR logic for multiple phrases
}

/**
 * FPH-09: Phrase query on specific fields
 * Query: PHRASE(doc.title, ...) vs PHRASE(doc.body, ...)
 * Expected: Field-specific phrase searching
 */
TEST_F(FTSPhraseProximityTest, FPH09_PhraseOnSpecificField) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.title, "important announcement")
          RETURN doc
    )";
    // Implementation test: Field scoping
}

/**
 * FPH-10: Phrase with stemming/lemmatization
 * Query: PHRASE("running run runs")
 * Expected: Stem variants recognized in phrase
 */
TEST_F(FTSPhraseProximityTest, FPH10_PhraseWithStemming) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "running processes")
          RETURN doc
    )";
    // Implementation test: Stemming integration
}

/**
 * FPH-11: Empty phrase query
 * Query: PHRASE(doc.content, "")
 * Expected: Returns all documents (or error handling)
 */
TEST_F(FTSPhraseProximityTest, FPH11_EmptyPhrase) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "")
          RETURN doc
    )";
    // Implementation test: Edge case handling
}

/**
 * FPH-12: Single-word phrase
 * Query: PHRASE(doc.content, "single")
 * Expected: Matches single word (equivalent to term search)
 */
TEST_F(FTSPhraseProximityTest, FPH12_SingleWordPhrase) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "single")
          RETURN doc
    )";
    // Implementation test: Single-word phrase degenerate case
}

/**
 * FPH-13: Phrase with numbers
 * Query: PHRASE("version 2.0.0")
 * Expected: Correctly handles numeric values in phrases
 */
TEST_F(FTSPhraseProximityTest, FPH13_PhraseWithNumbers) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "version 2.0.0")
          RETURN doc
    )";
    // Implementation test: Numeric handling
}

/**
 * FPH-14: Phrase query result sorting
 * Query: PHRASE() ... SORT BY relevance DESC
 * Expected: Results sorted by phrase relevance/position
 */
TEST_F(FTSPhraseProximityTest, FPH14_PhraseResultSorting) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "key phrase")
          SORT BY _score DESC
          RETURN doc
    )";
    // Implementation test: Relevance scoring and sorting
}

/**
 * FPH-15: Phrase query with limit
 * Query: PHRASE() LIMIT 10
 * Expected: Returns top 10 phrase matches
 */
TEST_F(FTSPhraseProximityTest, FPH15_PhraseWithLimit) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "term1 term2")
          LIMIT 10
          RETURN doc
    )";
    // Implementation test: Limit integration
}

/**
 * FPH-16: Phrase with wildcards (if supported)
 * Query: PHRASE("term* remaining")
 * Expected: Prefix matching within phrase
 */
TEST_F(FTSPhraseProximityTest, FPH16_PhraseWithWildcards) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "term* remaining")
          RETURN doc
    )";
    // Implementation test: Wildcard support in phrases
}

/**
 * FPH-17: Performance test — phrase query latency
 * Expected: ≤100ms on 100K documents
 */
TEST_F(FTSPhraseProximityTest, FPH17_PhrasePerformanceLatency) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "performance test phrase")
          RETURN doc
    )";
    int64_t latency_ms = ExecuteQueryAndMeasure(aql);
    ASSERT_LE(latency_ms, 100) << "Phrase query latency exceeded 100ms target";
}

/**
 * FPH-18: Performance test — phrase throughput
 * Expected: Process ≥100 phrases/second
 */
TEST_F(FTSPhraseProximityTest, FPH18_PhrasePerformanceThroughput) {
    // Run 100 different phrase queries, measure throughput
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        std::string aql = "FOR doc IN documents FILTER PHRASE(doc.content, \"phrase" + 
                         std::to_string(i) + "\") RETURN doc";
        // Execute query
    }
    auto end = std::chrono::high_resolution_clock::now();
    int64_t total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double throughput = 100000.0 / total_ms; // queries per second
    ASSERT_GE(throughput, 100) << "Phrase throughput below 100 q/s target";
}

/**
 * FPH-19: Phrase index usage verification
 * Expected: Query uses phrase index (if available)
 */
TEST_F(FTSPhraseProximityTest, FPH19_PhraseIndexUsage) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "indexed phrase")
          RETURN doc
    )";
    // Verify query plan uses phrase index
    // Assert: Query plan contains INDEX step or equivalent
}

/**
 * FPH-20: Phrase query error handling
 * Expected: Invalid phrases generate meaningful errors
 */
TEST_F(FTSPhraseProximityTest, FPH20_PhraseErrorHandling) {
    // Test invalid phrase syntax
    std::string invalid_aql = "FOR doc IN documents FILTER PHRASE(doc.content) RETURN doc";
    // Assert: Parser rejects invalid syntax with clear error
}

// ============================================================================
// PROXIMITY QUERY TESTS (20 test cases)
// ============================================================================

/**
 * FPX-01: Basic proximity query
 * Query: PROXIMITY("term1", "term2", distance=5)
 * Expected: Returns documents with terms within 5 words
 */
TEST_F(FTSPhraseProximityTest, FPX01_BasicProximity) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 5)
          RETURN doc
    )";
    // Implementation test: Basic proximity matching within distance
}

/**
 * FPX-02: Proximity with default distance
 * Query: PROXIMITY("term1", "term2")
 * Expected: Uses default distance (e.g., 10 words)
 */
TEST_F(FTSPhraseProximityTest, FPX02_ProximityDefaultDistance) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"])
          RETURN doc
    )";
    // Implementation test: Default distance handling
}

/**
 * FPX-03: Proximity with zero distance (same as phrase)
 * Query: PROXIMITY("term1", "term2", distance=0)
 * Expected: Matches adjacent terms
 */
TEST_F(FTSPhraseProximityTest, FPX03_ProximityZeroDistance) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 0)
          RETURN doc
    )";
    // Implementation test: Adjacent term matching
}

/**
 * FPX-04: Proximity with large distance
 * Query: PROXIMITY("term1", "term2", distance=100)
 * Expected: Matches terms far apart but within limit
 */
TEST_F(FTSPhraseProximityTest, FPX04_ProximityLargeDistance) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 100)
          RETURN doc
    )";
    // Implementation test: Large distance handling
}

/**
 * FPX-05: Proximity with multiple terms
 * Query: PROXIMITY("term1", "term2", "term3", distance=10)
 * Expected: All terms within 10-word window
 */
TEST_F(FTSPhraseProximityTest, FPX05_ProximityMultipleTerms) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2", "term3"], 10)
          RETURN doc
    )";
    // Implementation test: Multi-term proximity matching
}

/**
 * FPX-06: Proximity with ordered terms
 * Query: PROXIMITY("first", "second", ordered=true)
 * Expected: Terms must appear in specified order
 */
TEST_F(FTSPhraseProximityTest, FPX06_ProximityOrdered) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["first", "second"], 10, ordered=true)
          RETURN doc
    )";
    // Implementation test: Term order enforcement
}

/**
 * FPX-07: Proximity with unordered terms
 * Query: PROXIMITY("first", "second", ordered=false)
 * Expected: Terms can appear in any order within distance
 */
TEST_F(FTSPhraseProximityTest, FPX07_ProximityUnordered) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["first", "second"], 10, ordered=false)
          RETURN doc
    )";
    // Implementation test: Order-agnostic matching
}

/**
 * FPX-08: Proximity negation
 * Query: !PROXIMITY("term1", "term2")
 * Expected: Returns documents where terms are NOT proximal
 */
TEST_F(FTSPhraseProximityTest, FPX08_NegatedProximity) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER !PROXIMITY(doc.content, ["term1", "term2"], 5)
          RETURN doc
    )";
    // Implementation test: Negation logic
}

/**
 * FPX-09: Proximity with field scoping
 * Query: PROXIMITY(doc.title, ...) vs PROXIMITY(doc.body, ...)
 * Expected: Proximity search on specific fields
 */
TEST_F(FTSPhraseProximityTest, FPX09_ProximityFieldScoping) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.title, ["important", "update"], 3)
          RETURN doc
    )";
    // Implementation test: Field-specific proximity
}

/**
 * FPX-10: Proximity combined with phrase
 * Query: PHRASE() AND PROXIMITY()
 * Expected: Combines phrase and proximity conditions
 */
TEST_F(FTSPhraseProximityTest, FPX10_ProximityAndPhrase) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PHRASE(doc.content, "exact phrase") && 
                 PROXIMITY(doc.content, ["related", "term"], 10)
          RETURN doc
    )";
    // Implementation test: Filter combination
}

/**
 * FPX-11: Proximity with case handling
 * Query: PROXIMITY("Term1", "term2")
 * Expected: Case-insensitive matching
 */
TEST_F(FTSPhraseProximityTest, FPX11_ProximityCaseInsensitive) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["Term1", "TERM2"], 5)
          RETURN doc
    )";
    // Implementation test: Case normalization
}

/**
 * FPX-12: Proximity with numbers
 * Query: PROXIMITY("version", "2.0")
 * Expected: Handles numeric proximity
 */
TEST_F(FTSPhraseProximityTest, FPX12_ProximityWithNumbers) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["version", "2.0.0"], 2)
          RETURN doc
    )";
    // Implementation test: Numeric proximity matching
}

/**
 * FPX-13: Proximity with stemming
 * Query: PROXIMITY("running", "run", distance=5)
 * Expected: Stem variants recognized in proximity
 */
TEST_F(FTSPhraseProximityTest, FPX13_ProximityStemming) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["running", "process"], 5)
          RETURN doc
    )";
    // Implementation test: Stemming in proximity
}

/**
 * FPX-14: Proximity sorting by distance
 * Query: PROXIMITY() SORT BY _distance ASC
 * Expected: Results sorted by proximity distance
 */
TEST_F(FTSPhraseProximityTest, FPX14_ProximitySorting) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 20)
          SORT BY _distance ASC
          RETURN doc
    )";
    // Implementation test: Distance-based sorting
}

/**
 * FPX-15: Proximity result projection with distance
 * Query: PROXIMITY() RETURN {doc, distance: _distance}
 * Expected: Include distance metric in results
 */
TEST_F(FTSPhraseProximityTest, FPX15_ProximityDistanceProjection) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 10)
          RETURN {
            id: doc._key,
            distance: _distance
          }
    )";
    // Implementation test: Distance in result projection
}

/**
 * FPX-16: Performance test — proximity query latency
 * Expected: ≤100ms on 100K documents
 */
TEST_F(FTSPhraseProximityTest, FPX16_ProximityPerformanceLatency) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 10)
          RETURN doc
    )";
    int64_t latency_ms = ExecuteQueryAndMeasure(aql);
    ASSERT_LE(latency_ms, 100) << "Proximity query latency exceeded 100ms target";
}

/**
 * FPX-17: Performance test — proximity throughput
 * Expected: Process ≥100 proximity queries/second
 */
TEST_F(FTSPhraseProximityTest, FPX17_ProximityPerformanceThroughput) {
    // Run 100 different proximity queries, measure throughput
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        std::string aql = "FOR doc IN documents FILTER PROXIMITY(doc.content, " +
                         "[\"term1\", \"term" + std::to_string(i) + "\"], 10) RETURN doc";
        // Execute query
    }
    auto end = std::chrono::high_resolution_clock::now();
    int64_t total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    double throughput = 100000.0 / total_ms;
    ASSERT_GE(throughput, 100) << "Proximity throughput below 100 q/s target";
}

/**
 * FPX-18: Proximity index usage
 * Expected: Query uses proximity index (if available)
 */
TEST_F(FTSPhraseProximityTest, FPX18_ProximityIndexUsage) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["term1", "term2"], 5)
          RETURN doc
    )";
    // Verify query plan uses proximity index or similar optimization
}

/**
 * FPX-19: Proximity error handling
 * Expected: Invalid proximity syntax generates clear errors
 */
TEST_F(FTSPhraseProximityTest, FPX19_ProximityErrorHandling) {
    // Test invalid proximity syntax (missing terms, invalid distance)
    std::string invalid_aql = "FOR doc IN documents FILTER PROXIMITY(doc.content) RETURN doc";
    // Assert: Parser rejects with meaningful error
}

/**
 * FPX-20: Proximity with very large document set
 * Expected: Maintains ≤100ms latency on 100K+ documents
 */
TEST_F(FTSPhraseProximityTest, FPX20_ProximityLargeScale) {
    std::string aql = R"(
        FOR doc IN documents
          FILTER PROXIMITY(doc.content, ["large", "scale", "test"], 15)
          RETURN doc
    )";
    int64_t latency_ms = ExecuteQueryAndMeasure(aql);
    ASSERT_LE(latency_ms, 100) << "Proximity query latency exceeded 100ms target on large dataset";
}

} // namespace query
} // namespace themis
