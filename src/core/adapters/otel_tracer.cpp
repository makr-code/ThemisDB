/*
 * ThemisDB | File: otel_tracer.cpp | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 22
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2844 feat(core): add Prometheus ... (2026-03-12) | #2843 feat(core): implement OpenT... (2026-03-12) | #2841 [WIP] Add plugin-based adap... (2026-03-12) | #2840 fix(core): resolve ILogger:... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
