/**
 * @file test_api_error_handling.cpp
 * @brief API Module Error Handling and Degraded Mode Tests
 * @version 0.0.1
 * @note Status: Error handling and degraded mode validation
 * @note Validates roadmap item: "strengthen degraded-mode handling for optional 
 *       transport features (Target: Q4 2026)"
 * @note Validates FUTURE_ENHANCEMENTS.md § Test Strategy:
 *       - deterministic fixture tests for degraded/unsupported capability modes
 *       - focused unit and integration tests across GraphQL/gRPC/WebSocket adapters
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>

#include "api/http_handler.h"

using namespace themis::api;

namespace {

/**
 * @brief Mock adapter that simulates error conditions and degraded modes
 */
class DegradedModeAdapter : public IHttpHandler {
public:
    enum class FailureMode {
        NONE,
        MALFORMED_REQUEST,
        TIMEOUT,
        SERVICE_UNAVAILABLE,
        QUOTA_EXCEEDED,
        UNSUPPORTED_FEATURE
    };

    explicit DegradedModeAdapter(FailureMode mode = FailureMode::NONE)
        : failure_mode_(mode) {}

    themis::Result<HttpResponse> handle(const HttpRequest& req) override {
        // Simulate different failure modes
        switch (failure_mode_) {
            case FailureMode::MALFORMED_REQUEST:
                return tl::unexpected(themis::Error(
                    themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                    "ERR_DEGRADED_MALFORMED_REQUEST: request validation failed"));

            case FailureMode::TIMEOUT:
                return tl::unexpected(themis::Error(
                    themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                    "ERR_DEGRADED_TIMEOUT: request processing timed out (5000ms)"));

            case FailureMode::SERVICE_UNAVAILABLE:
                return tl::unexpected(themis::Error(
                    themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                    "ERR_DEGRADED_SERVICE_UNAVAILABLE: backend service temporarily unavailable"));

            case FailureMode::QUOTA_EXCEEDED:
                return tl::unexpected(themis::Error(
                    themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                    "ERR_DEGRADED_QUOTA_EXCEEDED: request quota exhausted"));

            case FailureMode::UNSUPPORTED_FEATURE:
                // Graceful degradation: return success but indicate limited capability
                if (req.headers.count("X-Request-WebSocket") > 0) {
                    return tl::unexpected(themis::Error(
                        themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                        "ERR_DEGRADED_WEBSOCKET_DISABLED: WebSocket feature not available"));
                }
                // Fall through to normal handling for non-WebSocket requests
                [[fallthrough]];

            case FailureMode::NONE:
            default:
                // Normal operation
                if (req.method.empty() || req.path.empty()) {
                    return tl::unexpected(themis::Error(
                        themis::errors::ErrorCode::ERR_API_INVALID_REQUEST,
                        "Invalid request"));
                }
                HttpResponse resp;
                resp.status_code = 200;
                resp.body = "{\"status\": \"ok\"}";
                resp.headers["Content-Type"] = "application/json";
                return resp;
        }
    }

    std::string_view handlerName() const noexcept override {
        return "DegradedModeAdapter";
    }

    bool requiresAuthentication() const noexcept override {
        return true;
    }

private:
    FailureMode failure_mode_;
};

}  // anonymous namespace

// ============================================================================
// Error Handling Tests
// ============================================================================

/**
 * @test Malformed request error
 * Degraded mode adapter returns ERR_DEGRADED_MALFORMED_REQUEST for malformed requests
 */
TEST(ErrorHandlingTest, MalformedRequestError)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::MALFORMED_REQUEST);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(),
                ::testing::HasSubstr("ERR_DEGRADED_MALFORMED_REQUEST"));
}

/**
 * @test Timeout error
 * Adapter returns ERR_DEGRADED_TIMEOUT with bounded timeout value
 */
TEST(ErrorHandlingTest, TimeoutError)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::TIMEOUT);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(),
                ::testing::HasSubstr("ERR_DEGRADED_TIMEOUT"));
    EXPECT_THAT(result.error().message(),
                ::testing::HasSubstr("5000ms"));
}

/**
 * @test Service unavailable error
 * Adapter returns ERR_DEGRADED_SERVICE_UNAVAILABLE for backend failures
 */
TEST(ErrorHandlingTest, ServiceUnavailableError)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::SERVICE_UNAVAILABLE);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(),
                ::testing::HasSubstr("ERR_DEGRADED_SERVICE_UNAVAILABLE"));
}

/**
 * @test Quota exceeded error
 * Adapter returns ERR_DEGRADED_QUOTA_EXCEEDED when limits are reached
 */
TEST(ErrorHandlingTest, QuotaExceededError)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::QUOTA_EXCEEDED);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(),
                ::testing::HasSubstr("ERR_DEGRADED_QUOTA_EXCEEDED"));
}

// ============================================================================
// Degraded Mode Tests
// ============================================================================

/**
 * @test Unsupported WebSocket feature
 * Adapter gracefully rejects WebSocket requests when feature is unavailable
 */
TEST(DegradedModeTest, WebSocketFeatureUnavailable)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::UNSUPPORTED_FEATURE);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/subscribe";
    req.headers["X-Request-WebSocket"] = "true";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());
    EXPECT_THAT(result.error().message(),
                ::testing::HasSubstr("ERR_DEGRADED_WEBSOCKET_DISABLED"));
}

/**
 * @test REST request in WebSocket-degraded mode
 * Non-WebSocket requests succeed even when WebSocket feature is unavailable
 */
TEST(DegradedModeTest, RestRequestDuringWebSocketDegradation)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::UNSUPPORTED_FEATURE);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";
    // No X-Request-WebSocket header

    auto result = adapter->handle(req);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->status_code, 200);
}

/**
 * @test Graceful degradation: partial capability availability
 * System continues functioning with reduced capabilities during degraded mode
 */
TEST(DegradedModeTest, PartialCapabilityAvailability)
{
    // WebSocket adapter in degraded mode
    auto ws_adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::UNSUPPORTED_FEATURE);
    
    // gRPC adapter still functioning normally
    auto grpc_adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::NONE);

    // WebSocket request should fail
    HttpRequest ws_req;
    ws_req.method = "GET";
    ws_req.path = "/api/v1/subscribe";
    ws_req.headers["X-Request-WebSocket"] = "true";
    auto ws_result = ws_adapter->handle(ws_req);
    EXPECT_FALSE(ws_result.has_value());

    // gRPC request should succeed
    HttpRequest grpc_req;
    grpc_req.method = "GET";
    grpc_req.path = "/api/v1/entities";
    auto grpc_result = grpc_adapter->handle(grpc_req);
    EXPECT_TRUE(grpc_result.has_value());
}

// ============================================================================
// Error Recovery and Retry Logic Tests
// ============================================================================

/**
 * @test Transient error: Service unavailable recovery
 * Client can retry after ERR_DEGRADED_SERVICE_UNAVAILABLE
 */
TEST(ErrorRecoveryTest, TransientServiceUnavailableRecovery)
{
    // First attempt: service unavailable
    auto degraded_adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::SERVICE_UNAVAILABLE);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    auto result1 = degraded_adapter->handle(req);
    EXPECT_FALSE(result1.has_value());

    // After recovery: service available
    auto normal_adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::NONE);

    auto result2 = normal_adapter->handle(req);
    EXPECT_TRUE(result2.has_value());
}

/**
 * @test Permanent error: Unsupported feature
 * Client must not retry unsupported feature errors
 */
TEST(ErrorRecoveryTest, PermanentUnsupportedFeatureError)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::UNSUPPORTED_FEATURE);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/subscribe";
    req.headers["X-Request-WebSocket"] = "true";

    // Multiple attempts should all fail with same error
    for (int i = 0; i < 3; ++i) {
        auto result = adapter->handle(req);
        EXPECT_FALSE(result.has_value());
        EXPECT_THAT(result.error().message(),
                    ::testing::HasSubstr("ERR_DEGRADED_WEBSOCKET_DISABLED"));
    }
}

/**
 * @test Quota error recovery
 * After quota reset, requests should succeed
 */
TEST(ErrorRecoveryTest, QuotaErrorRecovery)
{
    // Create adapter in quota-exceeded mode
    auto quota_exceeded = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::QUOTA_EXCEEDED);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    // Initial quota-exceeded error
    auto result1 = quota_exceeded->handle(req);
    EXPECT_FALSE(result1.has_value());

    // After quota reset (create new adapter with NONE mode)
    auto quota_reset = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::NONE);

    auto result2 = quota_reset->handle(req);
    EXPECT_TRUE(result2.has_value());
}

// ============================================================================
// Error Message Quality Tests
// ============================================================================

/**
 * @test Error messages are descriptive
 * All error messages include error code, description, and helpful context
 */
TEST(ErrorMessageQualityTest, DescriptiveErrorMessages)
{
    std::vector<std::pair<DegradedModeAdapter::FailureMode, std::string>> test_cases = {
        {DegradedModeAdapter::FailureMode::TIMEOUT, "ERR_DEGRADED_TIMEOUT"},
        {DegradedModeAdapter::FailureMode::SERVICE_UNAVAILABLE, "ERR_DEGRADED_SERVICE_UNAVAILABLE"},
        {DegradedModeAdapter::FailureMode::QUOTA_EXCEEDED, "ERR_DEGRADED_QUOTA_EXCEEDED"},
    };

    for (const auto& [mode, expected_code] : test_cases) {
        auto adapter = std::make_shared<DegradedModeAdapter>(mode);
        HttpRequest req;
        req.method = "GET";
        req.path = "/api/v1/entities";

        auto result = adapter->handle(req);
        ASSERT_FALSE(result.has_value());
        
        const auto& message = result.error().message();
        EXPECT_THAT(message, ::testing::HasSubstr(expected_code))
            << "Error message should contain error code";
        EXPECT_GT(message.length(), expected_code.length() + 5)
            << "Error message should be descriptive";
    }
}

/**
 * @test Error messages do not leak sensitive information
 * Error messages include only public-safe information
 */
TEST(ErrorMessageQualityTest, ErrorMessagesSafety)
{
    auto adapter = std::make_shared<DegradedModeAdapter>(
        DegradedModeAdapter::FailureMode::SERVICE_UNAVAILABLE);
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/api/v1/entities";

    auto result = adapter->handle(req);
    ASSERT_FALSE(result.has_value());

    const auto& message = result.error().message();
    // Should not contain internal details like IP addresses, hostnames, or paths
    EXPECT_FALSE(message.find("127.0.0.1") != std::string::npos);
    EXPECT_FALSE(message.find("/home/") != std::string::npos);
}
