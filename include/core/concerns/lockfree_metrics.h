/**
 * @file lockfree_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

class LockFreeMetrics : public IMetrics {
public:
    static constexpr size_t HISTOGRAM_RING_CAPACITY = 1024;

    static constexpr auto DEFAULT_FLUSH_INTERVAL = std::chrono::milliseconds{100};

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

    void flush() noexcept override;

    void shutdown() noexcept override;

    ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    uint64_t droppedObservations() const noexcept {
        return dropped_observations_.load(std::memory_order_relaxed);
    }

private:
    // =======================================================================
    // Internal helpers
    // =======================================================================

    /**
     * @brief Make Key.
     * @param[in] name Input parameter.
     * @param[in] labels Input parameter.
     * @return Return value.
     */
    static std::string makeKey(const std::string& name, const Labels& labels);

    // =======================================================================
    // Counters – std::atomic<int64_t>
    // =======================================================================

    struct CounterEntry {
        std::atomic<int64_t> value{0};
        std::string          name = {};
        Labels               labels;

        CounterEntry(std::string n, Labels l)
            : name(std::move(n)), labels(std::move(l)) {}
    };

    mutable std::shared_mutex                                         counters_mu_;
    std::unordered_map<std::string, std::unique_ptr<CounterEntry>>    counters_;

    /**
     * @brief Get Or Create Counter.
     * @param[in] key Input parameter.
     * @param[in] name Input parameter.
     * @param[in] labels Input parameter.
     * @return Pointer to the result.
     */
    CounterEntry* getOrCreateCounter(const std::string& key,
                                     const std::string& name,
                                     const Labels&      labels);

    // =======================================================================
    // Gauges – std::atomic<double>
    // =======================================================================

    struct GaugeEntry {
        std::atomic<double> value{0.0};
        std::string         name = {};
        Labels              labels;

        GaugeEntry(std::string n, Labels l)
            : name(std::move(n)), labels(std::move(l)) {}
    };

    mutable std::shared_mutex                                        gauges_mu_;
    std::unordered_map<std::string, std::unique_ptr<GaugeEntry>>     gauges_;

    /**
     * @brief Get Or Create Gauge.
     * @param[in] key Input parameter.
     * @param[in] name Input parameter.
     * @param[in] labels Input parameter.
     * @return Pointer to the result.
     */
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

    /**
     * @brief Get Or Register Thread Ring.
     * @return Return value.
     */
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

    /**
     * @brief Get Or Create Histo Aggregate.
     * @param[in] key Input parameter.
     * @param[in] name Input parameter.
     * @param[in] labels Input parameter.
     * @return Pointer to the result.
     */
    HistoAggregate* getOrCreateHistoAggregate(const std::string& key,
                                              const std::string& name,
                                              const Labels&      labels);

    /**
     * @brief Drain All Rings.
     * @note Exception safety: noexcept.
     */
    void drainAllRings() noexcept;

    /**
     * @brief Apply Observation.
     * @param[in] obs Input parameter.
     * @note Exception safety: noexcept.
     */
    void applyObservation(const HistoObservation& obs) noexcept;

    // =======================================================================
    // Background flush thread
    // =======================================================================

    std::atomic<bool>         running_{false};
    std::thread               flush_thread_;
    std::chrono::milliseconds flush_interval_;
    mutable std::mutex        flush_wait_mu_;
    std::condition_variable   flush_wait_cv_;

    /**
     * @brief Start Flush Thread.
     */
    void startFlushThread();
    /**
     * @brief Stop Flush Thread.
     * @note Exception safety: noexcept.
     */
    void stopFlushThread() noexcept;
    /**
     * @brief Flush Loop.
     * @note Exception safety: noexcept.
     */
    void flushLoop() noexcept;

    // =======================================================================
    // Statistics
    // =======================================================================

    std::atomic<uint64_t> dropped_observations_{0};
};

} // namespace concerns
} // namespace core
} // namespace themis
