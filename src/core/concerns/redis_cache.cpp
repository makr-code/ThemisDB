/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            redis_cache.cpp                                    ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-14 07:00:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     883                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9c5d3c282d  2026-03-13  fix(core): fix deadlock in RedisCache::invalidatePattern ... ║
    • 973cdd1915  2026-03-13  fix(core): address code review comments for RedisCache ║
    • e1c78c3604  2026-03-13  feat(core): implement RedisCache distributed cache adapte... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file redis_cache.cpp
 * @brief Redis-backed ICache with consistent hashing and pub/sub invalidation.
 *
 * Wire protocol: Redis Serialization Protocol (RESP) over plain TCP sockets.
 * No external Redis client library is required; all I/O is implemented using
 * POSIX sockets (Linux/macOS) with a thin Win32 compatibility shim.
 *
 * CacheEntry serialisation in Redis:
 *   "<version_decimal>\n<timestamp_ms_decimal>\n<payload>"
 * The first two newline-delimited fields are fixed-width-free decimal numbers;
 * the remainder of the value is the raw payload (binary-safe).
 */

#include "core/concerns/redis_cache.h"
#include "utils/logger.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>

#if !defined(_WIN32)
#  include <arpa/inet.h>
#  include <fcntl.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <unistd.h>
#else
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Module-level constants
// ---------------------------------------------------------------------------

static constexpr const char* kDefaultRedisHost = "127.0.0.1";
static constexpr uint16_t    kDefaultRedisPort  = 6379;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Split "host:port" into (host, port).  Returns ("", 0) on failure.
std::pair<std::string, uint16_t> splitHostPort(const std::string& addr) {
    auto colon = addr.rfind(':');
    if (colon == std::string::npos || colon == 0) {
        return {"", 0};
    }
    std::string host = addr.substr(0, colon);
    try {
        int port = std::stoi(addr.substr(colon + 1));
        if (port <= 0 || port > 65535) return {"", 0};
        return {host, static_cast<uint16_t>(port)};
    } catch (...) {
        return {"", 0};
    }
}

/// Parse redis://[:password@]host:port[,host2:port2,...] into a config.
RedisCacheConfig parseRedisUrl(const std::string& url) {
    RedisCacheConfig cfg;
    cfg.nodes.clear();

    std::string body = url;
    // Strip scheme
    if (body.substr(0, 8) == "redis://") {
        body = body.substr(8);
    }

    // Extract optional password: redis://:pass@host:port
    std::string password;
    auto at = body.find('@');
    if (at != std::string::npos) {
        std::string auth = body.substr(0, at);
        body = body.substr(at + 1);
        auto colon = auth.find(':');
        if (colon != std::string::npos) {
            password = auth.substr(colon + 1);
        }
    }
    cfg.password = password;

    // Remaining body may be comma-separated host:port pairs.
    std::istringstream ss(body);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            cfg.nodes.push_back(token);
        }
    }
    if (cfg.nodes.empty()) {
        cfg.nodes.push_back("127.0.0.1:6379");
    }
    return cfg;
}

#if !defined(_WIN32)
inline void closeSocketFd(int& fd) noexcept {
    if (fd >= 0) { ::close(fd); fd = -1; }
}
#else
inline void closeSocketFd(uintptr_t& fd) noexcept {
    if (fd != static_cast<uintptr_t>(~0ULL)) {
        ::closesocket(static_cast<SOCKET>(fd));
        fd = static_cast<uintptr_t>(~0ULL);
    }
}
#endif

} // anonymous namespace

// ---------------------------------------------------------------------------
// Factory methods
// ---------------------------------------------------------------------------

std::unique_ptr<RedisCache> RedisCache::create(const std::string& url) {
    return create(parseRedisUrl(url));
}

std::unique_ptr<RedisCache> RedisCache::create(const RedisCacheConfig& config) {
    return std::unique_ptr<RedisCache>(new RedisCache(config));
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

RedisCache::RedisCache(const RedisCacheConfig& config)
    : config_(config)
    , max_size_(config.max_size)
    , default_ttl_ms_(config.default_ttl_ms)
{
    // Build per-node connection objects.
    for (const auto& addr : config_.nodes) {
        auto nc = std::make_unique<NodeConn>();
        auto [h, p] = splitHostPort(addr);
        nc->host = h.empty() ? kDefaultRedisHost : h;
        nc->port = (p == 0) ? kDefaultRedisPort : p;
        nodes_.push_back(std::move(nc));
    }

    // Build the consistent hash ring.
    buildHashRing();

    // Start background pub/sub subscriber thread.
    sub_thread_ = std::thread(&RedisCache::subscriberLoop, this);

    THEMIS_INFO("RedisCache: initialized with {} node(s), {} ring positions",
                nodes_.size(), hash_ring_.size());
}

RedisCache::~RedisCache() {
    shutdown();
}

// ---------------------------------------------------------------------------
// Consistent hash ring
// ---------------------------------------------------------------------------

/*static*/
uint32_t RedisCache::fnv1a32(const char* data, size_t len) noexcept {
    uint32_t hash = 0x811c9dc5u;
    for (size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint8_t>(data[i]);
        hash *= 0x01000193u;
    }
    return hash;
}

void RedisCache::buildHashRing() {
    hash_ring_.clear();
    for (size_t ni = 0; ni < nodes_.size(); ++ni) {
        for (int v = 0; v < config_.virtual_nodes_per_node; ++v) {
            std::string vkey = nodes_[ni]->host + ":" +
                               std::to_string(nodes_[ni]->port) + "#" +
                               std::to_string(v);
            uint32_t pos = fnv1a32(vkey.data(), vkey.size());
            hash_ring_[pos] = ni;
        }
    }
}

size_t RedisCache::nodeIndexForKey(std::string_view key) const {
    if (nodes_.empty()) return 0;
    if (hash_ring_.empty()) return 0;

    const std::string prefixed = config_.key_prefix + std::string(key);
    uint32_t h = fnv1a32(prefixed.data(), prefixed.size());

    auto it = hash_ring_.lower_bound(h);
    if (it == hash_ring_.end()) {
        it = hash_ring_.begin();
    }
    return it->second;
}

std::string RedisCache::nodeForKey(std::string_view key) const {
    size_t idx = nodeIndexForKey(key);
    if (idx >= nodes_.size()) return "";
    return nodes_[idx]->host + ":" + std::to_string(nodes_[idx]->port);
}

size_t RedisCache::hashRingSize() const {
    return hash_ring_.size();
}

// ---------------------------------------------------------------------------
// TCP / RESP helpers
// ---------------------------------------------------------------------------

RedisCache::SocketFd RedisCache::tcpConnect(const std::string& host,
                                             uint16_t port) const {
#if defined(_WIN32)
    // Initialise Winsock exactly once per process.
    static std::once_flag wsa_init_flag;
    std::call_once(wsa_init_flag, [] {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
    });
#endif

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    if (::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
        return kInvalidSocket;
    }

    SocketFd fd = kInvalidSocket;
    for (auto* p = res; p != nullptr; p = p->ai_next) {
#if defined(_WIN32)
        SOCKET s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        fd = static_cast<SocketFd>(s);
#else
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
#endif
        // Set non-blocking temporarily to apply connect timeout
#if !defined(_WIN32)
        int flags = ::fcntl(fd, F_GETFL, 0);
        ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

        int rv = ::connect(fd,
#if defined(_WIN32)
                           p->ai_addr, static_cast<int>(p->ai_addrlen)
#else
                           p->ai_addr, p->ai_addrlen
#endif
                          );

#if !defined(_WIN32)
        if (rv == -1 && errno == EINPROGRESS) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(fd, &wfds);
            struct timeval tv;
            tv.tv_sec  = config_.connect_timeout_ms / 1000;
            tv.tv_usec = (config_.connect_timeout_ms % 1000) * 1000;
            if (::select(fd + 1, nullptr, &wfds, nullptr, &tv) > 0) {
                int err = 0;
                socklen_t elen = sizeof(err);
                ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen);
                if (err != 0) {
                    closeSocketFd(fd);
                    continue;
                }
                rv = 0;
            } else {
                closeSocketFd(fd);
                continue;
            }
        }
        // Restore blocking
        ::fcntl(fd, F_SETFL, flags);
#endif

        if (rv == 0) {
            // Disable Nagle for lower latency
#if !defined(_WIN32)
            int one = 1;
            ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                         reinterpret_cast<char*>(&one), sizeof(one));
#endif
            break;
        }

        closeSocketFd(fd);
        fd = kInvalidSocket;
    }

    ::freeaddrinfo(res);
    return fd;
}

/*static*/
void RedisCache::closeSocket(SocketFd& fd) noexcept {
    closeSocketFd(fd);
}

/*static*/
bool RedisCache::sendAll(SocketFd fd, const std::string& buf) noexcept {
    size_t total = 0;
    while (total < buf.size()) {
#if defined(_WIN32)
        int sent = ::send(static_cast<SOCKET>(fd),
                          buf.data() + total,
                          static_cast<int>(buf.size() - total), 0);
        if (sent == SOCKET_ERROR) return false;
#else
        ssize_t sent = ::send(fd, buf.data() + total, buf.size() - total,
                              MSG_NOSIGNAL);
        if (sent <= 0) return false;
#endif
        total += static_cast<size_t>(sent);
    }
    return true;
}

/*static*/
bool RedisCache::readLine(SocketFd fd, std::string& out) noexcept {
    out.clear();
    char ch;
    while (true) {
#if defined(_WIN32)
        int n = ::recv(static_cast<SOCKET>(fd), &ch, 1, 0);
        if (n <= 0) return false;
#else
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n <= 0) return false;
#endif
        if (ch == '\n') break;
        if (ch != '\r') out += ch;
    }
    return true;
}

bool RedisCache::redisHandshake(SocketFd fd) const noexcept {
    // AUTH
    if (!config_.password.empty()) {
        std::string cmd = buildRespCommand({"AUTH", config_.password});
        if (!sendAll(fd, cmd)) return false;
        std::string reply;
        if (!readLine(fd, reply)) return false;
        if (reply.empty() || reply[0] == '-') return false;
    }
    // SELECT
    if (config_.db_index != 0) {
        std::string cmd = buildRespCommand({"SELECT",
                                            std::to_string(config_.db_index)});
        if (!sendAll(fd, cmd)) return false;
        std::string reply;
        if (!readLine(fd, reply)) return false;
        if (reply.empty() || reply[0] == '-') return false;
    }
    return true;
}

/*static*/
std::string RedisCache::buildRespCommand(const std::vector<std::string>& args) {
    std::string cmd;
    cmd += '*';
    cmd += std::to_string(args.size());
    cmd += "\r\n";
    for (const auto& a : args) {
        cmd += '$';
        cmd += std::to_string(a.size());
        cmd += "\r\n";
        cmd += a;
        cmd += "\r\n";
    }
    return cmd;
}

bool RedisCache::ensureConnected(NodeConn& nc) const noexcept {
    if (nc.ok && nc.fd != kInvalidSocket) return true;
    closeSocket(nc.fd);
    nc.ok = false;

    nc.fd = tcpConnect(nc.host, nc.port);
    if (nc.fd == kInvalidSocket) return false;

    if (!redisHandshake(nc.fd)) {
        closeSocket(nc.fd);
        return false;
    }
    nc.ok = true;
    return true;
}

/*static*/
bool RedisCache::readReply(SocketFd fd, std::string& out) noexcept {
    std::string first;
    if (!readLine(fd, first) || first.empty()) return false;

    char type = first[0];
    std::string rest = first.substr(1);

    if (type == '+' || type == '-' || type == ':') {
        out = rest;
        return (type != '-');  // '-' is an error reply
    }

    if (type == '$') {
        // Bulk string
        int len = std::stoi(rest);
        if (len < 0) { out = ""; return true; }  // nil bulk string
        std::string data(static_cast<size_t>(len), '\0');
        size_t received = 0;
        while (received < static_cast<size_t>(len)) {
#if defined(_WIN32)
            int n = ::recv(static_cast<SOCKET>(fd),
                           &data[received],
                           static_cast<int>(static_cast<size_t>(len) - received), 0);
            if (n <= 0) return false;
#else
            ssize_t n = ::recv(fd, &data[received],
                               static_cast<size_t>(len) - received, 0);
            if (n <= 0) return false;
#endif
            received += static_cast<size_t>(n);
        }
        // consume trailing CRLF
        std::string crlf;
        readLine(fd, crlf);
        out = std::move(data);
        return true;
    }

    if (type == '*') {
        // Array – used for KEYS / SCAN responses; return first element.
        int count = std::stoi(rest);
        if (count <= 0) { out = ""; return true; }
        // Read all array elements; for SIZE we only count.
        std::string elem;
        for (int i = 0; i < count; ++i) {
            readReply(fd, elem);
        }
        out = std::to_string(count);
        return true;
    }

    return false;
}

std::optional<std::string> RedisCache::sendCommand(
        NodeConn& nc,
        const std::vector<std::string>& args) const noexcept {
    std::lock_guard<std::mutex> lock(nc.mutex);
    if (!ensureConnected(nc)) return std::nullopt;

    std::string cmd = buildRespCommand(args);
    if (!sendAll(nc.fd, cmd)) {
        nc.ok = false;
        closeSocket(nc.fd);
        return std::nullopt;
    }

    std::string reply;
    if (!readReply(nc.fd, reply)) {
        nc.ok = false;
        closeSocket(nc.fd);
        return std::nullopt;
    }
    return reply;
}

std::optional<std::string> RedisCache::sendCommandLocked(
        NodeConn& nc,
        const std::vector<std::string>& args) const noexcept {
    // PRECONDITION: caller holds nc.mutex.
    if (!ensureConnected(nc)) return std::nullopt;

    std::string cmd = buildRespCommand(args);
    if (!sendAll(nc.fd, cmd)) {
        nc.ok = false;
        closeSocket(nc.fd);
        return std::nullopt;
    }

    std::string reply;
    if (!readReply(nc.fd, reply)) {
        nc.ok = false;
        closeSocket(nc.fd);
        return std::nullopt;
    }
    return reply;
}

// ---------------------------------------------------------------------------
// Serialisation
// ---------------------------------------------------------------------------

/*static*/
std::string RedisCache::encodeEntry(const CacheEntry& e) {
    return std::to_string(e.version) + "\n" +
           std::to_string(e.timestamp_ms) + "\n" +
           e.payload;
}

/*static*/
std::optional<CacheEntry> RedisCache::decodeEntry(const std::string& raw) {
    auto nl1 = raw.find('\n');
    if (nl1 == std::string::npos) return std::nullopt;
    auto nl2 = raw.find('\n', nl1 + 1);
    if (nl2 == std::string::npos) return std::nullopt;

    try {
        CacheEntry e;
        e.version      = std::stoull(raw.substr(0, nl1));
        e.timestamp_ms = std::stoull(raw.substr(nl1 + 1, nl2 - nl1 - 1));
        e.payload      = raw.substr(nl2 + 1);
        return e;
    } catch (...) {
        return std::nullopt;
    }
}

// ---------------------------------------------------------------------------
// ICache – core operations
// ---------------------------------------------------------------------------

std::optional<CacheEntry> RedisCache::get(std::string_view key) const {
    if (nodes_.empty()) { ++misses_; return std::nullopt; }

    size_t ni = nodeIndexForKey(key);
    NodeConn& nc = *nodes_[ni];

    const std::string rkey = config_.key_prefix + std::string(key);
    auto reply = sendCommand(nc, {"GET", rkey});
    if (!reply || reply->empty()) {
        ++misses_;
        return std::nullopt;
    }

    auto entry = decodeEntry(*reply);
    if (!entry) {
        ++misses_;
        return std::nullopt;
    }
    ++hits_;
    return entry;
}

bool RedisCache::put(std::string_view key, const CacheEntry& entry,
                     uint64_t ttl_ms) {
    if (nodes_.empty()) return false;

    size_t ni = nodeIndexForKey(key);
    NodeConn& nc = *nodes_[ni];

    const std::string rkey   = config_.key_prefix + std::string(key);
    const std::string rvalue = encodeEntry(entry);

    uint64_t effective_ttl = ttl_ms > 0 ? ttl_ms : default_ttl_ms_.load();

    std::optional<std::string> reply;
    if (effective_ttl > 0) {
        reply = sendCommand(nc, {"SET", rkey, rvalue,
                                 "PX", std::to_string(effective_ttl)});
    } else {
        reply = sendCommand(nc, {"SET", rkey, rvalue});
    }
    return reply.has_value();
}

void RedisCache::invalidate(std::string_view key) {
    if (nodes_.empty()) return;

    size_t ni = nodeIndexForKey(key);
    NodeConn& nc = *nodes_[ni];
    const std::string rkey = config_.key_prefix + std::string(key);
    sendCommand(nc, {"DEL", rkey});

    // Publish cluster-wide invalidation.
    publishInvalidation(std::string(key));
}

void RedisCache::clear() {
    // Issue FLUSHDB on every node.
    for (auto& nc : nodes_) {
        sendCommand(*nc, {"FLUSHDB"});
    }
    publishInvalidation("*");
}

void RedisCache::invalidatePattern(std::string_view pattern) {
    // Use SCAN + DEL to avoid blocking the server.
    const std::string matchPat = config_.key_prefix + std::string(pattern);

    for (auto& nc : nodes_) {
        std::string cursor = "0";
        do {
            std::lock_guard<std::mutex> lock(nc->mutex);
            if (!ensureConnected(*nc)) break;

            // SCAN cursor MATCH pattern COUNT 100
            std::string cmd = buildRespCommand(
                {"SCAN", cursor, "MATCH", matchPat, "COUNT", "100"});
            if (!sendAll(nc->fd, cmd)) {
                nc->ok = false;
                closeSocket(nc->fd);
                break;
            }

            // RESP *2 reply: [new_cursor, [key, key, ...]]
            // Read array header
            std::string line;
            if (!readLine(nc->fd, line) || line.empty() || line[0] != '*') break;

            // First element: new cursor
            std::string cur_reply;
            readReply(nc->fd, cur_reply);
            cursor = cur_reply;

            // Second element: array of keys
            std::string count_line;
            if (!readLine(nc->fd, count_line) || count_line.empty() ||
                count_line[0] != '*') break;

            int num_keys = 0;
            try { num_keys = std::stoi(count_line.substr(1)); } catch (...) {}

            for (int i = 0; i < num_keys; ++i) {
                std::string k;
                readReply(nc->fd, k);
                if (!k.empty()) {
                    // Use sendCommandLocked because nc->mutex is already held.
                    // DEL is best-effort: if it fails the connection is marked
                    // dead and the next sendCommandLocked/sendCommand call will
                    // attempt reconnection.  The invalidation PUBLISH (below) is
                    // still sent so that other nodes can act on the notification.
                    auto del_reply = sendCommandLocked(*nc, {"DEL", k});
                    if (!del_reply) {
                        // Connection lost mid-scan; stop processing this node's
                        // cursor and let the reconnect loop re-establish.
                        break;
                    }
                }
            }
        } while (cursor != "0");
    }

    publishInvalidation(std::string(pattern));
}

// ---------------------------------------------------------------------------
// Pub/sub invalidation
// ---------------------------------------------------------------------------

void RedisCache::publishInvalidation(const std::string& key_or_pattern) {
    if (nodes_.empty() || config_.invalidation_channel.empty()) return;
    // Publish on node 0 (or any node that is connected).
    for (auto& nc : nodes_) {
        auto reply = sendCommand(*nc,
            {"PUBLISH", config_.invalidation_channel, key_or_pattern});
        if (reply) return;  // success; one publish is enough
    }
}

void RedisCache::subscribeInvalidations(InvalidationCallback cb) {
    std::lock_guard<std::mutex> lock(inv_cb_mutex_);
    inv_callback_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// Subscriber loop
// ---------------------------------------------------------------------------

void RedisCache::subscriberLoop() {
    while (!stop_.load(std::memory_order_relaxed)) {
        if (nodes_.empty()) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.reconnect_interval_ms));
            continue;
        }

        // Try each node in order for the subscriber connection.
        SocketFd fd = kInvalidSocket;
        for (auto& nc : nodes_) {
            fd = tcpConnect(nc->host, nc->port);
            if (fd != kInvalidSocket && redisHandshake(fd)) break;
            closeSocket(fd);
            fd = kInvalidSocket;
        }

        if (fd == kInvalidSocket) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.reconnect_interval_ms));
            continue;
        }

        // Subscribe to the invalidation channel.
        std::string sub_cmd = buildRespCommand(
            {"SUBSCRIBE", config_.invalidation_channel});
        if (!sendAll(fd, sub_cmd)) {
            closeSocket(fd);
            continue;
        }

        sub_connected_.store(true, std::memory_order_relaxed);
        subscriberSession(fd);
        sub_connected_.store(false, std::memory_order_relaxed);
        closeSocket(fd);
    }
}

void RedisCache::subscriberSession(SocketFd fd) {
    // Read and discard the SUBSCRIBE confirmation first.
    std::string channel_out, payload_out;
    readPubSubMessage(fd, channel_out, payload_out);

    while (!stop_.load(std::memory_order_relaxed)) {
        channel_out.clear();
        payload_out.clear();
        if (!readPubSubMessage(fd, channel_out, payload_out)) break;
        if (!channel_out.empty() && !payload_out.empty()) {
            dispatchInvalidation(payload_out);
        }
    }
}

/*static*/
bool RedisCache::readPubSubMessage(SocketFd fd,
                                   std::string& channel_out,
                                   std::string& payload_out) noexcept {
    // Pub/sub messages arrive as RESP arrays: *3\r\n … (message|subscribe)
    std::string line;
    if (!readLine(fd, line) || line.empty() || line[0] != '*') return false;

    int count = 0;
    try { count = std::stoi(line.substr(1)); } catch (...) { return false; }

    std::vector<std::string> parts;
    for (int i = 0; i < count; ++i) {
        std::string elem;
        if (!readReply(fd, elem)) return false;
        parts.push_back(std::move(elem));
    }

    // parts[0] = "message", parts[1] = channel, parts[2] = payload
    if (parts.size() >= 3 && parts[0] == "message") {
        channel_out = parts[1];
        payload_out = parts[2];
    } else if (parts.size() >= 2) {
        channel_out = (parts.size() > 1) ? parts[1] : "";
    }
    return true;
}

void RedisCache::dispatchInvalidation(const std::string& payload) {
    std::lock_guard<std::mutex> lock(inv_cb_mutex_);
    if (inv_callback_) {
        inv_callback_(payload);
    }
}

// ---------------------------------------------------------------------------
// ICache – statistics
// ---------------------------------------------------------------------------

size_t RedisCache::size() const {
    if (nodes_.empty()) return 0;

    size_t total = 0;
    for (auto& nc : nodes_) {
        // DBSIZE returns the number of keys in the current DB.
        auto reply = sendCommand(*nc, {"DBSIZE"});
        if (reply) {
            try { total += static_cast<size_t>(std::stoull(*reply)); }
            catch (...) {}
        }
    }
    return total;
}

uint64_t RedisCache::hitCount()  const { return hits_.load();   }
uint64_t RedisCache::missCount() const { return misses_.load(); }

double RedisCache::hitRate() const {
    uint64_t h = hits_.load();
    uint64_t m = misses_.load();
    uint64_t total = h + m;
    return (total == 0) ? 0.0 : static_cast<double>(h) / static_cast<double>(total);
}

// ---------------------------------------------------------------------------
// ICache – configuration
// ---------------------------------------------------------------------------

void RedisCache::setMaxSize(size_t maxSize) {
    max_size_.store(maxSize);
    config_.max_size = maxSize;
}

void RedisCache::setDefaultTTL(uint64_t ttl_ms) {
    default_ttl_ms_.store(ttl_ms);
    config_.default_ttl_ms = ttl_ms;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RedisCache::shutdown() noexcept {
    stop_.store(true, std::memory_order_relaxed);
    if (sub_thread_.joinable()) {
        sub_thread_.join();
    }
    for (auto& nc : nodes_) {
        std::lock_guard<std::mutex> lock(nc->mutex);
        closeSocket(nc->fd);
        nc->ok = false;
    }
}

ProbeResult RedisCache::isHealthy() const {
    if (nodes_.empty()) {
        return ProbeResult::unhealthy("RedisCache: no nodes configured");
    }
    for (auto& nc : nodes_) {
        auto reply = sendCommand(*nc, {"PING"});
        if (!reply) {
            return ProbeResult::unhealthy(
                "RedisCache: cannot reach " + nc->host + ":" +
                std::to_string(nc->port));
        }
    }
    return ProbeResult::healthy();
}

bool RedisCache::isConnected() const {
    for (auto& nc : nodes_) {
        std::lock_guard<std::mutex> lock(nc->mutex);
        if (nc->ok && nc->fd != kInvalidSocket) return true;
    }
    return false;
}

} // namespace concerns
} // namespace core
} // namespace themis
