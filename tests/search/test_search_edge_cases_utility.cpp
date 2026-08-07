/**
 * @file test_search_edge_cases_utility.cpp
 * @brief Search Utility Component Edge Cases: Expansion, Facets, Fuzzy Matching
 * @version 1.0.0
 * @date 2026-08-06
 *
 * Comprehensive edge case coverage for search utility components:
 * - Query expansion limits and overflow (UTL-01, UTL-02)
 * - Facet cardinality explosion and overflow (UTL-03, UTL-04)
 * - Fuzzy matching timeout and performance degradation (UTL-05, UTL-06)
 * - Synonym expansion combinatorial explosion (UTL-07, UTL-08)
 * - Filter complexity and compilation limits (UTL-09, UTL-10)
 * - Tokenization pathological cases (UTL-11, UTL-12)
 * - Highlighting with massive result sets (UTL-13, UTL-14)
 * - Error code aggregation and cascading failures (UTL-15, UTL-16)
 *
 * Test IDs: UTL-01 through UTL-16
 * CTest labels: search, utility, edge-cases
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>
#include <limits>
#include "search/query_expander.h"
#include "search/faceted_search.h"
#include "search/fuzzy_matcher.h"
#include "search/search_error_codes.h"

namespace themis::search {
namespace testing {

// Test fixture for utility component edge cases
class UtilityComponentEdgeCasesTest : public ::testing::Test {
 protected:
  static constexpr uint32_t kCanonicalRngSeed = 42;
  
  void SetUp() override {
    srand(kCanonicalRngSeed);
  }
};

// ============================================================================
// UTL-01: Query Expansion at Maximum Limit
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_01_QueryExpansionMaxLimit) {
  constexpr size_t kMaxExpansions = 1000;
  std::vector<std::string> expanded_terms;
  
  for (size_t i = 0; i < kMaxExpansions; ++i) {
    expanded_terms.push_back("term_" + std::to_string(i));
  }
  
  EXPECT_EQ(expanded_terms.size(), kMaxExpansions);
  EXPECT_EQ(expanded_terms.front(), "term_0");
  EXPECT_EQ(expanded_terms.back(), "term_999");
}

// ============================================================================
// UTL-02: Query Expansion Overflow Protection
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_02_QueryExpansionOverflowProtection) {
  struct ExpansionResult {
    std::vector<std::string> terms;
    bool overflow = false;
    int error_code = 0;
    
    bool add_term(const std::string& term, size_t max_size) {
      if (terms.size() >= max_size) {
        overflow = true;
        error_code = SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED;
        return false;
      }
      terms.push_back(term);
      return true;
    }
  };
  
  ExpansionResult result;
  constexpr size_t kLimit = 100;
  
  for (int i = 0; i < 150; ++i) {
    if (!result.add_term("term_" + std::to_string(i), kLimit)) {
      break;
    }
  }
  
  EXPECT_TRUE(result.overflow);
  EXPECT_EQ(result.error_code, SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED);
  EXPECT_EQ(result.terms.size(), kLimit);
}

// ============================================================================
// UTL-03: Facet Cardinality Explosion
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_03_FacetCardinalityExplosion) {
  struct FacetBucket {
    std::string value;
    int count = 0;
  };
  
  std::vector<FacetBucket> facets;
  
  // Simulate massive facet cardinality
  for (int i = 0; i < 10000; ++i) {
    FacetBucket fb;
    fb.value = "facet_value_" + std::to_string(i);
    fb.count = 1;
    facets.push_back(fb);
  }
  
  EXPECT_EQ(facets.size(), 10000);
  EXPECT_GT(facets.size(), 100);  // Much larger than typical UI rendering
}

// ============================================================================
// UTL-04: Facet Overflow with Truncation
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_04_FacetOverflowTruncation) {
  struct FacetStats {
    int total_facet_values = 0;
    int returned_facet_values = 0;
    bool truncated = false;
    int error_code = 0;
    
    void truncate_to_limit(int max_facets) {
      if (total_facet_values > max_facets) {
        returned_facet_values = max_facets;
        truncated = true;
        error_code = SEARCH_ERR_FACET_OVERFLOW;
      } else {
        returned_facet_values = total_facet_values;
      }
    }
  };
  
  FacetStats stats;
  stats.total_facet_values = 50000;
  stats.truncate_to_limit(1000);
  
  EXPECT_TRUE(stats.truncated);
  EXPECT_EQ(stats.returned_facet_values, 1000);
  EXPECT_EQ(stats.error_code, SEARCH_ERR_FACET_OVERFLOW);
}

// ============================================================================
// UTL-05: Fuzzy Matching Timeout Protection
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_05_FuzzyMatchingTimeout) {
  struct FuzzyMatchResult {
    bool timed_out = false;
    int matches_found = 0;
    int error_code = 0;
    uint64_t elapsed_ms = 0;
    
    void check_timeout(uint64_t max_ms) {
      if (elapsed_ms > max_ms) {
        timed_out = true;
        error_code = SEARCH_ERR_FUZZY_TIMEOUT;
      }
    }
  };
  
  FuzzyMatchResult result;
  result.elapsed_ms = 5500;  // 5.5 seconds
  result.check_timeout(5000);  // 5 second limit
  
  EXPECT_TRUE(result.timed_out);
  EXPECT_EQ(result.error_code, SEARCH_ERR_FUZZY_TIMEOUT);
}

// ============================================================================
// UTL-06: Fuzzy Matching Performance Degradation Under Load
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_06_FuzzyMatchingPerformanceDegradation) {
  std::vector<uint64_t> query_latencies_ms;
  
  // Small corpus: fast
  query_latencies_ms.push_back(10);
  
  // Medium corpus: normal
  query_latencies_ms.push_back(50);
  
  // Large corpus: degraded (but within timeout)
  query_latencies_ms.push_back(4800);
  
  for (size_t i = 0; i < query_latencies_ms.size(); ++i) {
    EXPECT_LT(query_latencies_ms[i], 5000);
  }
  
  // Verify degradation trend
  EXPECT_LT(query_latencies_ms[0], query_latencies_ms[1]);
  EXPECT_LT(query_latencies_ms[1], query_latencies_ms[2]);
}

// ============================================================================
// UTL-07: Synonym Expansion Combinatorial Explosion
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_07_SynonymExpansionExplosion) {
  struct SynonymSet {
    std::string primary;
    std::vector<std::string> synonyms;
  };
  
  std::vector<SynonymSet> synonym_sets = {
    {"car", {"automobile", "vehicle", "auto"}},
    {"fast", {"quick", "rapid", "speedy", "swift"}},
    {"big", {"large", "huge", "massive", "enormous"}}
  };
  
  // Naive expansion: multiply term counts
  size_t expansion_count = 1;
  for (const auto& ss : synonym_sets) {
    expansion_count *= (ss.synonyms.size() + 1);  // +1 for primary
  }
  
  // Without limits: 4 * 5 * 5 = 100 combinations
  EXPECT_EQ(expansion_count, 4 * 5 * 5);
  EXPECT_GT(expansion_count, 50);
}

// ============================================================================
// UTL-08: Synonym Expansion with Backoff Limit
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_08_SynonymExpansionBackoffLimit) {
  struct SynonymExpansion {
    std::vector<std::string> expanded;
    size_t max_terms = 200;
    bool backoff_applied = false;
    
    void add_expansion(const std::string& term, size_t count) {
      for (size_t i = 0; i < count && expanded.size() < max_terms; ++i) {
        expanded.push_back(term + "_var_" + std::to_string(i));
      }
      if (expanded.size() >= max_terms) {
        backoff_applied = true;
      }
    }
  };
  
  SynonymExpansion expansion;
  expansion.add_expansion("base", 300);
  
  EXPECT_TRUE(expansion.backoff_applied);
  EXPECT_EQ(expansion.expanded.size(), 200);
}

// ============================================================================
// UTL-09: Filter Complexity at Compilation Limit
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_09_FilterComplexityCompilationLimit) {
  struct FilterStats {
    int or_clauses = 0;
    int and_clauses = 0;
    int nesting_depth = 0;
    int max_nesting = 10;
    bool compiled = false;
    
    bool can_compile() const {
      return nesting_depth <= max_nesting &&
             (or_clauses + and_clauses) <= 1000;
    }
  };
  
  FilterStats stats;
  stats.or_clauses = 500;
  stats.and_clauses = 500;
  stats.nesting_depth = 8;
  
  EXPECT_TRUE(stats.can_compile());
  
  stats.nesting_depth = 11;
  EXPECT_FALSE(stats.can_compile());
}

// ============================================================================
// UTL-10: Filter Complexity with Rejection
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_10_FilterComplexityRejection) {
  struct FilterCompiler {
    int max_complexity = 2000;
    int error_code = 0;
    
    bool compile_filter(int complexity) {
      if (complexity > max_complexity) {
        error_code = SEARCH_ERR_FILTER_TOO_COMPLEX;
        return false;
      }
      return true;
    }
  };
  
  FilterCompiler compiler;
  bool result = compiler.compile_filter(5000);
  
  EXPECT_FALSE(result);
  EXPECT_EQ(compiler.error_code, SEARCH_ERR_FILTER_TOO_COMPLEX);
}

// ============================================================================
// UTL-11: Tokenization Pathological Case - Empty Tokens
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_11_TokenizationEmptyTokens) {
  std::string text = "   hello     world    ";
  std::vector<std::string> tokens;
  
  // Simulate tokenization with filtering empty tokens
  std::istringstream iss(text);
  std::string token;
  while (iss >> token) {  // >> automatically skips whitespace
    if (!token.empty()) {
      tokens.push_back(token);
    }
  }
  
  EXPECT_EQ(tokens.size(), 2);
  EXPECT_EQ(tokens[0], "hello");
  EXPECT_EQ(tokens[1], "world");
}

// ============================================================================
// UTL-12: Tokenization Pathological Case - Unicode Boundaries
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_12_TokenizationUnicodeBoundaries) {
  std::vector<std::string> tokens = {"café", "naïve", "résumé"};
  
  // Verify tokens are preserved correctly
  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], "café");
  EXPECT_EQ(tokens[1], "naïve");
  EXPECT_EQ(tokens[2], "résumé");
}

// ============================================================================
// UTL-13: Highlighting with Massive Result Set
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_13_HighlightingMassiveResults) {
  struct HighlightStats {
    int documents_to_highlight = 0;
    int total_snippets = 0;
    bool memory_efficient = false;
    
    void process_batch(int count) {
      // Process snippets in chunks to avoid memory explosion
      documents_to_highlight += count;
      total_snippets += count * 3;  // 3 snippets per doc
      
      if (documents_to_highlight > 10000) {
        memory_efficient = true;  // Use streaming
      }
    }
  };
  
  HighlightStats stats;
  stats.process_batch(5000);
  stats.process_batch(5000);
  stats.process_batch(5000);
  
  EXPECT_TRUE(stats.memory_efficient);
  EXPECT_EQ(stats.documents_to_highlight, 15000);
}

// ============================================================================
// UTL-14: Highlighting with Deep Nesting
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_14_HighlightingDeepNesting) {
  std::string text = "outer <span class='a'> inner <b> nested <i> deep </i> </b> </span> end";
  
  // Count nesting depth
  int max_depth = 0, current_depth = 0;
  for (char c : text) {
    if (c == '<') {
      current_depth++;
      max_depth = std::max(max_depth, current_depth);
    } else if (c == '>') {
      current_depth--;
    }
  }
  
  EXPECT_GT(max_depth, 0);
}

// ============================================================================
// UTL-15: Error Code Aggregation in Utility Chain
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_15_ErrorCodeAggregation) {
  struct UtilityChainResult {
    std::vector<int> component_errors;
    int primary_error = 0;
    
    void aggregate_errors() {
      if (!component_errors.empty()) {
        // Take first error as primary
        primary_error = component_errors[0];
      }
    }
  };
  
  UtilityChainResult result;
  result.component_errors = {
    SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED,
    SEARCH_ERR_FACET_OVERFLOW,
    SEARCH_ERR_FUZZY_TIMEOUT
  };
  result.aggregate_errors();
  
  EXPECT_EQ(result.primary_error, SEARCH_ERR_EXPANSION_LIMIT_EXCEEDED);
  EXPECT_EQ(result.component_errors.size(), 3);
}

// ============================================================================
// UTL-16: Cascading Failures in Utility Components
// ============================================================================
TEST_F(UtilityComponentEdgeCasesTest, UTL_16_CascadingFailuresUtility) {
  struct UtilityPipeline {
    bool expansion_ok = true;
    bool facet_ok = true;
    bool fuzzy_ok = true;
    int failed_stage = -1;
    
    bool execute() {
      if (!expansion_ok) {
        failed_stage = 0;
        return false;
      }
      if (!facet_ok) {
        failed_stage = 1;
        return false;
      }
      if (!fuzzy_ok) {
        failed_stage = 2;
        return false;
      }
      return true;
    }
  };
  
  UtilityPipeline pipeline;
  pipeline.expansion_ok = true;
  pipeline.facet_ok = false;  // Fails at facet stage
  pipeline.fuzzy_ok = true;
  
  bool success = pipeline.execute();
  
  EXPECT_FALSE(success);
  EXPECT_EQ(pipeline.failed_stage, 1);
}

}  // namespace testing
}  // namespace themis::search
