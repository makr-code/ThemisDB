/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            transaction_batcher.cpp                           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-03-18                                         ║
  Author:          Copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~320                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "transaction/transaction_batcher.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdexcept>

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

    // When adaptive is enabled, override window with the adaptive window
    // (unless a per-table window was explicitly set, to honour table overrides).
    if (config_.enable_adaptive && table.empty()) {
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

    PendingEntry entry;
    entry.commit_fn    = std::move(commit_fn);
    entry.table_hint   = table_hint;
    entry.submitted_at = std::chrono::steady_clock::now();

    auto future = entry.promise.get_future();

    bool need_immediate_flush = false;
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        queue_.push_back(std::move(entry));

        // Check whether the queue has hit max_batch_size for the table hint.
        std::lock_guard<std::mutex> cfg_lk(config_mutex_);
        auto ep = effectivePolicyFor(table_hint);
        if (queue_.size() >= ep.max_batch_size) {
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
    // Signal an immediate flush and wait until the queue is drained.
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        flush_requested_ = true;
    }
    queue_cv_.notify_all();

    // Spin-wait until the queue is empty.  This is safe because the flush thread
    // drains under queue_mutex_ and we poll without holding it.
    while (true) {
        {
            std::lock_guard<std::mutex> lk(queue_mutex_);
            if (queue_.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
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
        // ── Determine the window to wait ──────────────────────────────────────
        std::chrono::microseconds window;
        {
            std::lock_guard<std::mutex> cfg_lk(config_mutex_);
            window = config_.enable_adaptive ? adaptive_window_ : config_.window;
        }

        // ── Wait for the window to expire or an early-flush trigger ──────────
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);
            queue_cv_.wait_for(lk, window, [this] {
                return flush_requested_ || stopping_.load(std::memory_order_acquire);
            });
            flush_requested_ = false;
        }

        // ── Drain the queue ───────────────────────────────────────────────────
        std::vector<PendingEntry> batch;
        {
            std::lock_guard<std::mutex> lk(queue_mutex_);
            while (!queue_.empty()) {
                batch.push_back(std::move(queue_.front()));
                queue_.pop_front();
            }
        }

        if (!batch.empty()) {
            executeBatch(batch);
        }

        // ── Exit after draining when stopping ────────────────────────────────
        if (stopping_.load(std::memory_order_acquire)) {
            // Final drain pass to resolve remaining pending items.
            std::vector<PendingEntry> remainder;
            {
                std::lock_guard<std::mutex> lk(queue_mutex_);
                while (!queue_.empty()) {
                    remainder.push_back(std::move(queue_.front()));
                    queue_.pop_front();
                }
            }
            if (!remainder.empty()) {
                executeBatch(remainder);
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
        } catch (...) {
            st = Status::Error("unknown exception in commit_fn");
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
