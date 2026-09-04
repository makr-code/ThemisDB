/**
 * @file udp_server.cpp
 * @brief ThemisDB UDP ingestion server.
 *
 * High-throughput UDP datagram ingestion with optional client-side batching and
 * multi-threaded receive I/O.  Key responsibilities:
 *  - Bind a UDP socket and post datagrams to registered handlers.
 *  - Optional batch-flush background thread for amortising small writes.
 *  - Graceful bounded shutdown via timedJoin (5 s per thread).
 *
 * @note thread_join_no_timeout (W3): batch_thread_.join() and io_threads_ joins
 *   are replaced by timedJoin() to prevent indefinite block.
 *
 * @version 0.0.14
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 90/100
 * @note Gap Summary: total=2; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0
 * @note Status: Production Ready
 */


// ThemisDB – UDP Ingestion Server (v1.8.0)
// See include/network/udp_server.h for protocol documentation.

#include "network/udp_server.h"
#include "utils/logger.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <future>
#include <limits>
#include <string_view>
#include <thread>
#ifdef _WIN32
#  include <winsock2.h>   // ntohl / ntohs / htonl / htons
#else
#  include <arpa/inet.h>
#endif

namespace themis::network {

namespace {

uint64_t fnv1a64(std::string_view value) {
    uint64_t hash = 1469598103934665603;
    for (unsigned char ch : value) {
        hash ^= static_cast<uint64_t>(ch);
        hash *= 1099511628211;
    }
    return hash;
}

std::string anonymizeIpForLog(std::string_view ip) {
    if (ip.empty()) {
        return "peer#unknown";
    }
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "peer#%016llx",
                  static_cast<unsigned long long>(fnv1a64(ip)));
    return std::string(buffer);
}

/// Maximum ms to wait for any single thread to join during shutdown.
/// thread_join_no_timeout (W3): capped to prevent indefinite block.
constexpr int kUdpShutdownJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
///
/// @param t          Thread to join (moved into the internal watcher).
/// @param timeout_ms Maximum wait time in milliseconds (default 5 s).
static void timedJoin(std::thread& t,
                      int timeout_ms = kUdpShutdownJoinTimeoutMs) noexcept {
    if (!t.joinable()) {
      return;
    }
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable()) {
          inner.join();
        }
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) !=
            std::future_status::ready) {
        // thread_join_no_timeout: detach on deadline to avoid indefinite block.
        THEMIS_WARN("[UDPServer] Thread did not finish within {} ms during "
                    "shutdown; detaching.", timeout_ms);
    }
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

UDPServer::UDPServer(const Config& config, PacketHandler handler)
    : config_(config)
    , handler_(std::move(handler))
    , io_ctx_(std::make_unique<net::io_context>())
{
    recv_buf_.resize(std::min(config_.max_packet_size, kUdpServerMaxPacketSize));
}

UDPServer::~UDPServer() {
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// start / stop
// ─────────────────────────────────────────────────────────────────────────────

void UDPServer::start() {
    if (running_.load(std::memory_order_acquire)) {
        return;  // Already running
    }

    udp::endpoint endpoint(net::ip::make_address(config_.host), config_.port);
    {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        socket_ = std::make_unique<udp::socket>(*io_ctx_, endpoint);
    }
    running_.store(true, std::memory_order_release);

    THEMIS_INFO("[UDPServer] listening on {}:{}", config_.host, config_.port);

    // Start batch-flush background thread before I/O threads so the first
    // dispatch has somewhere to go.
    if (config_.enable_batching) {
        batch_stop_ = false;
        batch_thread_ = std::thread([this] { batchFlushLoop(); });
    }

    doReceive();

    const std::size_t n = std::max<std::size_t>(1, config_.num_threads);
    io_threads_.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        io_threads_.emplace_back([this] { io_ctx_->run(); });
    }
}

void UDPServer::stop() {
    if (!running_.exchange(false, std::memory_order_acq_rel)) {
        return;  // Already stopped
    }

    // Signal the batch-flush loop to exit
    if (config_.enable_batching) {
        {
            std::lock_guard<std::mutex> lk(batch_mutex_);
            batch_stop_ = true;
        }
        batch_cv_.notify_all();
        if (batch_thread_.joinable()) {
            // thread_join_no_timeout (W3): bounded join via timedJoin helper.
            timedJoin(batch_thread_);
        }
    }

    // Shut down I/O
    {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        if (socket_) {
            boost::system::error_code ec;
            socket_->cancel(ec);
            socket_->close(ec);
        }
    }
    if (io_ctx_) {
        io_ctx_->stop();
    }

    for (auto& t : io_threads_) {
        // thread_join_no_timeout (W3): bounded join via timedJoin helper.
        timedJoin(t);
    }
    io_threads_.clear();

    if (io_ctx_) {
        io_ctx_->restart();
    }
    THEMIS_INFO("[UDPServer] stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Receive loop
// ─────────────────────────────────────────────────────────────────────────────

void UDPServer::doReceive() {
    udp::socket* socket = nullptr;
    {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        socket = socket_.get();
    }
    if (!socket) {
        return;
    }

    socket->async_receive_from(
        net::buffer(recv_buf_),
        sender_endpoint_,
        [this](const boost::system::error_code& ec, std::size_t bytes) {
            if (ec) {
                if (ec != net::error::operation_aborted) {
                    THEMIS_ERROR("[UDPServer] receive error: {}", ec.message());
                }
                return;
            }

            // Copy datagram and endpoint before re-arming to avoid races
            std::vector<uint8_t> datagram(recv_buf_.begin(),
                                          recv_buf_.begin() + bytes);
            udp::endpoint sender = sender_endpoint_;

            {
                std::lock_guard<std::mutex> lk(stats_mutex_);
                ++stats_.packets_received;
                stats_.bytes_received += bytes;
            }

            handleDatagram(std::move(sender), std::move(datagram));

            if (running_.load(std::memory_order_acquire)) {
                doReceive();
            }
        });
}

// ─────────────────────────────────────────────────────────────────────────────
// Datagram dispatch
// ─────────────────────────────────────────────────────────────────────────────

void UDPServer::handleDatagram(udp::endpoint sender, std::vector<uint8_t> data) {
    const std::string ip   = sender.address().to_string();
    const uint16_t    port = sender.port();

    // ── Rate limiting ──────────────────────────────────────────────────────
    if (!checkRateLimit(ip)) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.packets_dropped;
            ++stats_.rate_limit_drops;
        }
        THEMIS_WARN("[UDPServer] rate-limited source {}", anonymizeIpForLog(ip));

        // Send RATE_LIMITED ACK if we can read a seq_num
        if (data.size() >= kUdpServerHeaderSize) {
            uint32_t seq_be = 0;
            std::memcpy(&seq_be, data.data() + 4, 4);
            sendAck(sender, ntohl(seq_be), UdpServerStatus::RATE_LIMITED);
        }
        return;
    }

    // ── Packet validation ─────────────────────────────────────────────────
    if (!validatePacket(data)) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.packets_dropped;
            ++stats_.parse_errors;
        }
        THEMIS_WARN("[UDPServer] malformed packet from {}", anonymizeIpForLog(ip));
        return;
    }

    // ── Parse header ──────────────────────────────────────────────────────
    const uint8_t opcode = data[3];

    uint32_t seq_be = {};
    std::memcpy(&seq_be, data.data() + 4, 4);
    const uint32_t seq_num = ntohl(seq_be);

    const uint8_t flags = data[8];

    uint16_t plen_be;
    std::memcpy(&plen_be, data.data() + 9, 2);
    const uint16_t payload_len = ntohs(plen_be);

    // ── Duplicate detection ───────────────────────────────────────────────
    if (checkDuplicate(ip, seq_num)) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.packets_dropped;
            ++stats_.duplicate_drops;
        }
        THEMIS_DEBUG("[UDPServer] duplicate seq={} from {}",
                     seq_num, anonymizeIpForLog(ip));

        // Inform the client (if ACK was requested) that this is a duplicate
        if ((flags & kUdpServerFlagAckRequested) || config_.enable_acks) {
            sendAck(sender, seq_num, UdpServerStatus::DUPLICATE);
        }
        return;
    }

    // ── Build UdpPacket ───────────────────────────────────────────────────
    UdpPacket pkt;
    pkt.opcode      = static_cast<UdpServerOpCode>(opcode);
    pkt.seq_num     = seq_num;
    pkt.flags       = flags;
    pkt.source_ip   = ip;
    pkt.source_port = port;
    if (payload_len > 0) {
        const uint8_t* payload_start = data.data() + kUdpServerHeaderSize;
        pkt.payload.assign(payload_start, payload_start + payload_len);
    }

    // ── Send ACK (if requested) ───────────────────────────────────────────
    if ((flags & kUdpServerFlagAckRequested) || config_.enable_acks) {
        sendAck(sender, seq_num, UdpServerStatus::OK);
    }

    // ── Dispatch ──────────────────────────────────────────────────────────
    // PING is handled inline; data-bearing opcodes go to batching or direct.
    if (pkt.opcode == UdpServerOpCode::PING) {
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            ++stats_.pings_answered;
        }
        // ACK already sent above; no further processing needed.
        return;
    }

    dispatchPacket(pkt);
}

// ─────────────────────────────────────────────────────────────────────────────
// Rate limiting
// ─────────────────────────────────────────────────────────────────────────────

bool UDPServer::checkRateLimit(const std::string& ip) {
    const uint64_t now_ms =
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
            .count());

    std::lock_guard<std::mutex> lk(rate_mutex_);
    auto& entry = rate_limits_[ip];

    if (now_ms - entry.window_start_ms >= 1000) {
        entry.window_start_ms = now_ms;
        entry.count           = 1;
        return true;
    }

    ++entry.count;
    // Guard against uint32_t wrap-around after ~4 billion packets/window
    if (entry.count == 0) {
        entry.count = std::numeric_limits<uint32_t>::max();
    }
    return entry.count <= config_.max_packets_per_second_per_ip;
}

// ─────────────────────────────────────────────────────────────────────────────
// Duplicate detection (sliding window per source IP)
// ─────────────────────────────────────────────────────────────────────────────

bool UDPServer::checkDuplicate(const std::string& ip, uint32_t seq_num) {
    std::lock_guard<std::mutex> lk(dedup_mutex_);
    auto& entry = dedup_state_[ip];

    if (entry.seq_set.count(seq_num)) {
        return true;  // already seen
    }

    // Add to window
    entry.seq_queue.push_back(seq_num);
    entry.seq_set.insert(seq_num);

    // Evict oldest when window is full
    if (entry.seq_queue.size() > config_.dedup_window_size) {
        const uint32_t oldest = entry.seq_queue.front();
        entry.seq_queue.pop_front();
        entry.seq_set.erase(oldest);
    }

    return false;  // not a duplicate
}

// ─────────────────────────────────────────────────────────────────────────────
// ACK dispatch
// ─────────────────────────────────────────────────────────────────────────────

void UDPServer::sendAck(const udp::endpoint& dest,
                         uint32_t             seq_num,
                         UdpServerStatus      status) {
    auto ack = buildAck(seq_num, status);
    boost::system::error_code ec;
    {
        std::lock_guard<std::mutex> lock(socket_mutex_);
        if (!socket_ || !socket_->is_open()) {
            return;
        }
        socket_->send_to(net::buffer(ack), dest, 0, ec);
    }
    if (!ec) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        ++stats_.acks_sent;
    } else {
        THEMIS_ERROR("[UDPServer] ACK send error to {}: {}",
                     anonymizeIpForLog(dest.address().to_string()), ec.message());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Packet dispatch (batching or direct)
// ─────────────────────────────────────────────────────────────────────────────

void UDPServer::dispatchPacket(const UdpPacket& pkt) {
    if (config_.enable_batching) {
        {
            std::lock_guard<std::mutex> lk(batch_mutex_);
            batch_.push_back(pkt);
        }
        // Do not notify the flush loop; it wakes on a timer instead so that
        // small bursts are coalesced into a single handler invocation.
        return;
    }

    // Immediate dispatch
    {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        switch (pkt.opcode) {
            case UdpServerOpCode::METRIC: ++stats_.metrics_ingested; break;
            case UdpServerOpCode::LOG:    ++stats_.logs_ingested;    break;
            case UdpServerOpCode::EVENT:  ++stats_.events_ingested;  break;
            case UdpServerOpCode::BATCH:  ++stats_.batches_ingested; break;
            default: break;
        }
    }

    if ([[maybe_unused]] handler_) {
        try {
            handler_([[maybe_unused]] pkt);
        } catch (const std::exception& ex) {
            THEMIS_ERROR("[UDPServer] handler exception: {}", ex.what());
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Batch-flush loop
// ─────────────────────────────────────────────────────────────────────────────

void UDPServer::batchFlushLoop() {
    const auto interval =
        std::chrono::milliseconds(std::max<uint32_t>(1, config_.batch_interval_ms));

    while (true) {
        // Wait for the batch interval or a stop signal
        {
            std::unique_lock<std::mutex> lk(batch_mutex_);
            batch_cv_.wait_for(lk, interval, [this] { return batch_stop_; });
            if (batch_stop_) {
                // Flush any remaining packets before exiting
                break;
            }
        }

        // Swap-out the batch under the lock so I/O threads can keep producing
        std::vector<UdpPacket> local;
        {
            std::lock_guard<std::mutex> lk(batch_mutex_);
            local.swap(batch_);
        }

        if (local.empty()) {
            continue;
        }

        // Update statistics
        {
            std::lock_guard<std::mutex> lk(stats_mutex_);
            for (const auto& pkt : local) {
                switch (pkt.opcode) {
                    case UdpServerOpCode::METRIC: ++stats_.metrics_ingested; break;
                    case UdpServerOpCode::LOG:    ++stats_.logs_ingested;    break;
                    case UdpServerOpCode::EVENT:  ++stats_.events_ingested;  break;
                    case UdpServerOpCode::BATCH:  ++stats_.batches_ingested; break;
                    default: break;
                }
            }
        }

        // Deliver to application handler
        if ([[maybe_unused]] handler_) {
            for (const auto& pkt : local) {
                try {
                    handler_([[maybe_unused]] pkt);
                } catch (const std::exception& ex) {
                    THEMIS_ERROR([[maybe_unused]] "[UDPServer] handler exception (batch): {}", ex.what());
                }
            }
        }
    }

    // Final drain after stop signal
    std::vector<UdpPacket> remaining;
    {
        std::lock_guard<std::mutex> lk(batch_mutex_);
        remaining.swap(batch_);
    }

    if (!remaining.empty()) {
        std::lock_guard<std::mutex> lk(stats_mutex_);
        for (const auto& pkt : remaining) {
            switch (pkt.opcode) {
                case UdpServerOpCode::METRIC: ++stats_.metrics_ingested; break;
                case UdpServerOpCode::LOG:    ++stats_.logs_ingested;    break;
                case UdpServerOpCode::EVENT:  ++stats_.events_ingested;  break;
                case UdpServerOpCode::BATCH:  ++stats_.batches_ingested; break;
                default: break;
            }
        }
    }

    if ([[maybe_unused]] handler_) {
        for (const auto& pkt : remaining) {
            try {
                handler_([[maybe_unused]] pkt);
            } catch (const std::exception& ex) {
                THEMIS_ERROR([[maybe_unused]] "[UDPServer] handler exception (final drain): {}", ex.what());
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Static packet helpers
// ─────────────────────────────────────────────────────────────────────────────

bool UDPServer::validatePacket(const std::vector<uint8_t>& data) {
    if (data.size() < kUdpServerHeaderSize) {
        return false;
    }
    if (data[0] != kUdpServerMagic0 || data[1] != kUdpServerMagic1) {
        return false;
    }
    if (data[2] != kUdpServerVersion) {
        return false;
    }
    // Payload-length consistency check
    uint16_t plen_be;
    std::memcpy(&plen_be, data.data() + 9, 2);
    const uint16_t payload_len = ntohs(plen_be);
    if (data.size() < kUdpServerHeaderSize + payload_len) {
        return false;
    }
    return true;
}

std::vector<uint8_t> UDPServer::buildAck(uint32_t        seq_num,
                                          UdpServerStatus status) {
    std::vector<uint8_t> ack;
    ack.reserve(kUdpServerAckSize);

    ack.push_back(kUdpServerAckMagic0);
    ack.push_back(kUdpServerAckMagic1);
    ack.push_back(kUdpServerVersion);
    ack.push_back(static_cast<uint8_t>(status));

    const uint32_t seq_be = htonl(seq_num);
    ack.insert(ack.end(),
               reinterpret_cast<const uint8_t*>(&seq_be),
               reinterpret_cast<const uint8_t*>(&seq_be) + 4);

    return ack;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

UDPServer::Stats UDPServer::getStats() const {
    std::lock_guard<std::mutex> lk(stats_mutex_);
    return stats_;
}

}  // namespace themis::network
