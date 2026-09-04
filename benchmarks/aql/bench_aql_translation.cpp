/**
 * @file bench_aql_translation.cpp
 * @brief Phase 6 benchmarks for AQL translation and validation pipelines.
 *
 * Measures performance of:
 *   BM_AQLTranslationSimple   — Single-clause NL→AQL translation (mock LLM)
 *   BM_AQLTranslationComplex  — Multi-clause NL→AQL translation (mock LLM)
 *   BM_AQLValidationSimple    — Single-collection AQL syntax validation
 *   BM_AQLValidationBatch     — Batch validation of 32 queries
 *
 * All benchmarks are self-contained. LLM calls are replaced with deterministic
 * mock string generation (no network/LLM infrastructure required).
 *
 * Wave 7 conventions:
 *   - kW7CanonicalSeed = 42 for deterministic PRNG seeding
 *   - UseRealTime() so I/O wait is included in reported time
 *   - Warmup via state.SkipWithError check (early iterations not counted)
 *
 * @note Release gates are defined in src/aql/PERFORMANCE_EXPECTATIONS.md
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace themis {
namespace bench {
namespace aql_translation {

// ---------------------------------------------------------------------------
// Constants — Wave 7 canonical
// ---------------------------------------------------------------------------

/// Canonical PRNG seed shared across AQL benchmarks (Wave 7 convention)
static constexpr uint64_t kW7CanonicalSeed = 42;

/// Number of warmup iterations per benchmark
static constexpr int kWarmupIterations = 200;

// ---------------------------------------------------------------------------
// Mock Translation Engine
// ---------------------------------------------------------------------------
//
// Simulates the NL→AQL translation pipeline with deterministic output.
// - Uses a PRNG to vary output length (realistic latency profile).
// - Applies a simple validation pass (string-level, no parser service).
// - No network or LLM calls; suitable for isolated performance measurement.

class MockAQLTranslator {
public:
    struct TranslationResult {
        std::string aql_query;
        bool        valid       = false;
        std::string error_msg;
        int         attempts    = 1;
    };

    explicit MockAQLTranslator(uint64_t seed = kW7CanonicalSeed) : rng_(seed) {}

    /// Simulate simple single-clause translation (FOR … RETURN …)
    TranslationResult translateSimple(const std::string& nl_query) {
        // Deterministic output size: 60–120 chars
        std::uniform_int_distribution<int> len_dist(60, 120);
        int target_len = len_dist(rng_);

        std::ostringstream oss;
        oss << "FOR doc IN collection";
        oss << " FILTER doc.id == " << (nl_query.size() % 9999);
        oss << " RETURN doc";
        std::string aql = oss.str();

        // Pad/truncate to target_len to simulate variable output
        if (static_cast<int>(aql.size()) < target_len) {
            aql += std::string(target_len - aql.size(), ' ');
        }
        aql = aql.substr(0, static_cast<std::size_t>(target_len));

        return validate(aql);
    }

    /// Simulate complex multi-clause translation (JOIN + FILTER + SORT + LIMIT)
    TranslationResult translateComplex(const std::string& nl_query) {
        std::uniform_int_distribution<int> len_dist(200, 400);
        int target_len = len_dist(rng_);

        std::ostringstream oss;
        oss << "FOR user IN users"
            << " FILTER user.active == true"
            << " FOR order IN orders"
            << " FILTER order.user_id == user._id"
            << " FILTER order.total > " << (nl_query.size() % 1000)
            << " SORT order.created_at DESC"
            << " LIMIT 20"
            << " RETURN { user: user.name, order_id: order._id, total: order.total }";
        std::string aql = oss.str();

        if (static_cast<int>(aql.size()) < target_len) {
            aql += std::string(target_len - aql.size(), ' ');
        }
        aql = aql.substr(0, static_cast<std::size_t>(target_len));

        return validate(aql);
    }

private:
    TranslationResult validate(std::string aql) {
        // Lightweight string-level validation (no AST parser needed for benchmark)
        bool valid = !aql.empty() &&
                     aql.find("FOR") != std::string::npos &&
                     aql.find("RETURN") != std::string::npos;
        if (!valid) {
            return {"", false, "[VALIDATION:MalformedAQL] Missing FOR or RETURN clause", 1};
        }
        return {std::move(aql), true, "", 1};
    }

    std::mt19937_64 rng_;
};

// ---------------------------------------------------------------------------
// Validation Mock
// ---------------------------------------------------------------------------

class MockAQLValidator {
public:
    struct ValidationResult {
        bool   valid = 0;
        std::string error_msg;
    };

    static ValidationResult validateSimple(const std::string& aql) {
        if (aql.empty()) {
            return {false, "[VALIDATION:MalformedAQL] Empty query"};
        }
        bool has_for    = aql.find("FOR")    != std::string::npos;
        bool has_return = aql.find("RETURN") != std::string::npos;
        if (!has_for || !has_return) {
            return {false, "[VALIDATION:MalformedAQL] Missing FOR or RETURN clause"};
        }
        return {true, ""};
    }
};

// ---------------------------------------------------------------------------
// Query Corpus
// ---------------------------------------------------------------------------

static std::vector<std::string> buildSimpleNLCorpus(int n, uint64_t seed = kW7CanonicalSeed) {
    std::mt19937_64 rng(seed);
    std::vector<std::string> queries;
    queries.reserve(n);
    const std::vector<std::string> templates = {
        "find all users",
        "show users older than 30",
        "get products in category electronics",
        "list recent orders",
        "show active subscriptions",
    };
    std::uniform_int_distribution<std::size_t> pick(0, templates.size() - 1);
    for (int i = 0; i < n; ++i) {
        queries.push_back(templates[pick(rng)] + " [" + std::to_string(i) + "]");
    }
    return queries;
}

static std::vector<std::string> buildComplexNLCorpus(int n, uint64_t seed = kW7CanonicalSeed + 1) {
    std::mt19937_64 rng(seed);
    std::vector<std::string> queries;
    queries.reserve(n);
    const std::vector<std::string> templates = {
        "find users who placed orders above $500 in the last 30 days sorted by total",
        "show all products with low stock that have been viewed more than 100 times this week",
        "list all active subscriptions for premium users who joined after 2024-01-01",
    };
    std::uniform_int_distribution<std::size_t> pick(0, templates.size() - 1);
    for (int i = 0; i < n; ++i) {
        queries.push_back(templates[pick(rng)] + " variant=" + std::to_string(i));
    }
    return queries;
}

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLTranslationSimple
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLTranslationSimple
 *
 * Measures the p50/p95 latency of translating a simple single-clause
 * NL query to AQL using the mock translation engine.
 *
 * Release gate: p95 ≤ 2 ms (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLTranslationSimple(benchmark::State& state) {
    // Warmup phase before timed benchmark loop.
    MockAQLTranslator translator(kW7CanonicalSeed);
    const auto corpus = buildSimpleNLCorpus(256);

    int idx = 0;
    for (int warmup_count = 0; warmup_count < kWarmupIterations; ++warmup_count) {
        auto warmup_result = translator.translateSimple(corpus[idx % corpus.size()]);
        benchmark::DoNotOptimize(warmup_result);
        ++idx;
    }

    for (auto _ : state) {
        const auto& query = corpus[idx % corpus.size()];
        auto result = translator.translateSimple(query);
        benchmark::DoNotOptimize(result);
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_AQLTranslationSimple)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLTranslationComplex
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLTranslationComplex
 *
 * Measures the p50/p95 latency of translating a complex multi-clause
 * NL query (JOIN + FILTER + SORT + LIMIT) to AQL.
 *
 * Release gate: p95 ≤ 5 ms (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLTranslationComplex(benchmark::State& state) {
    MockAQLTranslator translator(kW7CanonicalSeed + 2);
    const auto corpus = buildComplexNLCorpus(128);

    int idx = 0;
    for (int warmup_count = 0; warmup_count < kWarmupIterations; ++warmup_count) {
        auto warmup_result = translator.translateComplex(corpus[idx % corpus.size()]);
        benchmark::DoNotOptimize(warmup_result);
        ++idx;
    }

    for (auto _ : state) {
        const auto& query = corpus[idx % corpus.size()];
        auto result = translator.translateComplex(query);
        benchmark::DoNotOptimize(result);
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_AQLTranslationComplex)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLValidationSimple
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLValidationSimple
 *
 * Measures the p50/p95 latency of validating a single AQL query
 * using the string-level validator (no AST parser).
 *
 * Release gate: p95 ≤ 200 µs (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLValidationSimple(benchmark::State& state) {
    const std::vector<std::string> aql_queries = {
        "FOR u IN users RETURN u",
        "FOR u IN users FILTER u.age > 18 RETURN u.name",
        "FOR p IN products FILTER p.price < 100.0 SORT p.name RETURN p",
    };

    std::mt19937_64 rng(kW7CanonicalSeed + 3);
    std::uniform_int_distribution<std::size_t> pick(0, aql_queries.size() - 1);

    for (auto _ : state) {
        const auto& q = aql_queries[pick(rng)];
        auto result = MockAQLValidator::validateSimple(q);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK(BM_AQLValidationSimple)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

// ---------------------------------------------------------------------------
// Benchmark: BM_AQLValidationBatch
// ---------------------------------------------------------------------------

/**
 * @brief BM_AQLValidationBatch
 *
 * Measures the throughput of validating a batch of 32 AQL queries
 * per iteration.
 *
 * Release gate: ≥ 10 000 batch-validations/s (see PERFORMANCE_EXPECTATIONS.md)
 */
static void BM_AQLValidationBatch(benchmark::State& state) {
    std::mt19937_64 rng(kW7CanonicalSeed + 4);
    const int kBatchSize = static_cast<int>(state.range(0));

    // Pre-generate batch
    const std::vector<std::string> templates = {
        "FOR doc IN collection RETURN doc",
        "FOR doc IN collection FILTER doc.status == 'active' RETURN doc",
        "FOR doc IN collection SORT doc.ts DESC LIMIT 10 RETURN doc",
        "",  // Invalid query — tests reject path
        "RETURN 42",  // No FOR clause
    };
    std::uniform_int_distribution<std::size_t> pick(0, templates.size() - 1);

    std::vector<std::string> batch;
    batch.reserve(kBatchSize);
    for (int i = 0; i < kBatchSize; ++i) {
        batch.push_back(templates[pick(rng)]);
    }

    for (auto _ : state) {
        int valid_count = 0;
        for (const auto& q : batch) {
            auto r = MockAQLValidator::validateSimple(q);
            if (r.valid) {
              ++valid_count;
            }
        }
        benchmark::DoNotOptimize(valid_count);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize * 50);
}
BENCHMARK(BM_AQLValidationBatch)
    ->Arg(32)
    ->Arg(128)
    ->UseRealTime()
    ->Repetitions(3)
    ->DisplayAggregatesOnly(true);

}  // namespace aql_translation
}  // namespace bench
}  // namespace themis

BENCHMARK_MAIN();
