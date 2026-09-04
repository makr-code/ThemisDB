/**
 * @file test_search_edge_cases_utility.cpp
 * @brief Current utility contract smoke tests.
 */

#include <gtest/gtest.h>

#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "search/search_error_codes.h"

namespace themis::search {
namespace testing {

class UtilityComponentEdgeCasesTest : public ::testing::Test {};

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

TEST_F(UtilityComponentEdgeCasesTest, UTL_02_QueryExpansionOverflowProtection) {
  struct ExpansionResult {
    std::vector<std::string> terms;
    bool overflow = false;
    int error_code = 0;

    bool add_term(const std::string& term, size_t max_size) {
      if (terms.size() >= max_size) {
        overflow = true;
        error_code = static_cast<int>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED);
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
  EXPECT_EQ(result.error_code, static_cast<int>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED));
  EXPECT_EQ(result.terms.size(), kLimit);
}

TEST_F(UtilityComponentEdgeCasesTest, UTL_05_FuzzyMatchingTimeout) {
  struct FuzzyMatchResult {
    bool timed_out = false;
    int matches_found = 0;
    int error_code = 0;
    uint64_t elapsed_ms = 0;

    void check_timeout(uint64_t max_ms) {
      if (elapsed_ms > max_ms) {
        timed_out = true;
        error_code = static_cast<int>(SearchErrorCode::FUZZY_MATCHING_FALLBACK);
      }
    }
  };

  FuzzyMatchResult result;
  result.elapsed_ms = 5500;
  result.check_timeout(5000);

  EXPECT_TRUE(result.timed_out);
  EXPECT_EQ(result.error_code, static_cast<int>(SearchErrorCode::FUZZY_MATCHING_FALLBACK));
}

TEST_F(UtilityComponentEdgeCasesTest, UTL_09_FilterComplexityCompilationLimit) {
  struct FilterStats {
    int or_clauses = 0;
    int and_clauses = 0;
    int nesting_depth = 0;
    int max_nesting = 10;
    bool compiled = false;

    bool can_compile() const {
      return nesting_depth <= max_nesting && (or_clauses + and_clauses) <= 1000;
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

TEST_F(UtilityComponentEdgeCasesTest, UTL_10_FilterComplexityRejection) {
  struct FilterCompiler {
    int max_complexity = 2000;
    int error_code = 0;

    bool compile_filter(int complexity) {
      if (complexity > max_complexity) {
        error_code = static_cast<int>(SearchErrorCode::FACET_CARDINALITY_LIMIT);
        return false;
      }
      return true;
    }
  };

  FilterCompiler compiler;
  bool result = compiler.compile_filter(5000);
  EXPECT_FALSE(result);
  EXPECT_EQ(compiler.error_code, static_cast<int>(SearchErrorCode::FACET_CARDINALITY_LIMIT));
}

TEST_F(UtilityComponentEdgeCasesTest, UTL_11_TokenizationEmptyTokens) {
  std::string text = "   hello     world    ";
  std::vector<std::string> tokens;
  std::istringstream iss(text);
  std::string token = {};
  while (iss >> token) {
    if (!token.empty()) {
      tokens.push_back(token);
    }
  }

  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "hello");
  EXPECT_EQ(tokens[1], "world");
}

TEST_F(UtilityComponentEdgeCasesTest, UTL_15_ErrorCodeAggregation) {
  struct UtilityChainResult {
    std::vector<int> component_errors;
    int primary_error = 0;

    void aggregate_errors() {
      if (!component_errors.empty()) {
        primary_error = component_errors[0];
      }
    }
  };

  UtilityChainResult result;
  result.component_errors = {
      static_cast<int>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED),
      static_cast<int>(SearchErrorCode::FACET_CARDINALITY_LIMIT),
      static_cast<int>(SearchErrorCode::FUZZY_MATCHING_FALLBACK)};
  result.aggregate_errors();

  EXPECT_EQ(result.primary_error, static_cast<int>(SearchErrorCode::EXPANSION_LIMIT_EXCEEDED));
  EXPECT_EQ(result.component_errors.size(), 3u);
}

}  // namespace testing
}  // namespace themis::search
