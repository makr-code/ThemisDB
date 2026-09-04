/**
 * @file grpc_bridge.h
 * @brief Bridge layer between Beast HTTP/REST and gRPC protocol handlers.
 *
 * @details Adapts HTTP request/response abstractions to gRPC-compatible serialization
 * and message encoding.  Enables shared business logic across REST and gRPC transports.
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include "api/http_handler.h"
#include "utils/expected.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// ServiceDescriptor — plain-data descriptor for a gRPC service registration
// ---------------------------------------------------------------------------

/**
 * @brief Plain-data struct describing a gRPC service to be registered with the bridge.
 *
 * Passed to `IGRPCBridge::registerService()` at server initialization.
 * Maps a Protobuf service (identified by `service_name`) to the set of RPC
 * method names it exposes.
 */
struct ServiceDescriptor {
    /// Fully-qualified gRPC service name, e.g. "themis.v1.ThemisDB".
    std::string service_name = {};

    /// Package prefix prepended to `service_name` in the Protobuf namespace.
    std::string package; // e.g. "themis.v1"

    /// List of RPC method names exposed by this service, e.g. "GetEntity", "ExecuteAQL".
    std::vector<std::string> method_names;

    /// Serialization format used on the wire ("proto" or "json").
    /// Most gRPC services use "proto"; gRPC-JSON transcoding uses "json".
    std::string serialization_format = "proto";
};

// ---------------------------------------------------------------------------
// GRPCMetadata — propagates gRPC headers into the IHttpHandler invocation
// ---------------------------------------------------------------------------

/**
 * @brief Value type carrying gRPC metadata (headers) for a single RPC call.
 *
 * `IGRPCBridge::dispatch()` converts gRPC metadata into an `HttpRequest` and
 * passes the raw metadata separately as a `GRPCMetadata` so that handlers
 * can inspect gRPC-specific fields (authority, deadline, content-type) without
 * having to parse HTTP headers.
 */
struct GRPCMetadata {
    /// gRPC `:authority` pseudo-header (maps to HTTP/2 `:authority`).
    std::string authority;

    /// gRPC method path, e.g. "/themis.v1.ThemisDB/GetEntity".
    std::string method_path;

    /// gRPC `content-type` header, e.g. "application/grpc" or "application/grpc+json".
    std::string content_type = "application/grpc";

    /// Deadline expressed as ISO-8601 timestamp string (empty = no deadline).
    std::string deadline = {};

    /// Arbitrary user-defined key/value metadata entries (gRPC custom headers).
    std::unordered_map<std::string, std::string> user_metadata;

    /// Return `true` if a deadline was set for this RPC call.
    bool hasDeadline() const noexcept { return !deadline.empty(); }
};

// ---------------------------------------------------------------------------
// GRPCRequest — a single inbound gRPC call
// ---------------------------------------------------------------------------

/**
 * @brief Represents a single inbound gRPC request delivered to the bridge.
 *
 * The bridge converts this to an `HttpRequest` before calling the registered
 * `IHttpHandler`.  The `request_bytes` field contains the raw Protobuf-encoded
 * (or JSON-encoded) request message body.
 */
struct GRPCRequest {
    GRPCMetadata metadata;

    /// Name of the service being called (must match a registered `ServiceDescriptor`).
    std::string service_name;

    /// Name of the RPC method being called, e.g. "GetEntity".
    std::string method_name;

    /// Raw serialized request body (Protobuf wire format or JSON depending on
    /// the service's `serialization_format`).
    std::string request_bytes;
};

// ---------------------------------------------------------------------------
// IGRPCBridge — pure-virtual interface for gRPC-to-handler dispatch
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface that bridges gRPC requests to `IHttpHandler`
 *        implementations.
 *
 * The bridge:
 *  1. Accepts `registerService()` calls at initialization, mapping a
 *     `ServiceDescriptor` to an `IHttpHandler`.
 *  2. Receives `dispatch()` calls at runtime, converts the `GRPCRequest` to
 *     an `HttpRequest`, invokes the registered handler, and returns the
 *     `HttpResponse` to be serialized back into the gRPC response envelope.
 *
 * ### Contract
 * - `registerService()` must be called before `dispatch()`.
 * - `dispatch()` rejects requests with unknown `service_name` values at this
 *   layer (returns HTTP 404 equivalent) rather than forwarding to an
 *   unregistered handler.
 * - The bridge converts Protobuf wire format to an internal `HttpRequest::body`
 *   string before passing it to the handler; the handler never sees raw gRPC
 *   framing.
 * - gRPC metadata is propagated into `HttpRequest::headers` using the standard
 *   header naming convention (`:authority` → `Host`, etc.).
 *
 * ### Thread safety
 * `registerService()` must complete before any `dispatch()` calls.
 * `dispatch()` is safe to call concurrently from multiple threads.
 */
class IGRPCBridge {
public:
    virtual ~IGRPCBridge() = default;

    /**
     * @brief Register a gRPC service and the handler that processes its RPCs.
     *
     * @param service  Descriptor of the service (name, package, methods).
     * @param handler  Handler to invoke for requests targeting this service.
     *                 The bridge does NOT take ownership; the caller must keep
     *                 the handler alive for the bridge's lifetime.
     * @throws std::invalid_argument if a service with `service.service_name`
     *         is already registered.
     * @throws std::logic_error if called after `dispatch()` has already been
     *         invoked (post-init registration is not permitted).
     */
    virtual void registerService(ServiceDescriptor service, IHttpHandler& handler) = 0;

    /**
     * @brief Dispatch an inbound gRPC request to the registered handler.
     *
     * Converts `request` into an `HttpRequest`, finds the handler registered
     * for `request.service_name`, invokes `IHttpHandler::handle()`, and
     * returns the result.
     *
     * Returns an error if `request.service_name` is not registered.
     *
     * @param request  Inbound gRPC call.
     * @return `Result<HttpResponse>` — the handler's response, or an error
     *         describing why the dispatch failed (unknown service, handler
     *         error, etc.).
     */
    virtual themis::Result<HttpResponse> dispatch(const GRPCRequest& request) = 0;

    /**
     * @brief Return descriptors of all registered services.
     */
    virtual std::vector<ServiceDescriptor> registeredServices() const = 0;
};

} // namespace api
} // namespace themis
