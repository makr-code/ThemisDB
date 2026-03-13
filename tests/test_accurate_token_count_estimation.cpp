/**
 * @file test_accurate_token_count_estimation.cpp
 * @brief Tests and benchmark for accurate token-count estimation (Issue #145, v1.6.0).
 *
 * Verifies:
 *  AC-1  TokenEstimator abstract interface and CharDivisionEstimator (current behaviour).
 *  AC-2  TiktokenEstimator with a function-based tokenizer backend.
 *  AC-3  Injection of TokenEstimator into LLMAQLHandler via setTokenEstimator().
 *  AC-4  Benchmark: CharDivisionEstimator accuracy ≤ 10% error at the 95th percentile
 *        against a word-boundary reference tokenizer on the built-in few-shot corpus.
 */

#include <gtest/gtest.h>
#include "aql/llm_token_estimator.h"
#include "aql/llm_aql_handler.h"
#include "aql/aql_fewshot_example_library.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

using namespace themis::aql;

// ============================================================================
// Reference tokenizer: simple word-boundary split used as ground truth.
// Counts whitespace-separated tokens (each punctuation-attached word counts
// as one token), which is a recognised approximation for BPE token counts on
// short English + AQL text where word == subword in most cases.
// ============================================================================

namespace {

std::size_t referenceTokenCount(const std::string& text) {
    if (text.empty()) return 0;
    std::istringstream iss(text);
    std::string word;
    std::size_t count = 0;
    while (iss >> word) {
        ++count;
    }
    return count > 0 ? count : 1;
}

// Compute the absolute relative error: |estimated - reference| / reference.
double relativeError(std::size_t estimated, std::size_t reference) {
    if (reference == 0) return 0.0;
    double diff = static_cast<double>(estimated) > static_cast<double>(reference)
                    ? static_cast<double>(estimated) - static_cast<double>(reference)
                    : static_cast<double>(reference) - static_cast<double>(estimated);
    return diff / static_cast<double>(reference);
}

// Build the combined corpus text used for benchmarking: join nl_query + aql_query
// for every built-in example.
std::vector<std::string> buildFewShotCorpus() {
    AQLFewShotExampleLibrary lib;
    const auto& examples = lib.all();
    std::vector<std::string> corpus;
    corpus.reserve(examples.size() * 2);
    for (const auto& ex : examples) {
        corpus.push_back(ex.nl_query);
        corpus.push_back(ex.aql_query);
    }
    return corpus;
}

} // anonymous namespace

// ============================================================================
// AC-1 · TokenEstimator interface and CharDivisionEstimator
// ============================================================================

class CharDivisionEstimatorTest : public ::testing::Test {};

TEST_F(CharDivisionEstimatorTest, EmptyString_ReturnsZero) {
    CharDivisionEstimator est;
    EXPECT_EQ(est.estimate(""), 0u);
}

TEST_F(CharDivisionEstimatorTest, DefaultRatio4_FourCharsIsOneToken) {
    CharDivisionEstimator est;                     // default ratio = 4
    EXPECT_EQ(est.estimate("abcd"), 1u);
    EXPECT_EQ(est.estimate("abcdefgh"), 2u);
    EXPECT_EQ(est.estimate("abc"), 1u);            // ceiling: 3/4 → 1
}

TEST_F(CharDivisionEstimatorTest, CustomRatio_IsRespected) {
    CharDivisionEstimator est(3);
    EXPECT_EQ(est.estimate("abc"), 1u);
    EXPECT_EQ(est.estimate("abcdef"), 2u);
    EXPECT_EQ(est.estimate("ab"), 1u);             // ceiling: 2/3 → 1
}

TEST_F(CharDivisionEstimatorTest, ZeroRatioFallsBackToFour) {
    CharDivisionEstimator est(0);
    EXPECT_EQ(est.estimate("abcd"), 1u);           // ratio clamped to 4
}

TEST_F(CharDivisionEstimatorTest, LargeString_MatchesExpectedCeiling) {
    CharDivisionEstimator est;
    std::string s(100, 'x');
    EXPECT_EQ(est.estimate(s), 25u);               // ceil(100 / 4) = 25
}

TEST_F(CharDivisionEstimatorTest, PolymorphicInterface_WorksViaBasePointer) {
    std::unique_ptr<TokenEstimator> est =
        std::make_unique<CharDivisionEstimator>(4);
    EXPECT_EQ(est->estimate("test"), 1u);
}

// ============================================================================
// AC-2 · TiktokenEstimator
// ============================================================================

class TiktokenEstimatorTest : public ::testing::Test {};

TEST_F(TiktokenEstimatorTest, EmptyString_ReturnsZero) {
    TiktokenEstimator est([](const std::string& t) -> std::size_t {
        return t.size() / 2;  // dummy tokenizer
    });
    EXPECT_EQ(est.estimate(""), 0u);
}

TEST_F(TiktokenEstimatorTest, CustomFunction_IsInvoked) {
    std::size_t call_count = 0;
    TiktokenEstimator est([&call_count](const std::string& t) -> std::size_t {
        ++call_count;
        // Simulate a word-split tokenizer for test purposes.
        std::istringstream iss(t);
        std::string w;
        std::size_t n = 0;
        while (iss >> w) ++n;
        return n > 0 ? n : 1;
    });

    std::size_t result = est.estimate("hello world foo bar");
    EXPECT_EQ(result, 4u);
    EXPECT_EQ(call_count, 1u);
}

TEST_F(TiktokenEstimatorTest, NullFunction_FallsBackToCharDivision) {
    // Construct with an empty std::function (null).
    TiktokenEstimator est(TiktokenEstimator::TokenizeFunc{});
    // Falls back to CharDivisionEstimator{4}: 4 chars → 1 token.
    EXPECT_EQ(est.estimate("abcd"), 1u);
    EXPECT_EQ(est.estimate("abcdefgh"), 2u);
}

TEST_F(TiktokenEstimatorTest, PolymorphicInterface_WorksViaBasePointer) {
    std::unique_ptr<TokenEstimator> est =
        std::make_unique<TiktokenEstimator>(
            [](const std::string& t) -> std::size_t { return t.size(); }
        );
    EXPECT_EQ(est->estimate("hi"), 2u);
}

TEST_F(TiktokenEstimatorTest, FunctionReceivesExactInput) {
    std::string received;
    TiktokenEstimator est([&received](const std::string& t) -> std::size_t {
        received = t;
        return 1;
    });
    est.estimate("FOR x IN col RETURN x");
    EXPECT_EQ(received, "FOR x IN col RETURN x");
}

// ============================================================================
// AC-3 · Injection into LLMAQLHandler via setTokenEstimator()
// ============================================================================

class TokenEstimatorInjectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler_ = std::make_unique<LLMAQLHandler>();
    }

    void TearDown() override {
        handler_.reset();
    }

    std::unique_ptr<LLMAQLHandler> handler_;
};

TEST_F(TokenEstimatorInjectionTest, DefaultEstimatorIsCharDivision_NoBreakingChange) {
    // The handler must work with no explicit estimator set.
    // We inject a mock chat executor so translateNLToAQL doesn't need a real LLM.
    handler_->setChatExecutor([](const auto&) -> std::string {
        return "FOR d IN documents RETURN d";
    });

    // Should not throw — default CharDivisionEstimator is in place.
    EXPECT_NO_THROW({
        auto result = handler_->translateNLToAQL("find all documents");
        EXPECT_FALSE(result.empty());
    });
}

TEST_F(TokenEstimatorInjectionTest, SetTokenEstimator_CustomFunctionIsUsed) {
    std::size_t estimation_calls = 0;

    auto custom_est = std::make_unique<TiktokenEstimator>(
        [&estimation_calls](const std::string& t) -> std::size_t {
            ++estimation_calls;
            return t.size() / 4;
        }
    );
    handler_->setTokenEstimator(std::move(custom_est));

    // Inject a mock chat executor so executeInfer doesn't need a real LLM.
    handler_->setChatExecutor([](const auto&) -> std::string {
        return "FOR d IN documents RETURN d";
    });

    // Calling translateNLToAQL goes through executeChat (not executeInfer),
    // so estimation is not triggered there.  To exercise the estimator on a
    // code path that counts tokens, we exercise executeInfer.
    try {
        handler_->executeInfer("hello world");
    } catch (...) {
        // Ignore LLM errors — we only care that the estimator was called.
    }

    // The estimator should have been invoked for input + output token counting.
    EXPECT_GE(estimation_calls, 1u);
}

TEST_F(TokenEstimatorInjectionTest, SetNullEstimator_RestoresDefault) {
    // First set a custom estimator...
    handler_->setTokenEstimator(
        std::make_unique<CharDivisionEstimator>(8)
    );
    // ...then reset to default by passing nullptr.
    handler_->setTokenEstimator(nullptr);

    // Handler must still be operational.
    handler_->setChatExecutor([](const auto&) -> std::string {
        return "FOR d IN documents RETURN d";
    });
    EXPECT_NO_THROW({
        auto result = handler_->translateNLToAQL("find all documents");
        EXPECT_FALSE(result.empty());
    });
}

TEST_F(TokenEstimatorInjectionTest, SetTokenEstimator_SwitchAtRuntime) {
    // Verify that the estimator can be swapped after construction without
    // crashing (runtime polymorphism works correctly).
    handler_->setTokenEstimator(std::make_unique<CharDivisionEstimator>(3));
    handler_->setTokenEstimator(std::make_unique<CharDivisionEstimator>(5));
    handler_->setTokenEstimator(std::make_unique<TiktokenEstimator>(
        [](const std::string& t) -> std::size_t { return t.size(); }
    ));
    SUCCEED();
}

// ============================================================================
// AC-4 · Benchmark: CharDivisionEstimator accuracy on the few-shot corpus
//
// Reference: word-boundary tokenizer (whitespace split).
// Target:    ≤ 10 % relative error at the 95th percentile.
// ============================================================================

class TokenCountBenchmarkTest : public ::testing::Test {};

TEST_F(TokenCountBenchmarkTest, CharDivisionEstimator_AccuracyWithin10Pct_95thPctile) {
    const auto corpus = buildFewShotCorpus();
    ASSERT_FALSE(corpus.empty()) << "Few-shot corpus must not be empty";

    CharDivisionEstimator est;  // default ratio = 4

    std::vector<double> errors;
    errors.reserve(corpus.size());

    for (const auto& text : corpus) {
        if (text.empty()) continue;
        std::size_t ref  = referenceTokenCount(text);
        std::size_t pred = est.estimate(text);
        errors.push_back(relativeError(pred, ref));
    }

    ASSERT_FALSE(errors.empty()) << "No non-empty corpus items found";

    std::sort(errors.begin(), errors.end());

    // 95th percentile index (0-indexed, conservative rounding up)
    const std::size_t p95_idx =
        static_cast<std::size_t>(std::ceil(0.95 * static_cast<double>(errors.size()))) - 1;
    const double p95_error = errors[std::min(p95_idx, errors.size() - 1)];

    // Report statistics for observability
    const double mean_error =
        std::accumulate(errors.begin(), errors.end(), 0.0) /
        static_cast<double>(errors.size());

    // Emit stats via test message (visible with --gtest_verbose)
    SCOPED_TRACE("corpus_size=" + std::to_string(errors.size()) +
                 " mean_rel_error=" + std::to_string(mean_error) +
                 " p95_rel_error=" + std::to_string(p95_error));

    EXPECT_LE(p95_error, 0.10)
        << "CharDivisionEstimator 95th-percentile relative error is "
        << (p95_error * 100.0) << "% which exceeds the 10% target.\n"
        << "  corpus size:  " << errors.size() << "\n"
        << "  mean error:   " << (mean_error * 100.0) << "%\n"
        << "  p95 error:    " << (p95_error * 100.0) << "%";
}

TEST_F(TokenCountBenchmarkTest, TiktokenEstimatorWithWordSplit_ZeroErrorOnReference) {
    const auto corpus = buildFewShotCorpus();
    ASSERT_FALSE(corpus.empty());

    // TiktokenEstimator whose function IS the reference tokenizer → zero error.
    TiktokenEstimator est([](const std::string& t) -> std::size_t {
        return referenceTokenCount(t);
    });

    for (const auto& text : corpus) {
        if (text.empty()) continue;
        std::size_t ref  = referenceTokenCount(text);
        std::size_t pred = est.estimate(text);
        EXPECT_EQ(pred, ref) << "Mismatch for: " << text;
    }
}

TEST_F(TokenCountBenchmarkTest, AllCorpusTextsHaveNonZeroReferenceCount) {
    const auto corpus = buildFewShotCorpus();
    for (const auto& text : corpus) {
        if (text.empty()) continue;
        EXPECT_GE(referenceTokenCount(text), 1u)
            << "Unexpected zero token count for: " << text;
    }
}
