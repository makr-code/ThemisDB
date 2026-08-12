/**
 * @file bench_prompt_engineering.cpp
 * @brief Performance benchmarks for the prompt engineering module.
 *
 * Measures:
 *   1. PromptManager::createTemplate        – in-memory insert throughput
 *   2. PromptManager::getTemplate           – in-memory lookup latency (hit / miss)
 *   3. PromptManager::injectContext         – variable substitution throughput
 *   4. PromptManager::validateTemplate      – validation latency (valid / invalid)
 *   5. PromptVersionControl::commit         – version commit throughput
 *   6. PromptVersionControl::getHistory     – history retrieval (10 versions)
 *   7. FeedbackCollector::recordFeedback    – recording throughput
 *   8. FeedbackCollector::getStats          – aggregate stats latency
 *   9. PromptOptimizer::optimize            – single-iteration loop latency
 *  10. PromptEvaluator::evaluateSingle      – structural evaluation latency
 *  11. PromptEngineeringMetrics (record*)   – metrics recording throughput
 *  12. PromptManager concurrent writes      – 4-thread TBB concurrent_hash_map insert
 *  13. PromptVersionControl concurrent commits – 4-thread concurrent commit throughput
 *
 * Performance targets (from src/prompt_engineering/ROADMAP.md):
 *   - createTemplate (in-memory):   < 10 µs
 *   - getTemplate    (in-memory):   < 1 µs
 *   - injectContext  (5 vars):      < 5 µs
 *   - validateTemplate (valid):     < 5 µs
 *   - commit (in-memory):           < 20 µs
 *   - recordFeedback (in-memory):   < 10 µs
 *   - optimize (1 iter):            < 500 µs
 *   - evaluatePrompt (structural):  < 50 µs
 *
 * Build:
 *   cmake -DTHEMIS_BUILD_BENCHMARKS=ON ... && cmake --build . --target bench_prompt_engineering
 * Run:
 *   ./benchmarks/bench_prompt_engineering --benchmark_format=json
 */

#include <benchmark/benchmark.h>

#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_version_control.h"
#include "prompt_engineering/feedback_collector.h"
#include "prompt_engineering/prompt_evaluator.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_engineering_metrics.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::prompt_engineering;

// ============================================================================
// Shared fixtures
// ============================================================================

namespace {

// A valid template used across benchmarks.
static PromptManager::PromptTemplate MakeTemplate(int i = 0) {
    PromptManager::PromptTemplate t;
    t.name        = "bench_tmpl_" + std::to_string(i);
    t.version     = "v1";
    t.content     = "You are a helpful assistant. User query: {query}. Context: {context}. "
                    "Language: {lang}. Format: {format}. Extra: {extra}.";
    t.description = "Benchmark template " + std::to_string(i);
    t.active      = true;
    return t;
}

// Context map matching the 5 placeholders in MakeTemplate().
static std::unordered_map<std::string, std::string> MakeCtx() {
    return {
        {"query",   "What is ThemisDB?"},
        {"context", "ThemisDB is a hybrid database"},
        {"lang",    "en"},
        {"format",  "markdown"},
        {"extra",   "no additional info"},
    };
}

} // anonymous namespace

// ============================================================================
// 1. PromptManager::createTemplate – in-memory insert throughput
// ============================================================================

static void BM_PromptManager_CreateTemplate(benchmark::State& state) {
    PromptManager mgr;
    int i = 0;
    for (auto _ : state) {
        auto t  = MakeTemplate(i++);
        auto r  = mgr.createTemplate(std::move(t));
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptManager_CreateTemplate);

// ============================================================================
// 2. PromptManager::getTemplate – lookup latency (hit and miss)
// ============================================================================

static void BM_PromptManager_GetTemplate_Hit(benchmark::State& state) {
    PromptManager mgr;
    auto inserted = mgr.createTemplate(MakeTemplate(0));
    const std::string id = inserted.id;
    for (auto _ : state) {
        auto r = mgr.getTemplate(id);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptManager_GetTemplate_Hit);

static void BM_PromptManager_GetTemplate_Miss(benchmark::State& state) {
    PromptManager mgr;
    for (auto _ : state) {
        auto r = mgr.getTemplate("nonexistent-id-xyz");
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptManager_GetTemplate_Miss);

// ============================================================================
// 3. PromptManager::injectContext – variable substitution throughput
// ============================================================================

static void BM_PromptManager_InjectContext(benchmark::State& state) {
    PromptManager mgr;
    const std::string tmpl =
        "You are a helpful assistant. User query: {query}. Context: {context}. "
        "Language: {lang}. Format: {format}. Extra: {extra}.";
    auto ctx = MakeCtx();
    for (auto _ : state) {
        auto r = mgr.injectContext(tmpl, ctx);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptManager_InjectContext);

// ============================================================================
// 4. PromptManager::validateTemplate – valid and invalid paths
// ============================================================================

static void BM_PromptManager_ValidateTemplate_Valid(benchmark::State& state) {
    auto t = MakeTemplate(0);
    for (auto _ : state) {
        auto r = PromptManager::validateTemplate(t);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptManager_ValidateTemplate_Valid);

static void BM_PromptManager_ValidateTemplate_Invalid(benchmark::State& state) {
    PromptManager::PromptTemplate t; // empty name/content → validation errors
    for (auto _ : state) {
        auto r = PromptManager::validateTemplate(t);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptManager_ValidateTemplate_Invalid);

// ============================================================================
// 5. PromptVersionControl::commit – in-memory commit throughput
// ============================================================================

static void BM_VersionControl_Commit(benchmark::State& state) {
    PromptVersionControl vc;
    int i = 0;
    for (auto _ : state) {
        auto vid = vc.commit(
            "prompt-bench",
            "template content revision " + std::to_string(i++),
            "bench commit",
            "bench-author"
        );
        benchmark::DoNotOptimize(vid);
    }
}
BENCHMARK(BM_VersionControl_Commit);

// ============================================================================
// 6. PromptVersionControl::getHistory – history retrieval (10 versions pre-loaded)
// ============================================================================

static void BM_VersionControl_GetHistory(benchmark::State& state) {
    PromptVersionControl vc;
    for (int i = 0; i < 10; ++i) {
        vc.commit("prompt-bench", "content v" + std::to_string(i), "commit " + std::to_string(i));
    }
    for (auto _ : state) {
        auto h = vc.getHistory("prompt-bench");
        benchmark::DoNotOptimize(h);
    }
}
BENCHMARK(BM_VersionControl_GetHistory);

// ============================================================================
// 7. FeedbackCollector::recordFeedback – recording throughput
// ============================================================================

static void BM_FeedbackCollector_RecordFeedback(benchmark::State& state) {
    FeedbackCollector fc;
    int i = 0;
    for (auto _ : state) {
        auto id = fc.recordFeedback(
            "prompt-bench",
            "query " + std::to_string(i),
            "response " + std::to_string(i),
            FeedbackType::USER_POSITIVE
        );
        benchmark::DoNotOptimize(id);
        ++i;
    }
}
BENCHMARK(BM_FeedbackCollector_RecordFeedback);

// ============================================================================
// 8. FeedbackCollector::getStats – aggregate stats latency (100 entries pre-loaded)
// ============================================================================

static void BM_FeedbackCollector_GetStats(benchmark::State& state) {
    FeedbackCollector fc;
    for (int i = 0; i < 100; ++i) {
        fc.recordFeedback(
            "prompt-bench",
            "q" + std::to_string(i),
            "r" + std::to_string(i),
            (i % 3 == 0) ? FeedbackType::USER_NEGATIVE : FeedbackType::USER_POSITIVE
        );
    }
    for (auto _ : state) {
        auto s = fc.getStats("prompt-bench");
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_FeedbackCollector_GetStats);

// ============================================================================
// 9. PromptOptimizer::optimize – 1-iteration loop latency
// ============================================================================

static void BM_PromptOptimizer_Optimize_OneIter(benchmark::State& state) {
    OptimizationConfig cfg;
    cfg.max_iterations   = 1;
    cfg.min_improvement  = 0.0;  // always iterate
    cfg.target_score     = 0.0;  // satisfied after 1 step
    PromptOptimizer opt(cfg);

    // Deterministic eval: returns 0.8 regardless of input.
    EvaluationFunction eval_fn = [](const std::string&,
                                    const std::vector<TestCase>&) -> double {
        return 0.8;
    };
    ImprovementFunction improve_fn = [](const std::string& p,
                                        double,
                                        const std::string&) -> std::string {
        return p + " [improved]";
    };

    const std::string init_prompt = "Answer the question: {query}";
    const std::vector<TestCase> cases = {
        {"What is ThemisDB?", "A hybrid database", {}},
        {"How does indexing work?", "Via B-tree or hash", {}},
    };

    for (auto _ : state) {
        auto r = opt.optimize(init_prompt, cases, eval_fn, improve_fn);
        benchmark::DoNotOptimize(r);
    }
}
BENCHMARK(BM_PromptOptimizer_Optimize_OneIter);

// ============================================================================
// 10. PromptEvaluator::evaluatePrompt – structural similarity latency
// ============================================================================

static void BM_PromptEvaluator_EvaluatePrompt(benchmark::State& state) {
    PromptEvaluator evaluator;
    const std::string output   = "ThemisDB is a hybrid database system supporting multiple models.";
    const std::string expected = "ThemisDB is a hybrid database supporting multiple storage engines.";
    for (auto _ : state) {
        auto m = evaluator.evaluateSingle(output, expected);
        benchmark::DoNotOptimize(m);
    }
}
BENCHMARK(BM_PromptEvaluator_EvaluatePrompt);

// ============================================================================
// 11. PromptEngineeringMetrics – recording throughput (per-call overhead)
// ============================================================================

static void BM_Metrics_RecordOptimizationAttempt(benchmark::State& state) {
    PromptEngineeringMetrics metrics;
    int i = 0;
    for (auto _ : state) {
        metrics.recordOptimizationAttempt("prompt-bench-" + std::to_string(i % 16));
        ++i;
    }
}
BENCHMARK(BM_Metrics_RecordOptimizationAttempt);

static void BM_Metrics_RecordPromptExecution(benchmark::State& state) {
    PromptEngineeringMetrics metrics;
    int i = 0;
    for (auto _ : state) {
        metrics.recordPromptExecution("prompt-bench-" + std::to_string(i % 16), true, 12.5);
        ++i;
    }
}
BENCHMARK(BM_Metrics_RecordPromptExecution);

// ============================================================================
// 12. PromptManager concurrent writes – 4-thread TBB concurrent_hash_map insert
// ============================================================================

static void BM_PromptManager_ConcurrentCreate(benchmark::State& state) {
    // shared across all 4 threads — exercises TBB concurrent_hash_map under contention.
    static PromptManager shared_mgr;
    static std::atomic<int> shared_counter{0};
    for (auto _ : state) {
        int idx     = shared_counter.fetch_add(1, std::memory_order_relaxed);
        auto t      = MakeTemplate(idx);
        auto result = shared_mgr.createTemplate(std::move(t));
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_PromptManager_ConcurrentCreate)->Threads(4);

// ============================================================================
// 13. PromptVersionControl concurrent commits – 4-thread commit throughput
// ============================================================================

static void BM_VersionControl_ConcurrentCommit(benchmark::State& state) {
    // shared VersionControl — each thread commits to its own prompt-id branch
    // so HEAD-update contention is per-thread while the internal map is shared.
    static PromptVersionControl shared_vc;
    static std::atomic<int> shared_counter{0};
    for (auto _ : state) {
        int idx = shared_counter.fetch_add(1, std::memory_order_relaxed);
        // Use a per-thread prompt id to avoid contention on the same branch HEAD.
        auto vid = shared_vc.commit(
            "prompt-thread-" + std::to_string(state.thread_index()),
            "content rev " + std::to_string(idx),
            "concurrent bench commit"
        );
        benchmark::DoNotOptimize(vid);
    }
}
BENCHMARK(BM_VersionControl_ConcurrentCommit)->Threads(4);
