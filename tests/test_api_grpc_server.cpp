#ifdef THEMIS_ENABLE_GRPC

#include <gtest/gtest.h>
#include "api/grpc_server.h"
#include "api/themisdb_grpc_service.h"
#include <grpcpp/grpcpp.h>
#include <boost/asio.hpp>

using namespace themis::api;

// ---------------------------------------------------------------------------
// Helper: build a default insecure config pointing at a random high port
// ---------------------------------------------------------------------------
static uint16_t findFreeLoopbackPort() {
    boost::asio::io_context io;
    boost::asio::ip::tcp::acceptor acceptor(io);
    acceptor.open(boost::asio::ip::tcp::v4());
    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind({boost::asio::ip::address_v4::loopback(), 0});
    const auto port = acceptor.local_endpoint().port();
    acceptor.close();
    return static_cast<uint16_t>(port);
}

static GrpcServerConfig makeInsecureConfig(uint16_t port = 0) {
    GrpcServerConfig cfg;
    cfg.host        = "127.0.0.1";
    cfg.port        = (port == 0) ? findFreeLoopbackPort() : port;
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
    const auto port = findFreeLoopbackPort();
    GrpcServerConfig cfg = makeInsecureConfig(port);

    EXPECT_TRUE(srv.initialize(cfg));
    EXPECT_EQ(srv.getAddress(), "127.0.0.1:" + std::to_string(port));
    EXPECT_EQ(srv.getPort(), port);
    EXPECT_FALSE(srv.isRunning());
}

TEST(GrpcApiServerTest, InitializeInvalidPortZero) {
    GrpcApiServer srv;
    GrpcServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 0;
    cfg.tls_enabled = false;
    EXPECT_FALSE(srv.initialize(cfg));
}

TEST(GrpcApiServerTest, InitializeWhileRunning) {
    // Start a server and verify that calling initialize() again is rejected.
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig()));
    auto* service = testService();
    if (!service) {
        GTEST_SKIP() << "No generated gRPC API service available in this test target";
    }
    srv.registerService(service);
    ASSERT_TRUE(srv.start());
    ASSERT_TRUE(srv.isRunning());

    // A second initialize() while running must fail
    GrpcServerConfig cfg2 = makeInsecureConfig();
    EXPECT_FALSE(srv.initialize(cfg2));

    srv.stop();
}

// ============================================================================
// Lifecycle tests
// ============================================================================

TEST(GrpcApiServerTest, StartStopCycle) {
    GrpcApiServer srv;
    ASSERT_TRUE(srv.initialize(makeInsecureConfig()));
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
    ASSERT_TRUE(srv.initialize(makeInsecureConfig()));
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
    ASSERT_TRUE(srv.initialize(makeInsecureConfig()));
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
    ASSERT_TRUE(srv.initialize(makeInsecureConfig()));
    // Registering a null pointer must not crash
    ASSERT_NO_THROW(srv.registerService(nullptr));
}

// ============================================================================
// TLS configuration tests (file I/O failure path – no real certs needed)
// ============================================================================

TEST(GrpcApiServerTest, TlsEnabledWithMissingCertFailsToStart) {
    GrpcApiServer srv;
    GrpcServerConfig cfg = makeInsecureConfig();
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
