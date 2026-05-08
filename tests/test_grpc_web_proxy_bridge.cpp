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
