/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concerns_context.h                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     236                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_logger.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/i_metrics.h"
#include "core/concerns/i_cache.h"
// lifecycle.h (ProbeResult, HealthStatus) is already transitively included
// via each of the four interface headers above; no direct include needed.
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

    // -------------------------------------------------------------------------
    // Lifecycle hooks
    // -------------------------------------------------------------------------

    /**
     * @brief Flush all buffered data in every concern.
     *
     * Call this when you want to ensure pending log records, spans, and
     * metric observations have been forwarded to their respective sinks
     * without fully shutting down.  Safe to call multiple times.
     */
    void flush() {
        logger_->flush();
        tracer_->flush();
        metrics_->flush();
        cache_->flush();
    }

    /**
     * @brief Gracefully shut down all concerns and release resources.
     *
     * Flushes pending data before tearing down each concern.  After this
     * call the context must not be used; any further accessor calls have
     * undefined behaviour.
     *
     * Recommended usage:
     * @code
     *   // Register at application entry point
     *   std::atexit([]{ concerns->shutdown(); });
     *   // Or call explicitly in the signal handler / destructor.
     * @endcode
     */
    void shutdown() {
        logger_->flush();
        tracer_->flush();
        metrics_->flush();

        tracer_->shutdown();
        metrics_->shutdown();
        cache_->shutdown();
        logger_->shutdown();
    }

    // -------------------------------------------------------------------------
    // Health / readiness probes
    // -------------------------------------------------------------------------

    /**
     * @brief Run health probes for every concern.
     *
     * A concern is "healthy" when its underlying resource (sink, exporter,
     * backend) is operational.  The aggregate result is unhealthy if any
     * single concern reports unhealthy.
     *
     * Intended for liveness probes (e.g. Kubernetes /healthz).
     *
     * @return HealthStatus with per-concern ProbeResult instances.
     */
    HealthStatus healthCheck() const {
        return {
            logger_->isHealthy(),
            tracer_->isHealthy(),
            metrics_->isHealthy(),
            cache_->isHealthy()
        };
    }

    /**
     * @brief Run readiness probes for every concern.
     *
     * A concern is "ready" when it is initialized and capable of
     * accepting work.  For most in-process concerns readiness equals
     * health; for remote backends (e.g. a distributed cache) readiness
     * may require an active connection.
     *
     * Intended for readiness probes (e.g. Kubernetes /readyz).
     *
     * @return HealthStatus with per-concern ProbeResult instances.
     */
    HealthStatus readinessCheck() const {
        // For current implementations readiness == health.
        // Remote-backend adapters (Redis, etc.) may override isHealthy()
        // with a live ping to their backend.
        return healthCheck();
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
