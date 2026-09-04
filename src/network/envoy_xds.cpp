/**
 * @file envoy_xds.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=23, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – Envoy xDS v3 REST client for service mesh sidecar proxy mode.
// See include/network/envoy_xds.h for design documentation.

#ifdef THEMIS_ENABLE_SERVICE_MESH

#include "network/envoy_xds.h"
#include "utils/logger.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>

namespace themis::network {

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using     tcp   = net::ip::tcp;

// ─────────────────────────────────────────────────────────────────────────────
// Tiny JSON helpers (no external JSON library dependency)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Escape a string value for embedding in JSON.
static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

// Extract the value of a simple top-level JSON string field.
// Returns empty string if not found.
static std::string extractString(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    auto colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos) return {};
    auto q1 = json.find('"', colon + 1);
    if (q1 == std::string::npos) return {};
    std::string value;
    bool escape = false;
    for (std::size_t i = q1 + 1; i < json.size(); ++i) {
        if (escape) {
            switch (json[i]) {
                case '"': value += '"'; break;
                case '\\': value += '\\'; break;
                case 'n': value += '\n'; break;
                case 'r': value += '\r'; break;
                case 't': value += '\t'; break;
                default: value += json[i];
            }
            escape = false;
        } else if (json[i] == '\\') {
            escape = true;
        } else if (json[i] == '"') {
            break;
        } else {
            value += json[i];
        }
    }
    return value;
}

// Extract the raw JSON value (object, array, string, number) for a key.
// For arrays/objects this returns the complete bracketed content.
// Returns empty string if not found.
static std::string extractRawValue(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    auto colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos) return {};
    // Skip whitespace
    std::size_t start = colon + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t' ||
                                   json[start] == '\r' || json[start] == '\n')) {
        ++start;
    }
    if (start >= json.size()) return {};
    const char open = json[start];
    if (open != '[' && open != '{') {
        // Scalar – read until delimiter
        std::size_t end = start;
        while (end < json.size() && json[end] != ',' && json[end] != '}' &&
               json[end] != ']' && json[end] != '\n') {
            ++end;
        }
        return json.substr(start, end - start);
    }
    // Nested structure – balance brackets
    const char close = (open == '[') ? ']' : '}';
    int depth = 0;
    bool in_str = false;
    bool esc = false;
    std::size_t end = start;
    for (; end < json.size(); ++end) {
        const char c = json[end];
        if (esc) { esc = false; continue; }
        if (c == '\\' && in_str) { esc = true; continue; }
        if (c == '"') { in_str = !in_str; continue; }
        if (in_str) {
          continue;
        }
        if (c == open)  { ++depth; }
        else if (c == close) { if (--depth == 0) { ++end; break; } }
    }
    return json.substr(start, end - start);
}

// Split a JSON array body (the content inside '[' ... ']') into individual
// object tokens, handling nesting correctly.
static std::vector<std::string> splitJsonArray(const std::string& array_body) {
    std::vector<std::string> items;
    int depth = 0;
    bool in_str = false;
    bool esc = false;
    std::size_t item_start = std::string::npos;

    for (std::size_t i = 0; i < array_body.size(); ++i) {
        const char c = array_body[i];
        if (esc) { esc = false; continue; }
        if (c == '\\' && in_str) { esc = true; continue; }
        if (c == '"') {
            in_str = !in_str;
            if (!in_str && depth == 0) {
              continue;
            }
        }
        if (in_str) {
          continue;
        }

        if (c == '{' || c == '[') {
            if (depth == 0) {
              item_start = i;
            }
            ++depth;
        } else if (c == '}' || c == ']') {
            --depth;
            if (depth == 0 && item_start != std::string::npos) {
                items.push_back(array_body.substr(item_start, i - item_start + 1));
                item_start = std::string::npos;
            }
        }
    }
    return items;
}

// Parse a uint16 port from a string; returns 0 on failure.
static uint16_t parsePort(const std::string& s) {
    if (s.empty()) {
      return 0;
    }
    try {
        const long v = std::stol(s);
        if (v > 0 && v <= 65535) {
          return static_cast<uint16_t>(v);
        }
    } catch (...) {}
    return 0;
}

// Parse uint32 weight from a string; returns default_val on failure.
static uint32_t parseWeight(const std::string& s, uint32_t default_val = 100) {
    if (s.empty()) {
      return default_val;
    }
    try {
        const unsigned long v = std::stoul(s);
        if (v <= UINT32_MAX) {
          return static_cast<uint32_t>(v);
        }
    } catch (...) {}
    return default_val;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

EnvoyXdsClient::EnvoyXdsClient(const Config& config)
    : config_(config)
{}

EnvoyXdsClient::~EnvoyXdsClient() {
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

bool EnvoyXdsClient::start() {
    if (running_.load(std::memory_order_acquire)) {
        THEMIS_WARN("[xDS] start() called while already running");
        return false;
    }

    running_.store(true, std::memory_order_release);
    poll_thread_ = std::thread([this] { pollLoop(); });

    THEMIS_INFO("[xDS] client started; control plane: {}:{}",
                config_.control_plane_host,
                static_cast<int>(config_.control_plane_port));
    return true;
}

void EnvoyXdsClient::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    THEMIS_INFO("[xDS] client stopping");

    // Wake the polling thread so it exits its wait.
    {
        std::lock_guard<std::mutex> lk(wake_mutex_);
        wake_cv_.notify_all();
    }

    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }

    THEMIS_INFO("[xDS] client stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Callback registration
// ─────────────────────────────────────────────────────────────────────────────

void EnvoyXdsClient::setListenerCallback([[maybe_unused]] ListenerCallback cb) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    listener_cb_ = std::move([[maybe_unused]] cb);
}

void EnvoyXdsClient::setClusterCallback([[maybe_unused]] ClusterCallback cb) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    cluster_cb_ = std::move(cb);
}

void EnvoyXdsClient::setRouteCallback([[maybe_unused]] RouteCallback cb) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    route_cb_ = std::move(cb);
}

void EnvoyXdsClient::setEndpointCallback([[maybe_unused]] EndpointCallback cb) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
    endpoint_cb_ = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

EnvoyXdsClient::Stats EnvoyXdsClient::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

std::string EnvoyXdsClient::getListenerVersion() const {
    std::lock_guard<std::mutex> lk(versions_mutex_);
    return lds_version_;
}

std::string EnvoyXdsClient::getClusterVersion() const {
    std::lock_guard<std::mutex> lk(versions_mutex_);
    return cds_version_;
}

std::string EnvoyXdsClient::getRouteVersion() const {
    std::lock_guard<std::mutex> lk(versions_mutex_);
    return rds_version_;
}

std::string EnvoyXdsClient::getEndpointVersion() const {
    std::lock_guard<std::mutex> lk(versions_mutex_);
    return eds_version_;
}

// ─────────────────────────────────────────────────────────────────────────────
// DiscoveryRequest builder
// ─────────────────────────────────────────────────────────────────────────────

std::string EnvoyXdsClient::buildDiscoveryRequest(
    const std::string&              type_url,
    const std::string&              version,
    const std::string&              nonce,
    const std::vector<std::string>& names) const
{
    std::ostringstream ss;
    ss << "{"
       << "\"node\":{"
       <<   "\"id\":\"" << jsonEscape(config_.node_id) << "\","
       <<   "\"cluster\":\"" << jsonEscape(config_.node_cluster) << "\"";

    if (!config_.node_metadata.empty()) {
        ss << ",\"metadata\":{";
        bool first = true;
        for (const auto& [k, v] : config_.node_metadata) {
            if (!first) {
              ss << ',';
            }
            first = false;
            ss << "\"" << jsonEscape(k) << "\":\"" << jsonEscape(v) << "\"";
        }
        ss << "}";
    }

    ss << "},"
       << "\"type_url\":\"" << jsonEscape(type_url) << "\","
       << "\"version_info\":\"" << jsonEscape(version) << "\","
       << "\"response_nonce\":\"" << jsonEscape(nonce) << "\","
       << "\"resource_names\":[";

    for (std::size_t i = 0; i < names.size(); ++i) {
        if (i > 0) {
          ss << ',';
        }
        ss << "\"" << jsonEscape(names[i]) << "\"";
    }
    ss << "]}";

    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// DiscoveryResponse parser
// ─────────────────────────────────────────────────────────────────────────────

/* static */
bool EnvoyXdsClient::parseDiscoveryResponse(const std::string& json_body,
                                            std::string&       out_version,
                                            std::string&       out_nonce,
                                            std::string&       out_resources_json)
{
    if (json_body.empty()) {
      return false;
    }

    const std::string version   = extractString(json_body, "version_info");
    const std::string nonce     = extractString(json_body, "nonce");
    const std::string resources = extractRawValue(json_body, "resources");

    if (version.empty() || resources.empty()) {
      return false;
    }

    out_version        = version;
    out_nonce          = nonce;
    out_resources_json = resources;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// LDS resource parser
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::vector<EnvoyXdsClient::ListenerInfo>
EnvoyXdsClient::parseListeners(const std::string& resources_json)
{
    std::vector<ListenerInfo> result;
    if (resources_json.empty() || resources_json.front() != '[') {
      return result;
    }

    const std::string body = resources_json.substr(1, resources_json.size() - 2);
    for (const auto& item : splitJsonArray(body)) {
        ListenerInfo info;
        info.name     = extractString(item, "name");

        // Address is nested: address.socket_address.address / port_value
        const std::string addr_obj = extractRawValue(item, "address");
        if (!addr_obj.empty()) {
            const std::string sa = extractRawValue(addr_obj, "socket_address");
            if (!sa.empty()) {
                info.address  = extractString(sa, "address");
                info.port     = parsePort(extractRawValue(sa, "port_value"));
                info.protocol = extractString(sa, "protocol");
                if (info.protocol.empty()) {
                  info.protocol = "TCP";
                }
            }
        }

        if (!info.name.empty()) {
            result.push_back(std::move(info));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// CDS resource parser
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::vector<EnvoyXdsClient::ClusterInfo>
EnvoyXdsClient::parseClusters(const std::string& resources_json)
{
    std::vector<ClusterInfo> result;
    if (resources_json.empty() || resources_json.front() != '[') {
      return result;
    }

    const std::string body = resources_json.substr(1, resources_json.size() - 2);
    for (const auto& item : splitJsonArray(body)) {
        ClusterInfo info;
        info.name      = extractString(item, "name");
        info.type      = extractString(item, "type");
        info.lb_policy = extractString(item, "lb_policy");
        if (info.lb_policy.empty()) {
          info.lb_policy = "ROUND_ROBIN";
        }

        // Inline static endpoints (load_assignment.endpoints[].lb_endpoints[])
        const std::string la = extractRawValue(item, "load_assignment");
        if (!la.empty()) {
            const std::string eps_arr = extractRawValue(la, "endpoints");
            if (!eps_arr.empty() && eps_arr.front() == '[') {
                const std::string eps_body = eps_arr.substr(1, eps_arr.size() - 2);
                for (const auto& locality_ep : splitJsonArray(eps_body)) {
                    const std::string lb_eps = extractRawValue(locality_ep, "lb_endpoints");
                    if (lb_eps.empty() || lb_eps.front() != '[') {
                      continue;
                    }
                    const std::string lb_body = lb_eps.substr(1, lb_eps.size() - 2);
                    for (const auto& lbep : splitJsonArray(lb_body)) {
                        ClusterEndpoint ep;
                        const std::string ep_addr = extractRawValue(lbep, "endpoint");
                        if (ep_addr.empty()) {
                          continue;
                        }
                        const std::string addr_obj = extractRawValue(ep_addr, "address");
                        if (addr_obj.empty()) {
                          continue;
                        }
                        const std::string sa = extractRawValue(addr_obj, "socket_address");
                        if (sa.empty()) {
                          continue;
                        }
                        ep.address = extractString(sa, "address");
                        ep.port    = parsePort(extractRawValue(sa, "port_value"));
                        ep.weight  = parseWeight(extractRawValue(lbep, "load_balancing_weight"));
                        ep.health_status = extractString(lbep, "health_status");
                        if (ep.health_status.empty()) {
                          ep.health_status = "HEALTHY";
                        }
                        if (!ep.address.empty()) {
                            info.endpoints.push_back(std::move(ep));
                        }
                    }
                }
            }
        }

        if (!info.name.empty()) {
            result.push_back(std::move(info));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// EDS resource parser (ClusterLoadAssignment)
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::vector<EnvoyXdsClient::ClusterInfo>
EnvoyXdsClient::parseEndpoints(const std::string& resources_json)
{
    std::vector<ClusterInfo> result;
    if (resources_json.empty() || resources_json.front() != '[') {
      return result;
    }

    const std::string body = resources_json.substr(1, resources_json.size() - 2);
    for (const auto& item : splitJsonArray(body)) {
        ClusterInfo info;
        info.name = extractString(item, "cluster_name");

        const std::string eps_arr = extractRawValue(item, "endpoints");
        if (!eps_arr.empty() && eps_arr.front() == '[') {
            const std::string eps_body = eps_arr.substr(1, eps_arr.size() - 2);
            for (const auto& locality_ep : splitJsonArray(eps_body)) {
                const std::string lb_eps = extractRawValue(locality_ep, "lb_endpoints");
                if (lb_eps.empty() || lb_eps.front() != '[') {
                  continue;
                }
                const std::string lb_body = lb_eps.substr(1, lb_eps.size() - 2);
                for (const auto& lbep : splitJsonArray(lb_body)) {
                    ClusterEndpoint ep;
                    const std::string ep_addr = extractRawValue(lbep, "endpoint");
                    if (ep_addr.empty()) {
                      continue;
                    }
                    const std::string addr_obj = extractRawValue(ep_addr, "address");
                    if (addr_obj.empty()) {
                      continue;
                    }
                    const std::string sa = extractRawValue(addr_obj, "socket_address");
                    if (sa.empty()) {
                      continue;
                    }
                    ep.address = extractString(sa, "address");
                    ep.port    = parsePort(extractRawValue(sa, "port_value"));
                    ep.weight  = parseWeight(extractRawValue(lbep, "load_balancing_weight"));
                    ep.health_status = extractString(lbep, "health_status");
                    if (ep.health_status.empty()) {
                      ep.health_status = "HEALTHY";
                    }
                    if (!ep.address.empty()) {
                        info.endpoints.push_back(std::move(ep));
                    }
                }
            }
        }

        if (!info.name.empty()) {
            result.push_back(std::move(info));
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// RDS resource parser (RouteConfiguration)
// ─────────────────────────────────────────────────────────────────────────────

/* static */
std::vector<EnvoyXdsClient::VirtualHostInfo>
EnvoyXdsClient::parseRoutes(const std::string& resources_json)
{
    std::vector<VirtualHostInfo> result;
    if (resources_json.empty() || resources_json.front() != '[') {
      return result;
    }

    const std::string body = resources_json.substr(1, resources_json.size() - 2);
    for (const auto& rc : splitJsonArray(body)) {
        // Each resource is a RouteConfiguration with a virtual_hosts array.
        const std::string vhosts_raw = extractRawValue(r[[maybe_unused]] c, "virtual_host[[maybe_unused]] s");
        if (vhosts_raw.empty() || vhosts_raw.front() != '[') {
          continue;
        }

        const std::string vhosts_body = vhosts_raw.substr(1, vhosts_raw.size() - 2);
        for (const auto& vh : splitJsonArray(vhosts_body)) {
            VirtualHostInfo info;
            info.name = extractString(vh, "name");

            // Parse domains array
            const std::string domains_raw = extractRawValue(vh, "domains");
            if (!domains_raw.empty() && domains_raw.front() == '[') {
                const std::string dom_body = domains_raw.substr(1, domains_raw.size() - 2);
                // Simple string-array split: find quoted strings
                std::size_t pos = 0;
                while (pos < dom_body.size()) {
                    const auto q1 = dom_body.find('"', pos);
                    if (q1 == std::string::npos) {
                      break;
                    }
                    std::string domain;
                    bool esc = false;
                    std::size_t i = q1 + 1;
                    for (; i < dom_body.size(); ++i) {
                        if (esc) { domain += dom_body[i]; esc = false; continue; }
                        if (dom_body[i] == '\\') { esc = true; continue; }
                        if (dom_body[i] == '"') { ++i; break; }
                        domain += dom_body[i];
                    }
                    if (!domain.empty()) {
                      info.domains.push_back(domain);
                    }
                    pos = i;
                }
            }

            // Parse routes array
            const std::string routes_raw = extractRawValue(vh, "routes");
            if (!routes_raw.empty() && routes_raw.front() == '[') {
                const std::string routes_body = routes_raw.substr(1, routes_raw.size() - 2);
                for (const auto& rt : splitJsonArray(routes_body)) {
                    RouteInfo route;

                    // match.prefix or match.path
                    const std::string match_obj = extractRawValue(rt, "match");
                    if (!match_obj.empty()) {
                        route.prefix = extractString(match_obj, "prefix");
                        if (route.prefix.empty()) {
                            route.prefix = extractString(match_obj, "path");
                        }
                        route.method = extractString(match_obj, "method");
                    }

                    // route.cluster (action)
                    const std::string action = extractRawValue(rt, "route");
                    if (!action.empty()) {
                        route.cluster_name = extractString(action, "cluster");
                        const std::string to_raw = extractRawValue(action, "timeout");
                        if (!to_raw.empty()) {
                            // Timeout is a protobuf Duration string: "30s" or "30.500s".
                            // Strip the trailing 's' unit suffix before parsing.
                            try {
                                std::string numeric = to_raw;
                                if (!numeric.empty() && numeric.back() == 's') {
                                    numeric.pop_back();
                                }
                                route.timeout_ms = static_cast<uint32_t>(
                                    std::stof(numeric) * 1000.0f);
                            } catch (...) {}
                        }
                    }

                    if (!route.cluster_name.empty()) {
                        info.routes.push_back(std::move(route));
                    }
                }
            }

            if (!info.name.empty()) {
                result.push_back(std::move(info));
            }
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// HTTP/1.1 POST to control plane
// ─────────────────────────────────────────────────────────────────────────────

std::string EnvoyXdsClient::httpPost(const std::string& path,
                                     const std::string& body,
                                     int*               out_status_code) const
{
    try {
        net::io_context ioc;
        tcp::resolver   resolver(ioc);

        const auto results = resolver.resolve(
            config_.control_plane_host,
            std::to_string(config_.control_plane_port));

        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::milliseconds(config_.request_timeout_ms));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, path, 11};
        req.set(http::field::host, config_.control_plane_host);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::user_agent, "ThemisDB-xDS/1.0");
        req.body() = body;
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer             buf;
        http::response<http::string_body> res;
        stream.expires_after(std::chrono::milliseconds(config_.request_timeout_ms));
        http::read(stream, buf, res);

        if (out_status_code) {
            *out_status_code = static_cast<int>(res.result_int());
        }

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (res.result() == http::status::ok) {
            return res.body();
        }
        THEMIS_DEBUG("[xDS] control plane returned HTTP {} for {}",
                     static_cast<int>(res.result_int()), path);
        return {};

    } catch (const std::exception& ex) {
        THEMIS_WARN("[xDS] HTTP POST to {}:{}{} failed: {}",
                    config_.control_plane_host,
                    static_cast<int>(config_.control_plane_port),
                    path, ex.what());
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.connect_errors;
        return {};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Generic discovery-service poll helper
// ─────────────────────────────────────────────────────────────────────────────

bool EnvoyXdsClient::pollDiscoveryService(const std::string& type_url,
                                          const std::string& rest_path,
                                          std::string&       inout_version,
                                          std::string&       inout_nonce,
                                          std::string&       out_resources_json,
                                          bool&              out_error)
{
    out_error = false;

    const std::string req_body =
        buildDiscoveryRequest(type_url, inout_version, inout_nonce, {});

    const std::string resp_body = httpPost(rest_path, req_body);
    if (resp_body.empty()) {
        out_error = true;
        return false;
    }

    std::string new_version, new_nonce, resources_json;
    if (!parseDiscoveryResponse(resp_body, new_version, new_nonce, resources_json)) {
        THEMIS_WARN("[xDS] failed to parse DiscoveryResponse for {}", type_url);
        out_error = true;
        return false;
    }

    // Unchanged version – ACK silently, no callback.
    if (new_version == inout_version && !inout_version.empty()) {
        inout_nonce = new_nonce;
        return false;
    }

    inout_version      = new_version;
    inout_nonce        = new_nonce;
    out_resources_json = std::move(resources_json);

    THEMIS_INFO("[xDS] received {} update, version={}", type_url, new_version);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Main polling loop
// ─────────────────────────────────────────────────────────────────────────────

void EnvoyXdsClient::pollLoop() {
    // Per-resource version/nonce state – local to the polling thread.
    std::string lds_ver, lds_nc, cds_ver, cds_nc, eds_ver, eds_nc, rds_ver, rds_nc;

    while (running_.load(std::memory_order_acquire)) {
        bool had_any_update = false;

        // ── LDS ────────────────────────────────────────────────────────────
        if ([[maybe_unused]] config_.subscribe_listeners) {
            std::string res_json;
            bool err = false;
            const bool updated = pollDiscoveryService(
                kXdsTypeUrlListener, "/v3/discovery:listeners",
                lds_ver, lds_nc, res_json, err);

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                if (updated) {
                  ++stats_.lds_updates;
                }
                if (err) {
                  ++stats_.lds_errors;
                }
            }
            if (updated) {
                had_any_update = true;
                {
                    const auto listeners = parseListeners([[maybe_unused]] res_json);
                    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
                    if ([[maybe_unused]] listener_cb_) {
                      listener_cb_(listeners);
                    }
                }
                std::lock_guard<std::mutex> lk(versions_mutex_);
                lds_version_ = lds_ver;
                lds_nonce_   = lds_nc;
            }
        }

        // ── CDS ────────────────────────────────────────────────────────────
        if (config_.subscribe_clusters) {
            std::string res_json;
            bool err = false;
            const bool updated = pollDiscoveryService(
                kXdsTypeUrlCluster, "/v3/discovery:clusters",
                cds_ver, cds_nc, res_json, err);

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                if (updated) {
                  ++stats_.cds_updates;
                }
                if (err) {
                  ++stats_.cds_errors;
                }
            }
            if (updated) {
                had_any_update = true;
                {
                    const auto clusters = parseClusters(res_json);
                    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
                    if (cluster_cb_) {
                      cluster_cb_(clusters);
                    }
                }
                std::lock_guard<std::mutex> lk(versions_mutex_);
                cds_version_ = cds_ver;
                cds_nonce_   = cds_nc;
            }
        }

        // ── EDS ────────────────────────────────────────────────────────────
        if (config_.subscribe_endpoints) {
            std::string res_json;
            bool err = false;
            const bool updated = pollDiscoveryService(
                kXdsTypeUrlEndpoint, "/v3/discovery:endpoints",
                eds_ver, eds_nc, res_json, err);

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                if (updated) {
                  ++stats_.eds_updates;
                }
                if (err) {
                  ++stats_.eds_errors;
                }
            }
            if (updated) {
                had_any_update = true;
                {
                    const auto endpoints = parseEndpoints(res_json);
                    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
                    if (endpoint_cb_) {
                      endpoint_cb_(endpoints);
                    }
                }
                std::lock_guard<std::mutex> lk(versions_mutex_);
                eds_version_ = eds_ver;
                eds_nonce_   = eds_nc;
            }
        }

        // ── RDS ────────────────────────────────────────────────────────────
        if (config_.subscribe_routes) {
            std::string res_json;
            bool err = false;
            const bool updated = pollDiscoveryService(
                kXdsTypeUrlRoute, "/v3/discovery:routes",
                rds_ver, rds_nc, res_json, err);

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                if (updated) {
                  ++stats_.rds_updates;
                }
                if (err) {
                  ++stats_.rds_errors;
                }
            }
            if (updated) {
                had_any_update = true;
                {
                    const auto routes = parseRoutes(res_json);
                    std::lock_guard<std::mutex> lk([[maybe_unused]] callbacks_mutex_);
                    if (route_cb_) {
                      route_cb_(routes);
                    }
                }
                std::lock_guard<std::mutex> lk(versions_mutex_);
                rds_version_ = rds_ver;
                rds_nonce_   = rds_nc;
            }
        }

        // Wait for the next poll interval (or until stop() wakes us).
        const uint32_t wait_ms = had_any_update
                                     ? config_.poll_interval_ms
                                     : config_.reconnect_interval_ms;

        std::unique_lock<std::mutex> lk(wake_mutex_);
        wake_cv_.wait_for(lk, std::chrono::milliseconds(wait_ms),
                          [this] { return !running_.load(std::memory_order_acquire); });
    }
}

}  // namespace themis::network

#endif  // THEMIS_ENABLE_SERVICE_MESH


