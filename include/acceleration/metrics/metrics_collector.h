/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics_collector.h                                ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:13:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     404                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Metric types supported by the metrics system
 */
enum class MetricType {
    COUNTER,    // Monotonically increasing value
    GAUGE,      // Value that can go up or down
    HISTOGRAM,  // Distribution of values
    SUMMARY     // Quantiles of values
};

/**
 * @brief Base class for all metrics
 */
class Metric {
public:
    explicit Metric(const std::string& name, const std::string& description,
                   MetricType type)
        : name_(name), description_(description), type_(type) {}
    
    virtual ~Metric() = default;
    
    const std::string& name() const { return name_; }
    const std::string& description() const { return description_; }
    MetricType type() const { return type_; }
    
    virtual std::string serialize() const = 0;
    
protected:
    std::string name_;
    std::string description_;
    MetricType type_;
};

/**
 * @brief Counter metric - monotonically increasing value
 */
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
    
    void reset() {
        value_.store(0, std::memory_order_relaxed);
    }
    
    std::string serialize() const override {
        return name_ + "{} " + std::to_string(value());
    }
    
private:
    std::atomic<uint64_t> value_;
};

/**
 * @brief Gauge metric - value that can go up or down
 */
class Gauge : public Metric {
public:
    Gauge(const std::string& name, const std::string& description)
        : Metric(name, description, MetricType::GAUGE), value_(0.0) {}
    
    void set(double value) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ = value;
    }
    
    void increment(double delta = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ += delta;
    }
    
    void decrement(double delta = 1.0) {
        std::lock_guard<std::mutex> lock(mutex_);
        value_ -= delta;
    }
    
    double value() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_;
    }
    
    std::string serialize() const override {
        return name_ + "{} " + std::to_string(value());
    }
    
private:
    mutable std::mutex mutex_;
    double value_;
};

/**
 * @brief Histogram metric - distribution of values
 * Simplified implementation with fixed buckets
 */
class Histogram : public Metric {
public:
    Histogram(const std::string& name, const std::string& description,
             const std::vector<double>& buckets = {0.001, 0.01, 0.1, 1.0, 10.0})
        : Metric(name, description, MetricType::HISTOGRAM),
          buckets_(buckets), counts_(buckets.size() + 1, 0), sum_(0), count_(0) {
        // Ensure buckets are sorted
        std::sort(buckets_.begin(), buckets_.end());
    }
    
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
        std::lock_guard<std::mutex> lock(mutex_);
        return sum_;
    }
    
    uint64_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
    
    double mean() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_ > 0 ? sum_ / count_ : 0.0;
    }
    
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
    mutable std::mutex mutex_;
    std::vector<double> buckets_;
    std::vector<uint64_t> counts_;
    double sum_;
    uint64_t count_;
};

/**
 * @brief RAII timer for measuring operation duration
 */
class Timer {
public:
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
    Histogram* histogram_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * @brief Central metrics collector
 * Thread-safe singleton for collecting and exporting metrics
 */
class MetricsCollector {
public:
    static MetricsCollector& instance() {
        static MetricsCollector collector;
        return collector;
    }
    
    // Register metrics
    Counter* registerCounter(const std::string& name,
                            const std::string& description) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto counter = std::make_unique<Counter>(name, description);
        auto* ptr = counter.get();
        counters_[name] = std::move(counter);
        return ptr;
    }
    
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
        std::lock_guard<std::mutex> lock(mutex_);
        auto histogram = buckets.empty() 
            ? std::make_unique<Histogram>(name, description)
            : std::make_unique<Histogram>(name, description, buckets);
        auto* ptr = histogram.get();
        histograms_[name] = std::move(histogram);
        return ptr;
    }
    
    // Get existing metrics
    Counter* getCounter(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = counters_.find(name);
        return it != counters_.end() ? it->second.get() : nullptr;
    }
    
    Gauge* getGauge(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = gauges_.find(name);
        return it != gauges_.end() ? it->second.get() : nullptr;
    }
    
    Histogram* getHistogram(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = histograms_.find(name);
        return it != histograms_.end() ? it->second.get() : nullptr;
    }
    
    // Export metrics in Prometheus format
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
    
    // Export metrics in JSON format
    std::string exportJSON() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string output = "{\n";
        
        output += "  \"counters\": {\n";
        bool first = true;
        for (const auto& [name, counter] : counters_) {
            if (!first) output += ",\n";
            output += "    \"" + name + "\": " + std::to_string(counter->value());
            first = false;
        }
        output += "\n  },\n";
        
        output += "  \"gauges\": {\n";
        first = true;
        for (const auto& [name, gauge] : gauges_) {
            if (!first) output += ",\n";
            output += "    \"" + name + "\": " + std::to_string(gauge->value());
            first = false;
        }
        output += "\n  },\n";
        
        output += "  \"histograms\": {\n";
        first = true;
        for (const auto& [name, histogram] : histograms_) {
            if (!first) output += ",\n";
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
    
    // Reset all metrics
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, counter] : counters_) {
            counter->reset();
        }
        // Note: Gauges and histograms are not reset as they represent current state
    }
    
    // Clear all metrics
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
    
    mutable std::mutex mutex_;
    std::map<std::string, std::unique_ptr<Counter>> counters_;
    std::map<std::string, std::unique_ptr<Gauge>> gauges_;
    std::map<std::string, std::unique_ptr<Histogram>> histograms_;
};

} // namespace metrics
} // namespace acceleration
} // namespace themis
