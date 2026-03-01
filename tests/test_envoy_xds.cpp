// Unit tests for EnvoyXdsClient (include/network/envoy_xds.h).
//
// Validates configuration defaults, DiscoveryRequest construction, JSON
// response parsing (LDS, CDS, EDS, RDS), and statistics initialisation
// without requiring a live xDS management server.

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include "network/envoy_xds.h"
#include <string>
#include <vector>

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, DefaultControlPlanePort) {
    EnvoyXdsClient::Config cfg;
    EXPECT_EQ(cfg.control_plane_port, kXdsDefaultControlPlanePort);
    EXPECT_EQ(cfg.control_plane_port, 15010u);
}

TEST(EnvoyXdsClientTest, DefaultControlPlaneHost) {
    EnvoyXdsClient::Config cfg;
    EXPECT_EQ(cfg.control_plane_host, "istiod.istio-system.svc");
}

TEST(EnvoyXdsClientTest, DefaultNodeId) {
    EnvoyXdsClient::Config cfg;
    EXPECT_FALSE(cfg.node_id.empty());
    EXPECT_EQ(cfg.node_id, "themisdb-node");
}

TEST(EnvoyXdsClientTest, DefaultNodeCluster) {
    EnvoyXdsClient::Config cfg;
    EXPECT_EQ(cfg.node_cluster, "themisdb");
}

TEST(EnvoyXdsClientTest, DefaultPollInterval) {
    EnvoyXdsClient::Config cfg;
    EXPECT_EQ(cfg.poll_interval_ms, 15000u);
}

TEST(EnvoyXdsClientTest, DefaultReconnectInterval) {
    EnvoyXdsClient::Config cfg;
    EXPECT_EQ(cfg.reconnect_interval_ms, 5000u);
}

TEST(EnvoyXdsClientTest, DefaultRequestTimeout) {
    EnvoyXdsClient::Config cfg;
    EXPECT_EQ(cfg.request_timeout_ms, 10000u);
}

TEST(EnvoyXdsClientTest, DefaultSubscribeAllEnabled) {
    EnvoyXdsClient::Config cfg;
    EXPECT_TRUE(cfg.subscribe_listeners);
    EXPECT_TRUE(cfg.subscribe_clusters);
    EXPECT_TRUE(cfg.subscribe_endpoints);
    EXPECT_TRUE(cfg.subscribe_routes);
}

// ─────────────────────────────────────────────────────────────────────────────
// Protocol constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, TypeUrlListenerConstant) {
    EXPECT_EQ(std::string(kXdsTypeUrlListener),
              "type.googleapis.com/envoy.config.listener.v3.Listener");
}

TEST(EnvoyXdsClientTest, TypeUrlClusterConstant) {
    EXPECT_EQ(std::string(kXdsTypeUrlCluster),
              "type.googleapis.com/envoy.config.cluster.v3.Cluster");
}

TEST(EnvoyXdsClientTest, TypeUrlEndpointConstant) {
    EXPECT_EQ(std::string(kXdsTypeUrlEndpoint),
              "type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment");
}

TEST(EnvoyXdsClientTest, TypeUrlRouteConstant) {
    EXPECT_EQ(std::string(kXdsTypeUrlRoute),
              "type.googleapis.com/envoy.config.route.v3.RouteConfiguration");
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics initialisation
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, InitialStatsAllZero) {
    EnvoyXdsClient xds;
    const auto s = xds.getStats();
    EXPECT_EQ(s.lds_updates,    0u);
    EXPECT_EQ(s.cds_updates,    0u);
    EXPECT_EQ(s.eds_updates,    0u);
    EXPECT_EQ(s.rds_updates,    0u);
    EXPECT_EQ(s.lds_errors,     0u);
    EXPECT_EQ(s.cds_errors,     0u);
    EXPECT_EQ(s.eds_errors,     0u);
    EXPECT_EQ(s.rds_errors,     0u);
    EXPECT_EQ(s.connect_errors, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// isRunning state
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, NotRunningBeforeStart) {
    EnvoyXdsClient xds;
    EXPECT_FALSE(xds.isRunning());
}

TEST(EnvoyXdsClientTest, StopWithoutStartIsNoOp) {
    EnvoyXdsClient xds;
    EXPECT_NO_THROW(xds.stop());
    EXPECT_FALSE(xds.isRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// Version accessors (initially empty)
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, InitialVersionsEmpty) {
    EnvoyXdsClient xds;
    EXPECT_TRUE(xds.getListenerVersion().empty());
    EXPECT_TRUE(xds.getClusterVersion().empty());
    EXPECT_TRUE(xds.getRouteVersion().empty());
    EXPECT_TRUE(xds.getEndpointVersion().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// DiscoveryRequest builder
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestContainsTypeUrl) {
    EnvoyXdsClient::Config cfg;
    cfg.node_id      = "test-node";
    cfg.node_cluster = "test-cluster";
    EnvoyXdsClient xds(cfg);

    const std::string body = xds.buildDiscoveryRequest(
        kXdsTypeUrlCluster, "", "", {});

    EXPECT_NE(body.find(kXdsTypeUrlCluster), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestContainsNodeId) {
    EnvoyXdsClient::Config cfg;
    cfg.node_id      = "my-node-42";
    cfg.node_cluster = "my-cluster";
    EnvoyXdsClient xds(cfg);

    const std::string body = xds.buildDiscoveryRequest(
        kXdsTypeUrlListener, "", "", {});

    EXPECT_NE(body.find("my-node-42"), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestContainsNodeCluster) {
    EnvoyXdsClient::Config cfg;
    cfg.node_id      = "node";
    cfg.node_cluster = "production-cluster";
    EnvoyXdsClient xds(cfg);

    const std::string body = xds.buildDiscoveryRequest(
        kXdsTypeUrlCluster, "v1", "nonce1", {});

    EXPECT_NE(body.find("production-cluster"), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestContainsVersionInfo) {
    EnvoyXdsClient xds;
    const std::string body =
        xds.buildDiscoveryRequest(kXdsTypeUrlCluster, "2024-01", "abc", {});
    EXPECT_NE(body.find("2024-01"), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestContainsNonce) {
    EnvoyXdsClient xds;
    const std::string body =
        xds.buildDiscoveryRequest(kXdsTypeUrlCluster, "", "my-nonce", {});
    EXPECT_NE(body.find("my-nonce"), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestWithResourceNames) {
    EnvoyXdsClient xds;
    const std::string body =
        xds.buildDiscoveryRequest(kXdsTypeUrlCluster, "", "",
                                  {"cluster-a", "cluster-b"});
    EXPECT_NE(body.find("cluster-a"), std::string::npos);
    EXPECT_NE(body.find("cluster-b"), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestValidJSON) {
    EnvoyXdsClient xds;
    const std::string body =
        xds.buildDiscoveryRequest(kXdsTypeUrlCluster, "v1", "n1", {"svc"});
    // Must start with '{' and end with '}'
    EXPECT_FALSE(body.empty());
    EXPECT_EQ(body.front(), '{');
    EXPECT_EQ(body.back(),  '}');
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestEscapesSpecialChars) {
    EnvoyXdsClient::Config cfg;
    cfg.node_id      = "node\"with\"quotes";
    cfg.node_cluster = "cluster";
    EnvoyXdsClient xds(cfg);

    const std::string body =
        xds.buildDiscoveryRequest(kXdsTypeUrlCluster, "", "", {});
    // The node_id with quotes should be escaped, not break JSON structure
    EXPECT_NE(body.find("\\\""), std::string::npos);
}

TEST(EnvoyXdsClientTest, BuildDiscoveryRequestWithMetadata) {
    EnvoyXdsClient::Config cfg;
    cfg.node_id       = "n";
    cfg.node_cluster  = "c";
    cfg.node_metadata = {{"app", "themisdb"}, {"version", "1.5.0"}};
    EnvoyXdsClient xds(cfg);

    const std::string body =
        xds.buildDiscoveryRequest(kXdsTypeUrlCluster, "", "", {});
    EXPECT_NE(body.find("themisdb"),  std::string::npos);
    EXPECT_NE(body.find("1.5.0"),     std::string::npos);
    EXPECT_NE(body.find("metadata"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// DiscoveryResponse parser
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, ParseDiscoveryResponseEmptyReturnsFalse) {
    std::string v, n, r;
    EXPECT_FALSE(EnvoyXdsClient::parseDiscoveryResponse("", v, n, r));
}

TEST(EnvoyXdsClientTest, ParseDiscoveryResponseMissingVersionReturnsFalse) {
    const std::string json = R"({"nonce":"abc","resources":[]})";
    std::string v, n, r;
    EXPECT_FALSE(EnvoyXdsClient::parseDiscoveryResponse(json, v, n, r));
}

TEST(EnvoyXdsClientTest, ParseDiscoveryResponseExtractsVersion) {
    const std::string json =
        R"({"version_info":"2024-01-01","nonce":"n1","resources":[],"type_url":"x"})";
    std::string v, n, r;
    ASSERT_TRUE(EnvoyXdsClient::parseDiscoveryResponse(json, v, n, r));
    EXPECT_EQ(v, "2024-01-01");
}

TEST(EnvoyXdsClientTest, ParseDiscoveryResponseExtractsNonce) {
    const std::string json =
        R"({"version_info":"v1","nonce":"my-nonce","resources":[],"type_url":"x"})";
    std::string v, n, r;
    ASSERT_TRUE(EnvoyXdsClient::parseDiscoveryResponse(json, v, n, r));
    EXPECT_EQ(n, "my-nonce");
}

TEST(EnvoyXdsClientTest, ParseDiscoveryResponseExtractsResources) {
    const std::string json =
        R"({"version_info":"v1","nonce":"n","resources":[{"name":"c1"}],"type_url":"x"})";
    std::string v, n, r;
    ASSERT_TRUE(EnvoyXdsClient::parseDiscoveryResponse(json, v, n, r));
    EXPECT_NE(r.find("c1"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// LDS (Listener) parser
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, ParseListenersEmptyArray) {
    const auto result = EnvoyXdsClient::parseListeners("[]");
    EXPECT_TRUE(result.empty());
}

TEST(EnvoyXdsClientTest, ParseListenersInvalidInputEmpty) {
    const auto result = EnvoyXdsClient::parseListeners("");
    EXPECT_TRUE(result.empty());
}

TEST(EnvoyXdsClientTest, ParseListenersSingleEntry) {
    const std::string json = R"([{
        "name": "0.0.0.0_8080",
        "address": {
            "socket_address": {
                "address": "0.0.0.0",
                "port_value": 8080,
                "protocol": "TCP"
            }
        }
    }])";
    const auto result = EnvoyXdsClient::parseListeners(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name,     "0.0.0.0_8080");
    EXPECT_EQ(result[0].address,  "0.0.0.0");
    EXPECT_EQ(result[0].port,     8080u);
    EXPECT_EQ(result[0].protocol, "TCP");
}

TEST(EnvoyXdsClientTest, ParseListenersDefaultProtocolTCP) {
    const std::string json = R"([{
        "name": "listener-1",
        "address": {
            "socket_address": {
                "address": "10.0.0.1",
                "port_value": 9000
            }
        }
    }])";
    const auto result = EnvoyXdsClient::parseListeners(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].protocol, "TCP");
}

TEST(EnvoyXdsClientTest, ParseListenersMultipleEntries) {
    const std::string json = R"([
        {"name":"l1","address":{"socket_address":{"address":"0.0.0.0","port_value":8080}}},
        {"name":"l2","address":{"socket_address":{"address":"0.0.0.0","port_value":8443}}}
    ])";
    const auto result = EnvoyXdsClient::parseListeners(json);
    EXPECT_EQ(result.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CDS (Cluster) parser
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, ParseClustersEmptyArray) {
    const auto result = EnvoyXdsClient::parseClusters("[]");
    EXPECT_TRUE(result.empty());
}

TEST(EnvoyXdsClientTest, ParseClustersSingleCluster) {
    const std::string json = R"([{
        "name": "outbound|8080||service-a.default.svc.cluster.local",
        "type": "EDS",
        "lb_policy": "LEAST_REQUEST"
    }])";
    const auto result = EnvoyXdsClient::parseClusters(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name,      "outbound|8080||service-a.default.svc.cluster.local");
    EXPECT_EQ(result[0].type,      "EDS");
    EXPECT_EQ(result[0].lb_policy, "LEAST_REQUEST");
}

TEST(EnvoyXdsClientTest, ParseClustersDefaultLbPolicy) {
    const std::string json = R"([{"name":"my-cluster","type":"STATIC"}])";
    const auto result = EnvoyXdsClient::parseClusters(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].lb_policy, "ROUND_ROBIN");
}

TEST(EnvoyXdsClientTest, ParseClustersWithInlineEndpoints) {
    const std::string json = R"([{
        "name": "static-cluster",
        "type": "STATIC",
        "lb_policy": "ROUND_ROBIN",
        "load_assignment": {
            "cluster_name": "static-cluster",
            "endpoints": [{
                "lb_endpoints": [{
                    "endpoint": {
                        "address": {
                            "socket_address": {
                                "address": "192.168.1.1",
                                "port_value": 8080
                            }
                        }
                    },
                    "health_status": "HEALTHY",
                    "load_balancing_weight": 100
                }]
            }]
        }
    }])";
    const auto result = EnvoyXdsClient::parseClusters(json);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_EQ(result[0].endpoints.size(), 1u);
    EXPECT_EQ(result[0].endpoints[0].address, "192.168.1.1");
    EXPECT_EQ(result[0].endpoints[0].port,    8080u);
    EXPECT_EQ(result[0].endpoints[0].weight,  100u);
    EXPECT_EQ(result[0].endpoints[0].health_status, "HEALTHY");
}

TEST(EnvoyXdsClientTest, ParseClustersEndpointDefaultHealthy) {
    const std::string json = R"([{
        "name": "c1",
        "type": "STATIC",
        "load_assignment": {
            "endpoints": [{
                "lb_endpoints": [{
                    "endpoint": {
                        "address": {
                            "socket_address": {"address": "10.0.0.1","port_value": 9090}
                        }
                    }
                }]
            }]
        }
    }])";
    const auto result = EnvoyXdsClient::parseClusters(json);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_EQ(result[0].endpoints.size(), 1u);
    EXPECT_EQ(result[0].endpoints[0].health_status, "HEALTHY");
}

// ─────────────────────────────────────────────────────────────────────────────
// EDS (Endpoint) parser
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, ParseEndpointsEmptyArray) {
    const auto result = EnvoyXdsClient::parseEndpoints("[]");
    EXPECT_TRUE(result.empty());
}

TEST(EnvoyXdsClientTest, ParseEndpointsSingleAssignment) {
    const std::string json = R"([{
        "cluster_name": "outbound|80||svc.ns.svc.cluster.local",
        "endpoints": [{
            "lb_endpoints": [{
                "endpoint": {
                    "address": {
                        "socket_address": {"address": "172.16.0.5","port_value": 80}
                    }
                },
                "health_status": "HEALTHY"
            }]
        }]
    }])";
    const auto result = EnvoyXdsClient::parseEndpoints(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name, "outbound|80||svc.ns.svc.cluster.local");
    ASSERT_EQ(result[0].endpoints.size(), 1u);
    EXPECT_EQ(result[0].endpoints[0].address, "172.16.0.5");
    EXPECT_EQ(result[0].endpoints[0].port,    80u);
}

TEST(EnvoyXdsClientTest, ParseEndpointsMultipleEndpoints) {
    const std::string json = R"([{
        "cluster_name": "cluster-1",
        "endpoints": [{
            "lb_endpoints": [
                {"endpoint":{"address":{"socket_address":{"address":"10.0.0.1","port_value":8080}}}},
                {"endpoint":{"address":{"socket_address":{"address":"10.0.0.2","port_value":8080}}}}
            ]
        }]
    }])";
    const auto result = EnvoyXdsClient::parseEndpoints(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].endpoints.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// RDS (Route) parser
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, ParseRoutesEmptyArray) {
    const auto result = EnvoyXdsClient::parseRoutes("[]");
    EXPECT_TRUE(result.empty());
}

TEST(EnvoyXdsClientTest, ParseRoutesSingleVirtualHost) {
    const std::string json = R"([{
        "name": "local_route",
        "virtual_hosts": [{
            "name": "service-a",
            "domains": ["service-a","service-a.default.svc.cluster.local"],
            "routes": [{
                "match": {"prefix": "/api/"},
                "route": {"cluster": "outbound|8080||service-a.default.svc.cluster.local"}
            }]
        }]
    }])";
    const auto result = EnvoyXdsClient::parseRoutes(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].name, "service-a");
    ASSERT_EQ(result[0].domains.size(), 2u);
    EXPECT_EQ(result[0].domains[0], "service-a");
    ASSERT_EQ(result[0].routes.size(), 1u);
    EXPECT_EQ(result[0].routes[0].prefix,       "/api/");
    EXPECT_EQ(result[0].routes[0].cluster_name,
              "outbound|8080||service-a.default.svc.cluster.local");
}

TEST(EnvoyXdsClientTest, ParseRoutesMultipleVirtualHosts) {
    const std::string json = R"([{
        "name": "ingress",
        "virtual_hosts": [
            {
                "name": "vh-a",
                "domains": ["a.example.com"],
                "routes": [{"match":{"prefix":"/"},"route":{"cluster":"c-a"}}]
            },
            {
                "name": "vh-b",
                "domains": ["b.example.com"],
                "routes": [{"match":{"prefix":"/"},"route":{"cluster":"c-b"}}]
            }
        ]
    }])";
    const auto result = EnvoyXdsClient::parseRoutes(json);
    EXPECT_EQ(result.size(), 2u);
}

TEST(EnvoyXdsClientTest, ParseRoutesMultipleRoutesPerVirtualHost) {
    const std::string json = R"([{
        "name": "rc",
        "virtual_hosts": [{
            "name": "vh",
            "domains": ["*"],
            "routes": [
                {"match":{"prefix":"/api/v1/"},"route":{"cluster":"cluster-v1"}},
                {"match":{"prefix":"/api/v2/"},"route":{"cluster":"cluster-v2"}},
                {"match":{"prefix":"/"},"route":{"cluster":"cluster-default"}}
            ]
        }]
    }])";
    const auto result = EnvoyXdsClient::parseRoutes(json);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].routes.size(), 3u);
    EXPECT_EQ(result[0].routes[0].prefix,       "/api/v1/");
    EXPECT_EQ(result[0].routes[0].cluster_name, "cluster-v1");
    EXPECT_EQ(result[0].routes[2].cluster_name, "cluster-default");
}

TEST(EnvoyXdsClientTest, ParseRoutesNoRoutes) {
    const std::string json = R"([{
        "name": "rc",
        "virtual_hosts": [{
            "name": "vh",
            "domains": ["*"],
            "routes": []
        }]
    }])";
    const auto result = EnvoyXdsClient::parseRoutes(json);
    // VH with no valid routes (cluster_name empty) should still be returned
    // if it has a name.
    ASSERT_EQ(result.size(), 1u);
    EXPECT_TRUE(result[0].routes.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Callback registration (smoke test – just ensure no crash)
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, SetListenerCallbackNoThrow) {
    EnvoyXdsClient xds;
    EXPECT_NO_THROW(xds.setListenerCallback(
        [](const std::vector<EnvoyXdsClient::ListenerInfo>&) {}));
}

TEST(EnvoyXdsClientTest, SetClusterCallbackNoThrow) {
    EnvoyXdsClient xds;
    EXPECT_NO_THROW(xds.setClusterCallback(
        [](const std::vector<EnvoyXdsClient::ClusterInfo>&) {}));
}

TEST(EnvoyXdsClientTest, SetRouteCallbackNoThrow) {
    EnvoyXdsClient xds;
    EXPECT_NO_THROW(xds.setRouteCallback(
        [](const std::vector<EnvoyXdsClient::VirtualHostInfo>&) {}));
}

TEST(EnvoyXdsClientTest, SetEndpointCallbackNoThrow) {
    EnvoyXdsClient xds;
    EXPECT_NO_THROW(xds.setEndpointCallback(
        [](const std::vector<EnvoyXdsClient::ClusterInfo>&) {}));
}

// ─────────────────────────────────────────────────────────────────────────────
// start() / stop() lifecycle (no live control plane)
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnvoyXdsClientTest, StartAndStop) {
    EnvoyXdsClient::Config cfg;
    // Use a very long poll interval so the polling thread does not fire
    // during the test and attempt a real network connection.
    cfg.poll_interval_ms     = 60000;
    cfg.reconnect_interval_ms = 60000;
    // Point at localhost on a port that is almost certainly not listening –
    // the thread will fail to connect and back off immediately.
    cfg.control_plane_host = "127.0.0.1";
    cfg.control_plane_port = 19999;
    cfg.request_timeout_ms = 100;

    EnvoyXdsClient xds(cfg);
    EXPECT_FALSE(xds.isRunning());

    EXPECT_TRUE(xds.start());
    EXPECT_TRUE(xds.isRunning());

    xds.stop();
    EXPECT_FALSE(xds.isRunning());
}

TEST(EnvoyXdsClientTest, DoubleStartReturnsFalse) {
    EnvoyXdsClient::Config cfg;
    cfg.poll_interval_ms      = 60000;
    cfg.reconnect_interval_ms = 60000;
    cfg.control_plane_host    = "127.0.0.1";
    cfg.control_plane_port    = 19999;
    cfg.request_timeout_ms    = 100;

    EnvoyXdsClient xds(cfg);
    ASSERT_TRUE(xds.start());
    EXPECT_FALSE(xds.start());  // second start must return false
    xds.stop();
}

#endif  // THEMIS_ENABLE_SERVICE_MESH
