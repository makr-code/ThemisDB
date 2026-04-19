// STUB/SIMULATION NOTE:
// Purpose: Provides no-op implementations of grafana_metrics symbols needed
//          by focused test targets that compile continuous_batch_scheduler.cpp
//          without the full httplib.h HTTP server dependency.
// Activation: Compiled only for *_focused test executables that list this stub
//             as a source file instead of the real grafana_metrics.cpp.
// Production Delta: All methods are no-ops; no metrics are emitted.
// Removal Plan: Remove when the focused test is merged into themis_core builds
//               or when the real grafana_metrics.cpp no longer requires httplib.h.

#include "llm/grafana_metrics.h"
#include <map>
#include <string>
#include <spdlog/spdlog.h>

namespace themis::llm::monitoring {

// ── PrometheusExporter stubs ─────────────────────────────────────────────────

PrometheusExporter::PrometheusExporter() = default;
PrometheusExporter::~PrometheusExporter() = default;

void PrometheusExporter::registerMetric(const MetricDefinition&) {}
void PrometheusExporter::incrementCounter(
    const std::string&, double,
    const std::map<std::string, std::string>&) {}
void PrometheusExporter::setGauge(
    const std::string&, double,
    const std::map<std::string, std::string>&) {}
void PrometheusExporter::incrementGauge(
    const std::string&, double,
    const std::map<std::string, std::string>&) {}
void PrometheusExporter::observeHistogram(
    const std::string&, double,
    const std::map<std::string, std::string>&) {}
std::string PrometheusExporter::exportMetrics() const { return {}; }
std::string PrometheusExporter::handleMetricsRequest() const { return {}; }
void PrometheusExporter::reset() {}
std::string PrometheusExporter::makeMetricKey(
    const std::string& name,
    const std::map<std::string, std::string>&) const { return name; }
std::string PrometheusExporter::serializeMetric(
    const std::string&, const MetricValue&) const { return {}; }

// ── LLMMetricsCollector stubs ────────────────────────────────────────────────

LLMMetricsCollector::LLMMetricsCollector(PrometheusExporter* exporter)
    : exporter_(exporter) {}

LLMMetricsCollector::LLMMetricsCollector(PrometheusExporter* exporter,
                                         const Config& config)
    : exporter_(exporter), config_(config) {}

void LLMMetricsCollector::initializeMetrics() {}
void LLMMetricsCollector::recordRequest(const std::string&) {}
void LLMMetricsCollector::recordTokensGenerated(size_t, const std::string&) {}
void LLMMetricsCollector::recordCacheHit() {}
void LLMMetricsCollector::recordCacheMiss() {}
void LLMMetricsCollector::recordInferenceLatency(double, const std::string&) {}
void LLMMetricsCollector::recordTTFT(double) {}
void LLMMetricsCollector::recordQueueLength(size_t) {}
void LLMMetricsCollector::recordBackpressureDrop() {}
std::string LLMMetricsCollector::exportMetrics() const { return {}; }

} // namespace themis::llm::monitoring
