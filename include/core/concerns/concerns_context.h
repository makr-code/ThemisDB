#pragma once

#include "core/concerns/i_logger.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/i_metrics.h"
#include "core/concerns/i_cache.h"
#include <memory>
#include <string>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Central context for all cross-cutting concerns.
 * 
 * Provides dependency injection for logging, tracing, metrics, and caching
 * throughout the application. Components receive a ConcernsContext and use
 * it to access these services.
 * 
 * Usage:
 *   auto context = ConcernsContext::create();
 *   context->logger().info("Starting component");
 *   auto span = context->tracer().startSpan("operation");
 *   context->metrics().incrementCounter("requests_total");
 */
class ConcernsContext {
public:
    /**
     * @brief Configuration for concern implementations.
     */
    struct Config {
        // Logger config
        std::string logFile = "themisdb.log";
        std::string logLevel = "info";
        std::string logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
        /// When true, all structured log calls emit single-line JSON objects.
        bool jsonLogging = false;
        
        // Tracer config
        bool tracingEnabled = false;
        std::string tracingServiceName = "themisdb";
        std::string tracingEndpoint = "http://localhost:4318";
        
        // Metrics config
        bool metricsEnabled = true;
        /// Maximum number of unique label-set combinations allowed per metric name.
        /// Prevents unbounded cardinality growth from high-cardinality labels.
        /// Set to 0 to disable the limit.
        size_t maxMetricCardinality = 1000;
        
        // Cache config
        size_t cacheMaxSize = 10000;
        uint64_t cacheDefaultTTL = 0; // 0 = no TTL
    };

    /**
     * @brief Create a default context with production implementations.
     */
    static std::shared_ptr<ConcernsContext> create();
    static std::shared_ptr<ConcernsContext> create(const Config& config);

    /**
     * @brief Create a context with custom implementations (for testing).
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache
    );

    /**
     * @brief Create a no-op context (all concerns disabled).
     */
    static std::shared_ptr<ConcernsContext> createNoOp();

    // Accessor methods
    ILogger& logger() { return *logger_; }
    ITracer& tracer() { return *tracer_; }
    IMetrics& metrics() { return *metrics_; }
    ICache& cache() { return *cache_; }

    const ILogger& logger() const { return *logger_; }
    const ITracer& tracer() const { return *tracer_; }
    const IMetrics& metrics() const { return *metrics_; }
    const ICache& cache() const { return *cache_; }

    // Convenience methods for common operations
    void logInfo(const std::string& message) { logger_->info(message); }
    void logError(const std::string& message) { logger_->error(message); }
    void logWarn(const std::string& message) { logger_->warn(message); }
    void logDebug(const std::string& message) { logger_->debug(message); }

    std::unique_ptr<ITracer::ISpan> startSpan(const std::string& name) {
        return tracer_->startSpan(name);
    }

    void recordMetric(const std::string& name, double value) {
        metrics_->observeHistogram(name, value);
    }

private:
    ConcernsContext(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache
    ) : logger_(std::move(logger)),
        tracer_(std::move(tracer)),
        metrics_(std::move(metrics)),
        cache_(std::move(cache)) {}

    std::unique_ptr<ILogger> logger_;
    std::unique_ptr<ITracer> tracer_;
    std::unique_ptr<IMetrics> metrics_;
    std::unique_ptr<ICache> cache_;
};

} // namespace concerns
} // namespace core
} // namespace themis
