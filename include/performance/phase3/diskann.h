/**
 * @file diskann.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node
// Paper: "DiskANN: Fast Accurate Billion-point Nearest Neighbor Search on a Single Node" (NeurIPS'19)
// Authors: Suhas Jayaram Subramanya et al., Microsoft Research
//
// Key idea: SSD-optimized graph-based index for billion-scale vector search
// Expected gain: +300-400% throughput for >100M vectors
// Reference: https://papers.nips.cc/paper/2019/hash/09853c7fb1d3f8ee67a61b6bf4a7f8e6-Abstract.html

#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <atomic>

namespace themis {
namespace performance {
namespace phase3 {

using VectorID = uint64_t;

/// DiskANN graph node (stored on SSD)
struct DiskANNNode {
    VectorID id;
    std::vector<float> vector;  // Could be stored separately on SSD
    std::vector<VectorID> neighbors;  // Graph edges
    
    // For SSD optimization: sector-aligned I/O
    static constexpr size_t SECTOR_SIZE = 4096;
};

/// LRU Cache for hot vectors
template<typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}
    
    bool get(const Key& key, Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }
    
    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_[key] = value;
        if (cache_.size() > capacity_) {
            // Simple eviction: remove first element (not true LRU but simpler)
            cache_.erase(cache_.begin());
        }
    }
    
    size_t size() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size(); 
    }

private:
    size_t capacity_;
    std::unordered_map<Key, Value> cache_;
    mutable std::mutex mutex_;
};

// Forward declaration required because DiskANNIndex holds a unique_ptr<VantagePointTree>
class VantagePointTree;

/// DiskANN Index for billion-scale vector search
class DiskANNIndex {
public:
    DiskANNIndex(size_t dimension, const std::string& index_path, size_t cache_size_mb = 1024);
    ~DiskANNIndex();
    
    // Build index from vectors
    void build(const std::vector<std::pair<VectorID, std::vector<float>>>& vectors);
    
    // Add vector to existing index
    void add(VectorID id, const std::vector<float>& vector);
    
    // Greedy search on disk-resident graph
    struct SearchResult {
        VectorID id;
        float distance;
    };
    std::vector<SearchResult> search(const std::vector<float>& query, int k, int beam_width = 64);
    
    // Get statistics
    struct Stats {
        size_t num_vectors = 0;
        size_t graph_edges;
        size_t cache_hits;
        size_t cache_misses;
        size_t disk_reads;
    };
    Stats get_stats() const;
    
    // Flush graph file and save metadata sidecar (call after build/add)
    void flush();

    // Persist metadata (vector_offsets_, edge count) to a sidecar file.
    // Returns true on success.  The sidecar path is index_path + ".meta".
    bool save(const std::string& path) const;

    // Reload metadata from a previously saved sidecar file.
    // Returns true on success.
    bool load(const std::string& path);

private:
    size_t dimension_;
    std::string index_path_;
    
    // In-memory components
    std::unique_ptr<LRUCache<VectorID, DiskANNNode>> cache_;

    // VP-tree for fast entry point selection (built during build())
    std::unique_ptr<VantagePointTree> vp_tree_;
    
    // Metadata (kept in memory)
    std::unordered_map<VectorID, uint64_t> vector_offsets_;  // VectorID -> file offset
    
    // Statistics
    mutable std::atomic<size_t> cache_hits_{0};
    mutable std::atomic<size_t> cache_misses_{0};
    mutable std::atomic<size_t> disk_reads_{0};
    std::atomic<size_t> total_edges_{0};
    
    // File handle for SSD-resident graph
    std::unique_ptr<std::fstream> graph_file_;
    mutable std::mutex file_mutex_;
    
    // Helper: Load node from disk
    DiskANNNode load_node(VectorID id);
    
    // Helper: Save node to disk
    void save_node(const DiskANNNode& node);
    
    // Helper: Compute L2 distance
    float compute_distance(const std::vector<float>& a, const std::vector<float>& b) const;
    
    // Helper: Greedy search from entry point
    std::vector<VectorID> greedy_search_internal(
        const std::vector<float>& query,
        VectorID entry_point,
        int beam_width,
        int k
    );

    // Helper: Persist/reload vector_offsets_ and edge count to a sidecar file
    bool save_metadata(const std::string& meta_path) const;
    bool load_metadata(const std::string& meta_path);
};

/// Vantage Point Tree for entry point selection
class VantagePointTree {
public:
    VantagePointTree(const std::vector<std::pair<VectorID, std::vector<float>>>& vectors);
    
    // Find best entry point for query
    VectorID find_entry_point(const std::vector<float>& query) const;

private:
    struct Node {
        VectorID id;
        std::vector<float> vector;
        float threshold;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
    };
    
    std::unique_ptr<Node> root_;
    
    std::unique_ptr<Node> build_tree(
        std::vector<std::pair<VectorID, std::vector<float>>>& vectors,
        size_t start,
        size_t end
    );
    
    float compute_distance(const std::vector<float>& a, const std::vector<float>& b) const;
};

} // namespace phase3
} // namespace performance
} // namespace themis
