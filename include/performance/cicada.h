/**
 * @file cicada.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Cicada: Optimistic Concurrency Control
// Paper: "Cicada: Dependably Fast Multi-Core In-Memory Transactions" (SIGMOD'17)
// Authors: Hyeontaek Lim et al., Carnegie Mellon University
//
// Key idea: Best-effort inlining + contention regulation for minimal overhead
// Expected gain: +100-150% transaction throughput
// Reference: https://dl.acm.org/doi/10.1145/3035918.3064015

#pragma once

#include <cstdint>
#include <atomic>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace themis {
namespace performance {

/// Cicada versioned record with optimistic locking
/// Uses a single 64-bit word for version + write lock
/// Bit layout: [63: write lock] [62-0: version number]
class CicadaRecord {
public:
    static constexpr uint64_t WRITE_LOCK_BIT = 1ULL << 63;  // Bit 63 is write lock
    static constexpr uint64_t VERSION_MASK = ~WRITE_LOCK_BIT;  // Bits 0-62 are version
    
    CicadaRecord() : version_and_lock_(0) {}
    explicit CicadaRecord(std::string initial_data)
        : version_and_lock_(0), data_(std::move(initial_data)) {}
    
    // Try to acquire write lock
    bool try_lock() {
        uint64_t v = version_and_lock_.load(std::memory_order_acquire);
        if (v & WRITE_LOCK_BIT) {
            return false; // Already locked
        }
        return version_and_lock_.compare_exchange_strong(
            v, v | WRITE_LOCK_BIT, 
            std::memory_order_acquire, 
            std::memory_order_relaxed
        );
    }
    
    // Release write lock and bump version
    void unlock_and_increment_version() {
        uint64_t v = version_and_lock_.load(std::memory_order_relaxed);
        uint64_t new_version = ((v & VERSION_MASK) + 1) & VERSION_MASK;
        version_and_lock_.store(new_version, std::memory_order_release);
    }
    
    // Get current version (without lock bit)
    uint64_t get_version() const {
        return version_and_lock_.load(std::memory_order_acquire) & VERSION_MASK;
    }
    
    // Check if locked
    bool is_locked() const {
        return (version_and_lock_.load(std::memory_order_acquire) & WRITE_LOCK_BIT) != 0;
    }

    // Data access.
    // set_data(): caller must hold the write lock (try_lock() returned true).
    // get_data(): safe to call after confirming get_version() matches the snapshot
    //             taken before the read (standard OCC read-validation pattern).
    //             Concurrent set_data() + get_data() without lock/version-check
    //             is a data race — use the version stamp to detect this.
    void set_data(std::string new_data) {
        data_ = std::move(new_data);
    }
    const std::string& get_data() const { return data_; }

private:
    std::atomic<uint64_t> version_and_lock_;
    std::string data_;  // Record payload written by install_writes()
};

/// Transaction context for Cicada
class CicadaTransaction {
public:
    CicadaTransaction() : commit_timestamp_(0), aborted_(false) {}
    
    // Record read operation
    void record_read(CicadaRecord* record, uint64_t version_read);
    
    // Record write operation — data is the new value to install on commit
    void record_write(CicadaRecord* record, std::string data);
    
    // Execute transaction logic
    using TransactionFunc = std::function<bool()>;
    bool execute(const TransactionFunc& func);
    
    // Validate read set (Phase 1 of commit)
    bool validate_reads();
    
    // Acquire write locks (Phase 2 of commit)
    bool acquire_write_locks();
    
    // Install writes (Phase 3 of commit)
    void install_writes();
    
    // Release locks (cleanup)
    void release_locks();
    
    // Full commit protocol
    bool commit();
    
    // Abort transaction
    void abort();
    
    bool is_aborted() const { return aborted_; }

private:
    struct ReadEntry {
        CicadaRecord* record;
        uint64_t version;
    };
    
    struct WriteEntry {
        CicadaRecord* record;
        std::string data; // New data to write
    };
    
    std::vector<ReadEntry> read_set_;
    std::vector<WriteEntry> write_set_;
    uint64_t commit_timestamp_;
    bool aborted_;
};

/// Contention manager for Cicada
/// Regulates contention to reduce abort rate
class ContentionManager {
public:
    ContentionManager() : abort_count_(0), commit_count_(0) {}
    
    // Record transaction outcome
    void record_commit() {
        commit_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    void record_abort() {
        abort_count_.fetch_add(1, std::memory_order_relaxed);
    }
    
    // Get abort rate (for monitoring)
    double get_abort_rate() const {
        uint64_t aborts = abort_count_.load(std::memory_order_relaxed);
        uint64_t commits = commit_count_.load(std::memory_order_relaxed);
        if (commits + aborts == 0) {
          return 0.0;
        }
        return static_cast<double>(aborts) / (commits + aborts);
    }
    
    // Should back off? (high contention detected)
    bool should_backoff() const {
        return get_abort_rate() > 0.5; // >50% abort rate
    }
    
    void reset_stats() {
        abort_count_.store(0, std::memory_order_relaxed);
        commit_count_.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> abort_count_;
    std::atomic<uint64_t> commit_count_;
};

} // namespace performance
} // namespace themis
