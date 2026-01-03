/**
 * @file bench_process_mining.cpp
 * @brief Google Benchmark suite for Process Mining (v1.3.0 Phase 2)
 * 
 * This benchmark file provides performance testing for:
 * - Mining algorithm performance (Alpha, Heuristic, Inductive)
 * - Large log processing scalability
 * - Variant clustering performance
 * - Export performance (BPMN, PNML, JSON)
 * - DFG creation performance
 * - Conformance checking performance
 */

#include <benchmark/benchmark.h>
#include "analytics/process_mining.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <random>

using namespace themis;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Create a synthetic event log for benchmarking
 */
static EventLog createSyntheticEventLog(size_t num_cases, size_t events_per_case) {
    EventLog log;
    
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<> activity_dist(0, 9);
    
    std::vector<std::string> activities = {
        "Receive Order", "Check Inventory", "Validate Payment",
        "Pick Items", "Pack Order", "Ship Order",
        "Deliver Order", "Send Invoice", "Receive Payment", "Close Order"
    };
    
    for (size_t i = 0; i < num_cases; ++i) {
        ProcessTrace trace;
        trace.case_id = "case_" + std::to_string(i);
        
        int64_t timestamp = 1000 + (i * 10000);
        
        for (size_t j = 0; j < events_per_case; ++j) {
            ProcessEvent event;
            event.case_id = trace.case_id;
            event.activity = activities[activity_dist(gen)];
            event.timestamp_ms = timestamp + (j * 1000);
            event.resource = (j % 2 == 0) ? "Alice" : "Bob";
            
            trace.events.push_back(event);
        }
        
        log.traces.push_back(trace);
        log.total_events += trace.events.size();
    }
    log.unique_cases = log.traces.size();
    log.unique_activities = activities.size();
    
    return log;
}

/**
 * @brief Setup database for benchmarking
 */
static std::unique_ptr<RocksDBWrapper> setupDatabase(const std::string& path) {
    std::filesystem::remove_all(path);
    
    RocksDBWrapper::Config config;
    config.db_path = path;
    return std::make_unique<RocksDBWrapper>(config);
}

// ============================================================================
// Mining Algorithm Performance Benchmarks
// ============================================================================

/**
 * @benchmark Alpha Miner performance
 */
static void BM_ProcessMining_AlphaMiner(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_alpha");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        MiningConfig cfg; cfg.algorithm = MiningAlgorithm::ALPHA;
        auto model = mining.discoverProcess(log, cfg);
        benchmark::DoNotOptimize(model);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_alpha");
}
BENCHMARK(BM_ProcessMining_AlphaMiner)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Heuristic Miner performance
 */
static void BM_ProcessMining_HeuristicMiner(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_heuristic");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        MiningConfig cfg; cfg.algorithm = MiningAlgorithm::HEURISTIC;
        auto model = mining.discoverProcess(log, cfg);
        benchmark::DoNotOptimize(model);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_heuristic");
}
BENCHMARK(BM_ProcessMining_HeuristicMiner)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Inductive Miner performance
 */
static void BM_ProcessMining_InductiveMiner(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_inductive");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        MiningConfig cfg; cfg.algorithm = MiningAlgorithm::INDUCTIVE;
        auto model = mining.discoverProcess(log, cfg);
        benchmark::DoNotOptimize(model);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_inductive");
}
BENCHMARK(BM_ProcessMining_InductiveMiner)
    ->Arg(10)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Large Log Processing Benchmarks
// ============================================================================

/**
 * @benchmark Event log extraction with varying log sizes
 */
static void BM_ProcessMining_EventLogExtraction(benchmark::State& state) {
    auto db = setupDatabase("/tmp/bench_pm_extraction");
    ProcessMining mining(*db);
    
    // Pre-populate database with events
    size_t num_cases = state.range(0);
    for (size_t i = 0; i < num_cases; ++i) {
        BaseEntity event("evt_" + std::to_string(i), BaseEntity::FieldMap{
            {"case_id", "case_" + std::to_string(i % 100)},
            {"activity", "Activity A"},
            {"timestamp", int64_t(1000 + i * 1000)}
        });
        db->put("event_log:" + event.getPrimaryKey(), event.serialize());
    }
    
    EventLogConfig config;
    config.case_id_field = "case_id";
    config.activity_field = "activity";
    config.timestamp_field = "timestamp";
    
    for (auto _ : state) {
        auto log = mining.extractEventLog("event_log", config);
        benchmark::DoNotOptimize(log);
    }
    
    state.SetItemsProcessed(state.iterations() * num_cases);
    
    std::filesystem::remove_all("/tmp/bench_pm_extraction");
}
BENCHMARK(BM_ProcessMining_EventLogExtraction)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Processing large event logs with many traces
 */
static void BM_ProcessMining_LargeLogProcessing(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), state.range(1));
    
    auto db = setupDatabase("/tmp/bench_pm_large");
    ProcessMining mining(*db);
    
    MiningConfig cfg; cfg.algorithm = MiningAlgorithm::HEURISTIC;
    for (auto _ : state) {
        auto model = mining.discoverProcess(log, cfg);
        benchmark::DoNotOptimize(model);
    }
    
    state.SetItemsProcessed(state.iterations() * log.total_events);
    
    std::filesystem::remove_all("/tmp/bench_pm_large");
}
BENCHMARK(BM_ProcessMining_LargeLogProcessing)
    ->Args({100, 10})   // 100 cases, 10 events each
    ->Args({500, 20})   // 500 cases, 20 events each
    ->Args({1000, 10})  // 1000 cases, 10 events each
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// DFG Creation Benchmarks
// ============================================================================

/**
 * @benchmark DFG creation performance
 */
static void BM_ProcessMining_DFGCreation(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_dfg");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        auto dfg = mining.createDFG(log);
        benchmark::DoNotOptimize(dfg);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_dfg");
}
BENCHMARK(BM_ProcessMining_DFGCreation)
    ->Arg(10)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark DFG with performance metrics
 */
static void BM_ProcessMining_DFGWithPerformance(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_dfg_perf");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        auto dfg = mining.createDFG(log); // Current API returns DFG only
        benchmark::DoNotOptimize(dfg);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_dfg_perf");
}
BENCHMARK(BM_ProcessMining_DFGWithPerformance)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Variant Analysis Benchmarks
// ============================================================================

/**
 * @benchmark Variant analysis performance
 */
static void BM_ProcessMining_VariantAnalysis(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_variants");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        auto variants = mining.analyzeVariants(log);
        benchmark::DoNotOptimize(variants);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_variants");
}
BENCHMARK(BM_ProcessMining_VariantAnalysis)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark Variant clustering performance
 */
static void BM_ProcessMining_VariantClustering(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_clustering");
    ProcessMining mining(*db);
    
    for (auto _ : state) {
        auto clusters = mining.clusterVariants(log, 5); // 5 clusters
        benchmark::DoNotOptimize(clusters);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_clustering");
}
BENCHMARK(BM_ProcessMining_VariantClustering)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Conformance Checking Benchmarks
// ============================================================================

/**
 * @benchmark Token replay conformance checking
 */
static void BM_ProcessMining_ConformanceChecking(benchmark::State& state) {
    auto log = createSyntheticEventLog(state.range(0), 10);
    
    auto db = setupDatabase("/tmp/bench_pm_conformance");
    ProcessMining mining(*db);
    
    {
        MiningConfig cfg; cfg.algorithm = MiningAlgorithm::HEURISTIC;
        auto tmp = mining.discoverProcess(log, cfg);
        (void)tmp;
    }
    auto model = mining.discoverProcess(log).second;
    
    for (auto _ : state) {
        auto conformance = mining.checkConformance(log, model);
        benchmark::DoNotOptimize(conformance);
    }
    
    state.SetItemsProcessed(state.iterations() * log.traces.size());
    
    std::filesystem::remove_all("/tmp/bench_pm_conformance");
}
BENCHMARK(BM_ProcessMining_ConformanceChecking)
    ->Arg(50)
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Export Performance Benchmarks
// ============================================================================

/**
 * @benchmark BPMN export performance
 */
static void BM_ProcessMining_BPMNExport(benchmark::State& state) {
    auto log = createSyntheticEventLog(100, 10);
    
    auto db = setupDatabase("/tmp/bench_pm_bpmn");
    ProcessMining mining(*db);
    
    {
        MiningConfig cfg; cfg.algorithm = MiningAlgorithm::HEURISTIC;
        auto tmp = mining.discoverProcess(log, cfg);
        (void)tmp;
    }
    auto model = mining.discoverProcess(log).second;
    
    for (auto _ : state) {
        auto bpmn = mining.exportToBPMN(model);
        benchmark::DoNotOptimize(bpmn);
    }
    
    std::filesystem::remove_all("/tmp/bench_pm_bpmn");
}
BENCHMARK(BM_ProcessMining_BPMNExport)->Unit(benchmark::kMicrosecond);

/**
 * @benchmark Petri Net (PNML) export performance
 */
static void BM_ProcessMining_PNMLExport(benchmark::State& state) {
    auto log = createSyntheticEventLog(100, 10);
    
    auto db = setupDatabase("/tmp/bench_pm_pnml");
    ProcessMining mining(*db);
    
    MiningConfig cfg; cfg.algorithm = MiningAlgorithm::HEURISTIC;
    auto model = mining.discoverProcess(log, cfg).second;
    
    for (auto _ : state) {
        auto pnml = mining.exportToPNML(model);
        benchmark::DoNotOptimize(pnml);
    }
    
    std::filesystem::remove_all("/tmp/bench_pm_pnml");
}
BENCHMARK(BM_ProcessMining_PNMLExport)->Unit(benchmark::kMicrosecond);

// JSON-Export nicht verfügbar – Benchmark entfernt

// ============================================================================
// Social Network Mining Benchmarks
// ============================================================================

// Social-Network-Extraktion aktuell nicht Teil der API – Benchmark entfernt

// Main function for Google Benchmark
BENCHMARK_MAIN();
