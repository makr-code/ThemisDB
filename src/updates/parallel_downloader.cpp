/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            parallel_downloader.cpp                            ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:37:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     491                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1d10fc7b3f  2026-03-13  feat(build): add redis_cache and AQL components to build ... ║
    • 354c827c49  2026-03-13  audit(updates): fix 5 issues found in parallel downloader ║
    • ab135ba46b  2026-03-13  feat(updates): implement Parallel File Downloads (Issue #... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "updates/parallel_downloader.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#include <algorithm>
#include <array>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <openssl/evp.h>

namespace themis {
namespace updates {

namespace fs = std::filesystem;

// ============================================================================
// Constructor / destructor
// ============================================================================

ParallelDownloader::ParallelDownloader()
    : concurrency_(static_cast<size_t>(
          std::max(1u, std::thread::hardware_concurrency())))
    , bandwidth_limit_bps_(0)
    , connect_timeout_s_(10)
    , transfer_timeout_s_(30)
    , fetch_fn_(&ParallelDownloader::defaultFetch)
{
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count();
    last_refill_ms_.store(now_ms, std::memory_order_relaxed);
}

ParallelDownloader::~ParallelDownloader() = default;

// ============================================================================
// Configuration
// ============================================================================

void ParallelDownloader::setConcurrency(size_t n) {
    if (n == 0) {
        throw std::invalid_argument("ParallelDownloader::setConcurrency: n must be >= 1");
    }
    concurrency_ = n;
}

size_t ParallelDownloader::getConcurrency() const noexcept {
    return concurrency_;
}

void ParallelDownloader::setBandwidthLimit(uint64_t bytes_per_second) {
    bandwidth_limit_bps_ = bytes_per_second;
    if (bytes_per_second > 0) {
        // Pre-fill bucket with one 100 ms slice
        const uint64_t initial = bytes_per_second / 10;
        token_bucket_.store(initial, std::memory_order_relaxed);
    }
}

uint64_t ParallelDownloader::getBandwidthLimit() const noexcept {
    return bandwidth_limit_bps_;
}

void ParallelDownloader::setConnectTimeoutSeconds(long seconds) {
    connect_timeout_s_ = seconds;
}

void ParallelDownloader::setTransferTimeoutSeconds(long seconds) {
    transfer_timeout_s_ = seconds;
}

void ParallelDownloader::setProgressCallback(
    std::function<void(size_t, uint64_t, uint64_t, const std::string&)> callback)
{
    progress_cb_ = std::move(callback);
}

void ParallelDownloader::setFetchFunction(FetchFn fn) {
    fetch_fn_ = std::move(fn);
}

DownloadBatchStats ParallelDownloader::lastBatchStats() const noexcept {
    return last_stats_;
}

// ============================================================================
// Bandwidth throttle helpers (token-bucket)
// ============================================================================

void ParallelDownloader::refillTokens(uint64_t /*bytes_needed*/) const {
    if (bandwidth_limit_bps_ == 0) return;

    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count();

    int64_t last = last_refill_ms_.load(std::memory_order_acquire);
    const int64_t elapsed_ms = now_ms - last;

    if (elapsed_ms >= 10) {  // refill every 10 ms
        // Only one thread should update last_refill_ms_ per interval.
        // compare_exchange_strong atomically checks and updates `last`.
        // If the CAS fails, another thread already handled this interval.
        if (!last_refill_ms_.compare_exchange_strong(
                last, now_ms,
                std::memory_order_release, std::memory_order_relaxed)) {
            return;
        }
        const uint64_t new_tokens =
            (bandwidth_limit_bps_ * static_cast<uint64_t>(elapsed_ms)) / 1000ULL;
        // Cap at 2× the per-100ms slice to avoid burst accumulation
        const uint64_t max_tokens = bandwidth_limit_bps_ / 5;
        uint64_t current = token_bucket_.load(std::memory_order_relaxed);
        uint64_t refilled;
        do {
            refilled = std::min(current + new_tokens, max_tokens);
        } while (!token_bucket_.compare_exchange_weak(
                     current, refilled,
                     std::memory_order_relaxed, std::memory_order_relaxed));
    }
}

void ParallelDownloader::consumeBandwidth(uint64_t bytes) const {
    if (bandwidth_limit_bps_ == 0) return;

    while (true) {
        refillTokens(bytes);

        uint64_t avail = token_bucket_.load(std::memory_order_relaxed);
        if (avail >= bytes) {
            // Atomically subtract only if the bucket still has enough tokens.
            if (token_bucket_.compare_exchange_weak(
                    avail, avail - bytes,
                    std::memory_order_relaxed, std::memory_order_relaxed)) {
                return;
            }
            // CAS failed – another thread consumed tokens concurrently; retry.
            continue;
        }

        // Not enough tokens — sleep for approximately the time needed to
        // accumulate `bytes` worth of bandwidth.
        const uint64_t deficit = bytes - avail;
        const auto sleep_ms =
            std::chrono::milliseconds(
                static_cast<long>(deficit * 1000ULL / bandwidth_limit_bps_) + 1);
        std::this_thread::sleep_for(sleep_ms);
    }
}

// ============================================================================
// Resume offset
// ============================================================================

uint64_t ParallelDownloader::resumeOffset(const std::string& dest) const {
    std::error_code ec;
    const auto sz = fs::file_size(dest, ec);
    if (ec || sz == static_cast<std::uintmax_t>(-1)) return 0;
    return static_cast<uint64_t>(sz);
}

// ============================================================================
// SHA-256 helper
// ============================================================================

std::string ParallelDownloader::computeSha256(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return {};

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return {};

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return {};
    }

    std::array<char, 65536> buf{};
    while (f) {
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const std::streamsize n = f.gcount();
        if (n > 0) {
            EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(n));
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int  digest_len = 0;
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }
    return oss.str();
}

// ============================================================================
// Default HTTP fetch (thin shim that works without libcurl in the build)
// Real deployments replace this via setFetchFunction().
// ============================================================================

bool ParallelDownloader::defaultFetch(
    const std::string& url,
    const std::string& dest,
    uint64_t           resume_offset,
    long               /*connect_timeout_s*/,
    long               /*transfer_timeout_s*/,
    uint64_t*          out_bytes,
    uint64_t*          out_total,
    std::string*       out_error)
{
    // This default implementation is a no-op stub suitable for unit tests
    // that inject a custom FetchFn.  Production deployments provide their
    // own HTTP transport via setFetchFunction().
    (void)url;
    (void)dest;
    (void)resume_offset;
    if (out_bytes)  *out_bytes  = 0;
    if (out_total)  *out_total  = 0;
    if (out_error)  *out_error  = "No HTTP transport configured; call setFetchFunction()";
    return false;
}

// ============================================================================
// Per-task executor
// ============================================================================

DownloadResult ParallelDownloader::executeTask(
    size_t task_index, const DownloadTask& task)
{
    DownloadResult result;
    result.task = task;

    const auto t0 = std::chrono::steady_clock::now();

    // Ensure destination directory exists
    {
        const fs::path dest_dir = fs::path(task.dest).parent_path();
        if (!dest_dir.empty()) {
            std::error_code ec;
            fs::create_directories(dest_dir, ec);
            if (ec) {
                result.error_message =
                    "Cannot create destination directory: " + ec.message();
                result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - t0);
                return result;
            }
        }
    }

    // Determine resume offset
    uint64_t resume_offset = 0;
    if (task.enable_resume) {
        resume_offset = resumeOffset(task.dest);
        if (resume_offset > 0) {
            result.was_resumed = true;
            LOG_DEBUG("ParallelDownloader: resuming {} at offset {}", task.url, resume_offset);
        }
    }

    if (progress_cb_) {
        progress_cb_(task_index, resume_offset, 0, "downloading");
    }

    // Retry loop
    bool fetch_ok = false;
    for (int attempt = 0; attempt <= task.max_retries; ++attempt) {
        if (attempt > 0) {
            // Exponential back-off: 1 s, 2 s, 4 s …
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 * (1 << (attempt - 1))));
            LOG_DEBUG("ParallelDownloader: retry {}/{} for {}", attempt, task.max_retries, task.url);
        }

        uint64_t    bytes_this_call = 0;
        uint64_t    total_bytes     = 0;
        std::string fetch_error;

        fetch_ok = fetch_fn_(
            task.url,
            task.dest,
            resume_offset,
            connect_timeout_s_,
            transfer_timeout_s_,
            &bytes_this_call,
            &total_bytes,
            &fetch_error);

        result.bytes_downloaded += bytes_this_call;
        result.total_bytes       = total_bytes;
        result.retries_used      = attempt;

        // Account for bandwidth consumption
        if (bytes_this_call > 0) {
            consumeBandwidth(bytes_this_call);
        }

        if (progress_cb_) {
            progress_cb_(task_index, result.bytes_downloaded + resume_offset,
                         total_bytes, "downloading");
        }

        if (fetch_ok) {
            break;
        }
        result.error_message = fetch_error;
        LOG_WARN("ParallelDownloader: fetch failed for {} (attempt {}): {}",
                 task.url, attempt + 1, fetch_error);
    }

    if (!fetch_ok) {
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0);
        return result;
    }

    // Hash verification
    if (!task.expected_hash.empty()) {
        if (progress_cb_) {
            progress_cb_(task_index, result.bytes_downloaded + resume_offset,
                         result.total_bytes, "verifying");
        }
        const std::string actual_hash = computeSha256(task.dest);
        if (actual_hash.empty()) {
            result.error_message = "Hash computation failed for: " + task.dest;
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0);
            return result;
        }
        if (actual_hash != task.expected_hash) {
            result.error_message =
                "Hash mismatch for " + task.dest +
                ": expected " + task.expected_hash +
                ", got " + actual_hash;
            // Remove the corrupt file so a future attempt starts fresh
            std::error_code ec;
            fs::remove(task.dest, ec);
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0);
            return result;
        }
    }

    result.success  = true;
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0);

    if (progress_cb_) {
        progress_cb_(task_index, result.bytes_downloaded + resume_offset,
                     result.total_bytes, "done");
    }

    LOG_DEBUG("ParallelDownloader: completed {} ({} bytes)", task.url, result.bytes_downloaded);
    return result;
}

// ============================================================================
// downloadAll – priority-queue scheduler + thread pool
// ============================================================================

std::vector<DownloadResult> ParallelDownloader::downloadAll(
    std::vector<DownloadTask> tasks)
{
    const size_t n = tasks.size();
    std::vector<DownloadResult> results(n);

    if (n == 0) {
        last_stats_ = {};
        return results;
    }

    const auto batch_start = std::chrono::steady_clock::now();

    // Build a priority queue of (priority, original_index)
    // Higher priority → processed first.
    using Entry = std::pair<int, size_t>;  // (priority, index)
    std::priority_queue<Entry> pq;
    for (size_t i = 0; i < n; ++i) {
        pq.emplace(tasks[i].priority, i);
    }

    // Shared state for the worker pool
    std::mutex              queue_mutex;
    std::condition_variable cv;
    bool                    all_queued = false;

    // Launch up to concurrency_ workers
    const size_t workers = std::min(concurrency_, n);

    auto worker = [&]() {
        while (true) {
            size_t idx = SIZE_MAX;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                cv.wait(lock, [&]() { return !pq.empty() || all_queued; });
                if (pq.empty()) break;
                idx = pq.top().second;
                pq.pop();
            }
            if (idx == SIZE_MAX) break;

            results[idx] = executeTask(idx, tasks[idx]);
        }
    };

    // Mark all tasks as queued (the queue was pre-loaded)
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        all_queued = true;
    }
    cv.notify_all();  // wake workers that might be waiting

    std::vector<std::thread> threads;
    threads.reserve(workers);
    for (size_t w = 0; w < workers; ++w) {
        threads.emplace_back(worker);
    }
    // Also notify after threads are launched (in case they start before notify_all)
    cv.notify_all();

    for (auto& t : threads) {
        t.join();
    }

    // Aggregate stats
    DownloadBatchStats stats;
    stats.total_tasks = n;
    stats.wall_time   = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - batch_start);

    for (const auto& r : results) {
        if (r.success) {
            ++stats.succeeded;
            stats.total_bytes += r.bytes_downloaded;
        } else {
            ++stats.failed;
        }
        if (r.was_resumed) ++stats.resumed;
    }
    last_stats_ = stats;

    LOG_INFO("ParallelDownloader: batch complete – {}/{} succeeded, {} bytes in {} ms",
             stats.succeeded, stats.total_tasks,
             stats.total_bytes, stats.wall_time.count());

    return results;
}

} // namespace updates
} // namespace themis
