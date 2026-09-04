/**
 * @file udp_fast_path.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB – UDP Fast-Path for Read-Only Queries
// See include/network/udp_fast_path.h for protocol documentation.

#include "network/udp_fast_path.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <cstring>
#include <limits>
#ifdef _WIN32
#  include <winsock2.h>   // ntohl / ntohs / htonl / htons
#else
#  include <arpa/inet.h>
#endif

using json = nlohmann::json;

namespace themis::network {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

UDPFastPath::UDPFastPath(const Config& config,
                         std::shared_ptr<RocksDBWrapper> storage)
    : config_(config)
    , storage_(std::move(storage))
    , io_ctx_(std::make_unique<net::io_context>())
{
    recv_buf_.resize(std::min(config_.max_packet_size, kUdpFastPathMaxPacketSize));
}

UDPFastPath::~UDPFastPath() {
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// start / stop
// ─────────────────────────────────────────────────────────────────────────────

void UDPFastPath::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;  // Already running
    }

    udp::endpoint endpoint(net::ip::make_address(config_.host), config_.port);
    socket_ = std::make_unique<udp::socket>(*io_ctx_, endpoint);
    // Mark running only after the socket is successfully bound so that a
    // bind failure does not leave isRunning() returning true with no socket.
    running_.store(true, std::memory_order_release);

    THEMIS_INFO("[UDPFastPath] listening on {}:{}", config_.host, config_.port);

    doReceive();

    const std::size_t n = std::max<std::size_t>(1, config_.num_threads);
    threads_.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        threads_.emplace_back([this] { io_ctx_->run(); });
    }
}

void UDPFastPath::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;  // Already stopped
    }

    if (socket_) {
        boost::system::error_code ec;
        socket_->close(ec);
    }
    io_ctx_->stop();

    for (auto& t : threads_) {
        if (t.joinable()) {
          t.join();
        }
    }
    threads_.clear();

    io_ctx_->restart();
    THEMIS_INFO("[UDPFastPath] stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Receive loop
// ─────────────────────────────────────────────────────────────────────────────

void UDPFastPath::doReceive() {
    socket_->async_receive_from(
        net::buffer(recv_buf_),
        sender_endpoint_,
        [this](const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                if (ec != net::error::operation_aborted) {
                    THEMIS_ERROR("[UDPFastPath] receive error: {}", ec.message());
                }
                return;
            }

            // Copy datagram before re-arming the receive
            std::vector<uint8_t> datagram(recv_buf_.begin(),
                                          recv_buf_.begin() + bytes);
            udp::endpoint sender = sender_endpoint_;

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                ++stats_.packets_received;
                stats_.bytes_received += bytes;
            }

            handleDatagram(sender, datagram);

            // Re-arm for next datagram
            if (running_.load(std::memory_order_acquire)) {
                doReceive();
            }
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// Datagram dispatch
// ─────────────────────────────────────────────────────────────────────────────

void UDPFastPath::handleDatagram(const udp::endpoint&        sender,
                                  const std::vector<uint8_t>& data) {
    const std::string ip = sender.address().to_string();

    // ── Rate limiting ──────────────────────────────────────────────────────
    if (!checkRateLimit(ip)) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.packets_dropped;
            ++stats_.rate_limit_drops;
        }
        THEMIS_WARN("[UDPFastPath] rate-limited {}", ip);

        // Build and send RATE_LIMITED response if we have a request ID.
        // The send_to call is intentionally outside the stats lock to avoid
        // holding a mutex during a blocking I/O operation.
        if (data.size() >= kUdpFastPathHeaderSize) {
            uint32_t req_id_be = 0;
            std::memcpy(&req_id_be, data.data() + 4, 4);
            uint32_t request_id = ntohl(req_id_be);
            auto resp = buildResponse(request_id, UdpStatus::RATE_LIMITED,
                                      R"({"error":"rate limited"})");
            boost::system::error_code send_ec;
            socket_->send_to(net::buffer(resp), sender, 0, send_ec);
        }
        return;
    }

    // ── Packet validation ─────────────────────────────────────────────────
    if (!validatePacket(data)) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.packets_dropped;
        ++stats_.parse_errors;
        THEMIS_WARN("[UDPFastPath] malformed packet from {}", ip);
        return;
    }

    // ── Parse header ──────────────────────────────────────────────────────
    const uint8_t opcode = data[3];

    uint32_t req_id_be;
    std::memcpy(&req_id_be, data.data() + 4, 4);
    const uint32_t request_id = ntohl(req_id_be);

    uint16_t payload_len_be;
    std::memcpy(&payload_len_be, data.data() + 8, 2);
    const uint16_t payload_len = ntohs(payload_len_be);

    std::string payload_json;
    if (payload_len > 0) {
        const uint8_t* payload_start = data.data() + kUdpFastPathHeaderSize;
        payload_json.assign(reinterpret_cast<const char*>(payload_start), payload_len);
    }

    // ── Reject write opcodes ──────────────────────────────────────────────
    if (!isReadOnlyOpCode(opcode)) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.write_op_rejections;
            ++stats_.packets_dropped;
        }
        THEMIS_WARN("[UDPFastPath] write opcode 0x{:02X} rejected from {}",
                    opcode, ip);
        auto resp = buildResponse(request_id, UdpStatus::ERROR,
                                  R"({"error":"write operations not allowed on UDP fast-path"})");
        boost::system::error_code send_ec;
        socket_->send_to(net::buffer(resp), sender, 0, send_ec);
        return;
    }

    // ── Dispatch ──────────────────────────────────────────────────────────
    std::vector<uint8_t> response;
    switch (static_cast<UdpOpCode>(opcode)) {
        case UdpOpCode::GET:
            response = dispatchGet(request_id, payload_json);
            break;
        case UdpOpCode::QUERY_AQL:
            response = dispatchQuery(request_id, payload_json);
            break;
        case UdpOpCode::VECTOR_SEARCH:
            response = dispatchVectorSearch(request_id, payload_json);
            break;
        case UdpOpCode::PING:
            response = dispatchPing(request_id);
            break;
        default:
            response = buildResponse(request_id, UdpStatus::ERROR,
                                     R"({"error":"unknown opcode"})");
            break;
    }

    // ── Send response ─────────────────────────────────────────────────────
    boost::system::error_code send_ec;
    const std::size_t sent = socket_->send_to(net::buffer(response), sender,
                                               0, send_ec);
    if (!send_ec) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.packets_sent;
        stats_.bytes_sent += sent;
    } else {
        THEMIS_ERROR("[UDPFastPath] send error to {}: {}", ip, send_ec.message());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Rate limiting
// ─────────────────────────────────────────────────────────────────────────────

bool UDPFastPath::checkRateLimit(const std::string& ip) {
    const uint64_t now_ms =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
            .count());

    std::lock_guard<std::mutex> lk(rate_mutex_);
    auto& entry = rate_limits_[ip];

    if (now_ms - entry.window_start_ms >= 1000) {
        // Start a new 1-second window
        entry.window_start_ms = now_ms;
        entry.count           = 1;
        return true;
    }

    ++entry.count;
    // Saturate at max to prevent uint32_t wrap-around which could re-admit
    // traffic from a single IP after ~4 billion packets in one window.
    if (entry.count == 0) {
        entry.count = std::numeric_limits<uint32_t>::max();
    }
    return entry.count <= config_.max_packets_per_second_per_ip;
}

// ─────────────────────────────────────────────────────────────────────────────
// Opcode handlers
// ─────────────────────────────────────────────────────────────────────────────

std::vector<uint8_t> UDPFastPath::dispatchGet(uint32_t           request_id,
                                               const std::string& payload_json) {
    if (!storage_) {
        return buildResponse(request_id, UdpStatus::ERROR,
                             R"({"error":"storage not configured"})");
    }

    std::string key;
    try {
        auto j = json::parse(payload_json);
        key    = j.at("key").get<std::string>();
    } catch (const std::exception& ex) {
        json err;
        err["error"] = std::string("invalid payload: ") + ex.what();
        return buildResponse(request_id, UdpStatus::ERROR, err.dump());
    }

    if (key.empty()) {
        return buildResponse(request_id, UdpStatus::ERROR,
                             R"({"error":"key must not be empty"})");
    }

    std::string value;
    const bool found = storage_->get(key, value);
    if (!found) {
        return buildResponse(request_id, UdpStatus::NOT_FOUND,
                             R"({"error":"key not found"})");
    }

    json result;
    result["key"]   = key;
    result["value"] = value;
    return buildResponse(request_id, UdpStatus::OK, result.dump());
}

std::vector<uint8_t> UDPFastPath::dispatchQuery(uint32_t           request_id,
                                                 const std::string& /*payload_json*/) {
    // AQL query execution over UDP is intentionally limited to simple look-ups;
    // complex queries should use the TCP wire protocol.  This returns a
    // structured "not available on UDP fast-path" response so clients can
    // gracefully fall back to TCP.
    return buildResponse(request_id, UdpStatus::ERROR,
                         R"({"error":"AQL queries not available on UDP fast-path; use TCP wire protocol"})");
}

std::vector<uint8_t> UDPFastPath::dispatchVectorSearch(uint32_t           request_id,
                                                        const std::string& /*payload_json*/) {
    return buildResponse(request_id, UdpStatus::ERROR,
                         R"({"error":"vector search not available on UDP fast-path; use TCP wire protocol"})");
}

std::vector<uint8_t> UDPFastPath::dispatchPing([[maybe_unused]] uint32_t request_id) {
    const uint64_t now_ms =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
            .count());

    json pong;
    pong["pong"]      = true;
    pong["timestamp"] = now_ms;
    return buildResponse(request_id, UdpStatus::OK, pong.dump());
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

bool UDPFastPath::validatePacket(const std::vector<uint8_t>& data) {
    if (data.size() < kUdpFastPathHeaderSize) {
        return false;
    }
    // Magic bytes
    if (data[0] != kUdpFastPathReqMagic0 || data[1] != kUdpFastPathReqMagic1) {
        return false;
    }
    // Version
    if (data[2] != kUdpFastPathVersion) {
        return false;
    }
    // Payload length consistency: header says N bytes, datagram must be large enough
    uint16_t payload_len_be;
    std::memcpy(&payload_len_be, data.data() + 8, 2);
    const uint16_t payload_len = ntohs(payload_len_be);
    if (data.size() < kUdpFastPathHeaderSize + payload_len) {
        return false;
    }
    return true;
}

bool UDPFastPath::isReadOnlyOpCode([[maybe_unused]] uint8_t opcode) {
    switch (static_cast<UdpOpCode>(opcode)) {
        case UdpOpCode::GET:
        [[fallthrough]];\n        case UdpOpCode::QUERY_AQL:
        [[fallthrough]];\n        case UdpOpCode::VECTOR_SEARCH:
        [[fallthrough]];\n        case UdpOpCode::PING:
            // These opcodes are semantically read-only and accepted by the
            // fast path.  QUERY_AQL and VECTOR_SEARCH currently return an
            // advisory "use TCP" response because multi-result streaming does
            // not fit the single-datagram model; they remain in this list so
            // that the "write operation rejected" error is not returned for
            // legitimate read requests sent by forward-looking clients.
            return true;
        default:
            return false;
    }
}

std::vector<uint8_t> UDPFastPath::buildResponse(uint32_t           request_id,
                                                  UdpStatus          status,
                                                  const std::string& payload) {
    const uint16_t payload_len =
        static_cast<uint16_t>(std::min<std::size_t>(payload.size(), 0xFFFF));

    std::vector<uint8_t> out;
    out.reserve(kUdpFastPathHeaderSize + payload_len);

    // Magic
    out.push_back(kUdpFastPathRespMagic0);
    out.push_back(kUdpFastPathRespMagic1);
    // Version
    out.push_back(kUdpFastPathVersion);
    // Status
    out.push_back(static_cast<uint8_t>(status));
    // RequestID (big-endian)
    const uint32_t req_id_be = htonl(request_id);
    out.insert(out.end(),
               reinterpret_cast<const uint8_t*>(&req_id_be),
               reinterpret_cast<const uint8_t*>(&req_id_be) + 4);
    // PayloadLen (big-endian)
    const uint16_t plen_be = htons(payload_len);
    out.insert(out.end(),
               reinterpret_cast<const uint8_t*>(&plen_be),
               reinterpret_cast<const uint8_t*>(&plen_be) + 2);
    // Payload
    out.insert(out.end(),
               reinterpret_cast<const uint8_t*>(payload.data()),
               reinterpret_cast<const uint8_t*>(payload.data()) + payload_len);

    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

UDPFastPath::Stats UDPFastPath::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

}  // namespace themis::network
