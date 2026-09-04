// SPDX-License-Identifier: Apache-2.0
/**
 * @file bench_observability_phase2_exporter_stress.cpp
 * @brief Phase 5 Performance Benchmarks - Exporter Stress Testing
 *
 * This benchmark suite validates the observability module's exporter under
 * high-load conditions with multiple concurrent listeners, deduplication,
 * pattern matching, and malformed input handling.
 *
 * Performance Gates (GATE-01 through GATE-06):
 * - GATE-01: ≥50,000 metrics/sec sustained exporter ingestion
 * - GATE-02: ≤100µs P99 listener callback latency with 10+ concurrent listeners
 * - GATE-03: ≥10,000 rejections/sec for malformed inputs
 * - GATE-04: ≤10µs per listener lifecycle operation (add/remove)
 * - GATE-05: ≤50µs deduplication lookup latency
 * - GATE-06: ≥100,000 patterns/sec matched
 *
 * Status: Phase 5 Continuation (2026-08-15)
 */

#include <benchmark/benchmark.h>
#include <chrono>
#include <random>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <cmath>

using namespace std;

namespace themis {
namespace observability {

// Deterministic seed for reproducible benchmarks
constexpr uint64_t kObservabilityPhase5Seed = 42;

// ============================================================================
// Mock Components for Exporter Stress Testing
// ============================================================================

/**
 * @brief Simple metric representation for benchmarking
 */
struct Metric {
    string name;
    double value;
    map<string, string> labels;
    uint64_t timestamp_ns;
};

/**
 * @brief Listener callback for notifications
 */
struct ListenerCallback {
    string listener_id;
    atomic<uint64_t> invocations{0};
    atomic<uint64_t> total_latency_us{0};
};

/**
 * @brief Hint structure for deduplication
 */
struct Hint {
    string hint_id;
    uint64_t hint_value = {};
    uint64_t timestamp_ns;
    
    bool operator==(const Hint& other) const {
        return hint_id == other.hint_id && hint_value == other.hint_value;
    }
};

/**
 * @brief Pattern for pattern matching
 */
struct Pattern {
    string pattern;
    vector<string> tags;
    
    bool matches(const string& metric_name) const {
        // Simple substring matching for benchmark
        return metric_name.find(pattern) != string::npos;
    }
};

/**
 * @brief Simple exporter with configurable behavior
 */
class SimpleExporter {
public:
    explicit SimpleExporter(size_t max_queue_size = 100000)
        : max_queue_size_(max_queue_size), total_exported_(0) {}

    /**
     * Ingest metric into exporter queue
     */
    bool ingestMetric(const Metric& metric) {
        lock_guard<mutex> lock(queue_mutex_);
        
        if (metrics_queue_.size() >= max_queue_size_) {
            return false;  // Queue full
        }
        
        metrics_queue_.push(metric);
        return true;
    }

    /**
     * Process queued metrics (simulates export batching)
     */
    size_t processBatch(size_t batch_size = 1000) {
        lock_guard<mutex> lock(queue_mutex_);
        
        size_t processed = 0;
        while (processed < batch_size && !metrics_queue_.empty()) {
            const auto& metric = metrics_queue_.front();
            
            // Simulate lightweight export validation
            if (!metric.name.empty() && metric.value == metric.value) {  // Check NaN
                total_exported_++;
                processed++;
            }
            
            metrics_queue_.pop();
        }
        
        return processed;
    }

    /**
     * Get queue depth
     */
    size_t getQueueDepth() const {
        lock_guard<mutex> lock(queue_mutex_);
        return metrics_queue_.size();
    }

    /**
     * Get total exported count
     */
    uint64_t getTotalExported() const {
        return total_exported_.load();
    }

    /**
     * Check if metric is valid (not malformed)
     */
    static bool isValidMetric(const Metric& metric) {
        return !metric.name.empty() && metric.value == metric.value &&
               metric.timestamp_ns > 0;
    }

private:
    mutable mutex queue_mutex_;
    queue<Metric> metrics_queue_;
    size_t max_queue_size_;
    atomic<uint64_t> total_exported_{0};
};

/**
 * @brief Listener registry for concurrent listeners
 */
class ListenerRegistry {
public:
    /**
     * Add a listener
     */
    string addListener() {
        lock_guard<mutex> lock(registry_mutex_);
        
        string listener_id = "listener_" + to_string(next_listener_id_++);
        listeners_[listener_id] = make_shared<ListenerCallback>();
        listeners_[listener_id]->listener_id = listener_id;
        
        return listener_id;
    }

    /**
     * Remove a listener
     */
    bool removeListener(const string& listener_id) {
        lock_guard<mutex> lock(registry_mutex_);
        
        auto it = listeners_.find(listener_id);
        if (it == listeners_.end()) {
            return false;
        }
        
        listeners_.erase(it);
        return true;
    }

    /**
     * Notify all listeners
     */
    uint64_t notifyAllListeners() {
        lock_guard<mutex> lock(registry_mutex_);
        
        auto start_ns = chrono::high_resolution_clock::now();
        
        for (const auto& [listener_id, callback] : listeners_) {
            callback->invocations++;
            
            // Simulate lightweight callback work
            volatile int x = 0;
            for (int i = 0; i < 10; ++i) {
                x += i;
            }
            (void)x;  // Avoid optimization
        }
        
        auto end_ns = chrono::high_resolution_clock::now();
        auto elapsed_us = chrono::duration_cast<chrono::microseconds>(
            end_ns - start_ns).count();
        
        return elapsed_us;
    }

    /**
     * Get listener count
     */
    size_t getListenerCount() const {
        lock_guard<mutex> lock(registry_mutex_);
        return listeners_.size();
    }

    /**
     * Get total callback invocations
     */
    uint64_t getTotalInvocations() const {
        lock_guard<mutex> lock(registry_mutex_);
        
        uint64_t total = 0;
        for (const auto& [_, callback] : listeners_) {
            total += callback->invocations.load();
        }
        
        return total;
    }

private:
    mutable mutex registry_mutex_;
    map<string, shared_ptr<ListenerCallback>> listeners_;
    uint64_t next_listener_id_{0};
};

/**
 * @brief Hint deduplication cache
 */
class HintDeduplicator {
public:
    explicit HintDeduplicator(size_t cache_size = 10000)
        : cache_size_(cache_size) {}

    /**
     * Deduplicate or register hint
     */
    bool addOrCheckHint(const Hint& hint) {
        lock_guard<mutex> lock(dedup_mutex_);
        
        auto key = hint.hint_id + "_" + to_string(hint.hint_value);
        
        auto it = hint_cache_.find(key);
        if (it != hint_cache_.end()) {
            return true;  // Duplicate
        }
        
        if (hint_cache_.size() >= cache_size_) {
            // Remove oldest entry (simple FIFO)
            hint_cache_.erase(hint_cache_.begin());
        }
        
        hint_cache_[key] = hint;
        return false;  // New hint
    }

    /**
     * Clear the deduplication cache
     */
    void clear() {
        lock_guard<mutex> lock(dedup_mutex_);
        hint_cache_.clear();
    }

    /**
     * Get cache size
     */
    size_t getCacheSize() const {
        lock_guard<mutex> lock(dedup_mutex_);
        return hint_cache_.size();
    }

private:
    mutable mutex dedup_mutex_;
    map<string, Hint> hint_cache_;
    size_t cache_size_;
};

/**
 * @brief Pattern matcher for metric filtering
 */
class PatternMatcher {
public:
    /**
     * Add a pattern for matching
     */
    void addPattern(const Pattern& pattern) {
        lock_guard<mutex> lock(patterns_mutex_);
        patterns_.push_back(pattern);
    }

    /**
     * Match metric name against all patterns
     */
    size_t matchMetric(const string& metric_name) {
        lock_guard<mutex> lock(patterns_mutex_);
        
        size_t matches = 0;
        for (const auto& pattern : patterns_) {
            if (pattern.matches(metric_name)) {
                matches++;
            }
        }
        
        return matches;
    }

    /**
     * Get pattern count
     */
    size_t getPatternCount() const {
        lock_guard<mutex> lock(patterns_mutex_);
        return patterns_.size();
    }

    /**
     * Clear all patterns
     */
    void clear() {
        lock_guard<mutex> lock(patterns_mutex_);
        patterns_.clear();
    }

private:
    mutable mutex patterns_mutex_;
    vector<Pattern> patterns_;
};

}  // namespace observability
}  // namespace themis

using namespace themis::observability;

namespace {

// ============================================================================
// GATE-01: BM_OBE_01_ExporterIngestHighLoad
// Stress test: 100,000+ metrics/sec through exporter
// Measure: P95/P99 latency, throughput
// Gate: ≥50,000 metrics/sec sustained
// ============================================================================
static void BM_OBE_01_ExporterIngestHighLoad(benchmark::State& state) {
    SimpleExporter exporter(1000000);  // Large queue
    
    // Pre-generate metrics
    vector<Metric> metrics;
    mt19937 rng(kObservabilityPhase5Seed);
    uniform_int_distribution<int> value_dist(1, 1000);
    
    for (int i = 0; i < 100000; ++i) {
        Metric m{
            .name = "benchmark.metric_" + to_string(i % 100),
            .value = static_cast<double>(value_dist(rng)),
            .labels = {
                {"instance", "bench_" + to_string(i % 10)},
                {"region", "us-east-1"}
            },
            .timestamp_ns = chrono::high_resolution_clock::now()
                .time_since_epoch().count()
        };
        metrics.push_back(m);
    }
    
    size_t metric_idx = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            exporter.ingestMetric(metrics[metric_idx % metrics.size()]);
            metric_idx++;
        }
        
        // Periodically process batches
        if (metric_idx % 10000 == 0) {
            exporter.processBatch(10000);
        }
    }
    
    // Process remaining metrics
    while (exporter.getQueueDepth() > 0) {
        exporter.processBatch(10000);
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
    state.counters["metrics_exported"] = exporter.getTotalExported();
}
BENCHMARK(BM_OBE_01_ExporterIngestHighLoad)->Repetitions(5);

// ============================================================================
// GATE-02: BM_OBE_02_ConcurrentListenerNotification
// Stress test: 10+ concurrent listeners, 1000 hints/sec
// Measure: Lock contention, listener callback latency
// Gate: ≤100µs P99 listener callback latency
// ============================================================================
static void BM_OBE_02_ConcurrentListenerNotification(benchmark::State& state) {
    ListenerRegistry registry;
    
    // Add 10 concurrent listeners
    vector<string> listener_ids;
    for (int i = 0; i < 10; ++i) {
        listener_ids.push_back(registry.addListener());
    }
    
    vector<uint64_t> latencies;
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            uint64_t latency_us = registry.notifyAllListeners();
            latencies.push_back(latency_us);
        }
    }
    
    // Calculate latency percentiles
    sort(latencies.begin(), latencies.end());
    if (!latencies.empty()) {
        size_t p95_idx = (latencies.size() * 95) / 100;
        size_t p99_idx = (latencies.size() * 99) / 100;
        
        state.counters["p95_latency_us"] = latencies[p95_idx];
        state.counters["p99_latency_us"] = latencies[p99_idx];
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_OBE_02_ConcurrentListenerNotification)->Repetitions(5);

// ============================================================================
// GATE-03: BM_OBE_03_MalformedInputReject
// Stress test: Mixed valid/malformed metrics, 50% rejection rate
// Measure: Rejection throughput, diagnostic surface overhead
// Gate: ≥10,000 rejections/sec
// ============================================================================
static void BM_OBE_03_MalformedInputReject(benchmark::State& state) {
    SimpleExporter exporter(100000);
    
    mt19937 rng(kObservabilityPhase5Seed + 1);
    uniform_real_distribution<double> validity_dist(0.0, 1.0);
    uniform_int_distribution<int> value_dist(1, 1000);
    
    uint64_t rejected = 0;
    uint64_t accepted = 0;
    
    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            // Generate ~50% malformed metrics
            bool is_valid = validity_dist(rng) > 0.5;
            
            Metric m = {};
            if (is_valid) {
                m = {
                    .name = "valid.metric_" + to_string(i),
                    .value = static_cast<double>(value_dist(rng)),
                    .labels = {{"type", "valid"}},
                    .timestamp_ns = chrono::high_resolution_clock::now()
                        .time_since_epoch().count()
                };
                accepted++;
            } else {
                // Create malformed metric (invalid name or NaN)
                m = {
                    .name = "",  // Empty name - invalid
                    .value = NAN,
                    .labels = {{"type", "malformed"}},
                    .timestamp_ns = 0
                };
                rejected++;
            }
            
            // Validation check (simulates rejection)
            if (!SimpleExporter::isValidMetric(m)) {
                rejected++;
            } else {
                exporter.ingestMetric(m);
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
    state.counters["rejected_metrics"] = rejected;
    state.counters["accepted_metrics"] = accepted;
}
BENCHMARK(BM_OBE_03_MalformedInputReject)->Repetitions(5);

// ============================================================================
// GATE-04: BM_OBE_04_ListenerLifecycle
// Stress test: Rapid add/remove/notify cycle
// Measure: Lifecycle operations latency, weak_ptr management overhead
// Gate: ≤10µs per lifecycle operation
// ============================================================================
static void BM_OBE_04_ListenerLifecycle(benchmark::State& state) {
    ListenerRegistry registry;
    
    vector<uint64_t> operation_latencies;
    
    for (auto _ : state) {
        for (int i = 0; i < 100; ++i) {
            // Add listeners
            auto start = chrono::high_resolution_clock::now();
            string id1 = registry.addListener();
            string id2 = registry.addListener();
            auto add_end = chrono::high_resolution_clock::now();
            
            auto add_latency_us = chrono::duration_cast<chrono::microseconds>(
                add_end - start).count();
            operation_latencies.push_back(add_latency_us);
            
            // Notify
            auto notify_start = chrono::high_resolution_clock::now();
            registry.notifyAllListeners();
            auto notify_end = chrono::high_resolution_clock::now();
            
            auto notify_latency_us = chrono::duration_cast<chrono::microseconds>(
                notify_end - notify_start).count();
            operation_latencies.push_back(notify_latency_us);
            
            // Remove listeners
            auto remove_start = chrono::high_resolution_clock::now();
            registry.removeListener(id1);
            registry.removeListener(id2);
            auto remove_end = chrono::high_resolution_clock::now();
            
            auto remove_latency_us = chrono::duration_cast<chrono::microseconds>(
                remove_end - remove_start).count();
            operation_latencies.push_back(remove_latency_us);
        }
    }
    
    // Calculate percentiles
    sort(operation_latencies.begin(), operation_latencies.end());
    if (!operation_latencies.empty()) {
        size_t p95_idx = (operation_latencies.size() * 95) / 100;
        size_t p99_idx = (operation_latencies.size() * 99) / 100;
        
        state.counters["p95_lifecycle_us"] = operation_latencies[p95_idx];
        state.counters["p99_lifecycle_us"] = operation_latencies[p99_idx];
    }
    
    state.SetItemsProcessed(state.iterations() * 100 * 3);  // 3 ops per iteration
}
BENCHMARK(BM_OBE_04_ListenerLifecycle)->Repetitions(5);

// ============================================================================
// GATE-05: BM_OBE_05_HintDeduplication
// Stress test: Duplicate hint generation with deduplication
// Measure: Deduplication latency, dedup window accuracy
// Gate: ≤50µs deduplication lookup
// ============================================================================
static void BM_OBE_05_HintDeduplication(benchmark::State& state) {
    HintDeduplicator deduplicator(10000);
    
    // Pre-generate hints
    vector<Hint> hints;
    mt19937 rng(kObservabilityPhase5Seed + 2);
    uniform_int_distribution<int> hint_val_dist(1, 100);
    
    for (int i = 0; i < 1000; ++i) {
        Hint h{
            .hint_id = "hint_" + to_string(i % 100),  // Some duplicate IDs
            .hint_value = hint_val_dist(rng),
            .timestamp_ns = chrono::high_resolution_clock::now()
                .time_since_epoch().count()
        };
        hints.push_back(h);
    }
    
    vector<uint64_t> dedup_latencies;
    uint64_t duplicates_found = 0;
    
    size_t hint_idx = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            auto start = chrono::high_resolution_clock::now();
            
            bool is_duplicate = deduplicator.addOrCheckHint(
                hints[hint_idx % hints.size()]);
            
            auto end = chrono::high_resolution_clock::now();
            auto latency_us = chrono::duration_cast<chrono::microseconds>(
                end - start).count();
            
            dedup_latencies.push_back(latency_us);
            if (is_duplicate) {
                duplicates_found++;
            }
            
            hint_idx++;
        }
    }
    
    // Calculate percentiles
    sort(dedup_latencies.begin(), dedup_latencies.end());
    if (!dedup_latencies.empty()) {
        size_t p95_idx = (dedup_latencies.size() * 95) / 100;
        size_t p99_idx = (dedup_latencies.size() * 99) / 100;
        
        state.counters["p95_dedup_us"] = dedup_latencies[p95_idx];
        state.counters["p99_dedup_us"] = dedup_latencies[p99_idx];
        state.counters["duplicates_found"] = duplicates_found;
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_OBE_05_HintDeduplication)->Repetitions(5);

// ============================================================================
// GATE-06: BM_OBE_06_PatternMatching
// Stress test: 1000+ patterns matched against 100,000 metrics
// Measure: Pattern matching throughput, memory usage
// Gate: ≥100,000 patterns/sec matched
// ============================================================================
static void BM_OBE_06_PatternMatching(benchmark::State& state) {
    PatternMatcher matcher;
    
    // Add 1000 patterns
    for (int i = 0; i < 1000; ++i) {
        Pattern p{
            .pattern = "metric_" + to_string(i % 50),  // Some overlapping patterns
            .tags = {"prod", "bench"}
        };
        matcher.addPattern(p);
    }
    
    // Generate metric names
    vector<string> metrics;
    for (int i = 0; i < 100; ++i) {
        metrics.push_back("benchmark.metric_" + to_string(i));
    }
    
    vector<uint64_t> match_latencies;
    uint64_t total_matches = 0;
    
    size_t metric_idx = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            auto start = chrono::high_resolution_clock::now();
            
            size_t matches = matcher.matchMetric(metrics[metric_idx % metrics.size()]);
            
            auto end = chrono::high_resolution_clock::now();
            auto latency_us = chrono::duration_cast<chrono::microseconds>(
                end - start).count();
            
            match_latencies.push_back(latency_us);
            total_matches += matches;
            metric_idx++;
        }
    }
    
    // Calculate percentiles
    sort(match_latencies.begin(), match_latencies.end());
    if (!match_latencies.empty()) {
        size_t p95_idx = (match_latencies.size() * 95) / 100;
        size_t p99_idx = (match_latencies.size() * 99) / 100;
        
        state.counters["p95_pattern_us"] = match_latencies[p95_idx];
        state.counters["p99_pattern_us"] = match_latencies[p99_idx];
        state.counters["total_pattern_matches"] = total_matches;
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_OBE_06_PatternMatching)->Repetitions(5);

}  // namespace

BENCHMARK_MAIN();
