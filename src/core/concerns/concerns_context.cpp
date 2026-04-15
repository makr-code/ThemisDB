/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concerns_context.cpp                               ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:40:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     357                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 090f93ef21  2026-03-22  feat(core): implement InMemorySecrets and EnvSecretsProvi... ║
    • e1c78c3604  2026-03-13  feat(core): implement RedisCache distributed cache adapte... ║
    • 50ae658f67  2026-03-09  feat(core): implement dynamic log level adjustment and au... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "core/concerns/concerns_context.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/otel_tracer_adapter.h"
#include "core/concerns/jaeger_tracer_adapter.h"
#include "core/concerns/zipkin_tracer_adapter.h"
#include "core/concerns/prometheus_metrics_adapter.h"
#include "core/concerns/inmemory_cache_impl.h"
#include "core/concerns/inmemory_secrets.h"
#include "core/concerns/redis_cache.h"
#include "core/concerns/noop_implementations.h"
#include "core/production_mode.h"
#include "core/config_validator.h"
#include "observability/metrics_collector.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace core {
namespace concerns {

std::shared_ptr<ConcernsContext> ConcernsContext::create() {
    return create(Config{});
}

std::shared_ptr<ConcernsContext> ConcernsContext::create(const Config& config) {
    bool production_mode = core::ProductionMode::isEnabled();
    
    // Validate configuration
    auto log_validation = core::ConfigValidator::validateLogConfig(config.logLevel, config.logPattern);
    if (!log_validation.valid) {
        throw std::runtime_error("Invalid logging configuration:\n" + log_validation.formatErrors());
    }
    
    auto trace_validation = core::ConfigValidator::validateTracingConfig(
        config.tracingEnabled, config.tracingEndpoint, config.tracingServiceName);
    if (!trace_validation.valid) {
        throw std::runtime_error("Invalid tracing configuration:\n" + trace_validation.formatErrors());
    }
    
    auto cache_validation = core::ConfigValidator::validateCacheConfig(
        config.cacheMaxSize, config.cacheDefaultTTL);
    if (!cache_validation.valid) {
        throw std::runtime_error("Invalid cache configuration:\n" + cache_validation.formatErrors());
    }

    auto adapter_validation = core::ConfigValidator::validateAdapterConfig(
        config.loggerAdapter, config.tracerAdapter,
        config.metricsAdapter, config.cacheAdapter,
        config.circuitBreakerAdapter, config.featureFlagsAdapter,
        config.auditAdapter, config.secretsAdapter);
    if (!adapter_validation.valid) {
        throw std::runtime_error("Invalid adapter configuration:\n" + adapter_validation.formatErrors());
    }
    
    // Initialize logger
    auto logLevel = ILogger::levelFromString(config.logLevel);
    utils::Logger::init(config.logFile, static_cast<utils::Logger::Level>(
        static_cast<int>(logLevel)));
    std::unique_ptr<ILogger> logger;
    if (config.loggerAdapter == "noop") {
        logger = std::make_unique<NoOpLogger>();
    } else {
        // "spdlog" — only reachable after validation passes
        auto spdlogger = std::make_unique<SpdlogLoggerAdapter>(nullptr, config.jsonLogging);
        if (!config.jsonLogging) {
            spdlogger->setPattern(config.logPattern);
        }
        logger = std::move(spdlogger);
    }

    // Resolve effective tracer adapter name:
    // explicit non-empty tracerAdapter overrides tracingEnabled.
    std::string effective_tracer = config.tracerAdapter;
    if (effective_tracer.empty()) {
        effective_tracer = config.tracingEnabled ? "otel" : "noop";
    }

    // Initialize tracer
    std::unique_ptr<ITracer> tracer;
    if (effective_tracer == "otel") {
        auto otelTracer = std::make_unique<OpenTelemetryTracerAdapter>();
        otelTracer->initialize(config.tracingServiceName, config.tracingEndpoint);
        tracer = std::move(otelTracer);
    } else if (effective_tracer == "jaeger") {
        auto jaegerTracer = std::make_unique<JaegerTracerAdapter>();
        jaegerTracer->initialize(config.tracingServiceName, config.tracingEndpoint);
        tracer = std::move(jaegerTracer);
    } else if (effective_tracer == "zipkin") {
        auto zipkinTracer = std::make_unique<ZipkinTracerAdapter>();
        zipkinTracer->initialize(config.tracingServiceName, config.tracingEndpoint);
        tracer = std::move(zipkinTracer);
    } else {
        // "noop" — only reachable after validation passes
        if (production_mode && effective_tracer != "otel" &&
            effective_tracer != "jaeger" && effective_tracer != "zipkin") {
            throw std::runtime_error(
                "Production mode violation: Tracing is disabled. "
                "Set tracingEnabled=true or tracerAdapter=\"otel\", \"jaeger\", or \"zipkin\" "
                "in ConcernsContext::Config for production deployments."
            );
        }
        tracer = std::make_unique<NoOpTracer>();
    }

    // Resolve effective metrics adapter name:
    // explicit non-empty metricsAdapter overrides metricsEnabled.
    std::string effective_metrics = config.metricsAdapter;
    if (effective_metrics.empty()) {
        effective_metrics = config.metricsEnabled ? "prometheus" : "noop";
    }

    // Initialize metrics
    std::unique_ptr<IMetrics> metrics;
    if (effective_metrics == "prometheus") {
        if (config.maxMetricCardinality > 0) {
            observability::MetricsCollector::getInstance().setCardinalityLimit(config.maxMetricCardinality);
        }
        metrics = std::make_unique<PrometheusMetricsAdapter>();
    } else {
        // "noop" — only reachable after validation passes
        if (production_mode && effective_metrics != "prometheus") {
            throw std::runtime_error(
                "Production mode violation: Metrics are disabled. "
                "Set metricsEnabled=true or metricsAdapter=\"prometheus\" in ConcernsContext::Config for production deployments."
            );
        }
        metrics = std::make_unique<NoOpMetrics>();
    }

    // Initialize cache
    std::unique_ptr<ICache> cache;
    if (config.cacheAdapter == "noop") {
        cache = std::make_unique<NoOpCache>();
    } else if (config.cacheAdapter == "redis" && !config.cacheRedisUrl.empty()) {
        cache = RedisCache::create(config.cacheRedisUrl);
    } else {
        // "inmemory" — only reachable after validation passes
        cache = std::make_unique<InMemoryCacheImpl>(
            config.cacheMaxSize,
            config.cacheDefaultTTL
        );
    }

    // Initialize circuit breaker
    std::unique_ptr<ICircuitBreaker> circuit_breaker;
    if (config.circuitBreakerAdapter == "noop") {
        circuit_breaker = std::make_unique<NoOpCircuitBreaker>();
    } else {
        // "default" — production circuit breaker
        ICircuitBreaker::Config cb_cfg;
        cb_cfg.failure_threshold  = config.circuitBreakerFailureThreshold;
        cb_cfg.timeout            = config.circuitBreakerTimeout;
        cb_cfg.success_threshold  = config.circuitBreakerSuccessThreshold;
        cb_cfg.failure_window     = config.circuitBreakerFailureWindow;
        circuit_breaker = std::make_unique<DefaultCircuitBreaker>(cb_cfg);
    }

    // Initialize feature flags
    std::unique_ptr<IFeatureFlags> featureFlags;
    if (config.featureFlagsAdapter == "noop") {
        featureFlags = std::make_unique<NoOpFeatureFlags>();
    } else {
        // "inmemory" — only reachable after validation passes
        featureFlags = std::make_unique<InMemoryFeatureFlags>(config.initialFeatureFlags);
    }

    // Initialize audit log
    std::unique_ptr<IAuditLog> auditLog;
    if (config.auditAdapter == "inmemory") {
        auditLog = std::make_unique<InMemoryAuditLog>();
    } else {
        // "noop" — only reachable after validation passes
        auditLog = std::make_unique<NoOpAuditLog>();
    }

    // Initialize secrets provider
    std::unique_ptr<ISecrets> secrets;
    if (config.secretsAdapter == "inmemory") {
        secrets = std::make_unique<InMemorySecrets>(config.initialSecrets);
    } else if (config.secretsAdapter == "env") {
        secrets = std::make_unique<EnvSecretsProvider>(config.secretsEnvPrefix);
    } else {
        // "noop" — default; only reachable after validation passes
        secrets = std::make_unique<NoOpSecrets>();
    }

    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache),
        std::move(circuit_breaker),
        std::move(secrets),
        std::move(featureFlags),
        std::move(auditLog)
    ));
}

std::shared_ptr<ConcernsContext> ConcernsContext::createCustom(
    std::unique_ptr<ILogger> logger,
    std::unique_ptr<ITracer> tracer,
    std::unique_ptr<IMetrics> metrics,
    std::unique_ptr<ICache> cache,
    std::unique_ptr<ICircuitBreaker> circuit_breaker
) {
    if (!circuit_breaker) {
        circuit_breaker = std::make_unique<NoOpCircuitBreaker>();
    }
    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache),
        std::move(circuit_breaker),
        std::make_unique<NoOpSecrets>(),
        std::make_unique<NoOpFeatureFlags>(),
        std::make_unique<NoOpAuditLog>()
    ));
}

std::shared_ptr<ConcernsContext> ConcernsContext::createCustom(
    std::unique_ptr<ILogger> logger,
    std::unique_ptr<ITracer> tracer,
    std::unique_ptr<IMetrics> metrics,
    std::unique_ptr<ICache> cache,
    std::unique_ptr<ISecrets> secrets,
    std::unique_ptr<IFeatureFlags> featureFlags
) {
    if (!secrets) {
        secrets = std::make_unique<NoOpSecrets>();
    }
    if (!featureFlags) {
        featureFlags = std::make_unique<NoOpFeatureFlags>();
    }
    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache),
        std::make_unique<NoOpCircuitBreaker>(),
        std::move(secrets),
        std::move(featureFlags),
        std::make_unique<NoOpAuditLog>()
    ));
}

std::shared_ptr<ConcernsContext> ConcernsContext::createCustom(
    std::unique_ptr<ILogger> logger,
    std::unique_ptr<ITracer> tracer,
    std::unique_ptr<IMetrics> metrics,
    std::unique_ptr<ICache> cache,
    std::unique_ptr<IFeatureFlags> featureFlags
) {
    if (!featureFlags) {
        featureFlags = std::make_unique<NoOpFeatureFlags>();
    }
    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache),
        std::make_unique<NoOpCircuitBreaker>(),
        std::make_unique<NoOpSecrets>(),
        std::move(featureFlags),
        std::make_unique<NoOpAuditLog>()
    ));
}

std::shared_ptr<ConcernsContext> ConcernsContext::createCustom(
    std::unique_ptr<ILogger> logger,
    std::unique_ptr<ITracer> tracer,
    std::unique_ptr<IMetrics> metrics,
    std::unique_ptr<ICache> cache,
    std::unique_ptr<ISecrets> secrets,
    std::unique_ptr<IFeatureFlags> featureFlags,
    std::unique_ptr<IAuditLog> auditLog
) {
    if (!secrets) {
        secrets = std::make_unique<NoOpSecrets>();
    }
    if (!featureFlags) {
        featureFlags = std::make_unique<NoOpFeatureFlags>();
    }
    if (!auditLog) {
        auditLog = std::make_unique<NoOpAuditLog>();
    }
    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache),
        std::make_unique<NoOpCircuitBreaker>(),
        std::move(secrets),
        std::move(featureFlags),
        std::move(auditLog)
    ));
}

std::shared_ptr<ConcernsContext> ConcernsContext::createNoOp() {
    bool production_mode = core::ProductionMode::isEnabled();
    
    if (production_mode) {
        throw std::runtime_error(
            "Production mode violation: Cannot create no-op ConcernsContext in production. "
            "Use create() or createCustom() with real implementations instead."
        );
    }
    
    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::make_unique<NoOpCircuitBreaker>(),
        std::make_unique<NoOpSecrets>(),
        std::make_unique<NoOpFeatureFlags>(),
        std::make_unique<NoOpAuditLog>()
    ));
}

void ConcernsContext::logWithTrace(ILogger::Level level,
                                    const std::string& message,
                                    const ILogger::Fields& fields) {
    TraceContext ctx;
    ctx.trace_id   = themis::Tracer::getCurrentTraceId();
    ctx.span_id    = themis::Tracer::getCurrentSpanId();
    logger_->logWithContext(level, message, ctx, fields);
}

} // namespace concerns
} // namespace core
} // namespace themis
