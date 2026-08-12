/**
 * @file lockfree_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "core/concerns/lockfree_metrics.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

std::atomic<uint64_t> LockFreeMetrics::next_instance_id_{0};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

LockFreeMetrics::LockFreeMetrics(std::chrono::milliseconds flush_interval)
    : instance_id_(next_instance_id_.fetch_add(1, std::memory_order_relaxed)), flush_interval_(flush_interval) {
    startFlushThread();
}

LockFreeMetrics::~LockFreeMetrics() {
    shutdown();
}

// ---------------------------------------------------------------------------
// IMetrics – counters
// ---------------------------------------------------------------------------

void LockFreeMetrics::incrementCounter(const std::string &name, int64_t value, const Labels &labels) {
    const std::string key = makeKey(name, labels);
    CounterEntry *entry   = getOrCreateCounter(key, name, labels);
    entry->value.fetch_add(value, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// IMetrics – gauges
// ---------------------------------------------------------------------------

void LockFreeMetrics::setGauge(const std::string &name, double value, const Labels &labels) {
    const std::string key = makeKey(name, labels);
    GaugeEntry *entry     = getOrCreateGauge(key, name, labels);
    entry->value.store(value, std::memory_order_relaxed);
}

void LockFreeMetrics::incrementGauge(const std::string &name, double delta, const Labels &labels) {
    const std::string key = makeKey(name, labels);
    GaugeEntry *entry     = getOrCreateGauge(key, name, labels);
    // Atomic fetch_add for double (C++20).
    entry->value.fetch_add(delta, std::memory_order_relaxed);
}

void LockFreeMetrics::decrementGauge(const std::string &name, double delta, const Labels &labels) {
    const std::string key = makeKey(name, labels);
    GaugeEntry *entry     = getOrCreateGauge(key, name, labels);
    entry->value.fetch_sub(delta, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// IMetrics – histograms
// ---------------------------------------------------------------------------

void LockFreeMetrics::observeHistogram(const std::string &name, double value, const Labels &labels) {
    HistoRing &ring = getOrRegisterThreadRing();

    HistoObservation obs;
    obs.key    = makeKey(name, labels);
    obs.name   = name;
    obs.labels = labels;
    obs.value  = value;

    if (!ring.tryPush(std::move(obs))) {
        dropped_observations_.fetch_add(1, std::memory_order_relaxed);
    }
}

// ---------------------------------------------------------------------------
// IMetrics – convenience helpers
// ---------------------------------------------------------------------------

void LockFreeMetrics::recordLatency(const std::string &operation, double latencyMs, const Labels &labels) {
    observeHistogram(operation + "_latency_ms", latencyMs, labels);
}

void LockFreeMetrics::recordError(const std::string &operation, const Labels &labels) {
    incrementCounter(operation + "_errors_total", 1, labels);
}

void LockFreeMetrics::recordSuccess(const std::string &operation, const Labels &labels) {
    incrementCounter(operation + "_success_total", 1, labels);
}

// ---------------------------------------------------------------------------
// IMetrics – export
// ---------------------------------------------------------------------------

std::string LockFreeMetrics::exportMetrics() const {
    // Perform a synchronous flush so the exported snapshot is up-to-date.
    const_cast<LockFreeMetrics *>(this)->drainAllRings();

    std::ostringstream out;

    // -- Counters -----------------------------------------------------------
    {
        std::shared_lock<std::shared_mutex> lock(counters_mu_);
        for (const auto &[key, entry] : counters_) {
            out << "# TYPE " << entry->name << " counter\n";
            out << entry->name;
            if (!entry->labels.empty()) {
                out << '{';
                bool first = true;
                for (const auto &[k, v] : entry->labels) {
                    if (!first) {
                        out << ',';
                    }
                    out << k << "=\"" << v << '"';
                    first = false;
                }
                out << '}';
            }
            out << ' ' << entry->value.load(std::memory_order_relaxed) << '\n';
        }
    }

    // -- Gauges -------------------------------------------------------------
    {
        std::shared_lock<std::shared_mutex> lock(gauges_mu_);
        for (const auto &[key, entry] : gauges_) {
            out << "# TYPE " << entry->name << " gauge\n";
            out << entry->name;
            if (!entry->labels.empty()) {
                out << '{';
                bool first = true;
                for (const auto &[k, v] : entry->labels) {
                    if (!first) {
                        out << ',';
                    }
                    out << k << "=\"" << v << '"';
                    first = false;
                }
                out << '}';
            }
            out << ' ' << entry->value.load(std::memory_order_relaxed) << '\n';
        }
    }

    // -- Histograms ---------------------------------------------------------
    {
        std::lock_guard<std::mutex> lock(histos_mu_);
        for (const auto &[key, agg] : histos_) {
            out << "# TYPE " << agg->name << " summary\n";
            // _sum
            out << agg->name << "_sum";
            if (!agg->labels.empty()) {
                out << '{';
                bool first = true;
                for (const auto &[k, v] : agg->labels) {
                    if (!first) {
                        out << ',';
                    }
                    out << k << "=\"" << v << '"';
                    first = false;
                }
                out << '}';
            }
            out << ' ' << agg->sum << '\n';
            // _count
            out << agg->name << "_count";
            if (!agg->labels.empty()) {
                out << '{';
                bool first = true;
                for (const auto &[k, v] : agg->labels) {
                    if (!first) {
                        out << ',';
                    }
                    out << k << "=\"" << v << '"';
                    first = false;
                }
                out << '}';
            }
            out << ' ' << agg->count << '\n';
        }
    }

    return out.str();
}

// ---------------------------------------------------------------------------
// IMetrics – reset
// ---------------------------------------------------------------------------

void LockFreeMetrics::reset() {
    {
        std::unique_lock<std::shared_mutex> lock(counters_mu_);
        counters_.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(gauges_mu_);
        gauges_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(histos_mu_);
        histos_.clear();
    }
    dropped_observations_.store(0, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// IMetrics – lifecycle
// ---------------------------------------------------------------------------

void LockFreeMetrics::flush() noexcept {
    drainAllRings();
}

void LockFreeMetrics::shutdown() noexcept {
    stopFlushThread();
    // Final drain to capture any remaining observations.
    drainAllRings();
    // Invalidate all registered thread entries so threads that call
    // observeHistogram after shutdown drop their observations cleanly.
    {
        std::lock_guard<std::mutex> lock(thread_entries_mu_);
        for (auto &entry : thread_entries_) {
            entry->alive.store(false, std::memory_order_release);
        }
        thread_entries_.clear();
    }
}

ProbeResult LockFreeMetrics::isHealthy() const {
    if (running_.load(std::memory_order_relaxed)) {
        return ProbeResult::healthy();
    }
    // Healthy even after shutdown — the instance is still usable for reads.
    return ProbeResult::healthy();
}

// ---------------------------------------------------------------------------
// Internal helpers – key building
// ---------------------------------------------------------------------------

std::string LockFreeMetrics::makeKey(const std::string &name, const Labels &labels) {
    if (labels.empty()) {
        return name;
    }

    std::string key;
    key.reserve(name.size() + labels.size() * 16);
    key += name;
    key += '{';
    bool first = true;
    for (const auto &[k, v] : labels) {
        if (!first) {
            key += ',';
        }
        key += k;
        key += '=';
        key += v;
        first = false;
    }
    key += '}';
    return key;
}

// ---------------------------------------------------------------------------
// Internal helpers – counters
// ---------------------------------------------------------------------------

LockFreeMetrics::CounterEntry *LockFreeMetrics::getOrCreateCounter(const std::string &key, const std::string &name,
                                                                   const Labels &labels) {
    // Fast path: shared lock, entry already exists.
    {
        std::shared_lock<std::shared_mutex> lock(counters_mu_);
        auto it = counters_.find(key);
        if (it != counters_.end()) {
            return it->second.get();
        }
    }
    // Slow path: exclusive lock, insert if still missing.
    {
        std::unique_lock<std::shared_mutex> lock(counters_mu_);
        auto [it, inserted] = counters_.emplace(key, std::make_unique<CounterEntry>(name, labels));
        return it->second.get();
    }
}

// ---------------------------------------------------------------------------
// Internal helpers – gauges
// ---------------------------------------------------------------------------

LockFreeMetrics::GaugeEntry *LockFreeMetrics::getOrCreateGauge(const std::string &key, const std::string &name,
                                                               const Labels &labels) {
    // Fast path: shared lock.
    {
        std::shared_lock<std::shared_mutex> lock(gauges_mu_);
        auto it = gauges_.find(key);
        if (it != gauges_.end()) {
            return it->second.get();
        }
    }
    // Slow path: exclusive lock.
    {
        std::unique_lock<std::shared_mutex> lock(gauges_mu_);
        auto [it, inserted] = gauges_.emplace(key, std::make_unique<GaugeEntry>(name, labels));
        return it->second.get();
    }
}

// ---------------------------------------------------------------------------
// Internal helpers – histograms / ring buffers
// ---------------------------------------------------------------------------

LockFreeMetrics::HistoRing &LockFreeMetrics::getOrRegisterThreadRing() {
    // Function-local thread_local: each thread has its own map; no global
    // namespace access to the private ThreadEntry type required.
    static thread_local std::unordered_map<uint64_t, std::shared_ptr<ThreadEntry>> tls_map;

    auto &slot = tls_map[instance_id_];
    if (!slot || !slot->alive.load(std::memory_order_acquire)) {
        auto entry = std::make_shared<ThreadEntry>();
        {
            std::lock_guard<std::mutex> lock(thread_entries_mu_);
            thread_entries_.push_back(entry);
        }
        slot = entry;
    }
    return slot->ring;
}

LockFreeMetrics::HistoAggregate *
LockFreeMetrics::getOrCreateHistoAggregate(const std::string &key, const std::string &name, const Labels &labels) {
    auto it = histos_.find(key);
    if (it != histos_.end()) {
        return it->second.get();
    }

    auto [ins, ok]      = histos_.emplace(key, std::make_unique<HistoAggregate>());
    ins->second->name   = name;
    ins->second->labels = labels;
    return ins->second.get();
}

void LockFreeMetrics::drainAllRings() noexcept {
    // Collect a snapshot of the entry list so the flush doesn't hold the
    // registration lock for the entire drain.
    std::vector<std::shared_ptr<ThreadEntry>> snapshot;
    {
        std::lock_guard<std::mutex> lock(thread_entries_mu_);
        snapshot = thread_entries_;
    }

    HistoObservation obs;
    for (auto &entry : snapshot) {
        if (!entry->alive.load(std::memory_order_acquire)) {
            continue;
        }
        while (entry->ring.tryPop(obs)) {
            applyObservation(obs);
        }
    }
}

void LockFreeMetrics::applyObservation(const HistoObservation &obs) noexcept {
    std::lock_guard<std::mutex> lock(histos_mu_);
    HistoAggregate *agg = getOrCreateHistoAggregate(obs.key, obs.name, obs.labels);
    agg->count += 1;
    agg->sum += obs.value;
    if (obs.value < agg->min_val)
        agg->min_val = obs.value;
    if (obs.value > agg->max_val)
        agg->max_val = obs.value;
}

// ---------------------------------------------------------------------------
// Background flush thread
// ---------------------------------------------------------------------------

void LockFreeMetrics::startFlushThread() {
    if (flush_interval_.count() <= 0) {
        running_.store(false, std::memory_order_release);
        return;
    }

    running_.store(true, std::memory_order_release);
    flush_thread_ = std::thread(&LockFreeMetrics::flushLoop, this);
}

void LockFreeMetrics::stopFlushThread() noexcept {
    running_.store(false, std::memory_order_release);
    flush_wait_cv_.notify_all();
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
}

void LockFreeMetrics::flushLoop() noexcept {
    std::unique_lock<std::mutex> wait_lock(flush_wait_mu_);
    while (running_.load(std::memory_order_acquire)) {
        const bool stop_requested =
            flush_wait_cv_.wait_for(wait_lock, flush_interval_, [this] {
                return !running_.load(std::memory_order_acquire);
            });
        if (stop_requested) {
            break;
        }

        wait_lock.unlock();
        drainAllRings();
        wait_lock.lock();
    }
    wait_lock.unlock();

    // Drain once more after the stop signal.
    drainAllRings();
}

} // namespace concerns
} // namespace core
} // namespace themis
