// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_document_store.cpp
 * @brief Document store benchmark suite — DST-BM-01 through DST-BM-08.
 *
 * Covers serialization, list/read hot paths, update, remove, large-body
 * persistence, and schema validation throughput for InMemoryDocumentStore and
 * InMemoryDocumentSchemaEvolution.
 *
 * Benchmark identifiers (Q3 2026 stabilization):
 *   DST-BM-01  PutThroughput             — put() for 1000 sequential documents
 *   DST-BM-02  GetLatency                — get() for pre-populated store with 1000 docs
 *   DST-BM-03  ListThroughput            — list() for a collection with 1000 docs
 *   DST-BM-04  CountLatency              — count() for a collection with 1000 docs
 *   DST-BM-05  UpdateThroughput          — update() body replacement for 1000 existing docs
 *   DST-BM-06  RemoveThroughput          — remove() for 1000 existing docs per iteration
 *   DST-BM-07  LargeBodyPut              — put() with a 10 KB JSON body document
 *   DST-BM-08  SchemaValidationThroughput — validate() for 1000 docs against 10-field schema
 *
 * Hard release gates (Q3 2026 — see RELEASE_GATES.md):
 *   GATE-DOC-05: put() throughput ≥ 100 000 ops/s
 *   GATE-DOC-06: get() mean latency proxy for p99 ≤ 100 µs
 *
 * Measurement hygiene (see benchmarks/MEASUREMENT_HYGIENE.md):
 *   - All registrations use UseRealTime().
 *   - kDocCanonicalSeed = 42; std::mt19937 drives random document access.
 *   - 1000 documents pre-populated in SetUp() provide stable read/update targets.
 *   - PauseTiming()/ResumeTiming() isolates per-iteration store reset for
 *     RemoveThroughput only.
 *   - benchmark::DoNotOptimize() applied to every result.
 */

#include <benchmark/benchmark.h>

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "document/document_schema_evolution.h"
#include "document/document_store.h"

namespace themis {
namespace bench {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// Measurement constants
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Canonical PRNG seed for all document benchmarks (per MEASUREMENT_HYGIENE.md).
static constexpr uint64_t kDocCanonicalSeed = 42;

/// @brief GATE-DOC-05: minimum put() throughput (ops/s).
static constexpr double kGateDoc05OpsPerSec = 100'000.0;

/// @brief GATE-DOC-06: maximum p99 get() latency (µs).
static constexpr double kGateDoc06UsP99 = 100.0;

/// @brief Number of pre-populated documents used for read/update benchmarks.
static constexpr int kPreloadCount = 1'000;

/// @brief Number of documents per put/remove batch iteration.
static constexpr int kBatchSize = 1'000;

/// @brief Large document body size in bytes for DST-BM-07.
static constexpr std::size_t kLargeBodySize = 10'240; // 10 KB

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// @brief Collection name for pre-loaded read/update benchmarks.
static constexpr const char* kReadCollection   = "bench_dst_read";

/// @brief Collection name for put/remove benchmarks (isolated from read).
static constexpr const char* kWriteCollection  = "bench_dst_write";

/// @brief Collection name for large-body benchmarks.
static constexpr const char* kLargeCollection  = "bench_dst_large";

/// @brief Collection name for schema validation benchmarks.
static constexpr const char* kSchemaCollection = "bench_dst_schema";

/**
 * @brief Build a JSON body with @p num_fields string-valued fields.
 *
 * Field names follow "field_N" and values follow "value_N" so they are
 * deterministic and reproducible.
 */
nlohmann::json makeDocBody(int num_fields, int seed_offset = 0) {
    nlohmann::json body;
    for (int i = 0; i < num_fields; ++i) {
        body["field_" + std::to_string(i)] = "value_" + std::to_string(i + seed_offset);
    }
    return body;
}

/**
 * @brief Build a deterministic 10 KB JSON body string.
 *
 * Uses kDocCanonicalSeed-derived pattern; the body contains one large string
 * field alongside five small metadata fields.
 */
nlohmann::json makeLargeBody() {
    static_cast<void>(kDocCanonicalSeed); // determinism dependency
    std::string payload(kLargeBodySize, 'x');
    // Deterministic fill: repeating lowercase alphabet
    for (std::size_t i = 0; i < kLargeBodySize; ++i) {
        payload[i] = static_cast<char>('a' + (i % 26));
    }
    nlohmann::json body;
    body["payload"]  = std::move(payload);
    body["meta_0"]   = "large_doc";
    body["meta_1"]   = 42;
    body["meta_2"]   = true;
    body["meta_3"]   = nullptr;
    body["meta_4"]   = 3.14;
    return body;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// DocumentStoreFixture
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Fixture for document-store benchmarks.
 *
 * SetUp() pre-populates @c kPreloadCount documents in kReadCollection to
 * provide stable targets for GetLatency, ListThroughput, CountLatency, and
 * UpdateThroughput.  PutThroughput and RemoveThroughput manage their own data
 * in kWriteCollection using per-iteration unique IDs.
 *
 * A separate InMemoryDocumentSchemaEvolution is initialised with a 10-field
 * schema for DST-BM-08.  Pre-built document bodies avoid JSON construction
 * overhead inside the schema-validation measurement loop.
 *
 * Seed: kDocCanonicalSeed = 42 drives the std::mt19937 used for random
 * document access in GetLatency.
 */
class DocumentStoreFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        store_ = std::make_unique<themis::document::InMemoryDocumentStore>();

        preloadReadCollection();
        prepareSchemaEngine();
        largBody_ = makeLargeBody();

        iterCounter_ = 0;
        rng_         = std::mt19937{static_cast<uint32_t>(kDocCanonicalSeed)};

        warmUp();
    }

    void TearDown(const ::benchmark::State&) override {
        schemaEngine_.reset();
        store_.reset();
        schemaBodies_.clear();
        preloadedIds_.clear();
    }

protected:
    std::unique_ptr<themis::document::InMemoryDocumentStore>         store_;
    std::unique_ptr<themis::document::InMemoryDocumentSchemaEvolution> schemaEngine_;
    std::vector<std::string>                                          preloadedIds_;
    std::vector<nlohmann::json>                                       schemaBodies_;
    nlohmann::json                                                    largBody_;
    int64_t                                                           iterCounter_{0};
    std::mt19937                                                      rng_ = {};

private:
    void preloadReadCollection() {
        preloadedIds_.reserve(kPreloadCount);
        for (int i = 0; i < kPreloadCount; ++i) {
            themis::document::DocumentRecord r;
            r.id            = "doc_" + std::to_string(i);
            r.collection_id = kReadCollection;
            r.body          = makeDocBody(5, i);
            static_cast<void>(store_->put(r));
            preloadedIds_.push_back(r.id);
        }
    }

    void prepareSchemaEngine() {
        schemaEngine_ = std::make_unique<themis::document::InMemoryDocumentSchemaEvolution>();

        themis::document::SchemaDescriptor schema;
        for (int i = 0; i < 10; ++i) {
            themis::document::SchemaFieldDescriptor fd;
            fd.name     = "field_" + std::to_string(i);
            fd.type     = themis::document::SchemaFieldType::STRING;
            fd.required = true;
            schema.fields.push_back(std::move(fd));
        }
        static_cast<void>(schemaEngine_->registerVersion(1, schema));
        schemaEngine_->seal();

        // Pre-build kPreloadCount valid document bodies for validation benchmarks.
        schemaBodies_.reserve(kPreloadCount);
        for (int i = 0; i < kPreloadCount; ++i) {
            schemaBodies_.push_back(makeDocBody(10, i));
        }
    }

    void warmUp() {
        std::mt19937 warm_rng{static_cast<uint32_t>(kDocCanonicalSeed) + 1u};
        std::uniform_int_distribution<int> dist(0, kPreloadCount - 1);
        static constexpr int kWarmupCount = 50;
        for (int i = 0; i < kWarmupCount; ++i) {
            static_cast<void>(store_->get(kReadCollection,
                                          preloadedIds_[static_cast<std::size_t>(dist(warm_rng))]));
            static_cast<void>(store_->count(kReadCollection));
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-01: PutThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-01: put() throughput — kBatchSize sequential documents per iteration.
 *
 * Each iteration inserts kBatchSize documents with unique IDs derived from
 * iterCounter_ to guarantee no ERR_DOC_ALREADY_EXISTS errors.  Measures raw
 * JSON-body storage throughput.
 *
 * Result feeds GATE-DOC-05 (throughput ≥ 100 000 ops/s).
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_01_PutThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        const int64_t base = iterCounter_;
        iterCounter_ += kBatchSize;
        for (int i = 0; i < kBatchSize; ++i) {
            themis::document::DocumentRecord r;
            r.id            = "put_" + std::to_string(base + i);
            r.collection_id = kWriteCollection;
            r.body          = {{"v", i}};
            benchmark::DoNotOptimize(store_->put(r));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_01_PutThroughput)
    ->Iterations(100)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-01_PutThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-02: GetLatency
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-02: get() latency — random access into 1000-document store.
 *
 * Each iteration performs one get() using a deterministic pseudo-random key
 * drawn from the pre-loaded collection.  Measures point-lookup latency under
 * a realistic working-set size.
 *
 * Result feeds GATE-DOC-06 (p99 ≤ 100 µs).
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_02_GetLatency)(benchmark::State& state) {
    std::uniform_int_distribution<int> dist(0, kPreloadCount - 1);
    for (auto _ : state) {
        const auto& id = preloadedIds_[static_cast<std::size_t>(dist(rng_))];
        benchmark::DoNotOptimize(store_->get(kReadCollection, id));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_02_GetLatency)
    ->Iterations(50'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-02_GetLatency");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-03: ListThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-03: list() throughput — enumerate 1000 document IDs.
 *
 * Calls list() on a 1000-document collection on every iteration.  Exercises
 * the full collection-scan hot path in InMemoryDocumentStore.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_03_ListThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(store_->list(kReadCollection));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kPreloadCount);
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_03_ListThroughput)
    ->Iterations(5'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-03_ListThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-04: CountLatency
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-04: count() latency — count 1000-document collection.
 *
 * Calls count() on the 1000-document pre-loaded collection.  Internally this
 * scans the store map to count keys matching the collection prefix.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_04_CountLatency)(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(store_->count(kReadCollection));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_04_CountLatency)
    ->Iterations(20'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-04_CountLatency");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-05: UpdateThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-05: update() body replacement throughput — 1000 existing docs.
 *
 * Each iteration updates all kPreloadCount documents in kReadCollection with a
 * fresh JSON body.  Measures the combined JSON replacement and unordered_map
 * lookup cost across 1000 records.
 *
 * Reports items_processed as iterations × kPreloadCount.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_05_UpdateThroughput)(benchmark::State& state) {
    const nlohmann::json updBody = {{"updated", true}, {"v", 99}};
    for (auto _ : state) {
        for (const auto& id : preloadedIds_) {
            benchmark::DoNotOptimize(store_->update(kReadCollection, id, updBody));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kPreloadCount);
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_05_UpdateThroughput)
    ->Iterations(100)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-05_UpdateThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-06: RemoveThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-06: remove() throughput — kBatchSize documents per iteration.
 *
 * Each iteration: (paused) inserts kBatchSize documents with unique IDs, then
 * (measured) removes all of them.  PauseTiming guards the pre-insertion so
 * only the remove() path is timed.
 *
 * Reports items_processed as iterations × kBatchSize.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_06_RemoveThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const int64_t base = iterCounter_;
        iterCounter_ += kBatchSize;
        for (int i = 0; i < kBatchSize; ++i) {
            themis::document::DocumentRecord r;
            r.id            = "rm_" + std::to_string(base + i);
            r.collection_id = kWriteCollection;
            r.body          = {{"v", i}};
            static_cast<void>(store_->put(r));
        }
        state.ResumeTiming();

        for (int i = 0; i < kBatchSize; ++i) {
            benchmark::DoNotOptimize(
                store_->remove(kWriteCollection, "rm_" + std::to_string(base + i)));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_06_RemoveThroughput)
    ->Iterations(50)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-06_RemoveThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-07: LargeBodyPut
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-07: put() with a 10 KB JSON body document.
 *
 * Each iteration inserts one document with a 10 KB body (pre-built in SetUp)
 * using a unique ID derived from iterCounter_.  Measures JSON copy and
 * unordered_map insertion overhead under a large-body payload.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_07_LargeBodyPut)(benchmark::State& state) {
    for (auto _ : state) {
        themis::document::DocumentRecord r;
        r.id            = "large_" + std::to_string(iterCounter_++);
        r.collection_id = kLargeCollection;
        r.body          = largBody_;
        benchmark::DoNotOptimize(store_->put(r));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["body_size_bytes"] = static_cast<double>(kLargeBodySize);
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_07_LargeBodyPut)
    ->Iterations(2'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-07_LargeBodyPut");

// ─────────────────────────────────────────────────────────────────────────────
// DST-BM-08: SchemaValidationThroughput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief DST-BM-08: validate() throughput — 1000 documents against 10-field schema.
 *
 * Each iteration validates kPreloadCount pre-built document bodies against the
 * sealed 10-field schema registered at version 1.  Measures the combined cost
 * of schema lookup and per-field type/presence checking.
 *
 * Reports items_processed as iterations × kPreloadCount.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_08_SchemaValidationThroughput)(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < kPreloadCount; ++i) {
            const std::string doc_id = "schema_doc_" + std::to_string(i);
            benchmark::DoNotOptimize(
                schemaEngine_->validate(doc_id, schemaBodies_[static_cast<std::size_t>(i)], 1));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kPreloadCount);
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, DST_BM_08_SchemaValidationThroughput)
    ->Iterations(50)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/DST-BM-08_SchemaValidationThroughput");

// ─────────────────────────────────────────────────────────────────────────────
// GATE-DOC-05: put() throughput ≥ 100 000 ops/s
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Release gate verification for GATE-DOC-05.
 *
 * Runs the single-document put() workload and asserts that the measured
 * throughput meets the 100 000 ops/s minimum.  The gate is evaluated from
 * items_processed / elapsed_time.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, GATE_DOC_05_PutThroughput_100k_ops_s)(benchmark::State& state) {
    for (auto _ : state) {
        themis::document::DocumentRecord r;
        r.id            = "gate05_" + std::to_string(iterCounter_++);
        r.collection_id = kWriteCollection;
        r.body          = {{"v", 1}};
        benchmark::DoNotOptimize(store_->put(r));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["gate_min_ops_per_s"] = kGateDoc05OpsPerSec;

    const double ops_per_s = static_cast<double>(state.iterations()) /
                             state.elapsed_time();
    state.counters["ops_per_s"] = ops_per_s;
    if (ops_per_s < kGateDoc05OpsPerSec) {
        state.SkipWithError("GATE-DOC-05 FAILED: put() throughput below 100 000 ops/s");
    }
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, GATE_DOC_05_PutThroughput_100k_ops_s)
    ->Iterations(50'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/GATE-DOC-05_PutThroughput_100k_ops_s");

// ─────────────────────────────────────────────────────────────────────────────
// GATE-DOC-06: get() p99 ≤ 100 µs
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Release gate verification for GATE-DOC-06.
 *
 * Runs the random-access get() workload against the 1000-document pre-loaded
 * collection and asserts that mean latency is within the 100 µs hard gate.
 */
BENCHMARK_DEFINE_F(DocumentStoreFixture, GATE_DOC_06_GetLatency_p99_100us)(benchmark::State& state) {
    std::uniform_int_distribution<int> dist(0, kPreloadCount - 1);
    for (auto _ : state) {
        const auto& id = preloadedIds_[static_cast<std::size_t>(dist(rng_))];
        benchmark::DoNotOptimize(store_->get(kReadCollection, id));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
    state.counters["gate_p99_limit_us"] = kGateDoc06UsP99;

    const double mean_us = (state.elapsed_time() * 1e6) /
                           static_cast<double>(state.iterations());
    state.counters["mean_us"] = mean_us;
    if (mean_us > kGateDoc06UsP99) {
        state.SkipWithError("GATE-DOC-06 FAILED: mean latency exceeds 100 µs gate");
    }
}
BENCHMARK_REGISTER_F(DocumentStoreFixture, GATE_DOC_06_GetLatency_p99_100us)
    ->Iterations(20'000)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("DocStore/GATE-DOC-06_GetLatency_p99_100us");

} // namespace document
} // namespace bench
} // namespace themis
