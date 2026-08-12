/**
 * @file prometheus_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Compilation unit for PrometheusMetricsAdapter.
 *
 * The adapter bridges the generic IMetrics interface to the ThemisDB
 * MetricsCollector singleton, which exposes metrics in Prometheus
 * text-exposition format suitable for scraping by a Prometheus server.
 *
 * All method definitions live in the header
 * (include/core/concerns/prometheus_metrics_adapter.h) because they are
 * thin, inline forwarding wrappers around MetricsCollector.  This
 * translation unit exists to:
 *   1. Give the adapter a dedicated object file in the build graph,
 *      ensuring that linkers that perform whole-archive elimination still
 *      retain the adapter symbols.
 *   2. Provide an explicit boundary between the adapter interface and the
 *      MetricsCollector backend so that future non-inline logic (e.g.
 *      metric name validation, push-based export) can be added here
 *      without changing the header.
 */

#include "core/concerns/prometheus_metrics_adapter.h"

// PrometheusMetricsAdapter is fully defined inline in the header.
// All virtual-table symbols are emitted from this translation unit.
