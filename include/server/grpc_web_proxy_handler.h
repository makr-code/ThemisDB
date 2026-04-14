/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            grpc_web_proxy_handler.h                           ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-14 18:42:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     232                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 39ac8c3efe  2026-03-20  Split default-arg constructors into overloads ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • de101321ad  2026-03-01  feat(server): implement gRPC-Web proxy handler for browse... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <boost/beast/http.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief gRPC-Web proxy handler for browser clients.
 *
 * Enables browser clients to communicate with ThemisDB's native gRPC services
 * by acting as a translation layer between the gRPC-Web protocol (HTTP/1.1
 * compatible) and standard gRPC (HTTP/2).
 *
 * ## Protocol Overview
 *
 * The gRPC-Web protocol wraps gRPC messages so browsers can send them over
 * HTTP/1.1.  Each message is prefixed with a 5-byte frame header:
 *
 * @code
 * +---------+-------------------+
 * | flags   | message length    |
 * | 1 byte  | 4 bytes big-endian|
 * +---------+-------------------+
 * | message bytes (length)      |
 * +-----------------------------+
 * @endcode
 *
 * Response trailers are encoded as a special trailer data frame with the
 * high bit set in the flags byte (0x80):
 *
 * @code
 * +---------+-------------------+
 * | 0x80    | trailer length    |
 * | (flags) | 4 bytes big-endian|
 * +---------+-------------------+
 * | trailer text: "key: val\r\n"|
 * +-----------------------------+
 * @endcode
 *
 * ## Endpoints
 *
 *   POST    /grpc-web/<Package>.<Service>/<Method>      – proxy unary gRPC call
 *   OPTIONS /grpc-web/<Package>.<Service>/<Method>      – CORS preflight
 *   GET     /api/v1/grpc-web/status                     – proxy health / config
 *
 * ## Content-Type
 *
 *   `application/grpc-web+proto`  – binary protobuf (preferred)
 *   `application/grpc-web`        – same; treated as binary
 *
 * @note Thread-safe – all public methods are stateless; shared state (the gRPC
 *       channel) is thread-safe by the grpcpp contract.
 */
class GrpcWebProxyHandler {
public:
    /**
     * @brief Configuration for the proxy.
     */
    struct Config {
        /// Address of the backend gRPC server (host:port).
        std::string backend_address = "localhost:18765";

        /// Value for the Access-Control-Allow-Origin header.
        /// Use "*" to allow all origins or restrict to a specific domain.
        std::string cors_allow_origin = "*";

        /// Whether the backend gRPC connection uses TLS.
        bool backend_tls = false;

        /// Request deadline in milliseconds (0 = no deadline).
        uint32_t deadline_ms = 30000;
    };

    /**
     * @brief Construct a proxy with the given configuration.
     *
     * The gRPC channel to the backend is created lazily on the first request
     * so that the handler can be constructed before the gRPC server starts.
     *
     * @param config Proxy configuration.
     */
    GrpcWebProxyHandler();
    explicit GrpcWebProxyHandler(Config config);

    ~GrpcWebProxyHandler() = default;

    // Non-copyable, movable
    GrpcWebProxyHandler(const GrpcWebProxyHandler&) = delete;
    GrpcWebProxyHandler& operator=(const GrpcWebProxyHandler&) = delete;
    GrpcWebProxyHandler(GrpcWebProxyHandler&&) = default;
    GrpcWebProxyHandler& operator=(GrpcWebProxyHandler&&) = default;

    /**
     * @brief Handle an incoming gRPC-Web POST request.
     *
     * Parses the gRPC-Web frame from the request body, forwards the raw
     * protobuf payload to the backend gRPC server using a generic call,
     * and wraps the response in a gRPC-Web data frame followed by a trailer
     * frame.
     *
     * @param req  HTTP request with Content-Type application/grpc-web[+proto].
     * @param method gRPC method path, e.g. "/package.Service/Method".
     * @return HTTP 200 with gRPC-Web encoded body, or 4xx/5xx on error.
     */
    http::response<http::string_body> handlePost(
        const http::request<http::string_body>& req,
        const std::string& method);

    /**
     * @brief Handle a CORS preflight OPTIONS request.
     *
     * Returns the appropriate Access-Control-* headers to permit browser
     * clients to make gRPC-Web requests from any origin (or a configured
     * origin).
     *
     * @param req Incoming OPTIONS request.
     * @return HTTP 200 with CORS headers and empty body.
     */
    http::response<http::string_body> handleOptions(
        const http::request<http::string_body>& req);

    /**
     * @brief Return proxy status and configuration as JSON.
     *
     * Exposed at GET /api/v1/grpc-web/status.
     *
     * @param req Incoming GET request.
     * @return HTTP 200 with JSON status object.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req);

    // ── gRPC-Web frame helpers (public for testability) ─────────────────────

    /**
     * @brief Decode a gRPC-Web framed body into raw protobuf bytes.
     *
     * A well-formed gRPC-Web data frame starts with a single flag byte
     * (0 = uncompressed) followed by four big-endian bytes encoding the
     * message length, then exactly that many message bytes.  Compressed
     * frames (flag bit 0x01) are rejected because this proxy does not
     * perform re-compression for the backend.
     *
     * @param body     Raw HTTP request body.
     * @param out_msg  Populated with the decoded protobuf bytes on success.
     * @return true on success, false if the frame is malformed.
     */
    static bool decodeGrpcWebFrame(const std::string& body,
                                   std::string& out_msg);

    /**
     * @brief Encode a protobuf message and gRPC status into a gRPC-Web body.
     *
     * The output consists of two frames:
     *  1. A data frame  (flags = 0x00) containing @p proto_msg.
     *  2. A trailer frame (flags = 0x80) containing the gRPC status and
     *     message as ASCII "key: value\\r\\n" pairs.
     *
     * @param proto_msg   Serialised protobuf response bytes.
     * @param grpc_status gRPC status code (0 = OK).
     * @param grpc_message Human-readable status message (empty on success).
     * @return Binary string containing the two gRPC-Web frames.
     */
    static std::string encodeGrpcWebResponse(const std::string& proto_msg,
                                             int grpc_status,
                                             const std::string& grpc_message);

private:
    Config config_;

    // ── gRPC channel (lazily initialised, thread-safe after first use) ───────
    mutable std::shared_ptr<void> channel_holder_; ///< opaque grpc::Channel
    mutable std::shared_ptr<void> stub_holder_;    ///< opaque grpc::GenericStub

    void ensureChannel() const;

    // ── HTTP response helpers ────────────────────────────────────────────────

    void addCorsHeaders(http::response<http::string_body>& res) const;

    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const std::string& content_type,
        const http::request<http::string_body>& req) const;

    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;

    // ── Frame encoding helpers ───────────────────────────────────────────────

    static void appendFrameHeader(std::string& out,
                                   uint8_t flags,
                                   uint32_t length);
};

} // namespace server
} // namespace themis
