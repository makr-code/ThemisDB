/**
 * @file lirs_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <unordered_map>
#include <list>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace themis {
namespace performance {

/**
 * LIRS (Low Inter-reference Recency Set) Cache Implementation
 * 
 * Based on: "LIRS: An Efficient Low Inter-reference Recency Set 
 * Replacement Policy to Improve Buffer Cache Performance" (SIGMETRICS'02)
 * 
 * LIRS tracks inter-reference recency (IRR) instead of just recency,
 * maintaining separate LIR (low IRR, hot) and HIR (high IRR, cold) sets.
 * 
 * Key advantages over LRU:
 * - Scan resistant: One-time sequential access doesn't evict hot data
 * - Better hit rates: +30-40% improvement for database workloads
 * - Low overhead: O(1) operations with efficient stack management
 * 
 * Usage:
 *   LIRSCache<int, std::string> cache(1000, 0.9);
 *   cache.put(key, value);
 *   std::string value;
 *   if (cache.get(key, value)) { ... }
 */
template<typename Key, typename Value>
class LIRSCache {
public:
    /**
     * Constructor
     * @param capacity Total cache capacity
     * @param lir_ratio Ratio of LIR entries (typically 0.9 for 90% LIR)
     */
    LIRSCache(size_t capacity, double lir_ratio = 0.9)
        : capacity_(capacity)
        , lir_size_(static_cast<size_t>(capacity * lir_ratio))
        , hir_size_(capacity - lir_size_)
        , hits_(0)
        , misses_(0)
    {
        if (lir_size_ < 1) lir_size_ = 1;
        if (hir_size_ < 1) hir_size_ = 1;
    }

    /**
     * Get value from cache
     * @param key Key to lookup
     * @param value Output parameter for value
     * @return true if found (hit), false if not found (miss)
     */
    bool get(const Key& key, Value& value) {
        // Use a single unique_lock for the whole get() operation.
        //
        // A two-phase shared→unique approach (shared for lookup, then upgrade to
        // unique for access()) creates a TOCTOU race: the key can be evicted in the
        // window between releasing the shared lock and acquiring the unique lock, so
        // access() would silently be skipped and the caller would hold a stale value
        // with no access-pattern update.
        //
        // With a single exclusive lock the trade-off is slightly reduced read
        // concurrency; for workloads where reads dominate over writes the bottleneck
        // is typically memory bandwidth rather than lock contention at cache sizes
        // below ~10 M entries.  Large-scale deployments should use sharded instances.
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = map_.find(key);
        if (it == map_.end()) {
            misses_++;
            return false;
        }

        hits_++;
        value = it->second.value;
        access(key);
        return true;
    }

    /**
     * Put key-value pair into cache
     * @param key Key
     * @param value Value
     */
    void put(const Key& key, const Value& value) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing entry
            it->second.value = value;
            access(key);
            return;
        }
        
        // New entry
        if (map_.size() >= capacity_) {
            evict();
        }
        
        // Add as HIR initially (will promote to LIR if accessed frequently)
        Entry entry;
        entry.value = value;
        entry.is_lir = false;
        entry.in_stack = true;
        
        // Add to stack top
        stack_.push_front(key);
        entry.stack_iter = stack_.begin();
        
        // Add to HIR list
        hir_list_.push_front(key);
        entry.hir_iter = hir_list_.begin();
        
        map_[key] = std::move(entry);
        
        // Try to promote to LIR if we have space
        if (lir_count_ < lir_size_) {
            promote_to_lir(key);
        }
    }

    /**
     * Check if key exists in cache
     */
    bool contains(const Key& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return map_.find(key) != map_.end();
    }

    /**
     * Get current cache size
     */
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return map_.size();
    }

    /**
     * Get cache capacity
     */
    size_t capacity() const {
        return capacity_;
    }

    /**
     * Get hit count
     */
    size_t get_hits() const {
        return hits_.load();
    }

    /**
     * Get miss count
     */
    size_t get_misses() const {
        return misses_.load();
    }

    /**
     * Get hit rate (0.0 to 1.0)
     */
    double get_hit_rate() const {
        size_t total = hits_.load() + misses_.load();
        if (total == 0) return 0.0;
        return static_cast<double>(hits_.load()) / total;
    }

    /**
     * Clear cache and reset statistics
     */
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        map_.clear();
        stack_.clear();
        hir_list_.clear();
        lir_count_ = 0;
        hits_ = 0;
        misses_ = 0;
    }

    /**
     * Get LIR count
     */
    size_t get_lir_count() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return lir_count_;
    }

    /**
     * Get HIR count
     */
    size_t get_hir_count() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return map_.size() - lir_count_;
    }

private:
    struct Entry {
        Value value;
        bool is_lir;                          // LIR (hot) or HIR (cold)
        bool in_stack;                        // In LIRS stack
        typename std::list<Key>::iterator stack_iter;
        typename std::list<Key>::iterator hir_iter;  // Only valid if HIR
    };

    /**
     * Handle access to existing entry
     */
    void access(const Key& key) {
        auto& entry = map_[key];
        
        if (entry.is_lir) {
            // LIR access: Move to stack top
            if (entry.in_stack) {
                stack_.erase(entry.stack_iter);
            }
            stack_.push_front(key);
            entry.stack_iter = stack_.begin();
            entry.in_stack = true;
            
            // Prune stack if needed
            prune_stack();
        } else {
            // HIR access
            if (entry.in_stack) {
                // HIR in stack: Promote to LIR
                promote_to_lir(key);
            } else {
                // HIR not in stack: Move to HIR list front
                hir_list_.erase(entry.hir_iter);
                hir_list_.push_front(key);
                entry.hir_iter = hir_list_.begin();
                
                // Add to stack
                stack_.push_front(key);
                entry.stack_iter = stack_.begin();
                entry.in_stack = true;
            }
        }
    }

    /**
     * Promote HIR entry to LIR
     */
    void promote_to_lir(const Key& key) {
        auto& entry = map_[key];
        
        if (entry.is_lir) return;  // Already LIR
        
        // Remove from HIR list
        if (entry.in_stack) {
            hir_list_.erase(entry.hir_iter);
        }
        
        // Make it LIR
        entry.is_lir = true;
        lir_count_++;
        
        // Move to stack top
        if (entry.in_stack) {
            stack_.erase(entry.stack_iter);
        }
        stack_.push_front(key);
        entry.stack_iter = stack_.begin();
        entry.in_stack = true;
        
        // If we exceed LIR limit, demote bottom LIR to HIR
        if (lir_count_ > lir_size_) {
            demote_lir_to_hir();
        }
        
        prune_stack();
    }

    /**
     * Demote LIR entry to HIR (when LIR set is full)
     */
    void demote_lir_to_hir() {
        // Find bottom LIR in stack
        for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
            const Key& key = *it;
            auto& entry = map_[key];
            
            if (entry.is_lir) {
                // Demote this LIR to HIR
                entry.is_lir = false;
                lir_count_--;
                
                // Remove from stack
                stack_.erase(entry.stack_iter);
                entry.in_stack = false;
                
                // Add to HIR list
                hir_list_.push_front(key);
                entry.hir_iter = hir_list_.begin();
                
                break;
            }
        }
    }

    /**
     * Prune stack: Remove HIR entries from stack bottom
     */
    void prune_stack() {
        while (!stack_.empty()) {
            const Key& bottom_key = stack_.back();
            auto it = map_.find(bottom_key);
            
            if (it == map_.end() || it->second.is_lir) {
                break;  // Bottom is LIR or empty
            }
            
            // Bottom is HIR: Remove from stack
            auto& entry = it->second;
            stack_.erase(entry.stack_iter);
            entry.in_stack = false;
        }
    }

    /**
     * Evict entry when cache is full
     */
    void evict() {
        // Evict from HIR list (tail = least recently used HIR)
        if (!hir_list_.empty()) {
            const Key& victim = hir_list_.back();
            map_.erase(victim);
            hir_list_.pop_back();
        } else {
            // No HIR entries, evict bottom LIR (shouldn't happen normally)
            if (!stack_.empty()) {
                const Key& victim = stack_.back();
                auto& entry = map_[victim];
                if (entry.is_lir) {
                    lir_count_--;
                }
                map_.erase(victim);
                stack_.pop_back();
            }
        }
    }

    size_t capacity_;              // Total cache capacity
    size_t lir_size_;              // LIR set capacity
    size_t hir_size_;              // HIR set capacity
    size_t lir_count_ = 0;         // Current LIR count
    
    std::unordered_map<Key, Entry> map_;  // Key -> Entry
    std::list<Key> stack_;                // LIRS stack (top = most recent)
    std::list<Key> hir_list_;             // HIR list (top = most recent)
    
    mutable std::shared_mutex mutex_; // Thread safety: shared for reads, exclusive for writes
    std::atomic<size_t> hits_;     // Hit counter
    std::atomic<size_t> misses_;   // Miss counter
};

} // namespace performance
} // namespace themis
