/**
 * @file bench_aql_helper_paths.cpp
 * @brief Phase 6 benchmarks for AQL helper components.
 *
 * Measures performance of:
 *   BM_AQLConfidenceScorerSimple  — Confidence scoring for a single translation result
 *   BM_AQLFewShotRetrieval        — Few-shot example retrieval from template library
 *   BM_AQLHighlighterSimple       — AQL syntax annotation (no-ANSI highlighter path)
 *   BM_AQLTokenEstimation         — Token count estimation for conversation context
 *
 * All benchmarks are self-contained with mock implementations.
 * No real AQL infrastructure is required.
 *
 * Wave 7 conventions:
 *   - kW7CanonicalSeed = 42
 *   - UseRealTime() for I/O-bound paths
 *
 * @note Release gates: see src/aql/PERFORMANCE_EXPECTATIONS.md
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <cmath>

namespace themis {
namespace bench {
namespace aql_helpers {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kW7CanonicalSeed = 42;

// ---------------------------------------------------------------------------
// Mock Confidence Scorer
// ---------------------------------------------------------------------------
//
// Simulates the AQL confidence scoring component by computing a simple
// lexical-overlap score between NL query tokens and AQL tokens.

class MockConfidenceScorer {
public:
    struct ScoreResult {
        float   confidence;      // [0.0, 1.0]
        int     matching_tokens;
        int     total_nl_tokens;
    };

    static ScoreResult score(const std::string& nl_query, const std::string& aql_query) {
        auto nl_tokens  = tokenize(nl_query);
        auto aql_tokens = tokenize(aql_query);

        int matches = 0;
        for (const auto& t : nl_tokens) {
            if (std::find(aql_tokens.begin(), aql_tokens.end(), t) != aql_tokens.end()) {
                ++matches;
            }
        }

        float conf = nl_tokens.empty() ? 0.0f
                   : static_cast<float>(matches) / static_cast<float>(nl_tokens.size());

        return {conf, matches, static_cast<int>(nl_tokens.size())};
    }

private:
    static std::vector<std::string> tokenize(const std::string& s) {
        std::vector<std::string> tokens;
        std::string cur;
        for (char c : s) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                cur += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else {
                if (!cur.empty()) { tokens.push_back(cur); cur.clear(); }
            }
        }
        if (!cur.empty()) {
          tokens.push_back(cur);
        }
        return tokens;
    }
};

// ---------------------------------------------------------------------------
// Mock Few-Shot Retrieval
// ---------------------------------------------------------------------------
//
// Simulates retrieval of top-K few-shot examples by Jaccard similarity.

class MockFewShotLibrary {
public:
    struct Example {
        std::string nl;
        std::string aql;
    };

    struct RetrievalResult {
        std::vector<std::size_t> indices;  // Indices into example list
        float top_score;
    };

    MockFewShotLibrary() {
        // Seed with canonical examples
        examples_ = {
            {"show all users",            "FOR u IN users RETURN u"},
            {"filter active users",       "FOR u IN users FILTER u.active == true RETURN u"},
            {"get top 10 products",       "FOR p IN products SORT p.views DESC LIMIT 10 RETURN p"},
            {"find orders above 100",     "FOR o IN orders FILTER o.total > 100 RETURN o"},
            {"list recent subscriptions", "FOR s IN subscriptions SORT s.created_at DESC LIMIT 20 RETURN s"},
            {"count all documents",       "RETURN LENGTH(FOR d IN docs RETURN d)"},
            {"update user email",         "FOR u IN users FILTER u.id == @id UPDATE u WITH { email: @email } IN users"},
            {"delete old logs",           "FOR l IN logs FILTER l.ts < @cutoff REMOVE l IN logs"},
        };
    }

    RetrievalResult retrieveTopK(const std::string& nl_query, int k) const {
        struct Candidate { std::size_t idx; float score; };
        std::vector<Candidate> candidates;
        candidates.reserve(examples_.size());

        for (std::size_t i = 0; i < examples_.size(); ++i) {
            float s = jaccardScore(nl_query, examples_[i].nl);
            candidates.push_back({i, s});
        }

        std::sort(candidates.begin(), candidates.end(),
                  [](const Candidate& a, const Candidate& b) { return a.score > b.score; });

        RetrievalResult result;
        int take = std::min(k, static_cast<int>(candidates.size()));
        result.top_score = candidates.empty() ? 0.0f : candidates[0].score;
        for (int i = 0; i < take; ++i) {
            result.indices.push_back(candidates[i].idx);
        }
        return result;
    }

private:
    static float jaccardScore(const std::string& a, const std::string& b) {
        auto ta = tokenSet(a);
        auto tb = tokenSet(b);
        if (ta.empty() && tb.empty()) {
          return 1.0f;
        }
        if (ta.empty() || tb.empty()) {
          return 0.0f;
        }

        int intersection = 0;
        for (const auto& t : ta) {
            if (std::find(tb.begin(), tb.end(), t) != tb.end()) {
              ++intersection;
            }
        }
        int union_size = static_cast<int>(ta.size() + tb.size()) - intersection;
        return static_cast<float>(intersection) / static_cast<float>(union_size);
    }

    static std::vector<std::string> tokenSet(const std::string& s) {
        std::vector<std::string> tokens;
        std::string cur;
        for (char c : s) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                cur += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else {
                if (!cur.empty()) {
                    if (std::find(tokens.begin(), tokens.end(), cur) == tokens.end()) {
                        tokens.push_back(cur);
                    }
                    cur.clear();
                }
            }
        }
        if (!cur.empty() && std::find(tokens.begin(), tokens.end(), cur) == tokens.end()) {
            tokens.push_back(cur);
        }
        return tokens;
    }

    std::vector<Example> examples_;
};

// ---------------------------------------------------------------------------
// Mock Syntax Highlighter (annotation path)
// ---------------------------------------------------------------------------

class MockAQLHighlighter {
public:
    struct Annotation {
        int         line;
        int         col;
        std::string message;
    };

    /// Annotate a simple AQL string — returns warnings for suspicious patterns
    static std::vector<Annotation> annotate(const std::string& aql) {
        std::vector<Annotation> annotations;
        // Check for missing FILTER on non-trivial queries
        if (aql.find("FOR") != std::string::npos &&
            aql.find("FILTER") == std::string::npos &&
            aql.size() > 30) {
            annotations.push_back({1, 1, "Consider adding FILTER to restrict result set"});
        }
        // Check for SELECT (invalid in AQL)
        if (aql.find("SELECT") != std::string::npos) {
            annotations.push_back({1, 1, "AQL uses RETURN not SELECT"});
        }
        return annotations;
    }
};

// ---------------------------------------------------------------------------
// Mock Token Estimator
// ---------------------------------------------------------------------------

class MockTokenEstimator {
public:
    /// Estimate token count: 1 token ≈ 4 characters (GPT-3.5/4 approximation)
    static uint32_t estimate(const std::string& text) {
        return static_cast<uint32_t>(text.size() / 4) + 1;
    }

    /// Estimate for a conversation turn (NL + AQL)
    static uint32_t estimateTurn(const std::string& nl, const std::string& aql) {
        return estimate(nl) + estimate(aql);
    }

    /// Estimate total tokens for a conversation history
    static uint32_t estimateHistory(const std::vector<std::pair<std::string, std::string>>& turns) {
        uint32_t total = 0;
        for (const auto& [nl, aql] : turns) {
            total += estimateTurn(nl, aql);
        }
        return total;
    }
};

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLConfidenceScorerSimple
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLConfidenceScorerSimple
 *
 * Measures the p50 latency of computing confidence score for a single
 * NL→AQL translation result.
 *
 * Release gate: p95 ≤ 100 µs (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLConfidenceScorerSimple(benchmark::State& state) {
    std::mt19937_64 rng(kW7CanonicalSeed + 10);

    const std::vector<std::pair<std::string, std::string>> pairs = {
        {"show all users",          "FOR u IN users RETURN u"},
        {"find orders above 100",   "FOR o IN orders FILTER o.total > 100 RETURN o"},
        {"get latest products",     "FOR p IN products SORT p.ts DESC LIMIT 10 RETURN p"},
    };
    std::uniform_int_distribution<std::size_t> pick(0, pairs.size() - 1);

    for (auto _ : state) {
        const auto& [nl, aql] = pairs[pick(rng)];
        auto result = MockConfidenceScorer::score(nl, aql);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_AQLConfidenceScorerSimple)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLFewShotRetrieval
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLFewShotRetrieval
 *
 * Measures the p50 latency of retrieving top-K few-shot examples by
 * Jaccard similarity against a library of 8 examples.
 *
 * Release gate: p95 ≤ 500 µs (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLFewShotRetrieval(benchmark::State& state) {
    MockFewShotLibrary library;
    const int k = static_cast<int>(state.range(0));

    std::mt19937_64 rng(kW7CanonicalSeed + 20);
    const std::vector<std::string> queries = {
        "show all users",
        "get recent orders",
        "count documents",
        "find active subscriptions",
        "delete expired sessions",
    };
    std::uniform_int_distribution<std::size_t> pick(0, queries.size() - 1);

    for (auto _ : state) {
        const auto& q = queries[pick(rng)];
        auto result = library.retrieveTopK(q, k);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_AQLFewShotRetrieval)
    ->Arg(3)
    ->Arg(5)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLHighlighterSimple
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLHighlighterSimple
 *
 * Measures the p50 latency of annotating an AQL query string using
 * the no-ANSI highlighter path.
 *
 * Release gate: p95 ≤ 200 µs (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLHighlighterSimple(benchmark::State& state) {
    std::mt19937_64 rng(kW7CanonicalSeed + 30);
    const std::vector<std::string> queries = {
        "FOR u IN users RETURN u",
        "FOR u IN users FILTER u.age > 18 SORT u.name RETURN u",
        "SELECT * FROM users",   // Invalid AQL — triggers annotation
        "FOR p IN products LIMIT 100 RETURN p",
    };
    std::uniform_int_distribution<std::size_t> pick(0, queries.size() - 1);

    for (auto _ : state) {
        const auto& q = queries[pick(rng)];
        auto annotations = MockAQLHighlighter::annotate(q);
        benchmark::DoNotOptimize(annotations);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_AQLHighlighterSimple)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLTokenEstimation
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLTokenEstimation
 *
 * Measures the throughput of token count estimation for conversation
 * history entries (NL + AQL pairs).
 *
 * Release gate: p95 ≤ 50 µs per turn (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLTokenEstimation(benchmark::State& state) {
    const int history_len = static_cast<int>(state.range(0));

    // Build a fixed history
    std::vector<std::pair<std::string, std::string>> history;
    history.reserve(history_len);
    for (int i = 0; i < history_len; ++i) {
        history.push_back({
            "show me all records of type " + std::to_string(i),
            "FOR d IN collection FILTER d.type == " + std::to_string(i) + " RETURN d"
        });
    }

    for (auto _ : state) {
        auto total = MockTokenEstimator::estimateHistory(history);
        benchmark::DoNotOptimize(total);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * history_len);
}
BENCHMARK(BM_AQLTokenEstimation)
    ->Arg(5)
    ->Arg(20)
    ->Arg(50)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

}  // namespace aql_helpers
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
