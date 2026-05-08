/**
 * @file test_grpc_web_proxy_bridge.cpp
 * @brief Unit tests for GrpcWebProxyHandler injectable BackendInvokeFn bridge (STUB #73).
 *
 * Tests verify the BackendInvokeFn injectable callback slot in the non-gRPC
 * stub path of handlePost():
 *   GWPH-BRIDGE-01  no fn injected → handlePost returns UNIMPLEMENTED (12) in trailer
 *   GWPH-BRIDGE-02  fn injected → fn is called, its response proto is returned
 *   GWPH-BRIDGE-03  fn throws → handlePost returns INTERNAL (13) in trailer
 */

#include <gtest/gtest.h>
#include "server/grpc_web_proxy_handler.h"

#include <boost/beast/http.hpp>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace http = boost::beast::http;
using themis::server::GrpcWebProxyHandler;

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Build a minimal valid gRPC-Web request with a given proto body.
static http::request<http::string_body> makeGrpcWebRequest(
    const std::string& proto_payload,
    const std::string& method_path = "/pkg.Service/Method")
{
    // Encode the proto payload inside a 5-byte gRPC-Web data frame.
    std::string body;
    body.push_back(0x00); // flags: uncompressed data frame
    const uint32_t len = static_cast<uint32_t>(proto_payload.size());
    body.push_back(static_cast<char>((len >> 24) & 0xFF));
    body.push_back(static_cast<char>((len >> 16) & 0xFF));
    body.push_back(static_cast<char>((len >>  8) & 0xFF));
    body.push_back(static_cast<char>( len        & 0xFF));
    body += proto_payload;

    http::request<http::string_body> req{http::verb::post, method_path, 11};
#include <gtest/gtest.h>

#include "server/grpc_web_proxy_handler.h"

namespace http = boost::beast::http;
using namespace themis::server;

namespace {

std::string makeGrpcWebFrame(const std::string& msg)
{
    std::string frame;
    frame.push_back(static_cast<char>(0x00));
    const auto len = static_cast<uint32_t>(msg.size());
    frame.push_back(static_cast<char>((len >> 24) & 0xFF));
    frame.push_back(static_cast<char>((len >> 16) & 0xFF));
    frame.push_back(static_cast<char>((len >> 8) & 0xFF));
    frame.push_back(static_cast<char>(len & 0xFF));
    frame.append(msg);
    return frame;
}

http::request<http::string_body> makeRequest(const std::string& body)
{
    http::request<http::string_body> req{http::verb::post, "/grpc-web/themis.CoreService/Ping", 11};
    req.set(http::field::content_type, "application/grpc-web+proto");
    req.body() = body;
    req.prepare_payload();
    return req;
}

/// Extract grpc-status from a gRPC-Web response trailer frame embedded in the body.
/// Returns -1 if not found.
static int extractGrpcStatus(const std::string& body)
{
    // Scan for trailer frames (flags byte 0x80).
    size_t pos = 0;
    while (pos + 5 <= body.size()) {
        const uint8_t flags = static_cast<uint8_t>(body[pos]);
        const uint32_t frame_len =
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 1])) << 24) |
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 2])) << 16) |
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 3])) <<  8) |
            (static_cast<uint32_t>(static_cast<uint8_t>(body[pos + 4])));
        pos += 5;
        if (pos + frame_len > body.size()) break;

        if (flags == 0x80) {
            const std::string trailer(body.data() + pos, frame_len);
            const std::string marker = "grpc-status:";
            const auto it = trailer.find(marker);
            if (it != std::string::npos) {
                size_t value_start = it + marker.size();
                while (value_start < trailer.size() &&
                       trailer[value_start] == ' ') {
                    ++value_start;
                }
                return std::stoi(trailer.substr(value_start));
            }
        }
        pos += frame_len;
    }
    return -1;
}

// ── Fixture ───────────────────────────────────────────────────────────────────

class GrpcWebProxyBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        GrpcWebProxyHandler::setBackendInvokeFn({});
    }
};

// ── GWPH-BRIDGE-01 ────────────────────────────────────────────────────────────
// With no fn injected the stub returns HTTP 200 with gRPC status UNIMPLEMENTED (12).
TEST_F(GrpcWebProxyBridgeTest, NoFnReturnsUnimplemented) {
    GrpcWebProxyHandler handler;
    auto req = makeGrpcWebRequest("\x00\x01\x02");
    auto res = handler.handlePost(req, "/pkg.Service/Method");

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(extractGrpcStatus(res.body()), 12);
}

// ── GWPH-BRIDGE-02 ────────────────────────────────────────────────────────────
// With fn injected, fn is called and its response is returned (grpc-status 0).
TEST_F(GrpcWebProxyBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    GrpcWebProxyHandler::setBackendInvokeFn(
        [&](const std::string& method,
            const std::string& proto_req,
            int& out_status,
            std::string& out_msg) -> std::string {
            fn_called = true;
            EXPECT_EQ(method, "/pkg.Service/Method");
            EXPECT_EQ(proto_req, "\x0A\x03hey"); // proto_payload from request
            out_status = 0;
            out_msg    = "";
            return "\x0A\x05hello"; // fake proto response
        });

    GrpcWebProxyHandler handler;
    auto req = makeGrpcWebRequest("\x0A\x03hey");
    auto res = handler.handlePost(req, "/pkg.Service/Method");

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_TRUE(fn_called);
    EXPECT_EQ(extractGrpcStatus(res.body()), 0);
}

// ── GWPH-BRIDGE-03 ────────────────────────────────────────────────────────────
// When fn throws, handlePost returns HTTP 200 with grpc-status INTERNAL (13).
TEST_F(GrpcWebProxyBridgeTest, ThrowingFnReturnsInternal) {
    GrpcWebProxyHandler::setBackendInvokeFn(
        [](const std::string&, const std::string&,
           int&, std::string&) -> std::string {
            throw std::runtime_error("simulated backend error");
        });

    GrpcWebProxyHandler handler;
    auto req = makeGrpcWebRequest("\x00");
    auto res = handler.handlePost(req, "/pkg.Service/Method");

    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(extractGrpcStatus(res.body()), 13);
} // namespace

TEST(GrpcWebProxyBridgeTest, InjectedBackendInvokeFunctionIsUsed)
{
    GrpcWebProxyHandler::setBackendInvokeFn(nullptr);

    GrpcWebProxyHandler handler;
    const auto req = makeRequest(makeGrpcWebFrame("\x0A\x04test"));

    GrpcWebProxyHandler::setBackendInvokeFn(
        [](const std::string& method,
           const std::string& request_proto,
           std::string& response_proto,
           int& grpc_status,
           std::string& grpc_message) {
            if (method != "/themis.CoreService/Ping" || request_proto.empty()) {
                grpc_status = 3;
                grpc_message = "invalid argument";
                response_proto.clear();
                return true;
            }
            grpc_status = 0;
            grpc_message.clear();
            response_proto = "\x0A\x02ok";
            return true;
        });

    const auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    EXPECT_EQ(res.result(), http::status::ok);

    std::string decoded;
    ASSERT_TRUE(GrpcWebProxyHandler::decodeGrpcWebFrame(res.body(), decoded));
    EXPECT_EQ(decoded, "\x0A\x02ok");

    GrpcWebProxyHandler::setBackendInvokeFn(nullptr);
}
