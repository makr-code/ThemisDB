/*
 * ThemisDB | File: parallel_downloader.cpp | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 547
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=108 | delta=105 | status=divergent
 * External Severity (v3): C=9, H=86, M=13
 * PR: #5165 Wave A ML Enhancements: Phase 2 Production-Ready Implementation + F... (2026-05-20T15:19:21Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "updates/parallel_downloader.h"
#include "utils/logger.h"
#include "utils/retry_policy.h"

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
#ifdef THEMIS_ENABLE_CURL
#  include <curl/curl.h>
#endif

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
    long               connect_timeout_s,
    long               transfer_timeout_s,
    uint64_t*          out_bytes,
    uint64_t*          out_total,
    std::string*       out_error)
{
    if (out_bytes)  *out_bytes  = 0;
    if (out_total)  *out_total  = 0;

#ifdef THEMIS_ENABLE_CURL
    // ── libcurl-backed HTTP/HTTPS fetch with optional byte-range resume ────
    struct WriteCtx {
        FILE*     fp;
        uint64_t  written = 0;
    };

    auto write_cb = [](char* ptr, size_t sz, size_t nmemb, void* ud) -> size_t {
        auto* ctx = static_cast<WriteCtx*>(ud);
        size_t n = fwrite(ptr, sz, nmemb, ctx->fp);
        ctx->written += n;
        return n;
    };

    const char* open_mode = (resume_offset > 0) ? "ab" : "wb";
    FILE* fp = fopen(dest.c_str(), open_mode);
    if (!fp) {
        if (out_error) *out_error = "Failed to open destination file: " + dest;
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        if (out_error) *out_error = "curl_easy_init() failed";
        return false;
    }

    WriteCtx ctx{fp};
    curl_easy_setopt(curl, CURLOPT_URL,             url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   static_cast<curl_write_callback>(write_cb));
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &ctx);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,  1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,  connect_timeout_s);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,         transfer_timeout_s);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,       "ThemisDB-ParallelDownloader/1.0");
    if (resume_offset > 0) {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE,
                         static_cast<curl_off_t>(resume_offset));
    }

    CURLcode res = curl_easy_perform(curl);

    // Retrieve content-length for out_total
    curl_off_t cl = -1;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);

    curl_easy_cleanup(curl);
    fclose(fp);

    if (res != CURLE_OK) {
        if (out_error) *out_error = std::string("curl error: ") + curl_easy_strerror(res);
        fs::remove(dest);
        return false;
    }

    if (out_bytes) *out_bytes = ctx.written;
    if (out_total) *out_total = (cl >= 0) ? static_cast<uint64_t>(cl) + resume_offset
                                           : ctx.written + resume_offset;
    return true;
#else
    if (out_error) *out_error = "No HTTP transport: build with -DTHEMIS_ENABLE_CURL=ON "
                                "or inject a custom FetchFn via setFetchFunction()";
    return false;
#endif
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
    
    // Use centralized exponential backoff policy (Phase 2a consolidation)
    const themis::utils::RetryConfig retry_cfg{
        .max_attempts       = static_cast<uint32_t>(task.max_retries + 1),
        .initial_backoff_ms = 1000,  // 1 second
        .max_backoff_ms     = 30'000,
        .multiplier         = 2.0,
        .jitter_fraction    = 0.0,
    };
    themis::utils::ExponentialBackoff backoff(retry_cfg);
    
    for (int attempt = 0; attempt <= task.max_retries; ++attempt) {
        if (attempt > 0) {
            LOG_DEBUG("ParallelDownloader: retry {}/{} for {}", attempt, task.max_retries, task.url);
            if (!backoff.wait()) break;
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

