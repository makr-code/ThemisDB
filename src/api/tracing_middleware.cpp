/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tracing_middleware.cpp                             ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-14 11:31:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     142                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8ddf5b3504  2026-03-10  fix(api): address code review - clean up test, remove sta... ║
    • e24bce921d  2026-03-10  feat(api): add OTLP/HTTP span exporter + wire into Tracin... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 2672276c02  2026-02-28  feat(api): add TracingMiddleware for X-Correlation-ID pro... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "api/tracing_middleware.h"
#include "api/otlp_exporter.h"
#include "utils/logger.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>

namespace themis {
namespace api {

namespace {
/// Per-thread correlation ID context.
thread_local std::string tl_correlation_id;
/// Per-thread span start time (nanoseconds since epoch).
thread_local int64_t tl_span_start_ns = 0;
} // anonymous namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TracingMiddleware::TracingMiddleware(OtlpExporter* exporter)
    : exporter_(exporter)
{}

// ---------------------------------------------------------------------------
// processRequest
// ---------------------------------------------------------------------------

std::string TracingMiddleware::processRequest(std::string_view incoming_id) const {
    std::string corr_id;
    if (!incoming_id.empty()) {
        corr_id = std::string(incoming_id);
    } else {
        corr_id = generateUuidV4();
    }

    // Store in thread-local for later response injection
    tl_correlation_id = corr_id;

    // Record span start time (used by finishSpan())
    if (exporter_) {
        const auto now = std::chrono::system_clock::now().time_since_epoch();
        tl_span_start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    }

    // Inject into the logger so every log line on this thread carries the ID
    utils::Logger::setTraceContext(corr_id);

    return corr_id;
}

// ---------------------------------------------------------------------------
// finishSpan
// ---------------------------------------------------------------------------

void TracingMiddleware::finishSpan(std::string_view span_name, int http_status) const
{
    if (!exporter_) return;
    if (tl_correlation_id.empty()) return;
    if (tl_span_start_ns == 0) return;  // processRequest() was not called with this exporter

    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const int64_t end_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    SpanData span;
    span.trace_id          = tl_correlation_id;
    span.name              = std::string(span_name);
    span.start_time_unix_nano = tl_span_start_ns > 0 ? tl_span_start_ns : end_ns;
    span.end_time_unix_nano   = end_ns;

    if (http_status >= 500) {
        span.status_code    = 2; // Error
        span.status_message = "HTTP " + std::to_string(http_status);
    } else if (http_status > 0) {
        span.status_code    = 1; // OK
    }

    if (http_status > 0) {
        span.attributes["http.status_code"] = std::to_string(http_status);
    }

    exporter_->enqueue(std::move(span));
}

// ---------------------------------------------------------------------------
// Thread-local accessors
// ---------------------------------------------------------------------------

/*static*/
const std::string& TracingMiddleware::currentCorrelationId() noexcept {
    return tl_correlation_id;
}

/*static*/
void TracingMiddleware::clearContext() noexcept {
    tl_correlation_id.clear();
    tl_span_start_ns = 0;
    utils::Logger::setTraceContext("");
}

// ---------------------------------------------------------------------------
// UUID v4 generation
// ---------------------------------------------------------------------------

/*static*/
std::string TracingMiddleware::generateUuidV4() {
    // thread_local avoids synchronisation overhead; random_generator is not
    // thread-safe when shared, so we keep one generator per thread.
    thread_local boost::uuids::random_generator gen;
    return boost::uuids::to_string(gen());
}

} // namespace api
} // namespace themis
