/**
 * @file wire_protocol_v2.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// ThemisDB Wire Protocol V2 Implementation
// Multiplexed binary protocol with server push and flow control

#include "themis/network/wire_protocol_v2.hpp"

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "network/connection_compression.h"
#include "utils/logger.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace themis {
namespace wire {

namespace net = boost::asio;
using tcp     = net::ip::tcp;

// =============================================================================
// Internal helpers
// =============================================================================

namespace {

constexpr int kShutdownJoinTimeoutMs = 5000;

/// @brief Join @p t within @p timeout_ms; log and detach on timeout.
static void timedJoin(std::thread &t, int timeout_ms = kShutdownJoinTimeoutMs) noexcept {
    if (!t.joinable())
        return;
    std::promise<void> done;
    auto fut = done.get_future();
    std::thread watcher([inner = std::move(t), p = std::move(done)]() mutable {
        if (inner.joinable())
            inner.join();
        p.set_value();
    });
    watcher.detach();
    if (fut.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status::ready) {
        // thread_join_no_timeout: detach on deadline to avoid indefinite block
        THEMIS_WARN("Thread did not finish within {} ms during shutdown; detaching.", timeout_ms);
    }
}

} // namespace

static uint32_t htonl32([[maybe_unused]] uint32_t v) {
    return htonl(v);
}
static uint32_t ntohl32([[maybe_unused]] uint32_t v) {
    return ntohl(v);
}
static uint16_t htons16([[maybe_unused]] uint16_t v) {
    return htons(v);
}
static uint16_t ntohs16([[maybe_unused]] uint16_t v) {
    return ntohs(v);
}

// Bring compression utilities into this translation unit as thin wrappers.
static std::vector<uint8_t> compressLZ4(const std::vector<uint8_t> &data, uint32_t min_size) {
    return themis::network::compressLZ4(data, min_size);
}

static std::vector<uint8_t> compressZstd(const std::vector<uint8_t> &data, uint32_t min_size, int level) {
    return themis::network::compressZstd(data, min_size, level);
}

static std::vector<uint8_t> decompressLZ4(const std::vector<uint8_t> &data) {
    return themis::network::decompressLZ4(data);
}

static std::vector<uint8_t> decompressZstd(const std::vector<uint8_t> &data) {
    return themis::network::decompressZstd(data);
}

// Serialise a V2FrameHeader to wire bytes (big-endian fields where required)
static std::array<uint8_t, V2_HEADER_SIZE> serializeHeader(const V2FrameHeader &h) {
    std::array<uint8_t, V2_HEADER_SIZE> buf{};
    uint32_t magic_be = htonl32(h.magic);
    uint32_t sid_be   = htonl32(h.stream_id);
    uint32_t len_be   = htonl32(h.payload_length);
    uint16_t flags_be = htons16(h.flags);

    std::memcpy(buf.data() + 0, &magic_be, 4);
    buf[4] = h.version;
    buf[5] = h.frame_type;
    std::memcpy(buf.data() + 6, &flags_be, 2);
    std::memcpy(buf.data() + 8, &sid_be, 4);
    std::memcpy(buf.data() + 12, &len_be, 4);
    return buf;
}

// Deserialise a V2FrameHeader from a 16-byte buffer
static V2FrameHeader parseHeader(const uint8_t *buf) {
    V2FrameHeader h{};
    uint32_t magic_be, sid_be, len_be;
    uint16_t flags_be;

    std::memcpy(&magic_be, buf + 0, 4);
    h.version    = buf[4];
    h.frame_type = buf[5];
    std::memcpy(&flags_be, buf + 6, 2);
    std::memcpy(&sid_be, buf + 8, 4);
    std::memcpy(&len_be, buf + 12, 4);

    h.magic          = ntohl32(magic_be);
    h.flags          = ntohs16(flags_be);
    h.stream_id      = ntohl32(sid_be);
    h.payload_length = ntohl32(len_be);
    return h;
}

// =============================================================================
// V2SessionImpl – concrete per-connection implementation
// =============================================================================

/** @brief V2SessionImpl – concrete per-connection implementation. */
class V2SessionImpl : public V2Session {
  public:
    V2SessionImpl(tcp::socket socket, const V2ConnectionConfig &cfg, V2DataHandler data_handler,
                  V2HeadersHandler headers_handler, V2RstStreamHandler rst_handler)
        : socket_(std::move(socket)), cfg_(cfg), data_handler_(std::move(data_handler)),
          headers_handler_([[maybe_unused]] std::move(headers_handler)), rst_handler_(std::move(rst_handler)) {
        // Generate a simple connection ID
        static std::atomic<uint64_t> counter{1};
        std::ostringstream oss = {};
        oss << "v2conn-" << counter.fetch_add(1, std::memory_order_relaxed);
        connection_id_ = oss.str();
    }

    const std::string &connection_id() const override {
        return connection_id_;
    }

    void start() {
        asyncReadHeader();
    }

    void set_disconnect_handler(std::function<void(const std::string &)> cb) {
        on_disconnect_ = std::move(cb);
    }

    void send_data(uint32_[[maybe_unused]] t stream_i[[maybe_unused]] d, cons[[maybe_unused]] t st[[maybe_unused]] d::vecto[[maybe_unused]] r<uint8_[[maybe_unused]] t> &dat[[maybe_unused]] a, boo[[maybe_unused]] l end_strea[[maybe_unused]] m) override {
        // Attempt connection-level compression (Zstd preferred over LZ4).
        // Only use the compressed result when it is actually smaller than the
        // original (compression is not beneficial for high-entropy data).
        const std::vector<uint8_t> *payload = &data;
        std::vector<uint8_t> compressed_buf;

        if (cfg_.enable_zstd_compression) {
            compressed_buf = compressZstd(data, cfg_.min_compression_payload_size, cfg_.zstd_compression_level);
        } else if (cfg_.enable_lz4_compression) {
            compressed_buf = compressLZ4(data, cfg_.min_compression_payload_size);
        }

        V2FrameHeader hdr{};
        hdr.magic      = WIRE_V2_MAGIC;
        hdr.version    = WIRE_VERSION_2;
        hdr.frame_type = static_cast<uint8_t>(V2FrameType::DATA);
        hdr.stream_id  = stream_id;
        if (end_stream)
            hdr.flags |= static_cast<uint16_t>(V2FrameFlags::END_STREAM);

        // Use compressed payload only when it actually saves bytes
        if (!compressed_buf.empty() && compressed_buf.size() < data.size()) {
            payload = &compressed_buf;
            if (cfg_.enable_zstd_compression) {
                hdr.flags |= static_cast<uint16_t>(V2FrameFlags::ZSTD_COMPRESSED);
            } else {
                hdr.flags |= static_cast<uint16_t>(V2FrameFlags::COMPRESSED);
            }
        }

        hdr.payload_length = static_cast<uint32_t>(payload->size());

        auto hdr_bytes = serializeHeader(hdr);
        auto frame     = std::make_shared<std::vector<uint8_t>>();
        frame->insert(frame->end(), hdr_bytes.begin(), hdr_bytes.end());
        frame->insert(frame->end(), payload->begin(), payload->end());

        std::lock_guard<std::mutex> lock(write_mutex_);
        net::async_write(socket_, net::buffer(*frame), [this, frame](const boost::system::error_code &ec, size_t n) {
            if (!ec) {
                frames_sent_.fetch_add(1, std::memory_order_relaxed);
                bytes_sent_.fetch_add(n, std::memory_order_relaxed);
            }
        });
    }

    uint32_t push_promise(uint32_t associated_stream_id,
                          const std::unordered_map<std::string, std::string> &headers) override {
        // Allocate a server-initiated (even) stream ID
        uint32_t new_sid = next_server_stream_id_.fetch_add(2, std::memory_order_relaxed);

        V2FrameHeader hdr{};
        hdr.magic      = WIRE_V2_MAGIC;
        hdr.version    = WIRE_VERSION_2;
        hdr.frame_type = static_cast<uint8_t>(V2FrameType::PUSH_PROMISE);
        hdr.stream_id  = associated_stream_id;
        hdr.flags      = static_cast<uint16_t>(V2FrameFlags::END_HEADERS);

        // Minimal header encoding: "key: value\n" pairs
        std::string encoded = {};
        encoded += ":push-stream-id: " + std::to_string(new_sid) + "\n";
        for (const auto &[k, v] : headers)
            encoded += k + ": " + v + "\n";

        hdr.payload_length = static_cast<uint32_t>(encoded.size());
        auto hdr_bytes     = serializeHeader(hdr);
        std::vector<uint8_t> raw;
        raw.insert(raw.end(), hdr_bytes.begin(), hdr_bytes.end());
        raw.insert(raw.end(), encoded.begin(), encoded.end());

        sendFrame(std::move(raw));
        return new_sid;
    }

    void reset_stream(uint32_t stream_id, uint32_t error_code) override {
        V2FrameHeader hdr{};
        hdr.magic          = WIRE_V2_MAGIC;
        hdr.version        = WIRE_VERSION_2;
        hdr.frame_type     = static_cast<uint8_t>(V2FrameType::RST_STREAM);
        hdr.stream_id      = stream_id;
        hdr.payload_length = 4; // error code only

        auto hdr_bytes = serializeHeader(hdr);
        uint32_t ec_be = htonl32(error_code);
        std::vector<uint8_t> frame;
        frame.insert(frame.end(), hdr_bytes.begin(), hdr_bytes.end());
        const uint8_t *ec_ptr = reinterpret_cast<const uint8_t *>(&ec_be);
        frame.insert(frame.end(), ec_ptr, ec_ptr + 4);

        sendFrame(std::move(frame));

        std::lock_guard<std::mutex> sl(streams_mutex_);
        auto it = streams_.find(stream_id);
        if (it != streams_.end())
            it->second.state = V2StreamState::CLOSED;
    }

    void go_away(uint32_t last_stream_id, uint32_t error_code) override {
        V2FrameHeader hdr{};
        hdr.magic          = WIRE_V2_MAGIC;
        hdr.version        = WIRE_VERSION_2;
        hdr.frame_type     = static_cast<uint8_t>(V2FrameType::GOAWAY);
        hdr.stream_id      = 0;
        hdr.payload_length = 8; // last_stream_id + error_code

        auto hdr_bytes   = serializeHeader(hdr);
        uint32_t lsid_be = htonl32(last_stream_id);
        uint32_t ec_be   = htonl32(error_code);
        auto frame       = std::make_shared<std::vector<uint8_t>>();
        frame->insert(frame->end(), hdr_bytes.begin(), hdr_bytes.end());
        const uint8_t *p = reinterpret_cast<const uint8_t *>(&lsid_be);
        frame->insert(frame->end(), p, p + 4);
        p = reinterpret_cast<const uint8_t *>(&ec_be);
        frame->insert(frame->end(), p, p + 4);

        std::lock_guard<std::mutex> lock(write_mutex_);
        net::async_write(socket_, net::buffer(*frame),
                         [this, frame](const boost::system::error_code &, size_t) { socket_.close(); });
    }

    size_t open_stream_count() const override {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        size_t count = 0;
        for (const auto &[id, s] : streams_)
            if (s.is_open())
                ++count;
        return count;
    }

    int32_t send_window(uint32_[[maybe_unused]] t stream_i[[maybe_unused]] d) const override {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        auto it = streams_.find(stream_id);
        return it != streams_.end() ? it->second.send_window : 0;
    }

    void update_connection_window(uint32_[[maybe_unused]] t incremen[[maybe_unused]] t) override {
        connection_send_window_.fetch_add(static_cast<int32_t>(increment), std::memory_order_relaxed);
    }

    void set_stream_priority(uint32_t stream_id, uint32_t dependency, uint8_t weight, bool exclusive) override {
        // RFC 7540 §6.3: PRIORITY on stream 0 is a connection error.
        if (stream_id == 0)
            return;
        // RFC 7540 §5.3.1: A stream cannot depend on itself.
        if ((dependency & 0x7FFFFFFFu) == stream_id)
            return;

        // Build the 5-byte PRIORITY frame payload (RFC 7540 §6.3):
        //   E (1 bit) | Stream Dependency (31 bits) | Weight (8 bits)
        uint32_t dep_field = dependency & 0x7FFFFFFFu;
        if (exclusive)
            dep_field |= 0x80000000u;
        uint32_t dep_be = htonl32(dep_field);

        V2FrameHeader hdr{};
        hdr.magic          = WIRE_V2_MAGIC;
        hdr.version        = WIRE_VERSION_2;
        hdr.frame_type     = static_cast<uint8_t>(V2FrameType::PRIORITY);
        hdr.stream_id      = stream_id;
        hdr.payload_length = 5;

        auto hdr_bytes = serializeHeader(hdr);
        std::vector<uint8_t> frame;
        frame.insert(frame.end(), hdr_bytes.begin(), hdr_bytes.end());
        const uint8_t *p = reinterpret_cast<const uint8_t *>(&dep_be);
        frame.insert(frame.end(), p, p + 4);
        frame.push_back(weight);
        sendFrame(std::move(frame));

        // Also update local stream metadata so callers can inspect priority.
        std::lock_guard<std::mutex> lock(streams_mutex_);
        auto &s = streams_[stream_id];
        if (s.state == V2StreamState::IDLE) {
            s.stream_id = stream_id;
            s.state     = V2StreamState::OPEN;
        }
        s.priority             = weight;
        s.stream_dependency    = dependency & 0x7FFFFFFFu;
        s.exclusive_dependency = exclusive;
    }

    uint64_t frames_received() const override {
        return frames_received_.load(std::memory_order_relaxed);
    }
    uint64_t frames_sent() const override {
        return frames_sent_.load(std::memory_order_relaxed);
    }
    uint64_t bytes_received() const override {
        return bytes_received_.load(std::memory_order_relaxed);
    }
    uint64_t bytes_sent() const override {
        return bytes_sent_.load(std::memory_order_relaxed);
    }

  private:
    // ── Async read pipeline ────────────────────────────────────────────────

    void asyncReadHeader() {
        auto self = shared_from_this();
        auto buf  = std::make_shared<std::array<uint8_t, V2_HEADER_SIZE>>();
        net::async_read(socket_, net::buffer(*buf), [this, self, buf](const boost::system::error_code &ec, size_t) {
            if (ec) {
                if (on_disconnect_)
                    on_disconnect_(connection_id_);
                return;
            }
            V2FrameHeader hdr = parseHeader(buf->data());
            if (!hdr.is_valid()) {
                // Bad magic/version – send GOAWAY and disconnect
                go_away(0, 1 /* PROTOCOL_ERROR */);
                return;
            }
            asyncReadPayload(hdr);
        });
    }

    void asyncReadPayload(const V2FrameHeader &hdr) {
        if (hdr.payload_length > V2_MAX_PAYLOAD) {
            go_away(hdr.stream_id, 6 /* FRAME_SIZE_ERROR */);
            return;
        }
        auto self    = shared_from_this();
        auto payload = std::make_shared<std::vector<uint8_t>>(hdr.payload_length);
        net::async_read(socket_, net::buffer(*payload),
                        [this, self, hdr, payload](const boost::system::error_code &ec, size_t n) {
                            if (ec) {
                                if (on_disconnect_)
                                    on_disconnect_(connection_id_);
                                return;
                            }
                            frames_received_.fetch_add(1, std::memory_order_relaxed);
                            bytes_received_.fetch_add(V2_HEADER_SIZE + n, std::memory_order_relaxed);

                            handleFrame(hdr, *payload);
                            asyncReadHeader(); // read next frame
                        });
    }

    void handleFrame(const V2FrameHeader &hdr, const std::vector<uint8_t> &payload) {
        switch (hdr.get_type()) {
            case V2FrameType::DATA: {
                ensureStreamOpen(hdr.stream_id);
                bool eos = hdr.has_flag(V2FrameFlags::END_STREAM);

                const std::vector<uint8_t> *effective_payload = &payload;
                std::vector<uint8_t> decompressed_buf;

                if (hdr.has_flag(V2FrameFlags::ZSTD_COMPRESSED)) {
                    decompressed_buf = decompressZstd(payload);
                    if (decompressed_buf.empty()) {
                        reset_stream(hdr.stream_id, 2 /* INTERNAL_ERROR */);
                        break;
                    }
                    effective_payload = &decompressed_buf;
                } else if (hdr.has_flag(V2FrameFlags::COMPRESSED)) {
                    decompressed_buf = decompressLZ4(payload);
                    if (decompressed_buf.empty()) {
                        reset_stream(hdr.stream_id, 2 /* INTERNAL_ERROR */);
                        break;
                    }
                    effective_payload = &decompressed_buf;
                }

                if (data_handler_)
                    data_handler_(hdr.stream_id, *effective_payload, eos);
                if (eos)
                    closeStream(hdr.stream_id, true /*remote*/);

                // Flow control: send WINDOW_UPDATE to replenish remote window
                if (cfg_.enable_flow_control && !payload.empty())
                    sendWindowUpdate(hdr.stream_id, static_cast<uint32_t>(payload.size()));
                break;
            }
            case V2FrameType::HEADERS: {
                ensureStreamOpen(hdr.stream_id);
                bool eos     = hdr.has_flag(V2FrameFlags::END_STREAM);
                auto headers = decodeHeaders(payload);
                if (headers_handler_)
                    headers_handler_(hdr.stream_id, headers, eos);
                if (eos)
                    closeStream(hdr.stream_id, true);
                break;
            }
            case V2FrameType::RST_STREAM: {
                uint32_t ec = 0;
                if (payload.size() >= 4)
                    std::memcpy(&ec, payload.data(), 4), ec = ntohl32(ec);
                if (rst_handler_)
                    rst_handler_(hdr.stream_id, ec);
                closeStream(hdr.stream_id, true);
                break;
            }
            case V2FrameType::SETTINGS: {
                // Acknowledge settings
                V2FrameHeader ack{};
                ack.magic          = WIRE_V2_MAGIC;
                ack.version        = WIRE_VERSION_2;
                ack.frame_type     = static_cast<uint8_t>(V2FrameType::SETTINGS);
                ack.flags          = static_cast<uint16_t>(V2FrameFlags::ACK);
                ack.stream_id      = 0;
                ack.payload_length = 0;
                auto ack_bytes     = std::make_shared<std::array<uint8_t, V2_HEADER_SIZE>>(serializeHeader(ack));
                std::lock_guard<std::mutex> lock(write_mutex_);
                net::async_write(socket_, net::buffer(*ack_bytes),
                                 [ack_bytes](const boost::system::error_code &, size_t) {});
                break;
            }
            case V2FrameType::WINDOW_UPDATE: {
                if (payload.size() >= 4) {
                    uint32_t inc = 0;
                    std::memcpy(&inc, payload.data(), 4);
                    inc = ntohl32(inc) & 0x7FFFFFFF; // strip reserved bit
                    if (hdr.stream_id == 0) {
                        connection_send_window_.fetch_add(static_cast<int32_t>(inc), std::memory_order_relaxed);
                    } else {
                        std::lock_guard<std::mutex> lock(streams_mutex_);
                        auto it = streams_.find(hdr.stream_id);
                        if (it != streams_.end())
                            it->second.send_window += static_cast<int32_t>(inc);
                    }
                }
                break;
            }
            case V2FrameType::PING: {
                // Echo back with ACK flag
                V2FrameHeader pong = hdr;
                pong.flags |= static_cast<uint16_t>(V2FrameFlags::ACK);
                auto hdr_bytes = serializeHeader(pong);
                std::vector<uint8_t> frame;
                frame.insert(frame.end(), hdr_bytes.begin(), hdr_bytes.end());
                frame.insert(frame.end(), payload.begin(), payload.end());
                sendFrame(std::move(frame));
                break;
            }
            case V2FrameType::PRIORITY: {
                // RFC 7540 §6.3: PRIORITY on stream 0 is a connection error.
                if (hdr.stream_id == 0) {
                    go_away(0, 1 /* PROTOCOL_ERROR */);
                    break;
                }
                // PRIORITY frame payload (RFC 7540 §6.3): 5 bytes
                //   E (1 bit) | Stream Dependency (31 bits) | Weight (8 bits)
                if (payload.size() < 5)
                    break;
                uint32_t dep_field = 0;
                std::memcpy(&dep_field, payload.data(), 4);
                dep_field       = ntohl32(dep_field);
                bool exclusive  = (dep_field & 0x80000000u) != 0;
                uint32_t dep_id = dep_field & 0x7FFFFFFFu;
                uint8_t weight  = payload[4];
                // RFC 7540 §5.3.1: A stream cannot depend on itself.
                if (dep_id == hdr.stream_id) {
                    reset_stream(hdr.stream_id, 1 /* PROTOCOL_ERROR */);
                    break;
                }
                std::lock_guard<std::mutex> lock(streams_mutex_);
                auto &s = streams_[hdr.stream_id];
                if (s.state == V2StreamState::IDLE) {
                    s.stream_id = hdr.stream_id;
                    s.state     = V2StreamState::OPEN;
                }
                s.priority             = weight;
                s.stream_dependency    = dep_id;
                s.exclusive_dependency = exclusive;
                break;
            }
            default:
                break; // Unknown frame types are ignored per spec
        }
    }

    // ── Helpers ───────────────────────────────────────────────────────────

    void ensureStreamOpen([[maybe_unused]] uint32_t stream_id) {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        auto &s = streams_[stream_id];
        if (s.state == V2StreamState::IDLE) {
            s.stream_id   = stream_id;
            s.state       = V2StreamState::OPEN;
            s.send_window = static_cast<int32_t>(cfg_.initial_window_size);
            s.recv_window = static_cast<int32_t>(cfg_.initial_window_size);
        }
    }

    void closeStream(uint32_t stream_id, bool remote_closed) {
        std::lock_guard<std::mutex> lock(streams_mutex_);
        auto it = streams_.find(stream_id);
        if (it == streams_.end())
            return;

        V2Stream &s = it->second;
        if (remote_closed) {
            if (s.state == V2StreamState::OPEN)
                s.state = V2StreamState::HALF_CLOSED_REMOTE;
            else if (s.state == V2StreamState::HALF_CLOSED_LOCAL)
                s.state = V2StreamState::CLOSED;
        } else {
            if (s.state == V2StreamState::OPEN)
                s.state = V2StreamState::HALF_CLOSED_LOCAL;
            else if (s.state == V2StreamState::HALF_CLOSED_REMOTE)
                s.state = V2StreamState::CLOSED;
        }
    }

    void sendWindowUpdate(uint32_t stream_id, uint32_t increment) {
        V2FrameHeader hdr{};
        hdr.magic          = WIRE_V2_MAGIC;
        hdr.version        = WIRE_VERSION_2;
        hdr.frame_type     = static_cast<uint8_t>(V2FrameType::WINDOW_UPDATE);
        hdr.stream_id      = stream_id;
        hdr.payload_length = 4;

        uint32_t inc_be = htonl32(increment);
        auto hdr_bytes  = serializeHeader(hdr);
        std::vector<uint8_t> frame;
        frame.insert(frame.end(), hdr_bytes.begin(), hdr_bytes.end());
        const uint8_t *p = reinterpret_cast<const uint8_t *>(&inc_be);
        frame.insert(frame.end(), p, p + 4);

        sendFrame(std::move(frame));
    }

    // Decode a minimal "key: value\n" header block
    // Each key and value is capped at 8 KiB to prevent resource exhaustion.
    static constexpr size_t MAX_HEADER_FIELD_SIZE = 8 * 1024;

    static std::unordered_map<std::string, std::string> decodeHeaders(const std::vector<uint8_t> &payload) {
        std::unordered_map<std::string, std::string> headers = {};

        std::string text(reinterpret_cast<const char *>(payload.data()), payload.size());
        std::istringstream ss(text);
        std::string line = {};
        while (std::getline(ss, line)) {
            // Guard against excessively long lines
            if (line.size() > MAX_HEADER_FIELD_SIZE)
                continue;
            auto pos = line.find(':');
            if (pos == std::string::npos)
                continue;
            std::string key   = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            // Trim leading space from value
            if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);
            headers[std::move(key)] = std::move(value);
        }
        return headers;
    }

    // ── Write helper ──────────────────────────────────────────────────────

    // Keeps `frame` alive on the heap for the duration of the async write.
    void sendFrame(std::vector<uint8_t> frame) {
        auto buf = std::make_shared<std::vector<uint8_t>>(std::move(frame));
        std::lock_guard<std::mutex> lock(write_mutex_);
        net::async_write(socket_, net::buffer(*buf), [buf](const boost::system::error_code &, size_t) {});
    }

    // ── Member variables ──────────────────────────────────────────────────
    tcp::socket socket_;
    V2ConnectionConfig cfg_;
    std::string connection_id_;

    V2DataHandler data_handler_;
    V2HeadersHandler headers_handler_;
    V2RstStreamHandler rst_handler_;
    std::function<void(const std::string &)> on_disconnect_;

    mutable std::mutex streams_mutex_;
    std::unordered_map<uint32_t, V2Stream> streams_;

    std::mutex write_mutex_;

    std::atomic<uint32_t> next_server_stream_id_{2}; // even IDs for server
    std::atomic<int32_t> connection_send_window_{static_cast<int32_t>(V2_DEFAULT_WINDOW)};

    std::atomic<uint64_t> frames_received_{0};
    std::atomic<uint64_t> frames_sent_{0};
    std::atomic<uint64_t> bytes_received_{0};
    std::atomic<uint64_t> bytes_sent_{0};
};

// =============================================================================
// V2Server::Impl
// =============================================================================

/** @brief V2Server::Impl. */
class V2Server::Impl {
  public:
    explicit Impl(const V2ConnectionConfig &cfg) : cfg_(cfg), io_context_(), acceptor_(io_context_) {}

    void start() {
        if (running_.exchange(true))
            return; // already running

        tcp::endpoint ep(tcp::v4(), cfg_.port);
        acceptor_.open(ep.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();

        asyncAccept();

        for (size_t i = 0; i < cfg_.num_io_threads; ++i) {
            io_threads_.emplace_back([this]() {
                try {
                    io_context_.run();
                } catch (const std::exception &e) {
                    std::cerr << "[WireV2] I/O thread error: " << e.what() << '\n';
                }
            });
        }
    }

    void stop() {
        if (!running_.exchange(false))
            return;
        io_context_.stop();
        for (auto &t : io_threads_)
            timedJoin(t);
        io_threads_.clear();
        acceptor_.close();
    }

    bool is_running() const {
        return running_.load();
    }

    void set_data_handler([[maybe_unused]] V2DataHandler h) {
        data_handler_ = std::move([[maybe_unused]] h);
    }
    void set_headers_handler([[maybe_unused]] V2HeadersHandler h) {
        headers_handler_ = std::move([[maybe_unused]] h);
    }
    void set_rst_stream_handler([[maybe_unused]] V2RstStreamHandler h) {
        rst_handler_ = std::move([[maybe_unused]] h);
    }

    bool push_to_client(const std::string &conn_id, uint32_t associated_sid,
                        const std::unordered_map<std::string, std::string> &headers, const std::vector<uint8_t> &data) {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = sessions_.find(conn_id);
        if (it == sessions_.end())
            return false;

        uint32_t push_sid = it->second->push_promise(associated_sid, headers);
        it->second->send_data(push_sid, data, true /*end_stream*/);
        return true;
    }

    size_t active_connections() const {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        return sessions_.size();
    }

    uint64_t total_streams_opened() const {
        return connections_accepted_.load();
    }

    uint64_t total_frames_sent() const {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        uint64_t total = 0;
        for (const auto &[id, s] : sessions_)
            total += s->frames_sent();
        return total;
    }

    uint64_t total_frames_received() const {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        uint64_t total = 0;
        for (const auto &[id, s] : sessions_)
            total += s->frames_received();
        return total;
    }

  private:
    void asyncAccept() {
        acceptor_.async_accept([this](const boost::system::error_code &ec, tcp::socket socket) {
            if (!ec && running_.load()) {
                auto session = std::make_shared<V2SessionImpl>(std::move(socket), cfg_, data_handler_, headers_handler_,
                                                               rst_handler_);

                session->set_disconnect_handler([[maybe_unused]] [this](const std::string &id) {
                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                    sessions_.erase(id);
                });

                {
                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                    sessions_[session->connection_id()] = session;
                }

                streams_opened_.fetch_add(1, std::memory_order_relaxed);
                connections_accepted_.fetch_add(1, std::memory_order_relaxed);
                session->start();
            }
            if (running_.load())
                asyncAccept();
        });
    }

    V2ConnectionConfig cfg_;
    net::io_context io_context_;
    tcp::acceptor acceptor_;
    std::vector<std::thread> io_threads_;
    std::atomic<bool> running_{false};

    V2DataHandler data_handler_;
    V2HeadersHandler headers_handler_;
    V2RstStreamHandler rst_handler_;

    mutable std::mutex sessions_mutex_;
    std::unordered_map<std::string, std::shared_ptr<V2SessionImpl>> sessions_;

    std::atomic<uint64_t> connections_accepted_{0};
    std::atomic<uint64_t> streams_opened_{0};
};

// =============================================================================
// V2Server public API – thin delegation to Impl
// =============================================================================

V2Server::V2Server(const V2ConnectionConfig &config) : impl_(std::make_unique<Impl>(config)) {}

V2Server::~V2Server() {
    impl_->stop();
}

void V2Server::start() {
    impl_->start();
}
void V2Server::stop() {
    impl_->stop();
}
bool V2Server::is_running() const {
    return impl_->is_running();
}

void V2Server::set_data_handler([[maybe_unused]] V2DataHandler h) {
    impl_->set_data_handler([[maybe_unused]] std::move(h));
}
void V2Server::set_headers_handler([[maybe_unused]] V2HeadersHandler h) {
    impl_->set_headers_handler([[maybe_unused]] std::move(h));
}
void V2Server::set_rst_stream_handler([[maybe_unused]] V2RstStreamHandler h) {
    impl_->set_rst_stream_handler([[maybe_unused]] std::move(h));
}

bool V2Server::push_to_client(const std::string &conn_id, uint32_t associated_sid,
                              const std::unordered_map<std::string, std::string> &headers,
                              const std::vector<uint8_t> &data) {
    return impl_->push_to_client(conn_id, associated_sid, headers, data);
}

size_t V2Server::active_connections() const {
    return impl_->active_connections();
}
uint64_t V2Server::total_streams_opened() const {
    return impl_->total_streams_opened();
}
uint64_t V2Server::total_frames_sent() const {
    return impl_->total_frames_sent();
}
uint64_t V2Server::total_frames_received() const {
    return impl_->total_frames_received();
}

} // namespace wire
} // namespace themis
