/**
 * @file bench_metrics_collector.cpp
 * @brief Performance benchmarks for MetricsCollector (Observability)
 * 
 * Benchmarks metrics recording, aggregation, Prometheus export,
 * and concurrent access performance.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <benchmark/benchmark.h>
#include "observability/metrics_collector.h"
#include <random>
#include <string>
#include <vector>

using namespace themis::observability;

// ============================================================================
// Setup & Utilities
// ============================================================================

static void ResetCollector(benchmark::State& state) {
    if (state.thread_index() == 0) {
        MetricsCollector::getInstance().reset();
    }
}

std::vector<std::string> generateMetricNames(int count) {
    std::vector<std::string> names = {};

    for (int i = 0; i < count; i++) {
        names.push_back("metric_" + std::to_string(i));
    }
    return names;
}

// ============================================================================
// Basic Recording Benchmarks
// ============================================================================

static void BM_RecordQuery(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    for (auto _ : state) {
        collector.recordQuery("select", 10.5, 100);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RecordQuery);

static void BM_RecordCacheHit(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    for (auto _ : state) {
        collector.recordCacheHit("query_cache");
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RecordCacheHit);

static void BM_RecordTSStoreWrite(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    for (auto _ : state) {
        collector.recordTSStoreWrite("cpu.usage", 100, 5.5);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RecordTSStoreWrite);

static void BM_RecordShardLatency(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    for (auto _ : state) {
        collector.recordShardLatency("shard-001", 8.3);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_RecordShardLatency);

// ============================================================================
// Multiple Metric Types Benchmarks
// ============================================================================

static void BM_MixedMetrics(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    int counter = 0;
    for (auto _ : state) {
        switch (counter % 5) {
            case 0:
                collector.recordQuery("select", 10.0, 100);
                break;
            case 1:
                collector.recordCacheHit("cache");
                break;
            case 2:
                collector.recordTSStoreWrite("metric", 50, 5.0);
                break;
            case 3:
                collector.recordShardLatency("shard-001", 8.0);
                break;
            case 4:
                collector.recordEncryptionOperation("encrypt", 3.5);
                break;
        }
        counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MixedMetrics);

// ============================================================================
// Volume Benchmarks
// ============================================================================

static void BM_HighVolumeRecording(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int batch_size = state.range(0);
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; i++) {
            collector.recordQuery("select", i * 0.1, i);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_HighVolumeRecording)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

static void BM_ManyUniqueMetrics(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int num_unique = state.range(0);
    auto metric_names = generateMetricNames(num_unique);
    
    int idx = 0;
    for (auto _ : state) {
        collector.recordQuery(metric_names[idx % num_unique], 10.0, 100);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ManyUniqueMetrics)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

// ============================================================================
// Prometheus Export Benchmarks
// ============================================================================

static void BM_PrometheusExport_Empty(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    for (auto _ : state) {
        std::string metrics = collector.getPrometheusMetrics();
        benchmark::DoNotOptimize(metrics);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PrometheusExport_Empty);

static void BM_PrometheusExport_WithData(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    // Pre-populate with some metrics
    for (int i = 0; i < 100; i++) {
        collector.recordQuery("select", i * 0.5, i * 10);
        collector.recordCacheHit("cache");
        collector.recordTSStoreWrite("metric_" + std::to_string(i), 50, 5.0);
    }
    
    for (auto _ : state) {
        std::string metrics = collector.getPrometheusMetrics();
        benchmark::DoNotOptimize(metrics);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PrometheusExport_WithData);

static void BM_PrometheusExport_LargeDataset(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int num_metrics = state.range(0);
    
    // Pre-populate with many metrics
    for (int i = 0; i < num_metrics; i++) {
        collector.recordQuery("query_" + std::to_string(i % 10), i * 0.1, i);
        collector.recordCacheHit("cache_" + std::to_string(i % 5));
        collector.recordShardLatency("shard-" + std::to_string(i % 3), i * 0.5);
    }
    
    for (auto _ : state) {
        std::string metrics = collector.getPrometheusMetrics();
        benchmark::DoNotOptimize(metrics);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * collector.getPrometheusMetrics().size());
}
BENCHMARK(BM_PrometheusExport_LargeDataset)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============================================================================
// Concurrent Access Benchmarks
// ============================================================================

static void BM_ConcurrentRecording(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    for (auto _ : state) {
        collector.recordQuery("select", 10.0, 100);
        collector.recordCacheHit("cache");
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentRecording)->ThreadRange(1, 16);

static void BM_ConcurrentMixedOperations(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    int counter = 0;
    for (auto _ : state) {
        if (counter % 10 == 0) {
            // Occasional read
            std::string metrics = collector.getPrometheusMetrics();
            benchmark::DoNotOptimize(metrics);
        } else {
            // Mostly writes
            collector.recordQuery("select", 10.0, 100);
        }
        counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentMixedOperations)->ThreadRange(1, 16);

static void BM_ConcurrentExport(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    // Pre-populate with data
    if (state.thread_index() == 0) {
        for (int i = 0; i < 1000; i++) {
            collector.recordQuery("select", i * 0.1, i);
        }
    }
    
    for (auto _ : state) {
        std::string metrics = collector.getPrometheusMetrics();
        benchmark::DoNotOptimize(metrics);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentExport)->ThreadRange(1, 8);

// ============================================================================
// Specific Subsystem Benchmarks
// ============================================================================

static void BM_TSStoreMetricsBatch(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int batch_size = state.range(0);
    std::vector<std::string> metrics = {};

    for (int i = 0; i < 10; i++) {
        metrics.push_back("sensor_" + std::to_string(i));
    }
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; i++) {
            collector.recordTSStoreWrite(metrics[i % metrics.size()], 100, 5.0);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_TSStoreMetricsBatch)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

static void BM_ShardingMetricsBatch(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int num_shards = state.range(0);
    std::vector<std::string> shard_ids = {};

    for (int i = 0; i < num_shards; i++) {
        shard_ids.push_back("shard-" + std::to_string(i));
    }
    
    int idx = 0;
    for (auto _ : state) {
        collector.recordShardRequest(shard_ids[idx % num_shards], "read");
        collector.recordShardLatency(shard_ids[idx % num_shards], 8.0);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_ShardingMetricsBatch)
    ->Arg(4)
    ->Arg(16)
    ->Arg(64);

static void BM_CacheMetricsBatch(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int operations = state.range(0);
    
    for (auto _ : state) {
        for (int i = 0; i < operations; i++) {
            if (i % 5 == 0) {
                collector.recordCacheMiss("query_cache");
            } else {
                collector.recordCacheHit("query_cache");
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * operations);
}
BENCHMARK(BM_CacheMetricsBatch)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

static void BM_SecurityMetricsBatch(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int operations = state.range(0);
    
    for (auto _ : state) {
        for (int i = 0; i < operations; i++) {
            collector.recordAuthAttempt(i % 10 != 0); // 90% success
            collector.recordEncryptionOperation("encrypt", 5.0);
            collector.recordPolicyEvaluation(true, 2.0);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * operations * 3);
}
BENCHMARK(BM_SecurityMetricsBatch)
    ->Arg(10)
    ->Arg(100)
    ->Arg(1000);

// ============================================================================
// Latency Distribution Benchmarks
// ============================================================================

static void BM_HistogramRecording(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(50.0, 10.0); // mean=50ms, stddev=10ms
    
    for (auto _ : state) {
        double latency = dist(gen);
        collector.recordQuery("select", latency, 100);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HistogramRecording);

static void BM_MultipleHistograms(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dist(50.0, 10.0);
    
    std::vector<std::string> query_types = {"select", "insert", "update", "delete"};
    
    int idx = 0;
    for (auto _ : state) {
        double latency = dist(gen);
        collector.recordQuery(query_types[idx % query_types.size()], latency, 100);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MultipleHistograms);

// ============================================================================
// Memory Overhead Benchmarks
// ============================================================================

static void BM_MemoryFootprint(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    const int num_metrics = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        collector.reset();
        state.ResumeTiming();
        
        for (int i = 0; i < num_metrics; i++) {
            collector.recordQuery("query_" + std::to_string(i), i * 0.1, i);
        }
        
        std::string metrics = collector.getPrometheusMetrics();
        benchmark::DoNotOptimize(metrics);
    }
    
    state.SetItemsProcessed(state.iterations() * num_metrics);
}
BENCHMARK(BM_MemoryFootprint)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============================================================================
// Reset Performance Benchmarks
// ============================================================================

static void BM_ResetEmpty(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    
    for (auto _ : state) {
        collector.reset();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResetEmpty);

static void BM_ResetWithData(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    
    const int num_metrics = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        // Populate with data
        for (int i = 0; i < num_metrics; i++) {
            collector.recordQuery("select", i * 0.1, i);
        }
        state.ResumeTiming();
        
        collector.reset();
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ResetWithData)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000);

// ============================================================================
// Real-World Simulation Benchmarks
// ============================================================================

static void BM_SimulateQueryWorkload(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> latency_dist(25.0, 8.0);
    std::uniform_int_distribution<> result_dist(0, 1000);
    
    for (auto _ : state) {
        // Simulate query execution
        collector.recordQuery("select", latency_dist(gen), result_dist(gen));
        
        // 80% cache hit rate
        if (gen() % 10 < 8) {
            collector.recordCacheHit("query_cache");
        } else {
            collector.recordCacheMiss("query_cache");
        }
        
        // Index scan
        collector.recordIndexScan("btree", result_dist(gen));
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SimulateQueryWorkload);

static void BM_SimulateMonitoringWorkload(benchmark::State& state) {
    auto& collector = MetricsCollector::getInstance();
    ResetCollector(state);
    
    int counter = 0;
    for (auto _ : state) {
        // Record various system metrics
        collector.recordMemoryUsage(1024 * 1024 * (500 + counter % 100));
        collector.recordCPUUsage(50.0 + (counter % 50));
        collector.recordDiskIOps(1000 + counter % 500, 500 + counter % 250);
        
        // Export metrics periodically (every 100 iterations)
        if (counter % 100 == 0) {
            std::string metrics = collector.getPrometheusMetrics();
            benchmark::DoNotOptimize(metrics);
        }
        
        counter++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SimulateMonitoringWorkload);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
