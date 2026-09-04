#include <benchmark/benchmark.h>
#include <vector>
#include <chrono>
#include <string>
#include <memory>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <unordered_map>
#include <queue>

namespace themis::process::benchmark {

// ============================================================================
// Constants
// ============================================================================

constexpr uint64_t kCanonicalRngSeed = 42;
constexpr int kSmallDatasetSize = 100;
constexpr int kMediumDatasetSize = 1000;
constexpr int kLargeDatasetSize = 10000;

// ============================================================================
// Process Model Graph Types
// ============================================================================

/**
 * @brief Represents a link between two process models
 */
struct ProcessLink {
    std::string source_id;
    std::string target_id;
    std::string link_type;  // "depends_on", "triggers", "follows", etc.
    int64_t created_ms{0};
    bool is_valid{true};
};

/**
 * @brief Represents a node in the process dependency graph
 */
struct GraphNode {
    std::string node_id;
    std::vector<std::string> outgoing;  // IDs of nodes this points to
    std::vector<std::string> incoming;  // IDs of nodes that point to this
};

/**
 * @brief Process model linker with validation and cycle detection
 */
class ProcessLinker {
private:
    std::unordered_map<std::string, GraphNode> graph_;
    std::vector<ProcessLink> links_;
    std::unordered_set<std::string> visited_;
    std::unordered_set<std::string> rec_stack_;

public:
    ProcessLinker() = default;

    /**
     * @brief Create a link between two models
     */
    bool createLink(const std::string& source_id, const std::string& target_id,
                   const std::string& link_type) {
        // Basic validation
        if (source_id.empty() || target_id.empty() || source_id == target_id) {
            return false;
        }

        // Ensure nodes exist in graph
        if (graph_.find(source_id) == graph_.end()) {
            graph_[source_id] = GraphNode{source_id, {}, {}};
        }
        if (graph_.find(target_id) == graph_.end()) {
            graph_[target_id] = GraphNode{target_id, {}, {}};
        }

        // Add link
        auto& source_node = graph_[source_id];
        source_node.outgoing.push_back(target_id);

        auto& target_node = graph_[target_id];
        target_node.incoming.push_back(source_id);

        ProcessLink link;
        link.source_id = source_id;
        link.target_id = target_id;
        link.link_type = link_type;
        link.created_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        links_.push_back(link);

        return true;
    }

    /**
     * @brief Validate a link exists and is correct
     */
    bool validateLink(const std::string& source_id, const std::string& target_id) {
        auto it = graph_.find(source_id);
        if (it == graph_.end()) {
            return false;
        }

        const auto& outgoing = it->second.outgoing;
        return std::find(outgoing.begin(), outgoing.end(), target_id) != outgoing.end();
    }

    /**
     * @brief Detect cycles using DFS
     */
    bool hasCycle() {
        visited_.clear();
        rec_stack_.clear();

        for (const auto& [node_id, _] : graph_) {
            if (visited_.find(node_id) == visited_.end()) {
                if (hasCycleDFS(node_id)) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief Get all nodes reachable from a starting node (graph traversal)
     */
    std::vector<std::string> getReachableNodes(const std::string& start_id) {
        std::vector<std::string> reachable;
        std::unordered_set<std::string> visited;
        std::queue<std::string> q;

        q.push(start_id);
        visited.insert(start_id);

        while (!q.empty()) {
            std::string current = q.front();
            q.pop();
            reachable.push_back(current);

            auto it = graph_.find(current);
            if (it != graph_.end()) {
                for (const auto& neighbor : it->second.outgoing) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        q.push(neighbor);
                    }
                }
            }
        }

        return reachable;
    }

    /**
     * @brief Validate all links in the graph
     */
    int validateAllLinks() {
        int valid_count = 0;
        for (const auto& link : links_) {
            if (validateLink(link.source_id, link.target_id)) {
                valid_count++;
            }
        }
        return valid_count;
    }

    size_t linkCount() const { return links_.size(); }
    size_t nodeCount() const { return graph_.size(); }

private:
    /**
     * @brief DFS helper for cycle detection
     */
    bool hasCycleDFS(const std::string& node_id) {
        visited_.insert(node_id);
        rec_stack_.insert(node_id);

        auto it = graph_.find(node_id);
        if (it != graph_.end()) {
            for (const auto& neighbor : it->second.outgoing) {
                if (visited_.find(neighbor) == visited_.end()) {
                    if (hasCycleDFS(neighbor)) {
                        return true;
                    }
                } else if (rec_stack_.find(neighbor) != rec_stack_.end()) {
                    // Back edge found (cycle)
                    return true;
                }
            }
        }

        rec_stack_.erase(node_id);
        return false;
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate random process models for linking
 */
static std::vector<std::string> generateProcessModels(int count) {
    std::vector<std::string> models = {};

    for (int i = 0; i < count; ++i) {
        models.push_back("model_" + std::to_string(i));
    }
    return models;
}

/**
 * @brief Generate random links between models
 */
static std::vector<std::pair<int, int>> generateRandomLinks(int model_count, int link_count) {
    std::vector<std::pair<int, int>> links;
    std::mt19937 gen(kCanonicalRngSeed);
    std::uniform_int_distribution<> model_dist(0, model_count - 1);

    for (int i = 0; i < link_count; ++i) {
        int source = model_dist(gen);
        int target = model_dist(gen);

        // Avoid self-loops
        while (target == source) {
            target = model_dist(gen);
        }

        links.push_back({source, target});
    }

    return links;
}

/**
 * @brief Generate a link chain (for cycle testing)
 */
static std::vector<std::pair<int, int>> generateLinkChain(int length) {
    std::vector<std::pair<int, int>> links;
    for (int i = 0; i < length; ++i) {
        links.push_back({i, (i + 1) % length});
    }
    return links;
}

// ============================================================================
// LP-01: Linking Latency (100 pairs)
// ============================================================================

static void BM_LP01_LinkingLatency(benchmark::State& state) {
    const int num_models = kSmallDatasetSize;
    const int num_links = kSmallDatasetSize;

    auto models = generateProcessModels(num_models);
    auto link_pairs = generateRandomLinks(num_models, num_links);

    std::vector<double> latencies;
    latencies.reserve(num_links);

    for (auto _ : state) {
        state.PauseTiming();
        auto linker = std::make_unique<ProcessLinker>();
        latencies.clear();
        state.ResumeTiming();

        for (const auto& [source_idx, target_idx] : link_pairs) {
            auto start = std::chrono::high_resolution_clock::now();
            linker->createLink(models[source_idx], models[target_idx], "depends_on");
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double p99 = latencies[std::min(size_t(99), latencies.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(num_links * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_LP01_LinkingLatency)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// LP-02: Cyclic Dependency Detection (1k models)
// ============================================================================

static void BM_LP02_CyclicDependencyDetection(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;

    auto models = generateProcessModels(num_models);
    auto link_pairs = generateRandomLinks(num_models, num_models / 2);

    std::vector<double> latencies;
    latencies.reserve(10);

    for (auto _ : state) {
        state.PauseTiming();
        auto linker = std::make_unique<ProcessLinker>();
        
        // Create all links first
        for (const auto& [source_idx, target_idx] : link_pairs) {
            linker->createLink(models[source_idx], models[target_idx], "depends_on");
        }

        latencies.clear();
        state.ResumeTiming();

        // Now run cycle detection multiple times
        for (int i = 0; i < 10; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            bool has_cycle = linker->hasCycle();
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));

            benchmark::DoNotOptimize(has_cycle);
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double p99 = latencies[std::min(size_t(9), latencies.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 10);
}

BENCHMARK(BM_LP02_CyclicDependencyDetection)
    ->Iterations(5)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// LP-03: Link Validation (1k links)
// ============================================================================

static void BM_LP03_LinkValidation(benchmark::State& state) {
    const int num_models = kMediumDatasetSize;
    const int num_links = kMediumDatasetSize;

    auto models = generateProcessModels(num_models);
    auto link_pairs = generateRandomLinks(num_models, num_links);

    for (auto _ : state) {
        state.PauseTiming();
        auto linker = std::make_unique<ProcessLinker>();

        // Create all links
        for (const auto& [source_idx, target_idx] : link_pairs) {
            linker->createLink(models[source_idx], models[target_idx], "depends_on");
        }

        state.ResumeTiming();

        // Validate all links
        int validated = 0;
        for (const auto& [source_idx, target_idx] : link_pairs) {
            if (linker->validateLink(models[source_idx], models[target_idx])) {
                validated++;
            }
        }

        benchmark::DoNotOptimize(validated);
    }

    state.SetItemsProcessed(num_links * static_cast<int64_t>(state.iterations()));
    state.counters["validations_per_sec"] =
        benchmark::Counter(num_links, benchmark::Counter::kIsRate);
}

BENCHMARK(BM_LP03_LinkValidation)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// LP-04: Graph Traversal (10k nodes)
// ============================================================================

static void BM_LP04_GraphTraversal(benchmark::State& state) {
    const int num_models = kLargeDatasetSize;
    const int num_links = kLargeDatasetSize / 2;

    auto models = generateProcessModels(num_models);
    auto link_pairs = generateRandomLinks(num_models, num_links);

    std::vector<double> latencies;
    latencies.reserve(100);

    for (auto _ : state) {
        state.PauseTiming();
        auto linker = std::make_unique<ProcessLinker>();

        // Create all links
        for (const auto& [source_idx, target_idx] : link_pairs) {
            linker->createLink(models[source_idx], models[target_idx], "depends_on");
        }

        latencies.clear();
        state.ResumeTiming();

        // Traverse graph from different starting nodes
        std::mt19937 gen(kCanonicalRngSeed);
        std::uniform_int_distribution<> model_dist(0, num_models - 1);

        for (int i = 0; i < 10; ++i) {
            int start_idx = model_dist(gen);
            std::string start_id = models[start_idx];

            auto start = std::chrono::high_resolution_clock::now();
            auto reachable = linker->getReachableNodes(start_id);
            auto end = std::chrono::high_resolution_clock::now();

            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            latencies.push_back(static_cast<double>(duration_ms));

            benchmark::DoNotOptimize(reachable);
        }
    }

    if (!latencies.empty()) {
        std::sort(latencies.begin(), latencies.end());
        double p99 = latencies[std::min(size_t(9), latencies.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 10);
}

BENCHMARK(BM_LP04_GraphTraversal)
    ->Iterations(3)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// LP-05: Stale Link Detection
// ============================================================================

struct LinkRecord {
    std::string id;
    std::string source_model_id;
    std::string target_model_id;
    int64_t created_ms{0};
    int64_t last_verified_ms{0};
    bool is_stale{false};
};

static void BM_LP05_StaleLinkDetection(benchmark::State& state) {
    const int num_links = 5000;
    const int64_t stale_threshold_ms = 3600000;  // 1 hour
    
    std::vector<LinkRecord> links;
    int64_t current_time_ms = 
        std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    
    std::mt19937_64 rng(kCanonicalRngSeed);
    std::uniform_int_distribution<> model_dist(1, 100);
    std::uniform_int_distribution<int64_t> time_dist(-7200000, 0);  // -2h to now
    
    for (int i = 0; i < num_links; ++i) {
        LinkRecord link;
        link.id = "link_" + std::to_string(i);
        link.source_model_id = "model_" + std::to_string(model_dist(rng));
        link.target_model_id = "model_" + std::to_string(model_dist(rng));
        link.created_ms = current_time_ms + time_dist(rng);
        link.last_verified_ms = link.created_ms + time_dist(rng) / 2;
        links.push_back(link);
    }

    std::vector<double> detection_times = {};

    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        // Detect stale links
        int stale_count = 0;
        for (auto& link : links) {
            int64_t age_ms = current_time_ms - link.last_verified_ms;
            if (age_ms > stale_threshold_ms) {
                link.is_stale = true;
                stale_count++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        detection_times.push_back(static_cast<double>(duration_ms));
        
        benchmark::DoNotOptimize(stale_count);
    }

    if (!detection_times.empty()) {
        std::sort(detection_times.begin(), detection_times.end());
        double p99 = detection_times[std::min(size_t(99), detection_times.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(num_links * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_LP05_StaleLinkDetection)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

// ============================================================================
// LP-06: Batch Link Operations
// ============================================================================

static void BM_LP06_BatchLinkOperations(benchmark::State& state) {
    const int batch_size = 1000;
    
    std::vector<double> batch_times = {};

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<LinkRecord> batch_links;
        state.ResumeTiming();

        auto start = std::chrono::high_resolution_clock::now();

        // Perform batch link operations
        int64_t current_time_ms = 
            std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        for (int i = 0; i < batch_size; ++i) {
            LinkRecord link;
            link.id = "batch_link_" + std::to_string(i);
            link.source_model_id = "model_" + std::to_string(i % 100);
            link.target_model_id = "model_" + std::to_string((i + 1) % 100);
            link.created_ms = current_time_ms;
            link.last_verified_ms = current_time_ms;
            link.is_stale = false;
            batch_links.push_back(link);
        }

        // Validate all links
        int valid_count = 0;
        for (const auto& link : batch_links) {
            if (!link.source_model_id.empty() && !link.target_model_id.empty()) {
                valid_count++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration_ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        batch_times.push_back(static_cast<double>(duration_ms));
        
        benchmark::DoNotOptimize(valid_count);
        benchmark::DoNotOptimize(batch_links);
    }

    if (!batch_times.empty()) {
        std::sort(batch_times.begin(), batch_times.end());
        double p99 = batch_times[std::min(size_t(99), batch_times.size() - 1)];
        state.counters["p99_ms"] = benchmark::Counter(p99, benchmark::Counter::kAvgIterations);
    }

    state.SetItemsProcessed(batch_size * static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_LP06_BatchLinkOperations)
    ->Iterations(10)
    ->ReportAggregatesOnly(true)
    ->UseRealTime();

}  // namespace themis::process::benchmark

BENCHMARK_MAIN();
