/**
 * @file udp_fast_path.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB UDP Fast-Path for Read-Only Queries
// Connectionless, low-overhead alternative transport for GET/QUERY/VECTOR_SEARCH.
//
// Packet format (request):
//   [0-1]  Magic:      0x54 0x55 ("TU")
//   [2]    Version:    0x01
//   [3]    OpCode:     0x10 GET | 0x20 QUERY | 0x40 VECTOR_SEARCH | 0xFE PING
//   [4-7]  RequestID:  caller-generated uint32_t (big-endian), echoed in response
//   [8-9]  PayloadLen: uint16_t (big-endian), length of payload that follows
//   [10+]  Payload:    JSON-encoded parameters for the given OpCode
//
// Packet format (response):
//   [0-1]  Magic:      0x54 0x52 ("TR")
//   [2]    Version:    0x01
//   [3]    Status:     0x00 OK | 0x01 ERROR | 0x02 NOT_FOUND | 0x03 RATE_LIMITED
//   [4-7]  RequestID:  echoed from request
//   [8-9]  PayloadLen: uint16_t (big-endian)
//   [10+]  Payload:    JSON result or error message
//
// Only read-only opcodes are accepted.  Write opcodes are rejected with
// status ERROR and an explanatory payload.

#pragma once

#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <thread>

namespace themis {

class RocksDBWrapper;

namespace network {

namespace net = boost::asio;
using udp = net::ip::udp;

// ─────────────────────────────────────────────────────────────────────────────
// Packet-format constants
// ─────────────────────────────────────────────────────────────────────────────

/// Two-byte magic that begins every UDP fast-path request.
constexpr uint8_t kUdpFastPathReqMagic0 = 0x54;  // 'T'
constexpr uint8_t kUdpFastPathReqMagic1 = 0x55;  // 'U'

/// Two-byte magic that begins every UDP fast-path response.
constexpr uint8_t kUdpFastPathRespMagic0 = 0x54;  // 'T'
constexpr uint8_t kUdpFastPathRespMagic1 = 0x52;  // 'R'

constexpr uint8_t kUdpFastPathVersion = 0x01;

/// Minimum header size (bytes) for a well-formed request.
constexpr std::size_t kUdpFastPathHeaderSize = 10;

/// Maximum total datagram size accepted (platform UDP max is ~65 507 bytes).
constexpr std::size_t kUdpFastPathMaxPacketSize = 65507;

/// Read-only OpCodes accepted on the UDP fast path.
enum class UdpOpCode : uint8_t {
    GET           = 0x10,
    QUERY_AQL     = 0x20,
    VECTOR_SEARCH = 0x40,
    PING          = 0xFE,
};

/// Response status codes.
enum class UdpStatus : uint8_t {
    OK           = 0x00,
#ifdef ERROR
#undef ERROR
#endif
    ERROR        = 0x01,
    NOT_FOUND    = 0x02,
    RATE_LIMITED = 0x03,
};

// ─────────────────────────────────────────────────────────────────────────────
// UDPFastPath
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief UDP fast-path transport for read-only ThemisDB queries.
 *
 * Binds a UDP socket and processes incoming datagrams asynchronously on a
 * small, dedicated thread pool.  Only GET, QUERY_AQL, VECTOR_SEARCH, and
 * PING opcodes are accepted; any attempt to use a write opcode is rejected
 * with a structured error response.
 *
 * Thread safety: start()/stop() must not be called concurrently.  All other
 * state is protected internally.
 */
class UDPFastPath {
public:
    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        std::string host       = "0.0.0.0";
        uint16_t    port       = 8769;     // Dedicated UDP port
        std::size_t num_threads = 2;       // I/O + dispatch threads

        /// Maximum datagram size the server accepts.  Oversized packets are
        /// silently dropped to avoid allocating unbounded memory.
        std::size_t max_packet_size = kUdpFastPathMaxPacketSize;

        /// Per-source-IP rate limit: maximum datagrams per second.
        uint32_t max_packets_per_second_per_ip = 10000;

        Config() = default;
    };

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t packets_received    = 0;
        uint64_t packets_dropped     = 0;  ///< Oversized, malformed, or rate-limited
        uint64_t packets_sent        = 0;
        uint64_t bytes_received      = 0;
        uint64_t bytes_sent          = 0;
        uint64_t parse_errors        = 0;
        uint64_t write_op_rejections = 0;  ///< Write opcodes refused
        uint64_t rate_limit_drops    = 0;
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @brief Construct the fast-path server.
     *
     * @param config   Server configuration.
     * @param storage  Shared storage handle for GET lookups.  May be nullptr;
     *                 in that case GET returns a NOT_FOUND response.
     */
    explicit UDPFastPath(const Config& config,
                         std::shared_ptr<RocksDBWrapper> storage = nullptr);

    ~UDPFastPath();

    /// Bind the socket and start I/O threads.
    void start();

    /// Gracefully stop the server and join all threads.
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    Stats getStats() const;

    // ── Packet helpers (public for unit-test access) ──────────────────────────

    /**
     * @brief Validate the header of a raw UDP datagram.
     *
     * @param data  Raw bytes received.
     * @return true if magic, version, and payload-length field are consistent.
     */
    static bool validatePacket(const std::vector<uint8_t>& data);

    /**
     * @brief Return true if @p opcode is accepted on the UDP fast path.
     *
     * GET and PING are fully implemented.  QUERY_AQL and VECTOR_SEARCH are
     * semantically read-only and are accepted so that clients receive a
     * specific advisory response ("use TCP") rather than the generic
     * "write operations not allowed" error.
     */
    static bool isReadOnlyOpCode(uint8_t opcode);

    /**
     * @brief Build a serialised response datagram.
     *
     * @param request_id  Echoed from the incoming request.
     * @param status      Response status.
     * @param payload     JSON or error string written into the payload field.
     * @return Serialised datagram ready to send.
     */
    static std::vector<uint8_t> buildResponse(uint32_t    request_id,
                                               UdpStatus   status,
                                               const std::string& payload);

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    void doReceive();

    void handleDatagram(const udp::endpoint&        sender,
                        const std::vector<uint8_t>& data);

    bool checkRateLimit(const std::string& ip);

    std::vector<uint8_t> dispatchGet(uint32_t           request_id,
                                     const std::string& payload_json);

    std::vector<uint8_t> dispatchQuery(uint32_t           request_id,
                                       const std::string& payload_json);

    std::vector<uint8_t> dispatchVectorSearch(uint32_t           request_id,
                                              const std::string& payload_json);

    std::vector<uint8_t> dispatchPing(uint32_t request_id);

    // ── Members ──────────────────────────────────────────────────────────────

    Config config_;
    std::shared_ptr<RocksDBWrapper> storage_;

    std::unique_ptr<net::io_context> io_ctx_;
    std::unique_ptr<udp::socket>     socket_;
    std::vector<std::thread>         threads_;

    std::atomic<bool> running_{false};

    // Receive staging buffer (reused across receives; single receive at a time)
    std::vector<uint8_t> recv_buf_;
    udp::endpoint        sender_endpoint_;

    // Per-IP rate-limit state
    struct RateLimitEntry {
        uint64_t window_start_ms = 0;
        uint32_t count           = 0;
    };
    mutable std::mutex rate_mutex_;
    std::unordered_map<std::string, RateLimitEntry> rate_limits_;

    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
};

}  // namespace network
}  // namespace themis
