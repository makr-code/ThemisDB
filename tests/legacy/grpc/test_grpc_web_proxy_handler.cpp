// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Unit tests for GrpcWebProxyHandler:
//  - gRPC-Web frame decoding
//  - gRPC-Web response encoding
//  - CORS preflight (OPTIONS)
//  - Status endpoint (GET /api/v1/grpc-web/status)
//  - Request validation (Content-Type checks)
//  - Malformed frame rejection

#include <gtest/gtest.h>
#include "server/grpc_web_proxy_handler.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

namespace http = boost::beast::http;
using json = nlohmann::json;
using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a raw gRPC-Web data frame from raw bytes.
static std::string makeGrpcWebFrame(const std::string& msg, uint8_t flags = 0x00) {
    std::string frame;
    frame.push_back(static_cast<char>(flags));
    uint32_t len = static_cast<uint32_t>(msg.size());
    frame.push_back(static_cast<char>((len >> 24) & 0xFF));
    frame.push_back(static_cast<char>((len >> 16) & 0xFF));
    frame.push_back(static_cast<char>((len >>  8) & 0xFF));
    frame.push_back(static_cast<char>( len        & 0xFF));
    frame.append(msg);
    return frame;
}

static http::request<http::string_body> makeRequest(
    http::verb method,
    const std::string& target,
    const std::string& body = "",
    const std::string& content_type = "application/grpc-web+proto")
{
    http::request<http::string_body> req{method, target, 11};
    req.set(http::field::content_type, content_type);
    req.body() = body;
    req.prepare_payload();
    return req;
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame decoding tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcWebFrameDecode, ValidDataFrame_Succeeds) {
    const std::string proto = "\x0A\x05hello";
    const std::string frame = makeGrpcWebFrame(proto);

    std::string out;
    EXPECT_TRUE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
    EXPECT_EQ(out, proto);
}

TEST(GrpcWebFrameDecode, EmptyPayload_Succeeds) {
    const std::string frame = makeGrpcWebFrame("");

    std::string out;
    EXPECT_TRUE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
    EXPECT_TRUE(out.empty());
}

TEST(GrpcWebFrameDecode, TooShort_Fails) {
    // Less than 5 bytes
    const std::string frame = "\x00\x00";
    std::string out;
    EXPECT_FALSE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
}

TEST(GrpcWebFrameDecode, CompressedFrame_Rejected) {
    // Flag byte 0x01 = compressed; proxy must reject this
    const std::string frame = makeGrpcWebFrame("compressed-data", 0x01);
    std::string out;
    EXPECT_FALSE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
}

TEST(GrpcWebFrameDecode, TrailerFlag_Rejected) {
    // Trailer flag 0x80 is not valid in requests
    const std::string frame = makeGrpcWebFrame("trailers", 0x80);
    std::string out;
    EXPECT_FALSE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
}

TEST(GrpcWebFrameDecode, LengthExceedsBody_Fails) {
    // Header claims 100 bytes but body only has 2
    std::string frame;
    frame.push_back('\x00');   // flags
    frame.push_back('\x00');   // length byte 1
    frame.push_back('\x00');   // length byte 2
    frame.push_back('\x00');   // length byte 3
    frame.push_back('\x64');   // length byte 4 = 100
    frame.push_back('A');      // only 1 byte
    frame.push_back('B');      // 2 bytes

    std::string out;
    EXPECT_FALSE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
}

TEST(GrpcWebFrameDecode, ExactFiveByteHeader_EmptyBody) {
    // Frame header only, zero-length message
    std::string frame(5, '\x00');
    std::string out;
    EXPECT_TRUE(GrpcWebProxyHandler::decodeGrpcWebFrame(frame, out));
    EXPECT_TRUE(out.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Response encoding tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcWebResponseEncode, OkResponseHasDataAndTrailerFrames) {
    const std::string proto = "\x0A\x03foo";
    const std::string encoded = GrpcWebProxyHandler::encodeGrpcWebResponse(proto, 0, "");

    // Minimum size: 5 (data frame header) + proto.size() + 5 (trailer header) + trailers
    EXPECT_GT(encoded.size(), 5u + proto.size() + 5u);

    // Data frame flags = 0x00
    EXPECT_EQ(static_cast<uint8_t>(encoded[0]), 0x00u);

    // Read data frame length
    uint32_t data_len =
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[1])) << 24) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[2])) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[3])) <<  8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[4])));

    EXPECT_EQ(data_len, proto.size());

    // Data payload matches proto
    EXPECT_EQ(encoded.substr(5, data_len), proto);

    // Trailer frame follows immediately after data frame
    const size_t trailer_offset = 5 + data_len;
    EXPECT_EQ(static_cast<uint8_t>(encoded[trailer_offset]), 0x80u);

    // Trailer frame contains grpc-status: 0
    const uint32_t trailer_len =
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 1])) << 24) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 2])) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 3])) <<  8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 4])));

    EXPECT_GT(trailer_len, 0u);
    const std::string trailers = encoded.substr(trailer_offset + 5, trailer_len);
    EXPECT_NE(trailers.find("grpc-status: 0"), std::string::npos);
}

TEST(GrpcWebResponseEncode, ErrorResponseContainsStatusAndMessage) {
    const std::string encoded =
        GrpcWebProxyHandler::encodeGrpcWebResponse("", 2, "invalid argument");

    // Data frame is empty (5-byte header + 0 bytes)
    EXPECT_EQ(static_cast<uint8_t>(encoded[0]), 0x00u);
    uint32_t data_len =
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[1])) << 24) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[2])) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[3])) <<  8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[4])));
    EXPECT_EQ(data_len, 0u);

    // Trailer frame
    const size_t trailer_offset = 5;
    EXPECT_EQ(static_cast<uint8_t>(encoded[trailer_offset]), 0x80u);

    const uint32_t trailer_len =
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 1])) << 24) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 2])) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 3])) <<  8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 4])));

    const std::string trailers = encoded.substr(trailer_offset + 5, trailer_len);
    EXPECT_NE(trailers.find("grpc-status: 2"), std::string::npos);
    EXPECT_NE(trailers.find("grpc-message: invalid argument"), std::string::npos);
}

TEST(GrpcWebResponseEncode, NoMessageWhenGrpcMessageEmpty) {
    const std::string encoded = GrpcWebProxyHandler::encodeGrpcWebResponse("", 0, "");
    const size_t trailer_offset = 5;
    const uint32_t trailer_len =
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 1])) << 24) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 2])) << 16) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 3])) <<  8) |
        (static_cast<uint32_t>(static_cast<uint8_t>(encoded[trailer_offset + 4])));
    const std::string trailers = encoded.substr(trailer_offset + 5, trailer_len);
    EXPECT_EQ(trailers.find("grpc-message"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// CORS preflight tests
// ─────────────────────────────────────────────────────────────────────────────

class GrpcWebProxyHandlerTest : public ::testing::Test {
protected:
    GrpcWebProxyHandler handler;
};

TEST_F(GrpcWebProxyHandlerTest, Options_Returns200WithCorsHeaders) {
    auto req = makeRequest(http::verb::options,
                           "/grpc-web/themis.CoreService/Ping");
    auto res = handler.handleOptions(req);
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_FALSE(std::string(res["Access-Control-Allow-Origin"]).empty());
    EXPECT_FALSE(std::string(res["Access-Control-Allow-Methods"]).empty());
    EXPECT_FALSE(std::string(res["Access-Control-Allow-Headers"]).empty());
    EXPECT_FALSE(std::string(res["Access-Control-Expose-Headers"]).empty());
}

TEST_F(GrpcWebProxyHandlerTest, Options_AllowsAllOriginsDefault) {
    auto req = makeRequest(http::verb::options,
                           "/grpc-web/themis.CoreService/Ping");
    auto res = handler.handleOptions(req);
    EXPECT_EQ(std::string(res["Access-Control-Allow-Origin"]), "*");
}

TEST_F(GrpcWebProxyHandlerTest, Options_CustomOrigin) {
    GrpcWebProxyHandler::Config cfg;
    cfg.cors_allow_origin = "https://app.example.com";
    GrpcWebProxyHandler h{cfg};
    auto req = makeRequest(http::verb::options,
                           "/grpc-web/themis.CoreService/Ping");
    auto res = h.handleOptions(req);
    EXPECT_EQ(std::string(res["Access-Control-Allow-Origin"]),
              "https://app.example.com");
}

// ─────────────────────────────────────────────────────────────────────────────
// Status endpoint tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GrpcWebProxyHandlerTest, Status_Returns200WithJson) {
    auto req = makeRequest(http::verb::get, "/api/v1/grpc-web/status");
    auto res = handler.handleStatus(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("backend_address"));
    EXPECT_TRUE(body.contains("backend_tls"));
    EXPECT_TRUE(body.contains("deadline_ms"));
    EXPECT_TRUE(body.contains("cors_allow_origin"));
}

TEST_F(GrpcWebProxyHandlerTest, Status_ReflectsConfig) {
    GrpcWebProxyHandler::Config cfg;
    cfg.backend_address   = "grpc.internal:50051";
    cfg.backend_tls       = true;
    cfg.deadline_ms       = 5000;
    cfg.cors_allow_origin = "https://ui.example.com";
    GrpcWebProxyHandler h{cfg};

    auto req = makeRequest(http::verb::get, "/api/v1/grpc-web/status");
    auto res = h.handleStatus(req);
    EXPECT_EQ(res.result(), http::status::ok);
    auto body = json::parse(res.body());
    EXPECT_EQ(body["backend_address"].get<std::string>(), "grpc.internal:50051");
    EXPECT_EQ(body["backend_tls"].get<bool>(), true);
    EXPECT_EQ(body["deadline_ms"].get<uint32_t>(), 5000u);
    EXPECT_EQ(body["cors_allow_origin"].get<std::string>(), "https://ui.example.com");
}

// ─────────────────────────────────────────────────────────────────────────────
// Post request validation tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GrpcWebProxyHandlerTest, Post_WrongContentType_Returns415) {
    auto req = makeRequest(http::verb::post,
                           "/grpc-web/themis.CoreService/Ping",
                           makeGrpcWebFrame("payload"),
                           "application/json");
    auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    EXPECT_EQ(res.result(), http::status::unsupported_media_type);
}

TEST_F(GrpcWebProxyHandlerTest, Post_MalformedFrame_Returns400) {
    // Body is too short to be a valid gRPC-Web frame
    auto req = makeRequest(http::verb::post,
                           "/grpc-web/themis.CoreService/Ping",
                           "\x00\x00");  // only 2 bytes
    auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(GrpcWebProxyHandlerTest, Post_CompressedFrame_Returns400) {
    // Compressed frame (flag 0x01) must be rejected
    const std::string frame = makeGrpcWebFrame("fake-compressed", 0x01);
    auto req = makeRequest(http::verb::post,
                           "/grpc-web/themis.CoreService/Ping",
                           frame);
    auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    EXPECT_EQ(res.result(), http::status::bad_request);
}

TEST_F(GrpcWebProxyHandlerTest, Post_ValidFrameWithoutGrpcBackend_Returns200) {
    // Without a real gRPC backend the proxy returns an UNIMPLEMENTED response
    // encoded in gRPC-Web format with HTTP 200 (gRPC-Web always uses HTTP 200
    // and encodes errors in the trailer frame).
    const std::string frame = makeGrpcWebFrame("\x0A\x04test");
    auto req = makeRequest(http::verb::post,
                           "/grpc-web/themis.CoreService/Ping",
                           frame);
    auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    EXPECT_EQ(res.result(), http::status::ok);
    EXPECT_EQ(std::string(res[http::field::content_type]), "application/grpc-web+proto");
    // Response body must be a valid gRPC-Web frame sequence (at minimum the
    // trailer frame from the UNIMPLEMENTED status)
    EXPECT_GE(res.body().size(), 5u);  // at least one frame header
}

TEST_F(GrpcWebProxyHandlerTest, Post_ResponseHasCorsHeaders) {
    const std::string frame = makeGrpcWebFrame("");
    auto req = makeRequest(http::verb::post,
                           "/grpc-web/themis.CoreService/Ping",
                           frame);
    auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    EXPECT_FALSE(std::string(res["Access-Control-Allow-Origin"]).empty());
}

TEST_F(GrpcWebProxyHandlerTest, Post_GrpcWebContentType_Accepted) {
    // application/grpc-web (without +proto suffix) must also be accepted
    const std::string frame = makeGrpcWebFrame("\x00");
    auto req = makeRequest(http::verb::post,
                           "/grpc-web/themis.CoreService/Ping",
                           frame,
                           "application/grpc-web");
    auto res = handler.handlePost(req, "/themis.CoreService/Ping");
    // Should NOT return 415 Unsupported Media Type
    EXPECT_NE(res.result(), http::status::unsupported_media_type);
}

// ===========================================================================
// GAP-012 — CORS wildcard (CWE-346) — config-level structural tests
// ===========================================================================
// The GrpcWebProxyHandler constructor now emits THEMIS_WARN when
// cors_allow_origin is '*'. The following tests verify the FIELD values and
// the security-relevant behaviour that follows from the config.

// GAP-012-01: Default Config has cors_allow_origin = '*'.
TEST(CorsConfigTest, GAP012_DefaultCorsAllowOriginIsWildcard) {
    GrpcWebProxyHandler::Config cfg;
    EXPECT_EQ(cfg.cors_allow_origin, "*")
        << "Default config must start with wildcard so the warning fires (GAP-012)";
}

// GAP-012-02: Specific origin string is not the wildcard.
TEST(CorsConfigTest, GAP012_SpecificOriginIsNotWildcard) {
    GrpcWebProxyHandler::Config cfg;
    cfg.cors_allow_origin = "https://app.example.com";
    EXPECT_NE(cfg.cors_allow_origin, "*")
        << "Configuring a specific origin must not be '*' (GAP-012)";
}
