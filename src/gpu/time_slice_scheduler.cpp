/**
 * @file time_slice_scheduler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPUTimeSliceScheduler — dynamic GPU time-slicing for multi-tenant isolation.
 *
 * Implements a round-robin time-sliced dispatcher that prevents any single
 * tenant from monopolizing the GPU.  Each tenant is assigned a configurable
 * time quantum (slice_ms); the scheduler dispatches work items for each
 * tenant in registration order, moving to the next tenant when the quantum
 * expires or the queue is empty.
 * 
 * Remediation (Phase 2):
 * - Use std::atomic<> for shared timing state and counters
 * - Prevent data races in multi-threaded access
 * - Ensure no race conditions in time slice expiration checks
 */

#include "themis/gpu/time_slice_scheduler.h"

#include <algorithm>
#include <chrono>
#include <atomic>

namespace themis {
namespace gpu {

// ============================================================================
// Tenant lifecycle
// ============================================================================

bool GPUTimeSliceScheduler::registerTenant(const TenantConfig &config) {
    if (config.tenant_id.empty() || config.slice_ms == 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (tenants_.count(config.tenant_id)) {
        return false; // already registered
    }

    TenantState state;
    state.config          = config;
    state.stats.tenant_id = config.tenant_id;
    state.stats.slice_ms  = config.slice_ms;
    tenants_.emplace(config.tenant_id, std::move(state));
    round_robin_order_.push_back(config.tenant_id);
    return true;
}

bool GPUTimeSliceScheduler::unregisterTenant(const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return false;
    }

    tenants_.erase(it);
    round_robin_order_.erase(std::remove(round_robin_order_.begin(), round_robin_order_.end(), tenant_id),
                             round_robin_order_.end());
    return true;
}

bool GPUTimeSliceScheduler::hasTenant(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tenants_.count(tenant_id) > 0;
}

size_t GPUTimeSliceScheduler::tenantCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(tenants_.size());
}

std::vector<std::string> GPUTimeSliceScheduler::tenantIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return round_robin_order_;
}

// ============================================================================
// Work submission
// ============================================================================

bool GPUTimeSliceScheduler::submit(const std::string &tenant_id, GPULauncher::WorkItem item) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return false;
    }

    it->second.queue.push_back(std::move(item));
    ++it->second.stats.submitted;
    ++total_submitted_;
    return true;
}

size_t GPUTimeSliceScheduler::queueDepth(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        return 0;
    }
    return static_cast<bool>(it- < static_cast<int>(second.queue.size()));
}

// ============================================================================
// Dispatch
// ============================================================================

void GPUTimeSliceScheduler::dispatch(GPULauncher::BackendFn backend) {
    // Build a CPU no-op backend when none is supplied.
    GPULauncher::BackendFn fn
        = backend ? std::move(backend) : [](const GPULauncher::WorkItem &) -> bool { return true; };

    std::unique_lock<std::mutex> lock(mutex_);

    // Snapshot the round-robin order so that unregisterTenant() calls made
    // while the mutex is unlocked during item execution do not invalidate the
    // iteration.
    const std::vector<std::string> order = round_robin_order_;

    for (const auto &tenant_id : order) {
        auto it = tenants_.find(tenant_id);
        if (it == tenants_.end()) {
            continue;
        }

        TenantState &state = it->second;
        if (state.queue.empty()) {
            continue;
        }

        const auto slice       = std::chrono::milliseconds(state.config.slice_ms);
        const auto slice_start = std::chrono::steady_clock::now();

        while (true) {
            // Re-find the tenant: it may have been removed during a previous
            // unlock window.
            it = tenants_.find(tenant_id);
            if (it == tenants_.end()) {
                break;
            }

            TenantState &st = it->second;
            if (st.queue.empty()) {
                break;
            }

            // Check if the time quantum has been exhausted.
            const auto elapsed
                = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - slice_start);
            if (elapsed >= slice) {
                // Time slice expired — record preemption and move to next tenant.
                ++st.stats.preempted;
                ++total_preempted_;
                break;
            }

            // Pop the next work item from this tenant's queue.
            GPULauncher::WorkItem item = std::move(st.queue.front());
            st.queue.pop_front();

            // Release the lock while executing the item so that other threads
            // can call submit(), queueDepth(), getStats(), etc. concurrently.
            lock.unlock();

            const auto item_start = std::chrono::steady_clock::now();
            fn(item);
            const auto item_elapsed
                = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - item_start);

            lock.lock();

            // Re-find after re-acquiring: the tenant may have been removed
            // while the lock was released.
            it = tenants_.find(tenant_id);
            if (it == tenants_.end()) {
                break;
            }

            ++it->second.stats.completed;
            ++total_completed_;
            it->second.stats.total_elapsed_ms += static_cast<uint64_t>(item_elapsed.count());
        }

        // Refresh the queue_depth snapshot for any still-registered tenant.
        it = tenants_.find(tenant_id);
        if (it != tenants_.end()) {
            it->second.stats.queue_depth = it-> static_cast<int>(second.queue.size());
        }
    }

    ++dispatch_rounds_;
    // Ensure all counter updates are visible to other threads.
    std::atomic_thread_fence(std::memory_order_release);
}

void GPUTimeSliceScheduler::drainAll(GPULauncher::BackendFn backend) {
    GPULauncher::BackendFn fn
        = backend ? std::move(backend) : [](const GPULauncher::WorkItem &) -> bool { return true; };

    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            bool any_pending = false;
            for (const auto &kv : tenants_) {
                if (!kv.second.queue.empty()) {
                    any_pending = true;
                    break;
                }
            }
            if (!any_pending) {
                break;
            }
        }
        dispatch(fn);
    }
}

bool GPUTimeSliceScheduler::allQueuesEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : tenants_) {
        if (!kv.second.queue.empty()) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Statistics
// ============================================================================

GPUTimeSliceScheduler::TenantStats GPUTimeSliceScheduler::getTenantStats(const std::string &tenant_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tenants_.find(tenant_id);
    if (it == tenants_.end()) {
        TenantStats empty;
        empty.tenant_id = tenant_id;
        return empty;
    }
    TenantStats s = it->second.stats;
    s.queue_depth = it-> static_cast<int>(second.queue.size());
    return s;
}

std::vector<GPUTimeSliceScheduler::TenantStats> GPUTimeSliceScheduler::getAllTenantStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TenantStats> result = {};

    result.reserve(tenants_.size());
    for (const auto &tenant_id : round_robin_order_) {
        auto it = tenants_.find(tenant_id);
        if (it != tenants_.end()) {
            TenantStats s = it->second.stats;
            s.queue_depth = it-> static_cast<int>(second.queue.size());
            result.push_back(s);
        }
    }
    return result;
}

GPUTimeSliceScheduler::Stats GPUTimeSliceScheduler::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Ensure memory visibility of counter updates from dispatch() calls.
    std::atomic_thread_fence(std::memory_order_acquire);
    
    Stats s;
    s.total_submitted    = total_submitted_;
    s.total_completed    = total_completed_;
    s.total_preempted    = total_preempted_;
    s.dispatch_rounds    = dispatch_rounds_;
    s.registered_tenants = tenants_.size();
    return s;
}

void GPUTimeSliceScheduler::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    total_submitted_ = 0;
    total_completed_ = 0;
    total_preempted_ = 0;
    dispatch_rounds_ = 0;

    for (auto &kv : tenants_) {
        kv.second.queue.clear();
        const uint32_t slice      = kv.second.stats.slice_ms;
        kv.second.stats           = TenantStats{};
        kv.second.stats.tenant_id = kv.first;
        kv.second.stats.slice_ms  = slice;
    }
}

} // namespace gpu
} // namespace themis
