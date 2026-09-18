/**
 * @file metrics_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class MetricType {
    COUNTER,    ///< Monotonically increasing value (counts, totals)
    GAUGE,      ///< Value that can go up or down (current levels, measurements)
    HISTOGRAM,  ///< Distribution of observations with configurable buckets
    SUMMARY     ///< Quantiles of observed values (percentiles)
};

class Metric {
public:
    /**
     * @brief Metric.
     * @param[in] name Input parameter.
     * @param[in] description Input parameter.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    explicit Metric(const std::string& name, const std::string& description,
                   MetricType type)
        : name_(name), description_(description), type_(type) {}
    
    /**
     * @brief Metric.
     * @return Return value.
     */
    virtual ~Metric() = default;
    
    const std::string& name() const { return name_; }
    
    const std::string& description() const { return description_; }
    
    MetricType type() const { return type_; }
    
    [[nodiscard]] virtual std::string serialize() const = 0;
    
protected:
    std::string name_;              ///< Unique metric identifier
    std::string description_;       ///< Human-readable description
    MetricType type_;               ///< Metric type classification
};

class Counter : public Metric {
public:
    Counter(const std::string& name, const std::string& description)
        : Metric(name, description, MetricType::COUNTER), value_(0) {}
    
    void increment(uint64_t delta = 1) {
        value_.fetch_add(delta, std::memory_order_relaxed);
    }
    
    uint64_t value() const {
        return value_.load(std::memory_order_relaxed);
    }
    
    /**
     * @brief Reset the modification detection flag.
     * @details Calls: store().
     */
    void reset() {
        value_.store(0, std::memory_order_relaxed);
    }
    
    std::string serialize() const override {
        return name_ + "{} " + std::to_string(value());
    }
    
private:
    std::atomic<uint64_t> value_;  ///< Atomic counter value
};

class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& description)
        : Metric(name, description, MetricType::GAUGE), value_(0.0) {}
    
    /**
     * @brief Set.
     * @param[in] value Input parameter.
     * @details Calls: store().
     */
    void set(double value) {
        value_.store(value, std::memory_order_relaxed);
    }
    
    void increment(double delta = 1.0) {
        // fetch_add is not available for double; use compare_exchange loop.
        double expected = value_.load(std::memory_order_relaxed);
        while (!value_.compare_exchange_weak(expected, expected + delta,
                                             std::memory_order_relaxed)) {}
    }
    
    void decrement(double delta = 1.0) {
        double expected = value_.load(std::memory_order_relaxed);
        while (!value_.compare_exchange_weak(expected, expected - delta,
                                             std::memory_order_relaxed)) {}
    }
    
    double value() const {
        return value_.load(std::memory_order_relaxed);
    }
    
    std::string serialize() const override {
        return name_ + "{} " + std::to_string(value());
    }
    
private:
    std::atomic<double> value_;  ///< Atomic gauge value
};

class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& description,
             const std::vector<double>& buckets = {0.001, 0.01, 0.1, 1.0, 10.0})
        : Metric(name, description, MetricType::HISTOGRAM),
          buckets_(buckets), counts_(buckets.size() + 1, 0), sum_(0), count_(0) {
        // Ensure buckets are sorted
        std::sort(buckets_.begin(), buckets_.end());
    }
    
    /**
     * @brief Observe.
     * @param[in] value Input parameter.
     * @details Calls: lock(), size(), back().
     */
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
    
    double sum() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return sum_;
    }
    
    uint64_t count() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
    
    double mean() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ > 0 ? sum_ / count_ : 0.0;
    }
    
    std::string serialize() const override {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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

class Timer {
public:
    /**
     * @brief Timer.
     * @param[in,out] histogram Input/output parameter.
     * @return Return value.
     */
    explicit Timer(Histogram* histogram)
        : histogram_(histogram),
          start_(std::chrono::steady_clock::now()) {}
    
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
    Timer(Timer&& other) noexcept
        : histogram_(other.histogram_), start_(other.start_) {
        other.histogram_ = nullptr;
    }
    
private:
    Histogram* histogram_;                              ///< Histogram to record into (may be null)
    std::chrono::steady_clock::time_point start_;       ///< Start time of measurement
};

class MetricsCollector {
public:
    /**
     * @brief Instance.
     * @return Return value.
     * @details Implements instance without additional internal calls.
     */
    static MetricsCollector& instance() {
        static MetricsCollector collector;
        return collector;
    }
    
    
    /**
     * @brief Register Counter.
     * @param[in] name Input parameter.
     * @param[in] description Input parameter.
     * @return Pointer to the result.
     * @details Calls: lock(), get(), std::move().
     */
    Counter* registerCounter(const std::string& name,
                            const std::string& description) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto counter = std::make_unique<Counter>(name, description);
        auto* ptr = counter.get();
        counters_[name] = std::move(counter);
        return ptr;
    }
    
    /**
     * @brief Register Gauge.
     * @param[in] name Input parameter.
     * @param[in] description Input parameter.
     * @return Pointer to the result.
     * @details Calls: lock(), get(), std::move().
     */
    Gauge* registerGauge(const std::string& name,
                        const std::string& description) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto gauge = std::make_unique<Gauge>(name, description);
        auto* ptr = gauge.get();
        gauges_[name] = std::move(gauge);
        return ptr;
    }
    
    Histogram* registerHistogram(const std::string& name,
                                const std::string& description,
                                const std::vector<double>& buckets = {}) {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        auto histogram = buckets.empty() 
            ? std::make_unique<Histogram>(name, description)
            : std::make_unique<Histogram>(name, description, buckets);
        auto* ptr = histogram.get();
        histograms_[name] = std::move(histogram);
        return ptr;
    }
    
    
    /**
     * @brief Get Counter.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     * @details Calls: lock(), find(), end(), get().
     */
    Counter* getCounter(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = counters_.find(name);
        return it != counters_.end() ? it->second.get() : nullptr;
    }
    
    /**
     * @brief Get Gauge.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     * @details Calls: lock(), find(), end(), get().
     */
    Gauge* getGauge(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = gauges_.find(name);
        return it != gauges_.end() ? it->second.get() : nullptr;
    }
    
    /**
     * @brief Get Histogram.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     * @details Calls: lock(), find(), end(), get().
     */
    Histogram* getHistogram(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = histograms_.find(name);
        return it != histograms_.end() ? it->second.get() : nullptr;
    }
    
    // ── Export metrics ────────────────────────────────────────────────────────
    
    std::string exportPrometheus() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        std::string output = {};
        
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
    
    std::string exportJSON() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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
    
    /**
     * @brief Reset the modification detection flag.
     * @details Calls: lock().
     */
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, counter] : counters_) {
            counter->reset();
        }
    }
    
    /**
     * @brief Clear.
     * @details Calls: lock().
     */
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
