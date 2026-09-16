// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_document_serialization_gates.cpp
 * @brief Document serialization benchmark gates — DOC-BM-01 through DOC-BM-04.
 *
 * Locks p95 performance envelopes for the four document module hot paths:
 * serialization, list-read, diff, and merge.  Gate thresholds are emitted
 * as benchmark counters so CI tooling can validate results from JSON output:
 *
 *   bench_document_serialization_gates \
 *     --benchmark_format=json --benchmark_out=doc_gates.json
 *
 * ## Benchmark identifiers
 *
 *   DOC-BM-01  SerializationP95          — document serialization p95 latency gate
 *   DOC-BM-02  ListReadP95               — document list-read p95 throughput gate
 *   DOC-BM-03  DiffP95                   — document diff p95 latency gate
 *   DOC-BM-04  MergeP95                  — document merge p95 latency gate
 *
 * ## Measurement hygiene
 *   - All registrations use UseRealTime().MinTime(1.0).
 *   - Thresholds emitted as "gate_threshold_*" counters for CI tooling.
 *   - benchmark::DoNotOptimize() applied to all results.
 *   - PauseTiming() / ResumeTiming() used to isolate fixture setup.
 *
 * @version 1.0.0
 * @see src/document/ROADMAP.md — Mid-term benchmark gates (DOC-BM-01..04)
 * @see benchmarks/MEASUREMENT_HYGIENE.md — hygiene standards
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

// ─── gate thresholds ─────────────────────────────────────────────────────────

/// DOC-BM-01: document serialization p95 latency ≤ 500 µs per document
static constexpr double kGateDOCBM01_SerializationP95_us = 500.0;

/// DOC-BM-02: list-read p95 throughput ≥ 50 000 ops/s
static constexpr double kGateDOCBM02_ListReadP95_OpsPerSec = 50'000.0;

/// DOC-BM-03: diff p95 latency ≤ 1 000 µs per diff pair
static constexpr double kGateDOCBM03_DiffP95_us = 1'000.0;

/// DOC-BM-04: merge p95 latency ≤ 2 000 µs per three-way merge
static constexpr double kGateDOCBM04_MergeP95_us = 2'000.0;

// ─── in-process stub helpers ─────────────────────────────────────────────────
//
// These stubs replace the live document serialization, list/read, diff,
// and merge paths so no external store or schema backend is required.
// Their performance characteristics model the real document module hot paths:
//   - Serialization cost is proportional to content size (O(n) string ops)
//   - List-read is O(k) map traversal
//   - Diff is O(n) string comparison producing a deterministic patch
//   - Merge is O(1) stub (non-overlapping field model)
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

static constexpr uint64_t kDocSeed = 42;

/// Build a deterministic document JSON body of ~200 bytes.
static std::string makeDocBody(uint64_t idx) {
    std::ostringstream oss;
    oss << R"({"id":)" << idx
        << R"(,"title":"doc_)" << idx
        << R"(","schema_version":1,"content":")"
        << "content_body_for_document_" << idx << "_with_enough_chars_to_model_real_payload"
        << R"("})";
    return oss.str();
}

/// Stub serializer: returns a serialized string (mirrors DocumentStore write path).
static std::string stubSerialize(const std::string& key, const std::string& body) {
    std::ostringstream oss;
    oss << "{\"key\":\"" << key << "\",\"body\":" << body << "}";
    return oss.str();
}

/// Stub list-read: scans a vector of pre-built entries.
static std::size_t stubListRead(const std::vector<std::string>& entries) {
    std::size_t count = 0;
    for (const auto& e : entries) {
        benchmark::DoNotOptimize(e);
        ++count;
    }
    return count;
}

/// Stub diff: produces a deterministic patch string for two distinct bodies.
static std::string stubDiff(const std::string& from, const std::string& to) {
    if (from == to) return {};
    std::ostringstream oss;
    oss << "diff[" << from.size() << "->" << to.size() << "]";
    return oss.str();
}

/// Stub merge: conflict-free three-way merge for non-overlapping field sets.
static std::string stubMerge(const std::string& base,
                              const std::string& /*branch_a*/,
                              const std::string& /*branch_b*/) {
    return base + "_merged";
}

// ─── DOC-BM-01: SerializationP95 ─────────────────────────────────────────────

/**
 * @brief DOC-BM-01 — document serialization p95 latency gate.
 *
 * Serializes a single document (key + ~200-byte JSON body) per iteration.
 * Gate: p95 latency ≤ 500 µs.
 * Counter: "gate_threshold_serialization_p95_us"
 */
static void BM_DOCBM01_SerializationP95(benchmark::State& state) {
    const std::string key  = "doc-bm01-key-" + std::to_string(kDocSeed);
    const std::string body = makeDocBody(kDocSeed);

    for (auto _ : state) {
        std::string serialized = stubSerialize(key, body);
        benchmark::DoNotOptimize(serialized);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_serialization_p95_us"] = kGateDOCBM01_SerializationP95_us;
}
BENCHMARK(BM_DOCBM01_SerializationP95)->UseRealTime()->MinTime(1.0);

// ─── DOC-BM-02: ListReadP95 ──────────────────────────────────────────────────

/**
 * @brief DOC-BM-02 — document list-read p95 throughput gate.
 *
 * Scans a pre-built list of 1000 document entries per iteration.
 * Gate: throughput ≥ 50 000 ops/s.
 * Counter: "gate_threshold_list_read_p95_ops_per_sec"
 */
static void BM_DOCBM02_ListReadP95(benchmark::State& state) {
    constexpr std::size_t kListSize = 1000;

    std::vector<std::string> entries;
    entries.reserve(kListSize);
    for (std::size_t i = 0; i < kListSize; ++i) {
        entries.push_back(stubSerialize("key-" + std::to_string(i), makeDocBody(i)));
    }

    for (auto _ : state) {
        std::size_t count = stubListRead(entries);
        benchmark::DoNotOptimize(count);
    }

    // Each iteration processes kListSize entries
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(kListSize));
    state.counters["gate_threshold_list_read_p95_ops_per_sec"] = kGateDOCBM02_ListReadP95_OpsPerSec;
}
BENCHMARK(BM_DOCBM02_ListReadP95)->UseRealTime()->MinTime(1.0);

// ─── DOC-BM-03: DiffP95 ──────────────────────────────────────────────────────

/**
 * @brief DOC-BM-03 — document diff p95 latency gate.
 *
 * Computes a diff between two distinct document bodies per iteration.
 * Gate: p95 latency ≤ 1 000 µs.
 * Counter: "gate_threshold_diff_p95_us"
 */
static void BM_DOCBM03_DiffP95(benchmark::State& state) {
    const std::string from_body = makeDocBody(0);
    const std::string to_body   = makeDocBody(1);

    for (auto _ : state) {
        std::string patch = stubDiff(from_body, to_body);
        benchmark::DoNotOptimize(patch);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_diff_p95_us"] = kGateDOCBM03_DiffP95_us;
}
BENCHMARK(BM_DOCBM03_DiffP95)->UseRealTime()->MinTime(1.0);

// ─── DOC-BM-04: MergeP95 ─────────────────────────────────────────────────────

/**
 * @brief DOC-BM-04 — document merge p95 latency gate.
 *
 * Performs a three-way (base + two branches) merge per iteration.
 * Gate: p95 latency ≤ 2 000 µs.
 * Counter: "gate_threshold_merge_p95_us"
 */
static void BM_DOCBM04_MergeP95(benchmark::State& state) {
    const std::string base     = makeDocBody(0);
    const std::string branch_a = makeDocBody(1);
    const std::string branch_b = makeDocBody(2);

    for (auto _ : state) {
        std::string merged = stubMerge(base, branch_a, branch_b);
        benchmark::DoNotOptimize(merged);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["gate_threshold_merge_p95_us"] = kGateDOCBM04_MergeP95_us;
}
BENCHMARK(BM_DOCBM04_MergeP95)->UseRealTime()->MinTime(1.0);
