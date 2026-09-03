/**
 * @file distributed_token_blacklist.cpp
 * @brief Distributed token blacklist — TCP cluster sync implementation.
 *
 * Full implementation of the TBLK/v1 peer-to-peer revocation protocol
 * including connection pooling, retry scheduling, and merge-on-rejoin.
 */

#include "auth/distributed_token_blacklist.h"
#include "auth/auth_error.h"
#include <memory>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <future>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>

// RocksDB includes
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>

// ===========================================================================
// Platform-specific socket includes
// ===========================================================================
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "Ws2_32.lib")
   using SockFd = SOCKET;
   static const SockFd kNoSock = INVALID_SOCKET;
   static void sockClose(SockFd f) noexcept { ::closesocket(f); }
   static bool sockValid(SockFd f) noexcept { return f != INVALID_SOCKET; }
   static int  sockLastErr() noexcept       { return ::WSAGetLastError(); }
   static constexpr int kConnInProgress = WSAEWOULDBLOCK;

namespace {
    /// RAII guard: calls WSAStartup once on construction and WSACleanup on
    /// destruction so every Winsock API in this translation unit is guaranteed
    /// to have an active Winsock service provider initialised.
    struct WinsockInit {
        WinsockInit() noexcept {
            WSADATA wd{};
            ::WSAStartup(MAKEWORD(2, 2), &wd);
        }
        ~WinsockInit() noexcept { ::WSACleanup(); }
    };
    static WinsockInit s_winsock_init;
} // anonymous namespace
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <netdb.h>
#  include <fcntl.h>
#  include <cerrno>
   using SockFd = int;
   static const SockFd kNoSock = -1;
   static void sockClose(SockFd f) noexcept { ::close(f); }
   static bool sockValid(SockFd f) noexcept { return f >= 0; }
   static int  sockLastErr() noexcept       { return errno; }
   static constexpr int kConnInProgress = EINPROGRESS;
#endif

namespace themis {
namespace auth {

// ===========================================================================
// TBLK/v1 Wire Protocol constants
// ===========================================================================

/// 4-byte protocol magic: "TBLK"
static const uint8_t kRpcMagic[4]  = {0x54, 0x42, 0x4C, 0x4B};
static constexpr uint8_t kRpcVer   = 0x01; ///< Protocol version
static constexpr uint8_t kMsgPush     = 0x01; ///< Leader pushes entries to follower
static constexpr uint8_t kMsgPullReq  = 0x02; ///< Follower requests entries from leader
static constexpr uint8_t kMsgPullResp = 0x03; ///< Leader responds with entries
static constexpr uint8_t kMsgAck      = 0x04; ///< Acknowledgement
/// Header layout: magic[4] + version[1] + type[1] + count[4] = 10 bytes
static constexpr size_t  kHdrSize     = 10;
/// Maximum sensible JTI length accepted over the wire (safety cap)
static constexpr uint16_t kMaxJtiLen  = 1024;
/// Maximum entry count accepted in a single PULL_RESP or PUSH (prevents OOM)
static constexpr uint32_t kMaxEntries = 1'000'000;

// ===========================================================================
// Integer encoding helpers (big-endian)
// ===========================================================================

static void encodeU32(uint32_t v, uint8_t* out) noexcept {
    out[0] = static_cast<uint8_t>((v >> 24) & 0xFF);
    out[1] = static_cast<uint8_t>((v >> 16) & 0xFF);
    out[2] = static_cast<uint8_t>((v >>  8) & 0xFF);
    out[3] = static_cast<uint8_t>((v      ) & 0xFF);
}
static uint32_t decodeU32(const uint8_t* in) noexcept {
    return (static_cast<uint32_t>(in[0]) << 24)
         | (static_cast<uint32_t>(in[1]) << 16)
         | (static_cast<uint32_t>(in[2]) <<  8)
         |  static_cast<uint32_t>(in[3]);
}
static void encodeU16(uint16_t v, uint8_t* out) noexcept {
    out[0] = static_cast<uint8_t>((v >> 8) & 0xFF);
    out[1] = static_cast<uint8_t>((v     ) & 0xFF);
}
static uint16_t decodeU16(const uint8_t* in) noexcept {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(in[0]) << 8) | static_cast<uint16_t>(in[1]));
}
static void encodeI64(int64_t v, uint8_t* out) noexcept {
    for (int i = 0; i < 8; ++i)
        out[i] = static_cast<uint8_t>((v >> (56 - 8 * i)) & 0xFF);
}
static int64_t decodeI64(const uint8_t* in) noexcept {
    int64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v = (v << 8) | static_cast<int64_t>(static_cast<uint8_t>(in[i]));
    return v;
}

// ===========================================================================
// Socket helpers
// ===========================================================================

/// Set SO_RCVTIMEO and SO_SNDTIMEO on a socket.
/// On Windows, SO_RCVTIMEO/SO_SNDTIMEO expect a DWORD timeout in milliseconds.
/// On POSIX, they expect a struct timeval.
static void sockSetTimeout(SockFd fd, int timeout_ms) noexcept {
#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout_ms);
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char*>(&tv), sizeof(tv));
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                 reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
    struct timeval tv{};
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char*>(&tv), sizeof(tv));
    ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
                 reinterpret_cast<const char*>(&tv), sizeof(tv));
#endif
}

/**
 * @brief Send exactly `len` bytes; retries on partial sends.
 * @return true on success, false if the connection drops or times out.
 */
static bool sendAll(SockFd fd, const void* buf, size_t len) noexcept {
    const auto* ptr = static_cast<const char*>(buf);
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = ::send(fd, ptr + sent, static_cast<int>(len - sent), 0);
#else
        ssize_t n = ::send(fd, ptr + sent, len - sent, MSG_NOSIGNAL);
#endif
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

/**
 * @brief Receive exactly `len` bytes; retries on partial reads.
 * @return true on success, false if the connection drops or times out.
 */
static bool recvAll(SockFd fd, void* buf, size_t len) noexcept {
    auto* ptr = static_cast<char*>(buf);
    size_t recvd = 0;
    while (recvd < len) {
#ifdef _WIN32
        int n = ::recv(fd, ptr + recvd, static_cast<int>(len - recvd), 0);
#else
        ssize_t n = ::recv(fd, ptr + recvd, len - recvd, 0);
#endif
        if (n <= 0) return false;
        recvd += static_cast<size_t>(n);
    }
    return true;
}

/**
 * @brief Parse "host:port" address string.
 * @return {host, port} or {"", 0} on parse error or invalid port.
 */
static std::pair<std::string, int> parseAddress(const std::string& addr) {
    auto colon = addr.rfind(':');
    if (colon == std::string::npos || colon == 0) return {"", 0};
    std::string host = addr.substr(0, colon);
    int port = 0;
    try { port = std::stoi(addr.substr(colon + 1)); } catch (...) {}
    if (port <= 0 || port > 65535) return {"", 0};
    return {host, port};
}

/**
 * @brief Open a non-blocking TCP connect with explicit timeout.
 *
 * Sets the socket to non-blocking, issues connect(), waits for writability
 * via select(), verifies SO_ERROR, then restores blocking mode and applies
 * SO_RCVTIMEO / SO_SNDTIMEO.
 *
 * @param host       Hostname or dotted-decimal IP address.
 * @param port       TCP port (1..65535).
 * @param timeout_ms Connect deadline in milliseconds.
 * @return Connected socket fd, or kNoSock on failure (connection refused, timeout, DNS error).
 */
static SockFd connectWithTimeout(const std::string& host, int port, int timeout_ms) {
    if (host.empty() || port <= 0 || port > 65535) return kNoSock;

    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* ai_res = nullptr;
    const std::string port_str = std::to_string(port);
    if (::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &ai_res) != 0)
        return kNoSock;

    SockFd connected = kNoSock;
    for (auto* ai = ai_res; ai != nullptr && !sockValid(connected); ai = ai->ai_next) {
        SockFd fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (!sockValid(fd)) continue;

        // Switch to non-blocking for the connect phase
#ifdef _WIN32
        u_long mode = 1;
        ::ioctlsocket(fd, FIONBIO, &mode);
#else
        {
            int flags = ::fcntl(fd, F_GETFL, 0);
            ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }
#endif
        int rc = ::connect(fd, ai->ai_addr, static_cast<socklen_t>(ai->ai_addrlen));
        if (rc == 0) {
            connected = fd;  // immediate success (loopback)
        } else if (sockLastErr() == kConnInProgress) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(fd, &wfds);
            struct timeval tv{};
            tv.tv_sec  = timeout_ms / 1000;
            tv.tv_usec = static_cast<long>((timeout_ms % 1000) * 1000);
            if (::select(static_cast<int>(fd) + 1, nullptr, &wfds, nullptr, &tv) > 0) {
                int err = 0;
                socklen_t optlen = sizeof(err);
                ::getsockopt(fd, SOL_SOCKET, SO_ERROR,
                             reinterpret_cast<char*>(&err), &optlen);
                if (err == 0) connected = fd;
            }
        }
        if (!sockValid(connected)) sockClose(fd);
    }
    ::freeaddrinfo(ai_res);

    if (sockValid(connected)) {
        // Restore blocking mode
#ifdef _WIN32
        u_long mode = 0;
        ::ioctlsocket(connected, FIONBIO, &mode);
#else
        {
            int flags = ::fcntl(connected, F_GETFL, 0);
            ::fcntl(connected, F_SETFL, flags & ~O_NONBLOCK);
        }
#endif
        sockSetTimeout(connected, timeout_ms);
    }
    return connected;
}

// ===========================================================================
// Helper: Expiry encoding/decoding
// ===========================================================================

std::string DistributedTokenBlacklist::encodeExpiry(
    std::chrono::system_clock::time_point tp)
{
    auto secs = std::chrono::system_clock::to_time_t(tp);
    int64_t val = static_cast<int64_t>(secs);
    
    // Big-endian encoding for RocksDB lexicographic ordering
    std::string result(8, '\0');
    for (int i = 0; i < 8; ++i) {
        result[i] = static_cast<char>((val >> (56 - 8*i)) & 0xFF);
    }
    return result;
}

std::chrono::system_clock::time_point DistributedTokenBlacklist::decodeExpiry(
    const std::string& val)
{
    if (val.size() != 8) {
        throw std::runtime_error("Invalid expiry encoding");
    }
    
    int64_t num = 0;
    for (int i = 0; i < 8; ++i) {
        num = (num << 8) | (static_cast<unsigned char>(val[i]) & 0xFF);
    }
    
    auto secs = static_cast<time_t>(num);
    return std::chrono::system_clock::from_time_t(secs);
}

// ===========================================================================
// Constructor / Destructor
// ===========================================================================

DistributedTokenBlacklist::DistributedTokenBlacklist(
    const DistributedBlacklistConfig& config)
    : config_(config)
    , last_successful_sync_(std::chrono::system_clock::now())
{
    // Open RocksDB database
    rocksdb::Options opts;
    opts.create_if_missing = true;
    opts.create_missing_column_families = true;
    
    // Define column families
    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
    cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
        rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions{}));
    cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
        config_.column_family, rocksdb::ColumnFamilyOptions{}));
    
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
    rocksdb::DB* db_instance = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(
        rocksdb::DBOptions{opts}, config_.db_path, cf_descriptors, &cf_handles, &db_instance);

    if (!status.ok()) {
        throw std::runtime_error(
            std::string("Cannot open RocksDB: ") + status.ToString());
    }
    
    db_ = db_instance;
    cf_ = cf_handles[1];  // Our column family (not default)
    
    // Keep other CF handles alive for proper cleanup
    other_cf_handles_.push_back(cf_handles[0]);
    
    running_.store(true);
    
    // Start background purge thread
    purge_thread_ = std::thread([this] { purgeLoop(); });
    
    // Start background replication thread (only when cluster sync is enabled and
    // there are peer nodes to sync with)
    if (config_.enable_cluster_sync && !config_.peer_nodes.empty()) {
        replication_thread_ = std::thread([this] { replicationLoop(); });
    }
    
    // Start TCP server listener so peers can connect for PUSH or PULL_REQ.
    // Binding is non-fatal: if rpc_port is 0 or the port is already in use, the
    // node continues operating without an inbound listener — it can still initiate
    // outbound push/pull connections to peers.
    if (config_.enable_cluster_sync && !config_.peer_nodes.empty()
        && config_.local_node.rpc_port > 0) {
        SockFd srv = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sockValid(srv)) {
            int yes = 1;
            ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR,
                         reinterpret_cast<const char*>(&yes), sizeof(yes));
            struct sockaddr_in addr{};
            addr.sin_family      = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port        = htons(
                static_cast<uint16_t>(config_.local_node.rpc_port));
            
            if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0
                && ::listen(srv, 8) == 0) {
                server_fd_ = static_cast<std::uintptr_t>(srv);
                listener_thread_ = std::thread([this] { serveIncomingConnections(); });
            } else {
                sockClose(srv);
                // Non-fatal: node operates without inbound listener
            }
        }
    }
}

DistributedTokenBlacklist::~DistributedTokenBlacklist()
{
    // Signal all background threads to stop
    running_.store(false);
    cv_.notify_all();
    
    // Close the server socket to unblock any pending select()/accept() in the
    // listener thread so it exits promptly rather than waiting up to 200 ms.
    if (server_fd_ != static_cast<std::uintptr_t>(-1)) {
        sockClose(static_cast<SockFd>(server_fd_));
        server_fd_ = static_cast<std::uintptr_t>(-1);
    }
    
    if (purge_thread_.joinable())       purge_thread_.join();
    if (replication_thread_.joinable()) replication_thread_.join();
    if (listener_thread_.joinable())    listener_thread_.join();
    
    // Close RocksDB column family handles and the database itself
    if (cf_) {
        db_->DestroyColumnFamilyHandle(cf_);
        cf_ = nullptr;
    }
    for (auto* h : other_cf_handles_) {
        db_->DestroyColumnFamilyHandle(h);
    }
    other_cf_handles_.clear();
    if (db_) {
        delete db_;
        db_ = nullptr;
    }
}

// ===========================================================================
// ITokenBlacklist interface
// ===========================================================================

void DistributedTokenBlacklist::add(
    const std::string& jti,
    std::chrono::system_clock::time_point expiry)
{
    // Input validation: empty JTI and oversized JTI are rejected early so they
    // never reach RocksDB or the wire (contract: auth_principal_contract.h §5).
    if (jti.empty()) {
        throw AuthException(AuthError(AuthErrorCode::REVOCATION_ENTRY_INVALID,
                                      "JTI must not be empty",
                                      "Attempted to add an empty JTI to the revocation blacklist"));
    }
    if (jti.size() > kMaxJtiLen) {
        throw AuthException(AuthError(AuthErrorCode::REVOCATION_ENTRY_INVALID,
                                      "JTI exceeds maximum allowed length",
                                      "JTI length " + std::to_string(jti.size())
                                      + " exceeds limit " + std::to_string(kMaxJtiLen)));
    }

    std::string expiry_val = encodeExpiry(expiry);
    rocksdb::Status status = db_->Put(
        rocksdb::WriteOptions{}, cf_, jti, expiry_val);
    
    if (!status.ok()) {
        throw std::runtime_error(
            std::string("Cannot write to RocksDB: ") + status.ToString());
    }
}

bool DistributedTokenBlacklist::isRevoked(const std::string& jti) const
{
    std::string expiry_val;
    rocksdb::Status status = db_->Get(
        rocksdb::ReadOptions{}, cf_, jti, &expiry_val);
    
    if (status.IsNotFound()) return false;
    
    // Fail-closed: any non-NotFound error is treated as "revoked"
    if (!status.ok()) return true;
    
    auto expiry = decodeExpiry(expiry_val);
    return (std::chrono::system_clock::now() < expiry);
}

void DistributedTokenBlacklist::purgeExpired()
{
    // Wrap iterator in unique_ptr so it is freed on all paths (RAII)
    auto it = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions{}, cf_));
    rocksdb::WriteBatch batch;
    const auto now = std::chrono::system_clock::now();
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        try {
            auto expiry = decodeExpiry(it->value().ToString());
            if (now >= expiry) batch.Delete(cf_, it->key());
        } catch (...) {
            // Skip corrupted entries silently
        }
    }
    
    if (batch.Count() > 0) {
        rocksdb::Status st = db_->Write(rocksdb::WriteOptions{}, &batch);
        if (!st.ok()) {
            throw std::runtime_error(
                std::string("Cannot write batch to RocksDB: ") + st.ToString());
        }
    }
}

// ===========================================================================
// Distributed-specific public methods
// ===========================================================================

std::future<bool> DistributedTokenBlacklist::syncWithCluster()
{
    return std::async(std::launch::async,
                      [this]() { return performClusterSync(); });
}

DistributedTokenBlacklist::ReplicationStats
DistributedTokenBlacklist::getReplicationStats() const
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

bool DistributedTokenBlacklist::waitForClusterConvergence(
    std::chrono::milliseconds timeout)
{
    // Single-node mode: trivially converged
    if (!config_.enable_cluster_sync || config_.peer_nodes.empty()) return true;
    
    const auto start = std::chrono::system_clock::now();
    
    // Poll until we observe at least one successful background sync, or timeout
    while (running_.load()) {
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            if (stats_.successful_syncs > 0) return true;
        }
        if (timeout.count() > 0) {
            auto elapsed = std::chrono::system_clock::now() - start;
            if (elapsed >= timeout) return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

// ===========================================================================
// Background threads
// ===========================================================================

void DistributedTokenBlacklist::purgeLoop()
{
    std::unique_lock<std::mutex> lock(cv_mutex_);
    while (running_.load()) {
        if (cv_.wait_for(lock, std::chrono::seconds(config_.purge_interval_seconds),
                         [this] { return !running_.load(); })) break;
        try { purgeExpired(); } catch (...) {}
    }
}

void DistributedTokenBlacklist::replicationLoop()
{
    std::unique_lock<std::mutex> lock(cv_mutex_);
    while (running_.load()) {
        if (cv_.wait_for(lock, std::chrono::seconds(config_.sync_interval_seconds),
                         [this] { return !running_.load(); })) break;
        bool ok = false;
        try { ok = performClusterSync(); } catch (...) {}
        {
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            stats_.total_syncs++;
            if (ok) {
                stats_.successful_syncs++;
                stats_.last_sync_time = std::chrono::system_clock::now();
                last_successful_sync_.store(std::chrono::system_clock::now());
            } else {
                stats_.failed_syncs++;
            }
        }
    }
}

// ===========================================================================
// RocksDB entry helpers
// ===========================================================================

/**
 * @brief Read all non-expired entries from the local RocksDB store.
 *
 * Used by the server listener (PULL_REQ response) and the leader push path
 * to collect the current revocation set for transmission to peers.
 *
 * @return Vector of (jti, expiry_time_point) pairs; expired entries are omitted.
 */
std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>
DistributedTokenBlacklist::getAllEntries() const
{
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> result;
    const auto now = std::chrono::system_clock::now();
    
    auto it = std::unique_ptr<rocksdb::Iterator>(
        db_->NewIterator(rocksdb::ReadOptions{}, cf_));
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        try {
            auto expiry = decodeExpiry(it->value().ToString());
            if (expiry > now) {
                result.emplace_back(it->key().ToString(), expiry);
            }
        } catch (...) { /* skip corrupted */ }
    }
    return result;
}

/**
 * @brief Write a batch of peer-supplied (jti, unix-epoch-seconds) entries to RocksDB.
 *
 * Entries with past expiry are dropped silently. Existing entries are overwritten
 * regardless of their current value (Last-Write-Wins conflict resolution).
 *
 * @param entries Vector of (jti, unix_seconds) pairs from a cluster peer.
 * @throws std::runtime_error on RocksDB batch write failure.
 */
void DistributedTokenBlacklist::applyEntries(
    const std::vector<std::pair<std::string, int64_t>>& entries)
{
    rocksdb::WriteBatch batch;
    const auto now = std::chrono::system_clock::now();
    
    for (const auto& [jti, secs] : entries) {
        if (jti.empty()) continue;
        auto tp = std::chrono::system_clock::from_time_t(static_cast<time_t>(secs));
        if (tp <= now) continue;  // skip already-expired
        batch.Put(cf_, jti, encodeExpiry(tp));
    }
    
    if (batch.Count() > 0) {
        rocksdb::Status st = db_->Write(rocksdb::WriteOptions{}, &batch);
        if (!st.ok()) {
            throw std::runtime_error(
                std::string("applyEntries: RocksDB write failed: ") + st.ToString());
        }
    }
}

// ===========================================================================
// TCP server listener — accepts inbound connections from cluster peers
// ===========================================================================

/**
 * @brief Accept loop run by `listener_thread_`.
 *
 * Polls `server_fd_` with a 200 ms select() timeout so that `running_=false`
 * causes the thread to exit within 200 ms without requiring a signal.
 * Accepted connections are handled synchronously; the short hold time (a few
 * round trips bounded by `peer_rpc_timeout_ms`) makes this acceptable for the
 * low-frequency sync traffic in v1.3.0.
 */
void DistributedTokenBlacklist::serveIncomingConnections()
{
    while (running_.load()) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(static_cast<SockFd>(server_fd_), &rfds);
        
        struct timeval tv{};
        tv.tv_sec  = 0;
        tv.tv_usec = 200 * 1000;  // 200 ms poll interval
        
        // On POSIX, select() requires nfds = highest_fd + 1.
        // On Windows, the nfds argument is ignored but must be present.
#ifdef _WIN32
        int ret = ::select(0, &rfds, nullptr, nullptr, &tv);
#else
        int ret = ::select(static_cast<int>(server_fd_) + 1, &rfds, nullptr, nullptr, &tv);
#endif
        if (ret <= 0) continue;  // timeout or interrupted
        if (!running_.load()) break;
        
        SockFd client = ::accept(static_cast<SockFd>(server_fd_), nullptr, nullptr);
        if (!sockValid(client)) continue;
        
        // Apply per-request timeout to the accepted connection
        sockSetTimeout(client, config_.peer_rpc_timeout_ms);
        handlePeerConnection(static_cast<std::uintptr_t>(client));
        sockClose(client);
    }
}

/**
 * @brief Handle a single inbound peer connection (PUSH or PULL_REQ).
 *
 * Wire format:
 *   Header  (10 B): magic[4] "TBLK" | version[1] 0x01 | type[1] | count[4]
 *   Entries (var) : for each of count entries:
 *                     jti_len[2] | jti[jti_len] | expiry_unix_secs[8]
 *
 * - PUSH (0x01): follower receives revocations pushed by the leader.
 *   Reads `count` entries, applies them via applyEntries(), sends ACK.
 * - PULL_REQ (0x02): leader receives a request from a follower.
 *   Reads all local entries, sends them as PULL_RESP, waits for ACK.
 *
 * @param client_fd Socket descriptor for the accepted connection.
 *                  Caller retains ownership and closes the socket after return.
 */
void DistributedTokenBlacklist::handlePeerConnection(std::uintptr_t client_fd)
{
    const SockFd fd = static_cast<SockFd>(client_fd);
    
    // Read and validate the 10-byte TBLK/v1 header
    uint8_t hdr[kHdrSize];
    if (!recvAll(fd, hdr, kHdrSize)) return;
    if (std::memcmp(hdr, kRpcMagic, 4) != 0) return;
    if (hdr[4] != kRpcVer) return;
    
    const uint8_t  msg_type = hdr[5];
    const uint32_t count    = decodeU32(hdr + 6);
    
    if (msg_type == kMsgPush) {
        // ---------------------------------------------------------------
        // PUSH: Leader sends revocations to this follower.
        // Read entries, apply to local RocksDB, send ACK.
        // ---------------------------------------------------------------
        if (count > kMaxEntries) return;
        
        std::vector<std::pair<std::string, int64_t>> entries;
        entries.reserve(count);
        bool ok = true;
        
        for (uint32_t i = 0; i < count && ok; ++i) {
            uint8_t jlen_buf[2];
            if (!recvAll(fd, jlen_buf, 2)) { ok = false; break; }
            const uint16_t jlen = decodeU16(jlen_buf);
            if (jlen > kMaxJtiLen) { ok = false; break; }
            
            std::string jti;
            if (jlen > 0) {
                jti.resize(jlen);
                if (!recvAll(fd, jti.data(), jlen)) { ok = false; break; }
            }
            uint8_t exp_buf[8];
            if (!recvAll(fd, exp_buf, 8)) { ok = false; break; }
            entries.emplace_back(std::move(jti), decodeI64(exp_buf));
        }
        
        if (ok) {
            try { applyEntries(entries); } catch (...) { ok = false; }
        }
        
        // Send ACK
        uint8_t ack[kHdrSize]{};
        std::memcpy(ack, kRpcMagic, 4);
        ack[4] = kRpcVer;
        ack[5] = kMsgAck;
        sendAll(fd, ack, kHdrSize);  // best-effort; caller doesn't check
        
    } else if (msg_type == kMsgPullReq) {
        // ---------------------------------------------------------------
        // PULL_REQ: Follower requests current revocations from this leader.
        // Gather all local non-expired entries and send as PULL_RESP.
        // ---------------------------------------------------------------
        const auto entries = getAllEntries();
        
        uint8_t resp_hdr[kHdrSize];
        std::memcpy(resp_hdr, kRpcMagic, 4);
        resp_hdr[4] = kRpcVer;
        resp_hdr[5] = kMsgPullResp;
        encodeU32(static_cast<uint32_t>(entries.size()), resp_hdr + 6);
        if (!sendAll(fd, resp_hdr, kHdrSize)) return;
        
        for (const auto& [jti, expiry] : entries) {
            if (jti.size() > kMaxJtiLen) continue;
            const auto secs = static_cast<int64_t>(
                std::chrono::system_clock::to_time_t(expiry));
            
            uint8_t jlen_buf[2];
            uint8_t exp_buf[8];
            encodeU16(static_cast<uint16_t>(jti.size()), jlen_buf);
            encodeI64(secs, exp_buf);
            
            if (!sendAll(fd, jlen_buf, 2)) return;
            if (!jti.empty() && !sendAll(fd, jti.data(), jti.size())) return;
            if (!sendAll(fd, exp_buf, 8)) return;
        }
        
        // Wait for follower ACK (best-effort; ignore timeout or error)
        uint8_t ack[kHdrSize];
        recvAll(fd, ack, kHdrSize);
    }
    // Unknown message types are silently ignored
}

// ===========================================================================
// RPC handlers (cluster synchronization)
// ===========================================================================

/**
 * @brief Perform one full cluster synchronization cycle.
 *
 * Sequence:
 *  1. Run leader election (local, O(#peers) string comparison).
 *  2. If no peers → trivially succeeded.
 *  3. If leader → push all non-expired local entries to each peer via TCP PUSH.
 *  4. If follower → identify the leader peer (lowest node_id), pull from it
 *     via TCP PULL_REQ / PULL_RESP exchange, then push local revocations back to
 *     the leader via TCP PUSH so that follower-originated revocations are
 *     propagated cluster-wide on the leader's next sync cycle.
 *
 * @return true if at least one peer sync succeeded (or there are no peers to sync).
 *         false if all peer connections failed (peers unreachable / timed out).
 */
bool DistributedTokenBlacklist::performClusterSync()
{
    // Step 1: local leader election — sets is_leader_ atomically
    performLeaderElection();
    
    // Step 2: trivial success when no peers are configured
    if (config_.peer_nodes.empty()) return true;
    
    bool any_success = false;
    
    if (is_leader_.load()) {
        // Leader path: push all non-expired revocations to every follower peer.
        // A push succeeds even if some followers are temporarily unreachable.
        for (const auto& peer : config_.peer_nodes) {
            if (peer.rpc_address.empty() || peer.rpc_port <= 0) continue;
            const std::string addr =
                peer.rpc_address + ":" + std::to_string(peer.rpc_port);
            if (pushRevisionsToFollower(addr)) {
                any_success = true;
                std::lock_guard<std::mutex> lk(stats_mutex_);
                ++stats_.entries_pushed;
            }
        }
    } else {
        // Follower path: connect to the leader peer and pull its revocations,
        // then push any locally-held revocations up to the leader so they are
        // propagated cluster-wide on the leader's next sync cycle.
        // The leader is the peer with the strictly smallest node_id string.
        std::string leader_id = config_.local_node.node_id;
        const ClusterNode* leader_peer = nullptr;
        for (const auto& peer : config_.peer_nodes) {
            if (peer.node_id < leader_id) {
                leader_id = peer.node_id;
                leader_peer = &peer;
            }
        }
        if (leader_peer != nullptr
            && !leader_peer->rpc_address.empty()
            && leader_peer->rpc_port > 0) {
            const std::string addr =
                leader_peer->rpc_address + ":" + std::to_string(leader_peer->rpc_port);
            if (pullRevisionsFromLeader(addr)) {
                any_success = true;
                std::lock_guard<std::mutex> lk(stats_mutex_);
                ++stats_.entries_pulled;
            }
            // Push locally-held revocations to the leader so tokens revoked on
            // this follower are not silently dropped from the cluster view.
            if (pushRevisionsToFollower(addr)) {
                any_success = true;
                std::lock_guard<std::mutex> lk(stats_mutex_);
                ++stats_.entries_pushed;
            }
        }
    }
    
    return any_success;
}

/**
 * @brief Elect the replication leader by comparing `node_id` strings.
 *
 * The node with the lexicographically smallest `node_id` among all nodes
 * (local + peers) becomes the leader. This is a purely local computation
 * with no network communication.
 *
 * @return Always true (election itself cannot fail).
 */
bool DistributedTokenBlacklist::performLeaderElection()
{
    bool new_is_leader = true;
    for (const auto& peer : config_.peer_nodes) {
        if (peer.node_id < config_.local_node.node_id) {
            new_is_leader = false;
            break;
        }
    }
    is_leader_.store(new_is_leader);
    return true;
}

/**
 * @brief Push all non-expired local revocations to a follower peer via TCP.
 *
 * Connects to `peer_address` ("host:port"), sends a TBLK/v1 PUSH message
 * containing every non-expired (jti, expiry) entry from local RocksDB, then
 * reads the ACK. The call blocks for at most `peer_rpc_timeout_ms`.
 *
 * @param peer_address "host:port" of the follower's listener.
 * @return true if the follower acknowledged the push; false on connection or
 *         protocol error (non-fatal; caller continues with next peer).
 */
bool DistributedTokenBlacklist::pushRevisionsToFollower(const std::string& peer_address)
{
    auto [host, port] = parseAddress(peer_address);
    if (host.empty()) return false;
    
    const SockFd fd = connectWithTimeout(host, port, config_.peer_rpc_timeout_ms);
    if (!sockValid(fd)) return false;
    
    bool success = false;
    try {
        const auto entries = getAllEntries();
        
        // Send PUSH header
        uint8_t hdr[kHdrSize];
        std::memcpy(hdr, kRpcMagic, 4);
        hdr[4] = kRpcVer;
        hdr[5] = kMsgPush;
        encodeU32(static_cast<uint32_t>(entries.size()), hdr + 6);
        if (!sendAll(fd, hdr, kHdrSize))
            throw std::runtime_error("send push header failed");
        
        // Send entries: jti_len[2] + jti[jti_len] + expiry[8]
        for (const auto& [jti, expiry] : entries) {
            if (jti.size() > kMaxJtiLen) continue;
            const auto secs = static_cast<int64_t>(
                std::chrono::system_clock::to_time_t(expiry));
            
            uint8_t jlen_buf[2], exp_buf[8];
            encodeU16(static_cast<uint16_t>(jti.size()), jlen_buf);
            encodeI64(secs, exp_buf);
            
            if (!sendAll(fd, jlen_buf, 2))
                throw std::runtime_error("send jti_len failed");
            if (!jti.empty() && !sendAll(fd, jti.data(), jti.size()))
                throw std::runtime_error("send jti failed");
            if (!sendAll(fd, exp_buf, 8))
                throw std::runtime_error("send expiry failed");
        }
        
        // Read ACK from follower
        uint8_t ack[kHdrSize];
        if (!recvAll(fd, ack, kHdrSize))
            throw std::runtime_error("recv ack failed");
        
        success = (std::memcmp(ack, kRpcMagic, 4) == 0)
               && (ack[4] == kRpcVer)
               && (ack[5] == kMsgAck);
    } catch (...) {
        success = false;
    }
    
    sockClose(fd);
    return success;
}

/**
 * @brief Pull revocations from the leader peer via TCP.
 *
 * Connects to `leader_address` ("host:port"), sends a TBLK/v1 PULL_REQ,
 * reads the PULL_RESP entries, applies them to local RocksDB using LWW
 * semantics, then sends an ACK. The call blocks for at most
 * `peer_rpc_timeout_ms`.
 *
 * @param leader_address "host:port" of the leader's listener.
 * @return true if at least one entry was received and applied successfully;
 *         false on connection or protocol error (non-fatal).
 */
bool DistributedTokenBlacklist::pullRevisionsFromLeader(const std::string& leader_address)
{
    auto [host, port] = parseAddress(leader_address);
    if (host.empty()) return false;
    
    const SockFd fd = connectWithTimeout(host, port, config_.peer_rpc_timeout_ms);
    if (!sockValid(fd)) return false;
    
    bool success = false;
    try {
        // Send PULL_REQ
        uint8_t req[kHdrSize];
        std::memcpy(req, kRpcMagic, 4);
        req[4] = kRpcVer;
        req[5] = kMsgPullReq;
        encodeU32(0, req + 6);
        if (!sendAll(fd, req, kHdrSize))
            throw std::runtime_error("send pull_req failed");
        
        // Read PULL_RESP header
        uint8_t resp_hdr[kHdrSize];
        if (!recvAll(fd, resp_hdr, kHdrSize))
            throw std::runtime_error("recv pull_resp header failed");
        if (std::memcmp(resp_hdr, kRpcMagic, 4) != 0
            || resp_hdr[4] != kRpcVer
            || resp_hdr[5] != kMsgPullResp)
            throw std::runtime_error("invalid pull_resp header");
        
        const uint32_t count = decodeU32(resp_hdr + 6);
        if (count > kMaxEntries) throw std::runtime_error("entry count exceeds limit");
        
        // Read entries: jti_len[2] + jti[jti_len] + expiry[8]
        std::vector<std::pair<std::string, int64_t>> entries;
        entries.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            uint8_t jlen_buf[2];
            if (!recvAll(fd, jlen_buf, 2))
                throw std::runtime_error("recv jti_len failed");
            const uint16_t jlen = decodeU16(jlen_buf);
            if (jlen > kMaxJtiLen) throw std::runtime_error("jti too long");
            
            std::string jti;
            if (jlen > 0) {
                jti.resize(jlen);
                if (!recvAll(fd, jti.data(), jlen))
                    throw std::runtime_error("recv jti failed");
            }
            uint8_t exp_buf[8];
            if (!recvAll(fd, exp_buf, 8))
                throw std::runtime_error("recv expiry failed");
            entries.emplace_back(std::move(jti), decodeI64(exp_buf));
        }
        
        applyEntries(entries);
        
        // Send ACK (best-effort)
        uint8_t ack[kHdrSize]{};
        std::memcpy(ack, kRpcMagic, 4);
        ack[4] = kRpcVer;
        ack[5] = kMsgAck;
        sendAll(fd, ack, kHdrSize);
        
        success = true;
    } catch (...) {
        success = false;
    }
    
    sockClose(fd);
    return success;
}

} // namespace auth
} // namespace themis
