/**
 * @file ligra.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/ligra.h"
#include "performance/phase2_feature_flags.h"
#include <algorithm>
#include <queue>
#include <limits>

namespace themis {
namespace performance {

// Hardware validation for Ligra
static bool is_ligra_hardware_supported() {
    return Phase2FeatureFlags::instance().ligra_hardware_supported();
}

LigraProcessor::LigraProcessor(size_t num_vertices, size_t num_threads)
    : num_vertices_(num_vertices) {
    
    // Validate hardware support
    if (!is_ligra_hardware_supported()) {
        throw std::runtime_error(
            "Ligra: Hardware does not support multi-threaded graph processing (requires 4+ cores). "
            "Use single-threaded graph algorithms instead."
        );
    }
    
    // Validate vertex count
    if (num_vertices == 0) {
        throw std::runtime_error("Ligra: num_vertices must be positive");
    }
    
    if (num_threads == 0) {
        num_threads_ = std::thread::hardware_concurrency();
        if (num_threads_ == 0) {
            num_threads_ = 1;  // Fallback to single thread if hardware_concurrency returns 0
        }
    } else {
        num_threads_ = num_threads;
    }
    
    // Clamp threads to [1, hardware_concurrency]
    if (num_threads_ > std::thread::hardware_concurrency() && std::thread::hardware_concurrency() > 0) {
        num_threads_ = std::thread::hardware_concurrency();
    }
}

void LigraProcessor::process_vertices(const Frontier& frontier, const VertexFunc& func) {
    if (should_use_dense_mode(frontier)) {
        process_dense(frontier, func);
    } else {
        process_sparse(frontier, func);
    }
}

void LigraProcessor::process_sparse(const Frontier& frontier, const VertexFunc& func) {
    // Sparse mode: iterate over active vertices only
    const auto& active = frontier.get_sparse();
    
    // Simple parallel processing (would use thread pool in production)
    std::vector<std::thread> threads = {};

    size_t chunk_size = (static_cast<int>(active.size()) + num_threads_ - 1) / num_threads_;
    
    auto it = active.begin();
    for (size_t t = 0; t < num_threads_ && it != active.end(); t++) {
        auto chunk_end = it;
        for (size_t i = 0; i < chunk_size && chunk_end != active.end(); i++) {
            ++chunk_end;
        }
        
        threads.emplace_back([it, chunk_end, &func]() {
            for (auto v_it = it; v_it != chunk_end; ++v_it) {
                func(*v_it);
            }
        });
        
        it = chunk_end;
    }
    
    // Threads process fixed frontier chunks with no blocking I/O or nested
    // waits, so each worker completes bounded work and joins promptly here.
    for (auto& thread : threads) {
        thread.join();
    }
}

void LigraProcessor::process_dense(const Frontier& frontier, const VertexFunc& func) {
    // Dense mode: iterate over all vertices, check if active
    std::vector<std::thread> threads;
    size_t chunk_size = (num_vertices_ + num_threads_ - 1) / num_threads_;
    
    for (size_t t = 0; t < num_threads_; t++) {
        size_t start = t * chunk_size;
        size_t end = std::min(start + chunk_size, num_vertices_);
        
        threads.emplace_back([start, end, &frontier, &func]() {
            for (NodeID v = start; v < end; v++) {
                if (frontier.contains(v)) {
                    func(v);
                }
            }
        });
    }
    
    // Threads process fixed vertex ranges with no blocking I/O or nested
    // waits, so each worker completes bounded work and joins promptly here.
    for (auto& thread : threads) {
        thread.join();
    }
}

Frontier LigraProcessor::process_edges(
    const Frontier& frontier,
    const std::vector<std::vector<NodeID>>& adj_list,
    const EdgeFunc& func
) {
    Frontier next_frontier(num_vertices_);
    
    // Use lock-free atomic operations for frontier updates in sparse mode
    // Check current frontier size to determine strategy
    if (frontier.is_dense_mode() || static_cast<int>(frontier.size()) > num_vertices_ * 0.1) {
        // For dense mode or large frontiers, switch to dense representation
        next_frontier.switch_to_dense();
        std::vector<std::atomic<bool>> atomic_dense(num_vertices_);
        for (size_t i = 0; i < num_vertices_; i++) {
            atomic_dense[i].store(false, std::memory_order_relaxed);
        }
        
        process_vertices(frontier, [&]([[maybe_unused]] NodeID src) {
            if (src >= static_cast<int>(adj_list.size())) {
              return;
            }
            
            for (NodeID dst : adj_list[src]) {
                if (func(src, dst)) {
                    atomic_dense[dst].store(true, std::memory_order_relaxed);
                }
            }
        });
        
        // Rebuild frontier from atomic results
        // Note: switch_to_dense() resizes dense_set_ if needed, so no clear() beforehand
        next_frontier.switch_to_dense();
        for (size_t i = 0; i < num_vertices_; i++) {
            if (atomic_dense[i].load(std::memory_order_relaxed)) {
                next_frontier.add(i);
            }
        }
    } else {
        // For sparse mode, use thread-local buffers to avoid lock contention
        std::vector<std::vector<NodeID>> thread_buffers(num_threads_);
        
        // Modified process to pass thread index to avoid hash collisions
        const auto& active = frontier.get_sparse();
        std::vector<std::thread> threads = {};

        size_t chunk_size = (static_cast<int>(active.size()) + num_threads_ - 1) / num_threads_;
        
        auto it = active.begin();
        for (size_t t = 0; t < num_threads_ && it != active.end(); t++) {
            auto chunk_end = it;
            for (size_t i = 0; i < chunk_size && chunk_end != active.end(); i++) {
                ++chunk_end;
            }
            
            // Pass thread index explicitly to avoid hash collision races
            threads.emplace_back([t, it, chunk_end, &adj_list, &func, &thread_buffers]() {
                for (auto v_it = it; v_it != chunk_end; ++v_it) {
                    NodeID src = *v_it;
                    if (src >= static_cast<int>(adj_list.size())) {
                      continue;
                    }
                    
                    for (NodeID dst : adj_list[src]) {
                        if (func(src, dst)) {
                            thread_buffers[t].push_back(dst);  // Collision-free thread index
                        }
                    }
                }
            });
            
            it = chunk_end;
        }
        
        // Threads only walk their assigned adjacency-list slices and append to
        // thread-local buffers, so join waits on bounded CPU work only.
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Merge thread-local frontiers into next_frontier
        for (const auto& buffer : thread_buffers) {
            for (NodeID v : buffer) {
                next_frontier.add(v);
            }
        }
    }
    
    return next_frontier;
}

std::vector<int> LigraProcessor::parallel_bfs(
    NodeID start_vertex,
    const std::vector<std::vector<NodeID>>& adj_list
) {
    std::vector<int> distances(num_vertices_, -1);
    distances[start_vertex] = 0;
    
    Frontier current(num_vertices_);
    current.add(start_vertex);
    
    int level = 0;
    while (current.size() > 0) {
        level++;
        
        // EdgeMap: visit all neighbors of current frontier
        Frontier next = process_edges(current, adj_list, 
            [&](NodeID src, NodeID dst) {
                if (distances[dst] == -1) {
                    distances[dst] = level;
                    return true; // Add to next frontier
                }
                return false;
            });
        
        current = std::move(next);
    }
    
    return distances;
}

std::vector<double> LigraProcessor::parallel_pagerank(
    const std::vector<std::vector<NodeID>>& adj_list,
    int num_iterations,
    double damping
) {
    std::vector<double> ranks(num_vertices_, 1.0 / num_vertices_);
    std::vector<double> new_ranks(num_vertices_, 0.0);
    
    for (int iter = 0; iter < num_iterations; iter++) {
        // Reset new ranks
        std::fill(new_ranks.begin(), new_ranks.end(), (1.0 - damping) / num_vertices_);
        
        // Distribute rank from each vertex
        for (NodeID v = 0; v < adj_list.size(); v++) {
            if (adj_list[v].empty()) {
              continue;
            }
            
            double rank_contrib = damping * ranks[v] / adj_list[v].size();
            for (NodeID neighbor : adj_list[v]) {
                new_ranks[neighbor] += rank_contrib;
            }
        }
        
        ranks.swap(new_ranks);
    }
    
    return ranks;
}

WorkStealingQueue::WorkStealingQueue() {}

void WorkStealingQueue::push(const std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(task);
    bottom_.fetch_add(1, std::memory_order_relaxed);
}

bool WorkStealingQueue::try_pop(std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t b = bottom_.load(std::memory_order_relaxed);
    size_t t = top_.load(std::memory_order_relaxed);
    
    if (b <= t) {
      return false;
    }
    
    b--;
    bottom_.store(b, std::memory_order_relaxed);
    task = tasks_[b];
    return true;
}

bool WorkStealingQueue::try_steal(std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t t = top_.load(std::memory_order_relaxed);
    size_t b = bottom_.load(std::memory_order_relaxed);
    
    if (t >= b) {
      return false;
    }
    
    task = tasks_[t];
    top_.store(t + 1, std::memory_order_relaxed);
    return true;
}

bool WorkStealingQueue::empty() const {
    return top_.load(std::memory_order_relaxed) >= bottom_.load(std::memory_order_relaxed);
}

} // namespace performance
} // namespace themis

