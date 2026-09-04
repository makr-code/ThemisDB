/**
 * @file test_grpc_web_proxy_bridge.cpp
 * @brief Unit tests for GrpcWebProxyHandler injectable BackendInvokeFn bridge.
 */

#include <gtest/gtest.h>

#include "server/grpc_web_proxy_handler.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace http = boost::beast::http;
using themis::server::GrpcWebProxyHandler;

namespace {

static http::request<http::string_body> makeGrpcWebRequest(
    const std::string& proto_payload,
    const std::string& method_path = "/pkg.Service/Method")
{
    std::string body = {};
    body.push_back(0x00);

    const uint32_t len = static_cast<uint32_t>(proto_payload.size());
    body.push_back(static_cast<char>((len >> 24) & 0xFF));
    body.push_back(static_cast<char>((len >> 16) & 0xFF));
    body.push_back(static_cast<char>((len >> 8) & 0xFF));
    body.push_back(static_cast<char>(len & 0xFF));
    body += proto_payload;

    http::request<http::string_body> req{http::verb::post, method_path, 11};
    req.set(http::field::content_type, "application/grpc-web+proto");
    req.body() = body;
    req.prepare_payload();
    return req;
}

static int extractGrpcStatus(const std::string& body)
{
    size_t pos = 0;
    while (pos + 5 <= body.size()) {
        const uint8_t flags = static_cast<uint8_t>(body[pos]);
        const uint32_t frame_len =
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 1])) << 24) |
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 2])) << 16) |
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 3])) << 8) |
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 4])));

        pos += 5;
        if (pos + frame_len > body.size()) {
            break;
        }

        if (flags == 0x80) {
            const std::string trailer(body.data() + pos, frame_len);
            const std::string marker = "grpc-status:";
            const auto it = trailer.find(marker);
            if (it != std::string::npos) {
                size_t value_start = it + marker.size();
                while (value_start < trailer.size() && trailer[value_start] == ' ') {
                    ++value_start;
                }
                return std::stoi(trailer.substr(value_start));
            }
        }

        pos += frame_len;
    }

    return -1;
}

class GrpcWebProxyBridgeTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        GrpcWebProxyHandler::setBackendInvokeFn({});
    }
};

TEST_F(GrpcWebProxyBridgeTest, NoFnReturnsUnimplemented)
{
    GrpcWebProxyHandler handler;
    auto req = makeGrpcWebRequest("\x00\x01\x02");
    auto res = handler.handlePost(req, "/pkg.Service/Method");

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(extractGrpcStatus(res.body()), 12);
}

TEST_F(GrpcWebProxyBridgeTest, InjectedFnIsCalled)
{
    bool fn_called = false;

    GrpcWebProxyHandler::setBackendInvokeFn(
        [&](const std::string& method,
            const std::string& request_proto,
            std::string& response_proto,
            int& grpc_status,
            std::string& grpc_message) -> bool {
            fn_called = true;
            EXPECT_EQ(method, "/pkg.Service/Method");
            EXPECT_EQ(request_proto, "\x0A\x03hey");

            response_proto = "\x0A\x05hello";
            grpc_status = 0;
            grpc_message.clear();
            return true;
        });

    GrpcWebProxyHandler handler;
    auto req = makeGrpcWebRequest("\x0A\x03hey");
    auto res = handler.handlePost(req, "/pkg.Service/Method");

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_TRUE(fn_called);
    EXPECT_EQ(extractGrpcStatus(res.body()), 0);

#ifndef THEMIS_ENABLE_GRPC
    http::request<http::string_body> status_req{http::verb::get, "/api/v1/grpc-web/status", 11};
    auto status_res = handler.handleStatus(status_req);
    EXPECT_EQ(status_res.result(), http::status::ok);
    auto status_body = nlohmann::json::parse(status_res.body());
    EXPECT_TRUE(status_body["requests_supported"].get<bool>());
    EXPECT_EQ(status_body["backend_mode"].get<std::string>(), "override");
#endif
}

TEST_F(GrpcWebProxyBridgeTest, ThrowingFnPropagatesException)
{
    GrpcWebProxyHandler::setBackendInvokeFn(
        [](const std::string&,
           const std::string&,
           std::string&,
           int&,
           std::string&) -> bool {
            throw std::runtime_error("simulated backend error");
        });

    GrpcWebProxyHandler handler;
    auto req = makeGrpcWebRequest("\x00");
    EXPECT_THROW({
        (void)handler.handlePost(req, "/pkg.Service/Method");
    }, std::runtime_error);
}

} // namespace
