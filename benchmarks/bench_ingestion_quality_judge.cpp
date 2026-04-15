/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_ingestion_quality_judge.cpp                  ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 18:00:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   74.0/100                                       ║
    • Total Lines:     592                                            ║
    • Open Issues:     TODOs: 0, Stubs: 7                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB – Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_ingestion_quality_judge.cpp                  ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-15                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file bench_ingestion_quality_judge.cpp
 * @brief Google Benchmark suite for IngestionQualityJudge (Ingestion Phase 7).
 *
 * ## Scientific Foundation
 *
 * The quality-judge subsystem is grounded in three lines of research:
 *
 * 1. **LLM-as-judge** (Zheng et al., NeurIPS 2023)
 *    "Judging LLM-as-a-Judge with MT-Bench and Chatbot Arena"
 *    https://arxiv.org/abs/2306.05685
 *    → Motivation for using a language model as an evaluator for extraction
 *      quality rather than hand-crafted heuristics.
 *
 * 2. **RAGAS** (Es et al., EACL 2024)
 *    "RAGAS: Automated Evaluation of Retrieval Augmented Generation"
 *    https://arxiv.org/abs/2309.15217
 *    → The four quality dimensions (faithfulness ≈ groundedness,
 *      answer relevance ≈ completeness, context precision/recall ≈
 *      entity_coverage / relation_coherence) directly mirror RAGAS metrics.
 *
 * 3. **CRAG – Corrective Retrieval Augmented Generation** (Yan et al., ICLR 2024)
 *    https://arxiv.org/abs/2401.15884
 *    → Inspiration for the re-ingestion feedback loop in ReIngestionController:
 *      evaluate quality → if below threshold, correct and retry (up to
 *      max_reingestion_attempts).
 *
 * ## SoC / OOP Design
 *
 * | Concern                     | Component                           |
 * |-----------------------------|-------------------------------------|
 * | Text generation (LLM call)  | `ITextGenerationBackend`            |
 * | Quality evaluation logic    | `IngestionQualityJudge`             |
 * | Workflow orchestration      | `WorkflowEngine`                    |
 * | Re-ingestion loop           | `ReIngestionController`             |
 * | Observability               | `IIngestionQualityObserver`         |
 *
 * No component knows about the internals of its neighbours — they
 * communicate only through the interfaces declared in the public headers.
 * This follows the Dependency-Inversion Principle (DIP) of SOLID.
 *
 * ## Benchmark Scenarios
 *
 * | ID   | Scenario                                               |
 * |------|--------------------------------------------------------|
 * | QJ01 | evaluate() — NullBackend (fail-open fast path)         |
 * | QJ02 | evaluate() — single dimension (completeness only)     |
 * | QJ03 | evaluate() — all four dimensions (full eval path)     |
 * | QJ04 | evaluate() — sparse context shortcut (below threshold)|
 * | QJ05 | evaluate() — scaling with entity count               |
 * | QJ06 | evaluate() — response with bullet lists (parse cost)  |
 * | QJ07 | Observer dispatch — zero observers (baseline)        |
 * | QJ08 | Observer dispatch — N observers (notification cost)  |
 * | QJ09 | Config mutation — setConfig() overhead               |
 * | QJ10 | Construction / destruction cost                      |
 *
 * ## Performance Targets
 *
 * - QJ01 (fail-open):          < 1 µs   (no LLM call)
 * - QJ04 (sparse skip):        < 1 µs   (no LLM call)
 * - QJ02 (single dim, stub):   < 10 µs  (stub generate() + string parsing)
 * - QJ03 (all dims, stub):     < 40 µs  (4× stub LLM + aggregation)
 * - QJ07/08 (observer):        < 5 µs   per notification batch (lock snapshot)
 * - QJ09 (setConfig):          < 500 ns (struct copy)
 * - QJ10 (ctor/dtor):          < 1 µs
 */

#include <benchmark/benchmark.h>

#include "ingestion/ingestion_quality_judge.h"
#include "ingestion/extraction_context.h"
#include "ingestion/base_entity.h"
#include "ingestion/file_manifest.h"
#include "ingestion/inference_backend.h"

#include <memory>
#include <string>
#include <vector>
#include <sstream>

using namespace themis::ingestion;

// ─── Scripted backend (deterministic LLM stub) ───────────────────────────────

/**
 * @brief Lightweight in-process text generation stub for benchmarking.
 *
 * Returns a pre-configured response string for every prompt without
 * any I/O or thread synchronisation — ideal for isolating the
 * IngestionQualityJudge overhead from any real LLM cost.
 *
 * STUB/SIMULATION NOTE:
 * Purpose:    Benchmark isolation — measures judge overhead, not LLM latency.
 * Activation: Only used inside this benchmark compilation unit.
 * Production Delta: isAvailable() = true; generate() returns a fixed string.
 * Removal Plan: Not needed in production; stays test-scoped.
 */
class ScriptedTextBackend : public ITextGenerationBackend {
public:
    explicit ScriptedTextBackend(std::string response, bool available = true)
        : response_(std::move(response)), available_(available) {}

    std::string generate(const std::string& /*prompt*/,
                         int    /*max_tokens*/,
                         double /*temperature*/,
                         const std::string& /*lora*/) override {
        return response_;
    }

    bool        isAvailable() const override { return available_; }
    std::string description() const override { return "ScriptedTextBackend-bench-v1"; }

    void setResponse(const std::string& r) { response_ = r; }

private:
    std::string response_;
    bool        available_;
};

// ─── Counting observer (zero-overhead sink for benchmarks) ───────────────────

class NullQualityObserver : public IIngestionQualityObserver {
public:
    void onQualityEvaluated(const std::string&,
                            const IngestionQualityReport&) noexcept override {}
    void onReIngestionTriggered(const std::string&, int,
                                const std::vector<std::string>&) noexcept override {}
    void onReIngestionComplete(const std::string&, int, bool) noexcept override {}
};

// ─── Fixture helpers ─────────────────────────────────────────────────────────

/// Build a minimal but valid ExtractionContext with @p entity_count entities.
static ExtractionContext makeCtx(const std::string& raw_text,
                                 size_t entity_count = 1)
{
    ExtractionContext ctx;
    ctx.raw_text = raw_text;
    ctx.manifest.source_uri = "bench://test-document.txt";
    ctx.doc_id = "bench-doc-001";

    ctx.entities.reserve(entity_count);
    for (size_t i = 0; i < entity_count; ++i) {
        BaseEntity e;
        e.id    = "e" + std::to_string(i);
        e.label = "§ " + std::to_string(i + 1) + " BGB";
        e.type  = "LEGAL_PROVISION";
        ctx.entities.push_back(std::move(e));
    }
    return ctx;
}

/// Typical 200-byte German legal text used as benchmark input.
static const char* kLegalText200 =
    "Gemäß § 42 BGB ist der Verein verpflichtet, den Mitgliedern die "
    "Beschlüsse des Vorstands mitzuteilen. Der Vorstand besteht aus "
    "mindestens drei Personen.";

/// Typical 1000-byte legal text for scaling tests.
static const char* kLegalText1000 =
    "Gemäß § 42 BGB (Auflösung des Vereins) ist der Verein auflösbar. "
    "Der Bundesgerichtshof hat in seinem Urteil vom 12. März 2023 "
    "(Az. IV ZR 123/22) entschieden, dass die Satzungsänderung einer "
    "Dreiviertelmehrheit bedarf. Das Bürgerliche Gesetzbuch regelt in "
    "§ 823 BGB die Schadensersatzpflicht. Hans Müller, Vorsitzender des "
    "Deutschen Anwaltsvereins e.V. (DAV), erklärte in seiner Pressemitteilung "
    "vom 5. April 2023, dass die neue Rechtsprechung weitreichende Folgen für "
    "alle eingetragenen Vereine haben werde. Die Vorschrift des § 33 BGB zur "
    "Satzungsänderung sei daher im Licht des Urteils neu zu lesen. "
    "Zudem verwies er auf § 54 BGB und die Rechtsprechung des OLG München.";

/// Standard scored response format expected by the parser.
static std::string makeScoredResponse(double score,
                                      const std::string& extra = "")
{
    std::ostringstream oss;
    oss << "SCORE: " << score << "\n"
        << "RATIONALE: Synthetic benchmark response — no real LLM used.\n"
        << extra;
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// QJ01 – evaluate() with NullBackend (fail-open fast path)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures the overhead of evaluate() when the backend is unavailable.
 * This is the "fail-open" guard path: the judge returns immediately with
 * all scores = -1.0 and passed = true.  No prompt building, no parsing.
 *
 * Expected: < 1 µs (branch + struct initialisation only).
 */
static void BM_QJ01_EvaluateNullBackend(benchmark::State& state)
{
    auto backend = std::make_shared<NullTextGenerationBackend>();
    IngestionQualityJudge judge(backend);
    const auto ctx = makeCtx(kLegalText200, 2);

    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.passed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("NullBackend → fail-open (no LLM call, no prompt build)");
}
BENCHMARK(BM_QJ01_EvaluateNullBackend)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ02 – evaluate() with one dimension enabled (completeness only)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures the full evaluate() path with exactly one dimension active.
 * Captures: prompt building (completeness), generate() stub call, parseScore(),
 * parseRationale(), parseBulletList(), overall aggregation.
 *
 * Expected: < 10 µs (string ops + stub call overhead).
 */
static void BM_QJ02_EvaluateSingleDimension(benchmark::State& state)
{
    const auto resp = makeScoredResponse(0.90);
    auto backend    = std::make_shared<ScriptedTextBackend>(resp);

    IngestionJudgeConfig cfg;
    cfg.evaluate_completeness       = true;
    cfg.evaluate_groundedness       = false;
    cfg.evaluate_entity_coverage    = false;
    cfg.evaluate_relation_coherence = false;

    IngestionQualityJudge judge(backend, cfg);
    const auto ctx = makeCtx(kLegalText200, 2);

    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.completeness_score);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Single dimension (completeness) — scripted backend");
}
BENCHMARK(BM_QJ02_EvaluateSingleDimension)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ03 – evaluate() with all four dimensions (full evaluation path)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Full pipeline: 4 prompt builds, 4 generate() calls, 4 score parses,
 * 4 rationale parses, weighted aggregation, threshold check, step recommendation.
 *
 * Expected: < 40 µs (4× single-dimension cost + dedup pass).
 *
 * This is the hot path in production when a real LLM is injected — the
 * benchmark measures only ThemisDB's own overhead (prompt stitching, parsing,
 * aggregation) in isolation from actual LLM latency.
 */
class AllDimsFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        resp    = makeScoredResponse(0.88);
        backend = std::make_shared<ScriptedTextBackend>(resp);
        judge   = std::make_unique<IngestionQualityJudge>(backend);
        ctx     = makeCtx(kLegalText1000, 5);
    }

    void TearDown(const benchmark::State&) override {
        judge.reset();
        backend.reset();
    }

    std::string                                   resp;
    std::shared_ptr<ScriptedTextBackend>           backend;
    std::unique_ptr<IngestionQualityJudge>         judge;
    ExtractionContext                              ctx;
};

BENCHMARK_F(AllDimsFixture, QJ03_EvaluateAllDimensions)(benchmark::State& state)
{
    for (auto _ : state) {
        auto report = judge->evaluate(ctx);
        benchmark::DoNotOptimize(report.overall_score);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("All 4 dimensions — scripted backend, 1000-byte legal text");
}

// ─────────────────────────────────────────────────────────────────────────────
// QJ04 – evaluate() sparse context (below min_text_bytes_for_eval)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * When the document is too short or has too few entities the judge skips
 * all LLM calls and returns immediately (fail-open).
 * Identical performance target to QJ01.
 */
static void BM_QJ04_EvaluateSparseContext(benchmark::State& state)
{
    const auto resp = makeScoredResponse(0.90);
    auto backend    = std::make_shared<ScriptedTextBackend>(resp);

    IngestionJudgeConfig cfg;
    cfg.min_text_bytes_for_eval = 1000; // threshold > length of sparse text
    cfg.min_entities_for_eval   = 5;    // threshold > 0 entities

    IngestionQualityJudge judge(backend, cfg);
    // Sparse context: 50 bytes, 0 entities
    const auto ctx = makeCtx("Kurz.", 0);

    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.passed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Sparse context shortcut (no LLM call)");
}
BENCHMARK(BM_QJ04_EvaluateSparseContext)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ05 – evaluate() scaling with entity count
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures how evaluate() overhead grows with the number of extracted entities.
 * The entity summary (up to 30 labels) and relation summary are embedded in
 * prompts — this test quantifies the string-formatting cost at scale.
 *
 * Argument: entity count per document.
 */
static void BM_QJ05_EvaluateEntityScaling(benchmark::State& state)
{
    const auto resp = makeScoredResponse(0.85);
    auto backend    = std::make_shared<ScriptedTextBackend>(resp);

    // Enable only the two dimensions that embed entity lists in the prompt.
    IngestionJudgeConfig cfg;
    cfg.evaluate_completeness       = true;
    cfg.evaluate_entity_coverage    = true;
    cfg.evaluate_groundedness       = false;
    cfg.evaluate_relation_coherence = false;

    IngestionQualityJudge judge(backend, cfg);
    const auto ctx = makeCtx(kLegalText1000,
                              static_cast<size_t>(state.range(0)));

    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.entity_coverage_score);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Entity scaling — " + std::to_string(state.range(0)) + " entities");
}
BENCHMARK(BM_QJ05_EvaluateEntityScaling)
    ->Arg(1)->Arg(10)->Arg(50)->Arg(200)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ06 – evaluate() response with bullet lists (parser cost)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures the overhead of parseBulletList() when the LLM response contains
 * many missing-entity and hint entries.  This is the worst-case parser path.
 */
static void BM_QJ06_EvaluateBulletListParsing(benchmark::State& state)
{
    // Build a response with N bullet entries.
    const int kBullets = static_cast<int>(state.range(0));
    std::ostringstream resp_builder;
    resp_builder << makeScoredResponse(0.60);
    resp_builder << "MISSING:\n";
    for (int i = 0; i < kBullets; ++i)
        resp_builder << "- Entity_" << i << " (§ " << i << " BGB)\n";
    resp_builder << "HINTS:\n";
    for (int i = 0; i < kBullets; ++i)
        resp_builder << "- Hint_" << i << ": re-run NER for entity type LAW\n";

    const auto resp = resp_builder.str();
    auto backend    = std::make_shared<ScriptedTextBackend>(resp);

    IngestionJudgeConfig cfg;
    cfg.evaluate_completeness       = true;
    cfg.evaluate_groundedness       = false;
    cfg.evaluate_entity_coverage    = false;
    cfg.evaluate_relation_coherence = false;
    cfg.completeness_threshold      = 0.80; // 0.60 < 0.80 → fails → bullets parsed

    IngestionQualityJudge judge(backend, cfg);
    const auto ctx = makeCtx(kLegalText1000, 2);

    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.missing_entities.size());
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Bullet list parsing — " + std::to_string(kBullets) + " entries");
}
BENCHMARK(BM_QJ06_EvaluateBulletListParsing)
    ->Arg(0)->Arg(5)->Arg(20)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ07 – Observer dispatch — zero observers (baseline)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Baseline: evaluate() with no observers attached.
 * Measures the pure judge overhead without any notification cost.
 */
static void BM_QJ07_ObserverDispatch_Zero(benchmark::State& state)
{
    const auto resp = makeScoredResponse(0.90);
    auto backend    = std::make_shared<ScriptedTextBackend>(resp);

    IngestionJudgeConfig cfg;
    cfg.evaluate_completeness       = true;
    cfg.evaluate_groundedness       = false;
    cfg.evaluate_entity_coverage    = false;
    cfg.evaluate_relation_coherence = false;

    IngestionQualityJudge judge(backend, cfg);
    const auto ctx = makeCtx(kLegalText200, 2);

    // No observers added.
    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.passed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Observer dispatch — 0 observers (baseline)");
}
BENCHMARK(BM_QJ07_ObserverDispatch_Zero)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ08 – Observer dispatch — N observers (notification cost)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures the overhead of notifying N attached observers per evaluation.
 * The observer implementation is a no-op to isolate the dispatch cost
 * (lock acquisition + snapshot copy + virtual call loop).
 *
 * Argument: number of observers.
 */
static void BM_QJ08_ObserverDispatch_N(benchmark::State& state)
{
    const auto resp  = makeScoredResponse(0.90);
    auto backend     = std::make_shared<ScriptedTextBackend>(resp);

    IngestionJudgeConfig cfg;
    cfg.evaluate_completeness       = true;
    cfg.evaluate_groundedness       = false;
    cfg.evaluate_entity_coverage    = false;
    cfg.evaluate_relation_coherence = false;

    IngestionQualityJudge judge(backend, cfg);
    const auto ctx = makeCtx(kLegalText200, 2);

    const int kObservers = static_cast<int>(state.range(0));
    for (int i = 0; i < kObservers; ++i)
        judge.addObserver(std::make_shared<NullQualityObserver>());

    for (auto _ : state) {
        auto report = judge.evaluate(ctx);
        benchmark::DoNotOptimize(report.passed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("Observer dispatch — " + std::to_string(kObservers) + " observers");
}
BENCHMARK(BM_QJ08_ObserverDispatch_N)
    ->Arg(1)->Arg(4)->Arg(16)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ09 – Config mutation — setConfig() overhead
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures the cost of mutating a judge's configuration at runtime.
 * In production this happens at most once per configuration reload cycle.
 *
 * Expected: < 500 ns (plain struct copy, no allocation).
 */
static void BM_QJ09_SetConfig(benchmark::State& state)
{
    auto backend = std::make_shared<NullTextGenerationBackend>();
    IngestionQualityJudge judge(backend);

    IngestionJudgeConfig cfg;
    cfg.completeness_threshold    = 0.80;
    cfg.groundedness_threshold    = 0.80;
    cfg.max_reingestion_attempts  = 5;

    for (auto _ : state) {
        judge.setConfig(cfg);
        benchmark::DoNotOptimize(judge.config().completeness_threshold);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("setConfig() — struct copy, no alloc");
}
BENCHMARK(BM_QJ09_SetConfig)->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ10 – Construction / destruction cost
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Measures the overhead of constructing and immediately destroying
 * an IngestionQualityJudge.
 *
 * In production the judge is created once per IngestionManager lifetime,
 * so this is not on the critical path — it serves as a sanity check that
 * there are no hidden allocations in the constructor.
 *
 * Expected: < 1 µs.
 */
static void BM_QJ10_ConstructDestruct(benchmark::State& state)
{
    auto backend = std::make_shared<NullTextGenerationBackend>();

    for (auto _ : state) {
        IngestionQualityJudge judge(backend);
        benchmark::DoNotOptimize(&judge);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("IngestionQualityJudge ctor + dtor");
}
BENCHMARK(BM_QJ10_ConstructDestruct)->Unit(benchmark::kNanosecond);

// ─────────────────────────────────────────────────────────────────────────────
// QJ11 – evaluate() feedback loop simulation (CRAG-style re-ingestion cost)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Simulates the quality-judge side of the re-ingestion feedback loop
 * (motivated by CRAG, Yan et al., ICLR 2024) without involving the
 * WorkflowEngine — useful for isolating the judge's contribution to
 * end-to-end latency as the number of re-evaluation passes grows.
 *
 * Argument: number of evaluate() passes in the loop.
 */
static void BM_QJ11_FeedbackLoopJudgeOnly(benchmark::State& state)
{
    const auto resp = makeScoredResponse(0.90);
    auto backend    = std::make_shared<ScriptedTextBackend>(resp);
    IngestionQualityJudge judge(backend);
    const auto ctx = makeCtx(kLegalText1000, 5);

    const int kPasses = static_cast<int>(state.range(0));

    for (auto _ : state) {
        double best = -1.0;
        for (int p = 0; p < kPasses; ++p) {
            auto report = judge.evaluate(ctx);
            if (report.overall_score > best) best = report.overall_score;
            if (report.passed) break;
        }
        benchmark::DoNotOptimize(best);
    }

    state.SetItemsProcessed(state.iterations() * kPasses);
    state.SetLabel("CRAG-style loop — " + std::to_string(kPasses) + " judge passes");
}
BENCHMARK(BM_QJ11_FeedbackLoopJudgeOnly)
    ->Arg(1)->Arg(2)->Arg(3)->Arg(5)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

BENCHMARK_MAIN();
