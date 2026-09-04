/**
 * @file query_scheduler.cpp
 * @brief Phase 3 P3-04-C/D: SLA-aware query scheduler — implementation.
 * @version 1.0.0
 * @note Status: Block B P3-04-C/D delivery
 */

#include "execution/query_scheduler.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace themis::execution {

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

QueryScheduler::QueryScheduler()
    : QueryScheduler(Config{}) {}

QueryScheduler::QueryScheduler(const Config& cfg)
    : cfg_(cfg) {}

QueryScheduler::~QueryScheduler() {
    shutdown();
}

// ---------------------------------------------------------------------------
// enqueue
// ---------------------------------------------------------------------------

std::uint64_t QueryScheduler::enqueue(
    QueryEntry::ExecuteFn execute,
    SLAPriority           priority,
    long                  sla_ms,
    std::string           name,
    std::chrono::milliseconds timeout) {
    if (shutdown_.load(std::memory_order_acquire)) {
        return 0;
    }

    const auto t0       = std::chrono::steady_clock::now();
    const auto deadline = t0 + std::chrono::milliseconds(sla_ms);
    const auto deadline_abs = std::chrono::steady_clock::now() + timeout;

    std::unique_lock<std::mutex> lk(mutex_);
    const bool ok = enqueue_cv_.wait_until(lk, deadline_abs, [this] {
        return static_cast<int>(queue_.size()) < cfg_.max_queue_depth ||
               shutdown_.load(std::memory_order_relaxed);
    });

    if (!ok || shutdown_.load(std::memory_order_relaxed)) {
        return 0;
    }

    // Load shedding: if at shed threshold, drop the lowest-priority entry
    // (we can't easily remove from the middle of std::priority_queue, so
    // we simply reject the incoming low-priority query instead).
    if (static_cast<int>(queue_.size()) >= cfg_.shed_threshold && priority == SLAPriority::LOW) {
        ++total_shed_;
        return 0;
    }

    const std::uint64_t id = next_id_.fetch_add(1, std::memory_order_relaxed);

    QueryEntry entry;
    entry.id           = id;
    entry.priority     = priority;
    entry.deadline     = deadline;
    entry.enqueue_time = t0;
    entry.execute      = std::move(execute);
    entry.name         = std::move(name);

    pending_deadlines_[id] = deadline;
    queue_.push(std::move(entry));
    ++total_enqueued_;

    // Track per-priority depth.
    switch (priority) {
        case SLAPriority::HIGH:   ++count_high_;   break;
        case SLAPriority::MEDIUM: ++count_medium_;  break;
        case SLAPriority::LOW:    ++count_low_;     break;
        default:                                   break;
    }

    const double latency_us = std::chrono::duration<double, std::micro>(
        std::chrono::steady_clock::now() - t0).count();
    enqueue_latency_sum_ += latency_us;

    lk.unlock();
    dequeue_cv_.notify_one();
    return id;
}

// ---------------------------------------------------------------------------
// dequeue
// ---------------------------------------------------------------------------

bool QueryScheduler::dequeue(QueryEntry& out, std::chrono::milliseconds timeout) {
    const auto t0 = std::chrono::steady_clock::now();
    const auto deadline = t0 + timeout;

    std::unique_lock<std::mutex> lk(mutex_);
    const bool ok = dequeue_cv_.wait_until(lk, deadline, [this] {
        return !queue_.empty() || shutdown_.load(std::memory_order_relaxed);
    });

    if (!ok || queue_.empty()) {
        return false;
    }
    if (shutdown_.load(std::memory_order_relaxed) && queue_.empty()) {
        return false;
    }

    // std::priority_queue exposes only the top; we take it directly.
    out = std::move(const_cast<QueryEntry&>(queue_.top()));
    queue_.pop();
    ++total_dequeued_;

    // Decrement per-priority depth counter.
    switch (out.priority) {
        case SLAPriority::HIGH:   if (count_high_   > 0) --count_high_;   break;
        case SLAPriority::MEDIUM: if (count_medium_ > 0) --count_medium_; break;
        case SLAPriority::LOW:    if (count_low_    > 0) --count_low_;    break;
        default:                                                           break;
    }

    const double latency_us = std::chrono::duration<double, std::micro>(
        std::chrono::steady_clock::now() - t0).count();
    dequeue_latency_sum_ += latency_us;

    lk.unlock();
    enqueue_cv_.notify_one();
    return true;
}

// ---------------------------------------------------------------------------
// reportCompletion
// ---------------------------------------------------------------------------

void QueryScheduler::reportCompletion(
    std::uint64_t query_id,
    std::chrono::steady_clock::time_point completion_time) {
    std::lock_guard<std::mutex> lk(mutex_);
    ++completed_total_;
    auto it = pending_deadlines_.find(query_id);
    if (it != pending_deadlines_.end()) {
        if (completion_time <= it->second) {
            ++completed_in_sla_;
        }
        pending_deadlines_.erase(it);
    }
}

// ---------------------------------------------------------------------------
// metrics
// ---------------------------------------------------------------------------

QueryScheduler::Metrics QueryScheduler::metrics() const noexcept {
    Metrics m;
    std::lock_guard<std::mutex> lk(mutex_);
    m.queue_depth_high   = count_high_;
    m.queue_depth_medium = count_medium_;
    m.queue_depth_low    = count_low_;

    m.total_enqueued    = total_enqueued_;
    m.total_dequeued    = total_dequeued_;
    m.total_shed        = total_shed_;
    m.completed_in_sla  = completed_in_sla_;
    m.completed_total   = completed_total_;
    m.sla_compliance_pct = completed_total_ > 0
        ? 100.0 * static_cast<double>(completed_in_sla_) /
          static_cast<double>(completed_total_)
        : 0.0;
    m.avg_enqueue_us = total_enqueued_ > 0
        ? enqueue_latency_sum_ / static_cast<double>(total_enqueued_)
        : 0.0;
    m.avg_dequeue_us = total_dequeued_ > 0
        ? dequeue_latency_sum_ / static_cast<double>(total_dequeued_)
        : 0.0;
    return m;
}

// ---------------------------------------------------------------------------
// size / shutdown
// ---------------------------------------------------------------------------

std::size_t QueryScheduler::size() const noexcept {
    std::lock_guard<std::mutex> lk(mutex_);
    return static_cast<int>(queue_.size());
}

void QueryScheduler::shutdown() noexcept {
    shutdown_.store(true, std::memory_order_release);
    dequeue_cv_.notify_all();
    enqueue_cv_.notify_all();
}

}  // namespace themis::execution
