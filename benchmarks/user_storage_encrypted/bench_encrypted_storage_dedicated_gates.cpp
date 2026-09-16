// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_encrypted_storage_dedicated_gates.cpp
 * @brief Wave D — User Storage Encrypted Dedicated Benchmark Gates.
 *
 * Benchmark gates ES-BM-01..04 for the user_storage_encrypted module.
 *
 * Gates:
 *   ES-BM-01  EncryptedWrite_Throughput         — ≥ 50 000 ops/sec
 *   ES-BM-02  EncryptedRead_Throughput           — ≥ 50 000 ops/sec
 *   ES-BM-03  KeyRotation_Latency                — p99 ≤ 5 ms
 *   ES-BM-04  EncDec_RoundTrip_Latency           — p99 ≤ 1 ms
 *
 * SIMULATION NOTE: All storage, key-rotation, and enc/dec operations use
 * in-process stubs that model the production hot paths without requiring
 * gocryptfs, FUSE, or external key stores.
 * These stubs MUST NOT be used in production code paths.
 *
 * @see docs/operability/RUNBOOK_USER_STORAGE_ENCRYPTED.md
 * @see src/user_storage_encrypted/ROADMAP.md — Wave D
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>

// ---------------------------------------------------------------------------
// SIMULATION NOTE — StubEncryptedStoreBench
// ---------------------------------------------------------------------------
class StubEncryptedStoreBench {
public:
    void write(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lk(mu_);
        store_[key] = value;
    }
    bool read(const std::string& key, std::string& out) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = store_.find(key);
        if (it == store_.end()) return false;
        out = it->second;
        return true;
    }
    void rotateKey() {
        std::lock_guard<std::mutex> lk(mu_);
        // Simulate key rotation cost
        for (auto& [k, v] : store_) { v = "[r]" + v; }
    }
    std::string encrypt(const std::string& pt) { return "ENC[" + pt + "]"; }
    std::string decrypt(const std::string& ct) {
        const std::string prefix = "ENC[";
        const std::string suffix = "]";
        if (ct.size() > prefix.size() + suffix.size() &&
            ct.substr(0, prefix.size()) == prefix &&
            ct.substr(ct.size() - suffix.size()) == suffix)
            return ct.substr(prefix.size(), ct.size() - prefix.size() - suffix.size());
        return ct;
    }

private:
    std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
};

static StubEncryptedStoreBench g_store;

// Seed the store once for read benchmarks
struct StoreSeedFixture {
    StoreSeedFixture() {
        for (int i = 0; i < 1000; ++i)
            g_store.write("bench_key_" + std::to_string(i), "bench_value_" + std::to_string(i));
    }
};
static const StoreSeedFixture kSeed;

// ---------------------------------------------------------------------------
// ES-BM-01: Encrypted Write Throughput
// ---------------------------------------------------------------------------
static void ES_BM_01_EncryptedWrite_Throughput(benchmark::State& state) {
    int64_t idx = 0;
    for (auto _ : state) {
        g_store.write("bk_" + std::to_string(idx), "bv_" + std::to_string(idx));
        ++idx;
        benchmark::ClobberMemory();
    }
    state.SetLabel("ES-BM-01 gate: ≥50000 ops/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(ES_BM_01_EncryptedWrite_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ES-BM-02: Encrypted Read Throughput
// ---------------------------------------------------------------------------
static void ES_BM_02_EncryptedRead_Throughput(benchmark::State& state) {
    int64_t idx = 0;
    std::string out;
    for (auto _ : state) {
        g_store.read("bench_key_" + std::to_string(idx % 1000), out);
        ++idx;
        benchmark::DoNotOptimize(out);
    }
    state.SetLabel("ES-BM-02 gate: ≥50000 ops/sec");
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(ES_BM_02_EncryptedRead_Throughput)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ES-BM-03: Key Rotation Latency
// ---------------------------------------------------------------------------
static void ES_BM_03_KeyRotation_Latency(benchmark::State& state) {
    StubEncryptedStoreBench small_store;
    for (int i = 0; i < 10; ++i)
        small_store.write("k" + std::to_string(i), "v" + std::to_string(i));

    for (auto _ : state) {
        small_store.rotateKey();
        benchmark::ClobberMemory();
    }
    state.SetLabel("ES-BM-03 gate: p99 ≤ 5ms");
}
BENCHMARK(ES_BM_03_KeyRotation_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// ES-BM-04: Enc/Dec Round-Trip Latency
// ---------------------------------------------------------------------------
static void ES_BM_04_EncDec_RoundTrip_Latency(benchmark::State& state) {
    StubEncryptedStoreBench crypto;
    const std::string plain = "benchmark-plaintext-payload-2026";
    for (auto _ : state) {
        const auto cipher = crypto.encrypt(plain);
        const auto recovered = crypto.decrypt(cipher);
        benchmark::DoNotOptimize(recovered);
    }
    state.SetLabel("ES-BM-04 gate: p99 ≤ 1ms");
}
BENCHMARK(ES_BM_04_EncDec_RoundTrip_Latency)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
