/**
 * @file gossip_protocol.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 80/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=2, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/gossip_protocol.h"
#include <stdexcept>
#include "sharding/shard_topology.h"
#include "sharding/mtls_client.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// RAII deleters for OpenSSL resources
namespace {
struct EVP_MD_CTX_Deleter {
    void operator()(EVP_MD_CTX* ptr) const {
        if (ptr) {
          EVP_MD_CTX_free(ptr);
        }
    }
};

struct EVP_PKEY_Deleter {
    void operator()(EVP_PKEY* ptr) const {
        if (ptr) {
          EVP_PKEY_free(ptr);
        }
    }
};

struct FILE_Deleter {
    void operator()(FILE* ptr) const {
        if (ptr) {
          fclose(ptr);
        }
    }
};

using EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, EVP_MD_CTX_Deleter>;
using EVP_PKEY_ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_Deleter>;
using FILE_ptr = std::unique_ptr<FILE, FILE_Deleter>;
}

namespace themis {
namespace sharding {

// ============================================================================
// Retry Helper with Exponential Backoff
// ============================================================================

namespace {

/**
 * @brief Helper function to execute an operation with exponential backoff retry.
 * 
 * @tparam Func Callable that returns bool (true = success, false = transient failure)
 * @param func Operation to retry
 * @param max_retries Maximum number of retry attempts (default: 3)
 * @param initial_delay_ms Initial backoff delay in milliseconds (default: 100)
 * @param max_delay_ms Maximum backoff delay cap (default: 5000)
 * @return true if operation succeeded, false if all retries exhausted
 */
template <typename Func>
inline bool retryWithBackoff(
    Func&& func,
    int max_retries = 3,
    uint64_t initial_delay_ms = 100,
    uint64_t max_delay_ms = 5000
) {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            if (func()) {
                return true;  // Success
            }
            // Transient failure: prepare to retry
        } catch (const std::exception&) {
            // Exception indicates transient failure; retry
        }
        
        if (attempt < max_retries - 1) {
            // Exponential backoff: 100ms, 200ms, 400ms, ...
            uint64_t delay_ms = initial_delay_ms * (1 << attempt);
            delay_ms = std::min(delay_ms, max_delay_ms);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }
    
    return false;  // All retries exhausted
}

}  // anonymous namespace

GossipProtocol::GossipProtocol(
    const GossipConfig& config,
    std::shared_ptr<ShardTopology> topology
) : config_(config),
    topology_(std::move(topology)) {
    
    // Initialize mTLS client if required
    if (config_.require_mtls) {
        MTLSClient::Config client_config;
        client_config.verify_peer = config_.validate_certificates;
        client_config.connect_timeout_ms = 5000;
        client_config.request_timeout_ms = 10000;
        client_config.max_retries = 2;
        client_ = std::make_unique<MTLSClient>(client_config);
    }
}

GossipProtocol::~GossipProtocol() {
    stop();
}

void GossipProtocol::start() {
    if (!config_.enabled) {
        return;  // Gossip disabled
    }
    
    if (running_.load()) {
        return;  // Already running
    }
    
    running_.store(true);
    
    // Bootstrap from seed nodes
    bootstrapFromSeedNodes();
    
    // Start gossip thread
    gossip_thread_ = std::thread(&GossipProtocol::gossipLoop, this);
    
    // Start cleanup thread
    cleanup_thread_ = std::thread(&GossipProtocol::cleanupLoop, this);
}

void GossipProtocol::stop() {
    if (!running_.load()) {
        return;  // Already stopped
    }
    
    // Send leave message to all peers
    sendLeaveMessage();
    
    running_.store(false);
    
    // thread_join_no_timeout (W4): bounded join via joinThreadWithin
    if (!themis::utils::joinThreadWithin(gossip_thread_)) {
        THEMIS_WARN("[GossipProtocol] gossip thread did not finish within shutdown deadline; detaching.");
    }
    
    if (!themis::utils::joinThreadWithin(cleanup_thread_)) {
        THEMIS_WARN("[GossipProtocol] cleanup thread did not finish within shutdown deadline; detaching.");
    }
}

std::map<std::string, PeerInfo> GossipProtocol::getPeers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return peers_;
}

std::vector<PeerInfo> GossipProtocol::getHealthyPeers() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    std::vector<PeerInfo> healthy;
    
    for (const auto& [id, peer] : peers_) {
        if (peer.is_healthy) {
            healthy.push_back(peer);
        }
    }
    
    return healthy;
}

size_t GossipProtocol::getPeerCount() const {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    return static_cast<int>(peers_.size());
}

void GossipProtocol::addPeer(const PeerInfo& peer) {
    PeerDiscoveryCallback peer_discovered_callback;
    PeerInfo discovered_peer;

    {
        std::lock_guard<std::mutex> lock(peers_mutex_);

        // Check max peers limit
        if (static_cast<int>(peers_.size()) >= config_.max_peers &&
            peers_.find(peer.peer_id) == peers_.end()) {
            return;  // At capacity, don't add new peers
        }

        auto it = peers_.find(peer.peer_id);
        if (it == peers_.end()) {
            // New peer
            PeerInfo new_peer = peer;
            new_peer.first_seen = std::chrono::system_clock::now();
            new_peer.last_seen = std::chrono::system_clock::now();
            peers_[peer.peer_id] = new_peer;
            peers_discovered_++;

            // Sync with topology without re-acquiring peers_mutex_
            syncWithTopologyLocked();

            peer_discovered_callback = on_peer_discovered_;
            discovered_peer = std::move(new_peer);
        } else {
            // Update existing peer
            if (peer.version > it->second.version) {
                it->second.endpoint = peer.endpoint;
                it->second.datacenter = peer.datacenter;
                it->second.region = peer.region;
                it->second.version = peer.version;
                it->second.last_seen = std::chrono::system_clock::now();
                it->second.is_healthy = true;
            }
        }
    }

    if (peer_discovered_callback) {
        peer_discovered_callback(discovered_peer);
    }
}

void GossipProtocol::removePeer(const std::string& peer_id) {
    PeerLostCallback peer_lost_callback;

    {
        std::lock_guard<std::mutex> lock(peers_mutex_);

        auto it = peers_.find(peer_id);
        if (it == peers_.end()) {
            return;
        }

        peers_.erase(it);
        peers_lost_++;

        // Sync with topology without re-acquiring peers_mutex_
        syncWithTopologyLocked();

        peer_lost_callback = on_peer_lost_;
    }

    if (peer_lost_callback) {
        peer_lost_callback(peer_id);
    }
}

GossipMessage GossipProtocol::handleMessage(const GossipMessage& message) {
    messages_received_++;
    
    // Verify message signature
    if (config_.validate_certificates && !verifyMessage(message)) {
        return GossipMessage{};  // Invalid message, return empty
    }
    
    // Check rate limit
    if (!checkRateLimit(message.sender_id)) {
        return GossipMessage{};  // Rate limited
    }
    
    // Check message age (replay protection)
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    if (now - static_cast<int64_t>(message.timestamp) > config_.max_message_age_sec) {
        return GossipMessage{};  // Message too old
    }
    
    // Handle message based on type
    // 1. Custom handlers are dispatched first (registered via registerCustomHandler)
    {
        std::function<void(const GossipMessage&)> custom_handler;
        {
            std::lock_guard<std::mutex> lock(peers_mutex_);
            auto it = custom_handlers_.find(message.message_type);
            if (it != custom_handlers_.end()) {
                custom_handler = it->second;
            }
        }
        if (custom_handler) {
            custom_handler(message);
        }
    }

    // 2. Built-in type handling
    if (message.message_type == "heartbeat") {
        // Update peer status
        PeerInfo peer;
        peer.peer_id = message.sender_id;
        if (message.payload.contains("endpoint")) {
            peer.endpoint = message.payload["endpoint"].get<std::string>();
        }
        if (message.payload.contains("datacenter")) {
            peer.datacenter = message.payload["datacenter"].get<std::string>();
        }
        if (message.payload.contains("region")) {
            peer.region = message.payload["region"].get<std::string>();
        }
        if (message.payload.contains("version")) {
            peer.version = message.payload["version"].get<uint64_t>();
        }
        
        addPeer(peer);
        
        // Return our heartbeat
        return createHeartbeatMessage();
        
    } else if (message.message_type == "peer_list") {
        // Merge peer list
        if (message.payload.contains("peers") && message.payload["peers"].is_array()) {
            std::vector<PeerInfo> peers = {};

            for (const auto& p : message.payload["peers"]) {
                peers.push_back(PeerInfo::fromJson(p));
            }
            mergePeerList(peers);
        }
        
        // Return our peer list
        return createPeerListMessage();
        
    } else if (message.message_type == "peer_leave") {
        // Remove leaving peer
        removePeer(message.sender_id);
        
        // Acknowledge
        GossipMessage ack;
        ack.message_id = generateMessageId();
        ack.sender_id = config_.local_peer_id;
        ack.message_type = "ack";
        ack.timestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        return ack;
    }
    
    return GossipMessage{};
}

void GossipProtocol::onPeerDiscovered(PeerDiscoveryCallback callback) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    on_peer_discovered_ = std::move(callback);
}

void GossipProtocol::onPeerLost(PeerLostCallback callback) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    on_peer_lost_ = std::move(callback);
}

void GossipProtocol::setRaftMembershipGateFn(RaftMembershipGateFn fn) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    raft_membership_gate_fn_ = std::move(fn);
}

void GossipProtocol::registerCustomHandler(
    const std::string& message_type,
    std::function<void(const GossipMessage&)> handler
) {
    std::lock_guard<std::mutex> lock(peers_mutex_);
    auto it = custom_handlers_.find(message_type);
    if (it != custom_handlers_.end()) {
        std::cerr << "[GossipProtocol] WARNING: duplicate registerCustomHandler for type '"
                  << message_type << "' — previous handler overwritten\n";
    }
    custom_handlers_[message_type] = std::move(handler);
}

nlohmann::json GossipProtocol::getStatistics() const {
    return {
        {"enabled", config_.enabled},
        {"running", running_.load()},
        {"peer_count", getPeerCount()},
        {"healthy_peers", getHealthyPeers().size()},
        {"messages_sent", messages_sent_.load()},
        {"messages_received", messages_received_.load()},
        {"peers_discovered", peers_discovered_.load()},
        {"peers_lost", peers_lost_.load()},
        {"gossip_rounds", gossip_rounds_.load()},
        {"gossip_interval_sec", config_.gossip_interval_sec},
        {"max_peers", config_.max_peers}
    };
}

// Private methods

void GossipProtocol::gossipLoop() {
    while (running_.load()) {
        performGossipRound();
        
        // Sleep for gossip interval
        for (uint32_t i = 0; i < config_.gossip_interval_sec && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void GossipProtocol::cleanupLoop() {
    while (running_.load()) {
        updatePeerHealth();
        
        // Sleep for cleanup interval (half of gossip interval)
        uint32_t cleanup_interval = std::max(config_.gossip_interval_sec / 2, 1);
        for (uint32_t i = 0; i < cleanup_interval && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void GossipProtocol::performGossipRound() {
    gossip_rounds_++;
    
    // Select random peers to gossip with
    auto targets = selectRandomPeers(config_.fanout);
    
    for (const auto& peer : targets) {
        // Send heartbeat
        sendHeartbeat(peer);
        
        // Occasionally send full peer list (every 10 rounds)
        if (gossip_rounds_ % 10 == 0) {
            sendPeerList(peer);
        }
    }
}

void GossipProtocol::sendHeartbeat(const PeerInfo& peer) {
    if (!client_) {
      return;
    }
    
    auto message = createHeartbeatMessage();
    message.signature = signMessage(message);
    
    // Use retry logic with exponential backoff for transient failures
    const bool success = retryWithBackoff([this, &peer, &message]() -> bool {
        try {
            auto response = client_->post(
                peer.endpoint,
                "/api/v1/gossip",
                message.toJson()
            );
            
            if (response.success) {
                messages_sent_++;
                
                // Process response
                auto response_msg = GossipMessage::fromJson(response.body);
                if (!response_msg.sender_id.empty()) {
                    handleMessage(response_msg);
                }
                return true;
            }
            return false;  // Transient failure, retry
        } catch (const std::exception&) {
            return false;  // Exception, retry
        }
    }, 3, 100, 5000);  // max_retries=3, initial_delay=100ms, max_delay=5000ms
    
    if (!success) {
        // All retries exhausted, mark peer as potentially unhealthy
        std::lock_guard<std::mutex> lock(peers_mutex_);
        auto it = peers_.find(peer.peer_id);
        if (it != peers_.end()) {
            // Don't immediately mark unhealthy, wait for timeout
        }
    }
}

void GossipProtocol::sendPeerList(const PeerInfo& peer) {
    if (!client_) {
      return;
    }
    
    auto message = createPeerListMessage();
    message.signature = signMessage(message);
    
    // Use retry logic with exponential backoff for transient failures
    retryWithBackoff([this, &peer, &message]() -> bool {
        try {
            auto response = client_->post(
                peer.endpoint,
                "/api/v1/gossip",
                message.toJson()
            );
            
            if (response.success) {
                messages_sent_++;
                
                // Process response peer list
                auto response_msg = GossipMessage::fromJson(response.body);
                if (response_msg.message_type == "peer_list") {
                    handleMessage(response_msg);
                }
                return true;
            }
            return false;  // Transient failure, retry
        } catch (const std::exception&) {
            return false;  // Exception, retry
        }
    }, 3, 100, 5000);  // max_retries=3, initial_delay=100ms, max_delay=5000ms
}

void GossipProtocol::sendLeaveMessage() {
    if (!client_) {
      return;
    }
    
    auto message = createLeaveMessage();
    message.signature = signMessage(message);
    
    // Send to all known peers
    std::vector<PeerInfo> peers;
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        for (const auto& [id, peer] : peers_) {
            peers.push_back(peer);
        }
    }
    
    for (const auto& peer : peers) {
        // Use retry logic with exponential backoff for transient failures
        retryWithBackoff([this, &peer, &message]() -> bool {
            try {
                client_->post(
                    peer.endpoint,
                    "/api/v1/gossip",
                    message.toJson()
                );
                messages_sent_++;
                return true;
            } catch (const std::exception&) {
                return false;  // Exception, retry
            }
        }, 3, 100, 5000);  // max_retries=3, initial_delay=100ms, max_delay=5000ms
    }
}

void GossipProtocol::bootstrapFromSeedNodes() {
    for (const auto& seed : config_.seed_nodes) {
        PeerInfo peer;
        peer.peer_id = "seed_" + seed;  // Temporary ID until we get real ID
        peer.endpoint = seed;
        peer.first_seen = std::chrono::system_clock::now();
        peer.last_seen = std::chrono::system_clock::now();
        peer.version = 0;
        
        // Add as peer and send initial heartbeat
        addPeer(peer);
        sendHeartbeat(peer);
    }
}

std::vector<PeerInfo> GossipProtocol::selectRandomPeers(size_t count) {
    std::vector<PeerInfo> selected;
    
    std::lock_guard<std::mutex> lock(peers_mutex_);
    
    if (peers_.empty()) {
        return selected;
    }
    
    // Collect healthy peers
    std::vector<PeerInfo> candidates = {};

    for (const auto& [id, peer] : peers_) {
        if (peer.is_healthy) {
            candidates.push_back(peer);
        }
    }
    
    if (candidates.empty()) {
        return selected;
    }
    
    // GOS-3: Use thread_local RNG to avoid data races on the shared generator.
    thread_local std::mt19937 gen(std::random_device{}());

    std::shuffle(candidates.begin(), candidates.end(), gen);
    
    size_t select_count = std::min(count,static_cast<int>(candidates.size()));
    selected.insert(selected.end(), candidates.begin(), candidates.begin() + select_count);
    
    return selected;
}

void GossipProtocol::mergePeerList(const std::vector<PeerInfo>& peers) {
    for (const auto& peer : peers) {
        // Don't add ourselves
        if (peer.peer_id == config_.local_peer_id) {
            continue;
        }
        
        addPeer(peer);
    }
}

void GossipProtocol::updatePeerHealth() {
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_remove;
    
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        
        for (auto& [id, peer] : peers_) {
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                now - peer.last_seen
            ).count();
            
            if (age > config_.peer_remove_sec) {
                // Peer hasn't been seen for too long, mark for removal
                to_remove.push_back(id);
            } else if (age > config_.peer_timeout_sec) {
                // Peer is unhealthy
                peer.is_healthy = false;
            }
        }
    }
    
    // Remove stale peers
    for (const auto& id : to_remove) {
        removePeer(id);
    }
}

void GossipProtocol::syncWithTopology() {
    if (!topology_) {
      return;
    }
    
    // Convert discovered peers to ShardInfo and add to topology.
    // Acquire the lock so this public variant is safe to call from outside.
    std::lock_guard<std::mutex> lock(peers_mutex_);
    syncWithTopologyLocked();
}

// Called only when peers_mutex_ is already held by the current thread.
void GossipProtocol::syncWithTopologyLocked() {
    if (!topology_) {
      return;
    }

    for (const auto& [id, peer] : peers_) {
        if (!peer.is_healthy) {
          continue;
        }

        // Check if already in topology
        if (!topology_->hasShard(peer.peer_id)) {
            ShardInfo shard;
            shard.shard_id = peer.peer_id;
            shard.primary_endpoint = peer.endpoint;
            shard.datacenter = peer.datacenter;
            shard.is_healthy = peer.is_healthy;
            shard.certificate_serial = peer.certificate_serial;

            // CC-4: Gate gossip-driven topology mutations behind the Raft membership
            // protocol.  When a RaftMembershipGateFn is registered, only admit the peer
            // to the routing topology if the gate approves it (i.e., the peer has been
            // confirmed through Raft joint-consensus).  Without a gate (backward compat),
            // use the legacy warn+add path.
            if (raft_membership_gate_fn_) {
                if (raft_membership_gate_fn_(peer.peer_id, peer.endpoint)) {
                    topology_->addShard(shard);
                } else {
                    spdlog::debug("[GOSSIP] Peer '{}' denied by Raft membership gate; "
                                  "tracked for health monitoring only.",
                                  peer.peer_id);
                }
            } else {
                spdlog::warn("[GOSSIP] Adding gossip-discovered peer '{}' to topology. "
                             "This peer has NOT been admitted through Raft membership "
                             "change and MUST NOT affect quorum calculations. "
                             "Ensure quorum is computed only from Raft-confirmed members.",
                             peer.peer_id);
                topology_->addShard(shard);
            }
        } else {
            // Update health status
            topology_->updateHealth(peer.peer_id, peer.is_healthy);
        }
    }
}

std::string GossipProtocol::generateMessageId() const {
    // GOS-3: thread_local RNG to avoid data races on the shared generator.
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<uint64_t> dis;
    
    std::stringstream ss = {};
    ss << "msg_" << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return ss.str();
}

std::string GossipProtocol::signMessage(const GossipMessage& message) const {
    if (config_.private_key_path.empty()) {
        return "";  // No signing if no key configured
    }
    
    try {
        // Load private key with RAII
        FILE_ptr fp(fopen(config_.private_key_path.c_str(), "r"));
        if (!fp) {
            return "";
        }
        
        EVP_PKEY_ptr pkey(PEM_read_PrivateKey(fp.get(), nullptr, nullptr, nullptr));
        if (!pkey) {
            return "";
        }
        fp.reset();  // Close file after reading key
        
        // Create message to sign
        std::string to_sign = message.message_id + message.sender_id + 
                              message.message_type + std::to_string(message.timestamp);
        
        // Sign with RSA-SHA256 using RAII
        EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new());
        if (!ctx) {
            return "";
        }
        
        std::string signature = {};
        
        if (EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) == 1) {
            if (EVP_DigestSignUpdate(ctx.get(), to_sign.c_str(), to_sign.length()) == 1) {
                size_t sig_len = 0;
                if (EVP_DigestSignFinal(ctx.get(), nullptr, &sig_len) == 1) {
                    std::vector<unsigned char> sig(sig_len);
                    if (EVP_DigestSignFinal(ctx.get(), sig.data(), &sig_len) == 1) {
                        // Base64 encode
                        signature.resize(((sig_len + 2) / 3) * 4 + 1);
                        int out_len = EVP_EncodeBlock(
                            reinterpret_cast<unsigned char*>(signature.data()),
                            sig.data(), static_cast<int>(sig_len)
                        );
                        signature.resize(out_len);
                    }
                }
            }
        }
        
        return signature;
    } catch (const std::exception& e) {
        spdlog::error("Exception during message signing: {}", e.what());
        return "";
    }
}

bool GossipProtocol::verifyMessage(const GossipMessage& message) const {
    // For now, accept all messages if validation is disabled
    if (!config_.validate_certificates) {
        return true;
    }

    // If signature is empty and validation is enabled, fail-closed: an
    // unsigned message from a peer must not be accepted when the operator
    // has explicitly enabled certificate validation.
    if (message.signature.empty()) {
        spdlog::warn("GossipProtocol: rejecting unsigned message '{}' from '{}': "
                     "validate_certificates is enabled",
                     message.message_id, message.sender_id);
        return false;
    }

    // GOS-2: Attempt real RSA-SHA256 signature verification when a public key
    // is available for the sender. Public key files are expected at:
    //   {peer_public_keys_dir}/{sender_id}.pem
    if (config_.peer_public_keys_dir.empty()) {
        // No key directory configured — fail-closed: cannot verify signature.
        spdlog::warn("GossipProtocol: rejecting message '{}' from '{}': "
                     "validate_certificates is enabled but peer_public_keys_dir "
                     "is not configured",
                     message.message_id, message.sender_id);
        return false;
    }

    try {
        const std::string key_path = config_.peer_public_keys_dir + "/" +
                                     message.sender_id + ".pem";
        FILE_ptr fp(fopen(key_path.c_str(), "r"));
        if (!fp) {
            spdlog::warn("GossipProtocol: no public key file '{}' for peer '{}' — "
                         "rejecting signed message", key_path, message.sender_id);
            return false;
        }

        EVP_PKEY_ptr pkey(PEM_read_PUBKEY(fp.get(), nullptr, nullptr, nullptr));
        if (!pkey) {
            spdlog::error("GossipProtocol: failed to load public key from '{}': {}",
                          key_path, ERR_reason_error_string(ERR_get_error()));
            return false;
        }

        // Reconstruct the signed payload (must match signMessage())
        const std::string to_verify = message.message_id + message.sender_id +
                                      message.message_type +
                                      std::to_string(message.timestamp);

        // Base64-decode the stored signature
        const std::string& b64 = message.signature;
        std::vector<unsigned char> sig(b64.size());
        const int decoded_len = EVP_DecodeBlock(
            sig.data(),
            reinterpret_cast<const unsigned char*>(b64.data()),
            static_cast<int>(b64.size())
        );
        if (decoded_len <= 0) {
            spdlog::warn("GossipProtocol: base64 decode failed for signature from '{}'",
                         message.sender_id);
            return false;
        }
        // EVP_DecodeBlock pads to a multiple of 3 — strip trailing padding bytes.
        int sig_len = decoded_len;
        const size_t pad = std::count(b64.rbegin(), b64.rbegin() + 2, '=');
        sig_len -= static_cast<int>(pad);

        EVP_MD_CTX_ptr ctx(EVP_MD_CTX_new());
        bool valid = false;

        if (ctx) {
            if (EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, pkey.get()) == 1 &&
                EVP_DigestVerifyUpdate(ctx.get(), to_verify.data(),static_cast<int>(to_verify.size())) == 1 &&
                EVP_DigestVerifyFinal(ctx.get(),
                                      sig.data(),
                                      static_cast<size_t>(sig_len)) == 1) {
                valid = true;
            } else {
                spdlog::warn("GossipProtocol: signature verification failed for "
                             "message '{}' from '{}'",
                             message.message_id, message.sender_id);
            }
        }

        return valid;
    } catch (const std::exception& e) {
        spdlog::error("Exception during message verification: {}", e.what());
        return false;
    }
}

bool GossipProtocol::checkRateLimit(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto window_start = now - std::chrono::seconds(config_.gossip_interval_sec);
    
    // Clean old entries
    auto& timestamps = rate_limit_map_[peer_id];
    timestamps.erase(
        std::remove_if(timestamps.begin(), timestamps.end(),
            [&window_start](const auto& ts) { return ts < window_start; }),
        timestamps.end()
    );
    
    // Check limit
    if (static_cast<int>(timestamps.size()) >= config_.rate_limit_per_peer) {
        return false;  // Rate limited
    }
    
    // Add current timestamp
    timestamps.push_back(now);
    return true;
}

GossipMessage GossipProtocol::createHeartbeatMessage() const {
    GossipMessage msg;
    msg.message_id = generateMessageId();
    msg.sender_id = config_.local_peer_id;
    msg.message_type = "heartbeat";
    msg.payload = {
        {"endpoint", config_.local_endpoint},
        {"datacenter", config_.local_datacenter},
        {"region", config_.local_region},
        {"version", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    msg.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return msg;
}

GossipMessage GossipProtocol::createPeerListMessage() const {
    GossipMessage msg;
    msg.message_id = generateMessageId();
    msg.sender_id = config_.local_peer_id;
    msg.message_type = "peer_list";
    
    nlohmann::json peers_json = nlohmann::json::array();
    {
        std::lock_guard<std::mutex> lock(peers_mutex_);
        for (const auto& [id, peer] : peers_) {
            if (peer.is_healthy) {
                peers_json.push_back(peer.toJson());
            }
        }
    }
    
    msg.payload = {{"peers", peers_json}};
    msg.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return msg;
}

GossipMessage GossipProtocol::createLeaveMessage() const {
    GossipMessage msg;
    msg.message_id = generateMessageId();
    msg.sender_id = config_.local_peer_id;
    msg.message_type = "peer_leave";
    msg.payload = {};
    msg.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    return msg;
}

} // namespace sharding
} // namespace themis

