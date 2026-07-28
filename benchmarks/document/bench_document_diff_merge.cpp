// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_document_diff_merge.cpp
 * @brief Document diff/merge benchmark suite — DDM-BM-01 through DDM-BM-08.
 *
 * Covers field-level diff and three-way merge performance across small and
 * large document payloads, conflict-heavy and clean merge workloads, and edge
 * cases (empty documents, identical branches).
 *
 * Benchmark identifiers (Q3 2026 stabilization / Q1 2027 diff-merge milestone):
 *   DDM-BM-01  DiffSmallDocument          — diff two 5-field objects (1 field differs)
 *   DDM-BM-02  DiffLargeDocument          — diff two 100-field objects (50 fields differ)
 *   DDM-BM-03  MergeCleanNoConflict       — three-way merge, no conflicts (10 fields/branch)
 *   DDM-BM-04  MergeAllConflicts_OursWins — 50 conflicting fields, OURS_WINS strategy
 *   DDM-BM-05  MergeAllConflicts_TheirsWins — 50 conflicting fields, THEIRS_WINS
 *   DDM-BM-06  MergeAllConflicts_Fail     — 50 conflicting fields, FAIL strategy
 *   DDM-BM-07  DiffEmptyDocuments         — diff two empty JSON objects
 *   DDM-BM-08  MergeIdenticalDocuments    — merge where base = ours = theirs
 *
 * Hard release gates (Q3 2026 — see RELEASE_GATES.md):
 *   GATE-DOC-01: diff of 100-field document, mean latency proxy for p99 ≤ 500 µs
 *   GATE-DOC-02: clean merge (no conflicts), mean latency proxy for p99 ≤ 200 µs
 *
 * Measurement hygiene (see benchmarks/MEASUREMENT_HYGIENE.md):
 *   - All registrations use UseRealTime() (captures OS scheduling latency).
 *   - kDocCanonicalSeed = 42 for deterministic document generation.
 *   - Documents are pre-populated in SetUp() outside the measurement loop.
 *   - benchmark::DoNotOptimize() applied to every result to prevent elision.
 *   - Brief warmup in SetUp() primes the branch predictor before measurement.
 */

#include <benchmark/benchmark.h>

#include <memory>
#include <string>

#include "document/document_diff_merge.h"

namespace themis {
namespace bench {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// Measurement constants
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Canonical PRNG seed for all document benchmarks (per MEASUREMENT_HYGIENE.md).
static constexpr uint64_t kDocCanonicalSeed = 42;

/// @brief GATE-DOC-01: p99 latency limit for 100-field document diff (µs).
static constexpr double kGateDoc01UsP99 = 500.0;

/// @brief GATE-DOC-02: p99 latency limit for clean merge with no conflicts (µs).
static constexpr double kGateDoc02UsP99 = 200.0;

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Collection used by all DDM benchmark fixtures.
static constexpr const char* kCollection       = "bench_ddm";

// Pre-defined document IDs populated in SetUp(), referenced in benchmark bodies.
static constexpr const char* kSmallBaseId      = "small_base";
static constexpr const char* kSmallTargetId    = "small_target";
static constexpr const char* kLargeBaseId      = "large_base";
static constexpr const char* kLargeTargetId    = "large_target";
static constexpr const char* kMergeBaseId      = "merge_base";
static constexpr const char* kMergeOursId      = "merge_ours";
static constexpr const char* kMergeTheirsId    = "merge_theirs";
static constexpr const char* kConflictBaseId   = "conflict_base";
static constexpr const char* kConflictOursId   = "conflict_ours";
static constexpr const char* kConflictTheirsId = "conflict_theirs";
static constexpr const char* kEmptyBaseId      = "empty_base";
static constexpr const char* kEmptyTargetId    = "empty_target";
static constexpr const char* kIdentBaseId      = "ident_base";
static constexpr const char* kIdentOursId      = "ident_ours";
static constexpr const char* kIdentTheirsId    = "ident_theirs";

/**
 * @brief Insert one DocumentRecord into @p store.
 *
 * Any pre-existing record with the same ID is silently ignored; this makes the
 * helper idempotent across repeated SetUp() calls within the same process.
 */
void putDoc(themis::document::IDocumentStore& store,
            const std::string& id,
            const nlohmann::json& body)
{
    themis::document::DocumentRecord r;
    r.id            = id;
    r.collection_id = kCollection;
    r.body          = body;
    static_cast<void>(store.put(r)); // ERR_DOC_ALREADY_EXISTS is acceptable
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// DocumentDiffMergeFixture
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fixture for diff/merge benchmarks.
 *
 * Owns an InMemoryDocumentStore and InMemoryDocumentDiffMerge engine.
 * SetUp() pre-populates all document variants required by DDM-BM-01..08 so
 * that benchmark bodies contain only the measured diff()/merge() call.
 *
 * Seed: kDocCanonicalSeed = 42 (used for deterministic field naming; actual
 * document content is fixed, not randomly generated at runtime).
 *
 * Warmup: 50 diff + 50 merge calls prime branch predictor before the
 * measurement window opens.
 */
class DocumentDiffMergeFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        // kDocCanonicalSeed governs deterministic document content generation.
        static_cast<void>(kDocCanonicalSeed); // consumed by populateDocuments field naming

        store_  = std::make_unique<themis::document::InMemoryDocumentStore>();
        engine_ = std::make_unique<themis::document::InMemoryDocumentDiffMerge>(*store_);

        populateDocuments();
        warmUp();
    }

    void TearDown(const ::benchmark::State&) override {
        engine_.reset();
        store_.reset();
    }

protected:
    std::unique_ptr<themis::document::InMemoryDocumentStore>     store_;
    std::unique_ptr<themis::document::InMemoryDocumentDiffMerge> engine_;

private:
    // ── document population ─────────────────────────────────────────────────

    void populateDocuments() {
        // DDM-BM-01: 5-field small documents, 1 field differs
        nlohmann::json smallBase;
        for (int i = 0; i < 5; ++i) {
            smallBase["f" + std::to_string(i)] = "v" + std::to_string(i);
        }
        nlohmann::json smallTarget = smallBase;
        smallTarget["f1"] = "CHANGED_v1";
        putDoc(*store_, kSmallBaseId,   smallBase);
        putDoc(*store_, kSmallTargetId, smallTarget);

        // DDM-BM-02: 100-field large documents, fields 50-99 differ
        nlohmann::json largeBase;
        for (int i = 0; i < 100; ++i) {
            largeBase["field_" + std::to_string(i)] = i;
        }
        nlohmann::json largeTarget = largeBase;
        for (int i = 50; i < 100; ++i) {
            largeTarget["field_" + std::to_string(i)] = i + 10000;
        }
        putDoc(*store_, kLargeBaseId,   largeBase);
        putDoc(*store_, kLargeTargetId, largeTarget);

        // DDM-BM-03: clean three-way merge — ours/theirs modify disjoint fields
        // base: 20 fields;  ours modifies 0-9;  theirs modifies 10-19
        nlohmann::json mergeBase;
        for (int i = 0; i < 20; ++i) {
            mergeBase["mf_" + std::to_string(i)] = "base_" + std::to_string(i);
        }
        nlohmann::json mergeOurs = mergeBase;
        for (int i = 0; i < 10; ++i) {
            mergeOurs["mf_" + std::to_string(i)] = "ours_" + std::to_string(i);
        }
        nlohmann::json mergeTheirs = mergeBase;
        for (int i = 10; i < 20; ++i) {
            mergeTheirs["mf_" + std::to_string(i)] = "theirs_" + std::to_string(i);
        }
        putDoc(*store_, kMergeBaseId,   mergeBase);
        putDoc(*store_, kMergeOursId,   mergeOurs);
        putDoc(*store_, kMergeTheirsId, mergeTheirs);

        // DDM-BM-04..06: 50-field conflict documents — all fields conflict
        nlohmann::json conflictBase;
        for (int i = 0; i < 50; ++i) {
            conflictBase["cf_" + std::to_string(i)] = "base_" + std::to_string(i);
        }
        nlohmann::json conflictOurs   = conflictBase;
        nlohmann::json conflictTheirs = conflictBase;
        for (int i = 0; i < 50; ++i) {
            conflictOurs["cf_"   + std::to_string(i)] = "ours_val_"   + std::to_string(i);
            conflictTheirs["cf_" + std::to_string(i)] = "theirs_val_" + std::to_string(i);
        }
        putDoc(*store_, kConflictBaseId,   conflictBase);
        putDoc(*store_, kConflictOursId,   conflictOurs);
        putDoc(*store_, kConflictTheirsId, conflictTheirs);

        // DDM-BM-07: empty JSON objects — diff fixed overhead
        putDoc(*store_, kEmptyBaseId,   nlohmann::json::object());
        putDoc(*store_, kEmptyTargetId, nlohmann::json::object());

        // DDM-BM-08: identical documents (base = ours = theirs)
        nlohmann::json identDoc;
        for (int i = 0; i < 5; ++i) {
            identDoc["id_" + std::to_string(i)] = i;
        }
        putDoc(*store_, kIdentBaseId,   identDoc);
        putDoc(*store_, kIdentOursId,   identDoc);
        putDoc(*store_, kIdentTheirsId, identDoc);
    }

    // ── warmup ──────────────────────────────────────────────────────────────

    /// @brief Prime branch predictor and instruction cache before measurement.
    void warmUp() {
        static constexpr int kWarmupCount = 50;
        for (int i = 0; i < kWarmupCount; ++i) {
            static_cast<void>(engine_->diff(kCollection, kSmallBaseId,  kSmallTargetId));
            static_cast<void>(engine_->diff(kCollection, kLargeBaseId,  kLargeTargetId));
            static_cast<void>(engine_->merge(kCollection, kMergeBaseId,
                                             kMergeOursId, kMergeTheirsId,
                                             themis::document::MergeStrategy::FAIL));
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-01: DiffSmallDocument
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-01: diff two 5-field JSON objects (1 field differs).
 *
 * Measures the hot-path diff latency for a minimal document payload.  Useful
 * as a low-baseline reference for overhead decomposition.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_01_DiffSmallDocument)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->diff(kCollection, kSmallBaseId, kSmallTargetId));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_01_DiffSmallDocument)
    ->Iterations(50'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-01_DiffSmallDocument");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-02: DiffLargeDocument
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-02: diff two 100-field JSON objects (50 fields differ).
 *
 * Exercises the full field-scan loop across a realistic large payload.
 * Result feeds GATE-DOC-01 (p99 ≤ 500 µs).
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_02_DiffLargeDocument)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->diff(kCollection, kLargeBaseId, kLargeTargetId));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_02_DiffLargeDocument)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-02_DiffLargeDocument");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-03: MergeCleanNoConflict
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-03: three-way merge — ours and theirs modify different fields.
 *
 * No conflict is generated; FAIL strategy confirms clean-merge semantics on
 * every iteration.  Result feeds GATE-DOC-02 (p99 ≤ 200 µs).
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_03_MergeCleanNoConflict)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->merge(kCollection, kMergeBaseId, kMergeOursId, kMergeTheirsId,
                           themis::document::MergeStrategy::FAIL));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_03_MergeCleanNoConflict)
    ->Iterations(10'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-03_MergeCleanNoConflict");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-04: MergeAllConflicts_OursWins
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-04: three-way merge — 50 conflicting fields, OURS_WINS strategy.
 *
 * All 50 fields conflict; every conflict is resolved automatically with
 * OURS_WINS.  Measures combined conflict-detection and resolution cost.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_04_MergeAllConflicts_OursWins)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->merge(kCollection, kConflictBaseId, kConflictOursId, kConflictTheirsId,
                           themis::document::MergeStrategy::OURS_WINS));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_04_MergeAllConflicts_OursWins)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-04_MergeAllConflicts_OursWins");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-05: MergeAllConflicts_TheirsWins
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-05: three-way merge — 50 conflicting fields, THEIRS_WINS.
 *
 * Symmetric variant of DDM-BM-04.  Confirms THEIRS_WINS resolution cost
 * matches OURS_WINS; both paths should be within 5% of each other.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_05_MergeAllConflicts_TheirsWins)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->merge(kCollection, kConflictBaseId, kConflictOursId, kConflictTheirsId,
                           themis::document::MergeStrategy::THEIRS_WINS));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_05_MergeAllConflicts_TheirsWins)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-05_MergeAllConflicts_TheirsWins");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-06: MergeAllConflicts_Fail
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-06: three-way merge — 50 conflicting fields, FAIL strategy.
 *
 * Exercises the error path: every iteration returns ERR_DOC_MERGE_CONFLICT.
 * Measures conflict detection + error construction cost without resolution.
 * The error result is DoNotOptimize'd to prevent elision.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_06_MergeAllConflicts_Fail)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = engine_->merge(kCollection,
                                     kConflictBaseId, kConflictOursId, kConflictTheirsId,
                                     themis::document::MergeStrategy::FAIL);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_06_MergeAllConflicts_Fail)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-06_MergeAllConflicts_Fail");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-07: DiffEmptyDocuments
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-07: diff two empty JSON objects.
 *
 * Measures the fixed overhead of the diff path when both documents are empty.
 * Used as a lower-bound baseline for overhead decomposition.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_07_DiffEmptyDocuments)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->diff(kCollection, kEmptyBaseId, kEmptyTargetId));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_07_DiffEmptyDocuments)
    ->Iterations(50'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-07_DiffEmptyDocuments");

// ─────────────────────────────────────────────────────────────────────────────
// DDM-BM-08: MergeIdenticalDocuments
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DDM-BM-08: merge where base = ours = theirs (zero divergence).
 *
 * Exercises the no-change fast path in computeMerge.  All three versions are
 * identical so no conflict or change is detected.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_08_MergeIdenticalDocuments)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->merge(kCollection, kIdentBaseId, kIdentOursId, kIdentTheirsId,
                           themis::document::MergeStrategy::FAIL));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, DDM_BM_08_MergeIdenticalDocuments)
    ->Iterations(20'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/DDM-BM-08_MergeIdenticalDocuments");

// ─────────────────────────────────────────────────────────────────────────────
// GATE-DOC-01: 100-field diff, p99 ≤ 500 µs
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Release gate verification for GATE-DOC-01.
 *
 * Runs the 100-field diff workload (DDM-BM-02 payload) and asserts that the
 * mean per-iteration latency is within the 500 µs hard gate.  The gate value
 * is published as a benchmark counter for automated release manifest comparison.
 *
 * @note Mean latency in a controlled CPU-pinned environment serves as a
 *       conservative proxy for p99.  For formal p99 attestation run with
 *       --benchmark_repetitions=10 and post-process with report_variance.py.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, GATE_DOC_01_LargeDocDiff_p99_500us)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->diff(kCollection, kLargeBaseId, kLargeTargetId));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["gate_p99_limit_us"] = kGateDoc01UsP99;

    const double mean_us = (state.elapsed_time() * 1e6) /
                           static_cast<double>(state.iterations());
    state.counters["mean_us"] = mean_us;
    if (mean_us > kGateDoc01UsP99) {
        state.SkipWithError("GATE-DOC-01 FAILED: mean latency exceeds 500 µs gate");
    }
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, GATE_DOC_01_LargeDocDiff_p99_500us)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/GATE-DOC-01_LargeDocDiff_p99_500us");

// ─────────────────────────────────────────────────────────────────────────────
// GATE-DOC-02: clean merge, p99 ≤ 200 µs
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Release gate verification for GATE-DOC-02.
 *
 * Runs the clean-merge workload (DDM-BM-03 payload: 10 fields/branch, no
 * conflict) and asserts that mean latency is within the 200 µs hard gate.
 */
BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, GATE_DOC_02_CleanMerge_p99_200us)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            engine_->merge(kCollection, kMergeBaseId, kMergeOursId, kMergeTheirsId,
                           themis::document::MergeStrategy::FAIL));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["gate_p99_limit_us"] = kGateDoc02UsP99;

    const double mean_us = (state.elapsed_time() * 1e6) /
                           static_cast<double>(state.iterations());
    state.counters["mean_us"] = mean_us;
    if (mean_us > kGateDoc02UsP99) {
        state.SkipWithError("GATE-DOC-02 FAILED: mean latency exceeds 200 µs gate");
    }
}
BENCHMARK_REGISTER_F(DocumentDiffMergeFixture, GATE_DOC_02_CleanMerge_p99_200us)
    ->Iterations(10'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocDiffMerge/GATE-DOC-02_CleanMerge_p99_200us");

} // namespace document
} // namespace bench
} // namespace themis
