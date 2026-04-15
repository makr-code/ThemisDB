/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concerns_context.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:09:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     442                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 090f93ef21  2026-03-22  feat(core): implement InMemorySecrets and EnvSecretsProvi... ║
    • e1c78c3604  2026-03-13  feat(core): implement RedisCache distributed cache adapte... ║
    • 50ae658f67  2026-03-09  feat(core): implement dynamic log level adjustment and au... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
#include "core/concerns/i_audit_log.h"
// lifecycle.h (ProbeResult, HealthStatus) is already transitively included
// via each of the four interface headers above; no direct include needed.
#include <memory>
#include <string>
#include <map>
#include <unordered_map>

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
        /// Which tracer adapter to use: "otel", "jaeger", "zipkin", "noop", or ""
        /// (auto-select based on tracingEnabled — "otel" when true, "noop" when false).
        /// For Jaeger set tracingEndpoint to the Jaeger HTTP collector
        /// (default: http://localhost:14268/api/traces).
        /// For Zipkin set tracingEndpoint to the Zipkin spans endpoint
        /// (default: http://localhost:9411/api/v2/spans).
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
        /// Which cache adapter to use: "inmemory" (default), "redis", or "noop".
        /// Set to "redis" and provide cacheRedisUrl to use the Redis-backed
        /// distributed cache with consistent hashing and pub/sub invalidation.
        std::string cacheAdapter = "inmemory";
        /// Redis URL used when cacheAdapter == "redis".
        /// Format: redis://[:<password>@]host:port[,host2:port2,...]
        /// Example: redis://cache-cluster:6379
        std::string cacheRedisUrl;

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

        // Feature flags config
        /// Which feature flag adapter to use: "inmemory" (default) or "noop".
        std::string featureFlagsAdapter = "inmemory";
        /// Pre-populated flag values for the in-memory provider.
        /// Ignored when featureFlagsAdapter is "noop".
        std::unordered_map<std::string, bool> initialFeatureFlags;

        // Secrets config
        /// Which secrets adapter to use: "noop" (default), "inmemory", or "env".
        /// "noop"     — NoOpSecrets; always returns nullopt (testing/minimal builds).
        /// "inmemory" — InMemorySecrets; map-backed; pre-populate via initialSecrets.
        /// "env"      — EnvSecretsProvider; reads from environment variables using
        ///              secretsEnvPrefix as the variable prefix.
        std::string secretsAdapter = "noop";
        /// Pre-populated key-value pairs for the "inmemory" secrets provider.
        /// Ignored when secretsAdapter is not "inmemory".
        std::map<std::string, std::string> initialSecrets;
        /// Environment-variable prefix used by the "env" secrets provider.
        /// Default: "THEMIS_SECRET_".  The secret name is upper-cased and
        /// dots/dashes are replaced with underscores before appending to this prefix.
        /// Ignored when secretsAdapter is not "env".
        std::string secretsEnvPrefix = "THEMIS_SECRET_";

        // Audit log config
        /// Which audit log adapter to use: "noop" (default) or "inmemory".
        /// In production deployments replace with a persistent adapter.
        std::string auditAdapter = "noop";
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
     *
     * @p featureFlags is optional; nullptr installs a NoOpFeatureFlags.
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ISecrets> secrets,
        std::unique_ptr<IFeatureFlags> featureFlags = nullptr
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
     * @brief Create a context with a custom audit log implementation.
     *
     * All other concerns default to no-op.  Use in tests that need to
     * verify audit records, or in deployments with a custom audit backend.
     * @p auditLog is optional; nullptr installs a NoOpAuditLog.
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ISecrets> secrets,
        std::unique_ptr<IFeatureFlags> featureFlags,
        std::unique_ptr<IAuditLog> auditLog
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
    IAuditLog& auditLog() { return *auditLog_; }

    const ILogger& logger() const { return *logger_; }
    const ITracer& tracer() const { return *tracer_; }
    const IMetrics& metrics() const { return *metrics_; }
    const ICache& cache() const { return *cache_; }
    const ISecrets& secrets() const { return *secrets_; }
    const ICircuitBreaker& circuitBreaker() const { return *circuit_breaker_; }
    const IFeatureFlags& featureFlags() const { return *featureFlags_; }
    const IAuditLog& auditLog() const { return *auditLog_; }

    // Convenience methods for common operations
    void logInfo(const std::string& message) { logger_->info(message); }
    void logError(const std::string& message) { logger_->error(message); }
    void logWarn(const std::string& message) { logger_->warn(message); }
    void logDebug(const std::string& message) { logger_->debug(message); }

    /**
     * @brief Dynamically adjust the active log level at runtime.
     *
     * Delegates to `ILogger::setLevel()` on the active logger adapter so
     * that the minimum severity threshold can be changed without restarting
     * the database process (Issue #1412).
     *
     * @param level New minimum severity level.
     */
    void setLogLevel(ILogger::Level level) { logger_->setLevel(level); }

    /**
     * @brief Return the currently active log level.
     * @return Active minimum severity level.
     */
    ILogger::Level getLogLevel() const { return logger_->getLevel(); }

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
        auditLog_->flush();
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
        auditLog_->flush();

        secrets_->shutdown();
        tracer_->shutdown();
        metrics_->shutdown();
        cache_->shutdown();
        circuit_breaker_->shutdown();
        featureFlags_->shutdown();
        auditLog_->shutdown();
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
            featureFlags_->isHealthy(),
            auditLog_->isHealthy()
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
        std::unique_ptr<IFeatureFlags> featureFlags,
        std::unique_ptr<IAuditLog> auditLog
    ) : logger_(std::move(logger)),
        tracer_(std::move(tracer)),
        metrics_(std::move(metrics)),
        cache_(std::move(cache)),
        secrets_(std::move(secrets)),
        circuit_breaker_(std::move(circuit_breaker)),
        featureFlags_(std::move(featureFlags)),
        auditLog_(std::move(auditLog)) {}

    std::unique_ptr<ILogger> logger_;
    std::unique_ptr<ITracer> tracer_;
    std::unique_ptr<IMetrics> metrics_;
    std::unique_ptr<ICache> cache_;
    std::unique_ptr<ISecrets> secrets_;
    std::unique_ptr<ICircuitBreaker> circuit_breaker_;
    std::unique_ptr<IFeatureFlags> featureFlags_;
    std::unique_ptr<IAuditLog> auditLog_;
};

} // namespace concerns
} // namespace core
} // namespace themis
