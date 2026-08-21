/**
 * @file bench_hybrid_aql_sugar.cpp
 * @brief Performance benchmarks for hybrid AQL syntax-sugar components.
 *
 * Covers the deterministic, LLM-independent layers of the AQL module:
 *   - AQLSyntaxHighlighter  (tokenize, highlightBlock, annotateErrors,
 *                            formatLLMResponse)
 *   - AQLConfidenceScorer   (score with / without schema context)
 *   - AQLFewShotExampleLibrary (findRelevant, buildPromptSection,
 *                               findByDomain)
 *
 * All benchmarks are intentionally independent of a live LLM so that they
 * can run in CI without model files or network access.
 */

#include <benchmark/benchmark.h>
#include "aql/aql_syntax_highlighter.h"
#include "aql/aql_confidence_scorer.h"
#include "aql/aql_fewshot_example_library.h"

using namespace themis::aql;

// ============================================================================
// Shared fixtures / constants
// ============================================================================

// A representative multi-clause AQL query used across several benchmarks.
static const char* kSimpleAQL =
    "FOR u IN users FILTER u.city == \"Berlin\" SORT u.name RETURN u";

// A moderately complex query that exercises more token types.
static const char* kComplexAQL =
    "FOR u IN users\n"
    "  FILTER u.age >= 18 AND u.active == true\n"
    "  LET score = SIMILARITY(u.embedding, @query_vec)\n"
    "  SORT score DESC\n"
    "  LIMIT 10\n"
    "  RETURN { id: u._id, name: u.name, score: score }";

// An LLM response containing an embedded AQL code block.
static const char* kLLMResponse =
    "Here is the query you requested:\n\n"
    "```aql\n"
    "FOR doc IN products\n"
    "  FILTER doc.price < 100\n"
    "  SORT doc.rating DESC\n"
    "  RETURN doc\n"
    "```\n\n"
    "This returns all products cheaper than 100, sorted by rating.";

// Schema context string used for schema-aware scoring.
static const char* kSchemaContext =
    "collections: [users, orders, products]\n"
    "users: { _id, name, age, city, active, embedding }";

// ============================================================================
// AQLSyntaxHighlighter benchmarks
// ============================================================================

static void BM_Highlighter_Tokenize_Simple(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.tokenize(kSimpleAQL));
    }
}
BENCHMARK(BM_Highlighter_Tokenize_Simple);

static void BM_Highlighter_Tokenize_Complex(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.tokenize(kComplexAQL));
    }
}
BENCHMARK(BM_Highlighter_Tokenize_Complex);

// Tokenization throughput as query length scales.
static void BM_Highlighter_Tokenize_Scale(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    // Build a query of roughly state.range(0) * 10 characters.
    std::string query;
    query.reserve(static_cast<std::size_t>(state.range(0)) * 24);
    for (int i = 0; i < state.range(0); ++i) {
        query += "FILTER u.x" + std::to_string(i) + " == " + std::to_string(i) + " ";
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.tokenize(query));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Highlighter_Tokenize_Scale)->Range(1, 64)->Complexity();

static void BM_Highlighter_HighlightBlock_Plain(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.highlightBlock(kComplexAQL));
    }
}
BENCHMARK(BM_Highlighter_HighlightBlock_Plain);

static void BM_Highlighter_HighlightBlock_ANSI(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/true);
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.highlightBlock(kComplexAQL));
    }
}
BENCHMARK(BM_Highlighter_HighlightBlock_ANSI);

static void BM_Highlighter_AnnotateErrors_Valid(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.annotateErrors(kComplexAQL));
    }
}
BENCHMARK(BM_Highlighter_AnnotateErrors_Valid);

static void BM_Highlighter_AnnotateErrors_Malformed(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    // Intentionally malformed: unbalanced brace, unterminated string.
    static const char* kBad = "FOR u IN users { FILTER u.name == \"Alice RETURN u";
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.annotateErrors(kBad));
    }
}
BENCHMARK(BM_Highlighter_AnnotateErrors_Malformed);

static void BM_Highlighter_FormatLLMResponse(benchmark::State& state) {
    AQLSyntaxHighlighter h(/*use_ansi=*/false);
    for (auto _ : state) {
        benchmark::DoNotOptimize(h.formatLLMResponse(kLLMResponse));
    }
}
BENCHMARK(BM_Highlighter_FormatLLMResponse);

// ============================================================================
// AQLConfidenceScorer benchmarks
// ============================================================================

static void BM_ConfidenceScorer_NoSchema(benchmark::State& state) {
    AQLConfidenceScorer scorer;
    for (auto _ : state) {
        benchmark::DoNotOptimize(scorer.score(kComplexAQL));
    }
}
BENCHMARK(BM_ConfidenceScorer_NoSchema);

static void BM_ConfidenceScorer_WithSchema(benchmark::State& state) {
    AQLConfidenceScorer scorer;
    for (auto _ : state) {
        benchmark::DoNotOptimize(scorer.score(kComplexAQL, "", kSchemaContext));
    }
}
BENCHMARK(BM_ConfidenceScorer_WithSchema);

static void BM_ConfidenceScorer_Simple(benchmark::State& state) {
    AQLConfidenceScorer scorer;
    for (auto _ : state) {
        benchmark::DoNotOptimize(scorer.score(kSimpleAQL));
    }
}
BENCHMARK(BM_ConfidenceScorer_Simple);

// Scoring throughput as query length scales.
static void BM_ConfidenceScorer_Scale(benchmark::State& state) {
    AQLConfidenceScorer scorer;
    std::string query = "FOR u IN users ";
    for (int i = 0; i < state.range(0); ++i) {
        query += "FILTER u.f" + std::to_string(i) + " == " + std::to_string(i) + " ";
    }
    query += "RETURN u";
    for (auto _ : state) {
        benchmark::DoNotOptimize(scorer.score(query));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_ConfidenceScorer_Scale)->Range(1, 64)->Complexity();

// ============================================================================
// AQLFewShotExampleLibrary benchmarks
// ============================================================================

class FewShotFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        lib_ = std::make_unique<AQLFewShotExampleLibrary>();
    }
    std::unique_ptr<AQLFewShotExampleLibrary> lib_;
};

BENCHMARK_F(FewShotFixture, FindRelevant_Top3)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            lib_->findRelevant("find all users in a city", 3));
    }
}

BENCHMARK_F(FewShotFixture, FindRelevant_Top5)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            lib_->findRelevant("find all users in a city", 5));
    }
}

BENCHMARK_F(FewShotFixture, FindRelevant_DomainFilter)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            lib_->findRelevant("shortest path between nodes", 3,
                               AQLExampleDomain::GRAPH));
    }
}

BENCHMARK_F(FewShotFixture, BuildPromptSection)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            lib_->buildPromptSection("vector similarity search", 3));
    }
}

BENCHMARK_F(FewShotFixture, FindByDomain_Graph)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            lib_->findByDomain(AQLExampleDomain::GRAPH));
    }
}

BENCHMARK_F(FewShotFixture, FindByDomain_Vector)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            lib_->findByDomain(AQLExampleDomain::VECTOR));
    }
}

BENCHMARK_F(FewShotFixture, FormatForPrompt)(benchmark::State& state) {
    auto examples = lib_->findByDomain(AQLExampleDomain::DOCUMENT);
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            AQLFewShotExampleLibrary::formatForPrompt(examples));
    }
}

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
