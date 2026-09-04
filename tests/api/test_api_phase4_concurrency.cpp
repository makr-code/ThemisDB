/**
 * @file test_api_phase4_concurrency.cpp
 * @brief Phase 4: Focused regressions for high-concurrency and transport-edge scenarios.
 *
 * @version 0.0.1
 * @note Status: Phase 4 Q4 2026 — concurrency and transport-edge validation.
 * @note Validates ROADMAP.md Phase 4 items:
 *       - "expand focused regressions for high-concurrency and transport-edge scenarios"
 *       - "extend deterministic integration matrix coverage for protocol combinations"
 * @note Validates FUTURE_ENHANCEMENTS.md § Test Strategy:
 *       - concurrency and load tests for queueing, parsing, and session handling paths
 *       - deterministic fixture tests for degraded/unsupported capability modes
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>
#include <algorithm>
#include <numeric>

#include "api/http_handler.h"
#include "api/api_transport_contracts.h"
#include "api/api_error_taxonomy.h"
#include "api/api_transport_policy.h"

using namespace themis::api;
using namespace testing;

// ============================================================================
// Test adapter — deterministic responses driven by a per-request predicate
// ============================================================================

namespace {

/**
 * @brief Test adapter that deterministically produces successes or errors based
 *        on a per-request index, enabling matrix-style test verification.
 */
class DeterministicTestAdapter : public IHttpHandler {
public:
    explicit DeterministicTestAdapter(std::size_t fail_every_nth = 0)
        : fail_every_nth_(fail_every_nth) {}

    themis::Result<HttpResponse> handle(const HttpRequest& req) override {
        const std::size_t idx = counter_.fetch_add(1, std::memory_order_relaxed);

        if (fail_every_nth_ > 0 && (idx % fail_every_nth_ == 0)) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR,
                "deterministic-simulated-failure at index " + std::to_string(idx)));
        }

        HttpResponse resp;
        resp.status_code = 200;
        resp.body = "{\"index\":" + std::to_string(idx) + "}";
        resp.headers["Content-Type"] = "application/json";
        resp.headers["X-Request-ID"] =
            req.correlation_id.empty() ? "no-id" : req.correlation_id;
        return resp;
    }

    [[nodiscard]] std::string_view handlerName() const noexcept override {
        return "DeterministicTestAdapter";
    }

    [[nodiscard]] bool requiresAuthentication() const noexcept override {
        return false;
    }

    [[nodiscard]] std::size_t callCount() const noexcept {
        return counter_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::size_t> counter_{0};
    std::size_t              fail_every_nth_;
};

} // anonymous namespace

// ============================================================================
// Phase 4-A: High-concurrency correctness matrix
// ============================================================================

/**
 * @test 32 threads × 50 requests — all succeed through policy middleware
 *
 * Validates that TransportPolicyMiddleware produces no spurious failures under
 * high-concurrency load when all requests are well-formed.
 */
TEST(Phase4ConcurrencyTest, PolicyMiddlewareHighConcurrency32x50)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>(0 /* no forced failures */);
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    constexpr int kThreads  = 32;
    constexpr int kRequests = 50;
    std::atomic<int> successes{0};
    std::atomic<int> failures{0};

    auto worker = [&]() {
        for (int i = 0; i < kRequests; ++i) {
            std::ostringstream oss = {};
            oss << std::this_thread::get_id() << "-" << i;

            HttpRequest req;
            req.method         = "GET";
            req.path           = "/api/v2/entities";
            req.correlation_id = oss.str();

            auto result = policy->handle(req);
            if (result.has_value()) {
                successes.fetch_add(1, std::memory_order_relaxed);
            } else {
                failures.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(successes.load(), kThreads * kRequests);
    EXPECT_EQ(failures.load(), 0);
    EXPECT_EQ(inner->callCount(), static_cast<std::size_t>(kThreads * kRequests));
}

/**
 * @test Policy rejects all threads uniformly on a shared invalid version
 *
 * Ensures that all 16 concurrent threads get a well-formed UnsupportedVersion
 * error; no thread slips through with the bad X-API-Version header.
 */
TEST(Phase4ConcurrencyTest, PolicyRejectsInvalidVersionAcrossAllThreads)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    constexpr int kThreads  = 16;
    constexpr int kRequests = 20;
    std::atomic<int> expected_failures{0};
    std::atomic<int> unexpected{0};

    auto worker = [&]() {
        for (int i = 0; i < kRequests; ++i) {
            HttpRequest req;
            req.method                    = "GET";
            req.path                      = "/api/v99/entities";
            req.headers["X-API-Version"]  = "v99";

            auto result = policy->handle(req);
            if (!result.has_value()
                && result.error().code()
                       == themis::errors::ErrorCode::ERR_API_INVALID_REQUEST) {
                expected_failures.fetch_add(1, std::memory_order_relaxed);
            } else {
                unexpected.fetch_add(1, std::memory_order_relaxed);
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back(worker);
    }
    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(expected_failures.load(), kThreads * kRequests);
    EXPECT_EQ(unexpected.load(), 0);
    // Inner handler must NOT have been called — fail-closed at policy layer.
    EXPECT_EQ(inner->callCount(), 0u);
}

// ============================================================================
// Phase 4-B: Transport-edge scenarios (malformed / boundary inputs)
// ============================================================================

/**
 * @test Exact-boundary payload: kMaxPayloadBytes accepted, kMaxPayloadBytes+1 rejected
 */
TEST(Phase4EdgeCasesTest, PayloadBoundaryExactLimit)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    // Exactly at the limit — must succeed.
    {
        HttpRequest req;
        req.method                  = "POST";
        req.path                    = "/api/v1/data";
        req.headers["Content-Type"] = "application/octet-stream";
        req.body                    = std::string(kMaxPayloadBytes, 'x');

        auto result = policy->handle(req);
        EXPECT_TRUE(result.has_value()) << "Payload at limit must be accepted";
    }

    // One byte over the limit — must fail.
    {
        HttpRequest req;
        req.method                  = "POST";
        req.path                    = "/api/v1/data";
        req.headers["Content-Type"] = "application/octet-stream";
        req.body                    = std::string(kMaxPayloadBytes + 1, 'x');

        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value()) << "Payload over limit must be rejected";
        EXPECT_EQ(result.error().code(),
                  themis::errors::ErrorCode::ERR_API_INVALID_REQUEST);
        EXPECT_THAT(result.error().message(),
                    HasSubstr("ERR_TRANSPORT_PAYLOAD_TOO_LARGE"));
    }
}

/**
 * @test Path length boundary: kMaxPathBytes accepted, kMaxPathBytes+1 rejected
 */
TEST(Phase4EdgeCasesTest, PathLengthBoundary)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    const std::string base_path = "/api/v1/";
    const std::string filler(kMaxPathBytes - base_path.size(), 'a');

    // Exact limit.
    {
        HttpRequest req;
        req.method = "GET";
        req.path   = base_path + filler;
        ASSERT_EQ(req.path.size(), kMaxPathBytes);

        auto result = policy->handle(req);
        EXPECT_TRUE(result.has_value()) << "Path at limit must be accepted";
    }

    // One byte over.
    {
        HttpRequest req;
        req.method = "GET";
        req.path   = base_path + filler + "x";
        ASSERT_GT(req.path.size(), kMaxPathBytes);

        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value()) << "Path over limit must be rejected";
        EXPECT_EQ(result.error().code(),
                  themis::errors::ErrorCode::ERR_API_INVALID_REQUEST);
    }
}

/**
 * @test Empty method is rejected; empty path is rejected; both empty is rejected
 */
TEST(Phase4EdgeCasesTest, MalformedRequestVariants)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    const struct {
        std::string method = {};
        std::string path = {};
        const char* label;
    } cases[] = {
        {"",    "/api/v1/x", "empty method"},
        {"GET", "",           "empty path"},
        {"",    "",           "empty method and path"},
    };

    for (const auto& c : cases) {
        HttpRequest req;
        req.method = c.method;
        req.path   = c.path;

        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value()) << c.label << " must be rejected";
        EXPECT_EQ(result.error().code(),
                  themis::errors::ErrorCode::ERR_API_INVALID_REQUEST)
            << c.label;
        EXPECT_THAT(result.error().message(),
                    HasSubstr("ERR_TRANSPORT_MALFORMED_REQUEST"))
            << c.label;
    }
}

/**
 * @test Content-Type enforcement: POST with body and no Content-Type is rejected
 */
TEST(Phase4EdgeCasesTest, PostWithBodyRequiresContentType)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    // POST with body but no Content-Type → rejected.
    {
        HttpRequest req;
        req.method = "POST";
        req.path   = "/api/v1/entities";
        req.body   = "{\"key\":\"value\"}";

        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().code(),
                  themis::errors::ErrorCode::ERR_API_INVALID_REQUEST);
        EXPECT_THAT(result.error().message(),
                    HasSubstr("ERR_TRANSPORT_CONTENT_TYPE_MISSING"));
    }

    // POST with body and Content-Type → accepted.
    {
        HttpRequest req;
        req.method                  = "POST";
        req.path                    = "/api/v1/entities";
        req.body                    = "{\"key\":\"value\"}";
        req.headers["Content-Type"] = "application/json";

        auto result = policy->handle(req);
        EXPECT_TRUE(result.has_value());
    }

    // POST with empty body and no Content-Type → accepted (no body → no requirement).
    {
        HttpRequest req;
        req.method = "POST";
        req.path   = "/api/v1/entities";
        req.body   = "";

        auto result = policy->handle(req);
        EXPECT_TRUE(result.has_value());
    }
}

/**
 * @test PUT and PATCH also require Content-Type when carrying a body
 */
TEST(Phase4EdgeCasesTest, PutAndPatchWithBodyRequireContentType)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    for (const char* method : {"PUT", "PATCH"}) {
        HttpRequest req;
        req.method = method;
        req.path   = "/api/v1/entities/42";
        req.body   = "{\"field\":\"value\"}";

        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value()) << method << " with body needs Content-Type";
        EXPECT_THAT(result.error().message(),
                    HasSubstr("ERR_TRANSPORT_CONTENT_TYPE_MISSING"));
    }
}

/**
 * @test GET / DELETE / HEAD / OPTIONS do not require Content-Type even with a body
 */
TEST(Phase4EdgeCasesTest, ReadMethodsNeverRequireContentType)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    for (const char* method : {"GET", "DELETE", "HEAD", "OPTIONS"}) {
        HttpRequest req;
        req.method = method;
        req.path   = "/api/v1/entities";
        req.body   = "body-should-not-trigger-content-type-check";

        auto result = policy->handle(req);
        EXPECT_TRUE(result.has_value())
            << method << " must not require Content-Type";
    }
}

// ============================================================================
// Phase 4-C: Deterministic integration matrix (method × version × payload)
// ============================================================================

/**
 * @test Protocol combination matrix — all valid combinations succeed
 *
 * Tests 6 HTTP methods × 3 version states × 2 payload scenarios = 36 combinations.
 */
TEST(Phase4MatrixTest, ValidProtocolCombinationsSucceed)
{
    auto inner  = std::make_shared<DeterministicTestAdapter>();
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    const std::string versions[]      = {"", "v1", "v2"};
    const std::string methods[]       = {"GET", "POST", "PUT", "PATCH", "DELETE", "HEAD"};
    const std::string content_types[] = {"application/json", "application/octet-stream"};

    int accepted = 0;
    int rejected = 0;

    for (const auto& method : methods) {
        for (const auto& version : versions) {
            const bool needs_ct = (method == "POST" || method == "PUT" || method == "PATCH");
            for (const auto& ct : content_types) {
                HttpRequest req;
                req.method = method;
                req.path   = "/api/v1/entities";
                req.body   = needs_ct ? "{}" : "";

                if (!version.empty()) {
                    req.headers["X-API-Version"] = version;
                }
                if (needs_ct && !req.body.empty()) {
                    req.headers["Content-Type"] = ct;
                }

                auto result = policy->handle(req);
                if (result.has_value()) {
                    ++accepted;
                } else {
                    ++rejected;
                }
            }
        }
    }

    // All combinations in this matrix are valid — zero rejections expected.
    EXPECT_EQ(rejected, 0)
        << "Valid protocol combination matrix must produce zero policy rejections";
    EXPECT_GT(accepted, 0);
}

// ============================================================================
// Phase 4-D: API error taxonomy correctness
// ============================================================================

/**
 * @test ApiErrorTaxonomy maps all failure classes to correct HTTP status codes
 */
TEST(Phase4TaxonomyTest, AllFailureClassesHaveCorrectHttpStatus)
{
    using FC = TransportFailureClass;
    const struct {
        FC fc;
        int expected_status;
    } cases[] = {
        {FC::None,                  200},
        {FC::MalformedRequest,      400},
        {FC::UnsupportedVersion,    400},
        {FC::Unauthorized,          401},
        {FC::PayloadTooLarge,       413},
        {FC::ContentTypeMissing,    415},
        {FC::ContentTypeMismatch,   415},
        {FC::RateLimitExceeded,     429},
        {FC::CapabilityUnavailable, 501},
        {FC::InternalError,         500},
    };

    for (const auto& c : cases) {
        EXPECT_EQ(ApiErrorTaxonomy::toHttpStatus(c.fc), c.expected_status)
            << "Unexpected HTTP status for failure class "
            << static_cast<int>(c.fc);
    }
}

/**
 * @test ApiErrorTaxonomy correctly classifies client vs server errors
 */
TEST(Phase4TaxonomyTest, ClientVsServerErrorClassification)
{
    using FC = TransportFailureClass;

    const FC client_errors[] = {
        FC::MalformedRequest,
        FC::PayloadTooLarge,
        FC::UnsupportedVersion,
        FC::ContentTypeMissing,
        FC::ContentTypeMismatch,
        FC::Unauthorized,
        FC::RateLimitExceeded,
        FC::CapabilityUnavailable,
    };

    const FC server_errors[] = {
        FC::InternalError,
    };

    for (const auto& fc : client_errors) {
        EXPECT_TRUE(ApiErrorTaxonomy::isClientError(fc))
            << "FC " << static_cast<int>(fc) << " should be a client error";
    }

    for (const auto& fc : server_errors) {
        EXPECT_FALSE(ApiErrorTaxonomy::isClientError(fc))
            << "FC " << static_cast<int>(fc) << " should be a server error";
    }
}

/**
 * @test TransportContractValidator correctly validates all supported versions
 */
TEST(Phase4ContractsTest, SupportedVersionValidation)
{
    EXPECT_TRUE(TransportContractValidator::isSupportedVersion(""));    // absent = allowed
    EXPECT_TRUE(TransportContractValidator::isSupportedVersion("v1"));
    EXPECT_TRUE(TransportContractValidator::isSupportedVersion("v2"));
    EXPECT_FALSE(TransportContractValidator::isSupportedVersion("v0"));
    EXPECT_FALSE(TransportContractValidator::isSupportedVersion("v3"));
    EXPECT_FALSE(TransportContractValidator::isSupportedVersion("V1")); // case-sensitive
    EXPECT_FALSE(TransportContractValidator::isSupportedVersion("latest"));
}

/**
 * @test TransportContractValidator correctly identifies methods requiring Content-Type
 */
TEST(Phase4ContractsTest, ContentTypeRequirements)
{
    EXPECT_TRUE(TransportContractValidator::requiresContentType("POST"));
    EXPECT_TRUE(TransportContractValidator::requiresContentType("PUT"));
    EXPECT_TRUE(TransportContractValidator::requiresContentType("PATCH"));
    EXPECT_FALSE(TransportContractValidator::requiresContentType("GET"));
    EXPECT_FALSE(TransportContractValidator::requiresContentType("DELETE"));
    EXPECT_FALSE(TransportContractValidator::requiresContentType("HEAD"));
    EXPECT_FALSE(TransportContractValidator::requiresContentType("OPTIONS"));
}

/**
 * @test TransportPolicyConfig normalization clamps values to global limits
 */
TEST(Phase4ContractsTest, PolicyConfigNormalizationClampsOversize)
{
    TransportPolicyConfig cfg;
    cfg.max_payload_bytes = kMaxPayloadBytes * 10; // way over limit
    cfg.max_path_bytes    = kMaxPathBytes * 10;

    const auto normalized = cfg.normalized();
    EXPECT_EQ(normalized.max_payload_bytes, kMaxPayloadBytes);
    EXPECT_EQ(normalized.max_path_bytes,    kMaxPathBytes);
}
