/**
 * @file otel_tracer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
