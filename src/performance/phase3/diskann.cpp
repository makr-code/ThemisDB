/**
 * @file diskann.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase3/diskann.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <set>
#include <limits>

namespace themis {
namespace performance {
namespace phase3 {

// ==================== DiskANNIndex Implementation ====================

DiskANNIndex::DiskANNIndex(size_t dimension, const std::string& index_path, size_t cache_size_mb)
    : dimension_(dimension), index_path_(index_path) {
    
    // Calculate cache capacity (number of nodes based on MB)
    size_t node_size_estimate = sizeof(VectorID) + dimension * sizeof(float) + 64 * sizeof(VectorID);
    size_t cache_capacity = (cache_size_mb * 1024 * 1024) / node_size_estimate;
    
    cache_ = std::make_unique<LRUCache<VectorID, DiskANNNode>>(cache_capacity);
    
    // Open or create graph file
    graph_file_ = std::make_unique<std::fstream>(
        index_path_, std::ios::in | std::ios::out | std::ios::binary
    );
    
    if (!graph_file_->is_open()) {
        // Create new file
        graph_file_ = std::make_unique<std::fstream>(
            index_path_, std::ios::out | std::ios::binary
        );
        graph_file_->close();
        graph_file_ = std::make_unique<std::fstream>(
            index_path_, std::ios::in | std::ios::out | std::ios::binary
        );
    }
}

DiskANNIndex::~DiskANNIndex() {
    flush();
    if (graph_file_ && graph_file_->is_open()) {
        graph_file_->close();
    }
}

void DiskANNIndex::build(const std::vector<std::pair<VectorID, std::vector<float>>>& vectors) {
    if (vectors.empty()) {
        return;
    }
    
    // Step 1: Build initial graph using greedy algorithm
    // For simplicity, we'll build a k-NN graph (k=64 neighbors per node)
    const int R = 64;  // Max degree (from DiskANN paper)
    
    // Create nodes with initial empty neighbor lists
    std::vector<DiskANNNode> nodes = {};

    nodes.reserve(vectors.size());
    
    for (const auto& [id, vec] : vectors) {
        DiskANNNode node;
        node.id = id;
        node.vector = vec;
        nodes.push_back(std::move(node));
    }
    
    // Step 2: Build graph by connecting each node to R nearest neighbors
    size_t edge_count = 0;
    for (size_t i = 0; i < nodes.size(); i++) {
        std::priority_queue<std::pair<float, size_t>> nearest;
        
        // Find R nearest neighbors
        for (size_t j = 0; j < nodes.size(); j++) {
            if (i == j) {
              continue;
            }
            
            const float distance = compute_distance(nodes[i].vector, nodes[j].vector);
            
            if (nearest.size() < R) {
                nearest.push({distance, j});
            } else if (distance < nearest.top().first) {
                nearest.pop();
                nearest.push({distance, j});
            }
        }
        
        // Add neighbors
        while (!nearest.empty()) {
            nodes[i].neighbors.push_back(nodes[nearest.top().second].id);
            nearest.pop();
        }
        
        std::reverse(nodes[i].neighbors.begin(), nodes[i].neighbors.end());
        edge_count += nodes[i].neighbors.size();
    }
    total_edges_.store(edge_count, std::memory_order_relaxed);
    
    // Step 3: Save nodes to disk, recording exact byte offsets
    for (auto& node : nodes) {
        // Seek to end to capture the exact write position before appending
        {
            std::lock_guard<std::mutex> lock(file_mutex_);
            graph_file_->seekp(0, std::ios::end);
            vector_offsets_[node.id] = static_cast<uint64_t>(graph_file_->tellp());
        }
        save_node(node);
    }
    
    // Step 4: Build VP-tree for fast entry point selection
    vp_tree_ = std::make_unique<VantagePointTree>(vectors);
    
    flush();
}

void DiskANNIndex::add(VectorID id, const std::vector<float>& vector) {
    if (vector.size() != dimension_) {
        throw std::invalid_argument("Vector dimension mismatch");
    }
    
    // Create new node
    DiskANNNode node;
    node.id = id;
    node.vector = vector;
    
    // Find nearest neighbors from existing nodes
    const int R = 64;
    std::priority_queue<std::pair<float, VectorID>> nearest;
    
    for (const auto& [existing_id, offset] : vector_offsets_) {
        DiskANNNode existing_node = load_node(existing_id);
        float dist = compute_distance(vector, existing_node.vector);
        
        if (nearest.size() < R) {
            nearest.push({dist, existing_id});
        } else if (dist < nearest.top().first) {
            nearest.pop();
            nearest.push({dist, existing_id});
        }
    }
    
    // Add neighbors
    while (!nearest.empty()) {
        node.neighbors.push_back(nearest.top().second);
        nearest.pop();
    }
    
    std::reverse(node.neighbors.begin(), node.neighbors.end());
    total_edges_.fetch_add(node.neighbors.size(), std::memory_order_relaxed);
    
    // Record exact write position before appending
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        graph_file_->seekp(0, std::ios::end);
        vector_offsets_[id] = static_cast<uint64_t>(graph_file_->tellp());
    }
    save_node(node);
}

std::vector<DiskANNIndex::SearchResult> DiskANNIndex::search(
    const std::vector<float>& query, int k, int beam_width) {
    
    if (vector_offsets_.empty()) {
        return {};
    }
    
    // Select entry point using VP-tree if available, otherwise fall back to first vector
    VectorID entry_point = vp_tree_
        ? vp_tree_->find_entry_point(query)
        : vector_offsets_.begin()->first;
    
    // Greedy search from entry point
    auto candidates = greedy_search_internal(query, entry_point, beam_width, k * 2);
    
    // Compute distances and sort
    std::vector<SearchResult> results = {};

    for (VectorID id : candidates) {
        DiskANNNode node = load_node(id);
        float dist = compute_distance(query, node.vector);
        results.push_back({id, dist});
    }
    
    // Sort by distance and return top k
    std::sort(results.begin(), results.end(), 
        [](const SearchResult& a, const SearchResult& b) {
            return a.distance < b.distance;
        });
    
    if (static_cast<int>(results.size()) > static_cast<size_t>(k)) {
        results.resize(k);
    }
    
    return results;
}

DiskANNIndex::Stats DiskANNIndex::get_stats() const {
    Stats stats;
    stats.num_vectors = vector_offsets_.size();
    stats.graph_edges = total_edges_.load(std::memory_order_relaxed);
    stats.cache_hits = cache_hits_.load(std::memory_order_relaxed);
    stats.cache_misses = cache_misses_.load(std::memory_order_relaxed);
    stats.disk_reads = disk_reads_.load(std::memory_order_relaxed);
    return stats;
}

void DiskANNIndex::flush() {
    if (graph_file_) {
        graph_file_->flush();
    }
}

bool DiskANNIndex::save(const std::string& path) const {
    return save_metadata(path + ".meta");
}

bool DiskANNIndex::load(const std::string& path) {
    return load_metadata(path + ".meta");
}

bool DiskANNIndex::save_metadata(const std::string& meta_path) const {
    std::ofstream ofs(meta_path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
      return false;
    }

    // Write dimension (needed to correctly reconstruct the index on load)
    ofs.write(reinterpret_cast<const char*>(&dimension_), sizeof(dimension_));

    // Write edge count
    size_t edges = total_edges_.load(std::memory_order_relaxed);
    ofs.write(reinterpret_cast<const char*>(&edges), sizeof(edges));

    // Write number of offset entries
    size_t n = vector_offsets_.size();
    ofs.write(reinterpret_cast<const char*>(&n), sizeof(n));

    // Write each (VectorID, offset) pair
    for (const auto& [vid, off] : vector_offsets_) {
        ofs.write(reinterpret_cast<const char*>(&vid), sizeof(vid));
        ofs.write(reinterpret_cast<const char*>(&off), sizeof(off));
    }
    return ofs.good();
}

bool DiskANNIndex::load_metadata(const std::string& meta_path) {
    std::ifstream ifs(meta_path, std::ios::binary);
    if (!ifs) {
      return false;
    }

    // Read dimension
    size_t dim = 0;
    ifs.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    if (dim != dimension_) {
        // Stored dimension must match the instance dimension
        return false;
    }

    size_t edges = 0;
    ifs.read(reinterpret_cast<char*>(&edges), sizeof(edges));
    total_edges_.store(edges, std::memory_order_relaxed);

    size_t n = 0;
    ifs.read(reinterpret_cast<char*>(&n), sizeof(n));

    vector_offsets_.clear();
    vector_offsets_.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        VectorID vid = 0;
        uint64_t off = 0;
        ifs.read(reinterpret_cast<char*>(&vid), sizeof(vid));
        ifs.read(reinterpret_cast<char*>(&off), sizeof(off));
        vector_offsets_[vid] = off;
    }
    return ifs.good();
}

DiskANNNode DiskANNIndex::load_node(VectorID id) {
    // Check cache first
    DiskANNNode node = {};
    if (cache_->get(id, node)) {
        cache_hits_.fetch_add(1, std::memory_order_relaxed);
        return node;
    }
    
    cache_misses_.fetch_add(1, std::memory_order_relaxed);
    disk_reads_.fetch_add(1, std::memory_order_relaxed);
    
    // Load from disk
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    auto it = vector_offsets_.find(id);
    if (it == vector_offsets_.end()) {
        throw std::runtime_error("Vector ID not found in index");
    }
    
    graph_file_->seekg(it->second);
    
    // Read node data (simplified serialization)
    node.id = id;
    node.vector.resize(dimension_);
    
    // Read vector data
    graph_file_->read(reinterpret_cast<char*>(node.vector.data()), 
                      dimension_ * sizeof(float));
    
    // Read neighbor count
    uint32_t neighbor_count = {};
    graph_file_->read(reinterpret_cast<char*>(&neighbor_count), sizeof(uint32_t));
    
    // Read neighbors
    node.neighbors.resize(neighbor_count);
    graph_file_->read(reinterpret_cast<char*>(node.neighbors.data()), 
                      neighbor_count * sizeof(VectorID));
    
    // Add to cache
    cache_->put(id, node);
    
    return node;
}

void DiskANNIndex::save_node(const DiskANNNode& node) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    // Seek to end for append
    graph_file_->seekp(0, std::ios::end);
    
    // Write vector data
    graph_file_->write(reinterpret_cast<const char*>(node.vector.data()), 
                       dimension_ * sizeof(float));
    
    // Write neighbor count
    uint32_t neighbor_count = static_cast<uint32_t>(node.neighbors.size());
    graph_file_->write(reinterpret_cast<const char*>(&neighbor_count), sizeof(uint32_t));
    
    // Write neighbors
    graph_file_->write(reinterpret_cast<const char*>(node.neighbors.data()), 
                       neighbor_count * sizeof(VectorID));
}

float DiskANNIndex::compute_distance(const std::vector<float>& a, const std::vector<float>& b) const {
    if (a.size() != b.size()) {
        throw std::invalid_argument("Vector dimensions must match");
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

std::vector<VectorID> DiskANNIndex::greedy_search_internal(
    const std::vector<float>& query,
    VectorID entry_point,
    int beam_width,
    int k) {
    
    // Priority queue for beam search (max heap by distance)
    std::priority_queue<std::pair<float, VectorID>> beam;
    std::set<VectorID> visited;
    
    // Start with entry point
    DiskANNNode start_node = load_node(entry_point);
    float start_dist = compute_distance(query, start_node.vector);
    beam.push({start_dist, entry_point});
    visited.insert(entry_point);
    
    std::vector<std::pair<float, VectorID>> best_candidates;
    
    // Greedy search
    while (!beam.empty() && best_candidates.size() < static_cast<size_t>(k)) {
        auto [dist, current_id] = beam.top();
        beam.pop();
        
        best_candidates.push_back({dist, current_id});
        
        // Explore neighbors
        DiskANNNode current_node = load_node(current_id);
        for (VectorID neighbor_id : current_node.neighbors) {
            if (visited.find(neighbor_id) != visited.end()) {
                continue;
            }
            visited.insert(neighbor_id);
            
            DiskANNNode neighbor_node = load_node(neighbor_id);
            float neighbor_dist = compute_distance(query, neighbor_node.vector);
            
            if (beam.size() < static_cast<size_t>(beam_width)) {
                beam.push({neighbor_dist, neighbor_id});
            } else if (neighbor_dist < beam.top().first) {
                beam.pop();
                beam.push({neighbor_dist, neighbor_id});
            }
        }
    }
    
    // Extract IDs from best candidates
    std::vector<VectorID> result_ids = {};

    for (const auto& [dist, id] : best_candidates) {
        result_ids.push_back(id);
    }
    
    return result_ids;
}

// ==================== VantagePointTree Implementation ====================

VantagePointTree::VantagePointTree(const std::vector<std::pair<VectorID, std::vector<float>>>& vectors) {
    if (!vectors.empty()) {
        std::vector<std::pair<VectorID, std::vector<float>>> vec_copy = vectors;
        root_ = build_tree(vec_copy, 0, vec_copy.size());
    }
}

VectorID VantagePointTree::find_entry_point(const std::vector<float>& query) const {
    if (!root_) {
        throw std::runtime_error("VP-Tree is empty");
    }
    
    Node* current = root_.get();
    float best_dist = std::numeric_limits<float>::max();
    VectorID best_id = current->id;
    
    while (current) {
        float dist = compute_distance(query, current->vector);
        if (dist < best_dist) {
            best_dist = dist;
            best_id = current->id;
        }
        
        if (!current->left && !current->right) {
            break;
        }
        
        // Navigate tree based on threshold
        if (dist < current->threshold && current->left) {
            current = current->left.get();
        } else if (current->right) {
            current = current->right.get();
        } else {
            break;
        }
    }
    
    return best_id;
}

std::unique_ptr<VantagePointTree::Node> VantagePointTree::build_tree(
    std::vector<std::pair<VectorID, std::vector<float>>>& vectors,
    size_t start,
    size_t end) {
    
    if (start >= end) {
        return nullptr;
    }
    
    // Select vantage point (first element for simplicity)
    auto node = std::make_unique<Node>();
    node->id = vectors[start].first;
    node->vector = vectors[start].second;
    
    if (end - start == 1) {
        return node;
    }
    
    // Compute distances from vantage point
    std::vector<float> distances = {};

    for (size_t i = start + 1; i < end; i++) {
        distances.push_back(compute_distance(node->vector, vectors[i].second));
    }
    
    // Find median distance as threshold
    std::vector<float> sorted_dists = distances;
    std::sort(sorted_dists.begin(), sorted_dists.end());
    node->threshold = sorted_dists[sorted_dists.size() / 2];
    
    // Partition vectors
    size_t mid = start + 1;
    for (size_t i = start + 1; i < end; i++) {
        if (compute_distance(node->vector, vectors[i].second) < node->threshold) {
            std::swap(vectors[mid], vectors[i]);
            mid++;
        }
    }
    
    // Build subtrees
    node->left = build_tree(vectors, start + 1, mid);
    node->right = build_tree(vectors, mid, end);
    
    return node;
}

float VantagePointTree::compute_distance(const std::vector<float>& a, const std::vector<float>& b) const {
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

} // namespace phase3
} // namespace performance
} // namespace themis
