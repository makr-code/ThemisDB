// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/compaction_manager.h"
#include "utils/error_registry.h"

namespace themis {

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / destructor
// ──────────────────────────────────────────────────────────────────────────────

CompactionManager::CompactionManager(std::shared_ptr<RocksDBWrapper> db)
    : CompactionManager(std::move(db), Config{}) {}

CompactionManager::CompactionManager(std::shared_ptr<RocksDBWrapper> db,
                                     const Config& config)
    : db_(std::move(db)), config_(config) {
    if (!db_) {
        throw std::invalid_argument("CompactionManager: db cannot be null");
    }
}

CompactionManager::~CompactionManager() {
    stopBackgroundGC();
}

// ──────────────────────────────────────────────────────────────────────────────
// Manual compaction
// ──────────────────────────────────────────────────────────────────────────────

Result<void> CompactionManager::compactRange(std::string_view start_key,
                                              std::string_view end_key) {
    if (!db_->isOpen()) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "CompactionManager: database is not open");
    }
    db_->compactRange(start_key, end_key);
    manual_compactions_.fetch_add(1, std::memory_order_relaxed);
    return OkVoid();
}

Result<void> CompactionManager::compactAll() {
    if (!db_->isOpen()) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "CompactionManager: database is not open");
    }
    // Compact the entire keyspace by passing empty start/end.
    db_->compactRange("", "");
    manual_compactions_.fetch_add(1, std::memory_order_relaxed);
    return OkVoid();
}

// ──────────────────────────────────────────────────────────────────────────────
// Tombstone tracking
// ──────────────────────────────────────────────────────────────────────────────

void CompactionManager::recordDeletions(uint64_t count) {
    tombstones_.fetch_add(count, std::memory_order_relaxed);
}

Result<void> CompactionManager::runGC(bool force) {
    uint64_t t = tombstones_.load(std::memory_order_relaxed);
    if (!force && t < config_.tombstone_gc_threshold) {
        return OkVoid(); // threshold not reached
    }

    Result<void> res = OkVoid();
    if (config_.enable_full_compaction) {
        res = compactAll();
    }

    if (res) {
        tombstones_.store(0, std::memory_order_relaxed);
        gc_runs_.fetch_add(1, std::memory_order_relaxed);
    }
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
// Background GC
// ──────────────────────────────────────────────────────────────────────────────

void CompactionManager::startBackgroundGC() {
    std::lock_guard<std::mutex> lock(bg_mutex_);
    if (bg_thread_.joinable()) return; // already running
    bg_stop_.store(false, std::memory_order_relaxed);
    bg_thread_ = std::thread([this] { backgroundLoop(); });
}

void CompactionManager::stopBackgroundGC() {
    {
        std::lock_guard<std::mutex> lock(bg_mutex_);
        bg_stop_.store(true, std::memory_order_relaxed);
        bg_cv_.notify_all();
    }
    if (bg_thread_.joinable()) {
        bg_thread_.join();
    }
}

bool CompactionManager::isBackgroundGCRunning() const {
    std::lock_guard<std::mutex> lock(bg_mutex_);
    return bg_thread_.joinable() && !bg_stop_.load(std::memory_order_relaxed);
}

void CompactionManager::backgroundLoop() {
    while (!bg_stop_.load(std::memory_order_relaxed)) {
        // Wait for the configured interval or until woken by stopBackgroundGC().
        std::unique_lock<std::mutex> lock(bg_mutex_);
        bg_cv_.wait_for(lock, config_.bg_gc_interval,
                        [this] { return bg_stop_.load(std::memory_order_relaxed); });
        lock.unlock();

        if (bg_stop_.load(std::memory_order_relaxed)) break;

        // Run GC (non-forced: honours tombstone threshold).
        (void)runGC(false);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Metrics
// ──────────────────────────────────────────────────────────────────────────────

CompactionManager::Stats CompactionManager::stats() const {
    Stats s;
    s.tombstones_tracked = tombstones_.load(std::memory_order_relaxed);
    s.gc_runs            = gc_runs_.load(std::memory_order_relaxed);
    s.manual_compactions = manual_compactions_.load(std::memory_order_relaxed);
    if (db_->isOpen()) {
        s.rocksdb_stats = db_->getStats();
    }
    return s;
}

} // namespace themis
