/**
 * @file tiered_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: tiered_storage.h | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include "access_model/access_coordinator.h"

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Tier identity (maps to the physical location of a key's data).
 */
enum class StorageTierLevel {
    HOT,    ///< NVMe / fastest local SSD
    WARM,   ///< SATA SSD or spinning disk
    COLD    ///< Object storage or archive (slow, cheap)
};

/**
 * @brief Configuration for TieredStorageManager.
 *
 * Migration policy is evaluated by the background TierMigrationWorker at
 * intervals of `migration_check_interval`.  Both age-based and
 * access-frequency-based rules are checked; a key is demoted when *either*
 * condition is satisfied.
 */
struct TieredStorageConfig {
    // ── Tier paths ────────────────────────────────────────────────────────
    std::string hot_tier_path  = "./data/hot";   ///< Directory for hot-tier data
    std::string warm_tier_path = "./data/warm";  ///< Directory for warm-tier data
    /// Cold-tier backend URI, e.g. "s3://archive-bucket/prefix" or a path
    std::string cold_tier_path = "./data/cold";

    // ── Age-based migration policy ─────────────────────────────────────────
    /// Days since last write before hot→warm migration (0 = disabled)
    uint32_t hot_to_warm_days  = 30;
    /// Days since last write before warm→cold migration (0 = disabled)
    uint32_t warm_to_cold_days = 90;

    // ── Access-frequency policy ────────────────────────────────────────────
    /// Demote hot→warm if the key has zero reads within this window (0 = disabled)
    uint32_t hot_zero_access_days  = 14;
    /// Demote warm→cold if the key has zero reads within this window (0 = disabled)
    uint32_t warm_zero_access_days = 45;

    // ── Background worker ──────────────────────────────────────────────────
    /// How often the migration worker scans for candidates (seconds)
    uint32_t migration_check_interval_secs = 3600;  // 1 hour
    /// Maximum keys migrated per worker cycle (0 = unlimited)
    uint32_t max_migrations_per_cycle = 500;

    // ── Size-based migration policy ────────────────────────────────────────
    /// Migrate any key whose stored value meets or exceeds this size (bytes) to
    /// `large_blob_tier`, regardless of age or access frequency.
    /// Set to 0 to disable size-based migration (default).
    uint64_t large_blob_bytes = 0;
    /// Destination tier for oversized blobs (default: COLD).
    StorageTierLevel large_blob_tier = StorageTierLevel::COLD;

    // ── Safety ─────────────────────────────────────────────────────────────
    /// Abort a migration and retry if foreground I/O on the destination tier
    /// exceeds this ratio of free space (0.0–1.0).  Default 95%.
    double migration_abort_disk_usage_ratio = 0.95;
};

// ─────────────────────────────────────────────────────────────────────────────
// AccessTracker
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe per-key last-access tracker.
 *
 * Records write timestamp (creation) and the most recent read time.
 * Used by TierMigrationWorker to evaluate age- and access-based policies.
 */
class AccessTracker {
public:
    struct Entry {
        std::chrono::system_clock::time_point written_at;
        std::chrono::system_clock::time_point last_read_at;
        StorageTierLevel tier{StorageTierLevel::HOT};
        uint64_t value_size{0};  ///< Byte length of the stored value (set on put)
        uint64_t read_count{0};  ///< Number of times this entry has been read
    };

    /// Record a write (creates or resets the entry).
    void recordWrite(const std::string& key,
                     StorageTierLevel   tier      = StorageTierLevel::HOT,
                     uint64_t           value_size = 0);

    /// Record a read access.
    void recordRead(const std::string& key);

    /// Update the tier label (called after a successful migration).
    void setTier(const std::string& key, StorageTierLevel tier);

    /// Remove an entry (called on key deletion).
    void remove(const std::string& key);

    /// Return a snapshot of all tracked entries.
    std::unordered_map<std::string, Entry> snapshot() const;

    /// Return the number of tracked keys.
    std::size_t size() const;

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> entries_;
};

// ─────────────────────────────────────────────────────────────────────────────
// TieredStorageManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Manages a three-tier (hot / warm / cold) storage hierarchy.
 *
 * Provides transparent put/get/del operations that route to the correct tier,
 * and runs a background TierMigrationWorker that moves data between tiers
 * according to the configured age- and access-based policies.
 *
 * Data integrity guarantee: migration uses copy-then-delete so that a crash
 * mid-migration always leaves at least one valid copy.
 *
 * Thread-Safety: All public methods are thread-safe.
 */
class TieredStorageManager {
public:
    explicit TieredStorageManager(const TieredStorageConfig& config = TieredStorageConfig{});
    ~TieredStorageManager();

    // Non-copyable, non-movable (owns mutex, atomic, and thread)
    TieredStorageManager(const TieredStorageManager&)            = delete;
    TieredStorageManager& operator=(const TieredStorageManager&) = delete;
    TieredStorageManager(TieredStorageManager&&)                 = delete;
    TieredStorageManager& operator=(TieredStorageManager&&)      = delete;

    // ── Core CRUD ─────────────────────────────────────────────────────────

    /**
     * @brief Write a value to the hot tier.
     * @return false on I/O failure.
     */
    bool put(const std::string& key, const std::string& value);

    /**
     * @brief Read a value from whichever tier holds it.
     * @return empty string if not found.
     */
    std::string get(const std::string& key);

    /**
     * @brief Delete a key from all tiers.
     * @return true if the key existed in at least one tier.
     */
    bool del(const std::string& key);

    /**
     * @brief Return the current tier of a key (HOT if unknown).
     */
    StorageTierLevel tierOf(const std::string& key) const;

    // ── Background worker control ─────────────────────────────────────────

    /// Start the background migration worker (idempotent).
    void startMigrationWorker();

    /// Stop the background migration worker and wait for it to finish.
    void stopMigrationWorker();

    // ── Observability ─────────────────────────────────────────────────────

    struct Stats {
        uint64_t hot_keys{0};
        uint64_t warm_keys{0};
        uint64_t cold_keys{0};
        uint64_t migrations_hot_to_warm{0};
        uint64_t migrations_warm_to_cold{0};
        uint64_t migrations_size_based{0};
        uint64_t migration_errors{0};
    };

    Stats stats() const;

    const TieredStorageConfig& config() const { return config_; }
    AccessTracker&             accessTracker()       { return tracker_; }
    const AccessTracker&       accessTracker() const { return tracker_; }

    // ── BLOCK 3: Storage Module Integration — AccessCoordinator Listener ────

    /**
     * @brief Register a promotion listener for AccessCoordinator integration.
     *
     * Once registered, the TieredStorageManager notifies the listener when
     * detecting hot access patterns (candidates for promotion to cache L3).
     * Pass nullptr to unregister.
     *
     * Typical usage:
     * @code
     *   auto coordinator = std::make_shared<access_model::AccessCoordinator>();
     *   storage->setPromotionListener(coordinator.get());
     * @endcode
     *
     * @param listener Pointer to a PromotionListener implementation;
     *                 nullptr disables promotion notifications.
     *
     * @see include/access_model/access_coordinator.h
     * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
     */
    void setPromotionListener(access_model::PromotionListener* listener) noexcept;

    // ── Forced migration (for tests / admin tools) ────────────────────────

    /**
     * @brief Immediately evaluate migration candidates and move them.
     * @return Number of keys migrated.
     */
    uint32_t runMigrationCycle();

private:
    TieredStorageConfig config_;
    AccessTracker       tracker_;

    // Phase 5: BLOCK 3 Storage Integration — AccessCoordinator listener
    // Notified when warm/cold tiers detect hot access patterns
    access_model::PromotionListener* promotion_listener_{nullptr};
    mutable std::mutex promotion_listener_mutex_;

    // Stats counters
    std::atomic<uint64_t> stat_migrations_hot_to_warm_{0};
    std::atomic<uint64_t> stat_migrations_warm_to_cold_{0};
    std::atomic<uint64_t> stat_migrations_size_based_{0};
    std::atomic<uint64_t> stat_migration_errors_{0};

    // Background worker
    std::thread             worker_thread_;
    std::atomic<bool>       worker_running_{false};
    std::mutex              worker_mutex_;
    std::condition_variable worker_cv_;

    void workerLoop();

    // Phase 5: BLOCK 3 Storage Integration — Emit promotion events to coordinator
    void emitPromotionEvent(const std::string& key, access_model::TierLevel from_tier,
                           uint64_t access_count, int64_t access_window_secs);

    // ── Tier I/O helpers ──────────────────────────────────────────────────
    std::string tierPath(StorageTierLevel tier) const;
    std::string keyFilePath(const std::string& key, StorageTierLevel tier) const;

    bool writeToTier(const std::string& key, const std::string& value,
                     StorageTierLevel tier);
    std::string readFromTier(const std::string& key, StorageTierLevel tier) const;
    bool deleteFromTier(const std::string& key, StorageTierLevel tier);
    bool existsInTier(const std::string& key, StorageTierLevel tier) const;

    // Migrate one key: copy to destination, then delete from source.
    // Returns true on success.
    bool migrateKey(const std::string& key,
                    StorageTierLevel   from,
                    StorageTierLevel   to);
};

} // namespace storage
} // namespace themis
