/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lockfree_metrics.h                                 ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:14:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     340                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 56ac47b313  2026-03-13  feat(core): lock-free metrics — atomic counters, SPSC rin... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_metrics.h"

#include <atomic>
#include <array>
#include <chrono>
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

    void incrementCounter(const std::string& name, int64_t value = 1,
                          const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – gauges
    // -----------------------------------------------------------------------

    void setGauge(const std::string& name, double value,
                  const Labels& labels = {}) override;

    void incrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override;

    void decrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – histograms
    // -----------------------------------------------------------------------

    void observeHistogram(const std::string& name, double value,
                          const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – convenience helpers
    // -----------------------------------------------------------------------

    void recordLatency(const std::string& operation, double latencyMs,
                       const Labels& labels = {}) override;

    void recordError(const std::string& operation,
                     const Labels& labels = {}) override;

    void recordSuccess(const std::string& operation,
                       const Labels& labels = {}) override;

    // -----------------------------------------------------------------------
    // IMetrics – export and reset
    // -----------------------------------------------------------------------

    std::string exportMetrics() const override;

    void reset() override;

    // -----------------------------------------------------------------------
    // IMetrics – lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Synchronously drain all thread-local ring buffers into the
     *        global histogram aggregates.
     */
    void flush() noexcept override;

    /**
     * @brief Stop the background flush thread and perform a final flush.
     */
    void shutdown() noexcept override;

    ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Total histogram observations dropped because a thread's ring
     *        buffer was full at the time of the call.
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
            if (r == write_idx.load(std::memory_order_acquire)) return false;
            out = std::move(buf[r]);
            read_idx.store((r + 1) & (Cap - 1), std::memory_order_release);
            return true;
        }

        bool empty() const noexcept {
            return read_idx.load(std::memory_order_acquire) ==
                   write_idx.load(std::memory_order_acquire);
        }
    };

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
