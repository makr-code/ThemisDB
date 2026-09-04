/**
 * @file bwtree.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Bw-Tree: A Lock-Free B-tree for Multi-core Systems
// Paper: "The Bw-Tree: A B-tree for New Hardware Platforms" (ICDE'13)
// Authors: Justin Levandoski et al., Microsoft Research
//
// Key idea: Lock-free index using mapping table and delta updates
// Expected gain: +100-200% index update throughput
// Reference: https://www.microsoft.com/en-us/research/publication/the-bw-tree-a-b-tree-for-new-hardware-platforms/

#pragma once

#include <atomic>
#include <mutex>
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
    std::string value = {};
    
    DeltaInsert(int64_t k, const std::string& v) : key(k), value(v) {
        type = PageType::DELTA_INSERT;
    }
};

/// Delta record for delete
struct DeltaDelete : public BwTreePage {
    int64_t key;

    explicit DeltaDelete(int64_t k) : key(k) {
        type = PageType::DELTA_DELETE;
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
        size_t num_pages = 0;
        size_t num_deltas;
        size_t consolidations;
    };
    Stats get_stats() const;

private:
    std::unique_ptr<MappingTable> mapping_table_;
    PageID root_pid_;
    std::atomic<PageID> next_pid_{1};
    
    // Delta consolidation threshold
    static constexpr size_t DELTA_CHAIN_THRESHOLD = 10;
    
    // Delta consolidation
    void consolidate(PageID pid);
    
    // Helper: Apply deltas to get consolidated page
    std::unique_ptr<LeafPage> apply_deltas(BwTreePage* page) const;
    
    // Helper: Count delta chain length
    size_t count_delta_chain_length(BwTreePage* page) const;

    // -----------------------------------------------------------------------
    // Epoch-based memory reclamation for retired delta chains
    //
    // After a successful CAS in consolidate(), the old chain head is still
    // reachable from in-flight readers that loaded the mapping-table pointer
    // before the CAS.  We defer deletion until at least kSafeReclaimEpochs
    // consolidation rounds have elapsed, giving those readers time to finish
    // their apply_deltas() traversal (which is bounded by
    // DELTA_CHAIN_THRESHOLD nodes and completes in O(ns)).
    // -----------------------------------------------------------------------

    /// One entry in the deferred-deletion list.
    struct RetiredChain {
        BwTreePage* head;              ///< Head of the retired chain
        uint64_t    retirement_epoch;  ///< Value of consolidation_epoch_ at retirement
    };

    /// Number of additional consolidation epochs a retired chain must survive
    /// before it is considered safe to reclaim.  Three epochs provide a very
    /// conservative window: at the threshold of 10 deltas per chain, three
    /// more consolidation cycles mean ≥30 additional insert operations
    /// between retirement and reclamation.
    static constexpr uint64_t kSafeReclaimEpochs = 3;

    std::atomic<uint64_t>    consolidation_epoch_{0};
    std::mutex               retired_mutex_;
    std::vector<RetiredChain> retired_chains_;

    /// Push @p head onto the deferred-deletion list, tagged with the
    /// current consolidation epoch.
    void retire_chain(BwTreePage* head) noexcept;

    /// Walk the retired-chain list and delete chains whose retirement epoch
    /// satisfies (current_epoch - retirement_epoch) >= kSafeReclaimEpochs.
    /// Uses wrapping unsigned subtraction so the epoch counter can roll over
    /// UINT64_MAX without triggering premature reclamation.
    void reclaim_retired_chains() noexcept;

    /// Delete an entire delta chain starting at @p head.
    static void delete_chain(BwTreePage* head) noexcept;
};

} // namespace phase3
} // namespace performance
} // namespace themis
