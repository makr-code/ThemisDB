/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            changefeed_api_handler.h                           ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:46:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b56122b397  2026-03-11  feat(cdc): extend at-least-once delivery guarantee to SSE... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <optional>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include "server/auth_middleware.h"
#include "cdc/delivery_tracker.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;
class Changefeed;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class SseConnectionManager;

/**
 * @brief Handler for Changefeed (CDC) Operations
 * 
 * This handler manages all changefeed-related endpoints:
 * - GET /changefeed - Get changefeed events (polling)
 * - GET /changefeed/stream - Stream changefeed events via Server-Sent Events (SSE)
 * - GET /changefeed/stats - Get changefeed statistics
 * - POST /changefeed/retention - Configure retention policy
 * 
 * Features:
 * - Change Data Capture (CDC)
 * - Real-time event streaming
 * - SSE (Server-Sent Events) support
 * - Event filtering and pagination
 * - Retention management
 * 
 * Extracted from http_server.cpp (~400 lines) to improve maintainability.
 */
class ChangefeedApiHandler {
public:
    /**
     * @brief Construct a new Changefeed API Handler
     * 
     * @param storage Storage backend
     * @param changefeed Changefeed manager
     * @param sse_manager SSE connection manager (optional)
     * @param auth Authentication/authorization middleware
     * @param feature_cdc Whether CDC feature is enabled
     */
    ChangefeedApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<Changefeed> changefeed,
        std::shared_ptr<SseConnectionManager> sse_manager,
        std::shared_ptr<themis::AuthMiddleware> auth,
        bool feature_cdc
    );

    /**
     * @brief Handle GET /changefeed request
     * @param req HTTP request with query parameters (since, limit, filter)
     * @return HTTP response with changefeed events
     */
    http::response<http::string_body> handleGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /changefeed/stream request (SSE)
     * @param req HTTP request for SSE stream
     * @return HTTP response with SSE stream
     */
    http::response<http::string_body> handleStreamSse(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /changefeed/stats request
     * @param req HTTP request
     * @return HTTP response with changefeed statistics
     */
    http::response<http::string_body> handleStats(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/retention request
     * @param req HTTP request with retention configuration
     * @return HTTP response with update status
     */
    http::response<http::string_body> handleRetention(const http::request<http::string_body>& req);

    /**
     * @brief Handle GET /changefeed/retention request
     *
     * Returns the current retention status (log size, oldest event age,
     * next scheduled cleanup time, compact_on_cleanup flag).
     */
    http::response<http::string_body> handleRetentionGet(const http::request<http::string_body>& req);

    /**
     * @brief Handle PUT /changefeed/retention request
     *
     * Updates the retention policy at runtime.  Accepted JSON fields:
     *   enabled (bool), max_age_hours (uint32), max_event_count (uint64),
     *   max_size_bytes (uint64), cleanup_interval_minutes (uint32),
     *   compact_on_cleanup (bool)
     */
    http::response<http::string_body> handleRetentionPut(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/compact request
     *
     * Triggers a key-based log compaction: removes superseded entries,
     * keeping only the latest event per document key.
     *
     * @param req HTTP request
     * @return HTTP response with compaction statistics
     */
    http::response<http::string_body> handleCompact(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/stream/ack request
     *
     * Acknowledges delivery of SSE change events, clearing them from the
     * at-least-once in-flight tracking state for the given consumer.
     *
     * Accepted JSON body fields:
     *   consumer_id      (string, required) – opaque consumer identifier
     *   up_to_sequence   (uint64, required) – highest sequence number to acknowledge
     *
     * @param req HTTP request with JSON body
     * @return HTTP response with acknowledgement result
     */
    http::response<http::string_body> handleStreamAck(const http::request<http::string_body>& req);

    /**
     * @brief Handle POST /changefeed/redact request (GDPR right to erasure)
     *
     * Scrubs PII from all change log entries whose key starts with the
     * supplied key prefix.  Replaces the value, before_snapshot, and
     * after_snapshot fields with "[REDACTED]" / nullopt and sets
     * redacted = true.  Audit-critical fields (sequence, type, key,
     * timestamp_ms) are preserved.
     *
     * Accepted JSON body fields:
     *   key_prefix   (string, required) – data-subject key prefix, e.g. "user:42"
     *   tenant_id    (string, optional) – tenant scope for the audit record
     *   operator_id  (string, optional) – identity of the requesting operator
     *
     * @param req HTTP request with JSON body
     * @return HTTP response with GDPRRedactionResult JSON
     */
    http::response<http::string_body> handleGdprRedact(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<Changefeed> changefeed_;
    std::shared_ptr<SseConnectionManager> sse_manager_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    bool feature_cdc_;

    /// At-least-once delivery tracker for SSE consumers.
    /// Tracks in-flight events per consumer_id until the client ACKs them.
    cdc::DeliveryTracker delivery_tracker_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
    
    // Authorization helper
    std::optional<http::response<http::string_body>> checkAuth(
        const http::request<http::string_body>& req, const std::string& required_scope);
    
    // Tenant isolation helper
    struct TenantAuthContext {
        std::string user_id;
        std::string tenant_id;
        std::vector<std::string> groups;
    };
    std::optional<http::response<http::string_body>> checkAuthAndResolveTenant(
        const http::request<http::string_body>& req, 
        const std::string& required_scope,
        TenantAuthContext& out_context);
    
    // Governance headers
    void applyGovernanceHeaders(
        const http::request<http::string_body>& req,
        http::response<http::string_body>& res);
};

} // namespace server
} // namespace themis
