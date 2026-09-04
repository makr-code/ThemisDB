/**
 * @file parallel_traversal.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=20, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Parallel multi-source BFS/DFS implementation for ThemisDB graph module.

#include "graph/parallel_traversal.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <queue>
#include <thread>
#include <unordered_set>

#include "utils/error_registry.h"

namespace themis {
namespace graph {

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ParallelTraversal::ParallelTraversal(GraphIndexManager &graph_manager) : graph_manager_(graph_manager) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

size_t ParallelTraversal::effectiveThreadCount(const Config &config, size_t num_sources) {
    size_t requested = (config.num_threads > 0) ? static_cast<size_t>(config.num_threads) : []() -> size_t {
        const size_t hw   = std::thread::hardware_concurrency();
        const size_t base = (hw > 0) ? hw : 4u;
        return std::max<size_t>(2u, std::min<size_t>(base, 16u));
    }();

    // Never spawn more threads than there are sources.
    return std::min(requested, num_sources);
}

// ---------------------------------------------------------------------------
// Single-source BFS (runs inside an async task)
// ---------------------------------------------------------------------------

ParallelTraversal::SourceTraversalResult ParallelTraversal::runSingleBFS(const std::string &source,
                                                                         const Config &config) {
    SourceTraversalResult result;
    result.source = source;

    auto start_time = std::chrono::steady_clock::now();

    auto timedOut = [&]() -> bool {
        if (config.timeout_ms == 0) {
            return false;
        }
        auto elapsed
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time)
                  .count();
        return elapsed > static_cast<decltype(elapsed)>(config.timeout_ms);
    };

    std::unordered_set<std::string> visited;
    std::vector<std::string> current_frontier;

    visited.insert(source);
    current_frontier.push_back(source);

    const auto &forbidden = config.forbidden_vertices;

    for (int depth = 0; depth <= config.max_depth; ++depth) {
        if (current_frontier.empty()) {
            break;
        }

        if (timedOut()) {
            result.timed_out = true;
            break;
        }

        for (const auto &node : current_frontier) {
            result.visited.push_back(node);
            ++result.nodes_explored;

            if (config.max_results > 0 && static_cast<int>(result.visited.size()) >= config.max_results) {
                return result;
            }
        }

        if (depth == config.max_depth) {
            break;
        }

        std::vector<std::string> next_frontier;

        const bool use_fan_out_parallel
            = config.fan_out_threshold > 0 && static_cast<int>(current_frontier.size()) >= static_cast<size_t>(config.fan_out_threshold);

        if (use_fan_out_parallel) {
            // Parallel fan-out expansion: split the frontier into chunks and
            // collect neighbors from each chunk concurrently.  Each async task
            // produces a raw list of candidate neighbors; de-duplication against
            // the shared visited set is done serially by the main thread after
            // all tasks complete (no data races).
            const size_t nthreads   = effectiveThreadCount(config,static_cast<int>(current_frontier.size()));
            const size_t chunk_size = (static_cast<int>(current_frontier.size()) + nthreads - 1) / nthreads;

            struct ChunkResult {
                std::vector<std::string> candidates;
            };

            std::vector<std::future<ChunkResult>> futures;
            futures.reserve(nthreads);

            for (size_t t = 0; t < nthreads; ++t) {
                const size_t begin = t * chunk_size;
                if (begin >= static_cast<int>(current_frontier.size())) {
                    break;
                }
                const size_t end = std::min(begin + chunk_size,static_cast<int>(current_frontier.size()));

                futures.push_back(std::async(std::launch::async, [this, &current_frontier, begin, end]() {
                    ChunkResult cr;
                    for (size_t i = begin; i < end; ++i) {
                        auto [status, neighbors] = graph_manager_.outNeighbors(current_frontier[i]);
                        if (!status.ok) {
                            continue;
                        }
                        for (const auto &nb : neighbors) {
                            cr.candidates.push_back(nb);
                        }
                    }
                    return cr;
                }));
            }

            for (auto &fut : futures) {
                auto cr = fut.get();
                for (const auto &nb : cr.candidates) {
                    if (visited.count(nb)) {
                        continue;
                    }
                    if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
                        continue;
                    }
                    visited.insert(nb);
                    next_frontier.push_back(nb);
                    ++result.edges_traversed;
                }
            }
        } else {
            // Sequential frontier expansion (default path for small fan-out).
            for (const auto &node : current_frontier) {
                auto [status, neighbors] = graph_manager_.outNeighbors(node);
                if (!status.ok) {
                    continue;
                }
                for (const auto &nb : neighbors) {
                    if (visited.count(nb)) {
                        continue;
                    }
                    if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
                        continue;
                    }
                    visited.insert(nb);
                    next_frontier.push_back(nb);
                    ++result.edges_traversed;
                }
            }
        }

        current_frontier = std::move(next_frontier);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Single-source DFS (runs inside an async task)
// ---------------------------------------------------------------------------

ParallelTraversal::SourceTraversalResult ParallelTraversal::runSingleDFS(const std::string &source,
                                                                         const Config &config) {
    SourceTraversalResult result;
    result.source = source;

    auto start_time = std::chrono::steady_clock::now();

    auto timedOut = [&]() -> bool {
        if (config.timeout_ms == 0) {
            return false;
        }
        auto elapsed
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time)
                  .count();
        return elapsed > static_cast<decltype(elapsed)>(config.timeout_ms);
    };

    const auto &forbidden = config.forbidden_vertices;

    std::unordered_set<std::string> visited;
    // Stack holds (vertex, depth) pairs for iterative DFS.
    std::vector<std::pair<std::string, int>> stack;
    stack.push_back({source, 0});

    while (!stack.empty()) {
        auto [current, depth] = stack.back();
        stack.pop_back();

        if (visited.count(current)) {
            continue;
        }

        if (timedOut()) {
            result.timed_out = true;
            break;
        }

        visited.insert(current);
        result.visited.push_back(current);
        ++result.nodes_explored;

        if (config.max_results > 0 && static_cast<int>(result.visited.size()) >= config.max_results) {
            break;
        }

        if (depth >= config.max_depth) {
            continue;
        }

        auto [status, neighbors] = graph_manager_.outNeighbors(current);
        if (!status.ok) {
            continue;
        }
        for (const auto &nb : neighbors) {
            if (visited.count(nb)) {
                continue;
            }
            if (std::find(forbidden.begin(), forbidden.end(), nb) != forbidden.end()) {
                continue;
            }
            stack.push_back({nb, depth + 1});
            ++result.edges_traversed;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Merge per-source results
// ---------------------------------------------------------------------------

ParallelTraversal::MultiSourceResult ParallelTraversal::mergeResults(std::vector<SourceTraversalResult> &&per_source,
                                                                     double execution_time_ms) {
    MultiSourceResult merged;
    merged.execution_time_ms = execution_time_ms;

    std::unordered_set<std::string> seen = {};

    for (auto &sr : per_source) {
        merged.total_nodes_explored += sr.nodes_explored;
        merged.total_edges_traversed += sr.edges_traversed;
        if (sr.timed_out) {
            merged.timed_out = true;
        }

        for (const auto &v : sr.visited) {
            if (seen.insert(v).second) {
                merged.visited_vertices.push_back(v);
                merged.vertex_to_source.emplace(v, sr.source);
            }
        }
    }

    return merged;
}

// ---------------------------------------------------------------------------
// Public: multiSourceBFS
// ---------------------------------------------------------------------------

Result<ParallelTraversal::MultiSourceResult>
ParallelTraversal::multiSourceBFS(const std::vector<std::string> &sources) {
    return multiSourceBFS(sources, Config{});
}

Result<ParallelTraversal::MultiSourceResult> ParallelTraversal::multiSourceBFS(const std::vector<std::string> &sources,
                                                                               const Config &config) {
    if (sources.empty()) {
        return Err<MultiSourceResult>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                                      "multiSourceBFS: sources list must not be empty");
    }

    auto wall_start = std::chrono::steady_clock::now();

    const size_t max_concurrent = effectiveThreadCount(config,static_cast<int>(sources.size()));

    std::vector<SourceTraversalResult> per_source = {};

    per_source.reserve(sources.size());

    // Process sources in batches of max_concurrent to cap thread count.
    for (size_t batch_start = 0; batch_start < sources.size();) {
        const size_t batch_end = std::min(batch_start + max_concurrent,static_cast<int>(sources.size()));

        std::vector<std::future<SourceTraversalResult>> futures;
        futures.reserve(batch_end - batch_start);

        for (size_t i = batch_start; i < batch_end; ++i) {
            futures.push_back(std::async(
                std::launch::async, [this, &sources, i, &config]() { return this->runSingleBFS(sources[i], config); }));
        }

        for (auto &fut : futures) {
            per_source.push_back(fut.get());
        }

        batch_start = batch_end;
    }

    auto wall_end        = std::chrono::steady_clock::now();
    const double wall_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return Ok(mergeResults(std::move(per_source), wall_ms));
}

// ---------------------------------------------------------------------------
// Public: multiSourceDFS
// ---------------------------------------------------------------------------

Result<ParallelTraversal::MultiSourceResult>
ParallelTraversal::multiSourceDFS(const std::vector<std::string> &sources) {
    return multiSourceDFS(sources, Config{});
}

Result<ParallelTraversal::MultiSourceResult> ParallelTraversal::multiSourceDFS(const std::vector<std::string> &sources,
                                                                               const Config &config) {
    if (sources.empty()) {
        return Err<MultiSourceResult>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
                                      "multiSourceDFS: sources list must not be empty");
    }

    auto wall_start = std::chrono::steady_clock::now();

    const size_t max_concurrent = effectiveThreadCount(config,static_cast<int>(sources.size()));

    std::vector<SourceTraversalResult> per_source = {};

    per_source.reserve(sources.size());

    for (size_t batch_start = 0; batch_start < sources.size();) {
        const size_t batch_end = std::min(batch_start + max_concurrent,static_cast<int>(sources.size()));

        std::vector<std::future<SourceTraversalResult>> futures;
        futures.reserve(batch_end - batch_start);

        for (size_t i = batch_start; i < batch_end; ++i) {
            futures.push_back(std::async(
                std::launch::async, [this, &sources, i, &config]() { return this->runSingleDFS(sources[i], config); }));
        }

        for (auto &fut : futures) {
            per_source.push_back(fut.get());
        }

        batch_start = batch_end;
    }

    auto wall_end        = std::chrono::steady_clock::now();
    const double wall_ms = std::chrono::duration<double, std::milli>(wall_end - wall_start).count();

    return Ok(mergeResults(std::move(per_source), wall_ms));
}

} // namespace graph
} // namespace themis
