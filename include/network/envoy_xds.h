/**
 * @file envoy_xds.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Envoy xDS v3 REST client for service mesh sidecar proxy mode.
//
// Implements a lightweight Envoy xDS v3 REST/JSON client that subscribes to an
// xDS management server (e.g. Istio Pilot / Istiod) for dynamic configuration.
// This enables ThemisDB to operate in a service mesh sidecar proxy mode where
// routing, cluster, and listener configuration is pushed from the control plane.
//
// Supported discovery services:
//   - LDS (Listener Discovery Service) – inbound/outbound listener config
//   - CDS (Cluster Discovery Service) – upstream cluster definitions
//   - EDS (Endpoint Discovery Service) – cluster endpoint resolution
//   - RDS (Route Discovery Service)   – HTTP route configuration
//
// Protocol: xDS v3 REST (JSON) via HTTP/1.1 long-polling.
//   POST /v3/discovery:listeners  (LDS)
//   POST /v3/discovery:routes     (RDS)
//   POST /v3/discovery:clusters   (CDS)
//   POST /v3/discovery:endpoints  (EDS)
//
// Design constraints:
//   - No dependency on Envoy or Istio SDKs; JSON-over-HTTP avoids protobuf
//     library requirements (consistent with service_mesh.h approach)
//   - Transport-layer only: received xDS config is surfaced via callbacks;
//     the caller decides how to apply it to routing/connection logic
//   - Guarded by THEMIS_ENABLE_SERVICE_MESH
//   - Follows the same lifecycle pattern as ServiceMeshIntegration:
//     construct → start() → [callbacks receive updates] → stop()
//
// xDS REST request/response format (JSON):
//   Request body (DiscoveryRequest):
//     { "node": {"id": "<node_id>", "cluster": "<cluster>"},
//       "version_info": "<last_version_or_empty>",
//       "resource_names": [],
//       "response_nonce": "<last_nonce_or_empty>" }
//   Response body (DiscoveryResponse):
//     { "version_info": "<version>",
//       "resources": [ { "@type": "<type_url>", ... } ],
//       "type_url": "<type_url>",
//       "nonce": "<nonce>" }
//
// Port allocation:
//   ThemisDB default xDS control-plane port: 15010 (Istiod plaintext xDS REST)
//   Istiod secure xDS (gRPC/TLS): 15012 (not used by this client)

#pragma once

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace network {

/// Default port for the Istiod/Pilot xDS management server (plaintext REST).
constexpr uint16_t kXdsDefaultControlPlanePort = 15010;

/// xDS v3 type URL prefix.
constexpr char kXdsTypeUrlPrefix[] = "type.googleapis.com/";

/// xDS v3 Listener type URL.
constexpr char kXdsTypeUrlListener[] =
    "type.googleapis.com/envoy.config.listener.v3.Listener";

/// xDS v3 Cluster type URL.
constexpr char kXdsTypeUrlCluster[] =
    "type.googleapis.com/envoy.config.cluster.v3.Cluster";

/// xDS v3 ClusterLoadAssignment (EDS) type URL.
constexpr char kXdsTypeUrlEndpoint[] =
    "type.googleapis.com/envoy.config.endpoint.v3.ClusterLoadAssignment";

/// xDS v3 RouteConfiguration (RDS) type URL.
constexpr char kXdsTypeUrlRoute[] =
    "type.googleapis.com/envoy.config.route.v3.RouteConfiguration";

// ─────────────────────────────────────────────────────────────────────────────
// EnvoyXdsClient
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Envoy xDS v3 REST client for ThemisDB service mesh sidecar proxy mode.
 *
 * Periodically polls an xDS management server (Istiod/Pilot) for dynamic
 * configuration updates and delivers them to registered callbacks.
 *
 * Lifecycle:
 * @code
 *   EnvoyXdsClient::Config cfg;
 *   cfg.control_plane_host = "istiod.istio-system.svc";
 *   cfg.node_id            = "sidecar~10.0.0.1~themisdb-pod.default~default.svc.cluster.local";
 *   cfg.node_cluster       = "themisdb";
 *
 *   EnvoyXdsClient xds(cfg);
 *   xds.setClusterCallback([](const auto& clusters) {
 *       for (const auto& c : clusters) { // apply cluster config ... }
 *   });
 *   xds.start();
 *   // ... run ...
 *   xds.stop();
 * @endcode
 *
 * Thread safety: start()/stop() must not be called concurrently.
 *                Callbacks are invoked from the polling thread.
 */
class EnvoyXdsClient {
public:
    // ── Data structures ──────────────────────────────────────────────────────

    /// A single resolved endpoint within an xDS cluster.
    struct ClusterEndpoint {
        std::string address;       ///< IP or hostname
        uint16_t    port   = 0;    ///< TCP port
        uint32_t    weight = 100;  ///< Load-balancing weight (0 = use default)
        std::string health_status; ///< "HEALTHY", "UNHEALTHY", "DRAINING", ...
    };

    /// An upstream cluster as received via CDS/EDS.
    struct ClusterInfo {
        std::string                  name;       ///< Cluster name
        std::string                  lb_policy;  ///< "ROUND_ROBIN", "LEAST_REQUEST", ...
        std::string                  type;       ///< "STATIC", "EDS", "LOGICAL_DNS", ...
        std::vector<ClusterEndpoint> endpoints;  ///< Resolved endpoints (EDS or inline)
    };

    /// An inbound or outbound listener as received via LDS.
    struct ListenerInfo {
        std::string name;     ///< Listener name (e.g. "0.0.0.0_8080")
        std::string address;  ///< Bind address
        uint16_t    port = 0; ///< Bind port
        std::string protocol; ///< "TCP", "HTTP", "HTTPS"
    };

    /// A single route rule within a virtual host.
    struct RouteInfo {
        std::string prefix;        ///< Path prefix match (e.g. "/api/")
        std::string cluster_name;  ///< Target cluster
        std::string method;        ///< HTTP method match ("" = all methods)
        uint32_t    timeout_ms = 0;///< Per-route timeout (0 = inherit)
    };

    /// A virtual host (group of domains + routes) from RDS.
    struct VirtualHostInfo {
        std::string              name;
        std::vector<std::string> domains; ///< e.g. ["service.ns.svc", "*"]
        std::vector<RouteInfo>   routes;
    };

    // ── Callback types ───────────────────────────────────────────────────────

    using ListenerCallback   = std::function<void(const std::vector<ListenerInfo>&)>;
    using ClusterCallback    = std::function<void(const std::vector<ClusterInfo>&)>;
    using RouteCallback      = std::function<void(const std::vector<VirtualHostInfo>&)>;
    using EndpointCallback   = std::function<void(const std::vector<ClusterInfo>&)>;

    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        /// xDS management server hostname (Istiod/Pilot).
        std::string control_plane_host = "istiod.istio-system.svc";

        /// xDS management server port (15010 = Istiod plaintext REST).
        uint16_t control_plane_port = kXdsDefaultControlPlanePort;

        /// Envoy node identifier.
        /// Istiod format: "sidecar~<ip>~<pod_name>.<namespace>~<namespace>.svc.cluster.local"
        /// Example: "sidecar~10.0.0.1~themisdb-pod.default~default.svc.cluster.local"
        std::string node_id = "themisdb-node";

        /// Service cluster name (matches Kubernetes service name).
        std::string node_cluster = "themisdb";

        /// Optional arbitrary metadata key/value pairs reported to the control plane.
        std::vector<std::pair<std::string, std::string>> node_metadata;

        /// Interval between successive xDS polls when the server returns the
        /// same version (steady-state heartbeat).  Default: 15 s.
        uint32_t poll_interval_ms = 15000;

        /// Reconnect / back-off interval on control-plane connectivity errors.
        uint32_t reconnect_interval_ms = 5000;

        /// Per-request HTTP timeout for xDS REST calls.
        uint32_t request_timeout_ms = 10000;

        /// Enable LDS (Listener Discovery Service) subscription.
        bool subscribe_listeners = true;

        /// Enable CDS (Cluster Discovery Service) subscription.
        bool subscribe_clusters = true;

        /// Enable EDS (Endpoint Discovery Service) subscription.
        bool subscribe_endpoints = true;

        /// Enable RDS (Route Discovery Service) subscription.
        bool subscribe_routes = true;

        Config() = default;
    };

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t lds_updates  = 0; ///< Successful LDS responses applied
        uint64_t cds_updates  = 0; ///< Successful CDS responses applied
        uint64_t eds_updates  = 0; ///< Successful EDS responses applied
        uint64_t rds_updates  = 0; ///< Successful RDS responses applied
        uint64_t lds_errors   = 0; ///< LDS request/parse errors
        uint64_t cds_errors   = 0; ///< CDS request/parse errors
        uint64_t eds_errors   = 0; ///< EDS request/parse errors
        uint64_t rds_errors   = 0; ///< RDS request/parse errors
        uint64_t connect_errors = 0; ///< Control-plane connection failures
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    explicit EnvoyXdsClient(const Config& config = Config{});
    ~EnvoyXdsClient();

    /// Start the background polling thread.
    /// @return true on success; false if already running.
    bool start();

    /// Stop the background polling thread and release resources.
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    // ── Callbacks ────────────────────────────────────────────────────────────

    void setListenerCallback(ListenerCallback cb);
    void setClusterCallback(ClusterCallback cb);
    void setRouteCallback(RouteCallback cb);
    void setEndpointCallback(EndpointCallback cb);

    // ── Accessors ────────────────────────────────────────────────────────────

    Stats getStats() const;

    /// @return The last received xDS version for LDS (empty if never received).
    std::string getListenerVersion() const;

    /// @return The last received xDS version for CDS (empty if never received).
    std::string getClusterVersion() const;

    /// @return The last received xDS version for RDS (empty if never received).
    std::string getRouteVersion() const;

    /// @return The last received xDS version for EDS (empty if never received).
    std::string getEndpointVersion() const;

    // ── Helpers (public for unit-test access) ────────────────────────────────

    /// Build the JSON body for an xDS DiscoveryRequest.
    /// @param type_url    xDS resource type URL (e.g. kXdsTypeUrlCluster)
    /// @param version     Last received version_info (empty for initial request)
    /// @param nonce       Last received response nonce (empty for initial)
    /// @param names       Resource names to subscribe to (empty = all)
    std::string buildDiscoveryRequest(const std::string&              type_url,
                                      const std::string&              version,
                                      const std::string&              nonce,
                                      const std::vector<std::string>& names) const;

    /// Parse the JSON body of an xDS DiscoveryResponse.
    /// Extracts version_info, nonce, and the resources array as a JSON string.
    /// @return true if parsing succeeded and version differs from @p current_version.
    static bool parseDiscoveryResponse(const std::string& json_body,
                                       std::string&       out_version,
                                       std::string&       out_nonce,
                                       std::string&       out_resources_json);

    /// Parse LDS resources JSON array into ListenerInfo vector.
    static std::vector<ListenerInfo>    parseListeners(const std::string& resources_json);

    /// Parse CDS resources JSON array into ClusterInfo vector.
    static std::vector<ClusterInfo>     parseClusters(const std::string& resources_json);

    /// Parse EDS resources JSON array into ClusterInfo endpoint vectors.
    static std::vector<ClusterInfo>     parseEndpoints(const std::string& resources_json);

    /// Parse RDS resources JSON array into VirtualHostInfo vector.
    static std::vector<VirtualHostInfo> parseRoutes(const std::string& resources_json);

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    /// Main polling loop executed on poll_thread_.
    void pollLoop();

    /// Poll a single xDS endpoint and invoke callback on update.
    /// @param out_resources_json  Populated with the resources JSON array on update.
    /// @return true if a new version was received and applied; false on no change or error.
    bool pollDiscoveryService(const std::string&              type_url,
                              const std::string&              rest_path,
                              std::string&                    inout_version,
                              std::string&                    inout_nonce,
                              std::string&                    out_resources_json,
                              bool&                           out_error);

    /// Perform an HTTP/1.1 POST request to the control plane.
    /// @return HTTP response body on success; empty string on failure.
    std::string httpPost(const std::string& path,
                         const std::string& body,
                         int*               out_status_code = nullptr) const;

    // ── Members ──────────────────────────────────────────────────────────────

    Config config_;

    std::atomic<bool>       running_{false};
    std::thread             poll_thread_;
    mutable std::mutex      wake_mutex_;
    std::condition_variable wake_cv_;

    // Per-resource-type version and nonce tracking (protected by versions_mutex_).
    mutable std::mutex versions_mutex_;
    std::string lds_version_;
    std::string cds_version_;
    std::string eds_version_;
    std::string rds_version_;
    std::string lds_nonce_;
    std::string cds_nonce_;
    std::string eds_nonce_;
    std::string rds_nonce_;

    // Callbacks (protected by callbacks_mutex_).
    mutable std::mutex callbacks_mutex_;
    ListenerCallback   listener_cb_;
    ClusterCallback    cluster_cb_;
    EndpointCallback   endpoint_cb_;
    RouteCallback      route_cb_;

    // Statistics (protected by stats_mutex_).
    mutable std::mutex stats_mutex_;
    mutable Stats      stats_{};
};

}  // namespace network
}  // namespace themis

#endif  // THEMIS_ENABLE_SERVICE_MESH
