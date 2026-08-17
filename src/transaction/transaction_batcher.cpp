/**
 * @file transaction_batcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "transaction/transaction_batcher.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdexcept>
#include "utils/logger.h"

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

static constexpr std::chrono::microseconds kMinWindow{1000};   // 1 ms
static constexpr std::chrono::microseconds kMaxWindow{100000}; // 100 ms

// Maximum number of throughput samples kept for adaptive window adjustment.
static constexpr size_t kMaxThroughputSamples = 20;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

TransactionBatcher::TransactionBatcher()
    : config_{}
    , adaptive_window_{config_.window}
{
    flush_thread_ = std::thread(&TransactionBatcher::flushLoop, this);
}

TransactionBatcher::~TransactionBatcher()
{
    // Signal the background thread to stop and wake it up.
    stopping_.store(true, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        flush_requested_ = true;
    }
    queue_cv_.notify_all();

    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

void TransactionBatcher::setBatchConfig(const BatchConfig& config)
{
    BatchConfig clamped = config;

    // Clamp window to [1 ms, 100 ms]
    if (clamped.window < kMinWindow) clamped.window = kMinWindow;
    if (clamped.window > kMaxWindow) clamped.window = kMaxWindow;

    // Clamp sizes to >= 1
    if (clamped.max_batch_size < 1) clamped.max_batch_size = 1;
    if (clamped.min_batch_size < 1) clamped.min_batch_size = 1;
    if (clamped.min_batch_size > clamped.max_batch_size)
        clamped.min_batch_size = clamped.max_batch_size;

    std::lock_guard<std::mutex> lk(config_mutex_);
    config_          = clamped;
    adaptive_window_ = clamped.window;
}

TransactionBatcher::BatchConfig TransactionBatcher::getBatchConfig() const
{
    std::lock_guard<std::mutex> lk(config_mutex_);
    return config_;
}

void TransactionBatcher::setTablePolicy(const std::string& table,
                                        const BatchPolicy& policy)
{
    std::lock_guard<std::mutex> lk(config_mutex_);
    table_policies_[table] = policy;
}

TransactionBatcher::BatchPolicy
TransactionBatcher::getTablePolicy(const std::string& table) const
{
    std::lock_guard<std::mutex> lk(config_mutex_);
    auto it = table_policies_.find(table);
    if (it != table_policies_.end()) return it->second;
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// effectivePolicyFor
// ─────────────────────────────────────────────────────────────────────────────

TransactionBatcher::EffectivePolicy
TransactionBatcher::effectivePolicyFor(const std::string& table) const
{
    // config_mutex_ must already be held by the caller.
    EffectivePolicy ep{
        config_.window,
        config_.max_batch_size,
        config_.min_batch_size
    };

    if (!table.empty()) {
        auto it = table_policies_.find(table);
        if (it != table_policies_.end()) {
            const BatchPolicy& pol = it->second;
            if (pol.window.count() > 0) {
                auto w = pol.window;
                if (w < kMinWindow) w = kMinWindow;
                if (w > kMaxWindow) w = kMaxWindow;
                ep.window = w;
            }
            if (pol.max_batch_size > 0) ep.max_batch_size = pol.max_batch_size;
            if (pol.min_batch_size > 0) ep.min_batch_size = pol.min_batch_size;
        }
    }

    // Clamp size invariants after applying per-table overrides.
    if (ep.max_batch_size < 1) ep.max_batch_size = 1;
    if (ep.min_batch_size < 1) ep.min_batch_size = 1;
    if (ep.min_batch_size > ep.max_batch_size)
        ep.min_batch_size = ep.max_batch_size;

    // Apply adaptive window unless the table has an explicit window override.
    bool has_table_window = false;
    if (!table.empty()) {
        auto it2 = table_policies_.find(table);
        has_table_window = it2 != table_policies_.end() && it2->second.window.count() > 0;
    }
    if (config_.enable_adaptive && !has_table_window) {
        ep.window = adaptive_window_;
    }

    return ep;
}

// ─────────────────────────────────────────────────────────────────────────────
// submitAsync
// ─────────────────────────────────────────────────────────────────────────────

std::future<TransactionBatcher::Status>
TransactionBatcher::submitAsync(std::function<Status()> commit_fn,
                                 const std::string& table_hint)
{
    if (!commit_fn) {
        std::promise<Status> p;
        p.set_value(Status::Error("commit_fn must not be null"));
        return p.get_future();
    }

    if (stopping_.load(std::memory_order_acquire)) {
        std::promise<Status> p;
        p.set_value(Status::Error("batcher is shutting down"));
        return p.get_future();
    }

    // ── Phase 1: resolve effective policy under config_mutex_ only ───────────
    std::chrono::microseconds effective_window;
    size_t effective_max_batch_size;
    {
        std::lock_guard<std::mutex> cfg_lk(config_mutex_);
        auto ep                  = effectivePolicyFor(table_hint);
        effective_window         = ep.window;
        effective_max_batch_size = ep.max_batch_size;
    }

    PendingEntry entry;
    entry.commit_fn    = std::move(commit_fn);
    entry.table_hint   = table_hint;
    entry.submitted_at = std::chrono::steady_clock::now();
    entry.deadline     = entry.submitted_at + effective_window;

    auto future = entry.promise.get_future();

    // ── Phase 2: push to queue under queue_mutex_ only (no nested lock) ──────
    bool need_immediate_flush = false;
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        queue_.push_back(std::move(entry));

        if (queue_.size() >= effective_max_batch_size) {
            need_immediate_flush = true;
            flush_requested_     = true;
        }
    }

    queue_cv_.notify_one();
    if (need_immediate_flush) {
        queue_cv_.notify_all();
    }

    return future;
}

// ─────────────────────────────────────────────────────────────────────────────
// flush
// ─────────────────────────────────────────────────────────────────────────────

void TransactionBatcher::flush()
{
    // Signal an immediate flush.
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        flush_requested_ = true;
    }
    queue_cv_.notify_all();

    // Wait until the queue is empty AND the in-flight batch (if any) has finished.
    // This ensures all items submitted before flush() was called are fully resolved.
    // Use a 30-second timeout to prevent indefinite blocking.
    std::unique_lock<std::mutex> lk(queue_mutex_);
    const bool flushed = flush_cv_.wait_for(lk, std::chrono::seconds(30), [this] {
        return queue_.empty() && !batch_in_progress_;
    });
    
    if (!flushed) {
        THEMIS_WARN("TransactionBatcher::flush(): timeout waiting for queue to flush after 30s");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// getStats
// ─────────────────────────────────────────────────────────────────────────────

TransactionBatcher::Stats TransactionBatcher::getStats() const
{
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

// ─────────────────────────────────────────────────────────────────────────────
// flushLoop  (background thread)
// ─────────────────────────────────────────────────────────────────────────────

void TransactionBatcher::flushLoop()
{
    while (true) {
        // ── Determine the idle-fallback window from config ────────────────────
        std::chrono::microseconds idle_window;
        {
            std::lock_guard<std::mutex> cfg_lk(config_mutex_);
            idle_window = config_.enable_adaptive ? adaptive_window_ : config_.window;
        }

        // ── Wait until the earliest item deadline, an explicit flush, or stop ─
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);

            // On each (re)wakeup recompute the nearest deadline so newly-added
            // items with shorter per-table windows are honoured promptly.
            auto compute_wake = [&]() -> std::chrono::steady_clock::time_point {
                auto w = std::chrono::steady_clock::now() + idle_window;
                for (const auto& e : queue_) {
                    if (e.deadline < w) w = e.deadline;
                }
                return w;
            };

            auto wake_at = compute_wake();

            // Loop on spurious wakeups so new items can update wake_at.
            while (!flush_requested_ &&
                   !stopping_.load(std::memory_order_acquire)) {
                auto status = queue_cv_.wait_until(lk, wake_at);
                if (status == std::cv_status::timeout) break;
                // Notified: recompute in case new items have earlier deadlines.
                wake_at = compute_wake();
                if (std::chrono::steady_clock::now() >= wake_at) break;
            }
            flush_requested_ = false;
        }

        // ── Drain the queue into a local batch ───────────────────────────────
        std::vector<PendingEntry> batch;
        {
            std::lock_guard<std::mutex> lk(queue_mutex_);
            while (!queue_.empty()) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
            // Mark in-flight BEFORE releasing queue_mutex_ so flush() cannot
            // observe queue_.empty() && !batch_in_progress_ prematurely.
            if (!batch.empty()) {
                batch_in_progress_ = true;
            }
        }

        if (!batch.empty()) {
            executeBatch(batch);
            {
                std::lock_guard<std::mutex> lk(queue_mutex_);
                batch_in_progress_ = false;
            }
            flush_cv_.notify_all();
        }

        // ── Exit after draining when stopping ────────────────────────────────
        if (stopping_.load(std::memory_order_acquire)) {
            // Final drain pass to resolve any items added between last drain and now.
            std::vector<PendingEntry> remainder;
            {
                std::lock_guard<std::mutex> lk(queue_mutex_);
                while (!queue_.empty()) {
                    remainder.push_back(std::move(queue_.front()));
                    queue_.pop_front();
                }
                if (!remainder.empty()) {
                    batch_in_progress_ = true;
                }
            }
            if (!remainder.empty()) {
                executeBatch(remainder);
                {
                    std::lock_guard<std::mutex> lk(queue_mutex_);
                    batch_in_progress_ = false;
                }
                flush_cv_.notify_all();
            }
            break;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// executeBatch
// ─────────────────────────────────────────────────────────────────────────────

void TransactionBatcher::executeBatch(std::vector<PendingEntry>& batch)
{
    if (batch.empty()) return;

    auto batch_start = std::chrono::steady_clock::now();
    size_t committed = 0;
    size_t failed    = 0;

    for (auto& entry : batch) {
        Status st = Status::Error("unknown error");
        try {
            st = entry.commit_fn();
        } catch (const std::exception& ex) {
            st = Status::Error(std::string("exception in commit_fn: ") + ex.what());
        } catch (const std::string& ex) {
            st = Status::Error(std::string("exception in commit_fn: ") + ex);
        } catch (const char* ex) {
            st = Status::Error(std::string("exception in commit_fn: ") +
                               std::string(ex ? ex : "<null>"));
        }

        if (st.ok) ++committed; else ++failed;

        // Resolve the per-item latency (submission → resolution).
        auto now = std::chrono::steady_clock::now();
        double item_lat_ms =
            std::chrono::duration<double, std::milli>(now - entry.submitted_at).count();

        entry.promise.set_value(st);

        // Accumulate per-item latency into stats.
        {
            std::lock_guard<std::mutex> slk(stats_mutex_);
            double n = static_cast<double>(
                stats_.transactions_committed + stats_.transactions_failed + 1);
            stats_.avg_latency_ms =
                stats_.avg_latency_ms + (item_lat_ms - stats_.avg_latency_ms) / n;
        }
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - batch_start);

    // Update aggregate stats.
    {
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.batches_flushed;
        stats_.transactions_committed += committed;
        stats_.transactions_failed    += failed;

        double sz = static_cast<double>(batch.size());
        double n  = static_cast<double>(stats_.batches_flushed);
        stats_.avg_batch_size = stats_.avg_batch_size + (sz - stats_.avg_batch_size) / n;
    }

    // Adaptive window adjustment.
    {
        std::lock_guard<std::mutex> cfg_lk(config_mutex_);
        if (config_.enable_adaptive) {
            adaptWindow(batch.size(), elapsed);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// adaptWindow  (called under config_mutex_)
// ─────────────────────────────────────────────────────────────────────────────

void TransactionBatcher::adaptWindow(size_t batch_size,
                                      std::chrono::microseconds elapsed)
{
    // Compute instantaneous throughput (items / second).
    double elapsed_sec = elapsed.count() > 0
        ? static_cast<double>(elapsed.count()) * 1e-6
        : 1e-6;
    double throughput = static_cast<double>(batch_size) / elapsed_sec;

    recent_throughputs_.push_back(throughput);
    if (recent_throughputs_.size() > kMaxThroughputSamples)
        recent_throughputs_.pop_front();

    double avg_throughput = std::accumulate(
        recent_throughputs_.begin(), recent_throughputs_.end(), 0.0)
        / static_cast<double>(recent_throughputs_.size());

    // Target: fill at least min_batch_size items per batch on average.
    // If avg throughput implies more items arriving per window than min_batch_size,
    // the window is well-sized.  Otherwise widen or narrow.
    double target_batch = static_cast<double>(config_.min_batch_size);
    double current_window_sec =
        static_cast<double>(adaptive_window_.count()) * 1e-6;

    // Predicted items in current window given current throughput.
    double predicted = avg_throughput * current_window_sec;

    std::chrono::microseconds new_window = adaptive_window_;

    if (predicted < target_batch * 0.5) {
        // Low load: widen the window slightly (+10%) to gather more items and
        // reduce unnecessary flush cycles.
        auto wider = static_cast<long long>(adaptive_window_.count() * 1.10);
        new_window = std::chrono::microseconds(wider);
    } else if (predicted > static_cast<double>(config_.max_batch_size) * 0.9) {
        // Near-overflow load: narrow the window (-10%) to keep batches below
        // max_batch_size and reduce per-item latency.
        auto narrower = static_cast<long long>(adaptive_window_.count() * 0.90);
        new_window = std::chrono::microseconds(narrower);
    }

    // Clamp to configured global bounds.
    if (new_window < kMinWindow) new_window = kMinWindow;
    if (new_window > kMaxWindow) new_window = kMaxWindow;

    if (new_window != adaptive_window_) {
        adaptive_window_ = new_window;
        std::lock_guard<std::mutex> slk(stats_mutex_);
        ++stats_.adaptive_adjustments;
    }
}

} // namespace themis

