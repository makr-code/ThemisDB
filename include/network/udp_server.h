/**
 * @file udp_server.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB UDP Ingestion Server (v1.8.0)
// Fire-and-forget, connection-less UDP transport for high-throughput
// write operations: metrics, logs, events, and batched payloads.
//
// Packet format (request):
//   [0-1]   Magic:      0x54 0x4D ("TM")
//   [2]     Version:    0x01
//   [3]     OpCode:     METRIC=0x01 | LOG=0x02 | EVENT=0x03 | BATCH=0x04 | PING=0xFE
//   [4-7]   SeqNum:     uint32_t (big-endian); used for deduplication
//   [8]     Flags:      bit-0 = ACK_REQUESTED
//   [9-10]  PayloadLen: uint16_t (big-endian), length of payload that follows
//   [11+]   Payload:    JSON-encoded data for the given OpCode
//
// ACK packet format (server → client, only when FLAGS_ACK_REQUESTED is set):
//   [0-1]  Magic:  0x54 0x41 ("TA")
//   [2]    Version: 0x01
//   [3]    Status:  0x00 OK | 0x01 ERROR | 0x02 RATE_LIMITED | 0x03 DUPLICATE
//   [4-7]  SeqNum:  echoed from request (big-endian)
//
// Trade-offs:
//   ✅ Lower latency (no handshake overhead)
//   ✅ Higher throughput (no flow control overhead)
//   ✅ Lower CPU overhead vs TCP
//   ❌ No reliability guarantee (packets may be lost)
//   ❌ No ordering guarantee
//   ❌ No congestion control

#pragma once

#include <boost/asio.hpp>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace network {

namespace net = boost::asio;
using udp     = net::ip::udp;

// ─────────────────────────────────────────────────────────────────────────────
// Packet-format constants
// ─────────────────────────────────────────────────────────────────────────────

/// Two-byte magic that begins every UDP ingestion server request ("TM").
constexpr uint8_t kUdpServerMagic0 = 0x54;  // 'T'
constexpr uint8_t kUdpServerMagic1 = 0x4D;  // 'M'

/// Two-byte magic that begins every UDP ACK response ("TA").
constexpr uint8_t kUdpServerAckMagic0 = 0x54;  // 'T'
constexpr uint8_t kUdpServerAckMagic1 = 0x41;  // 'A'

/// Protocol version byte.
constexpr uint8_t kUdpServerVersion = 0x01;

/// Minimum header size (bytes) for a well-formed request.
/// magic(2) + version(1) + opcode(1) + seqnum(4) + flags(1) + payloadlen(2) = 11
constexpr std::size_t kUdpServerHeaderSize = 11;

/// Fixed size of an ACK datagram: magic(2)+version(1)+status(1)+seqnum(4) = 8.
constexpr std::size_t kUdpServerAckSize = 8;

/// Maximum total datagram size accepted (platform UDP max is ~65507 bytes).
constexpr std::size_t kUdpServerMaxPacketSize = 65507;

/// Flag bit: client requests an application-level acknowledgement.
constexpr uint8_t kUdpServerFlagAckRequested = 0x01;

// ─────────────────────────────────────────────────────────────────────────────
// OpCodes and status codes
// ─────────────────────────────────────────────────────────────────────────────

// Windows headers may define EVENT as a macro; clear it so enum value names remain valid.
#ifdef EVENT
#undef EVENT
#endif
#ifdef ERROR
#undef ERROR
#endif

/// Write-oriented opcodes accepted by the UDP ingestion server.
enum class UdpServerOpCode : uint8_t {
    METRIC = 0x01,  ///< Single metric data point
    LOG    = 0x02,  ///< Log entry
    EVENT  = 0x03,  ///< Arbitrary event
    BATCH  = 0x04,  ///< Batch of METRIC/LOG/EVENT records (JSON array)
    PING   = 0xFE,  ///< Liveness probe; always replied with ACK
};

/// ACK status codes sent back to the client.
enum class UdpServerStatus : uint8_t {
    OK           = 0x00,  ///< Packet accepted and queued
    ERROR        = 0x01,  ///< Malformed payload or internal error
    RATE_LIMITED = 0x02,  ///< Source IP exceeded its rate limit
    DUPLICATE    = 0x03,  ///< Sequence number already seen; silently dropped
};

// ─────────────────────────────────────────────────────────────────────────────
// UdpPacket – parsed packet handed to the application callback
// ─────────────────────────────────────────────────────────────────────────────

struct UdpPacket {
    UdpServerOpCode opcode;
    uint32_t        seq_num;
    uint8_t         flags;
    std::string     source_ip;
    uint16_t        source_port;
    std::vector<uint8_t> payload;  ///< Raw payload bytes (JSON or binary)
};

// ─────────────────────────────────────────────────────────────────────────────
// UDPServer
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief UDP ingestion server for high-throughput, fire-and-forget writes.
 *
 * Binds a UDP socket and asynchronously receives datagrams from any client.
 * Accepted opcodes: METRIC, LOG, EVENT, BATCH, PING.
 *
 * Features:
 *  - Per-source-IP rate limiting
 *  - Packet deduplication via sequence-number sliding window
 *  - Optional application-level ACKs (enable_acks = true, or per-packet flag)
 *  - Optional batching: accumulates packets and flushes to the handler at a
 *    configurable interval (enable_batching = true)
 *  - Comprehensive statistics (packets, duplicates, rate-limited, acks sent)
 *
 * Thread safety: start()/stop() must not be called concurrently.  All
 * statistics and internal state are protected internally.
 */
class UDPServer {
public:
    // ── Application callback ─────────────────────────────────────────────────

    /// Called for every accepted packet (or batch of packets when batching is
    /// enabled).  Invoked from an internal dispatch thread – must be thread-safe.
    using PacketHandler = std::function<void(const UdpPacket&)>;

    // ── Configuration ────────────────────────────────────────────────────────

    struct Config {
        std::string host         = "0.0.0.0";
        uint16_t    port         = 8768;    ///< Default UDP ingestion port
        std::size_t num_threads  = 4;       ///< I/O + dispatch threads

        /// Maximum datagram size accepted; oversized packets are silently dropped.
        std::size_t max_packet_size = kUdpServerMaxPacketSize;

        /// When true, incoming packets are accumulated in memory and flushed to
        /// the handler at batch_interval_ms intervals instead of immediately.
        bool     enable_batching    = true;
        uint32_t batch_interval_ms  = 100;

        /// When true, the server always sends an ACK for every well-formed
        /// packet.  Clients may also request per-packet ACKs by setting the
        /// FLAG_ACK_REQUESTED bit regardless of this setting.
        bool enable_acks = false;

        /// Per-source-IP rate limit (packets/second).
        uint32_t max_packets_per_second_per_ip = 100000;

        /// Number of recent sequence numbers remembered per source IP for
        /// duplicate detection.  Older entries are evicted in FIFO order.
        std::size_t dedup_window_size = 1024;

        Config() = default;
    };

    // ── Statistics ───────────────────────────────────────────────────────────

    struct Stats {
        uint64_t packets_received   = 0;  ///< Total datagrams accepted by the OS
        uint64_t packets_dropped    = 0;  ///< Malformed, oversized, or otherwise dropped
        uint64_t bytes_received     = 0;
        uint64_t parse_errors       = 0;  ///< Header validation failures
        uint64_t rate_limit_drops   = 0;  ///< Packets dropped due to IP rate-limit
        uint64_t duplicate_drops    = 0;  ///< Packets dropped due to seq-num dedup
        uint64_t acks_sent          = 0;  ///< ACK datagrams dispatched to clients
        uint64_t metrics_ingested   = 0;  ///< METRIC packets delivered to handler
        uint64_t logs_ingested      = 0;  ///< LOG packets delivered to handler
        uint64_t events_ingested    = 0;  ///< EVENT packets delivered to handler
        uint64_t batches_ingested   = 0;  ///< BATCH packets delivered to handler
        uint64_t pings_answered     = 0;  ///< PING packets ACKed
    };

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /**
     * @brief Construct the UDP ingestion server.
     *
     * @param config   Server configuration.
     * @param handler  Application callback invoked for every accepted packet.
     *                 May be nullptr; in that case packets are validated and
     *                 counted but the payload is discarded.
     */
    explicit UDPServer(const Config&  config,
                       PacketHandler  handler = nullptr);

    ~UDPServer();

    /// Bind the socket, start the batch-flush thread (if batching enabled),
    /// and launch I/O threads.
    void start();

    /// Gracefully stop all threads and release the socket.
    void stop();

    bool isRunning() const { return running_.load(std::memory_order_acquire); }

    Stats getStats() const;

    // ── Packet helpers (public for unit-test access) ──────────────────────────

    /**
     * @brief Validate the header of a raw UDP datagram.
     * @return true if magic, version, and payload-length field are consistent.
     */
    static bool validatePacket(const std::vector<uint8_t>& data);

    /**
     * @brief Build an ACK datagram to send back to the client.
     * @param seq_num  Echoed from the original request.
     * @param status   Outcome to report to the client.
     */
    static std::vector<uint8_t> buildAck(uint32_t seq_num, UdpServerStatus status);

private:
    // ── Internal helpers ─────────────────────────────────────────────────────

    void doReceive();
    void handleDatagram(udp::endpoint sender, std::vector<uint8_t> data);
    bool checkRateLimit(const std::string& ip);
    bool checkDuplicate(const std::string& ip, uint32_t seq_num);
    void sendAck(const udp::endpoint& dest, uint32_t seq_num, UdpServerStatus status);
    void dispatchPacket(const UdpPacket& pkt);
    void batchFlushLoop();

    // ── Members ──────────────────────────────────────────────────────────────

    Config        config_;
    PacketHandler handler_;

    std::unique_ptr<net::io_context> io_ctx_;
    std::unique_ptr<udp::socket>     socket_;
    mutable std::mutex               socket_mutex_;
    std::vector<std::thread>         io_threads_;

    std::atomic<bool> running_{false};

    // Receive staging buffer (single receive in-flight at a time)
    std::vector<uint8_t> recv_buf_;
    udp::endpoint        sender_endpoint_;

    // Per-IP rate-limit state
    struct RateLimitEntry {
        uint64_t window_start_ms = 0;
        uint32_t count           = 0;
    };
    mutable std::mutex rate_mutex_;
    std::unordered_map<std::string, RateLimitEntry> rate_limits_;

    // Per-IP sequence-number deduplication (sliding window)
    struct DedupEntry {
        std::deque<uint32_t>       seq_queue;  // oldest-first
        std::unordered_set<uint32_t> seq_set;
    };
    mutable std::mutex dedup_mutex_;
    std::unordered_map<std::string, DedupEntry> dedup_state_;

    // Batch accumulation (used when enable_batching == true)
    mutable std::mutex       batch_mutex_;
    std::condition_variable  batch_cv_;
    std::vector<UdpPacket>   batch_;
    bool                     batch_stop_ = false;
    std::thread              batch_thread_;

    // Statistics
    mutable std::mutex stats_mutex_;
    Stats              stats_;
};

}  // namespace network
}  // namespace themis
