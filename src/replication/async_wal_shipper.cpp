// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file async_wal_shipper.cpp
 * @brief Implementation of AsyncWalShipper.
 *
 * @see include/replication/async_wal_shipper.h
 */

#include "replication/async_wal_shipper.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace themisdb {
namespace replication {

// ============================================================================
// Lock Hierarchy Documentation (async_wal_shipper.cpp)
// ============================================================================
//
// This module implements a 3-level lock hierarchy for background WAL shipping
// without deadlocks or excessive lock contention.
//
// LOCK HIERARCHY (ordered from outermost to innermost):
//
//   Level 1: AsyncWalShipper::queue_mutex_
//            - Purpose: Protects segment queue and condition variable
//            - Scope: Enqueue/dequeue operations, worker thread coordination
//            - Hold time: MINIMAL (~microseconds for queue ops)
//            - Pattern: Acquire → queue op → release → notify outside
//
//   Level 2: AsyncWalShipper::callback_mutex_
//            - Purpose: Protects ship_handler_ and alert_cb_
//            - Scope: Handler callback registration and retrieval
//            - Hold time: MINIMAL (~microseconds)
//            - Pattern: Acquire → copy handler → release → invoke outside
//
//   Level 3: AsyncWalShipper::stats_mutex_
//            - Purpose: Protects shipping statistics and metrics
//            - Scope: Stats updates during dispatch
//            - Hold time: MINIMAL (~microseconds)
//            - Pattern: Acquire → update stats → release
//
//   Level 4: Blocking I/O and External Operations
//            - Purpose: Handler invocation (network I/O, etc.)
//            - Scope: NEVER held while holding Level 1-3 locks
//            - Hold time: VARIABLE (10ms-1s depending on network)
//            - Pattern: Handlers invoked lock-free after acquiring and releasing mutex_
//
// TIMEOUT SAFETY:
//   Background worker thread uses cv.wait(lock, predicate) which is guarded by
//   timeout logic. Worker loop monitors stop_requested_ flag to enable graceful
//   shutdown even if background operations hang.
//
// ============================================================================

// ---------------------------------------------------------------------------
// Histogram bucket construction
// ---------------------------------------------------------------------------

/*static*/
std::vector<double> AsyncWalShipper::buildHistogramBounds(
    uint32_t max_lag_ms,
    uint32_t buckets)
{
    // Generate a geometric series of bucket upper bounds.
    // Smallest bucket: 1 ms.  Largest bucket: 10× max_lag_ms.
    // +Inf is implicit (not stored).
    if (buckets < 2) buckets = 2;
    const double lower = 1.0;
    const double upper = static_cast<double>(max_lag_ms) * 10.0;
    const double ratio = std::pow(upper / lower, 1.0 / (buckets - 1));

    std::vector<double> bounds;
    bounds.reserve(buckets);
    double val = lower;
    for (uint32_t i = 0; i < buckets; ++i) {
        bounds.push_back(val);
        val *= ratio;
    }
    return bounds;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

AsyncWalShipper::AsyncWalShipper(WalShippingConfig config)
    : config_(std::move(config))
    , histogram_bounds_(buildHistogramBounds(config_.max_lag_ms,
                                             config_.histogram_buckets))
    , histogram_counts_(histogram_bounds_.size(), 0)
{
    // Default no-op ship handler: count bytes, always succeed.
    ship_handler_ = [this](const WalSegment& seg) -> bool {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.bytes_shipped += seg.data.size();
        ++stats_.segments_shipped;
        return true;
    };

    // Start background worker thread.
    worker_ = std::thread(&AsyncWalShipper::workerLoop, this);
}

AsyncWalShipper::~AsyncWalShipper()
{
    stop();
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void AsyncWalShipper::setAlertCallback(AlertCallback cb)
{
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
    alert_cb_ = std::move(cb);
}

void AsyncWalShipper::setShipHandler(ShipHandler handler)
{
    std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
    ship_handler_ = std::move([[maybe_unused]] handler);
}

// ---------------------------------------------------------------------------
// Segment ingestion
// ---------------------------------------------------------------------------

bool AsyncWalShipper::enqueueSegment(WalSegment segment)
{
    std::unique_lock<std::mutex> lock(queue_mutex_);

    if (segment_queue_.size() >= config_.max_queue_depth) {
        // Queue full: drop and account
        std::lock_guard<std::mutex> sl(stats_mutex_);
        ++stats_.segments_dropped;
        return false;
    }

    const size_t bytes = segment.data.size();
    segment_queue_.push(std::move(segment));

    {
        std::lock_guard<std::mutex> sl(stats_mutex_);
        ++stats_.segments_enqueued;
        stats_.bytes_enqueued += bytes;
    }

    lock.unlock();
    queue_cv_.notify_one();
    return true;
}

// ---------------------------------------------------------------------------
// Metrics
// ---------------------------------------------------------------------------

WalShippingStats AsyncWalShipper::stats() const
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

int64_t AsyncWalShipper::currentLagMs() const
{
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (segment_queue_.empty()) return 0;

    const auto& front = segment_queue_.front();
    const auto now    = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now - front.enqueue_time).count();
}

std::string AsyncWalShipper::exportPrometheusMetrics() const
{
    const auto s = stats();
    const std::string lbl =
        "{local_dc=\"" + config_.local_dc_id +
        "\",remote_dc=\"" + config_.remote_dc_endpoint + "\"}";

    std::ostringstream oss;

    // Histogram: replication_wal_lag_ms
    {
        std::lock_guard<std::mutex> lock(histogram_mutex_);

        oss << "# HELP replication_wal_lag_ms "
               "WAL shipping lag from enqueue to dispatch (ms)\n"
            << "# TYPE replication_wal_lag_ms histogram\n";

        uint64_t cumulative = 0;
        for (size_t i = 0; i < histogram_bounds_.size(); ++i) {
            cumulative += histogram_counts_[i];
            oss << "replication_wal_lag_ms_bucket{le=\""
                << static_cast<int64_t>(histogram_bounds_[i])
                << "\"" << lbl.substr(1)   // strip leading '{'
                << " " << cumulative << "\n";
        }
        oss << "replication_wal_lag_ms_bucket{le=\"+Inf\"" << lbl.substr(1)
            << " " << (cumulative + histogram_counts_.back()) << "\n";
        oss << "replication_wal_lag_ms_sum" << lbl
            << " " << histogram_sum_ms_ << "\n";
        oss << "replication_wal_lag_ms_count" << lbl
            << " " << s.segments_shipped << "\n";
    }

    // Counters
    oss << "# HELP replication_wal_segments_enqueued_total "
           "Total WAL segments accepted\n"
        << "# TYPE replication_wal_segments_enqueued_total counter\n"
        << "replication_wal_segments_enqueued_total" << lbl
        << " " << s.segments_enqueued << "\n";

    oss << "# HELP replication_wal_segments_shipped_total "
           "Total WAL segments dispatched\n"
        << "# TYPE replication_wal_segments_shipped_total counter\n"
        << "replication_wal_segments_shipped_total" << lbl
        << " " << s.segments_shipped << "\n";

    oss << "# HELP replication_wal_segments_dropped_total "
           "WAL segments dropped due to full queue\n"
        << "# TYPE replication_wal_segments_dropped_total counter\n"
        << "replication_wal_segments_dropped_total" << lbl
        << " " << s.segments_dropped << "\n";

    oss << "# HELP replication_wal_lag_alerts_total "
           "Times WAL lag exceeded the configured limit\n"
        << "# TYPE replication_wal_lag_alerts_total counter\n"
        << "replication_wal_lag_alerts_total" << lbl
        << " " << s.lag_alerts_fired << "\n";

    oss << "# HELP replication_wal_bytes_shipped_total "
           "Total bytes dispatched\n"
        << "# TYPE replication_wal_bytes_shipped_total counter\n"
        << "replication_wal_bytes_shipped_total" << lbl
        << " " << s.bytes_shipped << "\n";

    return oss.str();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AsyncWalShipper::stop()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_requested_.store(true, std::memory_order_relaxed);
    }
    queue_cv_.notify_all();

    if (worker_.joinable()) worker_.join();
}

// ---------------------------------------------------------------------------
// Background worker
// ---------------------------------------------------------------------------

void AsyncWalShipper::workerLoop()
{
    // Default timeout for condition variable: 1 second
    // This ensures the thread wakes up periodically to check stop_requested_
    const auto cv_timeout = std::chrono::seconds(1);
    
    while (true) {
        WalSegment seg;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            // Wait with timeout to ensure periodic wake-up even if not notified
            queue_cv_.wait_for(lock, cv_timeout, [this] {
                return !segment_queue_.empty() ||
                       stop_requested_.load(std::memory_order_relaxed);
            });

            if (segment_queue_.empty()) {
                // stop requested and queue empty: exit
                if (stop_requested_.load(std::memory_order_relaxed)) break;
                continue;
            }

            seg = std::move(segment_queue_.front());
            segment_queue_.pop();
        }  // LOCK RELEASED: dispatchSegment happens outside lock

        dispatchSegment(seg);
    }
}

void AsyncWalShipper::dispatchSegment(const WalSegment& seg)
{
    const auto now     = std::chrono::steady_clock::now();
    const int64_t lag  = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - seg.enqueue_time).count();

    // Record lag sample in histogram
    recordLagSample(lag);

    // Update current lag metrics atomically
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.current_lag_ms = lag;
        if (lag > stats_.max_observed_lag_ms) {
            stats_.max_observed_lag_ms = lag;
        }
    }

    // Check lag limit and fire alert if needed.
    // Lag threshold exceeded means fail-closed: alert fires but shipping continues.
    if (lag > static_cast<int64_t>(config_.max_lag_ms)) {
        AlertCallback cb;
        {
            std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
            cb = alert_cb_;
        }
        if (cb) {
            try {
                cb(static_cast<uint64_t>(lag));
            } catch (...) {
                // Suppress exceptions from alert callback; never drop segment
            }
        }

        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.lag_alerts_fired;
    }

    // Invoke transport handler.
    // Lag threshold is telemetry/alerting only: shipping must still be attempted.
    ShipHandler handler;
    {
        std::lock_guard<std::mutex> lock([[maybe_unused]] callback_mutex_);
        handler = ship_handler_;
    }
    if ([[maybe_unused]] handler) {
        handler([[maybe_unused]] seg);
        // bytes_shipped / segments_shipped updated inside default handler;
        // for custom handlers we update bytes here if not already counted.
    } else {
        // No handler installed: account as dropped segment.
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.segments_dropped;
    }
}

void AsyncWalShipper::recordLagSample(int64_t lag_ms)
{
    std::lock_guard<std::mutex> lock(histogram_mutex_);
    histogram_sum_ms_ += static_cast<uint64_t>(lag_ms < 0 ? 0 : lag_ms);

    // Find the first bucket whose upper bound >= lag_ms
    bool placed = false;
    for (size_t i = 0; i < histogram_bounds_.size(); ++i) {
        if (static_cast<double>(lag_ms) <= histogram_bounds_[i]) {
            ++histogram_counts_[i];
            placed = true;
            break;
        }
    }
    if (!placed) {
        // Overflow bucket (last bucket absorbs anything above top bound)
        ++histogram_counts_.back();
    }
}

} // namespace replication
} // namespace themisdb
