/**
 * @file test_api_interfaces.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>

#include "api/http_handler.h"
#include "api/graphql_schema_builder.h"
#include "api/websocket_handler.h"
#include "api/api_version_router.h"
#include "api/correlation_id.h"
#include "api/grpc_bridge.h"

using namespace themis::api;

// ============================================================================
// HttpRequest tests
// ============================================================================

TEST(HttpRequestTest, DefaultConstruct)
{
    HttpRequest req;
    EXPECT_TRUE(req.method.empty());
    EXPECT_TRUE(req.path.empty());
    EXPECT_FALSE(req.hasAuth());
}

TEST(HttpRequestTest, HasAuthFromHeader)
{
    HttpRequest req;
    req.headers["Authorization"] = "Bearer token123";
    EXPECT_TRUE(req.hasAuth());
}

TEST(HttpRequestTest, HeaderLookup)
{
    HttpRequest req;
    req.headers["Content-Type"] = "application/json";
    EXPECT_EQ(req.header("Content-Type"), "application/json");
    EXPECT_TRUE(req.header("X-Missing").empty());
}

// ============================================================================
// HttpResponse factory methods
// ============================================================================

TEST(HttpResponseTest, OkFactory)
{
    auto r = HttpResponse::ok("{\"id\":1}");
    EXPECT_EQ(r.status_code, 200);
    EXPECT_EQ(r.body, "{\"id\":1}");
    EXPECT_EQ(r.headers.at("Content-Type"), "application/json");
}

TEST(HttpResponseTest, CreatedFactory)
{
    auto r = HttpResponse::created("{\"id\":2}");
    EXPECT_EQ(r.status_code, 201);
}

TEST(HttpResponseTest, NoContentFactory)
{
    auto r = HttpResponse::noContent();
    EXPECT_EQ(r.status_code, 204);
    EXPECT_TRUE(r.body.empty());
}

TEST(HttpResponseTest, BadRequestFactory)
{
    auto r = HttpResponse::badRequest("invalid body");
    EXPECT_EQ(r.status_code, 400);
    EXPECT_NE(r.body.find("invalid body"), std::string::npos);
}

TEST(HttpResponseTest, UnauthorizedFactory)
{
    EXPECT_EQ(HttpResponse::unauthorized().status_code, 401);
}

TEST(HttpResponseTest, ForbiddenFactory)
{
    EXPECT_EQ(HttpResponse::forbidden().status_code, 403);
}

TEST(HttpResponseTest, NotFoundFactory)
{
    EXPECT_EQ(HttpResponse::notFound().status_code, 404);
}

TEST(HttpResponseTest, InternalErrorFactory)
{
    auto r = HttpResponse::internalError("oops");
    EXPECT_EQ(r.status_code, 500);
    EXPECT_NE(r.body.find("oops"), std::string::npos);
}

// ============================================================================
// MiddlewareChain tests
// ============================================================================

namespace {

struct FixedResponseHandler : public IHttpHandler {
    HttpResponse response;
    explicit FixedResponseHandler(HttpResponse r) : response(std::move(r)) {}

    themis::Result<HttpResponse> handle(const HttpRequest&) override {
        return response;
    }
    std::string_view handlerName() const noexcept override { return "FixedResponseHandler"; }
    bool requiresAuthentication() const noexcept override { return false; }
};

struct RejectingHandler : public IHttpHandler {
    themis::Result<HttpResponse> handle(const HttpRequest&) override {
        return tl::unexpected(themis::Error(
            themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR,
            "Forbidden by test"));
    }
    std::string_view handlerName() const noexcept override { return "RejectingHandler"; }
    bool requiresAuthentication() const noexcept override { return true; }
};

} // anonymous namespace

TEST(MiddlewareChainTest, EmptyChainReturnsError)
{
    MiddlewareChain chain;
    HttpRequest req;
    auto result = chain.handle(req);
    EXPECT_FALSE(result.has_value());
}

TEST(MiddlewareChainTest, SingleHandlerReturnsResponse)
{
    MiddlewareChain chain;
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok("{}")));
    auto result = chain.handle(HttpRequest{});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().status_code, 200);
}

TEST(MiddlewareChainTest, RejectingFirstHandlerShortCircuits)
{
    MiddlewareChain chain;
    // First handler rejects → second handler must not be called
    chain.append(std::make_shared<RejectingHandler>());
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok("{}")));
    auto result = chain.handle(HttpRequest{});
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("Forbidden by test"), std::string::npos);
}

TEST(MiddlewareChainTest, PassThroughLeadsToTerminalHandlerResponse)
{
    MiddlewareChain chain;
    // First handler (interceptor) passes (200 ok)
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok("pass")));
    // Second (terminal) handler returns the real 201 response
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::created("{\"id\":99}")));
    auto result = chain.handle(HttpRequest{});
    ASSERT_TRUE(result.has_value());
    // Terminal handler's response (201) must be returned, not the interceptor's (200)
    EXPECT_EQ(result.value().status_code, 201);
    EXPECT_EQ(result.value().body, "{\"id\":99}");
}

TEST(MiddlewareChainTest, RequiresAuthIfAnyLinkRequiresIt)
{
    MiddlewareChain chain;
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok()));
    chain.append(std::make_shared<RejectingHandler>());
    EXPECT_TRUE(chain.requiresAuthentication());
}

TEST(MiddlewareChainTest, NoAuthIfAllLinksOptOut)
{
    MiddlewareChain chain;
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok()));
    EXPECT_FALSE(chain.requiresAuthentication());
}

TEST(MiddlewareChainTest, SizeReflectsAppends)
{
    MiddlewareChain chain;
    EXPECT_EQ(chain.size(), 0u);
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok()));
    EXPECT_EQ(chain.size(), 1u);
    chain.append(std::make_shared<FixedResponseHandler>(HttpResponse::ok()));
    EXPECT_EQ(chain.size(), 2u);
}

// ============================================================================
// GraphQL schema builder types
// ============================================================================

TEST(SchemaValidationResultTest, OkResult)
{
    auto r = SchemaValidationResult::ok();
    EXPECT_TRUE(r.valid);
    EXPECT_TRUE(r.errors.empty());
}

TEST(SchemaValidationResultTest, FailResult)
{
    auto r = SchemaValidationResult::fail("Entity", "id", "field type unknown");
    EXPECT_FALSE(r.valid);
    ASSERT_EQ(r.errors.size(), 1u);
    EXPECT_EQ(r.errors[0].type_name,  "Entity");
    EXPECT_EQ(r.errors[0].field_name, "id");
    EXPECT_EQ(r.errors[0].message,    "field type unknown");
}

TEST(GraphQLTypeDescriptorTest, FieldConstruction)
{
    GraphQLFieldDescriptor f;
    f.name        = "id";
    f.type_string = "ID!";
    f.description = "Primary key";
    EXPECT_EQ(f.name,        "id");
    EXPECT_EQ(f.type_string, "ID!");
    EXPECT_FALSE(f.deprecated);
}

TEST(GraphQLTypeDescriptorTest, TypeConstruction)
{
    GraphQLTypeDescriptor t;
    t.name = "Entity";
    t.fields.push_back({"id", "ID!", "PK", false, {}});
    t.fields.push_back({"name", "String!", "Display name", false, {}});
    EXPECT_EQ(t.name, "Entity");
    EXPECT_EQ(t.fields.size(), 2u);
}

// ============================================================================
// WebSocket types
// ============================================================================

TEST(WebSocketCloseCodeTest, Values)
{
    EXPECT_EQ(static_cast<uint16_t>(WebSocketCloseCode::NormalClosure),    1000u);
    EXPECT_EQ(static_cast<uint16_t>(WebSocketCloseCode::GoingAway),        1001u);
    EXPECT_EQ(static_cast<uint16_t>(WebSocketCloseCode::InternalError),    1011u);
    EXPECT_EQ(static_cast<uint16_t>(WebSocketCloseCode::TlsHandshakeFailed), 1015u);
}

TEST(WebSocketFrameTest, TextFactory)
{
    auto f = WebSocketFrame::text("hello");
    EXPECT_EQ(f.type,    WebSocketFrame::Type::Text);
    EXPECT_EQ(f.payload, "hello");
}

TEST(WebSocketFrameTest, BinaryFactory)
{
    auto f = WebSocketFrame::binary("\x01\x02\x03");
    EXPECT_EQ(f.type, WebSocketFrame::Type::Binary);
    EXPECT_EQ(f.payload.size(), 3u);
}

// ============================================================================
// VersionDescriptor
// ============================================================================

TEST(VersionDescriptorTest, CurrentVersion)
{
    auto v = VersionDescriptor::current(1, 4, "v1.4 — Stable");
    EXPECT_EQ(v.major_version, 1);
    EXPECT_EQ(v.minor_version, 4);
    EXPECT_EQ(v.label,         "v1.4 — Stable");
    EXPECT_FALSE(v.deprecation_date.has_value());
}

TEST(VersionDescriptorTest, DeprecatedVersion)
{
    auto v = VersionDescriptor::deprecated(1, 0, "2027-01-01", "2027-07-01",
                                           "https://docs.example.com/migrate");
    EXPECT_EQ(v.major_version, 1);
    EXPECT_TRUE(v.deprecation_date.has_value());
    EXPECT_EQ(v.deprecation_date.value(), "2027-01-01");
    EXPECT_TRUE(v.sunset_date.has_value());
    EXPECT_TRUE(v.successor_url.has_value());
}

// ============================================================================
// CorrelationId
// ============================================================================

TEST(CorrelationIdTest, NilDefault)
{
    CorrelationId id;
    EXPECT_TRUE(id.isNil());
}

TEST(CorrelationIdTest, ParseAndSerialiseRoundTrip)
{
    const std::string uuid = "550e8400-e29b-41d4-a716-446655440000";
    auto id = CorrelationId::parse(uuid);
    EXPECT_FALSE(id.isNil());
    EXPECT_EQ(id.toString(), uuid);
}

TEST(CorrelationIdTest, ParseUppercase)
{
    const std::string upper = "550E8400-E29B-41D4-A716-446655440000";
    const std::string lower = "550e8400-e29b-41d4-a716-446655440000";
    auto id = CorrelationId::parse(upper);
    EXPECT_EQ(id.toString(), lower);
}

TEST(CorrelationIdTest, ParseWithoutDashes)
{
    // 32 hex chars without dashes
    const std::string nodash = "550e8400e29b41d4a716446655440000";
    auto id = CorrelationId::parse(nodash);
    EXPECT_EQ(id.toString(), "550e8400-e29b-41d4-a716-446655440000");
}

TEST(CorrelationIdTest, ParseInvalidThrows)
{
    EXPECT_THROW(CorrelationId::parse("not-a-uuid!"), std::invalid_argument);
}

TEST(CorrelationIdTest, EqualityAndInequality)
{
    auto id1 = CorrelationId::parse("550e8400-e29b-41d4-a716-446655440000");
    auto id2 = CorrelationId::parse("550e8400-e29b-41d4-a716-446655440000");
    auto id3 = CorrelationId::parse("00000000-0000-0000-0000-000000000001");
    EXPECT_EQ(id1,  id2);
    EXPECT_NE(id1,  id3);
}

TEST(CorrelationIdTest, Hashable)
{
    std::unordered_map<CorrelationId, std::string> map;
    auto id = CorrelationId::parse("550e8400-e29b-41d4-a716-446655440000");
    map[id] = "test";
    EXPECT_EQ(map.at(id), "test");
}

TEST(CorrelationIdTest, ByteSize)
{
    CorrelationId id;
    EXPECT_EQ(id.bytes().size(), 16u);
}

// ============================================================================
// ServiceDescriptor / GRPCMetadata
// ============================================================================

TEST(ServiceDescriptorTest, DefaultSerialization)
{
    ServiceDescriptor sd;
    sd.service_name = "themis.v1.ThemisDB";
    sd.package      = "themis.v1";
    sd.method_names = {"GetEntity", "ExecuteAQL"};
    EXPECT_EQ(sd.serialization_format, "proto");
    EXPECT_EQ(sd.method_names.size(), 2u);
}

TEST(GRPCMetadataTest, DeadlineDetection)
{
    GRPCMetadata m;
    EXPECT_FALSE(m.hasDeadline());
    m.deadline = "2026-03-10T17:00:00Z";
    EXPECT_TRUE(m.hasDeadline());
}

TEST(GRPCMetadataTest, UserMetadata)
{
    GRPCMetadata m;
    m.user_metadata["x-tenant-id"] = "acme";
    EXPECT_EQ(m.user_metadata.at("x-tenant-id"), "acme");
}
