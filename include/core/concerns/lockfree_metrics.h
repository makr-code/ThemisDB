/**
 * @file lockfree_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_metrics.h"

#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Lock-free implementation of IMetrics for the core domain.
 *
 * Replaces mutex-based counters with atomic operations and uses lock-free
 * per-thread ring buffers with a periodic background flush for histograms.
 * Expected improvement: ~80% reduction in metric update latency under
 * concurrent load versus a plain mutex-based approach.
 *
 * Design summary:
 *  - **Counters**: `std::atomic<int64_t>` values per named series. Map
 *    insertion (first access) takes a brief exclusive lock; subsequent
 *    increments are fully lock-free (`fetch_add`).
 *  - **Gauges**: `std::atomic<double>` values per named series with a
 *    compare-exchange loop for increment/decrement. `store` is always
 *    lock-free.
 *  - **Histograms**: each calling thread owns a single-producer /
 *    single-consumer ring buffer. A background thread periodically drains
 *    every registered thread buffer into shared aggregates, so the
 *    producer (application thread) never blocks on a mutex.
 *
 * Thread-safety guarantees:
 *  - `incrementCounter`, `setGauge`, `incrementGauge`, `decrementGauge`
 *    and `observeHistogram` are safe to call concurrently from any thread.
 *  - `exportMetrics`, `reset`, `flush`, and `shutdown` acquire internal
 *    locks; they are not intended for the latency-critical hot path.
 */
class LockFreeMetrics : public IMetrics {
public:
    /// Capacity of each per-thread histogram ring buffer (must be power-of-2).
    static constexpr size_t HISTOGRAM_RING_CAPACITY = 1024;

    /// Default interval at which the background thread flushes ring buffers.
    static constexpr auto DEFAULT_FLUSH_INTERVAL = std::chrono::milliseconds{100};

    /**
     * @param flush_interval  How often the background flush thread drains
     *                        per-thread ring buffers into global aggregates.
     *                        A zero duration disables periodic flushing and
     *                        leaves only explicit flush()/shutdown() drains.
     */
    explicit LockFreeMetrics(
        std::chrono::milliseconds flush_interval = DEFAULT_FLUSH_INTERVAL);

    ~LockFreeMetrics() override;

    // Non-copyable, non-movable (atomic members + background thread).
    LockFreeMetrics(const LockFreeMetrics&) = delete;
    LockFreeMetrics& operator=(const LockFreeMetrics&) = delete;
    LockFreeMetrics(LockFreeMetrics&&) = delete;
    LockFreeMetrics& operator=(LockFreeMetrics&&) = delete;

    // -----------------------------------------------------------------------
    // IMetrics – counters
    // -----------------------------------------------------------------------

    /**
     * @brief Increment a counter by the supplied amount.
     *
     * First use for a metric name incurs a short internal lock to create the
     * counter entry; subsequent increments are lock-free atomic updates.
     *
     * @param name   Metric name.
     * @param value  Amount to add; may be negative for corrections.
     * @param labels Optional label set used to disambiguate series.
     */
    void incrementCounter(const std::string& name, int64_t value = 1,
                          const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – gauges
    // -----------------------------------------------------------------------

    /**
     * @brief Set a gauge to an absolute value.
     *
     * Gauge updates are atomic and do not block the hot path after the entry
     * has been created.
     */
    void setGauge(const std::string& name, double value,
                  const Labels& labels = {}) override;

    /**
     * @brief Increase a gauge by the supplied delta.
     */
    void incrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override;

    /**
     * @brief Decrease a gauge by the supplied delta.
     */
    void decrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – histograms
    // -----------------------------------------------------------------------

    /**
     * @brief Record a histogram observation.
     *
     * Observations are buffered in a per-thread ring until the background
     * flush thread drains them into shared aggregates.
     */
    void observeHistogram(const std::string& name, double value,
                          const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – convenience helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Record an operation latency in milliseconds.
     *
     * The value is stored in the histogram series named
     * `<operation>_latency_ms`.
     */
    void recordLatency(const std::string& operation, double latencyMs,
                       const Labels& labels = {}) override;

    /**
     * @brief Increment the `<operation>_errors_total` counter.
     */
    void recordError(const std::string& operation,
                     const Labels& labels = {}) override;

    /**
     * @brief Increment the `<operation>_success_total` counter.
     */
    void recordSuccess(const std::string& operation,
                       const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – export and reset
    // -----------------------------------------------------------------------

    /**
     * @brief Export the current metrics snapshot in Prometheus text format.
     *
     * The method synchronously drains pending histogram observations before
     * serialising counters, gauges, and aggregates so the returned snapshot
     * reflects the most recent producer-thread activity.
     */
    std::string exportMetrics() const override;

    /**
     * @brief Clear all metric series and reset in-memory counters.
     */
    void reset() override;

    // -----------------------------------------------------------------------
    // IMetrics – lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Synchronously drain all thread-local ring buffers into the
        *        global histogram aggregates.
        *
        * Useful before scraping or shutdown to avoid losing observations that
        * are still sitting in producer-thread buffers.
     */
    void flush() noexcept override;

    /**
     * @brief Stop the background flush thread and perform a final flush.
        *
        * After shutdown, histogram observations may be dropped if producers keep
        * calling observeHistogram() on the dead instance.
     */
    void shutdown() noexcept override;

        /**
        * @brief Probe whether the metrics subsystem is healthy.
        *
        * Returns healthy when the background flush thread is running, or when
        * the instance has been cleanly shut down and is no longer accepting
        * writes.
        */
    ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Total histogram observations dropped because a thread's ring
     *        buffer was full at the time of the call.
     *
     * A rising value usually indicates the flush interval is too slow for the
     * current observation rate or the ring capacity is too small.
     */
    uint64_t droppedObservations() const noexcept {
        return dropped_observations_.load(std::memory_order_relaxed);
    }

private:
    // =======================================================================
    // Internal helpers
    // =======================================================================

    /// Build a unique string key from a metric name and its label set.
    static std::string makeKey(const std::string& name, const Labels& labels);

    // =======================================================================
    // Counters – std::atomic<int64_t>
    // =======================================================================

    struct CounterEntry {
        std::atomic<int64_t> value{0};
        std::string          name;
        Labels               labels;

        CounterEntry(std::string n, Labels l)
            : name(std::move(n)), labels(std::move(l)) {}
    };

    mutable std::shared_mutex                                         counters_mu_;
    std::unordered_map<std::string, std::unique_ptr<CounterEntry>>    counters_;

    /// Returns the atomic counter for the given key, creating it if absent.
    /// Only the initial creation acquires an exclusive lock.
    CounterEntry* getOrCreateCounter(const std::string& key,
                                     const std::string& name,
                                     const Labels&      labels);

    // =======================================================================
    // Gauges – std::atomic<double>
    // =======================================================================

    struct GaugeEntry {
        std::atomic<double> value{0.0};
        std::string         name;
        Labels              labels;

        GaugeEntry(std::string n, Labels l)
            : name(std::move(n)), labels(std::move(l)) {}
    };

    mutable std::shared_mutex                                        gauges_mu_;
    std::unordered_map<std::string, std::unique_ptr<GaugeEntry>>     gauges_;

    GaugeEntry* getOrCreateGauge(const std::string& key,
                                  const std::string& name,
                                  const Labels&      labels);

    // =======================================================================
    // Histograms – lock-free SPSC ring buffer per thread
    // =======================================================================

    struct HistoObservation {
        std::string key;
        std::string name;
        Labels      labels;
        double      value{0.0};
    };

    /// Minimal lock-free SPSC ring buffer (Single Producer, Single Consumer).
    /// The producing application thread calls tryPush; the background flush
    /// thread calls tryPop.  Cache-line alignment prevents false sharing.
    template<typename T, size_t Cap>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324)
#endif
    struct SPSCRing {
        static_assert((Cap & (Cap - 1)) == 0,
                      "SPSCRing capacity must be a power of 2");

        alignas(64) std::atomic<size_t>  write_idx{0};
        alignas(64) std::atomic<size_t>  read_idx{0};
        alignas(64) std::atomic<uint64_t> dropped{0};
        alignas(64) std::array<T, Cap>   buf{};

        bool tryPush(T item) noexcept {
            const size_t w  = write_idx.load(std::memory_order_relaxed);
            const size_t nw = (w + 1) & (Cap - 1);
            if (nw == read_idx.load(std::memory_order_acquire)) {
                dropped.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            buf[w] = std::move(item);
            write_idx.store(nw, std::memory_order_release);
            return true;
        }

        bool tryPop(T& out) noexcept {
            const size_t r = read_idx.load(std::memory_order_relaxed);
            if (r == write_idx.load(std::memory_order_acquire)) {
              return false;
            }
            out = std::move(buf[r]);
            read_idx.store((r + 1) & (Cap - 1), std::memory_order_release);
            return true;
        }

        bool empty() const noexcept {
            return read_idx.load(std::memory_order_acquire) ==
                   write_idx.load(std::memory_order_acquire);
        }
    };
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

    using HistoRing = SPSCRing<HistoObservation, HISTOGRAM_RING_CAPACITY>;

    /// One entry per registered thread.
    /// Shared ownership between the thread-local map and the instance registry.
    struct ThreadEntry {
        HistoRing         ring;
        std::atomic<bool> alive{true};
    };

    // Unique identifier for this instance (disambiguates multiple instances
    // in the thread-local map).
    const uint64_t instance_id_;
    static std::atomic<uint64_t> next_instance_id_;

    // All ThreadEntry objects registered with this instance.
    mutable std::mutex                                     thread_entries_mu_;
    std::vector<std::shared_ptr<ThreadEntry>>              thread_entries_;

    /// Returns the thread-local ring buffer for this instance, registering it
    /// on first call from a new thread.
    HistoRing& getOrRegisterThreadRing();

    // =======================================================================
    // Global histogram aggregates (written only by the flush thread)
    // =======================================================================

    struct HistoAggregate {
        std::string  name;
        Labels       labels;
        uint64_t     count{0};
        double       sum{0.0};
        double       min_val{std::numeric_limits<double>::max()};
        double       max_val{std::numeric_limits<double>::lowest()};
    };

    mutable std::mutex                                               histos_mu_;
    std::unordered_map<std::string, std::unique_ptr<HistoAggregate>> histos_;

    HistoAggregate* getOrCreateHistoAggregate(const std::string& key,
                                              const std::string& name,
                                              const Labels&      labels);

    /// Drain every registered thread-local ring into the global aggregates.
    void drainAllRings() noexcept;

    /// Apply a single observation to the global aggregate map.
    void applyObservation(const HistoObservation& obs) noexcept;

    // =======================================================================
    // Background flush thread
    // =======================================================================

    std::atomic<bool>         running_{false};
    std::thread               flush_thread_;
    std::chrono::milliseconds flush_interval_;
    mutable std::mutex        flush_wait_mu_;
    std::condition_variable   flush_wait_cv_;

    void startFlushThread();
    void stopFlushThread() noexcept;
    void flushLoop() noexcept;

    // =======================================================================
    // Statistics
    // =======================================================================

    std::atomic<uint64_t> dropped_observations_{0};
};

} // namespace concerns
} // namespace core
} // namespace themis
