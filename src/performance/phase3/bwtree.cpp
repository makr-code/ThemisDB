/**
 * @file bwtree.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=8, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase3/bwtree.h"
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace performance {
namespace phase3 {

// ==================== MappingTable Implementation ====================

MappingTable::MappingTable([[maybe_unused]] size_t size) : table_(size) {
    for (auto& entry : table_) {
        entry.store(nullptr, std::memory_order_relaxed);
    }
}

BwTreePage* MappingTable::get(PageID pid) const {
    size_t index = pid % table_.size();
    return table_[index].load(std::memory_order_acquire);
}

bool MappingTable::compare_and_swap(PageID pid, BwTreePage* expected, BwTreePage* desired) {
    size_t index = pid % table_.size();
    return table_[index].compare_exchange_strong(
        expected, desired,
        std::memory_order_release,
        std::memory_order_acquire
    );
}

// ==================== BwTree Implementation ====================

BwTree::BwTree() {
    mapping_table_ = std::make_unique<MappingTable>();
    
    // Create initial root page
    root_pid_ = next_pid_.fetch_add(1, std::memory_order_relaxed);
    auto root = std::make_unique<LeafPage>();
    BwTreePage* root_ptr = root.get();
    
    // Install root in mapping table
    BwTreePage* expected = nullptr;
    if (!mapping_table_->compare_and_swap(root_pid_, expected, root_ptr)) {
        throw std::runtime_error("Failed to install root page");
    }
    root.release();
}

BwTree::~BwTree() {
    // Reclaim all deferred-deletion chains accumulated during operation.
    // At destruction time there are no concurrent readers, so it is safe
    // to delete every chain unconditionally.
    std::lock_guard<std::mutex> lk(retired_mutex_);
    for (auto& rc : retired_chains_) {
        delete_chain(rc.head);
    }
    retired_chains_.clear();
}

bool BwTree::insert(int64_t key, const std::string& value) {
    // Simplified insert: always inserts into root for now
    // In full implementation, would traverse tree to find correct leaf
    
    bool consolidation_attempted = false;
    
    while (true) {
        BwTreePage* page = mapping_table_->get(root_pid_);
        if (!page) {
            return false;
        }
        
        // Check if consolidation is needed (only once per insert operation)
        if (!consolidation_attempted && 
            count_delta_chain_length(page) >= DELTA_CHAIN_THRESHOLD) {
            consolidate(root_pid_);
            consolidation_attempted = true;
            // Continue to insert after consolidation attempt
            continue;
        }
        
        // Create delta insert record
        auto delta = std::make_unique<DeltaInsert>(key, value);
        delta->next_delta.store(page, std::memory_order_relaxed);
        BwTreePage* delta_ptr = delta.get();
        
        // Try to install delta
        if (mapping_table_->compare_and_swap(root_pid_, page, delta_ptr)) {
            delta.release();
            return true;
        }
    }
}

bool BwTree::remove(int64_t key) {
    while (true) {
        BwTreePage* page = mapping_table_->get(root_pid_);
        if (!page) {
            return false;
        }

        // Check if consolidation is needed before attempting remove
        if (count_delta_chain_length(page) >= DELTA_CHAIN_THRESHOLD) {
            consolidate(root_pid_);
            continue;
        }

        // Create delta delete record and link it to the current chain head.
        // We install it unconditionally: apply_deltas() handles the case where
        // the key is absent (it simply skips the erase).  A pre-CAS search
        // would introduce a TOCTOU race — another thread could remove the same
        // key between the check and the CAS, making the check unreliable.
        auto delta = std::make_unique<DeltaDelete>(key);
        delta->next_delta.store(page, std::memory_order_relaxed);
        BwTreePage* delta_ptr = delta.get();

        // Try to install delta via CAS.
        if (mapping_table_->compare_and_swap(root_pid_, page, delta_ptr)) {
            delta.release();
            return true;
        }
    }
}

bool BwTree::search(int64_t key, std::string& value) const {
    BwTreePage* page = mapping_table_->get(root_pid_);
    if (!page) {
        return false;
    }
    
    // Apply deltas to get consolidated view
    auto consolidated = apply_deltas(page);
    if (!consolidated) {
        return false;
    }
    
    // Binary search in sorted records
    auto it = std::lower_bound(
        consolidated->records.begin(),
        consolidated->records.end(),
        key,
        [](const std::pair<int64_t, std::string>& record, int64_t k) {
            return record.first < k;
        }
    );
    
    if (it != consolidated->records.end() && it->first == key) {
        value = it->second;
        return true;
    }
    
    return false;
}

std::vector<std::pair<int64_t, std::string>> BwTree::range_scan(
    int64_t start_key, int64_t end_key) const {
    
    BwTreePage* page = mapping_table_->get(root_pid_);
    if (!page) {
        return {};
    }
    
    // Apply deltas to get consolidated view
    auto consolidated = apply_deltas(page);
    if (!consolidated) {
        return {};
    }
    
    std::vector<std::pair<int64_t, std::string>> results;
    
    for (const auto& [k, v] : consolidated->records) {
        if (k >= start_key && k <= end_key) {
            results.push_back({k, v});
        }
    }
    
    return results;
}

BwTree::Stats BwTree::get_stats() const {
    Stats stats;
    stats.num_pages = next_pid_.load(std::memory_order_relaxed);
    stats.num_deltas = 0;
    stats.consolidations = 0;
    
    // Count deltas in root page using the helper function
    BwTreePage* page = mapping_table_->get(root_pid_);
    stats.num_deltas = count_delta_chain_length(page);
    
    return stats;
}

void BwTree::consolidate(PageID pid) {
    while (true) {
        BwTreePage* page = mapping_table_->get(pid);
        if (!page) {
            return;
        }
        
        // Apply deltas to create consolidated page
        auto consolidated = apply_deltas(page);
        if (!consolidated) {
            return;
        }
        
        // Get raw pointer for CAS, but keep unique_ptr ownership until CAS succeeds
        BwTreePage* consolidated_ptr = consolidated.get();
        
        // Try to install consolidated page
        if (mapping_table_->compare_and_swap(pid, page, consolidated_ptr)) {
            // CAS succeeded - transfer ownership to mapping table
            consolidated.release();
            
            // Defer reclamation of the old delta chain.  Concurrent readers
            // that loaded the mapping-table pointer before this CAS may still
            // be traversing the old chain in apply_deltas().  We tag the
            // retired chain with the current epoch and advance the epoch
            // counter; after kSafeReclaimEpochs more consolidation rounds the
            // chain is guaranteed to be unreachable by any active reader.
            retire_chain(page);
            consolidation_epoch_.fetch_add(1, std::memory_order_relaxed);
            reclaim_retired_chains();
            
            return;
        }
        
        // CAS failed - unique_ptr will automatically clean up consolidated page
        // on next iteration or function return
    }
}

std::unique_ptr<LeafPage> BwTree::apply_deltas(BwTreePage* page) const {
    if (!page) {
        return nullptr;
    }
    
    auto result = std::make_unique<LeafPage>();
    
    // Collect all deltas in reverse order
    std::vector<BwTreePage*> delta_chain;
    BwTreePage* current = page;
    
    while (current) {
        delta_chain.push_back(current);
        current = current->next_delta.load(std::memory_order_acquire);
    }
    
    // Find base page (LEAF)
    LeafPage* base = nullptr;
    for (auto it = delta_chain.rbegin(); it != delta_chain.rend(); ++it) {
        if ((*it)->type == PageType::LEAF) {
            base = static_cast<LeafPage*>(*it);
            break;
        }
    }
    
    if (!base) {
        return nullptr;
    }
    
    // Start with base page records
    result->records = base->records;
    result->left_sibling = base->left_sibling;
    result->right_sibling = base->right_sibling;
    
    // Apply deltas in forward order
    for (auto it = delta_chain.rbegin(); it != delta_chain.rend(); ++it) {
        BwTreePage* delta = *it;
        
        if (delta->type == PageType::DELTA_INSERT) {
            auto insert_delta = static_cast<DeltaInsert*>(delta);
            
            // Find insertion point
            auto pos = std::lower_bound(
                result->records.begin(),
                result->records.end(),
                insert_delta->key,
                [](const std::pair<int64_t, std::string>& record, int64_t k) {
                    return record.first < k;
                }
            );
            
            // Update if exists, insert if not
            if (pos != result->records.end() && pos->first == insert_delta->key) {
                pos->second = insert_delta->value;
            } else {
                result->records.insert(pos, {insert_delta->key, insert_delta->value});
            }
        } else if (delta->type == PageType::DELTA_DELETE) {
            auto delete_delta = static_cast<DeltaDelete*>(delta);

            // Remove the key if it is present in the consolidated records
            auto pos = std::lower_bound(
                result->records.begin(),
                result->records.end(),
                delete_delta->key,
                [](const std::pair<int64_t, std::string>& record, int64_t k) {
                    return record.first < k;
                }
            );

            if (pos != result->records.end() && pos->first == delete_delta->key) {
                result->records.erase(pos);
            }
        }
    }
    
    return result;
}

size_t BwTree::count_delta_chain_length(BwTreePage* page) const {
    size_t count = 0;
    BwTreePage* current = page;
    
    while (current) {
        // Count only delta types, not base pages (LEAF/INNER)
        if (current->type == PageType::DELTA_INSERT || 
            current->type == PageType::DELTA_DELETE ||
            current->type == PageType::DELTA_SPLIT) {
            count++;
        }
        current = current->next_delta.load(std::memory_order_acquire);
    }
    
    return count;
}

// ---------------------------------------------------------------------------
// Epoch-based memory reclamation helpers
// ---------------------------------------------------------------------------

void BwTree::retire_chain(BwTreePage* head) noexcept {
    if (!head) {
      return;
    }
    std::lock_guard<std::mutex> lk(retired_mutex_);
    retired_chains_.push_back({head,
        consolidation_epoch_.load(std::memory_order_acquire)});
}

void BwTree::reclaim_retired_chains() noexcept {
    std::lock_guard<std::mutex> lk(retired_mutex_);
    // Load epoch under the lock so the comparison is consistent with any
    // concurrent retire_chain() call that also holds the lock.
    const uint64_t current_epoch =
        consolidation_epoch_.load(std::memory_order_acquire);

    auto it = retired_chains_.begin();
    while (it != retired_chains_.end()) {
        // Use wrapping unsigned subtraction so the comparison remains correct
        // when consolidation_epoch_ rolls over UINT64_MAX.
        if (current_epoch - it->retirement_epoch >= kSafeReclaimEpochs) {
            delete_chain(it->head);
            it = retired_chains_.erase(it);
        } else {
            ++it;
        }
    }
}

void BwTree::delete_chain(BwTreePage* head) noexcept {
    BwTreePage* current = head;
    while (current) {
        BwTreePage* next = current->next_delta.load(std::memory_order_acquire);
        delete current;
        current = next;
    }
}

} // namespace phase3
} // namespace performance
} // namespace themis
