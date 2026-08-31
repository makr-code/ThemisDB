/**
 * @file parallel_downloader.h
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
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace updates {

/**
 * @brief A single file download request
 *
 * Contains everything needed to download a file, verify its integrity,
 * and resume a partially-completed transfer.
 */
struct DownloadTask {
    /// HTTP/HTTPS URL to fetch
    std::string url;

    /// Absolute local path to write the file to
    std::string dest;

    /// Expected SHA-256 hash (hex-encoded). Empty → no integrity check.
    std::string expected_hash;

    /**
     * @brief Download priority (higher = scheduled first)
     *
     * Suggested convention: executables → 10, libraries → 5, data → 1.
     */
    int priority = 1;

    /**
     * @brief Maximum number of retry attempts on transient failure (0 = no retries)
     */
    int max_retries = 3;

    /**
     * @brief Enable HTTP range-request resume for this task
     *
     * When true and a partial file already exists at `dest`, the downloader
     * sends a `Range: bytes=<offset>-` header to resume from that offset.
     */
    bool enable_resume = true;
};

/**
 * @brief Result of a single DownloadTask
 */
struct DownloadResult {
    /// Original task (for correlation)
    DownloadTask task;

    /// True if the file was downloaded and (if requested) hash-verified
    bool success = false;

    /// Human-readable error description when success == false
    std::string error_message;

    /// Number of bytes written in this session (excludes previously-resumed bytes)
    uint64_t bytes_downloaded = 0;

    /// Total file size as reported by Content-Length (0 if unknown)
    uint64_t total_bytes = 0;

    /// Wall-clock duration of this download
    std::chrono::milliseconds duration{0};

    /// Number of retry attempts consumed
    int retries_used = 0;

    /// True if the download was resumed from a partial file
    bool was_resumed = false;
};

/**
 * @brief Aggregate statistics produced by a downloadAll() call
 */
struct DownloadBatchStats {
    size_t   total_tasks        = 0;
    size_t   succeeded          = 0;
    size_t   failed             = 0;
    size_t   resumed            = 0;
    uint64_t total_bytes        = 0;
    std::chrono::milliseconds wall_time{0};
};

/**
 * @brief Downloads multiple files concurrently with bandwidth throttling,
 *        priority ordering, and per-file resume support.
 *
 * ### Quick-start
 * ```cpp
 * ParallelDownloader dl;
 * dl.setConcurrency(4);
 * dl.setBandwidthLimit(100ULL * 1024 * 1024);   // 100 MB/s
 *
 * std::vector<DownloadTask> tasks = buildTaskList(manifest);
 * auto results = dl.downloadAll(tasks);
 *
 * for (const auto& r : results) {
 *     if (!r.success) log_error(r.task.url, r.error_message);
 * }
 * ```
 *
 * ### Acceptance criteria (Issue #128 / v1.6.0)
 * | AC | Mechanism |
 * |----|-----------|
 * | Configurable concurrency | `setConcurrency(n)` → worker thread pool of size n |
 * | Bandwidth throttling     | Token-bucket at `setBandwidthLimit(bps)` |
 * | Priority queue           | Max-heap by `DownloadTask::priority` |
 * | Resume support per file  | HTTP `Range:` header if partial file exists |
 *
 * ### Thread-safety
 * A single `ParallelDownloader` instance must not be called concurrently
 * from multiple threads.  Separate instances may run in parallel.
 */
class ParallelDownloader {
public:
    /**
     * @brief Construct with sensible defaults
     *        (concurrency = hardware_concurrency, no bandwidth cap).
     */
    ParallelDownloader();
    ~ParallelDownloader();

    // ── Configuration ────────────────────────────────────────────────────────

    /**
     * @brief Set the maximum number of simultaneous downloads (≥ 1)
     * @throws std::invalid_argument if n == 0
     */
    void setConcurrency(size_t n);
    size_t getConcurrency() const noexcept;

    /**
     * @brief Limit aggregate download throughput.
     *
     * Implemented as a token-bucket checked on every byte-consumption call and
     * refilled whenever at least 10 ms have elapsed since the last refill.
     *
     * @param bytes_per_second 0 = unlimited (default)
     */
    void setBandwidthLimit(uint64_t bytes_per_second);
    uint64_t getBandwidthLimit() const noexcept;

    /**
     * @brief Override the HTTP connect timeout (default: 10 s)
     */
    void setConnectTimeoutSeconds(long seconds);

    /**
     * @brief Override the HTTP transfer timeout per chunk (default: 30 s)
     */
    void setTransferTimeoutSeconds(long seconds);

    // ── Execution ────────────────────────────────────────────────────────────

    /**
     * @brief Download all tasks, respecting concurrency and bandwidth limits.
     *
     * Tasks are executed in priority-descending order (highest priority first).
     * The method blocks until all tasks have either completed or exhausted
     * their retry budget.
     *
     * @param tasks  List of download tasks (order does not matter; sorted internally)
     * @return       One DownloadResult per input task, in input order
     */
    std::vector<DownloadResult> downloadAll(std::vector<DownloadTask> tasks);

    /**
     * @brief Retrieve aggregate statistics from the most recent downloadAll() call
     */
    DownloadBatchStats lastBatchStats() const noexcept;

    // ── Observability ────────────────────────────────────────────────────────

    /**
     * @brief Register a per-file progress callback.
     *
     * Called from worker threads with:
     *  - task_index: position in the original tasks vector
     *  - bytes_so_far: bytes written for this file in this session
     *  - total_bytes: Content-Length (0 if unknown)
     *  - message: human-readable status (e.g. "downloading", "verifying", "done")
     *
     * The callback must be thread-safe.
     */
    void setProgressCallback(
        std::function<void(size_t task_index,
                           uint64_t bytes_so_far,
                           uint64_t total_bytes,
                           const std::string& message)> callback);

    // ── Testability ──────────────────────────────────────────────────────────

    /**
     * @brief Inject a custom HTTP fetch function (for unit tests).
     *
     * Signature:
     *   bool fetch(const std::string& url,
     *              const std::string& dest_path,
     *              uint64_t resume_offset,      // 0 = fresh download
     *              long connect_timeout_s,
     *              long transfer_timeout_s,
     *              uint64_t* out_bytes,         // bytes written this call
     *              uint64_t* out_total,         // Content-Length (may be 0)
     *              std::string* out_error);
     */
    using FetchFn = std::function<bool(
        const std::string& url,
        const std::string& dest,
        uint64_t resume_offset,
        long connect_timeout_s,
        long transfer_timeout_s,
        uint64_t* out_bytes,
        uint64_t* out_total,
        std::string* out_error)>;

    void setFetchFunction(FetchFn fn);

private:
    size_t     concurrency_        = 0;
    uint64_t   bandwidth_limit_bps_= 0;   ///< 0 = unlimited
    long       connect_timeout_s_  = 10;
    long       transfer_timeout_s_ = 30;

    FetchFn    fetch_fn_;
    std::function<void(size_t, uint64_t, uint64_t, const std::string&)> progress_cb_;

    mutable DownloadBatchStats last_stats_;

    // Token-bucket throttle state
    mutable std::atomic<uint64_t> token_bucket_{0};
    mutable std::atomic<int64_t>  last_refill_ms_{0};

    // Helpers
    DownloadResult executeTask(size_t task_index, const DownloadTask& task);
    uint64_t       resumeOffset(const std::string& dest) const;

    void   consumeBandwidth(uint64_t bytes) const;
    void   refillTokens(uint64_t bytes_needed) const;

    static std::string computeSha256(const std::string& path);
    static bool        defaultFetch(
        const std::string& url,
        const std::string& dest,
        uint64_t resume_offset,
        long connect_timeout_s,
        long transfer_timeout_s,
        uint64_t* out_bytes,
        uint64_t* out_total,
        std::string* out_error);
};

} // namespace updates
} // namespace themis
