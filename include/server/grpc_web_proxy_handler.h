/**
 * @file grpc_web_proxy_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <boost/beast/http.hpp>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>

namespace themis {
namespace server {

namespace beast = boost::beast;
namespace http  = beast::http;

class GrpcWebProxyHandler {
public:
    struct Config {
        std::string backend_address = "localhost:18765";

        std::string cors_allow_origin = "*";

        bool backend_tls = false;

        uint32_t deadline_ms = 30000;
    };

    using BackendInvokeFn = std::function<bool(const std::string& method,
                                               const std::string& request_proto,
                                               std::string& response_proto,
                                               int& grpc_status,
                                               std::string& grpc_message)>;

    /**
     * @brief Set Backend Invoke Fn.
     * @param[in] fn Input parameter.
     */
    static void setBackendInvokeFn(BackendInvokeFn fn);

    GrpcWebProxyHandler();
    /**
     * @brief Grpc Web Proxy Handler.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GrpcWebProxyHandler(Config config);

    ~GrpcWebProxyHandler() = default;

    // Non-copyable, movable
    GrpcWebProxyHandler(const GrpcWebProxyHandler&) = delete;
    GrpcWebProxyHandler& operator=(const GrpcWebProxyHandler&) = delete;
    GrpcWebProxyHandler(GrpcWebProxyHandler&&) noexcept = default;
    GrpcWebProxyHandler& operator=(GrpcWebProxyHandler&&) noexcept = default;

    /**
     * @brief Handle Post.
     * @param[in] req Input parameter.
     * @param[in] method Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handlePost(
        const http::request<http::string_body>& req,
        const std::string& method);

    /**
     * @brief Handle Options.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleOptions(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req);


    /**
     * @brief Decode Grpc Web Frame.
     * @param[in] body Input parameter.
     * @param[in,out] out_msg Input/output parameter.
     * @return True when the operation succeeds.
     */
    static bool decodeGrpcWebFrame(const std::string& body,
                                   std::string& out_msg);

    /**
     * @brief Encode Grpc Web Response.
     * @param[in] proto_msg Input parameter.
     * @param[in] grpc_status Input parameter.
     * @param[in] grpc_message Input parameter.
     * @return Return value.
     */
    static std::string encodeGrpcWebResponse(const std::string& proto_msg,
                                             int grpc_status,
                                             const std::string& grpc_message);

private:
    Config config_;

    // ── gRPC channel (lazily initialised, thread-safe after first use) ───────
    mutable std::shared_ptr<void> channel_holder_; ///< opaque grpc::Channel
    mutable std::shared_ptr<void> stub_holder_;    ///< opaque grpc::GenericStub

    /**
     * @brief Ensure Channel.
     */
    void ensureChannel() const;


    /**
     * @brief Add Cors Headers.
     * @param[in,out] res Input/output parameter.
     */
    void addCorsHeaders(http::response<http::string_body>& res) const;

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] content_type Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const std::string& content_type,
        const http::request<http::string_body>& req) const;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;


    /**
     * @brief Append Frame Header.
     * @param[in,out] out Input/output parameter.
     * @param[in] flags Input parameter.
     * @param[in] length Input parameter.
     */
    static void appendFrameHeader(std::string& out,
                                   uint8_t flags,
                                   uint32_t length);

};

} // namespace server
} // namespace themis

