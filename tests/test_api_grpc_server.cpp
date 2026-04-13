/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_api_grpc_server.cpp                           ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-13 20:40:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     212                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 8f4f0c9ea2  2026-02-23  Implement gRPC API server alongside REST (src/api/grpc_se... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifdef THEMIS_ENABLE_GRPC

#include <gtest/gtest.h>
#include "api/grpc_server.h"
#include "api/themisdb_grpc_service.h"
#include <grpcpp/grpcpp.h>

using namespace themis::api;

// ---------------------------------------------------------------------------
// Helper: build a default insecure config pointing at a random high port
// ---------------------------------------------------------------------------
static GrpcServerConfig makeInsecureConfig(uint16_t port = 50099) {
    GrpcServerConfig cfg;
    cfg.host        = "127.0.0.1";
    cfg.port        = port;
    cfg.tls_enabled = false;
    return cfg;
}

static grpc::Service* testService() {
    static ThemisDBGrpcService service(nullptr, nullptr);
    return static_cast<grpc::Service*>(service.service());
}

// ============================================================================
// Configuration / initialization tests
// ============================================================================

TEST(GrpcApiServerTest, DefaultConstruction) {
    GrpcApiServer srv;
    EXPECT_FALSE(srv.isRunning());
    EXPECT_TRUE(srv.getAddress().empty());
    EXPECT_EQ(srv.getPort(), 0);
}

TEST(GrpcApiServerTest, InitializeInsecure) {
    GrpcApiServer srv;
    GrpcServerConfig cfg = makeInsecureConfig();

    EXPECT_TRUE(srv.initialize(cfg));
    EXPECT_EQ(srv.getAddress(), "127.0.0.1:50099");
    EXPECT_EQ(srv.getPort(), 50099);
    EXPECT_FALSE(srv.isRunning());
}

TEST(GrpcApiServerTest, InitializeInvalidPortZero) {
    GrpcApiServer srv;
    GrpcServerConfig cfg = makeInsecureConfig(0);
    EXPECT_FALSE(srv.initialize(cfg));
}

TEST(GrpcApiServerTest, InitializeWhileRunning) {
    // Start a server and verify that calling initialize() again is rejected.
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig(50091)));
    auto* service = testService();
    if (!service) {
        GTEST_SKIP() << "No generated gRPC API service available in this test target";
    }
    srv.registerService(service);
    ASSERT_TRUE(srv.start());
    ASSERT_TRUE(srv.isRunning());

    // A second initialize() while running must fail
    GrpcServerConfig cfg2 = makeInsecureConfig(50092);
    EXPECT_FALSE(srv.initialize(cfg2));

    srv.stop();
}

// ============================================================================
// Lifecycle tests
// ============================================================================

TEST(GrpcApiServerTest, StartStopCycle) {
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig(50088)));
    auto* service = testService();
    if (!service) {
        GTEST_SKIP() << "No generated gRPC API service available in this test target";
    }
    srv.registerService(service);

    EXPECT_FALSE(srv.isRunning());
    ASSERT_TRUE(srv.start());
    EXPECT_TRUE(srv.isRunning());

    srv.stop();
    EXPECT_FALSE(srv.isRunning());
}

TEST(GrpcApiServerTest, StartWithoutInitializeFails) {
    GrpcApiServer srv;
    // start() before initialize() should return false gracefully
    EXPECT_FALSE(srv.start());
    EXPECT_FALSE(srv.isRunning());
}

TEST(GrpcApiServerTest, DoubleStartReturnsFalse) {
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig(50087)));
    auto* service = testService();
    if (!service) {
        GTEST_SKIP() << "No generated gRPC API service available in this test target";
    }
    srv.registerService(service);
    ASSERT_TRUE(srv.start());
    EXPECT_TRUE(srv.isRunning());

    // Second start() must not crash and must return false
    EXPECT_FALSE(srv.start());

    srv.stop();
}

TEST(GrpcApiServerTest, StopIdempotent) {
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig(50086)));
    auto* service = testService();
    if (!service) {
        GTEST_SKIP() << "No generated gRPC API service available in this test target";
    }
    srv.registerService(service);
    ASSERT_TRUE(srv.start());

    srv.stop();
    EXPECT_FALSE(srv.isRunning());

    // Calling stop() again must not crash
    ASSERT_NO_THROW(srv.stop());
    EXPECT_FALSE(srv.isRunning());
}

// ============================================================================
// Service registration tests
// ============================================================================

TEST(GrpcApiServerTest, RegisterNullServiceIsIgnored) {
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig(50085)));
    // Registering a null pointer must not crash
    ASSERT_NO_THROW(srv.registerService(nullptr));
}

// ============================================================================
// TLS configuration tests (file I/O failure path – no real certs needed)
// ============================================================================

TEST(GrpcApiServerTest, TlsEnabledWithMissingCertFailsToStart) {
    GrpcApiServer srv;
    GrpcServerConfig cfg = makeInsecureConfig(50083);
    cfg.tls_enabled  = true;
    cfg.tls_cert_path = "/nonexistent/cert.pem";
    cfg.tls_key_path  = "/nonexistent/key.pem";

    ASSERT_TRUE(srv.initialize(cfg));

    // start() fails closed and reports the credential error via a false return.
    EXPECT_FALSE(srv.start());
    EXPECT_FALSE(srv.isRunning());
}

// ============================================================================
// Address / port accessors
// ============================================================================

TEST(GrpcApiServerTest, AddressFormattedCorrectly) {
    GrpcApiServer srv;
    GrpcServerConfig cfg;
    cfg.host = "::1";
    cfg.port = 12345;
    ASSERT_TRUE(srv.initialize(cfg));
    EXPECT_EQ(srv.getAddress(), "::1:12345");
    EXPECT_EQ(srv.getPort(), 12345);
}

#else

// When gRPC is disabled, provide a placeholder so the translation unit is
// not empty and the build does not warn about an empty test binary.
#include <gtest/gtest.h>

TEST(GrpcApiServerTest, GrpcDisabledSkip) {
    GTEST_SKIP() << "THEMIS_ENABLE_GRPC not set; gRPC API server tests skipped";
}

#endif // THEMIS_ENABLE_GRPC
