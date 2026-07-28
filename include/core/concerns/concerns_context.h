/**
 * @file concerns_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=17; TODO=1, Stub=15, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include "core/concerns/adapter_registry.h"
// lifecycle.h (ProbeResult, HealthStatus) is already transitively included
// via each of the four interface headers above; no direct include needed.
#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <unordered_map>
#include <type_traits>

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
     *
     * Uses the default config values from Config and applies the repository's
     * fail-closed adapter selection rules. If production mode is enabled, the
     * resulting context is required to use production-capable adapters.
     *
     * @return Shared context configured from default settings.
     */
    static std::shared_ptr<ConcernsContext> create();

    /**
     * @brief Create a context from an explicit configuration object.
     *
     * The configuration is validated before any adapter objects are created.
     * Invalid log, tracing, cache, or adapter settings raise
     * std::runtime_error and do not yield a partially constructed context.
     *
     * @param config Runtime configuration for concern adapters and limits.
     * @return Shared context configured from @p config.
     * @throws std::runtime_error if validation fails or a production-only
     *         adapter requirement is not satisfied.
     */
    static std::shared_ptr<ConcernsContext> create(const Config& config);

    /**
     * @brief Create a context with custom implementations (for testing).
     *
    * The @p circuit_breaker parameter is optional; when nullptr a no-op
    * provider is used so that existing call-sites do not need to be updated.
    * The 4-argument overload automatically installs a NoOpFeatureFlags so
    * that existing call-sites do not need to be updated.
     *
     * @param logger           Logger implementation to install.
     * @param tracer           Tracer implementation to install.
     * @param metrics          Metrics implementation to install.
     * @param cache            Cache implementation to install.
     * @param circuit_breaker  Optional circuit-breaker implementation.
     * @return Shared context backed by the supplied adapters.
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
     *
     * @param logger        Logger implementation to install.
     * @param tracer        Tracer implementation to install.
     * @param metrics       Metrics implementation to install.
     * @param cache         Cache implementation to install.
     * @param secrets       Secrets implementation to install.
     * @param featureFlags  Optional feature-flag implementation.
     * @return Shared context backed by the supplied adapters.
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
     *
     * @param logger        Logger implementation to install.
     * @param tracer        Tracer implementation to install.
     * @param metrics       Metrics implementation to install.
     * @param cache         Cache implementation to install.
     * @param featureFlags  Feature-flag implementation to install.
     * @return Shared context backed by the supplied adapters.
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
     *
     * @param logger        Logger implementation to install.
     * @param tracer        Tracer implementation to install.
     * @param metrics       Metrics implementation to install.
     * @param cache         Cache implementation to install.
     * @param secrets       Secrets implementation to install.
     * @param featureFlags  Feature-flag implementation to install.
     * @param auditLog      Audit-log implementation to install.
     * @return Shared context backed by the supplied adapters.
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
      *
      * The returned context is suitable for tests that only need the wiring
      * surface but not actual side effects.
      *
      * @return Shared context using only no-op adapters.
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

    // -------------------------------------------------------------------------
    // Generic type-safe adapter resolution (Phase 1/2 — Issue #5638)
    // -------------------------------------------------------------------------

    /**
     * @brief Resolve an adapter by concrete type @c T in a thread-safe,
     *        ref-counted manner.
     *
     * For the eight built-in concern types (@c ILogger, @c ITracer,
     * @c IMetrics, @c ICache, @c ISecrets, @c IFeatureFlags, @c IAuditLog,
     * @c ICircuitBreaker) the method bridges directly to the corresponding
     * named accessor and returns a non-owning shared_ptr whose lifetime is
     * bounded by this @c ConcernsContext instance.
     *
     * For all other types the call is forwarded to the embedded
     * @c AdapterRegistry (see @c registry()).
     *
     * @tparam T  Adapter interface or concrete type to resolve.
     * @return    @c std::shared_ptr<T> to the active adapter, or @c nullptr
     *            if not registered.
     *
     * @note For built-in concern types the returned shared_ptr uses a no-op
     *       deleter — the context retains sole ownership.  Callers must not
     *       store the handle beyond the lifetime of this @c ConcernsContext.
     *
     * @threadsafety Safe to call concurrently; uses a brief shared read lock
     *              for built-in concern types.
     */
    template<typename T>
    std::shared_ptr<T> resolve() const {
        if constexpr (std::is_same_v<T, ILogger>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(logger_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, ITracer>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(tracer_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, IMetrics>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(metrics_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, ICache>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(cache_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, ISecrets>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(secrets_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, IFeatureFlags>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(featureFlags_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, IAuditLog>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(auditLog_.get(), [](T*) noexcept {});
        } else if constexpr (std::is_same_v<T, ICircuitBreaker>) {
            std::lock_guard<std::mutex> lk(adapters_mutex_);
            return std::shared_ptr<T>(circuit_breaker_.get(), [](T*) noexcept {});
        } else {
            return registry_->resolve<T>();
        }
    }

    /**
     * @brief Access the embedded AdapterRegistry for custom adapter types.
     *
     * Use this to register non-built-in adapters and resolve them via
     * @c resolve<T>().
     *
     * @return Mutable reference to the embedded @c AdapterRegistry.
     */
    AdapterRegistry& registry() { return *registry_; }

    /**
     * @brief Read-only access to the embedded AdapterRegistry.
     * @return Const reference to the embedded @c AdapterRegistry.
     */
    const AdapterRegistry& registry() const { return *registry_; }

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
        * @throws std::runtime_error if the logger adapter is unavailable.
     */
    void setLogLevel(ILogger::Level level) { logger_->setLevel(level); }

    /**
     * @brief Return the currently active log level.
     * @return Active minimum severity level.
     */
    ILogger::Level getLogLevel() const { return logger_->getLevel(); }

    // -------------------------------------------------------------------------
    // Dynamic Adapter Reconfiguration (Issue #1412 / core/FUTURE_ENHANCEMENTS.md)
    //
    // These methods replace an active concern adapter at runtime without
    // restarting the database process.  The old adapter is flushed before the
    // swap so that no buffered data is lost.  All replace* calls are
    // thread-safe: a brief exclusive lock is taken only to swap the pointer;
    // in-flight calls on the old adapter complete before the old object is
    // destroyed (the caller retains a shared_ptr reference until the return).
    //
    // Passing nullptr is rejected (throws std::invalid_argument).
    // -------------------------------------------------------------------------

    /**
    * @brief Swap the active logger adapter.
     *
    * Flushes the current adapter before installing @p new_logger. After
    * this call, `logger()` returns a reference to the new adapter. Callers
    * should expect a brief synchronization point while the swap occurs.
     *
     * Thread-safety: safe to call while other threads are logging.
     *
     * @param new_logger Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_logger is nullptr.
     */
    void replaceLogger(std::unique_ptr<ILogger> new_logger);

    /**
     * @brief Swap the active tracer adapter.
     *
     * Flushes and shuts down the current adapter before installing
     * @p new_tracer.
     *
     * @param new_tracer Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_tracer is nullptr.
     */
    void replaceTracer(std::unique_ptr<ITracer> new_tracer);

    /**
     * @brief Swap the active metrics adapter.
     *
     * Flushes the current adapter before installing @p new_metrics.
     *
     * @param new_metrics Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_metrics is nullptr.
     */
    void replaceMetrics(std::unique_ptr<IMetrics> new_metrics);

    /**
     * @brief Swap the active cache adapter.
     *
     * Flushes the current adapter before installing @p new_cache.
     *
     * @param new_cache Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_cache is nullptr.
     */
    void replaceCache(std::unique_ptr<ICache> new_cache);

    /**
     * @brief Swap the active secrets adapter.
     *
     * Flushes the current adapter before installing @p new_secrets.
     *
     * @param new_secrets Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_secrets is nullptr.
     */
    void replaceSecrets(std::unique_ptr<ISecrets> new_secrets);

    /**
     * @brief Swap the active feature-flags adapter.
     *
     * Flushes the current adapter before installing @p new_ff.
     *
     * @param new_ff Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_ff is nullptr.
     */
    void replaceFeatureFlags(std::unique_ptr<IFeatureFlags> new_ff);

    /**
     * @brief Swap the active audit-log adapter.
     *
     * Flushes the current adapter before installing @p new_audit.
     *
     * @param new_audit Replacement adapter; must not be nullptr.
     * @throws std::invalid_argument if @p new_audit is nullptr.
     */
    void replaceAuditLog(std::unique_ptr<IAuditLog> new_audit);

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
    * @throws std::runtime_error if the logger adapter cannot accept a
    *         structured event.
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
      * Any individual adapter failure should be handled by the adapter's own
      * implementation contract; the context does not swallow adapter errors.
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
      * Shutdown is idempotent from the caller's perspective, but adapters may
      * still reject repeated teardown if they enforce their own lifecycle.
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
        auditLog_(std::move(auditLog)),
        registry_(std::make_unique<AdapterRegistry>()) {}

    std::unique_ptr<ILogger> logger_;
    std::unique_ptr<ITracer> tracer_;
    std::unique_ptr<IMetrics> metrics_;
    std::unique_ptr<ICache> cache_;
    std::unique_ptr<ISecrets> secrets_;
    std::unique_ptr<ICircuitBreaker> circuit_breaker_;
    std::unique_ptr<IFeatureFlags> featureFlags_;
    std::unique_ptr<IAuditLog> auditLog_;
    /// Embedded registry for custom (non-built-in) adapter types.
    std::unique_ptr<AdapterRegistry> registry_;
    /// Guards all replaceX() adapter swaps.
    mutable std::mutex adapters_mutex_;
};

} // namespace concerns
} // namespace core
} // namespace themis
