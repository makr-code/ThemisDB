/**
 * @file grpc_web_proxy_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=10; TODO=1, Stub=4, Unimpl=3, Mock=1, Sim=1, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/grpc_web_proxy_handler.h"
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <chrono>
#include <mutex>
#include <spdlog/spdlog.h>

#ifdef THEMIS_ENABLE_GRPC
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>
#endif

#include <cstring>
#include <mutex>
#include <sstream>
#include <utility>
#include "utils/logger.h"

namespace themis {
namespace server {

using json = nlohmann::json;

namespace {

std::mutex& backendInvokeFnMutex()
{
    static std::mutex mutex;
    return mutex;
}

GrpcWebProxyHandler::BackendInvokeFn& backendInvokeFnStorage()
{
    static GrpcWebProxyHandler::BackendInvokeFn callback;
    return callback;
}

GrpcWebProxyHandler::BackendInvokeFn getBackendInvokeFn()
{
    std::lock_guard<std::mutex> lock(backendInvokeFnMutex());
    return backendInvokeFnStorage();
}

} // namespace

void GrpcWebProxyHandler::setBackendInvokeFn(BackendInvokeFn fn)
{
    std::lock_guard<std::mutex> lock(backendInvokeFnMutex());
    backendInvokeFnStorage() = std::move(fn);
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame helpers
// ─────────────────────────────────────────────────────────────────────────────

void GrpcWebProxyHandler::appendFrameHeader(std::string& out,
                                             uint8_t flags,
                                             uint32_t length)
{
    out.push_back(static_cast<char>(flags));
    out.push_back(static_cast<char>((length >> 24) & 0xFF));
    out.push_back(static_cast<char>((length >> 16) & 0xFF));
    out.push_back(static_cast<char>((length >>  8) & 0xFF));
    out.push_back(static_cast<char>( length        & 0xFF));
}

bool GrpcWebProxyHandler::decodeGrpcWebFrame(const std::string& body,
                                              std::string& out_msg)
{
    // Minimum frame: 5-byte header
    if (body.size() < 5) {
        return false;
    }

    const uint8_t flags = static_cast<uint8_t>(body[0]);

    // Reject compressed frames – this proxy does not handle re-compression
    if (flags & 0x01) {
        return false;
    }

    // Only data frames (flag = 0x00) are valid request frames
    if (flags != 0x00) {
        return false;
    }

    // Read 4-byte big-endian message length
    const uint32_t msg_len =
        (static_cast<uint32_t>(static_cast<uint8_t>(body[1])) << 24) |
        (static_cast<uint32_t>(static_cast<uint8_t>(body[2])) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(body[3])) <<  8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(body[4])));

    // Validate that the body contains the promised bytes
    if (body.size() < static_cast<size_t>(5) + msg_len) {
        return false;
    }

    out_msg.assign(body.data() + 5, msg_len);
    return true;
}

std::string GrpcWebProxyHandler::encodeGrpcWebResponse(
    const std::string& proto_msg,
    int grpc_status,
    const std::string& grpc_message)
{
    std::string result;

    // 1. Data frame  (flags = 0x00)
    appendFrameHeader(result, 0x00, static_cast<uint32_t>(proto_msg.size()));
    result.append(proto_msg);

    // 2. Trailer frame  (flags = 0x80)
    std::string trailers;
    trailers += "grpc-status: " + std::to_string(grpc_status) + "\r\n";
    if (!grpc_message.empty()) {
        trailers += "grpc-message: " + grpc_message + "\r\n";
    }
    appendFrameHeader(result, 0x80, static_cast<uint32_t>(trailers.size()));
    result.append(trailers);

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

GrpcWebProxyHandler::GrpcWebProxyHandler()
    : GrpcWebProxyHandler(Config{})
{}

GrpcWebProxyHandler::GrpcWebProxyHandler(Config config)
    : config_(std::move(config))
{
    // GAP-012: Warn when the CORS allow-origin is the wildcard '*' (CWE-346).
    // A wildcard allows any browser origin to read gRPC-Web responses, which
    // violates the principle of least privilege.  Set Config::cors_allow_origin
    // to a specific origin (e.g. "https://app.example.com") in production.
    if (config_.cors_allow_origin == "*") {
        spdlog::warn("[SECURITY] GrpcWebProxy: cors_allow_origin='*' - any origin can read "
                     "gRPC-Web responses. Configure a specific origin in production "
                     "(GAP-012/CWE-346).");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Channel management
// ─────────────────────────────────────────────────────────────────────────────

void GrpcWebProxyHandler::ensureChannel() const
{
#ifdef THEMIS_ENABLE_GRPC
    if (channel_holder_) {
        return;
    }
    std::shared_ptr<grpc::ChannelCredentials> creds =
        config_.backend_tls ? grpc::SslCredentials(grpc::SslCredentialsOptions{})
                            : grpc::InsecureChannelCredentials();
    auto channel = grpc::CreateChannel(config_.backend_address, creds);
    channel_holder_ = channel;
    stub_holder_    = std::make_shared<grpc::GenericStub>(channel);
#endif
    // PERMANENT HARDWARE FALLBACK NOTE (gRPC SDK not available):
    // Purpose: Allow GrpcWebProxyHandler to be instantiated without gRPC.
    //   When `THEMIS_ENABLE_GRPC` is not defined, `ensureChannel()` is a no-op,
    //   `channel_holder_` and `stub_holder_` remain null, and any subsequent
    //   `handlePost()` call returns HTTP 200 with a gRPC-Web trailer advertising
    //   status code 12 (UNIMPLEMENTED) and the message
    //   "gRPC backend not available in this build".
    // Activation: `THEMIS_ENABLE_GRPC` not defined at compile time (default for
    //   builds without the gRPC SDK or without `-DTHEMIS_ENABLE_GRPC=1`).
    // Production Delta: All gRPC-Web proxy calls are rejected with UNIMPLEMENTED.
    //   Browser/frontend clients using gRPC-Web will receive an error response
    //   for every RPC.  CORS preflight and status endpoints remain functional.
    // Hardware requirement: gRPC C++ libraries + -DTHEMIS_ENABLE_GRPC=1.
}

// ─────────────────────────────────────────────────────────────────────────────
// CORS helpers
// ─────────────────────────────────────────────────────────────────────────────

void GrpcWebProxyHandler::addCorsHeaders(
    http::response<http::string_body>& res) const
{
    res.set("Access-Control-Allow-Origin", config_.cors_allow_origin);
    res.set("Access-Control-Allow-Methods", "POST, OPTIONS");
    res.set("Access-Control-Allow-Headers",
            "Content-Type, x-user-agent, x-grpc-web, grpc-timeout");
    res.set("Access-Control-Expose-Headers",
            "grpc-status, grpc-message, grpc-status-details-bin");
    res.set("Access-Control-Max-Age", "86400");
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP response helpers
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GrpcWebProxyHandler::makeResponse(
    http::status status,
    const std::string& body,
    const std::string& content_type,
    const http::request<http::string_body>& req) const
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, content_type);
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    addCorsHeaders(res);
    return res;
}

http::response<http::string_body> GrpcWebProxyHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req) const
{
    json body = {{"error", message}};
    return makeResponse(status, body.dump(), "application/json", req);
}

// ─────────────────────────────────────────────────────────────────────────────
// OPTIONS – CORS preflight
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GrpcWebProxyHandler::handleOptions(
    const http::request<http::string_body>& req)
{
    return makeResponse(http::status::ok, "", "text/plain", req);
}

// ─────────────────────────────────────────────────────────────────────────────
// GET /api/v1/grpc-web/status
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GrpcWebProxyHandler::handleStatus(
    const http::request<http::string_body>& req)
{
    json status = {
        {"backend_address", config_.backend_address},
        {"backend_tls",     config_.backend_tls},
        {"deadline_ms",     config_.deadline_ms},
        {"cors_allow_origin", config_.cors_allow_origin}
    };
    return makeResponse(http::status::ok, status.dump(), "application/json", req);
}

// ─────────────────────────────────────────────────────────────────────────────
// POST /grpc-web/<Service>/<Method>
// ─────────────────────────────────────────────────────────────────────────────

http::response<http::string_body> GrpcWebProxyHandler::handlePost(
    const http::request<http::string_body>& req,
    const std::string& method)
{
    // Validate Content-Type
    const std::string content_type{req[http::field::content_type]};
    if (content_type.find("application/grpc-web") == std::string::npos) {
        return makeErrorResponse(http::status::unsupported_media_type,
            "Content-Type must be application/grpc-web or application/grpc-web+proto",
            req);
    }

    // Decode gRPC-Web data frame
    std::string proto_payload;
    if (!decodeGrpcWebFrame(req.body(), proto_payload)) {
        return makeErrorResponse(http::status::bad_request,
            "Invalid gRPC-Web frame: malformed 5-byte frame header", req);
    }

    std::string response_proto;
    int grpc_code = 0;          // grpc::StatusCode::OK
    std::string grpc_message;
    bool handled_by_override = false;
    if (auto backend_invoke = getBackendInvokeFn(); backend_invoke) {
        handled_by_override = backend_invoke(
            method, proto_payload, response_proto, grpc_code, grpc_message);
    }

    if (!handled_by_override) {
#ifdef THEMIS_ENABLE_GRPC
    ensureChannel();

    auto* stub = static_cast<grpc::GenericStub*>(stub_holder_.get());

    grpc::ClientContext ctx;

    // Propagate grpc-timeout if present
    const std::string timeout_hdr{req["grpc-timeout"]};
    if (!timeout_hdr.empty() && timeout_hdr.size() > 1) {
        // Format: <value><unit>  where unit ∈ {H,M,S,m,u,n}
        try {
            const char unit = timeout_hdr.back();
            const int64_t value = std::stoll(timeout_hdr.substr(0, timeout_hdr.size() - 1));
            using namespace std::chrono;
            system_clock::time_point deadline = system_clock::now();
            switch (unit) {
                case 'H': deadline = system_clock::now() + std::chrono::duration_cast<system_clock::duration>(hours(value));        break;
                case 'M': deadline = system_clock::now() + std::chrono::duration_cast<system_clock::duration>(minutes(value));      break;
                case 'S': deadline = system_clock::now() + std::chrono::duration_cast<system_clock::duration>(seconds(value));      break;
                case 'm': deadline = system_clock::now() + std::chrono::duration_cast<system_clock::duration>(milliseconds(value)); break;
                case 'u': deadline = system_clock::now() + std::chrono::duration_cast<system_clock::duration>(microseconds(value)); break;
                case 'n': deadline = system_clock::now() + std::chrono::duration_cast<system_clock::duration>(nanoseconds(value));  break;
                default:  break;
            }
            if (unit == 'H' || unit == 'M' || unit == 'S' ||
                unit == 'm' || unit == 'u' || unit == 'n') {
                ctx.set_deadline(deadline);
            }
        } catch (...) {
            THEMIS_WARN("grpc_web_proxy_handler: unhandled exception caught");
            // Ignore malformed grpc-timeout; use default deadline
        }
    } else if (config_.deadline_ms > 0) {
        ctx.set_deadline(std::chrono::system_clock::now() +
                         std::chrono::milliseconds(config_.deadline_ms));
    }

    // Forward gRPC metadata headers (x-grpc-web-* pass-through)
    for (const auto& field : req) {
        const std::string name = std::string(field.name_string());
        if (name.rfind("x-", 0) == 0) {       // forward x-* custom headers
            ctx.AddMetadata(name, std::string(field.value()));
        }
    }

    // Build request ByteBuffer from raw protobuf payload
    grpc::Slice req_slice(proto_payload.data(), proto_payload.size());
    grpc::ByteBuffer request_buf(&req_slice, 1);

    // Perform blocking generic unary call via CompletionQueue
    grpc::ByteBuffer response_buf;
    grpc::Status status;

    {
        grpc::CompletionQueue cq;
        auto call = stub->PrepareUnaryCall(&ctx, method, request_buf, &cq);
        call->StartCall();
        call->Finish(&response_buf, &status, reinterpret_cast<void*>(1));

        void* tag  = nullptr;
        bool  ok   = false;
        cq.Next(&tag, &ok);
    }

    grpc_code    = static_cast<int>(status.error_code());
    grpc_message = status.error_message();

    if (status.ok()) {
        // Deserialise ByteBuffer to string
        std::vector<grpc::Slice> slices;
        if (response_buf.Dump(&slices).ok()) {
            for (const auto& slice : slices) {
                response_proto.append(
                    reinterpret_cast<const char*>(slice.begin()),
                    slice.size());
            }
        }
    }
#else
    // gRPC not compiled in: rely on injected callback when available.
    if (auto fn = getBackendInvokeFn(); fn) {
        const bool ok = fn(method, proto_payload, response_proto, grpc_code, grpc_message);
        if (!ok && grpc_code == 0) {
            grpc_code = 13; // INTERNAL
            if (grpc_message.empty()) {
                grpc_message = "BackendInvokeFn returned false";
            }
        }
    } else {
        THEMIS_INFO("[AUDIT] gRPC-Web proxy request rejected: UNIMPLEMENTED");
        grpc_code    = 12; // grpc::StatusCode::UNIMPLEMENTED
        grpc_message = "gRPC backend not available in this build";
    }
#endif
    }

    // Encode gRPC-Web response (data frame + trailer frame)
    const std::string grpc_web_body =
        encodeGrpcWebResponse(response_proto, grpc_code, grpc_message);

    return makeResponse(http::status::ok,
                        grpc_web_body,
                        "application/grpc-web+proto",
                        req);
}

} // namespace server
} // namespace themis


