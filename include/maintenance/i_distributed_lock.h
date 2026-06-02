/*
 * ThemisDB | File: i_distributed_lock.h | Version: 0.0.9 | Last Modified: 2026-05-31 12:49:01
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 93/100 | Lines: 170
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4649 docs(maintenance): reality-... (2026-04-15) | #4630 feat(maintenance): Distribu... (2026-04-13)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file i_distributed_lock.h
 * @brief IDistributedLock — pluggable distributed lock interface for the
 *        maintenance orchestrator.
 *
 * In a multi-node cluster, each node independently schedules and fires
 * maintenance jobs.  Two nodes may trigger the same schedule concurrently,
 * causing compaction storms or double maintenance.
 *
 * The DatabaseMaintenanceOrchestrator uses an IDistributedLock to elect a
 * single maintenance leader per schedule.  Before firing a scheduled job the
 * orchestrator calls `tryAcquire(schedule_id, ttl_ms)`.  Only the node that
 * successfully acquires the lock runs the job; all others log a DEBUG-level
 * skip message and set the job state to SKIPPED.
 *
 * ### Implementations
 * - `InProcessDistributedLock` — single-process TTL-based lock (unit tests,
 *   single-node deployments).
 * - In production: inject a Raft-backed implementation that forwards acquire /
 *   release calls to `src/replication/raft_v2.cpp` or a dedicated distributed
 *   lock service.
 *
 * ### Thread Safety
 * All implementations must be fully thread-safe.
 */

#pragma once

#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// IDistributedLock
// ---------------------------------------------------------------------------

/**
 * @brief Strategy interface for a distributed, TTL-based lock.
 *
 * Used by DatabaseMaintenanceOrchestrator to prevent two cluster nodes from
 * executing the same maintenance schedule concurrently.
 */
class IDistributedLock {
public:
    virtual ~IDistributedLock() = default;

    /**
     * @brief Try to acquire the lock for @p key with the given TTL.
     *
     * Non-blocking: returns immediately with `true` (acquired) or `false`
     * (held by another node or a prior acquisition by this node has not yet
     * expired).
     *
     * @param key     Lock key; typically the schedule ID.
     * @param ttl_ms  Time-to-live in milliseconds.  The lock is automatically
     *                released after this duration even if `release()` is never
     *                called — prevents orphaned locks when a node crashes.
     * @return true if this node successfully acquired the lock.
     */
    [[nodiscard]] virtual bool tryAcquire(const std::string& key, int64_t ttl_ms) = 0;

    /**
     * @brief Release the lock for @p key previously acquired by this node.
     *
     * A no-op when this node does not currently hold the lock.
     *
     * @param key  Lock key to release.
     */
    virtual void release(const std::string& key) = 0;

    /**
     * @brief Return the node ID currently holding the lock for @p key.
     *
     * Used only for logging ("schedule {id} skipped — lock held by peer {node_id}").
     *
     * @return Node ID string, or empty string if the lock is not currently held
     *         (e.g. expired or never acquired).
     */
    [[nodiscard]] virtual std::string getHolderNodeId(const std::string& key) const = 0;

    /**
     * @brief Return the identifier of this node.
     */
    [[nodiscard]] virtual std::string nodeId() const = 0;
};

// ---------------------------------------------------------------------------
// InProcessDistributedLock — in-process TTL-based implementation
// ---------------------------------------------------------------------------

/**
 * @brief Single-process implementation of IDistributedLock.
 *
 * Suitable for unit tests and single-node deployments where true distributed
 * coordination is not required.  Lock state is stored in an in-process map
 * protected by a std::mutex; TTL expiry is checked on each `tryAcquire` call.
 *
 * Thread-safe.
 */
class InProcessDistributedLock : public IDistributedLock {
public:
    explicit InProcessDistributedLock(std::string node_id = "local-node")
        : node_id_(std::move(node_id)) {}

    bool tryAcquire(const std::string& key, int64_t ttl_ms) override {
        std::lock_guard<std::mutex> lg(mutex_);
        auto now = std::chrono::steady_clock::now();

        auto it = locks_.find(key);
        if (it != locks_.end()) {
            // Existing entry: check TTL expiry or re-acquire by same node
            const LockEntry& e = it->second;
            if (now < e.expires_at && e.holder != node_id_) {
                // Held by someone else and not yet expired
                return false;
            }
        }

        // Grant (fresh acquisition or TTL-expired or renewal by same node)
        LockEntry& e   = locks_[key];
        e.holder       = node_id_;
        e.expires_at   = now + std::chrono::milliseconds(ttl_ms);
        return true;
    }

    void release(const std::string& key) override {
        std::lock_guard<std::mutex> lg(mutex_);
        auto it = locks_.find(key);
        if (it != locks_.end() && it->second.holder == node_id_) {
            locks_.erase(it);
        }
    }

    std::string getHolderNodeId(const std::string& key) const override {
        std::lock_guard<std::mutex> lg(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto it  = locks_.find(key);
        if (it == locks_.end()) return {};
        if (now >= it->second.expires_at) return {};  // expired
        return it->second.holder;
    }

    std::string nodeId() const override { return node_id_; }

private:
    struct LockEntry {
        std::string                           holder;
        std::chrono::steady_clock::time_point expires_at;
    };

    std::string node_id_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, LockEntry> locks_;
};

} // namespace maintenance
} // namespace themis
