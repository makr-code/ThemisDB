#include "core/concerns/concerns_context.h"
#include "core/concerns/spdlog_logger_adapter.h"
#include "core/concerns/otel_tracer_adapter.h"
#include "core/concerns/prometheus_metrics_adapter.h"
#include "core/concerns/inmemory_cache_impl.h"
#include "core/concerns/noop_implementations.h"
#include "utils/logger.h"

namespace themis {
namespace core {
namespace concerns {

std::shared_ptr<ConcernsContext> ConcernsContext::create() {
    return create(Config{});
}

std::shared_ptr<ConcernsContext> ConcernsContext::create(const Config& config) {
    // Initialize logger
    auto logLevel = ILogger::levelFromString(config.logLevel);
    utils::Logger::init(config.logFile, static_cast<utils::Logger::Level>(
        static_cast<int>(logLevel)));
    auto logger = std::make_unique<SpdlogLoggerAdapter>();
    logger->setPattern(config.logPattern);

    // Initialize tracer
    std::unique_ptr<ITracer> tracer;
    if (config.tracingEnabled) {
        auto otelTracer = std::make_unique<OpenTelemetryTracerAdapter>();
        otelTracer->initialize(config.tracingServiceName, config.tracingEndpoint);
        tracer = std::move(otelTracer);
    } else {
        tracer = std::make_unique<NoOpTracer>();
    }

    // Initialize metrics
    std::unique_ptr<IMetrics> metrics;
    if (config.metricsEnabled) {
        metrics = std::make_unique<PrometheusMetricsAdapter>();
    } else {
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
