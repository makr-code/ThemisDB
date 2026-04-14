/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changefeed_api_handler.cpp                         ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:05:36                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     1250                                           ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 13a305368a  2026-04-13  feat(cdc): GDPR redaction audit log (cdc_redactions CF) +... ║
    • 5a69555883  2026-03-11  chore(cdc): audit fixes - documentation, THEMIS_ENABLE_SS... ║
    • b56122b397  2026-03-11  feat(cdc): extend at-least-once delivery guarantee to SSE... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 1c85680750  2026-02-24  Implement GDPR-aware PII field scrubbing HTTP endpoint fo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "server/changefeed_api_handler.h"
#include "server/tenant_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include "cdc/cdc_admin.h"
#ifdef THEMIS_ENABLE_SSE
#include "server/sse_connection_manager.h"
#endif
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <sstream>
#include <thread>
#include <chrono>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

// Helper: parse a comma-separated list of event type names into a set
// Accepted values: "PUT", "DELETE", "TRANSACTION_COMMIT", "TRANSACTION_ROLLBACK"
// Maximum length for the event_types query parameter value to prevent DoS via oversized input
static constexpr size_t EVENT_TYPES_MAX_LEN = 256;
// Length of the "event_types=" query parameter prefix
static constexpr size_t EVENT_TYPES_PARAM_LEN = sizeof("event_types=") - 1;

static std::set<Changefeed::ChangeEventType> parseEventTypes(const std::string& types_str) {
    std::set<Changefeed::ChangeEventType> result;
    if (types_str.size() > EVENT_TYPES_MAX_LEN) {
        THEMIS_WARN("parseEventTypes: input too long ({} bytes, max {} allowed), ignoring",
                    types_str.size(), EVENT_TYPES_MAX_LEN);
        return result;
    }
    std::istringstream ss(types_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim leading and trailing whitespace
        auto start = token.find_first_not_of(" \t");
        auto end = token.find_last_not_of(" \t");
        if (start == std::string::npos) {
            continue;
        }
        token = token.substr(start, end - start + 1);

        if (token == "PUT") {
            result.insert(Changefeed::ChangeEventType::EVENT_PUT);
        } else if (token == "DELETE") {
            result.insert(Changefeed::ChangeEventType::EVENT_DELETE);
        } else if (token == "TRANSACTION_COMMIT") {
            result.insert(Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT);
        } else if (token == "TRANSACTION_ROLLBACK") {
            result.insert(Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK);
        } else {
            THEMIS_WARN("parseEventTypes: unrecognized event type '{}'; ignoring", token);
        }
    }
    return result;
}

ChangefeedApiHandler::ChangefeedApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<Changefeed> changefeed,
    std::shared_ptr<SseConnectionManager> sse_manager,
    std::shared_ptr<AuthMiddleware> auth,
    bool feature_cdc
)
    : storage_(std::move(storage))
    , changefeed_(std::move(changefeed))
    , sse_manager_(std::move(sse_manager))
    , auth_(std::move(auth))
    , feature_cdc_(feature_cdc)
{
    // Start the at-least-once delivery tracker background thread.
    // The tracker accumulates per-consumer in-flight state across SSE requests.
    delivery_tracker_.start();
}

http::response<http::string_body> ChangefeedApiHandler::handleGet(
    const http::request<http::string_body>& req
) {
    // Authorization check
    if (auto auth_resp = checkAuth(req, "cdc:read")) {
        return *auth_resp;
    }
    
    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }
    
    auto span = Tracer::startSpan("handleChangefeedGet");
    span.setAttribute("http.path", "/changefeed");
    
    try {
        // Parse query parameters
        Changefeed::ListOptions options;
        
        std::string target = std::string(req.target());
        size_t query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_str = target.substr(query_pos + 1);
            
            // Parse from_seq
            size_t from_pos = query_str.find("from_seq=");
            if (from_pos != std::string::npos) {
                size_t from_end = query_str.find('&', from_pos);
                std::string from_str = query_str.substr(from_pos + 9,
                    from_end == std::string::npos ? std::string::npos : from_end - from_pos - 9);
                options.from_sequence = std::stoull(from_str);
            }
            
            // Parse limit
            size_t limit_pos = query_str.find("limit=");
            if (limit_pos != std::string::npos) {
                size_t limit_end = query_str.find('&', limit_pos);
                std::string limit_str = query_str.substr(limit_pos + 6,
                    limit_end == std::string::npos ? std::string::npos : limit_end - limit_pos - 6);
                options.limit = std::stoull(limit_str);
            }
            
            // Parse long_poll_ms
            size_t poll_pos = query_str.find("long_poll_ms=");
            if (poll_pos != std::string::npos) {
                size_t poll_end = query_str.find('&', poll_pos);
                std::string poll_str = query_str.substr(poll_pos + 13,
                    poll_end == std::string::npos ? std::string::npos : poll_end - poll_pos - 13);
                options.long_poll_ms = std::stoul(poll_str);
            }
            
            // Parse key_prefix
            size_t key_pos = query_str.find("key_prefix=");
            if (key_pos != std::string::npos) {
                size_t key_end = query_str.find('&', key_pos);
                std::string key_prefix = query_str.substr(key_pos + 11,
                    key_end == std::string::npos ? std::string::npos : key_end - key_pos - 11);
                options.key_prefix = key_prefix;
            }
            
            // Parse event_types (comma-separated: PUT,DELETE,TRANSACTION_COMMIT,TRANSACTION_ROLLBACK)
            size_t et_pos = query_str.find("event_types=");
            if (et_pos != std::string::npos) {
                size_t et_end = query_str.find('&', et_pos);
                std::string et_str = query_str.substr(et_pos + EVENT_TYPES_PARAM_LEN,
                    et_end == std::string::npos ? std::string::npos : et_end - et_pos - EVENT_TYPES_PARAM_LEN);
                options.event_types = parseEventTypes(et_str);
            }
        }
        
        // List events
        auto events = changefeed_->listEvents(options);
        
        // Build response
        json response;
        response["events"] = json::array();
        for (const auto& event : events) {
            response["events"].push_back(event.toJson());
        }
        response["count"] = events.size();
        response["latest_sequence"] = changefeed_->getLatestSequence();
        
        span.setAttribute("events.count", static_cast<int64_t>(events.size()));
        span.setAttribute("events.from_seq", static_cast<int64_t>(options.from_sequence));
        span.setStatus(true);
        
        return makeResponse(http::status::ok, response.dump(), req);
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleStreamSse(
    const http::request<http::string_body>& req
) {
    // Authorization check
    if (auto auth_resp = checkAuth(req, "cdc:read")) {
        return *auth_resp;
    }
    
    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }
    
    auto span = Tracer::startSpan("handleChangefeedStreamSse");
    span.setAttribute("http.path", "/changefeed/stream");
    
    try {
        // Parse query parameters
        uint64_t from_seq = 0;
        std::string key_prefix;
        std::set<Changefeed::ChangeEventType> event_types;
        bool keep_alive = true; // New parameter for production streaming
        int max_seconds = 30;   // Optional limit for testability
        int heartbeat_ms_override = -1; // Optional per-request heartbeat interval
        int retry_ms = 3000;
        size_t max_events_per_poll = 100; // Backpressure: limit events consumed per poll
        // At-least-once delivery: optional consumer identifier and ack timeout override.
        // When consumer_id is set, unacknowledged events from previous requests are
        // redelivered before new events; clients ACK via POST /changefeed/stream/ack.
        std::string consumer_id;
        std::optional<std::chrono::milliseconds> ack_timeout_override;
        
        std::string target = std::string(req.target());
        size_t query_pos = target.find('?');
        if (query_pos != std::string::npos) {
            std::string query_str = target.substr(query_pos + 1);
            
            // Parse from_seq
            size_t from_pos = query_str.find("from_seq=");
            if (from_pos != std::string::npos) {
                size_t from_end = query_str.find('&', from_pos);
                std::string from_str = query_str.substr(from_pos + 9,
                    from_end == std::string::npos ? std::string::npos : from_end - from_pos - 9);
                from_seq = std::stoull(from_str);
            }
            
            // Parse key_prefix
            size_t key_pos = query_str.find("key_prefix=");
            if (key_pos != std::string::npos) {
                size_t key_end = query_str.find('&', key_pos);
                key_prefix = query_str.substr(key_pos + 11,
                    key_end == std::string::npos ? std::string::npos : key_end - key_pos - 11);
            }
            
            // Parse event_types (comma-separated: PUT,DELETE,TRANSACTION_COMMIT,TRANSACTION_ROLLBACK)
            size_t et_pos = query_str.find("event_types=");
            if (et_pos != std::string::npos) {
                size_t et_end = query_str.find('&', et_pos);
                std::string et_str = query_str.substr(et_pos + EVENT_TYPES_PARAM_LEN,
                    et_end == std::string::npos ? std::string::npos : et_end - et_pos - EVENT_TYPES_PARAM_LEN);
                event_types = parseEventTypes(et_str);
            }
            
            // Parse keep_alive (default true for production)
            size_t ka_pos = query_str.find("keep_alive=");
            if (ka_pos != std::string::npos) {
                size_t ka_end = query_str.find('&', ka_pos);
                std::string ka_str = query_str.substr(ka_pos + 11,
                    ka_end == std::string::npos ? std::string::npos : ka_end - ka_pos - 11);
                keep_alive = (ka_str == "true" || ka_str == "1");
            }

            // Parse max_seconds (bounds 1..60)
            size_t ms_pos = query_str.find("max_seconds=");
            if (ms_pos != std::string::npos) {
                size_t ms_end = query_str.find('&', ms_pos);
                std::string ms_str = query_str.substr(ms_pos + 12,
                    ms_end == std::string::npos ? std::string::npos : ms_end - ms_pos - 12);
                try {
                    int v = std::stoi(ms_str);
                    if (v < 1) {
                        v = 1;
                    }
                    if (v > 60) {
                        v = 60;
                    }
                    max_seconds = v;
                } catch (...) {
                    THEMIS_DEBUG("changefeed: ignoring invalid max_seconds query param");
                }
            }

            // Parse heartbeat_ms (test override)
            size_t hb_pos = query_str.find("heartbeat_ms=");
            if (hb_pos != std::string::npos) {
                size_t hb_end = query_str.find('&', hb_pos);
                std::string hb_str = query_str.substr(hb_pos + 13,
                    hb_end == std::string::npos ? std::string::npos : hb_end - hb_pos - 13);
                try {
                    int v = std::stoi(hb_str);
                    if (v < 100) v = 100; // minimum 100ms
                    if (v > 60000) v = 60000;
                    heartbeat_ms_override = v;
                } catch (...) {
                    THEMIS_DEBUG("changefeed: ignoring invalid heartbeat_ms query param");
                }
            }

            // Parse retry_ms
            size_t r_pos = query_str.find("retry_ms=");
            if (r_pos != std::string::npos) {
                size_t r_end = query_str.find('&', r_pos);
                std::string r_str = query_str.substr(r_pos + 9,
                    r_end == std::string::npos ? std::string::npos : r_end - r_pos - 9);
                try {
                    int v = std::stoi(r_str);
                    if (v < 100) {
                        v = 100;
                    }
                    if (v > 120000) {
                        v = 120000;
                    }
                    retry_ms = v;
                } catch (...) {
                    THEMIS_DEBUG("changefeed: ignoring invalid retry_ms query param");
                }
            }

            // Parse max_events_per_poll
            size_t me_pos = query_str.find("max_events=");
            if (me_pos != std::string::npos) {
                size_t me_end = query_str.find('&', me_pos);
                std::string me_str = query_str.substr(me_pos + 11,
                    me_end == std::string::npos ? std::string::npos : me_end - me_pos - 11);
                try {
                    int v = std::stoi(me_str);
                    if (v < 1) {
                        v = 1;
                    }
                    if (v > 1000) {
                        v = 1000;
                    }
                    max_events_per_poll = static_cast<size_t>(v);
                } catch (...) {
                    THEMIS_DEBUG("changefeed: ignoring invalid max_events query param");
                }
            }

            // Parse consumer_id for at-least-once delivery tracking.
            // Maximum length for consumer_id to prevent DoS via oversized input.
            static constexpr size_t CONSUMER_ID_MAX_LEN = 128;
            size_t cid_pos = query_str.find("consumer_id=");
            if (cid_pos != std::string::npos) {
                size_t cid_end = query_str.find('&', cid_pos);
                std::string cid_str = query_str.substr(cid_pos + 12,
                    cid_end == std::string::npos ? std::string::npos : cid_end - cid_pos - 12);
                if (!cid_str.empty() && cid_str.size() <= CONSUMER_ID_MAX_LEN) {
                    consumer_id = std::move(cid_str);
                } else if (cid_str.size() > CONSUMER_ID_MAX_LEN) {
                    THEMIS_WARN("changefeed: consumer_id exceeds max length ({}), ignoring", CONSUMER_ID_MAX_LEN);
                }
            }

            // Parse ack_timeout_ms: per-request override for the at-least-once
            // redelivery timeout (useful for testing with short timeouts).
            size_t at_pos = query_str.find("ack_timeout_ms=");
            if (at_pos != std::string::npos) {
                size_t at_end = query_str.find('&', at_pos);
                std::string at_str = query_str.substr(at_pos + 15,
                    at_end == std::string::npos ? std::string::npos : at_end - at_pos - 15);
                try {
                    int v = std::stoi(at_str);
                    if (v >= 0) {
                        ack_timeout_override = std::chrono::milliseconds(v);
                    }
                } catch (...) {
                    THEMIS_DEBUG("changefeed: ignoring invalid ack_timeout_ms query param");
                }
            }
        }

        // Support Last-Event-ID header for resume
        // Search case-insensitively
        for (const auto& h : req) {
            auto name = h.name_string();
            if (beast::iequals(name, "Last-Event-ID")) {
                try {
                    uint64_t last_id = std::stoull(std::string(h.value()));
                    if (from_seq == 0) from_seq = last_id;
                } catch (...) {
                    THEMIS_DEBUG("changefeed: ignoring invalid Last-Event-ID header value");
                    break;
                }
            }
        }
        
        // Build SSE response
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "THEMIS/0.1.0");
        res.set(http::field::content_type, "text/event-stream");
        res.set(http::field::cache_control, "no-cache, no-transform");
        res.set(http::field::connection, "keep-alive");
        // Best-effort proxies
        res.set(http::field::access_control_allow_origin, "*");
        res.keep_alive(true);
        
        std::ostringstream body;
        // Advise client reconnect delay
        body << "retry: " << retry_ms << "\n\n";
        
        // Production streaming path via SSE manager (only when enabled)
#ifdef THEMIS_ENABLE_SSE
        if (keep_alive && sse_manager_) {
            // Production mode: Register connection for streaming
            // Note: Current Beast setup limits us to batch-based streaming
            // Full keep-alive requires custom async write loop (see TODO in docs)
            //
            // At-least-once delivery note: In this path, SseConnectionManager::pollEvents()
            // returns pre-formatted SSE strings ("id: N\ndata: {...}\n\n"), not raw
            // ChangeEvent objects.  Feeding them into delivery_tracker_.trackDelivery()
            // would require parsing them back, which is fragile.  For full at-least-once
            // support in the production SSE path, SseConnectionManager should be extended
            // to return raw ChangeEvent objects alongside formatted lines.  Until then,
            // use the MVP batch path (keep_alive=false or without sse_manager_) for
            // guaranteed at-least-once delivery via consumer_id + POST /changefeed/stream/ack.
            
            uint64_t conn_id = sse_manager_->registerConnection(from_seq, key_prefix, event_types);
            span.setAttribute("sse.connection_id", static_cast<int64_t>(conn_id));
            span.setAttribute("sse.consumer_id", consumer_id);
            
            // Stream events for limited duration (configurable for tests)
            auto start = std::chrono::steady_clock::now();
            const auto max_duration = std::chrono::seconds(max_seconds);
            size_t total_events = 0;
            size_t heartbeats = 0;
            
            auto last_hb = start;
            while (std::chrono::steady_clock::now() - start < max_duration) {
                // Poll for new events (returns pre-formatted SSE strings: "id: N\ndata: ...\n\n")
                auto sse_formatted_lines = sse_manager_->pollEvents(conn_id, max_events_per_poll);
                
                if (!sse_formatted_lines.empty()) {
                    for (const auto& event_line : sse_formatted_lines) {
                        body << event_line;
                        total_events++;
                    }
                } else {
                    bool sent_hb = false;
                    if (heartbeat_ms_override > 0) {
                        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - last_hb
                        ).count();
                        if (elapsed >= heartbeat_ms_override) {
                            body << ": heartbeat\n\n";
                            sse_manager_->recordHeartbeat(conn_id);
                            heartbeats++;
                            last_hb = std::chrono::steady_clock::now();
                            sent_hb = true;
                        }
                    }
                    if (!sent_hb && sse_manager_->needsHeartbeat(conn_id)) {
                        body << ": heartbeat\n\n";
                        sse_manager_->recordHeartbeat(conn_id);
                        heartbeats++;
                    }
                }
                
                // Sleep briefly to avoid busy-wait
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            // Cleanup connection
            sse_manager_->unregisterConnection(conn_id);
            
            span.setAttribute("sse.total_events", static_cast<int64_t>(total_events));
            span.setAttribute("sse.heartbeats", static_cast<int64_t>(heartbeats));
            span.setAttribute("sse.duration_s", static_cast<int64_t>(max_seconds));
            
            THEMIS_INFO("SSE stream completed: conn={}, consumer_id='{}', events={}, heartbeats={}",
                conn_id, consumer_id, total_events, heartbeats);
            
        } else
#endif
        {
            // MVP mode: Send one batch and close (backward compatible).
            // At-least-once delivery: when a consumer_id is provided, unacknowledged
            // events from the previous delivery window are redelivered first, then
            // new events are fetched and tracked as in-flight.
            Changefeed::ListOptions options;
            options.from_sequence = from_seq;
            options.limit = 1000;
            
            if (!key_prefix.empty()) {
                options.key_prefix = key_prefix;
            }
            
            if (!event_types.empty()) {
                options.event_types = event_types;
            }

            // --- At-least-once: prepend any pending redelivery events ---
            std::vector<Changefeed::ChangeEvent> redelivery_events;
            if (!consumer_id.empty()) {
                redelivery_events = delivery_tracker_.getPendingRedelivery(consumer_id, ack_timeout_override);
                for (const auto& ev : redelivery_events) {
                    body << "id: " << ev.sequence << "\n";
                    body << "data: " << ev.toJson().dump() << "\n\n";
                }
            }

            auto events = changefeed_->listEvents(options);
            
            for (const auto& ev : events) {
                body << "id: " << ev.sequence << "\n";
                body << "data: " << ev.toJson().dump() << "\n\n";
            }
            
            if (events.empty() && redelivery_events.empty()) {
                body << ": heartbeat\n\n";
            }

            // --- At-least-once: track newly delivered events ---
            if (!consumer_id.empty() && !events.empty()) {
                delivery_tracker_.trackDelivery(consumer_id, events);
            }

            span.setAttribute("sse.mode", "mvp_batch");
            span.setAttribute("sse.consumer_id", consumer_id);
            span.setAttribute("events.count", static_cast<int64_t>(events.size()));
            span.setAttribute("events.redelivered", static_cast<int64_t>(redelivery_events.size()));
        }
        
        res.body() = body.str();
        applyGovernanceHeaders(req, res);
        res.prepare_payload();
        
        span.setStatus(true);
        return res;
        
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleStreamAck(
    const http::request<http::string_body>& req
) {
    // Authorization check
    if (auto auth_resp = checkAuth(req, "cdc:read")) {
        return *auth_resp;
    }

    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedStreamAck");
    span.setAttribute("http.path", "/changefeed/stream/ack");

    try {
        auto body_json = nlohmann::json::parse(req.body());

        if (!body_json.contains("consumer_id") || !body_json.contains("up_to_sequence")) {
            return makeErrorResponse(http::status::bad_request,
                "Required fields: 'consumer_id' (string) and 'up_to_sequence' (uint64)", req);
        }

        std::string consumer_id = body_json["consumer_id"].get<std::string>();
        if (consumer_id.empty()) {
            return makeErrorResponse(http::status::bad_request, "'consumer_id' must not be empty", req);
        }

        uint64_t up_to_sequence = body_json["up_to_sequence"].get<uint64_t>();

        span.setAttribute("sse.consumer_id", consumer_id);
        span.setAttribute("sse.up_to_sequence", static_cast<int64_t>(up_to_sequence));

        size_t removed = delivery_tracker_.acknowledgeUpTo(consumer_id, up_to_sequence);

        nlohmann::json response = {
            {"consumer_id",     consumer_id},
            {"up_to_sequence",  up_to_sequence},
            {"acknowledged",    removed}
        };

        span.setAttribute("sse.acknowledged", static_cast<int64_t>(removed));
        span.setStatus(true);

        THEMIS_DEBUG("SSE ACK: consumer='{}', up_to_seq={}, acknowledged={}",
                     consumer_id, up_to_sequence, removed);

        return makeResponse(http::status::ok, response.dump(), req);

    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleStats(
    const http::request<http::string_body>& req
) {
    // Authorization check
    if (auto auth_resp = checkAuth(req, "cdc:admin")) {
        return *auth_resp;
    }
    
    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedStats");
    span.setAttribute("http.path", "/changefeed/stats");

    try {
        auto stats = changefeed_->getStats();
        nlohmann::json response = {
            {"total_events", stats.total_events},
            {"latest_sequence", stats.latest_sequence},
            {"total_size_bytes", stats.total_size_bytes}
        };
        span.setAttribute("events.total", static_cast<int64_t>(stats.total_events));
        span.setAttribute("events.latest_seq", static_cast<int64_t>(stats.latest_sequence));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleRetention(
    const http::request<http::string_body>& req
) {
    // Authorization check
    if (auto auth_resp = checkAuth(req, "cdc:admin")) {
        return *auth_resp;
    }
    
    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedRetention");
    span.setAttribute("http.path", "/changefeed/retention");

    try {
        auto body = nlohmann::json::parse(req.body());
        // Support either before_sequence (explicit) or max_age_ms (relative)
        uint64_t before_seq = 0;
        if (body.contains("before_sequence")) {
            before_seq = body["before_sequence"].get<uint64_t>();
        } else if (body.contains("max_age_ms")) {
            // Compute a cut based on timestamp: scan stats.latest_sequence backwards is expensive;
            // MVP: if max_age_ms is provided, require also current latest_sequence from client or ignore.
            // For simplicity, we ignore timestamp-based deletion in MVP and return 400 if latest not provided.
            return makeErrorResponse(http::status::bad_request, "Only 'before_sequence' is supported for retention in MVP", req);
        } else {
            return makeErrorResponse(http::status::bad_request, "Provide 'before_sequence' (uint64)", req);
        }

        span.setAttribute("retention.before_seq", static_cast<int64_t>(before_seq));
        auto deleted = changefeed_->deleteOldEvents(before_seq);
        nlohmann::json response = {
            {"deleted", deleted},
            {"before_sequence", before_seq}
        };
        span.setAttribute("retention.deleted", static_cast<int64_t>(deleted));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request, std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleCompact(
    const http::request<http::string_body>& req
) {
    // Authorization check
    if (auto auth_resp = checkAuth(req, "cdc:admin")) {
        return *auth_resp;
    }

    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedCompact");
    span.setAttribute("http.path", "/changefeed/compact");

    try {
        themis::cdc::CDCAdmin admin(changefeed_.get());
        auto result = admin.compactLog();

        nlohmann::json response = {
            {"events_scanned",  result.events_scanned},
            {"events_deleted",  result.events_deleted},
            {"keys_compacted",  result.keys_compacted},
            {"events_retained", result.events_retained}
        };
        span.setAttribute("compact.deleted", static_cast<int64_t>(result.events_deleted));
        span.setStatus(true);
        return makeResponse(http::status::ok, response.dump(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleRetentionGet(
    const http::request<http::string_body>& req
) {
    if (auto auth_resp = checkAuth(req, "cdc:read")) {
        return *auth_resp;
    }

    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedRetentionGet");
    span.setAttribute("http.path", "/changefeed/retention");

    try {
        themis::cdc::CDCAdmin admin(changefeed_.get());
        auto status = admin.getRetentionStatus();
        span.setStatus(true);
        return makeResponse(http::status::ok, status.toJson().dump(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleRetentionPut(
    const http::request<http::string_body>& req
) {
    if (auto auth_resp = checkAuth(req, "cdc:admin")) {
        return *auth_resp;
    }

    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedRetentionPut");
    span.setAttribute("http.path", "/changefeed/retention");

    try {
        auto body = nlohmann::json::parse(req.body());

        // Read existing policy and overlay with provided fields
        Changefeed::RetentionPolicy policy = changefeed_->getRetentionPolicy();

        if (body.contains("enabled")) {
            policy.enabled = body["enabled"].get<bool>();
        }
        if (body.contains("max_age_hours")) {
            auto v = body["max_age_hours"].get<uint32_t>();
            if (v < 1 || v > 87600) { // 1h .. 10 years
                return makeErrorResponse(http::status::bad_request,
                    "max_age_hours must be 1-87600", req);
            }
            policy.max_age_hours = std::chrono::hours(v);
        }
        if (body.contains("max_event_count")) {
            auto v = body["max_event_count"].get<uint64_t>();
            if (v < 1) {
                return makeErrorResponse(http::status::bad_request,
                    "max_event_count must be >= 1", req);
            }
            policy.max_event_count = v;
        }
        if (body.contains("max_size_bytes")) {
            auto v = body["max_size_bytes"].get<uint64_t>();
            if (v < 1024 * 1024) { // min 1 MB
                return makeErrorResponse(http::status::bad_request,
                    "max_size_bytes must be >= 1048576 (1 MB)", req);
            }
            policy.max_size_bytes = v;
        }
        if (body.contains("cleanup_interval_minutes")) {
            auto v = body["cleanup_interval_minutes"].get<uint32_t>();
            if (v < 1 || v > 10080) { // 1 min .. 1 week
                return makeErrorResponse(http::status::bad_request,
                    "cleanup_interval_minutes must be 1-10080", req);
            }
            policy.cleanup_interval = std::chrono::minutes(v);
        }
        if (body.contains("compact_on_cleanup")) {
            policy.compact_on_cleanup = body["compact_on_cleanup"].get<bool>();
        }

        changefeed_->updateRetentionPolicy(policy);

        // Return current status after the update
        themis::cdc::CDCAdmin admin(changefeed_.get());
        auto status = admin.getRetentionStatus();
        span.setStatus(true);
        return makeResponse(http::status::ok, status.toJson().dump(), req);
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::handleGdprRedact(
    const http::request<http::string_body>& req
) {
    // Authorization check – requires admin scope (data erasure is a privileged operation)
    if (auto auth_resp = checkAuth(req, "cdc:admin")) {
        return *auth_resp;
    }

    // Feature flag check
    if (!feature_cdc_) {
        return makeErrorResponse(http::status::not_found, "Feature 'cdc' disabled", req);
    }

    auto span = Tracer::startSpan("handleChangefeedGdprRedact");
    span.setAttribute("http.path", "/changefeed/redact");

    try {
        auto body = nlohmann::json::parse(req.body());

        const std::string key_prefix  = body.value("key_prefix",  "");
        const std::string tenant_id   = body.value("tenant_id",   "");
        const std::string operator_id = body.value("operator_id", "");

        if (key_prefix.empty()) {
            return makeErrorResponse(http::status::bad_request,
                "key_prefix is required and must not be empty", req);
        }

        themis::cdc::CDCAdmin admin(changefeed_.get());
        admin.setAuditStorage(storage_.get());
        auto result = admin.redactByKeyPrefix(tenant_id, key_prefix, operator_id);

        span.setAttribute("redact.scanned",  static_cast<int64_t>(result.events_scanned));
        span.setAttribute("redact.redacted", static_cast<int64_t>(result.events_redacted));
        span.setStatus(true);
        return makeResponse(http::status::ok, result.toJson().dump(), req);
    } catch (const nlohmann::json::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "json_parse_error");
        return makeErrorResponse(http::status::bad_request,
                                 std::string("JSON error: ") + e.what(), req);
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "internal_error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> ChangefeedApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, error_body.dump(), req);
}

http::response<http::string_body> ChangefeedApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    applyGovernanceHeaders(req, res);
    res.prepare_payload();
    return res;
}

std::optional<http::response<http::string_body>> ChangefeedApiHandler::checkAuth(
    const http::request<http::string_body>& req, const std::string& required_scope
) {
    // If auth is disabled, allow access
    if (!auth_ || !auth_->isEnabled()) {
        return std::nullopt;
    }
    
    // Check for Authorization header
    auto it = req.find(http::field::authorization);
    if (it == req.end()) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
        res.prepare_payload();
        return res;
    }
    
    // Extract and validate token
    auto token = AuthMiddleware::extractBearerToken(
        std::string_view(it->value().data(), it->value().size())
    );
    
    if (!token) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"invalid_token","message":"Invalid Authorization header format"})";
        res.prepare_payload();
        return res;
    }
    
    // Validate token and check required scope
    auto auth_result = auth_->authorize(*token, required_scope);
    if (!auth_result.authorized) {
        http::response<http::string_body> res{http::status::forbidden, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        nlohmann::json error_body = {
            {"error", "insufficient_scope"},
            {"message", "Token does not have required scope: " + required_scope}
        };
        res.body() = error_body.dump();
        res.prepare_payload();
        return res;
    }
    
    // Authorization successful
    return std::nullopt;
}

std::optional<http::response<http::string_body>> ChangefeedApiHandler::checkAuthAndResolveTenant(
    const http::request<http::string_body>& req,
    const std::string& required_scope,
    TenantAuthContext& out_context
) {
    // If auth is disabled, allow access but still require tenant ID from headers
    if (!auth_ || !auth_->isEnabled()) {
        // Try to extract tenant from headers/path even without auth
        auto& tm = TenantManager::instance();
        
        // Build headers map
        std::unordered_map<std::string, std::string> headers_map;
        for (const auto& h : req) {
            headers_map[std::string(h.name_string())] = std::string(h.value());
        }
        
        std::string path_str(req.target());
        auto tenant_id = tm.extractTenantId(headers_map, path_str);
        
        if (!tenant_id) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::server, "THEMIS/0.1.0");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"missing_tenant","message":"Missing X-Tenant-ID header or tenant in path"})";
            res.prepare_payload();
            return res;
        }
        
        // Verify tenant exists and is enabled
        auto tenant_config = tm.getTenant(*tenant_id);
        if (!tenant_config || !tenant_config->enabled) {
            http::response<http::string_body> res{http::status::forbidden, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::server, "THEMIS/0.1.0");
            res.keep_alive(req.keep_alive());
            res.body() = R"({"error":"invalid_tenant","message":"Tenant not found or disabled"})";
            res.prepare_payload();
            return res;
        }
        
        out_context.tenant_id = *tenant_id;
        out_context.user_id = "anonymous";
        return std::nullopt;
    }
    
    // Check for Authorization header
    auto it = req.find(http::field::authorization);
    if (it == req.end()) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"missing_authorization","message":"Missing Authorization header"})";
        res.prepare_payload();
        return res;
    }
    
    // Extract and validate token
    auto token = AuthMiddleware::extractBearerToken(
        std::string_view(it->value().data(), it->value().size())
    );
    
    if (!token) {
        http::response<http::string_body> res{http::status::unauthorized, req.version()};
        res.set(http::field::www_authenticate, "Bearer realm=\"themis\"");
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        res.body() = R"({"error":"invalid_token","message":"Invalid Authorization header format"})";
        res.prepare_payload();
        return res;
    }
    
    // Validate token and check required scope
    auto auth_result = auth_->authorize(*token, required_scope);
    if (!auth_result.authorized) {
        http::response<http::string_body> res{http::status::forbidden, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        nlohmann::json error_body = {
            {"error", "insufficient_scope"},
            {"message", "Token does not have required scope: " + required_scope}
        };
        res.body() = error_body.dump();
        res.prepare_payload();
        return res;
    }
    
    // Extract tenant from JWT or request headers
    auto& tm = TenantManager::instance();
    std::string tenant_id_from_auth = auth_result.tenant_id;
    
    // Build headers map
    std::unordered_map<std::string, std::string> headers_map;
    for (const auto& h : req) {
        headers_map[std::string(h.name_string())] = std::string(h.value());
    }
    
    std::string path_str(req.target());
    auto tenant_id_from_request = tm.extractTenantId(headers_map, path_str);
    
    // Determine which tenant ID to use
    std::string final_tenant_id;
    if (!tenant_id_from_auth.empty()) {
        // Use tenant from JWT/token
        final_tenant_id = tenant_id_from_auth;
        
        // If request also specified tenant, they must match (prevent cross-tenant access)
        if (tenant_id_from_request && *tenant_id_from_request != tenant_id_from_auth) {
            http::response<http::string_body> res{http::status::forbidden, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::server, "THEMIS/0.1.0");
            res.keep_alive(req.keep_alive());
            nlohmann::json error_body = {
                {"error", "tenant_mismatch"},
                {"message", "Tenant ID in request does not match authenticated tenant"}
            };
            res.body() = error_body.dump();
            res.prepare_payload();
            return res;
        }
    } else if (tenant_id_from_request) {
        // Use tenant from request header/path
        final_tenant_id = *tenant_id_from_request;
    } else {
        // No tenant specified - FAIL CLOSED
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        nlohmann::json error_body = {
            {"error", "missing_tenant"},
            {"message", "Tenant ID must be provided via X-Tenant-ID header, path, or JWT claim"}
        };
        res.body() = error_body.dump();
        res.prepare_payload();
        return res;
    }
    
    // Verify tenant exists and is enabled
    auto tenant_config = tm.getTenant(final_tenant_id);
    if (!tenant_config || !tenant_config->enabled) {
        http::response<http::string_body> res{http::status::forbidden, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::server, "THEMIS/0.1.0");
        res.keep_alive(req.keep_alive());
        nlohmann::json error_body = {
            {"error", "invalid_tenant"},
            {"message", "Tenant not found or disabled"}
        };
        res.body() = error_body.dump();
        res.prepare_payload();
        return res;
    }
    
    // Set output context
    out_context.user_id = auth_result.user_id;
    out_context.tenant_id = final_tenant_id;
    out_context.groups = auth_result.groups;
    
    // Record request for tenant
    tm.recordRequest(final_tenant_id);
    
    // Authorization and tenant validation successful
    return std::nullopt;
}

void ChangefeedApiHandler::applyGovernanceHeaders(
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res
) {
    // Apply governance headers to changefeed events
    // This ensures compliance and audit requirements are met
    
    auto to_lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    
    std::string path_only = std::string(req.target());
    auto qpos = path_only.find('?');
    if (qpos != std::string::npos) path_only = path_only.substr(0, qpos);
    
    // Read incoming governance hints from request headers
    std::string classification = ""; // offen | geheim | streng-geheim | vs-nfd
    std::string mode = "observe";    // observe (default) | enforce
    bool encrypt_logs = false;
    
    for (const auto& h : req) {
        auto name = h.name_string();
        if (beast::iequals(name, "X-Classification")) {
            classification = to_lower(std::string(h.value()));
        } else if (beast::iequals(name, "X-Governance-Mode")) {
            mode = to_lower(std::string(h.value()));
        } else if (beast::iequals(name, "X-Encrypt-Logs")) {
            std::string v = to_lower(std::string(h.value()));
            encrypt_logs = (v == "true" || v == "1" || v == "yes");
        }
    }
    
    // Resource-based default classification if none provided
    // Changefeeds typically contain sensitive data, default to vs-nfd for CDC
    if (classification.empty()) {
        if (path_only.rfind("/changefeed", 0) == 0) {
            classification = "vs-nfd"; // CDC data is sensitive by default
        } else {
            classification = "offen";
        }
    }
    
    // Normalize/validate known values
    if (classification != "offen" && classification != "geheim" && 
        classification != "streng-geheim" && classification != "vs-nfd") {
        // Unknown classification -> keep text but apply restrictive defaults
        // (no-op: classification value is preserved as-is)
    }
    if (mode != "observe" && mode != "enforce") mode = "observe";
    
    // Derive header values from classification level
    std::string content_enc = "optional";
    std::string export_perm = "allowed";
    std::string cache_perm = "disabled"; // CDC streams should not be cached
    std::string retention_days = "365";
    std::string redaction = "none";
    std::string cdc_encryption = "optional";
    std::string cdc_audit = "enabled"; // Always audit CDC access
    
    if (classification == "geheim") {
        cache_perm = "disabled";
        cdc_encryption = "recommended";
        retention_days = "730"; // 2 years for geheim
    } else if (classification == "streng-geheim") {
        content_enc = "required";
        export_perm = "forbidden";
        cache_perm = "disabled";
        redaction = "strict";
        retention_days = "1095"; // 3 years
        cdc_encryption = "required";
    } else if (classification == "vs-nfd") {
        content_enc = "required";
        retention_days = "730"; // 2 years
        cdc_encryption = "recommended";
    } else if (classification == "offen") {
        retention_days = "365"; // 1 year for public data
    }
    
    // Compose policy summary
    std::string policy_summary = "classification=" + classification + 
                                ";mode=" + mode + 
                                ";encrypt_logs=" + (encrypt_logs ? "true" : "false") + 
                                ";redaction=" + redaction;
    
    // Write governance headers
    res.set("X-Themis-Policy", policy_summary);
    res.set("X-Themis-Content-Enc", content_enc);
    res.set("X-Themis-Export", export_perm);
    res.set("X-Themis-Cache", cache_perm);
    res.set("X-Themis-Retention-Days", retention_days);
    
    // CDC-specific governance headers
    res.set("X-Themis-CDC-Encryption", cdc_encryption);
    res.set("X-Themis-CDC-Audit", cdc_audit);
    res.set("X-Themis-CDC-Classification", classification);
    
    // Add lineage tracking header
    res.set("X-Themis-CDC-Source", "ThemisDB");
    
    // Add timestamp for audit trail
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    res.set("X-Themis-CDC-Timestamp", std::to_string(ms));
    
    // Security headers for CDC endpoints
    res.set("X-Frame-Options", "DENY");
    res.set("X-Content-Type-Options", "nosniff");
    res.set("Referrer-Policy", "no-referrer");
    res.set("Content-Security-Policy", "default-src 'none'; frame-ancestors 'none'; base-uri 'none'");
    res.set("X-XSS-Protection", "0");
}

} // namespace server
} // namespace themis
