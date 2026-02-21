/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            concerns_context.cpp                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "core/concerns/concerns_context.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/otel_tracer_adapter.h"
#include "core/concerns/prometheus_metrics_adapter.h"
#include "core/concerns/inmemory_cache_impl.h"
#include "core/concerns/noop_implementations.h"
#include "core/production_mode.h"
#include "core/config_validator.h"
#include "observability/metrics_collector.h"
#include "utils/logger.h"

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
    
    // Initialize logger
    auto logLevel = ILogger::levelFromString(config.logLevel);
    utils::Logger::init(config.logFile, static_cast<utils::Logger::Level>(
        static_cast<int>(logLevel)));
    auto logger = std::make_unique<SpdlogLoggerAdapter>(nullptr, config.jsonLogging);
    if (!config.jsonLogging) {
        logger->setPattern(config.logPattern);
    }

    // Initialize tracer
    std::unique_ptr<ITracer> tracer;
    if (config.tracingEnabled) {
        auto otelTracer = std::make_unique<OpenTelemetryTracerAdapter>();
        otelTracer->initialize(config.tracingServiceName, config.tracingEndpoint);
        tracer = std::move(otelTracer);
    } else {
        if (production_mode) {
            throw std::runtime_error(
                "Production mode violation: Tracing is disabled. "
                "Set tracingEnabled=true in ConcernsContext::Config for production deployments."
            );
        }
        tracer = std::make_unique<NoOpTracer>();
    }

    // Initialize metrics
    std::unique_ptr<IMetrics> metrics;
    if (config.metricsEnabled) {
        if (config.maxMetricCardinality > 0) {
            observability::MetricsCollector::getInstance().setCardinalityLimit(config.maxMetricCardinality);
        }
        metrics = std::make_unique<PrometheusMetricsAdapter>();
    } else {
        if (production_mode) {
            throw std::runtime_error(
                "Production mode violation: Metrics are disabled. "
                "Set metricsEnabled=true in ConcernsContext::Config for production deployments."
            );
        }
        metrics = std::make_unique<NoOpMetrics>();
    }

    // Initialize cache
    auto cache = std::make_unique<InMemoryCacheImpl>(
        config.cacheMaxSize,
        config.cacheDefaultTTL
    );

    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache)
    ));
}

std::shared_ptr<ConcernsContext> ConcernsContext::createCustom(
    std::unique_ptr<ILogger> logger,
    std::unique_ptr<ITracer> tracer,
    std::unique_ptr<IMetrics> metrics,
    std::unique_ptr<ICache> cache
) {
    return std::shared_ptr<ConcernsContext>(new ConcernsContext(
        std::move(logger),
        std::move(tracer),
        std::move(metrics),
        std::move(cache)
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
        std::make_unique<NoOpCache>()
    ));
}

} // namespace concerns
} // namespace core
} // namespace themis
