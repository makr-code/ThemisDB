/**
 * @file tiered_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: tiered_storage.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 436
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=7, L=0
 * PR History (last 5): #4213 feat(storage): DistributedT... (2026-03-14) | #4150 feat(storage): implement si... (2026-03-13)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "storage/tiered_storage.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <stdexcept>

namespace fs = std::filesystem;

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// AccessTracker
// ─────────────────────────────────────────────────────────────────────────────

void AccessTracker::recordWrite(const std::string& key, StorageTierLevel tier,
                                uint64_t value_size) {
    std::unique_lock lock(mutex_);
    auto& e = entries_[key];
    e.written_at   = std::chrono::system_clock::now();
    e.last_read_at = e.written_at;
    e.tier         = tier;
    e.value_size   = value_size;
}

void AccessTracker::recordRead(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.last_read_at = std::chrono::system_clock::now();
    }
}

void AccessTracker::setTier(const std::string& key, StorageTierLevel tier) {
    std::unique_lock lock(mutex_);
    // iterator_invalidation scanner alert: the iterator is used only to update
    // it->second.tier (a field assignment); no element is inserted or erased from
    // entries_ between find() and the assignment, so the iterator remains valid.
    auto it = entries_.find(key);
    if (it != entries_.end()) {
        it->second.tier = tier;
    }
}

void AccessTracker::remove(const std::string& key) {
    std::unique_lock lock(mutex_);
    entries_.erase(key);
}

std::unordered_map<std::string, AccessTracker::Entry> AccessTracker::snapshot() const {
    std::shared_lock lock(mutex_);
    return entries_;
}

std::size_t AccessTracker::size() const {
    std::shared_lock lock(mutex_);
    return entries_.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// TieredStorageManager – helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Replace characters that are unsafe for filenames and reject path traversal sequences.
std::string sanitizeKey(const std::string& key) {
    if (key.empty()) return "_empty_";

    // Reject keys that could escape tier directories
    // uncaught_exception scanner alerts (lines 85, 94): sanitizeKey throws
    // std::invalid_argument for path-traversal attempts; callers must not pass
    // malformed keys — intentional security enforcement — false positives.
    if (key.find("..") != std::string::npos) {
        throw std::invalid_argument("Key contains path traversal sequence: " + key);
    }

    // Reject keys that are only '.' (current directory reference)
    auto trimmed = key;
    while (!trimmed.empty() && (trimmed.front() == '/' || trimmed.front() == '\\')) {
        trimmed.erase(trimmed.begin());
    }
    if (trimmed == ".") {
        throw std::invalid_argument("Key is a current-directory reference: " + key);
    }
    if (trimmed.empty()) return "_";

    std::string safe;
    safe.reserve(trimmed.size());
    for (char c : trimmed) {
        safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
                 c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
                    ? '_'
                    : c;
    }
    if (safe.empty()) safe = "_";
    return safe;
}

// Day-level comparison helper
using SClock = std::chrono::system_clock;
using Days   = std::chrono::duration<int64_t, std::ratio<86400>>;

int64_t daysSince(SClock::time_point tp) {
    return std::chrono::duration_cast<Days>(SClock::now() - tp).count();
}

} // anonymous namespace

TieredStorageManager::TieredStorageManager(const TieredStorageConfig& config)
    : config_(config) {
    for (auto& path : {config_.hot_tier_path,
                       config_.warm_tier_path,
                       config_.cold_tier_path}) {
        fs::create_directories(path);
    }
    THEMIS_INFO("TieredStorageManager initialised: hot={}, warm={}, cold={}",
                config_.hot_tier_path, config_.warm_tier_path, config_.cold_tier_path);
}

TieredStorageManager::~TieredStorageManager() {
    stopMigrationWorker();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tier I/O helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string TieredStorageManager::tierPath(StorageTierLevel tier) const {
    switch (tier) {
        case StorageTierLevel::HOT:  return config_.hot_tier_path;
        case StorageTierLevel::WARM: return config_.warm_tier_path;
        case StorageTierLevel::COLD: return config_.cold_tier_path;
    }
    return config_.hot_tier_path;
}

std::string TieredStorageManager::keyFilePath(const std::string& key,
                                               StorageTierLevel tier) const {
    return (fs::path(tierPath(tier)) / (sanitizeKey(key) + ".dat")).string();
}

bool TieredStorageManager::writeToTier(const std::string& key,
                                        const std::string& value,
                                        StorageTierLevel tier) {
    const std::string path = keyFilePath(key, tier);
    try {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) {
            THEMIS_ERROR("TieredStorage: failed to open '{}' for writing", path);
            return false;
        }
        f.write(value.data(), static_cast<std::streamsize>(value.size()));
        return f.good();
    } catch (const std::exception& ex) {
        THEMIS_ERROR("TieredStorage: write exception for key '{}': {}", key, ex.what());
        return false;
    }
}

std::string TieredStorageManager::readFromTier(const std::string& key,
                                                StorageTierLevel tier) const {
    const std::string path = keyFilePath(key, tier);
    if (!fs::exists(path)) {
        THEMIS_DEBUG("TieredStorageManager::readFromTier: path '{}' does not exist for key '{}', tier={}", path, key, static_cast<int>(tier));
        return {};
    }
    try {
        std::ifstream f(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(f),
                           std::istreambuf_iterator<char>());
    } catch (const std::exception& ex) {
        THEMIS_ERROR("TieredStorage: read exception for key '{}': {}", key, ex.what());
        return {};
    }
}

bool TieredStorageManager::deleteFromTier(const std::string& key,
                                           StorageTierLevel tier) {
    const std::string path = keyFilePath(key, tier);
    if (!fs::exists(path)) return false;
    std::error_code ec;
    fs::remove(path, ec);
    if (ec) {
        THEMIS_WARN("TieredStorage: failed to delete '{}': {}", path, ec.message());
        return false;
    }
    return true;
}

bool TieredStorageManager::existsInTier(const std::string& key,
                                         StorageTierLevel tier) const {
    return fs::exists(keyFilePath(key, tier));
}

// ─────────────────────────────────────────────────────────────────────────────
// Core CRUD
// ─────────────────────────────────────────────────────────────────────────────

bool TieredStorageManager::put(const std::string& key, const std::string& value) {
    if (!writeToTier(key, value, StorageTierLevel::HOT)) {
        return false;
    }
    // Remove stale copies from lower tiers (in case of re-promotion)
    deleteFromTier(key, StorageTierLevel::WARM);
    deleteFromTier(key, StorageTierLevel::COLD);
    tracker_.recordWrite(key, StorageTierLevel::HOT,
                         static_cast<uint64_t>(value.size()));
    return true;
}

std::string TieredStorageManager::get(const std::string& key) {
    // Fast path: check AccessTracker to determine which tier likely holds the key
    {
        auto snap = tracker_.snapshot();
        auto it = snap.find(key);
        if (it != snap.end()) {
            StorageTierLevel expected = it->second.tier;
            if (existsInTier(key, expected)) {
                tracker_.recordRead(key);
                
                // Phase 5: BLOCK 3 Integration — detect hot pattern in warm/cold tiers
                if (expected == StorageTierLevel::WARM || expected == StorageTierLevel::COLD) {
                    auto now = std::chrono::system_clock::now();
                    auto written_at = std::chrono::system_clock::from_time_t(
                        std::chrono::system_clock::to_time_t(it->second.written_at));
                    auto window_secs = std::chrono::duration_cast<std::chrono::seconds>(
                        now - written_at).count();
                    
                    // Emit promotion event if data is hot (accessed in cool tier)
                    auto tier = (expected == StorageTierLevel::WARM) ?
                               access_model::TierLevel::STORAGE_WARM :
                               access_model::TierLevel::STORAGE_COLD;
                    emitPromotionEvent(key, tier, it->second.read_count, window_secs);
                }
                
                return readFromTier(key, expected);
            }
        }
    }

    // Fallback: scan all tiers in hot-to-cold order (handles tracker miss)
    for (auto tier : {StorageTierLevel::HOT, StorageTierLevel::WARM, StorageTierLevel::COLD}) {
        if (existsInTier(key, tier)) {
            tracker_.recordRead(key);
            
            // Phase 5: BLOCK 3 Integration — detect hot pattern in warm/cold tiers
            if (tier == StorageTierLevel::WARM || tier == StorageTierLevel::COLD) {
                auto snap = tracker_.snapshot();
                auto it = snap.find(key);
                if (it != snap.end()) {
                    auto now = std::chrono::system_clock::now();
                    auto written_at = std::chrono::system_clock::from_time_t(
                        std::chrono::system_clock::to_time_t(it->second.written_at));
                    auto window_secs = std::chrono::duration_cast<std::chrono::seconds>(
                        now - written_at).count();
                    
                    auto tier_level = (tier == StorageTierLevel::WARM) ?
                                     access_model::TierLevel::STORAGE_WARM :
                                     access_model::TierLevel::STORAGE_COLD;
                    emitPromotionEvent(key, tier_level, it->second.read_count, window_secs);
                }
            }
            
            return readFromTier(key, tier);
        }
    }
    return {};
}

bool TieredStorageManager::del(const std::string& key) {
    bool found = false;
    for (auto tier : {StorageTierLevel::HOT, StorageTierLevel::WARM, StorageTierLevel::COLD}) {
        if (deleteFromTier(key, tier)) found = true;
    }
    if (found) tracker_.remove(key);
    return found;
}

StorageTierLevel TieredStorageManager::tierOf(const std::string& key) const {
    auto snap = tracker_.snapshot();
    auto it = snap.find(key);
    if (it != snap.end()) return it->second.tier;
    // Fall back to scanning actual tier directories
    if (existsInTier(key, StorageTierLevel::HOT))  return StorageTierLevel::HOT;
    if (existsInTier(key, StorageTierLevel::WARM)) return StorageTierLevel::WARM;
    if (existsInTier(key, StorageTierLevel::COLD)) return StorageTierLevel::COLD;
    return StorageTierLevel::HOT;
}

// ─────────────────────────────────────────────────────────────────────────────
// Migration
// ─────────────────────────────────────────────────────────────────────────────

bool TieredStorageManager::migrateKey(const std::string& key,
                                       StorageTierLevel from,
                                       StorageTierLevel to) {
    // Read from source (validate existence first)
    if (!existsInTier(key, from)) {
        THEMIS_WARN("TieredStorage: migrateKey({}, {} -> {}): key not found in source",
                    key,
                    static_cast<int>(from),
                    static_cast<int>(to));
        return false;
    }
    std::string value = readFromTier(key, from);
    // Validate that the read succeeded (readFromTier returns "" on I/O error)
    // A zero-byte file is a valid value, so we only abort on actual read failure.
    if (value.empty() && !existsInTier(key, from)) {
        // File disappeared between existsInTier and readFromTier – concurrent deletion
        THEMIS_WARN("TieredStorage: migrateKey({}) source disappeared during read", key);
        stat_migration_errors_++;
        return false;
    }

    // Write to destination (copy-then-delete for crash safety)
    if (!writeToTier(key, value, to)) {
        THEMIS_ERROR("TieredStorage: migrateKey({}) failed to write to destination tier {}",
                     key, static_cast<int>(to));
        stat_migration_errors_++;
        return false;
    }

    // Delete from source only after successful copy
    // delete_no_nullptr scanner alert (line 300): deleteFromTier() is a class method
    // that removes a file from a storage tier; it is not the delete operator and does
    // not dereference any raw pointer — false positive.
    if (!deleteFromTier(key, from)) {
        THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);
        // Not a hard error – we have a valid copy at destination; clean up later
    }

    tracker_.setTier(key, to);
    THEMIS_DEBUG("TieredStorage: migrated key '{}' {} -> {}", key,
                 static_cast<int>(from), static_cast<int>(to));
    return true;
}

uint32_t TieredStorageManager::runMigrationCycle() {
    auto entries = tracker_.snapshot();
    uint32_t migrated = 0;
    const uint32_t limit = config_.max_migrations_per_cycle;

    for (auto& [key, entry] : entries) {
        if (limit > 0 && migrated >= limit) break;

        // ── Size-based policy (checked first, overrides tier-based rules) ──
        if (config_.large_blob_bytes > 0 && entry.tier != config_.large_blob_tier) {
            if (entry.value_size >= config_.large_blob_bytes) {
                if (migrateKey(key, entry.tier, config_.large_blob_tier)) {
                    stat_migrations_size_based_++;
                    ++migrated;
                }
                continue;  // skip further policy checks for this key
            }
        }

        if (entry.tier == StorageTierLevel::HOT) {
            bool demote = false;

            // Age-based rule
            if (config_.hot_to_warm_days > 0 &&
                daysSince(entry.written_at) >= static_cast<int64_t>(config_.hot_to_warm_days)) {
                demote = true;
            }
            // Access-frequency rule
            if (!demote && config_.hot_zero_access_days > 0 &&
                daysSince(entry.last_read_at) >= static_cast<int64_t>(config_.hot_zero_access_days)) {
                demote = true;
            }

            if (demote && existsInTier(key, StorageTierLevel::HOT)) {
                if (migrateKey(key, StorageTierLevel::HOT, StorageTierLevel::WARM)) {
                    stat_migrations_hot_to_warm_++;
                    ++migrated;
                }
            }

        } else if (entry.tier == StorageTierLevel::WARM) {
            bool demote = false;
            bool promote = false;

            // Phase 5: BLOCK 3 Integration — detect hot promotion pattern
            // If warm tier data is accessed frequently, emit promotion event
            if (entry.read_count > 0 && daysSince(entry.last_read_at) < 1) {
                // Warm tier data accessed recently → potential hot candidate
                auto now = std::chrono::system_clock::now();
                auto window_secs = std::chrono::duration_cast<std::chrono::seconds>(
                    now - entry.last_read_at).count();
                
                // Emit promotion event for coordinator to consider moving back to HOT
                emitPromotionEvent(key, access_model::TierLevel::L2_EPISODIC,
                                  entry.read_count, window_secs);
                promote = true;
            }

            if (config_.warm_to_cold_days > 0 &&
                daysSince(entry.written_at) >= static_cast<int64_t>(config_.warm_to_cold_days)) {
                demote = true;
            }
            if (!demote && config_.warm_zero_access_days > 0 &&
                daysSince(entry.last_read_at) >= static_cast<int64_t>(config_.warm_zero_access_days)) {
                demote = true;
            }

            if (demote && existsInTier(key, StorageTierLevel::WARM)) {
                if (migrateKey(key, StorageTierLevel::WARM, StorageTierLevel::COLD)) {
                    stat_migrations_warm_to_cold_++;
                    ++migrated;
                }
            }
        }
    }

    if (migrated > 0) {
        THEMIS_INFO("TieredStorage: migration cycle complete, {} key(s) migrated", migrated);
    }
    return migrated;
}

// ─────────────────────────────────────────────────────────────────────────────
// Background worker
// ─────────────────────────────────────────────────────────────────────────────

void TieredStorageManager::workerLoop() {
    THEMIS_INFO("TieredStorage: migration worker started");
    // lock_contention scanner alert: worker_mutex_ is acquired with wait_for so the
    // thread sleeps while the lock is held by the condition variable; the lock is
    // released immediately after waking (lock.unlock()) before the next work cycle.
    // This is the canonical background-worker pattern — false positive.
    while (worker_running_.load(std::memory_order_relaxed)) {
        std::unique_lock lock(worker_mutex_);
        worker_cv_.wait_for(lock,
            std::chrono::seconds(config_.migration_check_interval_secs),
            [this] { return !worker_running_.load(std::memory_order_relaxed); });

        if (!worker_running_.load(std::memory_order_relaxed)) break;
        lock.unlock();

        try {
            runMigrationCycle();
        } catch (const std::exception& ex) {
            THEMIS_ERROR("TieredStorage: migration worker exception: {}", ex.what());
        }
    }
    THEMIS_INFO("TieredStorage: migration worker stopped");
}

void TieredStorageManager::startMigrationWorker() {
    if (worker_running_.exchange(true)) return;  // already running
    worker_thread_ = std::thread(&TieredStorageManager::workerLoop, this);
}

void TieredStorageManager::stopMigrationWorker() {
    if (!worker_running_.exchange(false)) return;  // already stopped
    {
        std::lock_guard lock(worker_mutex_);
        worker_cv_.notify_all();
    }
    if (worker_thread_.joinable() &&
        !utils::joinThreadWithin(worker_thread_)) {
        THEMIS_WARN("TieredStorage: migration worker exceeded shutdown timeout");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

TieredStorageManager::Stats TieredStorageManager::stats() const {
    Stats s;
    s.migrations_hot_to_warm = stat_migrations_hot_to_warm_.load();
    s.migrations_warm_to_cold = stat_migrations_warm_to_cold_.load();
    s.migrations_size_based = stat_migrations_size_based_.load();
    s.migration_errors = stat_migration_errors_.load();

    auto entries = tracker_.snapshot();
    for (auto& [key, entry] : entries) {
        switch (entry.tier) {
            case StorageTierLevel::HOT:  s.hot_keys++;  break;
            case StorageTierLevel::WARM: s.warm_keys++; break;
            case StorageTierLevel::COLD: s.cold_keys++; break;
        }
    }
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// BLOCK 3: Storage Module Integration — AccessCoordinator Listener
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5: BLOCK 3 Storage Integration — Promotion Event Emission
// ─────────────────────────────────────────────────────────────────────────────

void TieredStorageManager::emitPromotionEvent(const std::string& key,
                                             access_model::TierLevel from_tier,
                                             uint64_t access_count,
                                             int64_t access_window_secs) {
    std::lock_guard<std::mutex> lock(promotion_listener_mutex_);
    if (!promotion_listener_) {
        return;  // No listener registered
    }

    // Emit event to coordinator (detected hot pattern in warm/cold tier)
    promotion_listener_->onStorageAccess(key, from_tier, access_count,
                                        std::chrono::seconds(access_window_secs));
}

// ─────────────────────────────────────────────────────────────────────────────
// BLOCK 3: Storage Module Integration — AccessCoordinator Listener
// ─────────────────────────────────────────────────────────────────────────────

void TieredStorageManager::setPromotionListener(access_model::PromotionListener* listener) noexcept {
    std::lock_guard<std::mutex> lock(promotion_listener_mutex_);
    promotion_listener_ = listener;
    if (promotion_listener_) {
        THEMIS_INFO("TieredStorageManager: promotion listener registered for AccessCoordinator");
    } else {
        THEMIS_INFO("TieredStorageManager: promotion listener unregistered");
    }
}

} // namespace storage
} // namespace themis
