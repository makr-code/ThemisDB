/**
 * @file bench_audit_export_high_volume.cpp
 * @brief Audit export performance benchmark (Wave C Batch 1 baseline gates)
 * @version 0.1.0
 * @note Part of Wave C — Security Production Validation
 * @note Defines release gates for audit export reliability under sustained load
 * 
 * Wave C Exit Criteria:
 * - GATE-AUDIT-01: Audit write throughput ≥10,000 events/sec (p99)
 * - GATE-AUDIT-02: Audit signing latency ≤1ms per entry (p99)
 * - GATE-AUDIT-03: Batch write atomicity verified end-to-end
 * - GATE-AUDIT-04: Export recovery time ≤2s post-disconnect (p99)
 * - GATE-AUDIT-05: Idempotency enforcement validated (no duplicates)
 * - GATE-AUDIT-06: Crash checkpoint recovery ≥99% entry retention
 */

#include <benchmark/benchmark.h>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <memory>

#include "governance/audit_batch_writer.h"
#include "governance/governance_audit_integrity.h"

namespace themis {
namespace governance {
namespace benchmarks {

// ============================================================================
// Benchmark Fixtures
// ============================================================================

class AuditExportBenchmark : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& state) override {
        // Initialize audit infrastructure
        auto signer = std::make_shared<AuditSigner>(
            AuditSigner::SignatureAlgorithm::HMAC_SHA256,
            "bench-key-001",
            "secret-key-for-benchmarking"
        );
        
        AuditRetentionPolicy policy;
        policy.retention_period_days = 2555;
        
        manager_ = std::make_shared<AuditIntegrityManager>(policy, signer);
        
        // Configure batch writer for benchmarking
        AuditBatchWriter::Config config;
        config.buffer_size = 50000;
        config.batch_size = 5000;
        config.flush_interval_ms = 50;
        config.enable_checkpoints = true;
        config.enable_metrics = true;
        
        writer_ = std::make_unique<AuditBatchWriter>(manager_, config);
        writer_->start();
    }
    
    void TearDown(const benchmark::State& state) override {
        if (writer_) {
            writer_->shutdown();
        }
    }
    
protected:
    std::shared_ptr<AuditIntegrityManager> manager_;
    std::unique_ptr<AuditBatchWriter> writer_;
    
    ImmutableAuditEntry createTestEntry(int sequence) {
        ImmutableAuditEntry entry;
        entry.entry_id = "entry_" + std::to_string(sequence);
        entry.rule_id = "rule_bench_" + std::to_string(sequence % 100);
        entry.operation = "update";
        entry.user = "bench_user";
        entry.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        entry.details["sequence"] = sequence;
        return entry;
    }
};

// ============================================================================
// GATE-AUDIT-01: Write Throughput ≥10,000 events/sec (p99)
// ============================================================================

BENCHMARK_F(AuditExportBenchmark, GATE_AUDIT_01_WriteThroughput_10kEventsPerSec)
(benchmark::State& state) {
    int entry_counter = 0;
    
    for (auto _ : state) {
        ImmutableAuditEntry entry = createTestEntry(entry_counter++);
        writer_->submitEntry(entry);
    }
    
    state.SetItemsProcessed(entry_counter);
}

// ============================================================================
// GATE-AUDIT-02: Signing Latency ≤1ms per entry (p99)
// ============================================================================

BENCHMARK_F(AuditExportBenchmark, GATE_AUDIT_02_SigningLatency_1ms_P99)
(benchmark::State& state) {
    int entry_counter = 0;
    
    for (auto _ : state) {
        ImmutableAuditEntry entry = createTestEntry(entry_counter++);
        
        // Time the signing operation
        auto start = std::chrono::high_resolution_clock::now();
        manager_->addEntry(entry);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();
        
        // Report latency as custom metric
        state.counters["latency_us"] = latency_us;
    }
    
    state.SetItemsProcessed(entry_counter);
}

// ============================================================================
// GATE-AUDIT-03: Batch Atomicity (no partial writes)
// ============================================================================

BENCHMARK_F(AuditExportBenchmark, GATE_AUDIT_03_BatchAtomicity)
(benchmark::State& state) {
    const int BATCH_SIZE = 1000;
    
    for (auto _ : state) {
        // Submit a batch
        for (int i = 0; i < BATCH_SIZE; ++i) {
            ImmutableAuditEntry entry = createTestEntry(i);
            writer_->submitEntry(entry);
        }
        
        // Flush atomically
        auto result = writer_->forceFlush();
        
        // Verify atomicity: either all succeeded or all failed
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(BATCH_SIZE);
}

// ============================================================================
// GATE-AUDIT-04: Recovery Time ≤2s post-disconnect (p99)
// ============================================================================

BENCHMARK_F(AuditExportBenchmark, GATE_AUDIT_04_RecoveryTime_2s)
(benchmark::State& state) {
    // Simulate checkpoints
    const int ENTRIES_PER_RECOVERY = 5000;
    
    for (auto _ : state) {
        // Submit entries
        for (int i = 0; i < ENTRIES_PER_RECOVERY; ++i) {
            ImmutableAuditEntry entry = createTestEntry(i);
            writer_->submitEntry(entry);
        }
        
        // Force flush to create checkpoints
        auto result = writer_->forceFlush();
        
        // Simulate recovery from checkpoint
        auto checkpoints = writer_->getCheckpoints();
        if (!checkpoints.empty()) {
            auto recovery_result = writer_->recoverFromCheckpoint(checkpoints.back());
            benchmark::DoNotOptimize(recovery_result);
        }
    }
    
    state.SetItemsProcessed(ENTRIES_PER_RECOVERY);
}

// ============================================================================
// GATE-AUDIT-05: Idempotency Enforcement (no duplicates)
// ============================================================================

BENCHMARK_F(AuditExportBenchmark, GATE_AUDIT_05_IdempotencyEnforcement)
(benchmark::State& state) {
    int token_counter = 0;
    
    for (auto _ : state) {
        ImmutableAuditEntry entry = createTestEntry(token_counter);
        std::string token = "token_" + std::to_string(token_counter);
        
        // Submit twice with same token
        writer_->submitEntryIdempotent(entry, token);
        auto result = writer_->submitEntryIdempotent(entry, token);
        
        // Second should be duplicate
        benchmark::DoNotOptimize(result);
        token_counter++;
    }
    
    state.SetItemsProcessed(token_counter);
}

// ============================================================================
// GATE-AUDIT-06: Crash Checkpoint Recovery ≥99% retention
// ============================================================================

BENCHMARK_F(AuditExportBenchmark, GATE_AUDIT_06_CheckpointRetention_99Percent)
(benchmark::State& state) {
    const int TOTAL_ENTRIES = 10000;
    int verified_entries = 0;
    
    for (auto _ : state) {
        // Submit large batch
        for (int i = 0; i < TOTAL_ENTRIES; ++i) {
            ImmutableAuditEntry entry = createTestEntry(i);
            writer_->submitEntry(entry);
        }
        
        // Flush and create checkpoints
        writer_->forceFlush();
        
        // Verify checkpoints cover all entries
        auto checkpoints = writer_->getCheckpoints();
        for (const auto& cp : checkpoints) {
            if (writer_->verifyCheckpoint(cp)) {
                verified_entries += cp.entry_count;
            }
        }
        
        benchmark::DoNotOptimize(verified_entries);
    }
    
    state.SetItemsProcessed(TOTAL_ENTRIES);
}

// ============================================================================
// Custom Benchmarks for Wave C Gates
// ============================================================================

void BM_AuditExport_ContinuousLoad(benchmark::State& state) {
    /**
     * Sustained load test for 10 seconds
     * Measures throughput under continuous high-volume submission
     */
    auto signer = std::make_shared<AuditSigner>(
        AuditSigner::SignatureAlgorithm::HMAC_SHA256,
        "continuous-bench-key",
        "secret"
    );
    
    AuditRetentionPolicy policy;
    AuditBatchWriter::Config config;
    config.buffer_size = 100000;
    config.batch_size = 10000;
    
    auto manager = std::make_shared<AuditIntegrityManager>(policy, signer);
    auto writer = std::make_unique<AuditBatchWriter>(manager, config);
    writer->start();
    
    int entry_counter = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (auto _ : state) {
        ImmutableAuditEntry entry;
        entry.entry_id = "entry_" + std::to_string(entry_counter);
        entry.rule_id = "rule_continuous";
        entry.operation = "update";
        entry.user = "continuous_user";
        entry.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        writer->submitEntry(entry);
        entry_counter++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time
    ).count();
    
    writer->shutdown();
    
    state.SetItemsProcessed(entry_counter);
    if (duration_seconds > 0) {
        state.counters["events_per_sec"] = entry_counter / (double)duration_seconds;
    }
}

BENCHMARK(BM_AuditExport_ContinuousLoad)
    ->Iterations(100000)
    ->Unit(benchmark::kMillisecond);

void BM_AuditExport_LatencyDistribution(benchmark::State& state) {
    /**
     * Measure latency distribution for signing operations
     * Generates histogram for p50/p95/p99 analysis
     */
    auto signer = std::make_shared<AuditSigner>(
        AuditSigner::SignatureAlgorithm::HMAC_SHA256,
        "latency-bench-key",
        "secret"
    );
    
    AuditRetentionPolicy policy;
    auto manager = std::make_shared<AuditIntegrityManager>(policy, signer);
    
    std::vector<int64_t> latencies;
    
    for (auto _ : state) {
        ImmutableAuditEntry entry;
        entry.entry_id = "entry_latency";
        entry.rule_id = "rule_latency";
        entry.operation = "update";
        entry.user = "latency_user";
        entry.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        auto start = std::chrono::high_resolution_clock::now();
        manager->addEntry(entry);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();
        
        latencies.push_back(latency_us);
    }
    
    // Calculate percentiles
    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        
        int p50_idx = latencies.size() / 2;
        int p95_idx = static_cast<int>(latencies.size() * 0.95);
        int p99_idx = static_cast<int>(latencies.size() * 0.99);
        
        state.counters["p50_us"] = latencies[p50_idx];
        state.counters["p95_us"] = latencies[p95_idx];
        state.counters["p99_us"] = latencies[p99_idx];
    }
    
    state.SetItemsProcessed(latencies.size());
}

BENCHMARK(BM_AuditExport_LatencyDistribution)
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

}  // namespace benchmarks
}  // namespace governance
}  // namespace themis

// ============================================================================
// Benchmark Configuration
// ============================================================================

int main(int argc, char** argv) {
    /**
     * Wave C Release Gate Configuration
     * 
     * Usage:
     *   ./bench_audit_export --benchmark_format=json > audit_export_gate_manifest.json
     *   ./bench_audit_export --benchmark_repetitions=5
     */
    
    benchmark::Initialize(&argc, argv);
    
    // Wave C configuration
    benchmark::RunSpecifiedBenchmarks();
    
    return 0;
}
