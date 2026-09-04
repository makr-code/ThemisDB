/**
 * @file metrics_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace acceleration {
namespace metrics {

/// @brief Types of metrics supported by the metrics collection system
enum class MetricType {
    COUNTER,    ///< Monotonically increasing value (counts, totals)
    GAUGE,      ///< Value that can go up or down (current levels, measurements)
    HISTOGRAM,  ///< Distribution of observations with configurable buckets
    SUMMARY     ///< Quantiles of observed values (percentiles)
};

/// @brief Abstract base class for all metric types
///
/// Provides common interface for metric registration, naming, and serialization.
/// Subclasses implement specific metric behavior (Counter, Gauge, Histogram, Summary).
class Metric {
public:
    /// @brief Constructor
    /// @param name Unique metric identifier (e.g., "themis_cuda_init_success_total")
    /// @param description Human-readable metric description
    /// @param type Classification of the metric (Counter, Gauge, Histogram, Summary)
    explicit Metric(const std::string& name, const std::string& description,
                   MetricType type)
        : name_(name), description_(description), type_(type) {}
    
    virtual ~Metric() = default;
    
    /// @brief Get metric name
    /// @return Reference to the metric's unique identifier
    const std::string& name() const { return name_; }
    
    /// @brief Get metric description
    /// @return Reference to the human-readable description
    const std::string& description() const { return description_; }
    
    /// @brief Get metric type
    /// @return Classification (Counter, Gauge, Histogram, or Summary)
    MetricType type() const { return type_; }
    
    /// @brief Serialize metric to Prometheus text format
    /// @return Prometheus format string representation of current metric value
    [[nodiscard]] virtual std::string serialize() const = 0;
    
protected:
    std::string name_;              ///< Unique metric identifier
    std::string description_;       ///< Human-readable description
    MetricType type_;               ///< Metric type classification
};

/// @brief Counter metric - monotonically increasing value
///
/// Used for quantities that only increase over time (operation counts, total errors, etc.).
/// Never decreases. Thread-safe using atomic operations.
class Counter : public Metric {
public:
    /// @brief Constructor
    /// @param name Metric identifier
    /// @param description Human-readable description
    Counter(const std::string& name, const std::string& description)
        : Metric(name, description, MetricType::COUNTER), value_(0) {}
    
    /// @brief Increment counter by a delta
    /// @param delta Amount to increment by (default: 1)
    void increment(uint64_t delta = 1) {
        value_.fetch_add(delta, std::memory_order_relaxed);
    }
    
    /// @brief Get current counter value
    /// @return Current count value (monotonically non-decreasing)
    uint64_t value() const {
        return value_.load(std::memory_order_relaxed);
    }
    
    /// @brief Reset counter to zero
    /// @note Normally not used in production; provided for testing
    void reset() {
        value_.store(0, std::memory_order_relaxed);
    }
    
    /// @brief Serialize to Prometheus text format
    /// @return Prometheus format: "name{} value"
    std::string serialize() const override {
        return name_ + "{} " + std::to_string(value());
    }
    
private:
    std::atomic<uint64_t> value_;  ///< Atomic counter value
};

/// @brief Gauge metric - value that can go up or down
///
/// Used for quantities that can both increase and decrease (current memory usage,
/// active connections, queue depth, etc.). Thread-safe using atomic operations.
class Gauge : public Metric {
public:
    /// @brief Constructor
    /// @param name Metric identifier
    /// @param description Human-readable description
    Gauge(const std::string& name, const std::string& description)
        : Metric(name, description, MetricType::GAUGE), value_(0.0) {}
    
    /// @brief Set gauge to absolute value
    /// @param value New value to set
    void set(double value) {
        value_.store(value, std::memory_order_relaxed);
    }
    
    /// @brief Increment gauge by delta
    /// @param delta Amount to add (default: 1.0)
    void increment(double delta = 1.0) {
        // fetch_add is not available for double; use compare_exchange loop.
        double expected = value_.load(std::memory_order_relaxed);
        while (!value_.compare_exchange_weak(expected, expected + delta,
                                             std::memory_order_relaxed)) {}
    }
    
    /// @brief Decrement gauge by delta
    /// @param delta Amount to subtract (default: 1.0)
    void decrement(double delta = 1.0) {
        double expected = value_.load(std::memory_order_relaxed);
        while (!value_.compare_exchange_weak(expected, expected - delta,
                                             std::memory_order_relaxed)) {}
    }
    
    /// @brief Get current gauge value
    /// @return Current value
    double value() const {
        return value_.load(std::memory_order_relaxed);
    }
    
    /// @brief Serialize to Prometheus text format
    /// @return Prometheus format: "name{} value"
    std::string serialize() const override {
        return name_ + "{} " + std::to_string(value());
    }
    
private:
    std::atomic<double> value_;  ///< Atomic gauge value
};

/// @brief Histogram metric - distribution of observed values with configurable buckets
///
/// Simplified histogram implementation tracking the sum, count, and bucket distribution
/// of observations. Thread-safe using mutex locks. Supports Prometheus-compatible
/// histogram buckets with cumulative count representation.
class Histogram : public Metric {
public:
    /// @brief Constructor with optional custom buckets
    /// @param name Metric identifier
    /// @param description Human-readable description
    /// @param buckets Custom bucket boundaries (automatically sorted); defaults to {0.001, 0.01, 0.1, 1.0, 10.0}
    Histogram(const std::string& name, const std::string& description,
             const std::vector<double>& buckets = {0.001, 0.01, 0.1, 1.0, 10.0})
        : Metric(name, description, MetricType::HISTOGRAM),
          buckets_(buckets), counts_(buckets.size() + 1, 0), sum_(0), count_(0) {
        // Ensure buckets are sorted
        std::sort(buckets_.begin(), buckets_.end());
    }
    
    /// @brief Record an observation in the histogram
    /// @param value Observation value to add to distribution
    void observe(double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Update sum and count
        sum_ += value;
        count_++;
        
        // Find the bucket for this value
        size_t bucket_idx = 0;
        for (size_t i = 0; i < buckets_.size(); ++i) {
            if (value <= buckets_[i]) {
                bucket_idx = i;
                break;
            }
        }
        if (value > buckets_.back()) {
            bucket_idx = buckets_.size();
        }
        
        counts_[bucket_idx]++;
    }
    
    /// @brief Get sum of all observed values
    /// @return Total of all observations
    double sum() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sum_;
    }
    
    /// @brief Get count of observations
    /// @return Number of observations recorded
    uint64_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
    
    /// @brief Get mean (average) of observations
    /// @return sum() / count() or 0.0 if no observations
    double mean() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ > 0 ? sum_ / count_ : 0.0;
    }
    
    /// @brief Serialize to Prometheus text format
    /// @return Prometheus format histogram with bucket bounds and cumulative counts
    std::string serialize() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string result = name_ + "_sum " + std::to_string(sum_) + "\n";
        result += name_ + "_count " + std::to_string(count_) + "\n";
        
        uint64_t cumulative = 0;
        for (size_t i = 0; i < buckets_.size(); ++i) {
            cumulative += counts_[i];
            result += name_ + "_bucket{le=\"" + std::to_string(buckets_[i]) + 
                     "\"} " + std::to_string(cumulative) + "\n";
        }
        cumulative += counts_[buckets_.size()];
        result += name_ + "_bucket{le=\"+Inf\"} " + std::to_string(cumulative);
        
        return result;
    }
    
private:
    mutable std::mutex mutex_;                ///< Protects histogram data
    std::vector<double> buckets_;             ///< Sorted bucket boundaries
    std::vector<uint64_t> counts_;            ///< Observation counts per bucket
    double sum_;                              ///< Sum of all observations
    uint64_t count_;                          ///< Total observation count
};

/// @brief RAII timer for automatic duration measurement
///
/// Helper class for measuring operation duration. On destruction, automatically
/// records the elapsed time to the associated histogram. Non-copyable but movable.
class Timer {
public:
    /// @brief Constructor starting the timer
    /// @param histogram Pointer to histogram to record duration into; nullptr is safe (no-op)
    explicit Timer(Histogram* histogram)
        : histogram_(histogram),
          start_(std::chrono::steady_clock::now()) {}
    
    /// @brief Destructor recording elapsed time to histogram
    ///
    /// Computes elapsed time since construction and calls observe() on the histogram.
    /// Safe to call with null histogram pointer.
    ~Timer() {
        if (histogram_) {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration<double>(end - start_).count();
            histogram_->observe(duration);
        }
    }
    
    // Non-copyable
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    
    // Movable
    /// @brief Move constructor
    Timer(Timer&& other) noexcept
        : histogram_(other.histogram_), start_(other.start_) {
        other.histogram_ = nullptr;
    }
    
private:
    Histogram* histogram_;                              ///< Histogram to record into (may be null)
    std::chrono::steady_clock::time_point start_;       ///< Start time of measurement
};

/// @brief Central thread-safe metrics collection and export system
///
/// Singleton that manages all metrics (counters, gauges, histograms) for the acceleration
/// module. Provides registration, retrieval, and export in Prometheus and JSON formats.
/// All operations are thread-safe.
class MetricsCollector {
public:
    /// @brief Get the singleton instance
    /// @return Reference to the global metrics collector
    static MetricsCollector& instance() {
        static MetricsCollector collector;
        return collector;
    }
    
    // ── Register metrics ──────────────────────────────────────────────────────
    
    /// @brief Register a counter metric
    /// @param name Unique metric identifier (e.g., "themis_cuda_init_success_total")
    /// @param description Human-readable description for Prometheus metadata
    /// @return Pointer to the registered counter (valid for lifetime of collector)
    Counter* registerCounter(const std::string& name,
                            const std::string& description) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto counter = std::make_unique<Counter>(name, description);
        auto* ptr = counter.get();
        counters_[name] = std::move(counter);
        return ptr;
    }
    
    /// @brief Register a gauge metric
    /// @param name Unique metric identifier
    /// @param description Human-readable description for Prometheus metadata
    /// @return Pointer to the registered gauge (valid for lifetime of collector)
    Gauge* registerGauge(const std::string& name,
                        const std::string& description) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto gauge = std::make_unique<Gauge>(name, description);
        auto* ptr = gauge.get();
        gauges_[name] = std::move(gauge);
        return ptr;
    }
    
    /// @brief Register a histogram metric
    /// @param name Unique metric identifier
    /// @param description Human-readable description for Prometheus metadata
    /// @param buckets Custom bucket boundaries (empty = defaults); automatically sorted
    /// @return Pointer to the registered histogram (valid for lifetime of collector)
    Histogram* registerHistogram(const std::string& name,
                                const std::string& description,
                                const std::vector<double>& buckets = {}) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto histogram = buckets.empty() 
            ? std::make_unique<Histogram>(name, description)
            : std::make_unique<Histogram>(name, description, buckets);
        auto* ptr = histogram.get();
        histograms_[name] = std::move(histogram);
        return ptr;
    }
    
    // ── Get existing metrics ──────────────────────────────────────────────────
    
    /// @brief Retrieve previously registered counter
    /// @param name Metric identifier
    /// @return Pointer to counter, or nullptr if not found or wrong type
    Counter* getCounter(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = counters_.find(name);
        return it != counters_.end() ? it->second.get() : nullptr;
    }
    
    /// @brief Retrieve previously registered gauge
    /// @param name Metric identifier
    /// @return Pointer to gauge, or nullptr if not found or wrong type
    Gauge* getGauge(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = gauges_.find(name);
        return it != gauges_.end() ? it->second.get() : nullptr;
    }
    
    /// @brief Retrieve previously registered histogram
    /// @param name Metric identifier
    /// @return Pointer to histogram, or nullptr if not found or wrong type
    Histogram* getHistogram(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = histograms_.find(name);
        return it != histograms_.end() ? it->second.get() : nullptr;
    }
    
    // ── Export metrics ────────────────────────────────────────────────────────
    
    /// @brief Export all metrics in Prometheus text-based format
    /// @return Prometheus format string suitable for scraping by Prometheus server
    std::string exportPrometheus() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string output;
        
        for (const auto& [name, counter] : counters_) {
            output += "# HELP " + name + " " + counter->description() + "\n";
            output += "# TYPE " + name + " counter\n";
            output += counter->serialize() + "\n\n";
        }
        
        for (const auto& [name, gauge] : gauges_) {
            output += "# HELP " + name + " " + gauge->description() + "\n";
            output += "# TYPE " + name + " gauge\n";
            output += gauge->serialize() + "\n\n";
        }
        
        for (const auto& [name, histogram] : histograms_) {
            output += "# HELP " + name + " " + histogram->description() + "\n";
            output += "# TYPE " + name + " histogram\n";
            output += histogram->serialize() + "\n\n";
        }
        
        return output;
    }
    
    /// @brief Export all metrics in JSON format
    /// @return JSON object with "counters", "gauges", and "histograms" sections
    std::string exportJSON() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string output = "{\n";
        
        output += "  \"counters\": {\n";
        bool first = true;
        for (const auto& [name, counter] : counters_) {
            if (!first) {
              output += ",\n";
            }
            output += "    \"" + name + "\": " + std::to_string(counter->value());
            first = false;
        }
        output += "\n  },\n";
        
        output += "  \"gauges\": {\n";
        first = true;
        for (const auto& [name, gauge] : gauges_) {
            if (!first) {
              output += ",\n";
            }
            output += "    \"" + name + "\": " + std::to_string(gauge->value());
            first = false;
        }
        output += "\n  },\n";
        
        output += "  \"histograms\": {\n";
        first = true;
        for (const auto& [name, histogram] : histograms_) {
            if (!first) {
              output += ",\n";
            }
            output += "    \"" + name + "\": {";
            output += "\"count\": " + std::to_string(histogram->count()) + ", ";
            output += "\"sum\": " + std::to_string(histogram->sum()) + ", ";
            output += "\"mean\": " + std::to_string(histogram->mean()) + "}";
            first = false;
        }
        output += "\n  }\n";
        
        output += "}\n";
        return output;
    }
    
    /// @brief Reset all counters to zero
    /// @note Gauges and histograms are not reset (represent current state)
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, counter] : counters_) {
            counter->reset();
        }
    }
    
    /// @brief Clear all registered metrics
    /// @note All metric pointers become invalid after this call
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        counters_.clear();
        gauges_.clear();
        histograms_.clear();
    }
    
private:
    MetricsCollector() = default;
    ~MetricsCollector() = default;
    
    // Non-copyable, non-movable
    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;
    
    mutable std::mutex mutex_;                                          ///< Protects all metric collections
    std::map<std::string, std::unique_ptr<Counter>> counters_;          ///< All registered counters
    std::map<std::string, std::unique_ptr<Gauge>> gauges_;              ///< All registered gauges
    std::map<std::string, std::unique_ptr<Histogram>> histograms_;      ///< All registered histograms
};

} // namespace metrics
} // namespace acceleration
} // namespace themis
