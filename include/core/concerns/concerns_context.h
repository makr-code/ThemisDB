/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concerns_context.h                                 ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     252                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • 7f17c52b6  2026-02-21  feat(core): structured log correlation — span_id in Trace... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_logger.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/i_metrics.h"
#include "core/concerns/i_cache.h"
#include "core/concerns/i_secrets.h"
#include "core/concerns/i_circuit_breaker.h"
#include "core/concerns/i_feature_flags.h"
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
        /// Which logger adapter to use: "spdlog" (default) or "noop".
        std::string loggerAdapter = "spdlog";
        
        // Tracer config
        bool tracingEnabled = false;
        std::string tracingServiceName = "themisdb";
        std::string tracingEndpoint = "http://localhost:4318";
        /// Which tracer adapter to use: "otel", "noop", or "" (auto-select
        /// based on tracingEnabled — "otel" when true, "noop" when false).
        std::string tracerAdapter = "";
        
        // Metrics config
        bool metricsEnabled = true;
        /// Maximum number of unique label-set combinations allowed per metric name.
        /// Prevents unbounded cardinality growth from high-cardinality labels.
        /// Set to 0 to disable the limit.
        size_t maxMetricCardinality = 1000;
        /// Which metrics adapter to use: "prometheus", "noop", or "" (auto-select
        /// based on metricsEnabled — "prometheus" when true, "noop" when false).
        std::string metricsAdapter = "";
        
        // Cache config
        size_t cacheMaxSize = 10000;
        uint64_t cacheDefaultTTL = 0; // 0 = no TTL
        /// Which cache adapter to use: "inmemory" (default) or "noop".
        std::string cacheAdapter = "inmemory";

        // Circuit breaker config
        /// Which circuit breaker adapter to use: "default" or "noop".
        std::string circuitBreakerAdapter = "default";
        /// Failure threshold before opening the circuit.
        size_t circuitBreakerFailureThreshold = 5;
        /// Seconds the circuit stays OPEN before probing recovery.
        std::chrono::seconds circuitBreakerTimeout = std::chrono::seconds(30);
        /// Consecutive successes in HALF_OPEN required to close the circuit.
        size_t circuitBreakerSuccessThreshold = 2;
        /// Rolling window for counting failures.
        std::chrono::seconds circuitBreakerFailureWindow = std::chrono::seconds(60);
    };

    /**
     * @brief Create a default context with production implementations.
     */
    static std::shared_ptr<ConcernsContext> create();
    static std::shared_ptr<ConcernsContext> create(const Config& config);

    /**
     * @brief Create a context with custom implementations (for testing).
     *
     * The @p secrets parameter is optional; when nullptr a no-op provider is
     * used so that existing call-sites do not need to be updated.
     * The 4-argument overload automatically installs a NoOpFeatureFlags so
     * that existing call-sites do not need to be updated.
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ICircuitBreaker> circuit_breaker = nullptr
    );

    /**
     * @brief Create a context with custom secrets implementation.
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ISecrets> secrets
    );

    /**
     * @brief Create a context with custom feature-flag implementation.
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<IFeatureFlags> featureFlags
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
    ISecrets& secrets() { return *secrets_; }
    ICircuitBreaker& circuitBreaker() { return *circuit_breaker_; }
    IFeatureFlags& featureFlags() { return *featureFlags_; }

    const ILogger& logger() const { return *logger_; }
    const ITracer& tracer() const { return *tracer_; }
    const IMetrics& metrics() const { return *metrics_; }
    const ICache& cache() const { return *cache_; }
    const ISecrets& secrets() const { return *secrets_; }
    const ICircuitBreaker& circuitBreaker() const { return *circuit_breaker_; }
    const IFeatureFlags& featureFlags() const { return *featureFlags_; }

    // Convenience methods for common operations
    void logInfo(const std::string& message) { logger_->info(message); }
    void logError(const std::string& message) { logger_->error(message); }
    void logWarn(const std::string& message) { logger_->warn(message); }
    void logDebug(const std::string& message) { logger_->debug(message); }

    std::unique_ptr<ITracer::ISpan> startSpan(const std::string& name) {
        return tracer_->startSpan(name);
    }

    /**
     * @brief Extract W3C TraceContext from inbound headers and start a linked
     *        span via the active tracer.
     *
     * Delegates to ITracer::startSpanFromHeaders().
     */
    std::unique_ptr<ITracer::ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>& headers) {
        return tracer_->startSpanFromHeaders(name, headers);
    }

    /**
     * @brief Inject the active span's W3C TraceContext into outgoing headers
     *        via the active tracer.
     *
     * Delegates to ITracer::injectContext().
     */
    void injectContext(std::map<std::string, std::string>& headers) {
        tracer_->injectContext(headers);
    }

    void recordMetric(const std::string& name, double value) {
        metrics_->observeHistogram(name, value);
    }

    /**
     * @brief Emit a structured log record with the active trace/span IDs
     *        automatically injected.
     *
     * Fetches the current thread's OpenTelemetry trace-id and span-id via
     * `Tracer::getCurrentTraceId()` / `Tracer::getCurrentSpanId()` and
     * forwards them together with @p fields to `ILogger::logWithContext()`.
     *
     * Use this instead of `logInfo()` / `logError()` when you want log lines
     * to be correlated with the active distributed trace without manually
     * building a `TraceContext`.
     *
     * @param level   Severity level.
     * @param message Human-readable log text.
     * @param fields  Optional additional structured key/value fields.
     */
    void logWithTrace(ILogger::Level level,
                      const std::string& message,
                      const ILogger::Fields& fields = {});

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
        secrets_->flush();
        circuit_breaker_->flush();
        featureFlags_->flush();
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
        featureFlags_->flush();

        secrets_->shutdown();
        tracer_->shutdown();
        metrics_->shutdown();
        cache_->shutdown();
        circuit_breaker_->shutdown();
        featureFlags_->shutdown();
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
            cache_->isHealthy(),
            secrets_->isHealthy(),
            circuit_breaker_->isHealthy(),
            featureFlags_->isHealthy()
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
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ICircuitBreaker> circuit_breaker,
        std::unique_ptr<ISecrets> secrets,
        std::unique_ptr<IFeatureFlags> featureFlags
    ) : logger_(std::move(logger)),
        tracer_(std::move(tracer)),
        metrics_(std::move(metrics)),
        cache_(std::move(cache)),
        secrets_(std::move(secrets)),
        circuit_breaker_(std::move(circuit_breaker)),
        featureFlags_(std::move(featureFlags)) {}

    std::unique_ptr<ILogger> logger_;
    std::unique_ptr<ITracer> tracer_;
    std::unique_ptr<IMetrics> metrics_;
    std::unique_ptr<ICache> cache_;
    std::unique_ptr<ISecrets> secrets_;
    std::unique_ptr<ICircuitBreaker> circuit_breaker_;
    std::unique_ptr<IFeatureFlags> featureFlags_;
};

} // namespace concerns
} // namespace core
} // namespace themis
