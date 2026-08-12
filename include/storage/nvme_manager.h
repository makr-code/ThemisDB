/**
 * @file nvme_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for NVMe-specific storage optimizations.
 *
 * All fields default to safe, portable values.  Enable individual optimizations
 * only after verifying that the underlying device and kernel support them.
 */
struct NVMeConfig {
    // ── io_uring (Linux ≥ 5.1) ───────────────────────────────────────────
    /// Enable io_uring-based async I/O for storage operations.
    /// Falls back to synchronous pread/pwrite when unavailable.
    bool enable_io_uring = false;

    /// Depth of the io_uring submission / completion queues.
    /// Higher values allow more in-flight I/Os (optimal: 64–256 for NVMe).
    uint32_t io_uring_queue_depth = 128;

    /// Pre-register fixed file descriptors to eliminate per-op fd table lookups.
    bool io_uring_register_files = true;

    /// Pre-register fixed I/O buffers for zero-copy DMA paths.
    bool io_uring_register_buffers = false;

    // ── Multi-queue parallel I/O ─────────────────────────────────────────
    /// Number of parallel I/O submission queues (one per hardware queue pair).
    /// Set to 0 for auto-detect (uses /sys/block/<dev>/queue/nr_hw_queues).
    uint32_t num_io_queues = 0;

    /// Maximum outstanding I/O requests per queue.
    uint32_t queue_depth = 256;

    // ── Zone Namespace (ZNS) support ─────────────────────────────────────
    /// Enable ZNS-aware write placement.  Requires a ZNS NVMe device and
    /// kernel ≥ 5.9 with CONFIG_BLK_DEV_ZONED enabled.
    bool enable_zns = false;

    /// Capacity in bytes of a single zone (must match device zone size).
    /// Typically 512 MiB (536_870_912) on commercial ZNS drives.
    uint64_t zone_capacity_bytes = 536'870'912ULL;  // 512 MiB

    /// Maximum number of open zones allowed simultaneously.
    /// Constrained by the device's max_open_zones firmware limit.
    uint32_t max_open_zones = 14;

    // ── Direct I/O ───────────────────────────────────────────────────────
    /// Bypass the OS page cache for reads (O_DIRECT).
    /// Eliminates double-buffering overhead when RocksDB block cache is large.
    bool use_direct_reads = false;

    /// Bypass the OS page cache for flush and compaction writes (O_DIRECT).
    bool use_direct_io_for_flush_and_compaction = false;

    /// I/O alignment requirement in bytes (must be a power of 2).
    /// Typical value: 4096 (4 KiB) for NVMe direct I/O.
    uint32_t direct_io_alignment_bytes = 4096;

    // ── Device path ──────────────────────────────────────────────────────
    /// Block-device path used for capability detection (e.g. "/dev/nvme0n1").
    /// Leave empty to skip runtime detection.
    std::string device_path;
};

// ─────────────────────────────────────────────────────────────────────────────
// Capability report
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Detected NVMe / OS capabilities on the current host.
 */
struct NVMeCapabilities {
    bool io_uring_available  = false;  ///< Kernel supports io_uring (Linux ≥ 5.1)
    bool zns_available       = false;  ///< Block device is ZNS-capable
    bool direct_io_available = false;  ///< O_DIRECT supported on the filesystem
    uint32_t hw_queue_count  = 1;      ///< Number of hardware queue pairs detected
    uint32_t kernel_major    = 0;      ///< Running kernel major version
    uint32_t kernel_minor    = 0;      ///< Running kernel minor version
    std::string device_model;          ///< NVMe device model string (if detected)
};

// ─────────────────────────────────────────────────────────────────────────────
// NVMe async I/O request handle
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Opaque handle for an in-flight async I/O request.
 *
 * Callers submit a request via NVMeManager::submitRead / submitWrite and poll
 * via NVMeManager::pollCompletions.
 */
struct NVMeIORequest {
    int     fd          = -1;       ///< File descriptor of the target file
    void*   buf         = nullptr;  ///< I/O buffer (must be aligned for Direct I/O)
    size_t  len         = 0;        ///< Transfer length in bytes
    int64_t offset      = 0;        ///< File offset (use int64_t for cross-platform portability)
    bool    is_write    = false;    ///< true = write, false = read
    int64_t user_data   = 0;        ///< Caller-defined tag returned on completion
};

/**
 * @brief Result returned by NVMeManager::pollCompletions for each finished I/O.
 */
struct NVMeIOResult {
    int64_t user_data   = 0;   ///< Tag provided at submission time
    int32_t result      = 0;   ///< Bytes transferred (>= 0) or -errno on error
};

// ─────────────────────────────────────────────────────────────────────────────
// NVMeManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief NVMe-specific storage optimization manager.
 *
 * Provides:
 *  - Runtime detection of io_uring, ZNS, and multi-queue capabilities.
 *  - io_uring async I/O submission and completion polling.
 *  - ZNS zone management helpers (reset, finish, query zone state).
 *  - Helper to build the recommended RocksDB Direct I/O flags for the device.
 *
 * Thread-safety:
 *  - detectCapabilities() is safe to call concurrently from any thread;
 *    results are computed at most once and cached via std::call_once.
 *  - submitRead / submitWrite / pollCompletions are thread-safe; they acquire
 *    an internal ring_mutex_ before accessing the shared io_uring ring.
 *  - Zone management functions are serialised via an internal mutex.
 */
class NVMeManager {
public:
    explicit NVMeManager(const NVMeConfig& config = {});
    ~NVMeManager();

    NVMeManager(const NVMeManager&)            = delete;
    NVMeManager& operator=(const NVMeManager&) = delete;
    NVMeManager(NVMeManager&&)                 = delete;
    NVMeManager& operator=(NVMeManager&&)      = delete;

    // ── Lifecycle ────────────────────────────────────────────────────────

    /**
     * @brief Initialise the manager and set up the io_uring ring if enabled.
     *
     * Always returns true.  If io_uring or ZNS are requested but unavailable,
     * those features are silently disabled (with a WARN log) and the manager
     * continues operating in degraded mode.
     */
    bool initialize();

    /**
     * @brief Release all resources (io_uring ring, registered buffers/files).
     */
    void shutdown();

    // ── Capability detection ─────────────────────────────────────────────

    /**
     * @brief Probe the host for NVMe / kernel capabilities.
     *
     * Result is cached after the first call.
     */
    NVMeCapabilities detectCapabilities() const;

    /**
     * @brief Return whether io_uring was successfully initialised.
     */
    bool isIoUringActive() const noexcept;

    /**
     * @brief Return the number of hardware I/O queue pairs detected.
     */
    uint32_t detectedQueueCount() const noexcept;

    // ── io_uring async I/O ───────────────────────────────────────────────

    /**
     * @brief Submit an asynchronous read request.
     *
     * @param req  I/O request descriptor.
     * @return true if the request was enqueued; false on ring overflow or error.
     *
     * @note Falls back to synchronous pread() when io_uring is unavailable.
     */
    bool submitRead(const NVMeIORequest& req);

    /**
     * @brief Submit an asynchronous write request.
     *
     * @param req  I/O request descriptor.
     * @return true if the request was enqueued; false on ring overflow or error.
     *
     * @note Falls back to synchronous pwrite() when io_uring is unavailable.
     */
    bool submitWrite(const NVMeIORequest& req);

    /**
     * @brief Flush pending submissions and collect completions.
     *
     * @param results       Output vector populated with completed I/O results.
     * @param min_complete  Minimum completions to wait for (0 = non-blocking).
     * @return Number of completions harvested, or -1 on error.
     */
    int pollCompletions(std::vector<NVMeIOResult>& results,
                        uint32_t min_complete = 0);

    // ── ZNS zone management ──────────────────────────────────────────────

    /**
     * @brief Reset a zone (erase all data, mark zone as empty).
     *
     * @param zone_offset  Byte offset of the zone start (must be zone-aligned).
     * @return true on success.
     */
    bool resetZone(uint64_t zone_offset);

    /**
     * @brief Mark a zone as full (no further writes until reset).
     *
     * @param zone_offset  Byte offset of the zone start.
     * @return true on success.
     */
    bool finishZone(uint64_t zone_offset);

    /**
     * @brief Query the write pointer (current append position) of a zone.
     *
     * @param zone_offset  Byte offset of the zone start.
     * @return Write pointer byte offset, or UINT64_MAX on error.
     */
    uint64_t getZoneWritePointer(uint64_t zone_offset) const;

    // ── RocksDB integration helpers ──────────────────────────────────────

    /**
     * @brief Populate a RocksDB-compatible Direct I/O flag summary.
     *
     * Returns a pair of booleans representing
     *  { use_direct_reads, use_direct_io_for_flush_and_compaction }
     * adjusted to reflect only the flags that the detected device supports.
     */
    std::pair<bool, bool> recommendedDirectIOFlags() const;

    /**
     * @brief Recommended number of RocksDB background I/O threads.
     *
     * Returns min(detected_hw_queue_count * 2, 16) as a practical upper bound.
     */
    uint32_t recommendedBackgroundThreads() const;

    /**
     * @brief Return the active configuration.
     */
    const NVMeConfig& config() const noexcept { return config_; }

private:
    NVMeConfig config_;

    // io_uring state (Linux only; opaque to avoid platform-specific headers)
    struct IoUringState;
    std::unique_ptr<IoUringState> ring_;

    mutable NVMeCapabilities capabilities_;
    mutable std::once_flag   capabilities_once_;  ///< Guards one-time detection
    mutable std::mutex       state_mutex_;         ///< Guards cached capabilities/config reads used across threads
    mutable std::mutex       zone_mutex_;          ///< Serialises zone management calls
    mutable std::mutex       ring_mutex_;          ///< Serialises io_uring ring operations

    std::atomic<bool> initialized_{false};

    // Internal helpers
    bool    setupIoUring();
    void    teardownIoUring();
    bool    probeIoUringKernel() const;
    uint32_t readHwQueueCount() const;
};

}  // namespace storage
}  // namespace themis
