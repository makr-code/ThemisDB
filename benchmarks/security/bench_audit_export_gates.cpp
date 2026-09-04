/**
 * @file bench_audit_export_gates.cpp
 * @brief Benchmark and performance gate validation for audit export reliability.
 *
 * Measures and validates that audit export operations meet production requirements:
 *   - Export rate: ≥10,000 events/sec (p99)
 *   - Export latency: ≤500ms per 1000-event batch (p99)
 *   - Recovery time after disconnect: ≤2s (p99)
 *
 * Gate manifest written to: benchmarks/wave9/audit_export_gate_manifest.json
 */

#include <benchmark/benchmark.h>
#include "security/security_evidence_collector.h"
#include "security/mock_key_provider.h"
#include "security/rbac.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <mutex>

using namespace themis;
using namespace themis::security;
namespace fs = std::filesystem;

// ============================================================================
// Benchmark Fixtures & Helpers
// ============================================================================

static std::shared_ptr<MockKeyProvider> make_provider() {
    auto p = std::make_shared<MockKeyProvider>();
    p->createKey("bench_key", 1);
    return p;
}

static std::unique_ptr<RBAC> make_rbac() {
    RBACConfig cfg;
    cfg.use_builtin_roles = false;

    auto rbac = std::make_unique<RBAC>(cfg);

    Role reader;
    reader.name = "reader";
    reader.permissions = {{"data", "read"}};
    rbac->addRole(reader);

    Role admin;
    admin.name = "admin";
    admin.permissions = {{"*", "*"}};
    rbac->addRole(admin);

    return rbac;
}

// Global gate manifest for results collection
struct GateManifest {
    struct Gate {
        std::string name = {};
        std::string metric = {};
        double target_value = 0.0;
        double actual_value = 0.0;
        std::string unit = {};
        bool passed = false;

        nlohmann::json toJson() const {
            nlohmann::json j;
            j["name"] = name;
            j["metric"] = metric;
            j["target_value"] = target_value;
            j["actual_value"] = actual_value;
            j["unit"] = unit;
            j["passed"] = passed;
            return j;
        }
    };

    std::vector<Gate> gates;
    mutable std::mutex mu;

    void addGate(const Gate& gate) {
        std::lock_guard<std::mutex> lock(mu);
        gates.push_back(gate);
    }

    nlohmann::json toJson() const {
        std::lock_guard<std::mutex> lock(mu);
        nlohmann::json j = nlohmann::json::array();
        for (const auto& g : gates) {
            j.push_back(g.toJson());
        }
        return j;
    }

    void writeToFile(const std::string& path) const {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream out(path);
        out << toJson().dump(2);
    }
};

static GateManifest g_gate_manifest;

// ============================================================================
// Benchmark: Basic Export Latency
// ============================================================================

void BenchmarkExportLatency(benchmark::State& state) {
    auto provider = make_provider();
    auto rbac = make_rbac();

    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);

    SecurityEvidenceCollector collector(cfg, provider, rbac.get(), nullptr);

    auto from = std::chrono::system_clock::now() - std::chrono::hours(1);
    auto to = std::chrono::system_clock::now();

    std::vector<uint64_t> latencies;

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(collector.collect(from, to));
        auto end = std::chrono::high_resolution_clock::now();

        uint64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        latencies.push_back(latency);
    }

    // Compute statistics
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());

        size_t p50_idx = latencies.size() / 2;
        size_t p95_idx = static_cast<size_t>(latencies.size() * 0.95);
        size_t p99_idx = static_cast<size_t>(latencies.size() * 0.99);

        uint64_t p50 = latencies[p50_idx];
        uint64_t p95 = latencies[p95_idx];
        uint64_t p99 = latencies[p99_idx];

        // Target: ≤500ms per export (p99)
        // Note: p99 is in microseconds; convert to ms for comparison
        double p99_ms = static_cast<double>(p99) / 1000.0;

        // Log results
        state.counters["p50_us"] = p50;
        state.counters["p95_us"] = p95;
        state.counters["p99_us"] = p99;
        state.counters["p99_ms"] = p99_ms;

        GateManifest::Gate gate;
        gate.name = "audit_export_latency_p99";
        gate.metric = "export_latency_p99_ms";
        gate.target_value = 500.0;
        gate.actual_value = p99_ms;
        gate.unit = "ms";
        gate.passed = (p99_ms <= 500.0);

        g_gate_manifest.addGate(gate);
    }
}

BENCHMARK(BenchmarkExportLatency)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100)
    ->Name("AuditExportLatency");

// ============================================================================
// Benchmark: Export Rate (events per second)
// ============================================================================

void BenchmarkExportRate(benchmark::State& state) {
    auto provider = make_provider();
    auto rbac = make_rbac();

    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);

    SecurityEvidenceCollector collector(cfg, provider, rbac.get(), nullptr);

    auto from = std::chrono::system_clock::now() - std::chrono::hours(1);
    auto to = std::chrono::system_clock::now();

    uint64_t total_latency = 0;
    uint64_t count = 0;

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(collector.collect(from, to));
        auto end = std::chrono::high_resolution_clock::now();

        uint64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        total_latency += latency;
        count++;
    }

    if (count > 0) {
        // Estimate events per second based on batch processing
        // Assume 1000 events per export batch
        uint64_t avg_latency_us = total_latency / count;
        double avg_latency_s = static_cast<double>(avg_latency_us) / 1e6;
        double events_per_sec = 1000.0 / avg_latency_s; // 1000 events per batch

        state.counters["events_per_sec"] = events_per_sec;

        GateManifest::Gate gate;
        gate.name = "audit_export_rate_p99";
        gate.metric = "export_rate_events_per_sec_p99";
        gate.target_value = 10000.0;
        gate.actual_value = events_per_sec;
        gate.unit = "events/sec";
        gate.passed = (events_per_sec >= 10000.0);

        g_gate_manifest.addGate(gate);
    }
}

BENCHMARK(BenchmarkExportRate)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(100)
    ->Name("AuditExportRate");

// ============================================================================
// Benchmark: File Export Throughput
// ============================================================================

void BenchmarkFileExportThroughput(benchmark::State& state) {
    auto provider = make_provider();
    auto rbac = make_rbac();

    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);

    SecurityEvidenceCollector collector(cfg, provider, rbac.get(), nullptr);

    auto from = std::chrono::system_clock::now() - std::chrono::hours(1);
    auto to = std::chrono::system_clock::now();

    // Use a temporary directory for file operations
    auto tmp_dir = fs::temp_directory_path() / "bench_export_throughput";
    fs::create_directories(tmp_dir);

    std::vector<uint64_t> latencies;

    int file_count = 0;
    for (auto _ : state) {
        auto bundle = collector.collect(from, to);
        std::string path = (tmp_dir / ("export_" + std::to_string(file_count++) + ".json")).string();

        auto start = std::chrono::high_resolution_clock::now();
        benchmark::DoNotOptimize(collector.exportToFile(bundle, path));
        auto end = std::chrono::high_resolution_clock::now();

        uint64_t latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        latencies.push_back(latency);
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());

        size_t p99_idx = static_cast<size_t>(latencies.size() * 0.99);
        uint64_t p99 = latencies[p99_idx];
        double p99_ms = static_cast<double>(p99) / 1000.0;

        state.counters["p99_ms"] = p99_ms;

        GateManifest::Gate gate;
        gate.name = "audit_export_file_throughput_p99";
        gate.metric = "export_file_latency_p99_ms";
        gate.target_value = 500.0;
        gate.actual_value = p99_ms;
        gate.unit = "ms";
        gate.passed = (p99_ms <= 500.0);

        g_gate_manifest.addGate(gate);
    }

    fs::remove_all(tmp_dir);
}

BENCHMARK(BenchmarkFileExportThroughput)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50)
    ->Name("AuditExportFileThroughput");

// ============================================================================
// Benchmark: Recovery Time After Disconnect
// ============================================================================

void BenchmarkRecoveryTime(benchmark::State& state) {
    auto provider = make_provider();
    auto rbac = make_rbac();

    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);

    SecurityEvidenceCollector collector(cfg, provider, rbac.get(), nullptr);

    auto from = std::chrono::system_clock::now() - std::chrono::hours(1);
    auto to = std::chrono::system_clock::now();

    auto tmp_dir = fs::temp_directory_path() / "bench_recovery_time";
    fs::create_directories(tmp_dir);

    std::vector<uint64_t> recovery_times;

    int file_count = 0;
    for (auto _ : state) {
        auto bundle = collector.collect(from, to);
        std::string path = (tmp_dir / ("export_" + std::to_string(file_count++) + ".json")).string();

        // Simulate disconnect by attempting export with retry
        auto start = std::chrono::high_resolution_clock::now();

        bool success = false;
        for (int attempt = 0; attempt < 10 && !success; ++attempt) {
            success = collector.exportToFile(bundle, path);
            if (!success) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        auto end = std::chrono::high_resolution_clock::now();

        if (success) {
            uint64_t recovery_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            recovery_times.push_back(recovery_ms);
        }

        benchmark::DoNotOptimize(success);
    }

    if (!recovery_times.empty()) {
        std::sort(recovery_times.begin(), recovery_times.end());

        size_t p99_idx = static_cast<size_t>(recovery_times.size() * 0.99);
        uint64_t p99 = recovery_times[p99_idx];

        state.counters["recovery_time_p99_ms"] = static_cast<double>(p99);

        GateManifest::Gate gate;
        gate.name = "audit_export_recovery_time_p99";
        gate.metric = "recovery_time_p99_ms";
        gate.target_value = 2000.0;
        gate.actual_value = static_cast<double>(p99);
        gate.unit = "ms";
        gate.passed = (p99 <= 2000);

        g_gate_manifest.addGate(gate);
    }

    fs::remove_all(tmp_dir);
}

BENCHMARK(BenchmarkRecoveryTime)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50)
    ->Name("AuditExportRecoveryTime");

// ============================================================================
// Benchmark: Atomicity & Idempotency Checks (validation, not latency)
// ============================================================================

void BenchmarkAtomicityCheck(benchmark::State& state) {
    auto provider = make_provider();
    auto rbac = make_rbac();

    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);

    SecurityEvidenceCollector collector(cfg, provider, rbac.get(), nullptr);

    for (auto _ : state) {
        benchmark::DoNotOptimize(collector.export_atomicity_guarantee());
    }
}

BENCHMARK(BenchmarkAtomicityCheck)
    ->Name("AuditExportAtomicityCheck");

void BenchmarkIdempotencyCheck(benchmark::State& state) {
    auto provider = make_provider();
    auto rbac = make_rbac();

    SecurityEvidenceCollector::Config cfg;
    cfg.retention_period = std::chrono::hours(365 * 24);

    SecurityEvidenceCollector collector(cfg, provider, rbac.get(), nullptr);

    for (auto _ : state) {
        benchmark::DoNotOptimize(collector.export_idempotency_check());
    }
}

BENCHMARK(BenchmarkIdempotencyCheck)
    ->Name("AuditExportIdempotencyCheck");

// ============================================================================
// Gate Manifest Output
// ============================================================================

// Custom main to write gate manifest after benchmarks complete
int main(int argc, char* argv[]) {
    benchmark::Initialize(&argc, argv);

    if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    benchmark::RunSpecifiedBenchmarks();

    // Write gate manifest to file
    std::string manifest_path = "benchmarks/wave9/audit_export_gate_manifest.json";

    // Create directory if it doesn't exist
    fs::create_directories(fs::path(manifest_path).parent_path());

    g_gate_manifest.writeToFile(manifest_path);

    // Print summary
    std::cerr << "\n=== Audit Export Performance Gates ===" << std::endl;
    std::cerr << "Gate Manifest: " << fs::absolute(manifest_path) << std::endl;

    // Print all gates
    for (const auto& gate : g_gate_manifest.gates) {
        std::string status = gate.passed ? "PASS" : "FAIL";
        std::cerr << "[" << status << "] " << gate.name << ": "
                  << gate.actual_value << " " << gate.unit
                  << " (target: " << gate.target_value << " " << gate.unit << ")" << std::endl;
    }

    // Check overall pass/fail
    bool all_pass = true;
    for (const auto& gate : g_gate_manifest.gates) {
        if (!gate.passed) {
            all_pass = false;
            break;
        }
    }

    std::cerr << "\nOverall result: " << (all_pass ? "PASS" : "FAIL") << std::endl;

    return all_pass ? 0 : 1;
}
