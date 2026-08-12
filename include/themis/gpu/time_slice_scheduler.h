/**
 * @file time_slice_scheduler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <deque>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/launcher.h"

namespace themis {
namespace gpu {

/**
 * @brief Dynamic GPU time-slicing scheduler for multi-tenant isolation.
 *
 * Provides fair time-sharing of the GPU across multiple tenants so that no
 * single tenant can monopolize the compute resource.  Each tenant is
 * registered with a configurable time quantum (`slice_ms`).  The scheduler
 * dispatches GPU work items in round-robin order; within each tenant's turn
 * it executes as many queued items as fit inside the time quantum before
 * moving to the next tenant.
 *
 * Work submission
 * ---------------
 * Callers enqueue work with `submit(tenant_id, item)`.  Items accumulate in
 * per-tenant FIFO queues until the scheduler dispatches them.
 *
 * Dispatch modes
 * --------------
 * - `dispatch()` — one scheduling round: visit each registered tenant in
 *   round-robin order and execute items until the time slice is exhausted.
 *   Items that were not reached in this round remain in the queue.
 * - `drainAll(backend)` — keep calling `dispatch()` until every tenant
 *   queue is empty, useful in tests and batch workflows.
 *
 * Tenant isolation guarantee
 * --------------------------
 * After each tenant's slice expires the scheduler stops dispatching that
 * tenant's items for the current round.  Any item already executing is
 * allowed to finish; the quantum is enforced between item boundaries, not
 * inside a running kernel.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUTimeSliceScheduler {
public:
    // -----------------------------------------------------------------------
    // Tenant configuration
    // -----------------------------------------------------------------------
    struct TenantConfig {
        std::string tenant_id;
        /// Time quantum in milliseconds for this tenant.  Each dispatch round
        /// executes items from this tenant until `slice_ms` milliseconds have
        /// elapsed.  Must be > 0.
        uint32_t slice_ms = 10;
    };

    // -----------------------------------------------------------------------
    // Per-tenant statistics
    // -----------------------------------------------------------------------
    struct TenantStats {
        std::string tenant_id;
        size_t   submitted       = 0;  ///< Items submitted via submit()
        size_t   completed       = 0;  ///< Items executed to completion
        size_t   preempted       = 0;  ///< Number of times this tenant's slice expired with items remaining in the queue
        uint64_t total_elapsed_ms = 0; ///< Cumulative wall-clock time in dispatch
        uint32_t slice_ms        = 0;  ///< Configured time quantum
        size_t   queue_depth     = 0;  ///< Items currently waiting in the queue
    };

    // -----------------------------------------------------------------------
    // Aggregate scheduler statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t   total_submitted  = 0;  ///< Sum across all tenants
        size_t   total_completed  = 0;  ///< Sum across all tenants
        size_t   total_preempted  = 0;  ///< Sum of preemption events across all tenants (slice expired with items remaining)
        size_t   dispatch_rounds  = 0;  ///< dispatch() calls completed
        size_t   registered_tenants = 0;
    };

    // -----------------------------------------------------------------------
    // Construction / singleton
    // -----------------------------------------------------------------------
    GPUTimeSliceScheduler() = default;
    ~GPUTimeSliceScheduler() = default;

    // Non-copyable.
    GPUTimeSliceScheduler(const GPUTimeSliceScheduler&) = delete;
    GPUTimeSliceScheduler& operator=(const GPUTimeSliceScheduler&) = delete;

    static GPUTimeSliceScheduler& GetInstance() {
        static GPUTimeSliceScheduler inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Tenant lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Register a tenant with its time-slice configuration.
     *
     * @param config  Tenant configuration.  `tenant_id` must be non-empty and
     *                `slice_ms` must be > 0.
     * @return true on success; false if the tenant is already registered or
     *         the configuration is invalid.
     */
    bool registerTenant(const TenantConfig& config);

    /**
     * @brief Unregister a tenant and discard any pending queue entries.
     *
     * @return true if the tenant was found and removed; false otherwise.
     */
    bool unregisterTenant(const std::string& tenant_id);

    /**
     * @brief Return true if the tenant is currently registered.
     */
    bool hasTenant(const std::string& tenant_id) const;

    /**
     * @brief Return the number of registered tenants.
     */
    size_t tenantCount() const;

    /**
     * @brief Return all registered tenant identifiers.
     */
    std::vector<std::string> tenantIds() const;

    // -----------------------------------------------------------------------
    // Work submission
    // -----------------------------------------------------------------------

    /**
     * @brief Enqueue a work item for @p tenant_id.
     *
     * The item is placed at the back of the tenant's FIFO queue and will be
     * dispatched in a future `dispatch()` call.
     *
     * @return true on success; false if the tenant is not registered.
     */
    bool submit(const std::string& tenant_id, GPULauncher::WorkItem item);

    /**
     * @brief Return the number of items currently queued for @p tenant_id.
     *
     * Returns 0 if the tenant is not registered.
     */
    size_t queueDepth(const std::string& tenant_id) const;

    // -----------------------------------------------------------------------
    // Dispatch
    // -----------------------------------------------------------------------

    /**
     * @brief Execute one scheduling round.
     *
     * Visits each registered tenant in round-robin order (registration order).
     * For each tenant, executes queued items via @p backend until either:
     *   - the tenant's time quantum (`slice_ms`) has elapsed, or
     *   - the tenant's queue is empty.
     *
     * Items not reached within this round remain in the queue for the next
     * `dispatch()` call.  The `preempted` counter is incremented for a tenant
     * when the slice expires while items remain in the queue.
     *
     * @param backend  GPU execution backend.  Receives each `WorkItem` and
     *                 returns true on success.  When nullptr is passed a
     *                 CPU no-op backend (always succeeds) is used.
     */
    void dispatch(GPULauncher::BackendFn backend = nullptr);

    /**
     * @brief Drain all tenant queues by calling dispatch() until empty.
     *
     * Terminates when every tenant's queue is empty.  Intended for tests and
     * batch workflows.  Avoid calling from a latency-sensitive path.
     *
     * @param backend  GPU execution backend; nullptr = CPU no-op.
     */
    void drainAll(GPULauncher::BackendFn backend = nullptr);

    /**
     * @brief Return true when all tenant queues are empty.
     */
    bool allQueuesEmpty() const;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Return stats for a specific tenant.
     *
     * Returns a zero-filled TenantStats if the tenant is not registered.
     */
    TenantStats getTenantStats(const std::string& tenant_id) const;

    /**
     * @brief Return stats for all registered tenants.
     */
    std::vector<TenantStats> getAllTenantStats() const;

    /**
     * @brief Return aggregate scheduler statistics.
     */
    Stats getStats() const;

    /**
     * @brief Reset all statistics and clear all queues (keeps tenant registrations).
     *
     * Intended for unit tests.
     */
    void resetStats();

private:
    struct TenantState {
        TenantConfig                        config;
        std::deque<GPULauncher::WorkItem>   queue;
        TenantStats                         stats;
    };

    mutable std::mutex                              mutex_;
    std::unordered_map<std::string, TenantState>   tenants_;
    std::vector<std::string>                        round_robin_order_;  ///< Stable insertion-order list for round-robin.

    // Aggregate counters.
    size_t total_submitted_  = 0;
    size_t total_completed_  = 0;
    size_t total_preempted_  = 0;
    size_t dispatch_rounds_  = 0;
};

} // namespace gpu
} // namespace themis
