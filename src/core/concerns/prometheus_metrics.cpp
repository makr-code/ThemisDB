/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prometheus_metrics.cpp                             ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:16:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     47                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a190846a5c  2026-02-24  feat(core): implement Prometheus metrics adapter compilat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
