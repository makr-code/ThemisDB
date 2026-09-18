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

struct ServiceDescriptor {
    std::string service_name = {};

    std::string package; // e.g. "themis.v1"

    std::vector<std::string> method_names;

    std::string serialization_format = "proto";
};

// ---------------------------------------------------------------------------
// GRPCMetadata — propagates gRPC headers into the IHttpHandler invocation
// ---------------------------------------------------------------------------

struct GRPCMetadata {
    std::string authority;

    std::string method_path;

    std::string content_type = "application/grpc";

    std::string deadline = {};

    std::unordered_map<std::string, std::string> user_metadata;

    bool hasDeadline() const noexcept { return !deadline.empty(); }
};

// ---------------------------------------------------------------------------
// GRPCRequest — a single inbound gRPC call
// ---------------------------------------------------------------------------

struct GRPCRequest {
    GRPCMetadata metadata;

    std::string service_name;

    std::string method_name;

    std::string request_bytes;
};

// ---------------------------------------------------------------------------
// IGRPCBridge — pure-virtual interface for gRPC-to-handler dispatch
// ---------------------------------------------------------------------------

class IGRPCBridge {
public:
    /**
     * @brief IGRPCBridge.
     * @return Return value.
     */
    virtual ~IGRPCBridge() = default;

    /**
     * @brief Register Service.
     * @param[in] service Input parameter.
     * @param[in,out] handler Input/output parameter.
     */
    virtual void registerService(ServiceDescriptor service, IHttpHandler& handler) = 0;

    /**
     * @brief Dispatch.
     * @param[in] request Input parameter.
     * @return Return value.
     */
    virtual themis::Result<HttpResponse> dispatch(const GRPCRequest& request) = 0;

    /**
     * @brief Registered Services.
     * @return Return value.
     */
    virtual std::vector<ServiceDescriptor> registeredServices() const = 0;
};

} // namespace api
} // namespace themis
