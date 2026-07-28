/**
 * @file bench_ldm.cpp
 * @brief LDM performance benchmarks for the ethics_ai module.
 *
 * All benchmarks use deterministic stub-LLM fixtures (no real LLM required).
 * Canonical RNG seed: kCanonicalRngSeed = 42 (MEASUREMENT_HYGIENE.md §1).
 * UseRealTime() applied to benchmarks with async/parallel I/O-bound paths
 * (MEASUREMENT_HYGIENE.md §4).
 *
 * Hard gates define the release acceptance criteria for LDM Ebene-1/3.
 */

// ============================================================================
// Hard Gates: ethics_ai LDM Release Gates (GATE-EAL-01..06)
// GATE-EAL-01: BM_LDM_PlanGeneration < 5 ms
// GATE-EAL-02: BM_LDM_Ebene1_22Schools P95 ≤ 200 ms
// GATE-EAL-03: BM_LDM_Ebene1_TimeoutFailsafe ≤ 10 ms overhead
// GATE-EAL-04: BM_LDM_MetaVerdictAssembly ≤ 50 ms
// GATE-EAL-05: BM_LDM_MirrorSchool_Parallel4 ≤ 200 ms total
// GATE-EAL-06: BM_LDM_EndToEnd_LAYERED_FAST P95 ≤ 1.2 s
// ============================================================================

#include <benchmark/benchmark.h>

#include "ethics_ai/ethics_ai_types.h"
#include "ethics_ai/ethics_selection_router.h"
#include "ethics_ai/ethics_profile_registry.h"
#include "ethics_ai/discourse_orchestrator.h"
#include "ethics_ai/meta_verdict_builder.h"
#include "ethics_ai/mirror_school_handler.h"

#include <algorithm>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace themis::plugins::ethics;

// ============================================================================
// Benchmark canonical seed (MEASUREMENT_HYGIENE.md §1)
// ============================================================================

static constexpr uint64_t kCanonicalRngSeed = 42;

// ============================================================================
// Shared fixtures
// ============================================================================

namespace {

/// Minimal mock registry for benchmark use (no YAML I/O).
class BenchEthicsRegistry : public IEthicsProfileRegistry {
public:
    void addSchool(const std::string& school_id,
                   const std::string& taxonomy_class) {
        EthicsProfileMeta m;
        m.school_id      = school_id;
        m.taxonomy_class = taxonomy_class;
        m.name           = school_id;
        schools_.push_back(std::move(m));
    }

    std::vector<EthicsProfileMeta> queryIndex(
        const EthicsIndexQuery& /*q*/) const override { return schools_; }

    std::variant<PhilosophyProfile, Status> getProfile(
        const std::string& sid) override {
        PhilosophyProfile p; p.school_id = sid; return p;
    }

    std::variant<size_t, Status> rebuildIndex(
        const std::string& /*dir*/) override { return schools_.size(); }

    size_t indexSize() const override { return schools_.size(); }

    bool hasProfile(const std::string& sid) const override {
        for (const auto& m : schools_) {
            if (m.school_id == sid) return true;
        }
        return false;
    }

private:
    std::vector<EthicsProfileMeta> schools_;
};

/// Build the canonical 22-school registry used by all LDM benchmarks.
std::unique_ptr<BenchEthicsRegistry> makeBenchRegistry() {
    auto reg = std::make_unique<BenchEthicsRegistry>();

    // Cluster A — Deontological (4)
    reg->addSchool("kant",            "deontological");
    reg->addSchool("contractualism",  "deontological");
    reg->addSchool("rawls",           "deontological");
    reg->addSchool("rationalism",     "deontological");
    // Cluster B — Consequentialist (2)
    reg->addSchool("utilitarianism",  "consequentialist");
    reg->addSchool("adam_smith",      "consequentialist");
    // Cluster C — Virtue (2)
    reg->addSchool("socratic",        "virtue");
    reg->addSchool("konfuzianismus",  "virtue");
    // Cluster D — Cultural-Religious (3)
    reg->addSchool("islamische_ethik",    "cultural_religious");
    reg->addSchool("juedische_bioethik",  "cultural_religious");
    reg->addSchool("buddhistische_ethik", "cultural_religious");
    // Cluster E — Non-Mainstream (6)
    reg->addSchool("nietzsche",       "non_mainstream");
    reg->addSchool("marx",            "non_mainstream");
    reg->addSchool("schopenhauer",    "non_mainstream");
    reg->addSchool("dilthey",         "non_mainstream");
    reg->addSchool("arendt",          "non_mainstream");
    reg->addSchool("durkheim",        "non_mainstream");
    // Cluster F — Institutional (5)
    reg->addSchool("behoerden_ethik",     "institutional");
    reg->addSchool("universitaere_ethik", "institutional");
    reg->addSchool("wiener",              "institutional");
    reg->addSchool("merton",              "institutional");
    reg->addSchool("leopold",             "institutional");

    return reg;
}

static const std::string kDilemmaText =
    "An autonomous vehicle must choose between endangering its passenger or "
    "a group of pedestrians. Analyse the ethical dimensions under deontological, "
    "consequentialist, virtue, and cultural-religious frameworks.";

} // anonymous namespace

// ============================================================================
// BM_LDM_PlanGeneration — GATE-EAL-01: < 5 ms
// Plan generation for N=22 schools in LAYERED_FULL mode.
// CPU-bound (no I/O) → default CPU time.
// ============================================================================

static void BM_LDM_PlanGeneration(benchmark::State& state) {
    auto reg = makeBenchRegistry();
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FULL;
    EthicsSelectionRouter router(reg.get(), cfg);

    for (auto _ : state) {
        auto plan = router.planDiscourse();
        benchmark::DoNotOptimize(plan);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_LDM_PlanGeneration)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(1000);

// ============================================================================
// BM_LDM_Ebene1_22Schools — GATE-EAL-02: P95 ≤ 200 ms
// Parallel equal-weight Ebene-1 scoring with stub LLM, N=22.
// I/O-bound (async futures) → UseRealTime().
// ============================================================================

static void BM_LDM_Ebene1_22Schools(benchmark::State& state) {
    auto reg = makeBenchRegistry();
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FAST;
    EthicsSelectionRouter router(reg.get(), cfg);
    const auto plan = router.planDiscourse();

    DiscourseOrchestrator orch(reg.get(), cfg);
    // Default stub LLM — no injection needed.
    MirrorSchoolPolicy no_mirror;

    for (auto _ : state) {
        auto results = orch.runEbene1(plan, kDilemmaText, no_mirror);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(plan.ebene1_school_ids.size()));
}

BENCHMARK(BM_LDM_Ebene1_22Schools)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(50);

// ============================================================================
// BM_LDM_Ebene1_TimeoutFailsafe — GATE-EAL-03: ≤ 10 ms overhead
// Measures the overhead of the timeout-check path when LLM calls complete
// well within the timeout (stub returns immediately).
// I/O-bound (future wait_for) → UseRealTime().
// ============================================================================

static void BM_LDM_Ebene1_TimeoutFailsafe(benchmark::State& state) {
    auto reg = makeBenchRegistry();
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FAST;
    EthicsSelectionRouter router(reg.get(), cfg);
    const auto plan = router.planDiscourse();

    DiscourseOrchestrator orch(reg.get(), cfg);
    // Generous timeout (1 s) — stub completes in microseconds.
    // This measures the overhead of the timeout-check bookkeeping only.
    orch.setSchoolTimeoutMs(1000);
    MirrorSchoolPolicy no_mirror;

    for (auto _ : state) {
        auto results = orch.runEbene1(plan, kDilemmaText, no_mirror);
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(plan.ebene1_school_ids.size()));
}

BENCHMARK(BM_LDM_Ebene1_TimeoutFailsafe)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(50);

// ============================================================================
// BM_LDM_MetaVerdictAssembly — GATE-EAL-04: ≤ 50 ms
// MetaVerdictBuilder::buildMetaVerdict with N=22 Ebene-1 results.
// CPU-bound → default CPU time.
// ============================================================================

static void BM_LDM_MetaVerdictAssembly(benchmark::State& state) {
    // Pre-build N=22 Ebene-1 results deterministically (kCanonicalRngSeed).
    std::mt19937 rng{kCanonicalRngSeed};

    auto reg = makeBenchRegistry();
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FAST;
    EthicsSelectionRouter router(reg.get(), cfg);
    const auto plan = router.planDiscourse();

    std::vector<DiscourseRoundOutput> results;
    const std::vector<DiscourseVerdict> verdicts = {
        DiscourseVerdict::PROHIBIT, DiscourseVerdict::PERMIT, DiscourseVerdict::CONDITIONAL
    };
    std::uniform_int_distribution<int> dist(0, 2);

    for (const auto& sid : plan.ebene1_school_ids) {
        DiscourseRoundOutput out;
        out.school_id      = sid;
        out.ldm_verdict    = verdicts[static_cast<size_t>(dist(rng))];
        out.verdict        = out.ldm_verdict == DiscourseVerdict::PROHIBIT ? "PROHIBIT"
                           : out.ldm_verdict == DiscourseVerdict::PERMIT   ? "PERMIT"
                                                                           : "CONDITIONAL";
        out.timed_out      = false;
        out.initial_weight = plan.initial_weight;
        results.push_back(std::move(out));
    }

    LegalGrounding grounding;
    grounding.grounding_available = true;
    grounding.citation_ids        = {"GG-Art1", "DSGVO-Art5"};
    grounding.norm_refs           = {"GG Art. 1", "DSGVO Art. 5"};

    MetaVerdictBuilder builder;

    for (auto _ : state) {
        auto mv = builder.buildMetaVerdict(
            results, {}, grounding, DiscourseMode::LAYERED_FAST, {});
        benchmark::DoNotOptimize(mv);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_LDM_MetaVerdictAssembly)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(5000);

// ============================================================================
// BM_LDM_MirrorSchool_Parallel4 — GATE-EAL-05: ≤ 200 ms total
// 4 mirror schools running in parallel with stub LLM.
// I/O-bound (async futures) → UseRealTime().
// ============================================================================

static void BM_LDM_MirrorSchool_Parallel4(benchmark::State& state) {
    MirrorSchoolHandler handler;
    // Default stub LLM.
    const std::vector<std::string> mirrors = {
        "islamische_ethik", "konfuzianismus",
        "buddhistische_ethik", "juedische_bioethik"
    };

    for (auto _ : state) {
        auto results = handler.runMirror(mirrors, kDilemmaText, "bioethics");
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(
        static_cast<int64_t>(state.iterations()) *
        static_cast<int64_t>(mirrors.size()));
}

BENCHMARK(BM_LDM_MirrorSchool_Parallel4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(50);

// ============================================================================
// BM_LDM_EndToEnd_LAYERED_FAST — GATE-EAL-06: P95 ≤ 1.2 s
// Ebene-1 (N=22) + Ebene-3 MetaVerdict assembly in LAYERED_FAST mode.
// I/O-bound (async Ebene-1) → UseRealTime().
// ============================================================================

static void BM_LDM_EndToEnd_LAYERED_FAST(benchmark::State& state) {
    auto reg = makeBenchRegistry();
    RouterConfig cfg;
    cfg.discourse_mode = DiscourseMode::LAYERED_FAST;
    EthicsSelectionRouter router(reg.get(), cfg);
    const auto plan = router.planDiscourse();

    DiscourseOrchestrator orch(reg.get(), cfg);
    MirrorSchoolPolicy no_mirror;

    MetaVerdictBuilder builder;
    LegalGrounding grounding;
    grounding.grounding_available = true;

    for (auto _ : state) {
        // Ebene-1
        auto ebene1 = orch.runEbene1(plan, kDilemmaText, no_mirror);
        // Ebene-3 (MetaVerdict assembly)
        auto mv = builder.buildMetaVerdict(
            ebene1, {}, grounding, DiscourseMode::LAYERED_FAST, {});
        benchmark::DoNotOptimize(mv);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_LDM_EndToEnd_LAYERED_FAST)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Iterations(20);

// ============================================================================

BENCHMARK_MAIN();
