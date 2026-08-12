// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Unit tests for CdnCacheMiddleware:
//  - Default policy (no policy registered) → no-store
//  - Route policy registration and longest-prefix matching
//  - Write method override → no-store
//  - Server error override → no-store
//  - Governance X-Themis-Cache: disabled override → no-store
//  - Public cacheable policy (max-age, s-maxage)
//  - Private cacheable policy (browser cache only)
//  - stale-while-revalidate / stale-if-error directives
//  - CDN-Cache-Control header (Cloudflare / Fastly)
//  - Surrogate-Control header (Varnish)
//  - Surrogate-Key / Cache-Tag headers
//  - ETag generation (GET responses only)
//  - Conditional request handling (If-None-Match → 304)
//  - buildCacheControlValue helper
//  - generateETag determinism and format

#include <gtest/gtest.h>
#include "server/cdn_cache_middleware.h"
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;
using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static http::request<http::string_body> makeGet(const std::string& target) {
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "localhost");
    return req;
}

static http::request<http::string_body> makePost(const std::string& target,
                                                  const std::string& body = "{}") {
    http::request<http::string_body> req{http::verb::post, target, 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::content_type, "application/json");
    req.body() = body;
    req.prepare_payload();
    return req;
}

static http::response<http::string_body> makeOkResponse(const std::string& body = R"({"ok":true})") {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::content_type, "application/json");
    res.body() = body;
    res.prepare_payload();
    return res;
}

static http::response<http::string_body> makeResponse(http::status status,
                                                       const std::string& body = "") {
    http::response<http::string_body> res{status, 11};
    res.body() = body;
    res.prepare_payload();
    return res;
}

static std::string getHeader(const http::response<http::string_body>& res,
                              http::field field) {
    auto it = res.find(field);
    return (it != res.end()) ? std::string(it->value()) : "";
}

static std::string getHeader(const http::response<http::string_body>& res,
                              const std::string& name) {
    auto it = res.find(name);
    return (it != res.end()) ? std::string(it->value()) : "";
}

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class CdnCacheMiddlewareTest : public ::testing::Test {
protected:
    CdnCacheMiddleware cdn;
};

// ─────────────────────────────────────────────────────────────────────────────
// Default behaviour (no policy registered)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, NoPolicy_GET_GetsNoStore) {
    auto req = makeGet("/entities/123");
    auto res = makeOkResponse();
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, NoPolicy_POST_GetsNoStore) {
    auto req = makePost("/entities");
    auto res = makeOkResponse();
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

// ─────────────────────────────────────────────────────────────────────────────
// Write method override
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, WriteMethod_POST_AlwaysNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/entities/", pol);

    auto req = makePost("/entities/");
    auto res = makeOkResponse();
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, WriteMethod_PUT_AlwaysNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/", pol);

    http::request<http::string_body> req{http::verb::put, "/entities/abc", 11};
    auto res = makeOkResponse();
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, WriteMethod_DELETE_AlwaysNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/", pol);

    http::request<http::string_body> req{http::verb::delete_, "/entities/abc", 11};
    auto res = makeOkResponse();
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, WriteMethod_PATCH_AlwaysNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/", pol);

    http::request<http::string_body> req{http::verb::patch, "/entities/abc", 11};
    auto res = makeOkResponse();
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

// ─────────────────────────────────────────────────────────────────────────────
// Server error override
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, ServerError_500_GetsNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/", pol);

    auto req = makeGet("/health");
    auto res = makeResponse(http::status::internal_server_error, "error");
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, ServerError_503_GetsNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/", pol);

    auto req = makeGet("/health");
    auto res = makeResponse(http::status::service_unavailable, "unavailable");
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, ClientError_404_UsesPolicy) {
    // 4xx errors are NOT overridden to no-store (only 5xx)
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::NO_STORE;
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/missing");
    auto res = makeResponse(http::status::not_found, "not found");
    cdn.apply(req, res);
    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

// ─────────────────────────────────────────────────────────────────────────────
// Governance override
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, GovernanceDisabled_OverridesToNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/123");
    auto res = makeOkResponse();
    res.set("X-Themis-Cache", "disabled");  // governance says no-cache
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, GovernanceAllowed_UsesPolicy) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/123");
    auto res = makeOkResponse();
    res.set("X-Themis-Cache", "allowed");
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "public, max-age=300");
}

// ─────────────────────────────────────────────────────────────────────────────
// Cache directive rendering
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, PublicPolicy_RendersCorrectly) {
    CdnRoutePolicy pol;
    pol.directive           = CacheDirective::PUBLIC;
    pol.max_age_seconds     = 300;
    pol.cdn_max_age_seconds = 600;
    cdn.registerPolicy("/public/", pol);

    auto req = makeGet("/public/data");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control),
              "public, max-age=300, s-maxage=600");
}

TEST_F(CdnCacheMiddlewareTest, PrivatePolicy_RendersCorrectly) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PRIVATE;
    pol.max_age_seconds = 60;
    cdn.registerPolicy("/user/", pol);

    auto req = makeGet("/user/profile");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "private, max-age=60");
}

TEST_F(CdnCacheMiddlewareTest, NoCachePolicy_RendersCorrectly) {
    CdnRoutePolicy pol;
    pol.directive = CacheDirective::NO_CACHE;
    cdn.registerPolicy("/query/", pol);

    auto req = makeGet("/query/run");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-cache");
}

TEST_F(CdnCacheMiddlewareTest, NoStorePolicy_RendersCorrectly) {
    CdnRoutePolicy pol;
    pol.directive = CacheDirective::NO_STORE;
    cdn.registerPolicy("/secure/", pol);

    auto req = makeGet("/secure/key");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, StaleDirectives_Appended) {
    CdnRoutePolicy pol;
    pol.directive                    = CacheDirective::PUBLIC;
    pol.max_age_seconds              = 300;
    pol.stale_while_revalidate_seconds = 60;
    pol.stale_if_error_seconds       = 86400;
    cdn.registerPolicy("/assets/", pol);

    auto req = makeGet("/assets/logo.png");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    std::string cc = getHeader(res, http::field::cache_control);
    EXPECT_NE(cc.find("stale-while-revalidate=60"), std::string::npos);
    EXPECT_NE(cc.find("stale-if-error=86400"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Longest-prefix matching
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, LongestPrefixMatch) {
    CdnRoutePolicy generic;
    generic.directive       = CacheDirective::PUBLIC;
    generic.max_age_seconds = 60;
    cdn.registerPolicy("/entities/", generic);

    CdnRoutePolicy specific;
    specific.directive       = CacheDirective::PRIVATE;
    specific.max_age_seconds = 10;
    cdn.registerPolicy("/entities/secret/", specific);

    auto req1 = makeGet("/entities/abc");
    auto res1 = makeOkResponse();
    cdn.apply(req1, res1);
    EXPECT_EQ(getHeader(res1, http::field::cache_control), "public, max-age=60");

    auto req2 = makeGet("/entities/secret/key");
    auto res2 = makeOkResponse();
    cdn.apply(req2, res2);
    EXPECT_EQ(getHeader(res2, http::field::cache_control), "private, max-age=10");
}

// ─────────────────────────────────────────────────────────────────────────────
// CDN-Cache-Control header
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, CdnCacheControl_EmittedForPublic) {
    CdnRoutePolicy pol;
    pol.directive              = CacheDirective::PUBLIC;
    pol.max_age_seconds        = 300;
    pol.cdn_max_age_seconds    = 600;
    pol.emit_cdn_cache_control = true;
    cdn.registerPolicy("/public/", pol);

    auto req = makeGet("/public/data");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    // CDN-Cache-Control should be present
    EXPECT_FALSE(getHeader(res, "CDN-Cache-Control").empty());
}

TEST_F(CdnCacheMiddlewareTest, CdnCacheControl_NotEmitted_WhenDisabled) {
    CdnRoutePolicy pol;
    pol.directive              = CacheDirective::PUBLIC;
    pol.max_age_seconds        = 300;
    pol.emit_cdn_cache_control = false;
    cdn.registerPolicy("/public/", pol);

    auto req = makeGet("/public/data");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_TRUE(getHeader(res, "CDN-Cache-Control").empty());
}

TEST_F(CdnCacheMiddlewareTest, CdnCacheControl_PrivateRoute_CDNOverride) {
    // PRIVATE directive with cdn_max_age → CDN-Cache-Control uses public
    CdnRoutePolicy pol;
    pol.directive              = CacheDirective::PRIVATE;
    pol.max_age_seconds        = 60;
    pol.cdn_max_age_seconds    = 300;
    pol.emit_cdn_cache_control = true;
    cdn.registerPolicy("/user/", pol);

    auto req = makeGet("/user/profile");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    // Browser sees private
    EXPECT_EQ(getHeader(res, http::field::cache_control), "private, max-age=60");
    // CDN sees public with longer TTL
    std::string cdn_cc = getHeader(res, "CDN-Cache-Control");
    EXPECT_NE(cdn_cc.find("public"), std::string::npos);
    EXPECT_NE(cdn_cc.find("s-maxage=300"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Surrogate-Control header
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, SurrogateControl_EmittedWhenEnabled) {
    CdnRoutePolicy pol;
    pol.directive              = CacheDirective::PUBLIC;
    pol.max_age_seconds        = 300;
    pol.cdn_max_age_seconds    = 600;
    pol.emit_surrogate_control = true;
    cdn.registerPolicy("/assets/", pol);

    auto req = makeGet("/assets/image");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, "Surrogate-Control"), "max-age=600");
}

TEST_F(CdnCacheMiddlewareTest, SurrogateControl_NotEmitted_WhenDisabled) {
    CdnRoutePolicy pol;
    pol.directive              = CacheDirective::PUBLIC;
    pol.max_age_seconds        = 300;
    pol.cdn_max_age_seconds    = 600;
    pol.emit_surrogate_control = false;
    cdn.registerPolicy("/assets/", pol);

    auto req = makeGet("/assets/image");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_TRUE(getHeader(res, "Surrogate-Control").empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Surrogate-Key / Cache-Tag headers
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, SurrogateKey_EmittedWhenConfigured) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    pol.surrogate_keys  = "entities schema";
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/123");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, "Surrogate-Key"), "entities schema");
    EXPECT_EQ(getHeader(res, "Cache-Tag"), "entities schema");
}

TEST_F(CdnCacheMiddlewareTest, SurrogateKey_NotEmitted_ForWriteMethod) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    pol.surrogate_keys  = "entities";
    cdn.registerPolicy("/entities/", pol);

    auto req = makePost("/entities/");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_TRUE(getHeader(res, "Surrogate-Key").empty());
    EXPECT_TRUE(getHeader(res, "Cache-Tag").empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ETag generation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, ETag_GeneratedForGETWithBody) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    pol.enable_etag     = true;
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/123");
    auto res = makeOkResponse(R"({"id":"123","name":"Alice"})");
    cdn.apply(req, res);

    std::string etag = getHeader(res, http::field::etag);
    EXPECT_FALSE(etag.empty());
    EXPECT_EQ(etag.substr(0, 3), "W/\"");
    EXPECT_EQ(etag.back(), '"');
}

TEST_F(CdnCacheMiddlewareTest, ETag_NotGeneratedForPOST) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    pol.enable_etag     = true;
    cdn.registerPolicy("/entities/", pol);

    auto req = makePost("/entities/");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_TRUE(getHeader(res, http::field::etag).empty());
}

TEST_F(CdnCacheMiddlewareTest, ETag_NotGeneratedWhenDisabled) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    pol.enable_etag     = false;
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/123");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_TRUE(getHeader(res, http::field::etag).empty());
}

TEST_F(CdnCacheMiddlewareTest, ETag_Deterministic) {
    const std::string body = R"({"id":"123"})";
    std::string e1 = CdnCacheMiddleware::generateETag(body);
    std::string e2 = CdnCacheMiddleware::generateETag(body);
    EXPECT_EQ(e1, e2);
}

TEST_F(CdnCacheMiddlewareTest, ETag_DiffersForDifferentBodies) {
    std::string e1 = CdnCacheMiddleware::generateETag(R"({"id":"1"})");
    std::string e2 = CdnCacheMiddleware::generateETag(R"({"id":"2"})");
    EXPECT_NE(e1, e2);
}

TEST_F(CdnCacheMiddlewareTest, ETag_WeakFormat) {
    std::string etag = CdnCacheMiddleware::generateETag("hello world");
    EXPECT_EQ(etag.substr(0, 2), "W/");
    EXPECT_EQ(etag[2], '"');
    EXPECT_EQ(etag.back(), '"');
}

// ─────────────────────────────────────────────────────────────────────────────
// Conditional request handling (If-None-Match)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, ConditionalRequest_MatchingETag_ReturnsTrue) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    pol.enable_etag     = true;
    cdn.registerPolicy("/entities/", pol);

    const std::string body = R"({"id":"123"})";

    // Step 1: apply headers to get ETag
    auto req1 = makeGet("/entities/123");
    auto res1 = makeOkResponse(body);
    cdn.apply(req1, res1);
    std::string etag = getHeader(res1, http::field::etag);
    ASSERT_FALSE(etag.empty());

    // Step 2: conditional request with the same ETag
    auto req2 = makeGet("/entities/123");
    req2.set(http::field::if_none_match, etag);
    EXPECT_TRUE(cdn.checkConditional(req2, res1));
}

TEST_F(CdnCacheMiddlewareTest, ConditionalRequest_WildcardETag_ReturnsTrue) {
    auto req = makeGet("/entities/123");
    req.set(http::field::if_none_match, "*");
    auto res = makeOkResponse();
    res.set(http::field::etag, "W/\"abc123\"");
    EXPECT_TRUE(cdn.checkConditional(req, res));
}

TEST_F(CdnCacheMiddlewareTest, ConditionalRequest_NonMatchingETag_ReturnsFalse) {
    auto req = makeGet("/entities/123");
    req.set(http::field::if_none_match, "W/\"outdated_etag\"");
    auto res = makeOkResponse();
    res.set(http::field::etag, "W/\"current_etag\"");
    EXPECT_FALSE(cdn.checkConditional(req, res));
}

TEST_F(CdnCacheMiddlewareTest, ConditionalRequest_NoIfNoneMatch_ReturnsFalse) {
    auto req = makeGet("/entities/123");
    auto res = makeOkResponse();
    res.set(http::field::etag, "W/\"abc123\"");
    EXPECT_FALSE(cdn.checkConditional(req, res));
}

TEST_F(CdnCacheMiddlewareTest, ConditionalRequest_PostMethod_ReturnsFalse) {
    auto req = makePost("/entities/");
    req.set(http::field::if_none_match, "*");
    auto res = makeOkResponse();
    res.set(http::field::etag, "W/\"abc123\"");
    EXPECT_FALSE(cdn.checkConditional(req, res));
}

// ─────────────────────────────────────────────────────────────────────────────
// buildCacheControlValue helper
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, BuildCacheControl_WriteMethodAlwaysNoStore) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 300;
    EXPECT_EQ(CdnCacheMiddleware::buildCacheControlValue(pol, /*is_write=*/true),
              "no-store");
}

TEST_F(CdnCacheMiddlewareTest, BuildCacheControl_NoStoreDirective) {
    CdnRoutePolicy pol;
    pol.directive = CacheDirective::NO_STORE;
    EXPECT_EQ(CdnCacheMiddleware::buildCacheControlValue(pol, false), "no-store");
}

TEST_F(CdnCacheMiddlewareTest, BuildCacheControl_NoCacheDirective) {
    CdnRoutePolicy pol;
    pol.directive = CacheDirective::NO_CACHE;
    EXPECT_EQ(CdnCacheMiddleware::buildCacheControlValue(pol, false), "no-cache");
}

TEST_F(CdnCacheMiddlewareTest, BuildCacheControl_PublicWithMaxAge) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 3600;
    EXPECT_EQ(CdnCacheMiddleware::buildCacheControlValue(pol, false),
              "public, max-age=3600");
}

TEST_F(CdnCacheMiddlewareTest, BuildCacheControl_PrivateSMaxAgeNotIncluded) {
    // s-maxage is only emitted for PUBLIC routes in Cache-Control
    CdnRoutePolicy pol;
    pol.directive           = CacheDirective::PRIVATE;
    pol.max_age_seconds     = 60;
    pol.cdn_max_age_seconds = 300;
    std::string cc = CdnCacheMiddleware::buildCacheControlValue(pol, false);
    EXPECT_NE(cc.find("private"), std::string::npos);
    EXPECT_EQ(cc.find("s-maxage"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query string is stripped for policy matching
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, QueryStringStripped_PolicyMatchesPath) {
    CdnRoutePolicy pol;
    pol.directive       = CacheDirective::PUBLIC;
    pol.max_age_seconds = 120;
    cdn.registerPolicy("/entities/", pol);

    auto req = makeGet("/entities/123?include=metadata&format=json");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "public, max-age=120");
}

// ─────────────────────────────────────────────────────────────────────────────
// CdnCacheMiddlewareTest – no CDN-Cache-Control for write methods (no-store)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CdnCacheMiddlewareTest, WriteMethod_NoCdnHeaders) {
    CdnRoutePolicy pol;
    pol.directive              = CacheDirective::PUBLIC;
    pol.max_age_seconds        = 300;
    pol.cdn_max_age_seconds    = 600;
    pol.emit_cdn_cache_control = true;
    pol.emit_surrogate_control = true;
    pol.surrogate_keys         = "entities";
    cdn.registerPolicy("/entities/", pol);

    auto req = makePost("/entities/");
    auto res = makeOkResponse();
    cdn.apply(req, res);

    EXPECT_EQ(getHeader(res, http::field::cache_control), "no-store");
    EXPECT_TRUE(getHeader(res, "CDN-Cache-Control").empty());
    EXPECT_TRUE(getHeader(res, "Surrogate-Control").empty());
    EXPECT_TRUE(getHeader(res, "Surrogate-Key").empty());
    EXPECT_TRUE(getHeader(res, "Cache-Tag").empty());
    EXPECT_TRUE(getHeader(res, http::field::etag).empty());
}
