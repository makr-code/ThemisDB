/**
 * @file test_api_degraded_mode.cpp
 * @brief Q4 2026 planned feature: degraded-mode handling for optional transport features.
 *
 * @version 0.0.1
 * @note Status: Q4 2026 planned feature — degraded-mode hardening.
 * @note Validates ROADMAP.md Planned Features (Q4 2026):
 *       - "strengthen degraded-mode handling for optional transport features"
 *       - "extend integration diagnostics for protocol-level failure classes"
 * @note Validates FUTURE_ENHANCEMENTS.md § Design Constraints:
 *       - "adapter behavior remains fail-closed on invalid or unsupported protocol input"
 *       - "high-concurrency paths remain bounded by explicit runtime controls"
 * @note Validates FUTURE_ENHANCEMENTS.md § Security / Reliability:
 *       - "enforce fail-closed behavior for malformed payloads and unsupported protocol states"
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <memory>

#include "api/http_handler.h"
#include "api/api_transport_contracts.h"
#include "api/api_error_taxonomy.h"
#include "api/api_transport_policy.h"

using namespace themis::api;
using namespace testing;

// ============================================================================
// Degraded-mode adapter — simulates an optional capability being unavailable
// ============================================================================

namespace {

/**
 * @brief Simulates an adapter where an optional capability is disabled.
 *
 * Models the scenario where a feature (e.g., gRPC streaming, WebSocket
 * subscriptions) is unavailable in the current deployment profile.
 *
 * When the capability is disabled:
 *   - Requests that require it are rejected with CapabilityUnavailable.
 *   - Other requests are handled normally.
 */
class DegradedCapabilityAdapter : public IHttpHandler,
                                   public ITransportContract {
public:
    /// Paths that require the optional capability.
    static constexpr std::string_view kStreamPath    = "/api/v1/stream";
    static constexpr std::string_view kSubscribePath = "/api/v1/subscribe";

    explicit DegradedCapabilityAdapter(bool capability_enabled)
        : capability_enabled_(capability_enabled) {}

    themis::Result<HttpResponse> handle(const HttpRequest& req) override {
        // Fail-closed: reject if request targets a path requiring missing capability.
        if (!capability_enabled_
            && (req.path == kStreamPath || req.path == kSubscribePath)) {
            return tl::unexpected(themis::Error(
                ApiErrorTaxonomy::toErrorCode(
                    TransportFailureClass::CapabilityUnavailable),
                ApiErrorTaxonomy::toMessage(
                    TransportFailureClass::CapabilityUnavailable,
                    adapterName())));
        }

        HttpResponse resp;
        resp.status_code = 200;
        resp.body        = "{\"ok\":true}";
        resp.headers["Content-Type"] = "application/json";
        return resp;
    }

    [[nodiscard]] TransportCapability capabilities() const noexcept override {
        if (capability_enabled_) {
            return TransportCapability::StreamingSupport
                 | TransportCapability::SubscriptionSupport;
        }
        return TransportCapability::None;
    }

    [[nodiscard]] std::string_view adapterName() const noexcept override {
        return "degraded-capability-adapter";
    }

    [[nodiscard]] std::string_view handlerName() const noexcept override {
        return "DegradedCapabilityAdapter";
    }

    [[nodiscard]] bool requiresAuthentication() const noexcept override {
        return false;
    }

private:
    bool capability_enabled_;
};

/**
 * @brief Adapter that simulates intermittent internal failures (partial degraded state).
 *
 * After a configurable number of successful requests it begins returning
 * transient internal errors, modelling infrastructure-level degradation.
 */
class TransientFailureAdapter : public IHttpHandler {
public:
    explicit TransientFailureAdapter(int fail_after)
        : fail_after_(fail_after) {}

    themis::Result<HttpResponse> handle(const HttpRequest& /*req*/) override {
        const int n = counter_.fetch_add(1) + 1;
        if (n > fail_after_) {
            return tl::unexpected(themis::Error(
                themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR,
                "ERR_DEGRADED_TRANSIENT: adapter in degraded state after "
                + std::to_string(fail_after_) + " requests"));
        }
        HttpResponse resp;
        resp.status_code = 200;
        resp.body        = "{\"ok\":true}";
        return resp;
    }

    [[nodiscard]] std::string_view handlerName() const noexcept override {
        return "TransientFailureAdapter";
    }

    [[nodiscard]] bool requiresAuthentication() const noexcept override {
        return false;
    }

private:
    std::atomic<int> counter_{0};
    int              fail_after_;
};

} // anonymous namespace

// ============================================================================
// Degraded-mode: capability unavailable
// ============================================================================

/**
 * @test Requests to capability-gated paths are rejected when capability is disabled
 *
 * Models gRPC streaming or WebSocket subscription unavailability in a deployment
 * where the feature has not been enabled via configuration.
 */
TEST(DegradedModeTest, CapabilityUnavailableRejectsGatedPaths)
{
    auto adapter = std::make_shared<DegradedCapabilityAdapter>(false /* disabled */);

    HttpRequest req;
    req.method = "GET";
    req.path   = std::string(DegradedCapabilityAdapter::kStreamPath);

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              ApiErrorTaxonomy::toErrorCode(
                  TransportFailureClass::CapabilityUnavailable));
    EXPECT_THAT(result.error().message(),
                HasSubstr("ERR_TRANSPORT_CAPABILITY_UNAVAILABLE"));
}

/**
 * @test Non-gated paths succeed even when the optional capability is disabled
 */
TEST(DegradedModeTest, NonGatedPathsSucceedWhenCapabilityDisabled)
{
    auto adapter = std::make_shared<DegradedCapabilityAdapter>(false /* disabled */);

    HttpRequest req;
    req.method = "GET";
    req.path   = "/api/v1/entities";

    auto result = adapter->handle(req);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->status_code, 200);
}

/**
 * @test Capability flags are correct when capability is enabled vs disabled
 */
TEST(DegradedModeTest, CapabilityFlagsReflectDeploymentState)
{
    auto enabled  = std::make_shared<DegradedCapabilityAdapter>(true);
    auto disabled = std::make_shared<DegradedCapabilityAdapter>(false);

    EXPECT_TRUE(
        hasCapability(enabled->capabilities(), TransportCapability::StreamingSupport));
    EXPECT_TRUE(
        hasCapability(enabled->capabilities(), TransportCapability::SubscriptionSupport));

    EXPECT_FALSE(
        hasCapability(disabled->capabilities(), TransportCapability::StreamingSupport));
    EXPECT_FALSE(
        hasCapability(disabled->capabilities(), TransportCapability::SubscriptionSupport));
}

/**
 * @test Subscribe path is also rejected when capability is disabled
 */
TEST(DegradedModeTest, SubscribePathRejectedWhenCapabilityDisabled)
{
    auto adapter = std::make_shared<DegradedCapabilityAdapter>(false);

    HttpRequest req;
    req.method = "GET";
    req.path   = std::string(DegradedCapabilityAdapter::kSubscribePath);

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ApiErrorTaxonomy::toHttpStatus(TransportFailureClass::CapabilityUnavailable), 501);
}

// ============================================================================
// Degraded-mode: transient failure recovery
// ============================================================================

/**
 * @test Requests succeed before degradation threshold; fail after threshold
 *
 * Verifies that callers can detect degradation via error codes and that the
 * adapter provides a well-formed error response for every post-threshold request.
 */
TEST(DegradedModeTest, TransientFailureAdapterDegradationBehavior)
{
    constexpr int kFail_after = 5;
    auto adapter = std::make_shared<TransientFailureAdapter>(kFail_after);

    // First `kFail_after` requests succeed.
    for (int i = 0; i < kFail_after; ++i) {
        HttpRequest req;
        req.method = "GET";
        req.path   = "/api/v1/ping";
        auto result = adapter->handle(req);
        EXPECT_TRUE(result.has_value())
            << "Request " << (i + 1) << " should succeed before threshold";
    }

    // Subsequent requests must fail with a well-formed error.
    for (int i = 0; i < 5; ++i) {
        HttpRequest req;
        req.method = "GET";
        req.path   = "/api/v1/ping";
        auto result = adapter->handle(req);
        ASSERT_FALSE(result.has_value())
            << "Request " << (kFail_after + i + 1) << " should fail after threshold";
        EXPECT_EQ(result.error().code(),
                  themis::errors::ErrorCode::ERR_API_INTERNAL_ERROR);
        EXPECT_THAT(result.error().message(), HasSubstr("ERR_DEGRADED_TRANSIENT"));
    }
}

// ============================================================================
// Degraded-mode: policy + degraded adapter composition
// ============================================================================

/**
 * @test TransportPolicyMiddleware composes correctly with a degraded inner adapter
 *
 * Valid requests pass the policy layer and hit the degraded inner adapter.
 * Invalid requests are rejected by the policy layer without touching the adapter.
 */
TEST(DegradedModeTest, PolicyComposesWithDegradedAdapter)
{
    auto inner  = std::make_shared<DegradedCapabilityAdapter>(false);
    auto policy = std::make_shared<TransportPolicyMiddleware>(inner);

    // Valid request to a non-gated path → policy passes → inner succeeds.
    {
        HttpRequest req;
        req.method = "GET";
        req.path   = "/api/v1/entities";
        auto result = policy->handle(req);
        EXPECT_TRUE(result.has_value());
    }

    // Invalid version → rejected at policy layer (inner never called).
    {
        HttpRequest req;
        req.method                   = "GET";
        req.path                     = "/api/v1/entities";
        req.headers["X-API-Version"] = "v999";
        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error().message(),
                    HasSubstr("ERR_TRANSPORT_UNSUPPORTED_VERSION"));
    }

    // Valid request to gated path → policy passes → inner rejects (capability unavailable).
    {
        HttpRequest req;
        req.method = "GET";
        req.path   = std::string(DegradedCapabilityAdapter::kStreamPath);
        auto result = policy->handle(req);
        ASSERT_FALSE(result.has_value());
        EXPECT_THAT(result.error().message(),
                    HasSubstr("ERR_TRANSPORT_CAPABILITY_UNAVAILABLE"));
    }
}

// ============================================================================
// Degraded-mode: concurrent requests to a partially-degraded adapter
// ============================================================================

/**
 * @test 8 threads mix non-gated (success) and gated (failure) requests concurrently
 *
 * Validates that the degraded adapter handles mixed concurrent traffic
 * without data races and produces the correct error vs success split.
 */
TEST(DegradedModeTest, ConcurrentMixedRequestsDegradedAdapter)
{
    auto adapter = std::make_shared<DegradedCapabilityAdapter>(false /* disabled */);

    constexpr int kThreads         = 8;
    constexpr int kRequestsPerThread = 25;
    std::atomic<int> successes{0};
    std::atomic<int> capability_errors{0};

    auto worker = [&]() {
        for (int i = 0; i < kRequestsPerThread; ++i) {
            const bool gated = (i % 2 == 0);

            HttpRequest req;
            req.method = "GET";
            req.path   = gated
                ? std::string(DegradedCapabilityAdapter::kStreamPath)
                : "/api/v1/entities";

            auto result = adapter->handle(req);
            if (result.has_value()) {
                successes.fetch_add(1, std::memory_order_relaxed);
            } else if (result.error().code()
                       == ApiErrorTaxonomy::toErrorCode(
                              TransportFailureClass::CapabilityUnavailable)) {
                capability_errors.fetch_add(1, std::memory_order_relaxed);
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

    // Half the requests per thread are gated (i % 2 == 0 → indices 0,2,4,…).
    // kRequestsPerThread = 25: indices 0,2,4,6,8,10,12,14,16,18,20,22,24 = 13 gated;
    // 25 - 13 = 12 non-gated.
    constexpr int expected_gated     = 13;
    constexpr int expected_non_gated = 12;

    EXPECT_EQ(capability_errors.load(), kThreads * expected_gated);
    EXPECT_EQ(successes.load(),         kThreads * expected_non_gated);
}

// ============================================================================
// Integration diagnostics: error taxonomy completeness
// ============================================================================

/**
 * @test Every TransportFailureClass produces a non-empty structured message
 *
 * Validates the Q4 2026 "extend integration diagnostics for protocol-level
 * failure classes" requirement: every failure class must produce an actionable,
 * non-empty message string that operators can triage.
 */
TEST(DegradedModeDiagnosticsTest, AllFailureClassesHaveActionableMessages)
{
    using FC = TransportFailureClass;
    const FC all_classes[] = {
        FC::MalformedRequest,
        FC::PayloadTooLarge,
        FC::UnsupportedVersion,
        FC::ContentTypeMissing,
        FC::ContentTypeMismatch,
        FC::Unauthorized,
        FC::RateLimitExceeded,
        FC::CapabilityUnavailable,
        FC::InternalError,
    };

    for (const auto& fc : all_classes) {
        const auto msg = ApiErrorTaxonomy::toMessage(fc, "test-adapter");
        EXPECT_FALSE(msg.empty())
            << "Failure class " << static_cast<int>(fc) << " has empty message";
        // Every message must contain an ERR_ prefix for traceability.
        EXPECT_THAT(msg, HasSubstr("ERR_"))
            << "Failure class " << static_cast<int>(fc)
            << " message lacks ERR_ prefix";
    }
}
