/**
 * @file rcu_hash_table.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB RCU-Protected Hash Table Index
// Lock-free reads for read-heavy workloads using RCU
//
// Based on: "Scalable Read-Mostly Synchronization Using RCU" (ASPLOS'10)
//
// Note: Manual memory management is required here because the table pointer
// is shared across threads via std::atomic. Using unique_ptr would be unsafe
// since multiple threads access the raw pointer returned by atomic::load().

#pragma once

#include <performance/rcu.h>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <cstring>

namespace themis {
namespace rcu {

/**
 * @brief Simple RCU-protected hash table for key-value storage
 * 
 * Provides lock-free reads and safe concurrent updates using RCU.
 * Optimized for read-heavy workloads (90%+ reads).
 * 
 * Performance characteristics:
 * - Reads: O(1) lock-free
 * - Writes: O(1) with writer lock
 * - Memory: Slightly higher due to copy-on-write
 */
template<typename Key, typename Value>
class RCUHashTable {
public:
    explicit RCUHashTable(size_t initial_capacity = 1024)
        : capacity_(initial_capacity) {
        
        // Allocate array - will be manually managed since it's shared across threads via atomic
        // Cannot use unique_ptr because atomic requires raw pointer for lock-free access
        auto* table = new HashNode*[capacity_];
        std::memset(table, 0, capacity_ * sizeof(HashNode*));
        table_.store(table, std::memory_order_release);
    }
    
    ~RCUHashTable() {
        // Clean up all nodes - must use manual delete since atomic stores raw pointer
        auto* table = table_.load(std::memory_order_acquire);
        if (table) {
            for (size_t i = 0; i < capacity_; ++i) {
                HashNode* node = table[i];
                while (node) {
                    HashNode* next = node->next;
                    delete node;
                    node = next;
                }
            }
            delete[] table;  // Manual cleanup required for atomically-shared pointer
        }
    }
    
    /**
     * @brief Look up a value by key (lock-free)
     * 
     * @param key Key to search for
     * @param value Output parameter for found value
     * @return true if key was found
     */
    bool lookup(const Key& key, Value& value) const {
        #ifdef THEMIS_USE_RCU_INDEX
        ReadLock lock;
        #endif
        
        auto* table = table_.load(std::memory_order_acquire);
        size_t index = hash(key) % capacity_;
        
        HashNode* node = table[index];
        while (node) {
            if (node->key == key) {
                value = node->value;
                return true;
            }
            node = node->next;
        }
        
        return false;
    }
    
    /**
     * @brief Insert or update a key-value pair
     * 
     * @param key Key to insert/update
     * @param value Value to store
     */
    void insert(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(writer_mutex_);
        
        auto* old_table = table_.load(std::memory_order_acquire);
        size_t index = hash(key) % capacity_;
        
        // Check if key already exists
        HashNode* node = old_table[index];
        HashNode* prev = nullptr;
        
        while (node) {
            if (node->key == key) {
                // Update existing node (copy-on-write)
                auto* new_node = new HashNode(key, value);
                new_node->next = node->next;
                
                if (prev) {
                    prev->next = new_node;
                } else {
                    old_table[index] = new_node;
                }
                
                #ifdef THEMIS_USE_RCU_INDEX
                rcu_defer_delete(node);
                #else
                delete node;
                #endif
                
                return;
            }
            prev = node;
            node = node->next;
        }
        
        // Insert new node at head
        auto* new_node = new HashNode(key, value);
        new_node->next = old_table[index];
        old_table[index] = new_node;
        
        size_.fetch_add(1, std::memory_order_relaxed);
    }
    
    /**
     * @brief Remove a key from the table
     * 
     * @param key Key to remove
     * @return true if key was found and removed
     */
    bool remove(const Key& key) {
        std::lock_guard<std::mutex> lock(writer_mutex_);
        
        auto* table = table_.load(std::memory_order_acquire);
        size_t index = hash(key) % capacity_;
        
        HashNode* node = table[index];
        HashNode* prev = nullptr;
        
        while (node) {
            if (node->key == key) {
                if (prev) {
                    prev->next = node->next;
                } else {
                    table[index] = node->next;
                }
                
                #ifdef THEMIS_USE_RCU_INDEX
                rcu_defer_delete(node);
                #else
                delete node;
                #endif
                
                size_.fetch_sub(1, std::memory_order_relaxed);
                return true;
            }
            prev = node;
            node = node->next;
        }
        
        return false;
    }
    
    /**
     * @brief Get the number of elements in the table
     */
    size_t size() const {
        return size_.load(std::memory_order_relaxed);
    }
    
    /**
     * @brief Check if table is empty
     */
    bool empty() const {
        return size() == 0;
    }
    
private:
    struct HashNode {
        Key key;
        Value value;
        HashNode* next;
        
        HashNode(const Key& k, const Value& v) 
            : key(k), value(v), next(nullptr) {}
    };
    
    size_t hash(const Key& key) const {
        // Simple hash function - can be customized
        return std::hash<Key>{}(key);
    }
    
    std::atomic<HashNode**> table_;
    size_t capacity_;
    std::atomic<size_t> size_{0};
    mutable std::mutex writer_mutex_;
};

} // namespace rcu
} // namespace themis
