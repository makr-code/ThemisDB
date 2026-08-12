// Unit tests for ServiceMeshApiHandler (include/server/service_mesh_api_handler.h).
//
// These tests validate:
//  - Disabled-mode responses when THEMIS_ENABLE_SERVICE_MESH is not set
//  - Disabled-mode responses when no ServiceMeshIntegration is provided
//  - Status, config, and annotations endpoints with a live integration
//    (guarded by THEMIS_ENABLE_SERVICE_MESH)

#include <gtest/gtest.h>
#include "server/service_mesh_api_handler.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;
using namespace themis::server;

// Helper: build a minimal GET request.
static http::request<http::string_body> makeGet(const std::string& target) {
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.prepare_payload();
    return req;
}

// ─────────────────────────────────────────────────────────────────────────────
// Disabled mode (no ServiceMeshIntegration supplied)
// ─────────────────────────────────────────────────────────────────────────────

class ServiceMeshApiHandlerDisabledTest : public ::testing::Test {
protected:
    // Construct handler without any integration → disabled / not-started
    ServiceMeshApiHandler handler;
};

TEST_F(ServiceMeshApiHandlerDisabledTest, Status_Returns200) {
    auto res = handler.handleStatus(makeGet("/api/v1/service-mesh/status"));
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Status_ContentTypeIsJson) {
    auto res = handler.handleStatus(makeGet("/api/v1/service-mesh/status"));
    EXPECT_EQ(std::string(res[http::field::content_type]), "application/json");
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Status_BodyIsValidJson) {
    auto res = handler.handleStatus(makeGet("/api/v1/service-mesh/status"));
    EXPECT_NO_THROW((void)json::parse(res.body()));
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Status_ContainsEnabledOrMessage) {
    auto res = handler.handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    // Either "enabled": false (no compile-time support)
    // or "enabled": true + "message" (compile-time support but no instance)
    EXPECT_TRUE(body.contains("enabled") || body.contains("message"));
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Config_Returns200) {
    auto res = handler.handleConfig(makeGet("/api/v1/service-mesh/config"));
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Config_BodyIsValidJson) {
    auto res = handler.handleConfig(makeGet("/api/v1/service-mesh/config"));
    EXPECT_NO_THROW((void)json::parse(res.body()));
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Annotations_Returns200) {
    auto res = handler.handleAnnotations(makeGet("/api/v1/service-mesh/annotations"));
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServiceMeshApiHandlerDisabledTest, Annotations_BodyIsValidJson) {
    auto res = handler.handleAnnotations(makeGet("/api/v1/service-mesh/annotations"));
    EXPECT_NO_THROW((void)json::parse(res.body()));
}

// ─────────────────────────────────────────────────────────────────────────────
// Enabled mode – tests only compiled when THEMIS_ENABLE_SERVICE_MESH is set
// ─────────────────────────────────────────────────────────────────────────────

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include "network/service_mesh.h"
using namespace themis::network;

class ServiceMeshApiHandlerEnabledTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use default config; do NOT start the probe server to avoid
        // binding a real port in unit tests.
        smi_ = std::make_shared<ServiceMeshIntegration>();
        handler_ = std::make_unique<ServiceMeshApiHandler>(smi_);
    }

    std::shared_ptr<ServiceMeshIntegration> smi_;
    std::unique_ptr<ServiceMeshApiHandler>  handler_;
};

// ── Status endpoint ──────────────────────────────────────────────────────────

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_Returns200) {
    auto res = handler_->handleStatus(makeGet("/api/v1/service-mesh/status"));
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_EnabledIsTrue) {
    auto res = handler_->handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("enabled"));
    EXPECT_TRUE(body["enabled"].get<bool>());
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_RunningIsFalseBeforeStart) {
    auto res = handler_->handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("running"));
    EXPECT_FALSE(body["running"].get<bool>());
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_ContainsAddressField) {
    auto res = handler_->handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("address"));
    EXPECT_FALSE(body["address"].get<std::string>().empty());
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_ContainsStatsObject) {
    auto res = handler_->handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("stats"));
    const auto& stats = body["stats"];
    EXPECT_TRUE(stats.contains("healthz_requests"));
    EXPECT_TRUE(stats.contains("readyz_requests"));
    EXPECT_TRUE(stats.contains("healthz_ok"));
    EXPECT_TRUE(stats.contains("readyz_ok"));
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_InitialStatsAreZero) {
    auto res = handler_->handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    const auto& stats = body["stats"];
    EXPECT_EQ(stats["healthz_requests"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["readyz_requests"].get<uint64_t>(),  0u);
    EXPECT_EQ(stats["healthz_ok"].get<uint64_t>(),       0u);
    EXPECT_EQ(stats["readyz_ok"].get<uint64_t>(),        0u);
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Status_TlsOffloadedMatchesConfig) {
    ServiceMeshIntegration::Config cfg;
    cfg.trust_sidecar_mtls = true;
    auto smi  = std::make_shared<ServiceMeshIntegration>(cfg);
    ServiceMeshApiHandler h{smi};

    auto res  = h.handleStatus(makeGet("/api/v1/service-mesh/status"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("tls_offloaded"));
    EXPECT_TRUE(body["tls_offloaded"].get<bool>());
}

// ── Config endpoint ──────────────────────────────────────────────────────────

TEST_F(ServiceMeshApiHandlerEnabledTest, Config_Returns200) {
    auto res = handler_->handleConfig(makeGet("/api/v1/service-mesh/config"));
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Config_ContainsRequiredFields) {
    auto res  = handler_->handleConfig(makeGet("/api/v1/service-mesh/config"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains("probe_port"));
    EXPECT_TRUE(body.contains("inbound_ports"));
    EXPECT_TRUE(body.contains("excluded_ports"));
    EXPECT_TRUE(body.contains("tls_offloaded_to_sidecar"));
    EXPECT_TRUE(body.contains("address"));
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Config_DefaultProbePort) {
    auto res  = handler_->handleConfig(makeGet("/api/v1/service-mesh/config"));
    auto body = json::parse(res.body());
    EXPECT_EQ(body["probe_port"].get<uint16_t>(),
              themis::network::kServiceMeshProbeDefaultPort);
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Config_DefaultInboundPorts) {
    auto res  = handler_->handleConfig(makeGet("/api/v1/service-mesh/config"));
    auto body = json::parse(res.body());
    EXPECT_EQ(body["inbound_ports"].get<std::string>(), "8766,8080");
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Config_DefaultExcludedPorts) {
    auto res  = handler_->handleConfig(makeGet("/api/v1/service-mesh/config"));
    auto body = json::parse(res.body());
    EXPECT_EQ(body["excluded_ports"].get<std::string>(), "8082,8769");
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Config_ReflectsCustomProbePort) {
    ServiceMeshIntegration::Config cfg;
    cfg.probe_port = 8099;
    auto smi  = std::make_shared<ServiceMeshIntegration>(cfg);
    ServiceMeshApiHandler h{smi};

    auto res  = h.handleConfig(makeGet("/api/v1/service-mesh/config"));
    auto body = json::parse(res.body());
    EXPECT_EQ(body["probe_port"].get<uint16_t>(), 8099u);
}

// ── Annotations endpoint ─────────────────────────────────────────────────────

TEST_F(ServiceMeshApiHandlerEnabledTest, Annotations_Returns200) {
    auto res = handler_->handleAnnotations(
        makeGet("/api/v1/service-mesh/annotations"));
    EXPECT_EQ(res.result(), http::status::ok);
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Annotations_ContainsIstioKeys) {
    auto res  = handler_->handleAnnotations(
        makeGet("/api/v1/service-mesh/annotations"));
    auto body = json::parse(res.body());
    EXPECT_TRUE(body.contains(
        "traffic.sidecar.istio.io/includeInboundPorts"));
    EXPECT_TRUE(body.contains(
        "traffic.sidecar.istio.io/excludeInboundPorts"));
}

TEST_F(ServiceMeshApiHandlerEnabledTest, Annotations_ValuesMatchConfig) {
    auto res  = handler_->handleAnnotations(
        makeGet("/api/v1/service-mesh/annotations"));
    auto body = json::parse(res.body());
    EXPECT_EQ(
        body["traffic.sidecar.istio.io/includeInboundPorts"].get<std::string>(),
        smi_->getInboundPorts());
    EXPECT_EQ(
        body["traffic.sidecar.istio.io/excludeInboundPorts"].get<std::string>(),
        smi_->getExcludedPorts());
}

#endif  // THEMIS_ENABLE_SERVICE_MESH
