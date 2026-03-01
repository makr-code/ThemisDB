// Unit tests for ServiceMeshIntegration (include/network/service_mesh.h).
//
// These tests validate configuration defaults, port-validation logic,
// address formatting, Envoy detection, and statistics initialisation
// without requiring a live probe socket or Istio/Envoy installation.

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include "network/service_mesh.h"
#include <string>

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, DefaultConfigPort) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_EQ(cfg.probe_port, kServiceMeshProbeDefaultPort);
    EXPECT_EQ(cfg.probe_port, 8082u);
}

TEST(ServiceMeshTest, DefaultConfigHost) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

TEST(ServiceMeshTest, DefaultTrustSidecarMTLSDisabled) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_FALSE(cfg.trust_sidecar_mtls);
}

TEST(ServiceMeshTest, DefaultDrainTimeout) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_EQ(cfg.drain_timeout_ms, 5000u);
}

TEST(ServiceMeshTest, DefaultInboundPorts) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_EQ(cfg.inbound_ports, "8766,8080");
}

TEST(ServiceMeshTest, DefaultExcludedPorts) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_EQ(cfg.excluded_ports, "8082,8769");
}

TEST(ServiceMeshTest, DefaultPropagateTracingHeaders) {
    ServiceMeshIntegration::Config cfg;
    EXPECT_TRUE(cfg.propagate_tracing_headers);
}

// ─────────────────────────────────────────────────────────────────────────────
// Port validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, ValidPortDefault) {
    EXPECT_TRUE(ServiceMeshIntegration::isValidPort(kServiceMeshProbeDefaultPort));
}

TEST(ServiceMeshTest, InvalidPortZero) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(0));
}

TEST(ServiceMeshTest, InvalidPortHTTP) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(80));
}

TEST(ServiceMeshTest, InvalidPortHTTPS) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(443));
}

TEST(ServiceMeshTest, InvalidPortHTTPAPI) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(8080));  // HTTP API server
}

TEST(ServiceMeshTest, InvalidPortHealthEndpoint) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(8081));  // general health endpoint
}

TEST(ServiceMeshTest, InvalidPortTCPWireProtocol) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(8766));  // TCP wire protocol
}

TEST(ServiceMeshTest, InvalidPortUDPFastPath) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(8769));  // UDP fast-path
}

TEST(ServiceMeshTest, InvalidPortQuicTransport) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(8770));  // QUIC transport
}

TEST(ServiceMeshTest, InvalidPortGrpcTransport) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(8771));  // gRPC native transport
}

TEST(ServiceMeshTest, InvalidPortGrpcAPIServer) {
    EXPECT_FALSE(ServiceMeshIntegration::isValidPort(50051));  // gRPC API (server module)
}

TEST(ServiceMeshTest, ValidPortCustom) {
    EXPECT_TRUE(ServiceMeshIntegration::isValidPort(8082));   // default probe port
    EXPECT_TRUE(ServiceMeshIntegration::isValidPort(8083));
    EXPECT_TRUE(ServiceMeshIntegration::isValidPort(9000));
    EXPECT_TRUE(ServiceMeshIntegration::isValidPort(50052));
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics initialisation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, InitialStatsAllZero) {
    ServiceMeshIntegration smi;

    ServiceMeshIntegration::Stats s = smi.getStats();
    EXPECT_EQ(s.healthz_requests, 0u);
    EXPECT_EQ(s.readyz_requests,  0u);
    EXPECT_EQ(s.healthz_ok,       0u);
    EXPECT_EQ(s.readyz_ok,        0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Protocol constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, DefaultPortDoesNotConflict) {
    // Ensure default port 8082 differs from all other ThemisDB transports.
    EXPECT_NE(kServiceMeshProbeDefaultPort, 8080u);   // HTTP API server
    EXPECT_NE(kServiceMeshProbeDefaultPort, 8081u);   // general health endpoint
    EXPECT_NE(kServiceMeshProbeDefaultPort, 8766u);   // TCP wire protocol
    EXPECT_NE(kServiceMeshProbeDefaultPort, 8769u);   // UDP fast-path
    EXPECT_NE(kServiceMeshProbeDefaultPort, 8770u);   // QUIC transport
    EXPECT_NE(kServiceMeshProbeDefaultPort, 8771u);   // gRPC native transport
    EXPECT_NE(kServiceMeshProbeDefaultPort, 443u);    // HTTPS
    EXPECT_NE(kServiceMeshProbeDefaultPort, 80u);     // HTTP
    EXPECT_NE(kServiceMeshProbeDefaultPort, 50051u);  // gRPC API server
}

// ─────────────────────────────────────────────────────────────────────────────
// Address formatting
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, GetAddressDefaultConfig) {
    ServiceMeshIntegration smi;
    EXPECT_EQ(smi.getAddress(), "0.0.0.0:8082");
}

TEST(ServiceMeshTest, GetAddressCustomHostPort) {
    ServiceMeshIntegration::Config cfg;
    cfg.host       = "127.0.0.1";
    cfg.probe_port = 9999;
    ServiceMeshIntegration smi(cfg);
    EXPECT_EQ(smi.getAddress(), "127.0.0.1:9999");
}

TEST(ServiceMeshTest, GetPortMatchesConfig) {
    ServiceMeshIntegration::Config cfg;
    cfg.probe_port = 8083;
    ServiceMeshIntegration smi(cfg);
    EXPECT_EQ(smi.getPort(), 8083u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sidecar TLS offload
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, TLSOffloadDefaultFalse) {
    ServiceMeshIntegration smi;
    EXPECT_FALSE(smi.isTLSOffloadedToSidecar());
}

TEST(ServiceMeshTest, TLSOffloadRespectsTrustSidecarMTLS) {
    ServiceMeshIntegration::Config cfg;
    cfg.trust_sidecar_mtls = true;
    ServiceMeshIntegration smi(cfg);
    EXPECT_TRUE(smi.isTLSOffloadedToSidecar());
}

// ─────────────────────────────────────────────────────────────────────────────
// Istio annotation helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, InboundPortsDefault) {
    ServiceMeshIntegration smi;
    EXPECT_EQ(smi.getInboundPorts(), "8766,8080");
}

TEST(ServiceMeshTest, ExcludedPortsDefault) {
    ServiceMeshIntegration smi;
    EXPECT_EQ(smi.getExcludedPorts(), "8082,8769");
}

TEST(ServiceMeshTest, InboundPortsCustom) {
    ServiceMeshIntegration::Config cfg;
    cfg.inbound_ports = "8766";
    ServiceMeshIntegration smi(cfg);
    EXPECT_EQ(smi.getInboundPorts(), "8766");
}

// ─────────────────────────────────────────────────────────────────────────────
// isRunning state
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, NotRunningBeforeStart) {
    ServiceMeshIntegration smi;
    EXPECT_FALSE(smi.isRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// start() / stop() lifecycle (probe HTTP server, ephemeral port)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ServiceMeshTest, StartAndStop) {
    ServiceMeshIntegration::Config cfg;
    cfg.probe_port      = 8082;
    cfg.drain_timeout_ms = 0;  // Skip drain delay in tests
    ServiceMeshIntegration smi(cfg);

    ASSERT_FALSE(smi.isRunning());
    const bool started = smi.start();
    if (started) {
        EXPECT_TRUE(smi.isRunning());
        smi.stop();
        EXPECT_FALSE(smi.isRunning());
    }
    // If start() fails (port unavailable in CI), the instance must still be
    // in a clean non-running state.
    EXPECT_FALSE(smi.isRunning());
}

TEST(ServiceMeshTest, DoubleStartReturnsFalse) {
    ServiceMeshIntegration::Config cfg;
    cfg.probe_port       = 8083;
    cfg.drain_timeout_ms = 0;
    ServiceMeshIntegration smi(cfg);

    const bool first = smi.start();
    if (first) {
        EXPECT_TRUE(smi.isRunning());
        // Second start() while already running must return false.
        EXPECT_FALSE(smi.start());
        EXPECT_TRUE(smi.isRunning());  // still running
        smi.stop();
    }
    EXPECT_FALSE(smi.isRunning());
}

TEST(ServiceMeshTest, StopWithoutStartIsNoOp) {
    ServiceMeshIntegration smi;
    // stop() on a never-started instance must not crash.
    EXPECT_NO_THROW(smi.stop());
    EXPECT_FALSE(smi.isRunning());
}

#endif  // THEMIS_ENABLE_SERVICE_MESH
