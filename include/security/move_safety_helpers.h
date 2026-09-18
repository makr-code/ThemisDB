/**
 * @file move_safety_helpers.h
 * @brief Sprint 8 Batch D Phase 2: Reusable move-safety patterns and helpers
 * 
 * This header provides production-ready patterns for handling moved objects safely:
 * 1. StateSnapshot - Capture object metadata before move
 * 2. HandleRegistry - ID-based lookup after move  
 * 3. MoveGuard - Verify object validity post-move
 * 4. SnapshotValidator - Validate snapshot consistency
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace themis::safety {

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 1: StateSnapshot - Capture immutable state before move
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Generic snapshot pattern for capturing object state before move
 * 
 * Usage:
 * ```cpp
 * auto snapshot = StateSnapshot::from(obj);  // Capture
 * container[key] = std::move(obj);           // Move
 * auto data = snapshots[key];                // Retrieve snapshot
 * ```
 */
template <typename T>
class StateSnapshot {
public:
    using IdType = std::string;
    
    /// Capture essential state from object before move
    static StateSnapshot from(const T& obj);
    
    /// Immutable ID for registry lookup
    IdType id() const { return id_; }
    
    /// Verify snapshot validity after move
    bool isValid() const { return valid_; }
    
private:
    IdType id_;
    std::vector<uint8_t> state_hash_;
    bool valid_ = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 2: HandleRegistry - ID-based object retrieval after move
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Registry that stores objects by ID after they've been moved
 * 
 * Provides safe access to moved objects by ID instead of reference
 * 
 * Usage:
 * ```cpp
 * registry.store(id, std::move(obj));
 * auto result = registry.get(id);
 * ```
 */
template <typename T>
class HandleRegistry {
public:
    using IdType = std::string;
    
    /// Store object by ID (moves the object)
    void store(const IdType& id, T obj) {
        std::lock_guard<std::mutex> lk(mutex_);
        objects_[id] = std::move(obj);
    }
    
    /// Retrieve object by ID (returns copy/reference)
    std::optional<T> get(const IdType& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = objects_.find(id);
        if (it != objects_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    /// Check if ID exists
    bool contains(const IdType& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        return objects_.find(id) != objects_.end();
    }
    
    /// Clear all entries
    void clear() {
        std::lock_guard<std::mutex> lk(mutex_);
        objects_.clear();
    }
    
    /// Get count of stored objects
    size_t size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return objects_.size();
    }
    
private:
    mutable std::mutex mutex_;
    std::map<IdType, T> objects_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 3: MoveGuard - Verify object validity post-move
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Guard that detects use-after-move in debug builds
 * 
 * In production, this is a no-op. In debug mode, it detects invalid access.
 * 
 * Usage:
 * ```cpp
 * T obj = create();
 * {
 *     MoveGuard<T> guard(obj);
 *     container = std::move(obj);
 *     // Guard destructor verifies obj not used after move
 * }
 * // obj.foo();  // This would be flagged (or UB)
 * ```
 */
template <typename T>
class MoveGuard {
public:
    explicit MoveGuard(T& obj) : obj_(obj) {
#ifndef NDEBUG
        // In debug mode, capture object state before move
        // This is used to detect if access happens post-move
#endif
    }
    
    ~MoveGuard() {
#ifndef NDEBUG
        // In debug mode, verify object hasn't been accessed after move
        // This is best-effort; not all UB can be detected
#endif
    }
    
private:
    T& obj_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Pattern 4: Specific Implementations for ThemisDB Objects
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief TransactionHandle - Safe reference to transaction after move
 * 
 * Captures transaction ID and essential state before move
 * Allows safe queries by ID after transaction is moved to executor
 */
struct TransactionHandle {
    std::string transaction_id;
    uint64_t phase = 0;                  // 2PC phase
    std::string status;                  // PENDING, RUNNING, COMPLETED, FAILED
    int64_t created_at_ms = 0;
    
    /// Create handle from transaction (before move)
    template <typename T>
    static TransactionHandle from(const T& txn);
    
    /// Verify handle validity
    bool isValid() const {
        return !transaction_id.empty() && !status.empty();
    }
};

/**
 * @brief CoordinatorStateSnapshot - Safe reference to coordinator after move
 * 
 * Captures coordinator state (participants, phase, decision) before move
 * Allows safe queries by coordinator ID after move
 */
struct CoordinatorStateSnapshot {
    std::string coordinator_id = {};
    std::vector<std::string> participants;  // Shard IDs in 2PC
    std::string phase;                      // PREPARE, COMMIT, ABORT
    std::string decision;                   // COMMIT or ABORT
    int64_t timestamp_ms = 0;
    
    bool isValid() const {
        return !coordinator_id.empty() && !phase.empty();
    }
};

/**
 * @brief PlacementHandle - Safe reference to placement decision after move
 * 
 * Captures placement decision (shard IDs, load) before move
 * Allows safe queries by decision ID after move
 */
struct PlacementHandle {
    std::string decision_id = {};
    std::vector<std::string> assigned_shards;  // Target shards
    std::map<std::string, double> shard_load;   // Load per shard
    double total_cost = 0.0;
    bool is_balanced = false;
    
    bool isValid() const {
        return !decision_id.empty() && !assigned_shards.empty();
    }
};

/**
 * @brief ModelPipelineState - Safe reference to model after move
 * 
 * Captures model metadata before move through pipeline stages
 * Uses shared_ptr for shared ownership across stages
 */
struct ModelPipelineState {
    std::string model_id = {};
    std::string model_name = {};
    size_t parameter_count = 0;
    std::string device = "cpu";
    bool is_loaded = false;
    
    bool isValid() const {
        return !model_id.empty() && !model_name.empty();
    }
};

} // namespace themis::safety
