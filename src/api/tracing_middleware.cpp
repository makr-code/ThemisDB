/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tracing_middleware.cpp                             ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:56:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     65                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2672276c0  2026-02-28  feat(api): add TracingMiddleware for X-Correlation-ID pro... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "api/tracing_middleware.h"
#include "utils/logger.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace themis {
namespace api {

namespace {
/// Per-thread correlation ID context.
thread_local std::string tl_correlation_id;
} // anonymous namespace

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

    // Inject into the logger so every log line on this thread carries the ID
    utils::Logger::setTraceContext(corr_id);

    return corr_id;
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
