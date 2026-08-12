/**
 * @file distributed_gateway.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=2, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/distributed_gateway.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace themis::server {

// ===========================================================================
// GatewayRouteConfig – JSON serialisation
// ===========================================================================

nlohmann::json GatewayRouteConfig::toJson() const {
    return {
        {"path_prefix",                        path_prefix},
        {"upstream_url",                       upstream_url},
        {"timeout_ms",                         timeout_ms},
        {"retry_count",                        retry_count},
        {"circuit_breaker_enabled",            circuit_breaker_enabled},
        {"circuit_breaker_failure_threshold",  circuit_breaker_failure_threshold},
    };
}

GatewayRouteConfig GatewayRouteConfig::fromJson(const nlohmann::json& j) {
    GatewayRouteConfig cfg;
    cfg.path_prefix   = j.value("path_prefix",   "");
    cfg.upstream_url  = j.value("upstream_url",  "");
    cfg.timeout_ms    = j.value("timeout_ms",    static_cast<uint32_t>(30000));
    cfg.retry_count   = j.value("retry_count",   static_cast<uint32_t>(2));
    cfg.circuit_breaker_enabled
        = j.value("circuit_breaker_enabled", true);
    cfg.circuit_breaker_failure_threshold
        = j.value("circuit_breaker_failure_threshold", static_cast<uint32_t>(5));
    return cfg;
}

// ===========================================================================
// ClusterGatewayConfig – JSON serialisation
// ===========================================================================

nlohmann::json ClusterGatewayConfig::toJson() const {
    nlohmann::json routes_json = nlohmann::json::array();
    for (const auto& r : routes) {
        routes_json.push_back(r.toJson());
    }

    nlohmann::json rate_limits_json = nlohmann::json::object();
    for (const auto& [k, v] : rate_limits) {
        rate_limits_json[k] = v;
    }

    auto tp = std::chrono::system_clock::to_time_t(updated_at);
    return {
        {"version",              version},
        {"routes",               routes_json},
        {"rate_limits",          rate_limits_json},
        {"global_rate_limit_rps", global_rate_limit_rps},
        {"updated_by",           updated_by},
        {"updated_at",           tp},
    };
}

ClusterGatewayConfig ClusterGatewayConfig::fromJson(const nlohmann::json& j) {
    ClusterGatewayConfig cfg;
    cfg.version              = j.value("version", static_cast<uint64_t>(0));
    cfg.global_rate_limit_rps
        = j.value("global_rate_limit_rps", static_cast<uint32_t>(100000));
    cfg.updated_by           = j.value("updated_by", "");

    if (j.contains("updated_at")) {
        time_t t = j["updated_at"].get<time_t>();
        cfg.updated_at = std::chrono::system_clock::from_time_t(t);
    }

    if (j.contains("routes") && j["routes"].is_array()) {
        for (const auto& r : j["routes"]) {
            cfg.routes.push_back(GatewayRouteConfig::fromJson(r));
        }
    }

    if (j.contains("rate_limits") && j["rate_limits"].is_object()) {
        for (auto it = j["rate_limits"].begin(); it != j["rate_limits"].end(); ++it) {
            cfg.rate_limits[it.key()] = it.value().get<uint32_t>();
        }
    }

    return cfg;
}

// ===========================================================================
// ConsistentHashRing
// ===========================================================================

ConsistentHashRing::ConsistentHashRing(uint32_t virtual_nodes)
    : virtual_nodes_(virtual_nodes) {}

// FNV-1a 64-bit hash with a replica seed suffix for virtual nodes.
uint64_t ConsistentHashRing::hash(const std::string& key, uint32_t replica) {
    static constexpr uint64_t kFNVOffset = 14695981039346656037ULL;
    static constexpr uint64_t kFNVPrime  = 1099511628211ULL;

    std::string salted = key + "#" + std::to_string(replica);
    uint64_t h = kFNVOffset;
    for (unsigned char c : salted) {
        h ^= c;
        h *= kFNVPrime;
    }
    return h;
}

void ConsistentHashRing::addNode(const GatewayNode& node) {
    std::unique_lock lock(mutex_);
    for (uint32_t i = 0; i < virtual_nodes_; ++i) {
        uint64_t h = hash(node.node_id, i);
        ring_[h] = node;
    }
    spdlog::debug("ConsistentHashRing: added node '{}' ({} virtual nodes)",
                  node.node_id, virtual_nodes_);
}

void ConsistentHashRing::removeNode(const std::string& node_id) {
    std::unique_lock lock(mutex_);
    for (uint32_t i = 0; i < virtual_nodes_; ++i) {
        uint64_t h = hash(node_id, i);
        ring_.erase(h);
    }
    spdlog::debug("ConsistentHashRing: removed node '{}'", node_id);
}

std::optional<GatewayNode> ConsistentHashRing::getNode(
    const std::string& session_key) const
{
    std::shared_lock lock(mutex_);
    if (ring_.empty()) {
        return std::nullopt;
    }
    uint64_t h = hash(session_key, 0);
    auto it = ring_.lower_bound(h);
    if (it == ring_.end()) {
        it = ring_.begin();
    }
    return it->second;
}

std::size_t ConsistentHashRing::nodeCount() const {
    std::shared_lock lock(mutex_);
    std::unordered_set<std::string> seen;
    for (const auto& [h, node] : ring_) {
        seen.insert(node.node_id);
    }
    return seen.size();
}

// ===========================================================================
// DistributedGateway
// ===========================================================================

DistributedGateway::DistributedGateway(
    const Config& config,
    std::shared_ptr<APIGateway> gateway
)
    : config_(config)
    , gateway_(std::move(gateway))
    , hash_ring_(config.virtual_nodes_per_peer)
{
    if (!gateway_) {
        throw std::invalid_argument("DistributedGateway: gateway must be non-null");
    }

    // Initialise hash ring
    rebuildHashRing();

    // Build and configure Raft
    raft_ = std::make_unique<themisdb::sharding::RaftConsensus>(buildRaftConfig());

    // Replication callback – applied every time the Raft leader replicates an
    // entry to a follower (simulated in-process for now; real deployments use
    // an RPC transport).
    raft_->setReplicationCallback(
        [this](const std::string& /*node_id*/,
               const themisdb::sharding::LogEntry& entry) -> bool {
            return applyConfigEntry(entry.command);
        });

    spdlog::info("DistributedGateway: node='{}' cluster_size={} ring_nodes={}",
                 config_.node_id,
                 config_.cluster_nodes.size(),
                 hash_ring_.nodeCount());
}

DistributedGateway::~DistributedGateway() {
    stop();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DistributedGateway::start() {
    if (running_.exchange(true)) {
        return; // already running
    }
    raft_->start();
    spdlog::info("DistributedGateway: started (node='{}')", config_.node_id);
}

void DistributedGateway::stop() {
    if (!running_.exchange(false)) {
        return; // already stopped
    }
    raft_->stop();
    spdlog::info("DistributedGateway: stopped (node='{}')", config_.node_id);
}

// ---------------------------------------------------------------------------
// Request routing
// ---------------------------------------------------------------------------

http::response<http::string_body> DistributedGateway::handleRequest(
    const http::request<http::string_body>& req,
    std::function<http::response<http::string_body>(
        const http::request<http::string_body>&)> local_handler
)
{
    // Quorum-loss detection: check on every request so operators get a CRITICAL
    // alert as soon as the cluster loses quorum, not just at config-change time.
    // All state changes are made inside the lock; logging happens after release.
    if (running_.load()) {
        const bool has_q = raft_->hasQuorum();
        bool emit_critical = false;
        bool emit_restored = false;
        uint64_t cfg_ver   = 0;
        {
            std::unique_lock lock(config_mutex_);
            if (!has_q && !quorum_lost_) {
                quorum_lost_ = true;
                cfg_ver      = current_config_.version;
                emit_critical = true;
            } else if (has_q && quorum_lost_) {
                quorum_lost_ = false;
                emit_restored = true;
            }
        }
        if (emit_critical) {
            spdlog::critical(
                "DistributedGateway: quorum lost on node '{}' – "
                "serving requests with last-known config (version {})",
                config_.node_id, cfg_ver);
        } else if (emit_restored) {
            spdlog::info("DistributedGateway: quorum restored on node '{}'",
                         config_.node_id);
        }
    }

    // Session-affinity check for WebSocket / SSE
    if (needsSessionAffinity(req)) {
        const std::string key = sessionKey(req);
        auto node = resolveAffinityNode(key);
        if (node.has_value() && node->node_id != config_.node_id) {
            // The responsible node for this session is a peer – in a real
            // deployment we would proxy the request there.  Here we log it and
            // fall through to the local handler so that single-process tests
            // continue to work without a real network.
            spdlog::debug("DistributedGateway: affinity key='{}' routed to peer '{}'",
                          key, node->node_id);
        }
    }

    // Delegate to the underlying APIGateway for actual request processing,
    // retrying on transient HTTP 5xx / 429 errors with exponential backoff.
    // Retries are restricted to idempotent, read-only methods (GET, HEAD,
    // OPTIONS) because non-idempotent mutations (POST, PUT, PATCH, DELETE)
    // may have been processed by the server before a transient error was
    // returned, making a retry a double-apply.
    const auto method = req.method();
    const bool is_retry_safe = (method == http::verb::get  ||
                                 method == http::verb::head ||
                                 method == http::verb::options);
    if (!is_retry_safe || config_.max_retries == 0) {
        return gateway_->handleRequest(req, std::move(local_handler));
    }

    // The local_handler is copied (not moved) so it remains callable for
    // subsequent retry attempts.  Only the final attempt passes ownership.
    const uint32_t effective_max_retries = config_.max_retries;
    http::response<http::string_body> resp;
    for (uint32_t attempt = 0; attempt <= effective_max_retries; ++attempt) {
        if (attempt == effective_max_retries) {
            // Last attempt — pass ownership of local_handler (no further retries).
            resp = gateway_->handleRequest(req, std::move(local_handler));
        } else {
            // Intermediate attempt — keep local_handler alive for possible retry.
            auto handler_copy = local_handler;
            resp = gateway_->handleRequest(req, std::move(handler_copy));
        }

        const auto status_code =
            static_cast<unsigned>(resp.result_int());
        if (!isTransientError(status_code)) {
            // Fast path: successful or non-retryable response.
            return resp;
        }

        // Transient error on a non-final attempt: log, back off, retry.
        const auto delay = retryDelay(attempt,
                                      config_.retry_base_delay_ms,
                                      config_.retry_max_delay_ms);
        spdlog::warn(
            "DistributedGateway: transient error {} on attempt {}/{} — "
            "retrying in {}ms (node='{}')",
            status_code, attempt + 1, effective_max_retries + 1,
            delay.count(), config_.node_id);
        std::this_thread::sleep_for(delay);
    }
    return resp;
}

std::optional<GatewayNode> DistributedGateway::resolveAffinityNode(
    const std::string& session_key) const
{
    return hash_ring_.getNode(session_key);
}

// ---------------------------------------------------------------------------
// Config management
// ---------------------------------------------------------------------------

bool DistributedGateway::proposeConfig(const ClusterGatewayConfig& new_config) {
    if (!raft_->isLeader()) {
        spdlog::warn("DistributedGateway::proposeConfig: not the leader – refusing write");
        return false;
    }
    if (!raft_->hasQuorum()) {
        spdlog::error("DistributedGateway::proposeConfig: quorum unavailable – refusing write");
        return false;
    }

    try {
        std::string entry_json = new_config.toJson().dump();
        auto future = raft_->propose(entry_json);
        // Wait up to the configured failover timeout for the entry to commit.
        auto status = future.wait_for(config_.leader_failover_timeout);
        if (status == std::future_status::ready) {
            return future.get();
        }
        spdlog::warn("DistributedGateway::proposeConfig: commit timed out after {}ms",
                     config_.leader_failover_timeout.count());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("DistributedGateway::proposeConfig: exception: {}", e.what());
        return false;
    }
}

ClusterGatewayConfig DistributedGateway::getCurrentConfig() const {
    std::shared_lock lock(config_mutex_);
    return current_config_;
}

// ---------------------------------------------------------------------------
// Extensibility
// ---------------------------------------------------------------------------

void DistributedGateway::registerHandler(
    const std::string& pattern,
    std::function<http::response<http::string_body>(
        const http::request<http::string_body>&)> handler
)
{
    gateway_->registerHandler(pattern, std::move(handler));
}

void DistributedGateway::registerDeprecation(
    const std::string& endpoint,
    const APIDeprecationInfo& info
)
{
    gateway_->registerDeprecation(endpoint, info);
}

bool DistributedGateway::applyConfigEntry(const std::string& entry_json) {
    if (entry_json.empty()) {
        return true; // heartbeat / no-op entries
    }

    try {
        auto j = nlohmann::json::parse(entry_json);
        ClusterGatewayConfig new_cfg = ClusterGatewayConfig::fromJson(j);

        uint64_t applied_version = 0;
        std::size_t applied_routes = 0;
        {
            std::unique_lock lock(config_mutex_);
            if (new_cfg.version <= current_config_.version) {
                // Stale entry – ignore (idempotent apply)
                return true;
            }
            current_config_ = std::move(new_cfg);
            quorum_lost_ = false;
            // Capture for logging while still holding the lock to avoid a data race.
            applied_version = current_config_.version;
            applied_routes  = current_config_.routes.size();
        }

        spdlog::info("DistributedGateway: applied config v{} ({} routes)",
                     applied_version, applied_routes);
        return true;

    } catch (const nlohmann::json::exception& e) {
        spdlog::error("DistributedGateway::applyConfigEntry: JSON parse error: {}",
                      e.what());
        return false;
    } catch (const std::exception& e) {
        spdlog::error("DistributedGateway::applyConfigEntry: error: {}", e.what());
        return false;
    }
}

// ---------------------------------------------------------------------------
// Cluster status
// ---------------------------------------------------------------------------

bool DistributedGateway::isLeader() const {
    return raft_->isLeader();
}

bool DistributedGateway::hasQuorum() const {
    return raft_->hasQuorum();
}

std::string DistributedGateway::getLeaderId() const {
    return raft_->getLeaderId();
}

nlohmann::json DistributedGateway::getClusterStatus() const {
    nlohmann::json nodes = nlohmann::json::array();
    for (const auto& n : config_.cluster_nodes) {
        nodes.push_back({
            {"node_id", n.node_id},
            {"address", n.address},
            {"port",    n.port},
        });
    }

    ClusterGatewayConfig cfg;
    {
        std::shared_lock lock(config_mutex_);
        cfg = current_config_;
    }

    return {
        {"node_id",        config_.node_id},
        {"is_leader",      isLeader()},
        {"leader_id",      getLeaderId()},
        {"has_quorum",     hasQuorum()},
        {"quorum_lost",    quorum_lost_},
        {"config_version", cfg.version},
        {"route_count",    cfg.routes.size()},
        {"ring_nodes",     hash_ring_.nodeCount()},
        {"cluster_nodes",  nodes},
    };
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

themisdb::sharding::RaftConsensus::Config
DistributedGateway::buildRaftConfig() const
{
    themisdb::sharding::RaftConfig raft_cfg;
    raft_cfg.node_id = config_.node_id;
    raft_cfg.election_timeout_min_ms = config_.election_timeout_min_ms;
    raft_cfg.election_timeout_max_ms = config_.election_timeout_max_ms;
    raft_cfg.heartbeat_interval_ms   = config_.heartbeat_interval_ms;

    for (const auto& n : config_.cluster_nodes) {
        raft_cfg.cluster_members.push_back(n.node_id);
    }

    themisdb::sharding::RaftConsensus::Config consensus_cfg;
    consensus_cfg.raft_config                  = raft_cfg;
    consensus_cfg.heartbeat_timeout            = std::chrono::milliseconds{config_.heartbeat_interval_ms};
    consensus_cfg.enable_partition_detection   = true;
    consensus_cfg.enable_split_brain_prevention = true;
    consensus_cfg.read_only_on_partition       = true;

    return consensus_cfg;
}

void DistributedGateway::rebuildHashRing() {
    for (const auto& n : config_.cluster_nodes) {
        hash_ring_.addNode(n);
    }
}

std::string DistributedGateway::sessionKey(
    const http::request<http::string_body>& req) const
{
    // Use the target path as the primary key; in production the client IP
    // would also be included for better distribution.
    std::string key = std::string(req.target());
    // Strip query string
    auto q = key.find('?');
    if (q != std::string::npos) {
        key.erase(q);
    }
    return key;
}

bool DistributedGateway::needsSessionAffinity(
    const http::request<http::string_body>& req) const
{
    // WebSocket upgrade
    if (req.count(http::field::upgrade) > 0) {
        auto upgrade = req[http::field::upgrade];
        std::string upg_lower(upgrade.begin(), upgrade.end());
        std::transform(upg_lower.begin(), upg_lower.end(),
                       upg_lower.begin(), ::tolower);
        if (upg_lower.find("websocket") != std::string::npos) {
            return true;
        }
    }

    // Server-Sent Events (text/event-stream)
    if (req.count(http::field::accept) > 0) {
        auto accept = req[http::field::accept];
        std::string accept_lower(accept.begin(), accept.end());
        std::transform(accept_lower.begin(), accept_lower.end(),
                       accept_lower.begin(), ::tolower);
        if (accept_lower.find("text/event-stream") != std::string::npos) {
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Wire-protocol retry helpers (P5-S01)
// ---------------------------------------------------------------------------

/**
 * @brief Classify an HTTP status code as a transient error.
 *
 * Retryable status codes:
 *   - 429 Too Many Requests (downstream throttle, momentarily retryable)
 *   - 500 Internal Server Error (transient backend fault)
 *   - 502 Bad Gateway          (upstream unreachable / crashed)
 *   - 503 Service Unavailable  (overloaded / maintenance)
 *   - 504 Gateway Timeout      (upstream timed out)
 *
 * All other codes (1xx, 2xx, 3xx, 4xx except 429) are considered final and
 * must **not** be retried to avoid double-posting mutations.
 */
bool DistributedGateway::isTransientError(unsigned status) noexcept {
    return status == 429
        || status == 500
        || status == 502
        || status == 503
        || status == 504;
}

/**
 * @brief Exponential-backoff delay: base × 2^attempt, clamped to max.
 *
 * Example with base=50ms, max=2000ms:
 *   attempt 0 → 50ms, attempt 1 → 100ms, attempt 2 → 200ms, … cap at 2000ms.
 *
 * @param attempt   0-based retry index.
 * @param base_ms   Initial delay in milliseconds.
 * @param max_ms    Maximum delay in milliseconds.
 * @return          Delay for this attempt.
 */
std::chrono::milliseconds DistributedGateway::retryDelay(
    uint32_t attempt,
    uint32_t base_ms,
    uint32_t max_ms) noexcept
{
    // Clamp the shift to avoid undefined behaviour on 32-bit left-shift overflow.
    constexpr uint32_t kMaxShift = 31u;
    const uint32_t shift = (attempt < kMaxShift) ? attempt : kMaxShift;
    // base_ms × 2^shift, saturating at UINT32_MAX before the max clamp.
    const uint64_t raw = static_cast<uint64_t>(base_ms) << shift;
    const uint32_t clamped = (raw > max_ms) ? max_ms : static_cast<uint32_t>(raw);
    return std::chrono::milliseconds(clamped);
}

} // namespace themis::server
