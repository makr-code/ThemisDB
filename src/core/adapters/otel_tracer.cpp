/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            otel_tracer.cpp                                    ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:16:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     37                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 522e9ae575  2026-02-24  feat(core): implement OTel tracer adapter flush() via Tra... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "core/concerns/otel_tracer_adapter.h"
#include "utils/tracing.h"

namespace themis {
namespace core {
namespace concerns {

void OpenTelemetryTracerAdapter::flush() noexcept {
    themis::Tracer::flush();
}

} // namespace concerns
} // namespace core
} // namespace themis
