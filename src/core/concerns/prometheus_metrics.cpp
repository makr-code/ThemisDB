/*
 * ThemisDB | File: prometheus_metrics.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
