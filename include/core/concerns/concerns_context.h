/**
 * @file concerns_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
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
#include <shared_mutex>
#include <string>
#include <map>
#include <unordered_map>
#include <type_traits>

namespace themis {
namespace core {
namespace concerns {

class ConcernsContext {
public:
    struct Config {
        // Logger config
        std::string logFile = "themisdb.log";
        std::string logLevel = "info";
        std::string logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";
        bool jsonLogging = false;
        std::string loggerAdapter = "spdlog";
        
        // Tracer config
        bool tracingEnabled = false;
        std::string tracingServiceName = "themisdb";
        std::string tracingEndpoint = "http://localhost:4318";
        std::string tracerAdapter = "";
        
        // Metrics config
        bool metricsEnabled = true;
        size_t maxMetricCardinality = 1000;
        std::string metricsAdapter = "";
        
        // Cache config
        size_t cacheMaxSize = 10000;
        uint64_t cacheDefaultTTL = 0; // 0 = no TTL
        std::string cacheAdapter = "inmemory";
        std::string cacheRedisUrl;

        // Circuit breaker config
        std::string circuitBreakerAdapter = "default";
        size_t circuitBreakerFailureThreshold = 5;
        std::chrono::seconds circuitBreakerTimeout = std::chrono::seconds(30);
        size_t circuitBreakerSuccessThreshold = 2;
        std::chrono::seconds circuitBreakerFailureWindow = std::chrono::seconds(60);

        // Feature flags config
        std::string featureFlagsAdapter = "inmemory";
        std::unordered_map<std::string, bool> initialFeatureFlags;

        // Secrets config
        std::string secretsAdapter = "noop";
        std::map<std::string, std::string> initialSecrets;
        std::string secretsEnvPrefix = "THEMIS_SECRET_";

        // Audit log config
        std::string auditAdapter = "noop";
    };

    /**
     * @brief Create.
     * @return Return value.
     */
    static std::shared_ptr<ConcernsContext> create();

    /**
     * @brief Create.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<ConcernsContext> create(const Config& config);

    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ICircuitBreaker> circuit_breaker = nullptr
    );

    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<ISecrets> secrets,
        std::unique_ptr<IFeatureFlags> featureFlags = nullptr
    );

    /**
     * @brief Create Custom.
     * @param[in] logger Input parameter.
     * @param[in] tracer Input parameter.
     * @param[in] metrics Input parameter.
     * @param[in] cache Input parameter.
     * @param[in] featureFlags Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<ConcernsContext> createCustom(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<ITracer> tracer,
        std::unique_ptr<IMetrics> metrics,
        std::unique_ptr<ICache> cache,
        std::unique_ptr<IFeatureFlags> featureFlags
    );

    /**
     * @brief Create Custom.
     * @param[in] logger Input parameter.
     * @param[in] tracer Input parameter.
     * @param[in] metrics Input parameter.
     * @param[in] cache Input parameter.
     * @param[in] secrets Input parameter.
     * @param[in] featureFlags Input parameter.
     * @param[in] auditLog Input parameter.
     * @return Return value.
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
     * @brief Create No Op.
     * @return Return value.
     */
    static std::shared_ptr<ConcernsContext> createNoOp();

    // Accessor methods
    /**
     * @brief Logger.
     * @return Return value.
     * @details Implements logger without additional internal calls.
     */
    ILogger& logger() { return *logger_; }
    /**
     * @brief Tracer.
     * @return Return value.
     * @details Implements tracer without additional internal calls.
     */
    ITracer& tracer() { return *tracer_; }
    /**
     * @brief Metrics.
     * @return Return value.
     * @details Implements metrics without additional internal calls.
     */
    IMetrics& metrics() { return *metrics_; }
    /**
     * @brief Cache.
     * @return Return value.
     * @details Implements cache without additional internal calls.
     */
    ICache& cache() { return *cache_; }
    /**
     * @brief Secrets.
     * @return Return value.
     * @details Implements secrets without additional internal calls.
     */
    ISecrets& secrets() { return *secrets_; }
    /**
     * @brief Circuit Breaker.
     * @return Return value.
     * @details Implements circuitBreaker without additional internal calls.
     */
    ICircuitBreaker& circuitBreaker() { return *circuit_breaker_; }
    /**
     * @brief Feature Flags.
     * @return Return value.
     * @details Implements featureFlags without additional internal calls.
     */
    IFeatureFlags& featureFlags() { return *featureFlags_; }
    /**
     * @brief Audit Log.
     * @return Return value.
     * @details Implements auditLog without additional internal calls.
     */
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

    template<typename T>
    std::shared_ptr<T> resolve() const {
        if constexpr (std::is_same_v<T, ILogger>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(logger_);
        } else if constexpr (std::is_same_v<T, ITracer>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(tracer_);
        } else if constexpr (std::is_same_v<T, IMetrics>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(metrics_);
        } else if constexpr (std::is_same_v<T, ICache>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(cache_);
        } else if constexpr (std::is_same_v<T, ISecrets>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(secrets_);
        } else if constexpr (std::is_same_v<T, IFeatureFlags>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(featureFlags_);
        } else if constexpr (std::is_same_v<T, IAuditLog>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(auditLog_);
        } else if constexpr (std::is_same_v<T, ICircuitBreaker>) {
            /**
             * @brief Lk.
             * @param[in] adapters_mutex_ Input parameter.
             * @return Return value.
             */
            std::shared_lock<std::shared_mutex> lk(adapters_mutex_);
            return std::static_pointer_cast<T>(circuit_breaker_);
        } else {
            return registry_->resolve<T>();
        }
    }

    /**
     * @brief Registry.
     * @return Return value.
     * @details Implements registry without additional internal calls.
     */
    AdapterRegistry& registry() { return *registry_; }

    const AdapterRegistry& registry() const { return *registry_; }

    /**
     * @brief Convenience methods for common operations
     * @param[in] message Input parameter.
     * @details Calls: info().
     */
    void logInfo(const std::string& message) { logger_->info(message); }
    /**
     * @brief Log Error.
     * @param[in] message Input parameter.
     * @details Calls: error().
     */
    void logError(const std::string& message) { logger_->error(message); }
    /**
     * @brief Log Warn.
     * @param[in] message Input parameter.
     * @details Calls: warn().
     */
    void logWarn(const std::string& message) { logger_->warn(message); }
    /**
     * @brief Log Debug.
     * @param[in] message Input parameter.
     * @details Calls: debug().
     */
    void logDebug(const std::string& message) { logger_->debug(message); }

    /**
     * @brief Set Log Level.
     * @param[in] level Input parameter.
     * @details Calls: setLevel().
     */
    void setLogLevel(ILogger::Level level) { logger_->setLevel(level); }

    ILogger::Level getLogLevel() const { return logger_->getLevel(); }

    /**
     * @brief ------------------------------------------------------------------------- Dynamic Adapter Reconfiguration (Issue #1412 / core/FUTURE_ENHANCEMENTS.
     * @param[in] new_logger Input parameter.
     * @details md) These methods replace an active concern adapter at runtime without restarting the database process. The old adapter is flushed before the swap so that no buffered data is lost. All replace* calls are thread-safe for callers that resolve adapters through resolve<T>() shared_ptr snapshots: a brief exclusive lock is taken only to swap the pointer; in-flight calls on the old adapter complete while those snapshots remain alive. Passing nullptr is rejected (throws std::invalid_argument). -------------------------------------------------------------------------
     */

    void replaceLogger(std::unique_ptr<ILogger> new_logger);

    /**
     * @brief Replace Tracer.
     * @param[in] new_tracer Input parameter.
     */
    void replaceTracer(std::unique_ptr<ITracer> new_tracer);

    /**
     * @brief Replace Metrics.
     * @param[in] new_metrics Input parameter.
     */
    void replaceMetrics(std::unique_ptr<IMetrics> new_metrics);

    /**
     * @brief Replace Cache.
     * @param[in] new_cache Input parameter.
     */
    void replaceCache(std::unique_ptr<ICache> new_cache);

    /**
     * @brief Replace Secrets.
     * @param[in] new_secrets Input parameter.
     */
    void replaceSecrets(std::unique_ptr<ISecrets> new_secrets);

    /**
     * @brief Replace Feature Flags.
     * @param[in] new_ff Input parameter.
     */
    void replaceFeatureFlags(std::unique_ptr<IFeatureFlags> new_ff);

    /**
     * @brief Replace Audit Log.
     * @param[in] new_audit Input parameter.
     */
    void replaceAuditLog(std::unique_ptr<IAuditLog> new_audit);

    /**
     * @brief Start Span.
     * @param[in] name Input parameter.
     * @return Return value.
     * @details Implements startSpan without additional internal calls.
     */
    std::unique_ptr<ITracer::ISpan> startSpan(const std::string& name) {
        return tracer_->startSpan(name);
    }

    std::unique_ptr<ITracer::ISpan> startSpanFromHeaders(
            const std::string& name,
            const std::map<std::string, std::string>& headers) {
        return tracer_->startSpanFromHeaders(name, headers);
    }

    void injectContext(std::map<std::string, std::string>& headers) {
        tracer_->injectContext(headers);
    }

    /**
     * @brief Record Metric.
     * @param[in] name Input parameter.
     * @param[in] value Input parameter.
     * @details Calls: observeHistogram().
     */
    void recordMetric(const std::string& name, double value) {
        metrics_->observeHistogram(name, value);
    }

    void logWithTrace(ILogger::Level level,
                      const std::string& message,
                      const ILogger::Fields& fields = {});

    // -------------------------------------------------------------------------
    // Lifecycle hooks
    // -------------------------------------------------------------------------

    /**
     * @brief Flush.
     * @details Implements flush without additional internal calls.
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
     * @brief Shutdown.
     * @details Calls: flush().
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

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<ITracer> tracer_;
    std::shared_ptr<IMetrics> metrics_;
    std::shared_ptr<ICache> cache_;
    std::shared_ptr<ISecrets> secrets_;
    std::shared_ptr<ICircuitBreaker> circuit_breaker_;
    std::shared_ptr<IFeatureFlags> featureFlags_;
    std::shared_ptr<IAuditLog> auditLog_;
    std::unique_ptr<AdapterRegistry> registry_;
    mutable std::shared_mutex adapters_mutex_;
};

} // namespace concerns
} // namespace core
} // namespace themis
