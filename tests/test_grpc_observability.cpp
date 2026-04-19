/**
 * gRPC Server v0.3.0 — Health & Observability unit tests.
 *
 * Test IDs:
 *   GOB-01 … GOB-04  — Health service (setServiceHealth / isServiceHealthy)
 *   GOB-05 … GOB-08  — Interceptor metrics (recordRPC)
 *   GOB-09 … GOB-10  — Prometheus metrics text (getMetricsText)
 *   GOB-11 … GOB-12  — Structured access log (setAccessLogSink / logAccess)
 */

#include <gtest/gtest.h>
#include "rpc_grpc/grpc_plugin.h"
#include "plugins/rpc_plugin_interface.h"

#include <atomic>
#include <string>
#include <vector>

using namespace themis::plugins;
using namespace themis::plugins::rpc;
using namespace themis::plugins::rpc::grpc_plugin;

// ============================================================================
// Test fixture
// ============================================================================

class GRPCObservabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<GRPCServer>();
    }

    std::unique_ptr<GRPCServer> server;
};

// ============================================================================
// GOB-01: isServiceHealthy returns true for unknown service (default SERVING)
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB01_HealthDefaultServing) {
    EXPECT_TRUE(server->isServiceHealthy(""));
    EXPECT_TRUE(server->isServiceHealthy("my.Service"));
}

// ============================================================================
// GOB-02: setServiceHealth(false) makes isServiceHealthy return false
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB02_HealthSetNotServing) {
    server->setServiceHealth("my.Service", false);
    EXPECT_FALSE(server->isServiceHealthy("my.Service"));
}

// ============================================================================
// GOB-03: setServiceHealth(true) restores SERVING after NOT_SERVING
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB03_HealthRestoreServing) {
    server->setServiceHealth("my.Service", false);
    server->setServiceHealth("my.Service", true);
    EXPECT_TRUE(server->isServiceHealthy("my.Service"));
}

// ============================================================================
// GOB-04: Global ("") and per-service health are independent
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB04_HealthIndependentPerService) {
    server->setServiceHealth("", false);
    EXPECT_FALSE(server->isServiceHealthy(""));
    EXPECT_TRUE(server->isServiceHealthy("other.Service"));
}

// ============================================================================
// GOB-05: recordRPC increments total_requests in stats
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB05_RecordRPCIncrementsTotalRequests) {
    auto before = server->getStats().total_requests;
    server->recordRPC("/svc/Method", true, 5);
    auto after = server->getStats().total_requests;
    EXPECT_EQ(before + 1, after);
}

// ============================================================================
// GOB-06: recordRPC(success=true) increments successful_requests
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB06_RecordRPCSuccessCounter) {
    auto before = server->getStats().successful_requests;
    server->recordRPC("/svc/Hello", true, 10);
    EXPECT_EQ(before + 1, server->getStats().successful_requests);
    EXPECT_EQ(0u,         server->getStats().failed_requests);
}

// ============================================================================
// GOB-07: recordRPC(success=false) increments failed_requests
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB07_RecordRPCErrorCounter) {
    auto before = server->getStats().failed_requests;
    server->recordRPC("/svc/Err", false, 20);
    EXPECT_EQ(before + 1, server->getStats().failed_requests);
}

// ============================================================================
// GOB-08: Multiple recordRPC calls accumulate correctly
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB08_RecordRPCMultipleCalls) {
    for (int i = 0; i < 5; ++i)
        server->recordRPC("/svc/Batch", i % 2 == 0, static_cast<uint64_t>(i * 10));
    auto stats = server->getStats();
    EXPECT_EQ(5u, stats.total_requests);
    EXPECT_EQ(3u, stats.successful_requests); // i=0,2,4 → even
    EXPECT_EQ(2u, stats.failed_requests);     // i=1,3   → odd
}

// ============================================================================
// GOB-09: getMetricsText returns empty string when no calls recorded
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB09_MetricsTextEmptyBeforeAnyRPC) {
    EXPECT_EQ("", server->getMetricsText());
}

// ============================================================================
// GOB-10: getMetricsText contains required metric families after recordRPC
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB10_MetricsTextContainsRequiredFamilies) {
    server->recordRPC("/pkg.Svc/Method", true, 42);
    server->recordRPC("/pkg.Svc/Method", false, 7);

    std::string text = server->getMetricsText();
    EXPECT_NE(std::string::npos, text.find("grpc_server_requests_total"))
        << "Missing grpc_server_requests_total in:\n" << text;
    EXPECT_NE(std::string::npos, text.find("grpc_server_errors_total"))
        << "Missing grpc_server_errors_total in:\n" << text;
    EXPECT_NE(std::string::npos, text.find("grpc_server_latency_ms_total"))
        << "Missing grpc_server_latency_ms_total in:\n" << text;
    EXPECT_NE(std::string::npos, text.find("grpc_server_active_connections"))
        << "Missing grpc_server_active_connections in:\n" << text;
    // Method label must appear
    EXPECT_NE(std::string::npos, text.find("/pkg.Svc/Method"))
        << "Method label missing in:\n" << text;
    // Counter values
    EXPECT_NE(std::string::npos, text.find("} 2"))   // 2 requests total
        << "Expected counter value 2 in:\n" << text;
}

// ============================================================================
// GOB-11: setAccessLogSink receives one call per logAccess
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB11_AccessLogSinkCalledOnLogAccess) {
    std::vector<std::string> entries;
    server->setAccessLogSink([&](const std::string& line) {
        entries.push_back(line);
    });
    server->logAccess("/svc/Foo", 0, 15, "client.example.com");
    ASSERT_EQ(1u, entries.size());
    // Entry must be a JSON object containing expected fields
    const auto& e = entries[0];
    EXPECT_EQ('{', e.front());
    EXPECT_NE(std::string::npos, e.find("timestamp_ms"))   << "Missing timestamp_ms";
    EXPECT_NE(std::string::npos, e.find("/svc/Foo"))       << "Missing method";
    EXPECT_NE(std::string::npos, e.find("status_code"))    << "Missing status_code";
    EXPECT_NE(std::string::npos, e.find("duration_ms"))    << "Missing duration_ms";
    EXPECT_NE(std::string::npos, e.find("client.example.com")) << "Missing client_cn";
}

// ============================================================================
// GOB-12: recordRPC triggers access log sink automatically
// ============================================================================
TEST_F(GRPCObservabilityTest, GOB12_RecordRPCTriggersAccessLog) {
    std::atomic<int> call_count{0};
    server->setAccessLogSink([&](const std::string&) { ++call_count; });

    server->recordRPC("/auto/Log", true, 8);
    server->recordRPC("/auto/Log", false, 3);

    EXPECT_EQ(2, call_count.load());
}
