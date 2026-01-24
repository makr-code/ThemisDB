#pragma once

#include "core/concerns/i_logger.h"

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief No-op logger implementation for testing or when logging is disabled.
 */
class NoOpLogger : public ILogger {
public:
    void log(Level level, const std::string& message) override {}
    void trace(const std::string& message) override {}
    void debug(const std::string& message) override {}
    void info(const std::string& message) override {}
    void warn(const std::string& message) override {}
    void error(const std::string& message) override {}
    void critical(const std::string& message) override {}
    
    void setLevel(Level level) override { level_ = level; }
    Level getLevel() const override { return level_; }
    void setPattern(const std::string& pattern) override {}

private:
    Level level_ = Level::INFO;
};

/**
 * @brief No-op tracer implementation for testing or when tracing is disabled.
 */
class NoOpTracer : public ITracer {
public:
    class NoOpSpan : public ISpan {
    public:
        void setAttribute(const std::string& key, const std::string& value) override {}
        void setAttribute(const std::string& key, int64_t value) override {}
        void setAttribute(const std::string& key, double value) override {}
        void setAttribute(const std::string& key, bool value) override {}
        void recordError(const std::string& errorMessage) override {}
        void setStatus(bool ok, const std::string& description = "") override {}
        void end() override {}
        bool isValid() const override { return false; }
    };

    std::unique_ptr<ISpan> startSpan(const std::string& name) override {
        return std::make_unique<NoOpSpan>();
    }

    std::unique_ptr<ISpan> startChildSpan(const std::string& name, const ISpan& parent) override {
        return std::make_unique<NoOpSpan>();
    }

    bool initialize(const std::string& serviceName, const std::string& endpoint) override {
        return true;
    }

    void shutdown() override {}
    bool isInitialized() const override { return true; }
};

/**
 * @brief No-op metrics implementation for testing or when metrics are disabled.
 */
class NoOpMetrics : public IMetrics {
public:
    void incrementCounter(const std::string& name, int64_t value = 1, const Labels& labels = {}) override {}
    void setGauge(const std::string& name, double value, const Labels& labels = {}) override {}
    void incrementGauge(const std::string& name, double delta, const Labels& labels = {}) override {}
    void decrementGauge(const std::string& name, double delta, const Labels& labels = {}) override {}
    void observeHistogram(const std::string& name, double value, const Labels& labels = {}) override {}
    void recordLatency(const std::string& operation, double latencyMs, const Labels& labels = {}) override {}
    void recordError(const std::string& operation, const Labels& labels = {}) override {}
    void recordSuccess(const std::string& operation, const Labels& labels = {}) override {}
    std::string exportMetrics() const override { return ""; }
    void reset() override {}
};

/**
 * @brief No-op cache implementation for testing or when caching is disabled.
 */
class NoOpCache : public ICache {
public:
    std::optional<CacheEntry> get(std::string_view key) override { return std::nullopt; }
    bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) override { return true; }
    void invalidate(std::string_view key) override {}
    void clear() override {}
    void invalidatePattern(std::string_view pattern) override {}
    size_t size() const override { return 0; }
    uint64_t hitCount() const override { return 0; }
    uint64_t missCount() const override { return 0; }
    double hitRate() const override { return 0.0; }
    void setMaxSize(size_t maxSize) override {}
    void setDefaultTTL(uint64_t ttl_ms) override {}
};

} // namespace concerns
} // namespace core
} // namespace themis
