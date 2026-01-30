// Bw-Tree: A Lock-Free B-tree for Multi-core Systems
// Paper: "The Bw-Tree: A B-tree for New Hardware Platforms" (ICDE'13)
// Authors: Justin Levandoski et al., Microsoft Research
//
// Key idea: Lock-free index using mapping table and delta updates
// Expected gain: +100-200% index update throughput
// Reference: https://www.microsoft.com/en-us/research/publication/the-bw-tree-a-b-tree-for-new-hardware-platforms/

#pragma once

#include <atomic>
#include <vector>
#include <memory>
#include <cstdint>
#include <string>

namespace themis {
namespace performance {
namespace phase3 {

using PageID = uint64_t;

/// Bw-Tree page types
enum class PageType {
    LEAF,
    INNER,
    DELTA_INSERT,
    DELTA_DELETE,
    DELTA_SPLIT
};

/// Base page structure
struct BwTreePage {
    PageType type;
    std::atomic<BwTreePage*> next_delta{nullptr};  // Delta chain
    
    virtual ~BwTreePage() = default;
};

/// Leaf page
struct LeafPage : public BwTreePage {
    std::vector<std::pair<int64_t, std::string>> records;  // key-value pairs
    PageID left_sibling{0};
    PageID right_sibling{0};
    
    LeafPage() { type = PageType::LEAF; }
};

/// Delta record for insert
struct DeltaInsert : public BwTreePage {
    int64_t key;
    std::string value;
    
    DeltaInsert(int64_t k, const std::string& v) : key(k), value(v) {
        type = PageType::DELTA_INSERT;
    }
};

/// Mapping table (lock-free hash table)
class MappingTable {
public:
    MappingTable(size_t size = 10000);
    
    // Atomic get/set operations
    BwTreePage* get(PageID pid) const;
    bool compare_and_swap(PageID pid, BwTreePage* expected, BwTreePage* desired);
    
private:
    std::vector<std::atomic<BwTreePage*>> table_;
};

/// Bw-Tree lock-free index
class BwTree {
public:
    BwTree();
    ~BwTree();
    
    // Lock-free operations
    bool insert(int64_t key, const std::string& value);
    bool remove(int64_t key);
    bool search(int64_t key, std::string& value) const;
    
    // Range scan
    std::vector<std::pair<int64_t, std::string>> range_scan(int64_t start_key, int64_t end_key) const;
    
    // Statistics
    struct Stats {
        size_t num_pages;
        size_t num_deltas;
        size_t consolidations;
    };
    Stats get_stats() const;

private:
    std::unique_ptr<MappingTable> mapping_table_;
    PageID root_pid_;
    std::atomic<PageID> next_pid_{1};
    
    // Delta consolidation
    void consolidate(PageID pid);
    
    // Helper: Apply deltas to get consolidated page
    std::unique_ptr<LeafPage> apply_deltas(BwTreePage* page) const;
};

} // namespace phase3
} // namespace performance
} // namespace themis
