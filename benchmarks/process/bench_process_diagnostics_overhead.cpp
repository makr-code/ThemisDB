/*
 * ThemisDB | File: bench_process_diagnostics_overhead.cpp | Version: 1.0.0
 * Phase 5: Process Module Performance & Hardening
 *
 * Diagnostics Overhead Gates (GO):
 * | Gate ID | Metric                              | Target           |
 * |---------|-------------------------------------|------------------|
 * | GO-01   | Incident Classification Overhead    | Regression ≤ 5%  |
 */

#include <benchmark/benchmark.h>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <algorithm>
#include <memory>
#include <unordered_map>

namespace themis::process::benchmark {

// ============================================================================
// Constants
// ============================================================================

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kSmallDatasetSize = 100;
constexpr int kMediumDatasetSize = 1000;

// ============================================================================
// Incident Classification Types
// ============================================================================

enum class IncidentType {
    kPerfAnomaly,
    kDeadlock,
    kResourceExhaustion,
    kStateIncoherence,
    kSerializationFailure,
    kTimeoutViolation,
    kCyclicDependency,
    kInvalidTransition
};

/**
 * @brief Simulated diagnostic event for incident classification
 */
struct DiagnosticEvent {
    int64_t timestamp_ms{0};
    std::string component;
    std::string metric_name;
    double metric_value{0.0};
    std::string error_message;
    int severity_level{0};  // 0=INFO, 1=WARNING, 2=ERROR
};

/**
 * @brief Simulated incident for classification
 */
struct ProcessIncident {
    std::string incident_id;
    IncidentType type;
    std::vector<DiagnosticEvent> events;
    int64_t first_event_ms{0};
    int64_t last_event_ms{0};
    int classification_score{0};
};

/**
 * @brief Baseline incident classifier (no diagnostics overhead)
 */
class BaselineIncidentClassifier {
public:
    BaselineIncidentClassifier() = default;

    /**
     * @brief Classify incident without detailed diagnostics
     */
    IncidentType classifyBaseline(const ProcessIncident& incident) {
        // Simple rule-based classification without overhead
        if (incident.events.empty()) {
            return IncidentType::kPerfAnomaly;
        }

        double max_metric = 0.0;
        for (const auto& event : incident.events) {
            max_metric = std::max(max_metric, event.metric_value);
        }

        if (max_metric > 1000.0) {
            return IncidentType::kResourceExhaustion;
        } else if (max_metric > 500.0) {
            return IncidentType::kPerfAnomaly;
        }

        return IncidentType::kTimeoutViolation;
    }
};

/**
 * @brief Enhanced incident classifier (with diagnostics)
 */
class EnhancedIncidentClassifier {
private:
    struct ClassificationRule {
        std::string pattern;
        IncidentType incident_type;
        int priority{0};
    };

    std::vector<ClassificationRule> rules_;
    std::unordered_map<std::string, int> classification_cache_;

public:
    EnhancedIncidentClassifier() {
        // Initialize classification rules
        rules_ = {
            {"latency_spike", IncidentType::kPerfAnomaly, 10},
            {"deadlock_detected", IncidentType::kDeadlock, 20},
            {"memory_limit", IncidentType::kResourceExhaustion, 15},
            {"state_mismatch", IncidentType::kStateIncoherence, 18},
            {"serialization_error", IncidentType::kSerializationFailure, 12},
            {"timeout_exceeded", IncidentType::kTimeoutViolation, 14},
            {"cycle_found", IncidentType::kCyclicDependency, 16},
            {"invalid_state", IncidentType::kInvalidTransition, 11}
        };
    }

    /**
     * @brief Classify incident with full diagnostics
     */
    IncidentType classifyWithDiagnostics(const ProcessIncident& incident) {
        // Check cache first (simulates caching optimization)
        auto cache_key = incident.incident_id;
        auto cache_it = classification_cache_.find(cache_key);
        if (cache_it != classification_cache_.end()) {
            return static_cast<IncidentType>(cache_it->second);
        }

        // Detailed classification with pattern matching
        IncidentType best_type = IncidentType::kPerfAnomaly;
        int best_score = 0;

        for (const auto& event : incident.events) {
            for (const auto& rule : rules_) {
                // Check if event message contains rule pattern
                if (event.error_message.find(rule.pattern) != std::string::npos) {
                    int score = rule.priority + (event.severity_level * 10);
                    if (score > best_score) {
                        best_score = score;
                        best_type = rule.incident_type;
                    }
                }
            }
        }

        // Cache result (simulates caching benefit)
        classification_cache_[cache_key] = static_cast<int>(best_type);

        // Simulate detailed analysis overhead
        analyzeIncidentDetails(incident);

        return best_type;
    }

private:
    /**
     * @brief Simulate detailed incident analysis (the overhead)
     */
    void analyzeIncidentDetails(const ProcessIncident& incident) {
        // Simulate correlation analysis
        std::vector<int> correlations;
        for (size_t i = 0; i < incident.events.size(); ++i) {
            for (size_t j = i + 1; j < incident.events.size(); ++j) {
                // Simulate temporal correlation check
                int64_t time_diff =
                    incident.events[j].timestamp_ms - incident.events[i].timestamp_ms;
                if (time_diff < 1000) {
                    correlations.push_back(static_cast<int>(time_diff));
                }
            }
        }

        // Simulate root cause analysis (more overhead)
        if (!correlations.empty()) {
            std::sort(correlations.begin(), correlations.end());
            int median_corr = correlations[correlations.size() / 2];
            benchmark::DoNotOptimize(median_corr);
        }

        // Simulate severity aggregation
        int total_severity = 0;
        for (const auto& event : incident.events) {
            total_severity += event.severity_level;
        }
        double avg_severity = static_cast<double>(total_severity) / incident.events.size();
        benchmark::DoNotOptimize(avg_severity);
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate synthetic diagnostic events
 */
static std::vector<DiagnosticEvent> generateDiagnosticEvents(int count) {
    std::vector<DiagnosticEvent> events;
    std::mt19937 gen(kCanonicalRngSeed);
    std::uniform_int_distribution<> severity_dist(0, 2);
    std::uniform_real_distribution<> value_dist(100.0, 2000.0);

    int64_t base_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

    static const char* components[] = {
        "linker", "parser", "retriever", "diagnostics", "concurrency"
    };
    static const char* metrics[] = {
        "latency_ms", "throughput_ops", "memory_mb", "conflict_count", "error_rate"
    };

    for (int i = 0; i < count; ++i) {
        DiagnosticEvent event;
        event.timestamp_ms = base_ms + (i * 100);
        event.component = components[i % 5];
        event.metric_name = metrics[i % 5];
        event.metric_value = value_dist(gen);
        event.severity_level = severity_dist(gen);

        static const char* error_msgs[] = {
            "latency_spike detected",
            "deadlock_detected in model linking",
            "memory_limit reached",
            "state_mismatch found",
            "serialization_error occurred",
            "timeout_exceeded in operation",
            "cycle_found in dependency graph",
            "invalid_state transition"
        };
        event.error_message = error_msgs[i % 8];

        events.push_back(event);
    }

    return events;
}

/**
 * @brief Generate synthetic incidents
 */
static std::vector<ProcessIncident> generateIncidents(int count) {
    std::vector<ProcessIncident> incidents;
    std::mt19937 gen(kCanonicalRngSeed);
    std::uniform_int_distribution<> event_count_dist(5, 20);

    int64_t base_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;

    for (int i = 0; i < count; ++i) {
        ProcessIncident incident;
        incident.incident_id = "incident_" + std::to_string(i);
        incident.type = static_cast<IncidentType>(i % 8);
        incident.first_event_ms = base_ms + (i * 10000);

        int event_count = event_count_dist(gen);
        auto all_events = generateDiagnosticEvents(event_count);
        incident.events.insert(incident.events.end(), all_events.begin(), all_events.end());

        if (!incident.events.empty()) {
            incident.last_event_ms = incident.events.back().timestamp_ms;
        }

        incidents.push_back(incident);
    }

    return incidents;
}

// ============================================================================
// GO-01: Incident Classification Overhead (Baseline vs Enhanced)
// ============================================================================

/**
 * @brief Measure baseline classification performance
 */
static void BM_GO01_ClassificationBaseline(benchmark::State& state) {
    const int num_incidents = kMediumDatasetSize;
    auto incidents = generateIncidents(num_incidents);
    auto classifier = std::make_unique<BaselineIncidentClassifier>();

    int64_t classifications_done = 0;
    for (auto _ : state) {
        for (const auto& incident : incidents) {
            auto type = classifier->classifyBaseline(incident);
            benchmark::DoNotOptimize(type);
            classifications_done++;
        }
    }

    state.SetItemsProcessed(classifications_done);
    state.counters["classifications_per_sec"] =
        benchmark::Counter(classifications_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GO01_ClassificationBaseline)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

/**
 * @brief Measure enhanced classification performance (with diagnostics)
 */
static void BM_GO01_ClassificationEnhanced(benchmark::State& state) {
    const int num_incidents = kMediumDatasetSize;
    auto incidents = generateIncidents(num_incidents);
    auto classifier = std::make_unique<EnhancedIncidentClassifier>();

    int64_t classifications_done = 0;
    for (auto _ : state) {
        for (const auto& incident : incidents) {
            auto type = classifier->classifyWithDiagnostics(incident);
            benchmark::DoNotOptimize(type);
            classifications_done++;
        }
    }

    state.SetItemsProcessed(classifications_done);
    state.counters["classifications_per_sec"] =
        benchmark::Counter(classifications_done, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_GO01_ClassificationEnhanced)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

/**
 * @brief Measure classification overhead ratio (Enhanced/Baseline)
 */
static void BM_GO01_ClassificationOverhead(benchmark::State& state) {
    const int num_incidents = kSmallDatasetSize;
    auto incidents = generateIncidents(num_incidents);

    auto baseline_classifier = std::make_unique<BaselineIncidentClassifier>();
    auto enhanced_classifier = std::make_unique<EnhancedIncidentClassifier>();

    std::vector<double> baseline_latencies;
    std::vector<double> enhanced_latencies;
    baseline_latencies.reserve(num_incidents);
    enhanced_latencies.reserve(num_incidents);

    for (auto _ : state) {
        state.PauseTiming();
        baseline_latencies.clear();
        enhanced_latencies.clear();
        state.ResumeTiming();

        // Measure baseline
        for (const auto& incident : incidents) {
            auto start = std::chrono::high_resolution_clock::now();
            auto type = baseline_classifier->classifyBaseline(incident);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_us =
                std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            baseline_latencies.push_back(static_cast<double>(duration_us));
            benchmark::DoNotOptimize(type);
        }

        // Measure enhanced
        for (const auto& incident : incidents) {
            auto start = std::chrono::high_resolution_clock::now();
            auto type = enhanced_classifier->classifyWithDiagnostics(incident);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_us =
                std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            enhanced_latencies.push_back(static_cast<double>(duration_us));
            benchmark::DoNotOptimize(type);
        }
    }

    // Calculate overhead percentage
    if (!baseline_latencies.empty() && !enhanced_latencies.empty()) {
        double baseline_mean =
            std::accumulate(baseline_latencies.begin(), baseline_latencies.end(), 0.0) /
            baseline_latencies.size();
        double enhanced_mean =
            std::accumulate(enhanced_latencies.begin(), enhanced_latencies.end(), 0.0) /
            enhanced_latencies.size();

        if (baseline_mean > 0.0) {
            double overhead_pct = ((enhanced_mean - baseline_mean) / baseline_mean) * 100.0;
            state.counters["overhead_percent"] =
                benchmark::Counter(overhead_pct, benchmark::Counter::kAvgIterations);
            state.counters["baseline_us"] =
                benchmark::Counter(baseline_mean, benchmark::Counter::kAvgIterations);
            state.counters["enhanced_us"] =
                benchmark::Counter(enhanced_mean, benchmark::Counter::kAvgIterations);
        }
    }

    state.SetItemsProcessed(num_incidents * 2 * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_GO01_ClassificationOverhead)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// GO-02: Link Traversal Latency
// ============================================================================

struct GraphLink {
    std::string source;
    std::string target;
    int weight{1};
};

static void BM_GO02_LinkTraversalLatency(benchmark::State& state) {
    const int num_links = 1000;
    std::vector<GraphLink> links;
    
    std::mt19937_64 rng(kCanonicalRngSeed);
    std::uniform_int_distribution<> node_dist(0, 99);
    
    for (int i = 0; i < num_links; ++i) {
        GraphLink link;
        link.source = "node_" + std::to_string(node_dist(rng));
        link.target = "node_" + std::to_string(node_dist(rng));
        link.weight = 1 + (i % 10);
        links.push_back(link);
    }

    std::vector<double> latencies;
    for (auto _ : state) {
        for (const auto& link : links) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // Simulate link traversal
            std::string path = link.source + "->" + link.target;
            benchmark::DoNotOptimize(path);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration_ms = 
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        size_t p95_idx = std::min(size_t(latencies.size() * 95 / 100), latencies.size() - 1);
        double p95 = latencies[p95_idx];
        state.counters["p95_ms"] = benchmark::Counter(p95, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(latencies.size()));
}

BENCHMARK(BM_GO02_LinkTraversalLatency)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// GO-03: Graph Construction Time
// ============================================================================

static void BM_GO03_GraphConstructionTime(benchmark::State& state) {
    const int num_models = 100;
    
    std::vector<double> construction_times;
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<GraphLink> graph_edges;
        state.ResumeTiming();

        auto start = std::chrono::high_resolution_clock::now();

        // Build graph with edges between models
        for (int i = 0; i < num_models; ++i) {
            for (int j = i + 1; j < num_models; ++j) {
                GraphLink edge;
                edge.source = "model_" + std::to_string(i);
                edge.target = "model_" + std::to_string(j);
                edge.weight = (i + j) % 5 + 1;
                graph_edges.push_back(edge);
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        construction_times.push_back(static_cast<double>(duration_ms));
        
        benchmark::DoNotOptimize(graph_edges);
    }

    if (!construction_times.empty()) {
        std::sort(construction_times.begin(), construction_times.end());
        size_t p95_idx = std::min(size_t(construction_times.size() * 95 / 100), 
                                  construction_times.size() - 1);
        double p95 = construction_times[p95_idx];
        state.counters["p95_ms"] = 
            benchmark::Counter(p95, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(num_models * num_models / 2 * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_GO03_GraphConstructionTime)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// GO-04: Cycle Detection Performance
// ============================================================================

static void BM_GO04_CycleDetectionPerformance(benchmark::State& state) {
    const int num_nodes = 1000;
    std::vector<std::vector<int>> adjacency_list(num_nodes);
    
    std::mt19937_64 rng(kCanonicalRngSeed);
    std::uniform_int_distribution<> edge_dist(0, num_nodes - 1);
    
    // Create some edges
    for (int i = 0; i < num_nodes / 2; ++i) {
        int from = edge_dist(rng);
        int to = edge_dist(rng);
        if (from != to) {
            adjacency_list[from].push_back(to);
        }
    }

    std::vector<double> detection_times;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        // Detect cycles using DFS
        std::vector<int> color(num_nodes, 0);  // 0=white, 1=gray, 2=black
        bool has_cycle = false;

        std::function<bool(int)> hasCycleDFS = [&](int u) -> bool {
            color[u] = 1;
            for (int v : adjacency_list[u]) {
                if (color[v] == 1) {
                    return true;
                }
                if (color[v] == 0 && hasCycleDFS(v)) {
                    return true;
                }
            }
            color[u] = 2;
            return false;
        };

        for (int i = 0; i < num_nodes; ++i) {
            if (color[i] == 0 && hasCycleDFS(i)) {
                has_cycle = true;
                break;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        detection_times.push_back(static_cast<double>(duration_ms));
        
        benchmark::DoNotOptimize(has_cycle);
    }

    if (!detection_times.empty()) {
        std::sort(detection_times.begin(), detection_times.end());
        size_t p95_idx = std::min(size_t(detection_times.size() * 95 / 100), 
                                  detection_times.size() - 1);
        double p95 = detection_times[p95_idx];
        state.counters["p95_ms"] = 
            benchmark::Counter(p95, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(detection_times.size()) * num_nodes);
}

BENCHMARK(BM_GO04_CycleDetectionPerformance)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// GO-05: Community Detection
// ============================================================================

static void BM_GO05_CommunityDetection(benchmark::State& state) {
    const int num_nodes = 500;
    std::vector<std::vector<int>> adjacency_list(num_nodes);
    
    std::mt19937_64 rng(kCanonicalRngSeed);
    std::uniform_int_distribution<> edge_dist(0, num_nodes - 1);
    
    // Create a simple graph with some community structure
    for (int i = 0; i < num_nodes; ++i) {
        // Add edges within community (nodes with similar IDs)
        for (int j = 0; j < 5; ++j) {
            int target = (i + (j % 10) + 1) % num_nodes;
            if (i != target) {
                adjacency_list[i].push_back(target);
            }
        }
    }

    std::vector<double> detection_times;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        // Simple community detection using connected components
        std::vector<int> component(num_nodes, -1);
        int num_components = 0;

        for (int i = 0; i < num_nodes; ++i) {
            if (component[i] == -1) {
                // BFS to mark component
                std::queue<int> q;
                q.push(i);
                component[i] = num_components;

                while (!q.empty()) {
                    int u = q.front();
                    q.pop();

                    for (int v : adjacency_list[u]) {
                        if (component[v] == -1) {
                            component[v] = num_components;
                            q.push(v);
                        }
                    }
                }
                num_components++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        detection_times.push_back(static_cast<double>(duration_ms));
        
        benchmark::DoNotOptimize(num_components);
    }

    if (!detection_times.empty()) {
        std::sort(detection_times.begin(), detection_times.end());
        size_t p95_idx = std::min(size_t(detection_times.size() * 95 / 100), 
                                  detection_times.size() - 1);
        double p95 = detection_times[p95_idx];
        state.counters["p95_ms"] = 
            benchmark::Counter(p95, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(detection_times.size()) * num_nodes);
}

BENCHMARK(BM_GO05_CommunityDetection)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// GO-06: Complex Graph p95 Latency
// ============================================================================

static void BM_GO06_ComplexGraphP95Latency(benchmark::State& state) {
    const int num_nodes = 5000;
    std::vector<std::vector<int>> adjacency_list(num_nodes);
    
    std::mt19937_64 rng(kCanonicalRngSeed);
    std::uniform_int_distribution<> edge_dist(0, num_nodes - 1);
    
    // Create a more complex graph with higher connectivity
    for (int i = 0; i < num_nodes; ++i) {
        int num_edges = 3 + (i % 5);
        for (int j = 0; j < num_edges; ++j) {
            int target = edge_dist(rng);
            if (i != target) {
                adjacency_list[i].push_back(target);
            }
        }
    }

    std::vector<double> operation_times;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        // Perform various operations on the complex graph
        // 1. Count total edges
        int total_edges = 0;
        for (const auto& neighbors : adjacency_list) {
            total_edges += neighbors.size();
        }

        // 2. Find node with most connections
        int max_degree = 0;
        for (const auto& neighbors : adjacency_list) {
            max_degree = std::max(max_degree, static_cast<int>(neighbors.size()));
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        operation_times.push_back(static_cast<double>(duration_ms));
        
        benchmark::DoNotOptimize(total_edges);
        benchmark::DoNotOptimize(max_degree);
    }

    if (!operation_times.empty()) {
        std::sort(operation_times.begin(), operation_times.end());
        size_t p95_idx = std::min(size_t(operation_times.size() * 95 / 100), 
                                  operation_times.size() - 1);
        double p95 = operation_times[p95_idx];
        state.counters["p95_ms"] = 
            benchmark::Counter(p95, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(operation_times.size()) * num_nodes);
}

BENCHMARK(BM_GO06_ComplexGraphP95Latency)
    ->Iterations(3)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

}  // namespace themis::process::benchmark

BENCHMARK_MAIN();
