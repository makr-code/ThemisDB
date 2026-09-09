/**
 * @file continuous_query_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 2.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/continuous_query_api_handler.h"
#include "query/continuous_query_engine.h"
#include "query/continuous_query_registry.h"
#include "query/window_spec.h"
#include "utils/logger.h"
#include "utils/input_validator.h"

#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <chrono>

namespace themis {
namespace server {

using json = nlohmann::json;
using themis::query::ContinuousQueryEngine;
using themis::query::ContinuousQuerySpec;
using themis::query::ContinuousQueryInfo;
using themis::query::ResultMode;
using themis::query::WindowSpec;

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ContinuousQueryApiHandler::ContinuousQueryApiHandler(
    std::shared_ptr<ContinuousQueryEngine> engine)
    : engine_(std::move(engine))
{
    if (!engine_) {
        throw std::invalid_argument(
            "ContinuousQueryApiHandler: engine must not be null");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ContinuousQueryApiHandler::makeError(
    http::status status, const std::string& message,
    const http::request<http::string_body>& req)
{
    json body{{"error", message}};
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> ContinuousQueryApiHandler::makeJson(
    http::status status, const std::string& body,
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

WindowSpec ContinuousQueryApiHandler::windowFromJson(const json& j) {
    WindowSpec ws;
    const std::string type_str = j.value("type", "TIME_SLIDING");
    if (type_str == "COUNT_SLIDING") {
        ws.type       = WindowSpec::Type::COUNT_SLIDING;
        ws.rows       = j.value("rows",       static_cast<int64_t>(1000));
        ws.slide_rows = j.value("slide_rows", static_cast<int64_t>(100));
        ws.partition_by = j.value("partition_by", "");
    } else if (type_str == "TUMBLING") {
        ws.type     = WindowSpec::Type::TUMBLING;
        ws.range_ms = j.value("range_ms", static_cast<int64_t>(60'000));
    } else {
        // Default: TIME_SLIDING
        ws.type     = WindowSpec::Type::TIME_SLIDING;
        ws.range_ms = j.value("range_ms", static_cast<int64_t>(60'000));
        ws.slide_ms = j.value("slide_ms", static_cast<int64_t>(1'000));
    }
    return ws;
}

json ContinuousQueryApiHandler::infoToJson(const ContinuousQueryInfo& info) {
    auto tp_to_ms = [](std::chrono::system_clock::time_point tp) -> int64_t {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   tp.time_since_epoch()).count();
    };

    auto mode_str = [](ResultMode m) -> std::string {
        switch (m) {
            case ResultMode::DELTA:    return "DELTA";
            case ResultMode::SNAPSHOT: return "SNAPSHOT";
            case ResultMode::CHANGES:  return "CHANGES";
        }
        return "DELTA";
    };

    auto win_type = [](WindowSpec::Type t) -> std::string {
        switch (t) {
            case WindowSpec::Type::TIME_SLIDING:  return "TIME_SLIDING";
            case WindowSpec::Type::COUNT_SLIDING: return "COUNT_SLIDING";
            case WindowSpec::Type::TUMBLING:      return "TUMBLING";
        }
        return "TIME_SLIDING";
    };

    json w{
        {"type",         win_type(info.window.type)},
        {"range_ms",     info.window.range_ms},
        {"slide_ms",     info.window.slide_ms},
        {"rows",         info.window.rows},
        {"slide_rows",   info.window.slide_rows},
        {"partition_by", info.window.partition_by}
    };

    return json{
        {"name",               info.name},
        {"source_collection",  info.source_collection},
        {"window",             w},
        {"result_mode",        mode_str(info.result_mode)},
        {"registered_at_ms",   tp_to_ms(info.registered_at)},
        {"last_tick_at_ms",    tp_to_ms(info.last_tick_at)},
        {"tuples_processed",   info.tuples_processed},
        {"result_queue_depth", info.result_queue_depth}
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /v1/queries/continuous  — register
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ContinuousQueryApiHandler::handleRegister(
    const http::request<http::string_body>& req)
{
    json body;
    try {
        body = json::parse(req.body());
    } catch (const json::exception& ex) {
        return makeError(http::status::bad_request,
            std::string("Invalid JSON body: ") + ex.what(), req);
    }

    // Required fields
    if (!body.contains("name") || !body["name"].is_string() ||
        body["name"].get<std::string>().empty())
    {
        return makeError(http::status::bad_request,
            "Field 'name' is required and must be a non-empty string", req);
    }
    if (!body.contains("source_collection") ||
        !body["source_collection"].is_string() ||
        body["source_collection"].get<std::string>().empty())
    {
        return makeError(http::status::bad_request,
            "Field 'source_collection' is required and must be a non-empty string", req);
    }

    ContinuousQuerySpec spec;
    spec.name               = body["name"].get<std::string>();
    spec.source_collection  = body["source_collection"].get<std::string>();
    
    // QW-46 Guard: Fail-closed collection name validation
    {
        utils::InputValidator validator;
        if (!validator.validateStringLength(spec.source_collection, 256) || 
            !validator.validatePathSegment(spec.source_collection)) {
            THEMIS_ERROR("QW-46 Guard: Invalid source_collection in handleRegister");
            return makeError(http::status::bad_request,
                "Invalid source_collection: only alphanumeric, underscore, and hyphen allowed; max 256 characters", req);
        }
    }
    
    spec.aql_body           = body.value("aql_body", "");

    if (body.contains("window") && body["window"].is_object()) {
        spec.window = windowFromJson(body["window"]);
    }

    if (body.contains("result_mode") && body["result_mode"].is_string()) {
        const std::string rm = body["result_mode"].get<std::string>();
        if (rm == "SNAPSHOT") {
          spec.result_mode = ResultMode::SNAPSHOT;
        }
        else if (rm == "CHANGES") spec.result_mode = ResultMode::CHANGES;
        else                       spec.result_mode = ResultMode::DELTA;
    }

    spec.allowed_lateness_ms = body.value("allowed_lateness_ms", static_cast<int64_t>(500));

    auto result = engine_->registerQuery(std::move(spec));
    if (!result) {
        const auto& err = result.error();
        THEMIS_WARN("ContinuousQueryApiHandler::handleRegister failed: {}",
                    err.message());
        // 409 for duplicate name, 400 for validation errors
        return makeError(http::status::bad_request, err.message(), req);
    }

    json resp_body{{"name", *result}};
    return makeJson(http::status::created, resp_body.dump(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// DELETE /v1/queries/continuous/:name  — drop
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ContinuousQueryApiHandler::handleDrop(
    const http::request<http::string_body>& req,
    const std::string& name)
{
    if (name.empty()) {
        return makeError(http::status::bad_request, "Query name must not be empty", req);
    }

    auto result = engine_->dropQuery(name);
    if (!result) {
        const auto& err = result.error();
        THEMIS_WARN("ContinuousQueryApiHandler::handleDrop('{}'): {}",
                    name, err.message());
        return makeError(http::status::not_found, err.message(), req);
    }

    json resp_body{{"dropped", name}};
    return makeJson(http::status::ok, resp_body.dump(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /v1/queries/continuous  — list
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ContinuousQueryApiHandler::handleList(
    const http::request<http::string_body>& req)
{
    const auto queries = engine_->listQueries();

    json arr = json::array();
    for (const auto& info : queries) {
        arr.push_back(infoToJson(info));
    }

    return makeJson(http::status::ok, arr.dump(), req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /v1/queries/continuous/:name/results  — SSE stream
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> ContinuousQueryApiHandler::handleStreamSse(
    const http::request<http::string_body>& req,
    const std::string& name)
{
    if (name.empty()) {
        return makeError(http::status::bad_request, "Query name must not be empty", req);
    }

    // Subscribe in DELTA mode (the default for SSE consumers)
    auto sub_result = engine_->subscribe(name, ResultMode::DELTA);
    if (!sub_result) {
        const auto& err = sub_result.error();
        THEMIS_WARN("ContinuousQueryApiHandler::handleStreamSse('{}'): {}",
                    name, err.message());
        return makeError(http::status::not_found, err.message(), req);
    }

    auto stream = std::move(*sub_result);

    // Build the full SSE body synchronously.
    // We poll the stream for up to a bounded number of events (or until the
    // query is dropped).  Each poll waits at most 60 s for a result; on
    // timeout a heartbeat comment is emitted to keep the connection alive.
    //
    // The response body is accumulated in memory.  For production deployments
    // with long-lived streams an async chunked-transfer implementation is
    // preferred; this synchronous version is appropriate for the current
    // Boost.Beast HTTP/1.1 session model and for the integration tests.
    constexpr size_t  kMaxEvents       = 1024;   // cap per request to avoid OOM
    constexpr int     kPollTimeoutSec  = 60;
    constexpr size_t  kHeartbeatEvery  = 1;      // emit heartbeat every N timeouts

    std::ostringstream sse_body = {};
    // SSE preamble: retry hint
    sse_body << "retry: 3000\n\n";

    size_t events_sent     = 0;
    size_t timeout_count   = 0;

    while (stream->hasMore() && events_sent < kMaxEvents) {
        auto item = stream->next(std::chrono::seconds(kPollTimeoutSec));
        if (!item) {
            // Poll timeout — emit heartbeat comment
            ++timeout_count;
            if (timeout_count % kHeartbeatEvery == 0) {
                sse_body << ": heartbeat\n\n";
            }
            continue;
        }

        json event_json{
            {"payload",    item->payload},
            {"is_retract", item->is_retract}
        };
        sse_body << "data: " << event_json.dump() << "\n\n";
        ++events_sent;
    }

    stream->cancel();

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/event-stream");
    res.set(http::field::cache_control, "no-cache");
    res.set("X-Accel-Buffering", "no");
    res.keep_alive(false);  // close after stream ends
    res.body() = sse_body.str();
    res.prepare_payload();
    return res;
}

}  // namespace server
}  // namespace themis
