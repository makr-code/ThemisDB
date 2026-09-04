/**
 * @file mqtt_client_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=14, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/mqtt_client_service.h"

#ifdef THEMIS_ENABLE_MQTT

#include <boost/asio.hpp>
#ifdef THEMIS_ENABLE_MQTT_TLS
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>
#endif
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <future>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include "utils/logger.h"

namespace asio = boost::asio;

namespace themis {
namespace server {

// ── MQTT packet helpers ───────────────────────────────────────────────────────

namespace detail {

// Encode a variable-length integer (MQTT remaining-length encoding).
static std::vector<uint8_t> encodeVarLen([[maybe_unused]] uint32_t value) {
    std::vector<uint8_t> out;
    do {
        uint8_t byte = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;
        if (value > 0) {
          byte |= 0x80;
        }
        out.push_back(byte);
    } while (value > 0);
    return out;
}

// Decode variable-length integer from buffer.
// Returns (decoded_value, bytes_consumed) or (0,0) on incomplete data.
static std::pair<uint32_t, size_t> decodeVarLen(const uint8_t* data, size_t len) {
    uint32_t value = 0;
    uint32_t multiplier = 1;
    size_t   consumed = 0;
    for (size_t i = 0; i < std::min(len, size_t{4}); ++i) {
        uint8_t byte = data[i];
        value += (byte & 0x7F) * multiplier;
        multiplier <<= 7;
        ++consumed;
        if ((byte & 0x80) == 0) return {value, consumed};
    }
    return {0, 0}; // incomplete
}

// Write a 2-byte big-endian integer.
static void write16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val >> 8));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

// Write an MQTT UTF-8 string (2-byte length prefix + data).
static void writeStr(std::vector<uint8_t>& buf, const std::string& s) {
    write16(buf, static_cast<uint16_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

// Read an MQTT UTF-8 string from buffer.
// Returns (string, bytes_consumed) or ("", 0) on incomplete data.
static std::pair<std::string, size_t> readStr(const uint8_t* data, size_t len) {
    if (len < 2) return {"", 0};
    uint16_t slen = static_cast<uint16_t>((data[0] << 8) | data[1]);
    if (len < size_t{2} + slen) return {"", 0};
    return {std::string(reinterpret_cast<const char*>(data + 2), slen),
            size_t{2} + slen};
}

// ── Packet builders ────────────────────────────────────────────────────────

static std::vector<uint8_t> buildConnect(const MqttClientConfig& cfg,
                                         const std::string& client_id) {
    // Variable header + payload
    std::vector<uint8_t> payload;

    // Protocol Name "MQTT" + version 4 (3.1.1)
    writeStr(payload, "MQTT");
    payload.push_back(0x04); // protocol level

    // Connect Flags
    uint8_t flags = 0;
    if (cfg.clean_session) {
      flags |= 0x02;
    }
    if (!cfg.username.empty()) {
      flags |= 0x80;
    }
    if (!cfg.password.empty()) {
      flags |= 0x40;
    }
    payload.push_back(flags);

    // Keep Alive
    write16(payload, cfg.keepalive_seconds);

    // Payload: client_id
    writeStr(payload, client_id);
    if (!cfg.username.empty()) {
      writeStr(payload, cfg.username);
    }
    if (!cfg.password.empty()) {
      writeStr(payload, cfg.password);
    }

    // Assemble packet
    std::vector<uint8_t> pkt;
    pkt.push_back(0x10); // CONNECT type
    auto vl = encodeVarLen(static_cast<uint32_t>(payload.size()));
    pkt.insert(pkt.end(), vl.begin(), vl.end());
    pkt.insert(pkt.end(), payload.begin(), payload.end());
    return pkt;
}

static std::vector<uint8_t> buildPublish(const std::string& topic,
                                         const std::string& payload,
                                         uint8_t qos, bool retain,
                                         uint16_t packet_id) {
    std::vector<uint8_t> vheader;
    writeStr(vheader, topic);
    if (qos > 0) {
      write16(vheader, packet_id);
    }
    vheader.insert(vheader.end(), payload.begin(), payload.end());

    uint8_t flags = static_cast<uint8_t>((qos & 0x03) << 1);
    if (retain) {
      flags |= 0x01;
    }

    std::vector<uint8_t> pkt;
    pkt.push_back(static_cast<uint8_t>(0x30 | flags));
    auto vl = encodeVarLen(static_cast<uint32_t>(vheader.size()));
    pkt.insert(pkt.end(), vl.begin(), vl.end());
    pkt.insert(pkt.end(), vheader.begin(), vheader.end());
    return pkt;
}

static std::vector<uint8_t> buildSubscribe(
        uint16_t packet_id,
        const std::vector<std::pair<std::string, uint8_t>>& topics) {
    std::vector<uint8_t> vheader;
    write16(vheader, packet_id);
    for (const auto& [filter, qos] : topics) {
        writeStr(vheader, filter);
        vheader.push_back(qos & 0x03);
    }

    std::vector<uint8_t> pkt;
    pkt.push_back(0x82); // SUBSCRIBE
    auto vl = encodeVarLen(static_cast<uint32_t>(vheader.size()));
    pkt.insert(pkt.end(), vl.begin(), vl.end());
    pkt.insert(pkt.end(), vheader.begin(), vheader.end());
    return pkt;
}

static std::vector<uint8_t> buildUnsubscribe(
        uint16_t packet_id,
        const std::vector<std::string>& topics) {
    std::vector<uint8_t> vheader;
    write16(vheader, packet_id);
    for (const auto& filter : topics)
        writeStr(vheader, filter);

    std::vector<uint8_t> pkt;
    pkt.push_back(0xA2); // UNSUBSCRIBE
    auto vl = encodeVarLen(static_cast<uint32_t>(vheader.size()));
    pkt.insert(pkt.end(), vl.begin(), vl.end());
    pkt.insert(pkt.end(), vheader.begin(), vheader.end());
    return pkt;
}

static std::vector<uint8_t> buildPingReq()    { return {0xC0, 0x00}; }
static std::vector<uint8_t> buildDisconnect() { return {0xE0, 0x00}; }
static std::vector<uint8_t> buildPubAck([[maybe_unused]] uint16_t id) {
    return {0x40, 0x02,
            static_cast<uint8_t>(id >> 8),
            static_cast<uint8_t>(id & 0xFF)};
}

} // namespace detail

// ── AsioImpl ─────────────────────────────────────────────────────────────────
// PIMPL wrapper that keeps all Boost.Asio types out of the public header.

struct MqttClientService::AsioImpl {
    asio::io_context              io_ctx;
    asio::ip::tcp::socket         socket{io_ctx};
    asio::steady_timer            keepalive_timer{io_ctx};
    asio::steady_timer            reconnect_timer{io_ctx};
    asio::steady_timer            connect_timer{io_ctx};
    asio::executor_work_guard<asio::io_context::executor_type>
                                  work_guard{asio::make_work_guard(io_ctx)};
#ifdef THEMIS_ENABLE_MQTT_TLS
    // TLS context and stream.  Both are re-created on each connection attempt
    // when tls_enabled=true.  ssl_stream takes ownership of the connected
    // plain socket via std::move; socket is then reset to a fresh value.
    std::unique_ptr<boost::asio::ssl::context>
        ssl_ctx;
    std::unique_ptr<boost::asio::ssl::stream<asio::ip::tcp::socket>>
        ssl_stream;
    bool tls_ready{false};  ///< true after the TLS handshake succeeds.
#endif
};

// ── MqttClientService ─────────────────────────────────────────────────────────

static std::string generateClientIdImpl() {
    std::random_device rd = {};
    std::mt19937       gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);
    std::ostringstream oss = {};
    oss << "themisdb-" << std::hex << std::setw(8) << std::setfill('0')
        << dist(gen);
    return oss.str();
}

MqttClientService::MqttClientService(MqttClientConfig config)
    : config_(std::move(config))
    , effective_client_id_(config_.client_id.empty()
                               ? generateClientIdImpl()
                               : config_.client_id)
    , asio_(std::make_unique<AsioImpl>())
    , read_buf_(8192)
    , cdc_transport_(*this) {}

MqttClientService::~MqttClientService() {
    stop();
}

void MqttClientService::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
      return;
    }

    asio::post(asio_->io_ctx, [this] { doConnect(); });
    io_thread_ = std::thread([this] { ioThreadEntry(); });
}

void MqttClientService::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
      return;
    }

    asio::post(asio_->io_ctx, [this] {
        asio_->keepalive_timer.cancel();
        asio_->reconnect_timer.cancel();
        asio_->connect_timer.cancel();
        if (stats_.is_connected.load()) {
            boost::system::error_code ec;
            auto pkt = detail::buildDisconnect();
            // W1-FIX(no_timeout): set a 5-second send-timeout on the socket
            // before the synchronous DISCONNECT write so it cannot block forever.
            constexpr int kSendTimeoutSec = 5;
#ifdef THEMIS_ENABLE_MQTT_TLS
            if (config_.tls_enabled && asio_->ssl_stream) {
                using opt = boost::asio::socket_base::send_timeout;
                asio_->ssl_stream->lowest_layer().set_option(
                    opt{kSendTimeoutSec}, ec);
                asio::write(*asio_->ssl_stream, asio::buffer(pkt), ec);
                asio_->ssl_stream->lowest_layer().shutdown(
                    asio::ip::tcp::socket::shutdown_both, ec);
                asio_->ssl_stream->lowest_layer().close(ec);
                asio_->ssl_stream.reset();
                asio_->tls_ready = false;
            } else
#endif
            {
                using opt = boost::asio::socket_base::send_timeout;
                asio_->socket.set_option(opt{kSendTimeoutSec}, ec);
                asio::write(asio_->socket, asio::buffer(pkt), ec);
                asio_->socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                asio_->socket.close(ec);
            }
        } else {
            boost::system::error_code ec;
#ifdef THEMIS_ENABLE_MQTT_TLS
            if (config_.tls_enabled && asio_->ssl_stream) {
                asio_->ssl_stream->lowest_layer().close(ec);
                asio_->ssl_stream.reset();
                asio_->tls_ready = false;
            } else
#endif
            {
                asio_->socket.close(ec);
            }
        }
        asio_->work_guard.reset();
    });

    // Join the io_thread.  work_guard.reset() above lets the io_context run
    // out of work, so the thread will exit its io_context::run() call very
    // shortly.  The previous std::async-based timed join was semantically
    // broken: std::future (std::launch::async) destructors always block until
    // the async lambda completes, meaning a wait_for() timeout still resulted
    // in an indefinite block at scope exit.
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    stats_.is_connected = false;
}

bool MqttClientService::isConnected() const noexcept {
    return stats_.is_connected.load();
}

bool MqttClientService::publish(const std::string& topic,
                                const std::string& payload,
                                uint8_t qos, bool retain) {
    if (!stats_.is_connected.load()) {
        ++stats_.publish_errors;
        return false;
    }
    uint16_t pid = 0;
    if (qos > 0) {
        // Generate packet ID under the outbound_mutex_
        std::lock_guard<std::mutex> lk(outbound_mutex_);
        pid = ++next_packet_id_;
        if (next_packet_id_ == 0) {
          next_packet_id_ = 1;
        }
    }
    auto pkt = detail::buildPublish(topic, payload, qos, retain, pid);
    enqueuePacket(std::move(pkt));
    ++stats_.messages_published;
    // bytes_sent is counted in doWrite() from the actual bytes written to the socket.
    return true;
}

bool MqttClientService::subscribe(const std::string& topic_filter, uint8_t qos) {
    asio::post(asio_->io_ctx, [this, topic_filter, qos] {
        subscriptions_[topic_filter] = qos;
        if (stats_.is_connected.load()) {
            uint16_t pid;
            {
                std::lock_guard<std::mutex> lk(outbound_mutex_);
                pid = ++next_packet_id_;
                if (next_packet_id_ == 0) {
                  next_packet_id_ = 1;
                }
            }
            auto pkt = detail::buildSubscribe(
                pid, {{topic_filter, qos}});
            enqueuePacket(std::move(pkt));
            ++stats_.subscribe_count;
        }
    });
    return true;
}

bool MqttClientService::unsubscribe(const std::string& topic_filter) {
    asio::post(asio_->io_ctx, [this, topic_filter] {
        subscriptions_.erase(topic_filter);
        if (stats_.is_connected.load()) {
            uint16_t pid;
            {
                std::lock_guard<std::mutex> lk(outbound_mutex_);
                pid = ++next_packet_id_;
                if (next_packet_id_ == 0) {
                  next_packet_id_ = 1;
                }
            }
            auto pkt = detail::buildUnsubscribe(pid, {topic_filter});
            enqueuePacket(std::move(pkt));
        }
    });
    return true;
}

void MqttClientService::setMessageHandler(
        std::shared_ptr<IMqttMessageHandler> handler) {
    std::lock_guard<std::mutex> lk([[maybe_unused]] handler_mutex_);
    handler_ = std::move([[maybe_unused]] handler);
}

void MqttClientService::registerWithServiceRegistry(
        const std::string& service_name) {
    plugins::rpc::RPCServiceRegistry::registerService(service_name,
                                                       static_cast<void*>(this));
    registered_service_name_ = service_name;
}

void MqttClientService::unregisterFromServiceRegistry(
        const std::string& service_name) {
    plugins::rpc::RPCServiceRegistry::unregisterService(service_name);
    if (registered_service_name_ == service_name)
        registered_service_name_.clear();
}

// ── Internal ──────────────────────────────────────────────────────────────────

void MqttClientService::ioThreadEntry() {
    asio_->io_ctx.run();
}

void MqttClientService::doConnect() {
    if (!running_.load()) {
      return;
    }

    boost::system::error_code ec;
#ifdef THEMIS_ENABLE_MQTT_TLS
    if (asio_->ssl_stream) {
        asio_->ssl_stream->lowest_layer().close(ec);
        asio_->ssl_stream.reset();
        asio_->tls_ready = false;
    }
#endif
    asio_->socket.close(ec);
    stats_.is_connected = false;
    packet_buf_.clear();

    asio::ip::tcp::resolver resolver(asio_->io_ctx);
    auto results = resolver.resolve(
        config_.broker_host,
        std::to_string(config_.broker_port), ec);

    if (ec) { scheduleReconnect(); return; }

    // Use a heap-allocated flag shared between the connect handler and the
    // timeout handler so neither captures a dangling stack reference.
    auto connected = std::make_shared<std::atomic<bool>>(false);

    asio_->socket.async_connect(
        *results.begin(),
        [this, connected](boost::system::error_code ec2) {
            asio_->connect_timer.cancel(); // cancel connection timeout
            connected->store(true);
            if (ec2) { scheduleReconnect(); return; }
#ifdef THEMIS_ENABLE_MQTT_TLS
            if (config_.tls_enabled) {
                doHandshake();
                return;
            }
#endif
            sendMqttConnect();
        });

    // Independent timer for connection-establishment timeout.
    asio_->connect_timer.expires_after(
        std::chrono::milliseconds(config_.connect_timeout_ms));
    asio_->connect_timer.async_wait([this, connected](boost::system::error_code ec3) {
        if (!ec3 && !connected->load()) {
            boost::system::error_code ce;
            asio_->socket.close([[maybe_unused]] ce); // triggers the async_connect error handler
        }
    });
}

/**
 * @brief Send MQTT CONNECT packet to the broker.
 *
 * W1-FIX(no_timeout): a 30-second send timeout is applied to the socket
 * before the synchronous write so the call cannot block indefinitely.
 */
void MqttClientService::sendMqttConnect() {
    auto pkt = detail::buildConnect(config_, effective_client_id_);
    boost::system::error_code we;
    // Apply send timeout before synchronous write to avoid indefinite blocking.
    constexpr int kConnectSendTimeoutSec = 30;
#ifdef THEMIS_ENABLE_MQTT_TLS
    if (config_.tls_enabled && asio_->tls_ready && asio_->ssl_stream) {
        using opt = boost::asio::socket_base::send_timeout;
        asio_->ssl_stream->lowest_layer().set_option(opt{kConnectSendTimeoutSec}, we);
        asio::write(*asio_->ssl_stream, asio::buffer(pkt), we);
    } else
#endif
    {
        using opt = boost::asio::socket_base::send_timeout;
        asio_->socket.set_option(opt{kConnectSendTimeoutSec}, we);
        asio::write(asio_->socket, asio::buffer(pkt), we);
    }
    if (we) { scheduleReconnect(); return; }
    stats_.bytes_sent += pkt.size();
    doRead();
}

void MqttClientService::doRead() {
    if (!running_.load()) {
      return;
    }

#ifdef THEMIS_ENABLE_MQTT_TLS
    if (config_.tls_enabled && asio_->tls_ready && asio_->ssl_stream) {
        asio_->ssl_stream->async_read_some(
            asio::buffer(read_buf_),
            [this](boost::system::error_code ec, size_t n) {
                if (ec) { handleDisconnect(ec.message()); return; }
                stats_.bytes_received += n;
                packet_buf_.insert(packet_buf_.end(),
                                   read_buf_.begin(),
                                   read_buf_.begin() + static_cast<ptrdiff_t>(n));
                processBuffer();
                doRead();
            });
        return;
    }
#endif
    asio_->socket.async_read_some(
        asio::buffer(read_buf_),
        [this](boost::system::error_code ec, size_t n) {
            if (ec) { handleDisconnect(ec.message()); return; }
            stats_.bytes_received += n;
            packet_buf_.insert(packet_buf_.end(),
                               read_buf_.begin(),
                               read_buf_.begin() + static_cast<ptrdiff_t>(n));
            processBuffer();
            doRead();
        });
}

void MqttClientService::processBuffer() {
    while (packet_buf_.size() >= 2) {
        // Fixed header: type byte
        uint8_t type_flags = packet_buf_[0];

        // Decode remaining length
        auto [rem_len, hdr_extra] = detail::decodeVarLen(
            packet_buf_.data() + 1,
            packet_buf_.size() - 1);
        if (hdr_extra == 0) break; // incomplete length

        size_t total = 1 + hdr_extra + rem_len;
        if (packet_buf_.size() < total) break; // incomplete payload

        const uint8_t* payload = packet_buf_.data() + 1 + hdr_extra;
        uint8_t type = type_flags >> 4;

        switch (type) {
        case 2: { // CONNACK
            if (rem_len >= 2) {
              onConnAck(payload[0], payload[1]);
            }
            break;
        }
        case 3: { // PUBLISH
            auto [topic, tlen] = detail::readStr(payload, rem_len);
            if (tlen == 0) {
              break;
            }
            uint8_t qos = (type_flags >> 1) & 0x03;
            size_t  off = tlen;
            uint16_t pid = 0;
            if (qos > 0 && off + 1 < rem_len) {
                pid = static_cast<uint16_t>((payload[off] << 8) | payload[off + 1]);
                off += 2;
            }
            std::string msg_payload(
                reinterpret_cast<const char*>(payload + off),
                rem_len - off);
            onPublishReceived(topic, msg_payload, qos);
            if (qos == 1) {
              enqueuePacket(detail::buildPubAck(pid));
            }
            break;
        }
        case 9:  // SUBACK — accepted, nothing extra to do
        [[fallthrough]];\n        case 11: // UNSUBACK
        [[fallthrough]];\n        case 13: // PINGRESP
            break;
        case 14: // DISCONNECT from broker
            handleDisconnect("broker sent DISCONNECT");
            break;
        default:
            break;
        }

        packet_buf_.erase(packet_buf_.begin(),
                          packet_buf_.begin() + static_cast<ptrdiff_t>(total));
    }
}

void MqttClientService::onConnAck(uint8_t /*flags*/, uint8_t return_code) {
    if (return_code != 0) {
        handleDisconnect("CONNACK rejected (code=" + std::to_string(return_code) + ")");
        return;
    }
    stats_.is_connected = true;
    ++stats_.connect_count;
    reconnect_attempt_ = 0;

    sendSubscriptions();
    startKeepalive();

    std::string cid = effective_client_id_;
    std::shared_ptr<IMqttMessageHandler> h;
    {
        std::lock_guard<std::mutex> lk([[maybe_unused]] handler_mutex_);
        h = handler_;
    }
    if (h) {
        try { h->onConnected(cid); } catch (...) {}
    }

    doWrite(); // Flush any queued publishes
}

void MqttClientService::onPublishReceived(const std::string& topic,
                                          const std::string& payload,
                                          uint8_t qos) {
    ++stats_.messages_received;
    std::shared_ptr<IMqttMessageHandler> h;
    {
        std::lock_guard<std::mutex> lk([[maybe_unused]] handler_mutex_);
        h = handler_;
    }
    if (h) {
        try { h->onMessage(topic, payload, qos); } catch (...) {}
    }
}

void MqttClientService::enqueuePacket(std::vector<uint8_t> packet) {
    {
        std::lock_guard<std::mutex> lk(outbound_mutex_);
        if (outbound_queue_.size() >= config_.max_outbound_queue) {
            ++stats_.publish_errors;
            return;
        }
        outbound_queue_.push_back(std::move(packet));
    }
    asio::post(asio_->io_ctx, [this] { doWrite(); });
}

void MqttClientService::doWrite() {
    if (writing_) {
      return;
    }
    std::vector<uint8_t> pkt;
    {
        std::lock_guard<std::mutex> lk(outbound_mutex_);
        if (outbound_queue_.empty()) {
          return;
        }
        pkt = std::move(outbound_queue_.front());
        outbound_queue_.pop_front();
    }
    writing_ = true;
    auto buf = std::make_shared<std::vector<uint8_t>>(std::move(pkt));
#ifdef THEMIS_ENABLE_MQTT_TLS
    if (config_.tls_enabled && asio_->tls_ready && asio_->ssl_stream) {
        asio::async_write(
            *asio_->ssl_stream,
            asio::buffer(*buf),
            [this, buf](boost::system::error_code ec, size_t n) {
                writing_ = false;
                if (ec) { handleDisconnect(ec.message()); return; }
                stats_.bytes_sent += n;
                doWrite(); // drain next
            });
        return;
    }
#endif
    asio::async_write(
        asio_->socket,
        asio::buffer(*buf),
        [this, buf](boost::system::error_code ec, size_t n) {
            writing_ = false;
            if (ec) { handleDisconnect(ec.message()); return; }
            stats_.bytes_sent += n;
            doWrite(); // drain next
        });
}

void MqttClientService::sendSubscriptions() {
    if (subscriptions_.empty()) {
      return;
    }
    std::vector<std::pair<std::string, uint8_t>> topics(
        subscriptions_.begin(), subscriptions_.end());
    uint16_t pid;
    {
        std::lock_guard<std::mutex> lk(outbound_mutex_);
        pid = ++next_packet_id_;
        if (next_packet_id_ == 0) {
          next_packet_id_ = 1;
        }
    }
    auto pkt = detail::buildSubscribe(pid, topics);
    enqueuePacket(std::move(pkt));
    ++stats_.subscribe_count;
}

void MqttClientService::startKeepalive() {
    if (config_.keepalive_seconds == 0) {
      return;
    }
    asio_->keepalive_timer.expires_after(
        std::chrono::seconds(config_.keepalive_seconds));
    asio_->keepalive_timer.async_wait([this](boost::system::error_code ec) {
        if (ec) {
          return;
        }
        if (stats_.is_connected.load())
            enqueuePacket(detail::buildPingReq());
        startKeepalive();
    });
}

void MqttClientService::scheduleReconnect() {
    if (!running_.load()) {
      return;
    }

    ++reconnect_attempt_;
    ++stats_.reconnect_count;

    const MqttRetryConfig& r = config_.retry;
    if (r.maxRetries > 0 && reconnect_attempt_ > r.maxRetries) {
      return;
    }

    uint32_t delay_ms = r.initialRetryDelayMs;
    if (r.exponentialBackoff && reconnect_attempt_ > 1) {
        // Use std::pow to avoid a loop that grows with reconnect_attempt_.
        float f = static_cast<float>(r.initialRetryDelayMs) *
                  std::pow(r.backoffMultiplier,
                           static_cast<float>(reconnect_attempt_ - 1));
        delay_ms = static_cast<uint32_t>(std::min(f,
                       static_cast<float>(r.maxRetryDelayMs)));
    }
    delay_ms = std::min(delay_ms, r.maxRetryDelayMs);

    asio_->reconnect_timer.expires_after(std::chrono::milliseconds(delay_ms));
    asio_->reconnect_timer.async_wait([this](boost::system::error_code ec) {
        if (!ec && running_.load()) {
          doConnect();
        }
    });
}

void MqttClientService::handleDisconnect(const std::string& reason) {
    bool was_connected = stats_.is_connected.exchange(false);
    asio_->keepalive_timer.cancel();

    boost::system::error_code ec;
#ifdef THEMIS_ENABLE_MQTT_TLS
    if (config_.tls_enabled && asio_->ssl_stream) {
        asio_->ssl_stream->lowest_layer().close(ec);
        asio_->ssl_stream.reset();
        asio_->tls_ready = false;
    } else
#endif
    {
        asio_->socket.close(ec);
    }

    if (was_connected) {
        std::shared_ptr<IMqttMessageHandler> h;
        {
            std::lock_guard<std::mutex> lk([[maybe_unused]] handler_mutex_);
            h = handler_;
        }
        if (h) {
            try { h->onDisconnected(reason); } catch (...) {}
        }
    }

    if (running_.load()) {
      scheduleReconnect();
    }
}

std::string MqttClientService::generateClientId() {
    return generateClientIdImpl();
}

#ifdef THEMIS_ENABLE_MQTT_TLS
// ── TLS Handshake ─────────────────────────────────────────────────────────────

void MqttClientService::doHandshake() {
    // Create a fresh SSL context for this connection attempt.
    try {
        asio_->ssl_ctx = std::make_unique<boost::asio::ssl::context>(
            boost::asio::ssl::context::tlsv12_client);
    } catch (...) {
        THEMIS_WARN("mqtt_client_service: unhandled exception caught");
        scheduleReconnect();
        return;
    }

    auto& ctx = *asio_->ssl_ctx;
    boost::system::error_code ec;

    ctx.set_options(
        boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2               |
        boost::asio::ssl::context::no_sslv3               |
        boost::asio::ssl::context::single_dh_use,
        ec);
    if (ec) { scheduleReconnect(); return; }

    // Broker CA certificate (peer verification).
    if (!config_.tls_ca_path.empty()) {
        ctx.load_verify_file(config_.tls_ca_path, ec);
        if (ec) { scheduleReconnect(); return; }
        ctx.set_verify_mode(boost::asio::ssl::verify_peer, ec);
        if (ec) { scheduleReconnect(); return; }
    } else {
        // GAP-017: No CA certificate configured — peer verification is disabled.
        // This means the TLS connection will NOT authenticate the broker; a
        // man-in-the-middle can intercept MQTT traffic.
        // Set verify_none only when the operator has explicitly omitted tls_ca_path,
        // and emit a loud warning so the configuration gap is visible.
        spdlog::warn("{}: "
                     "tls_ca_path is empty for broker {}. "
                     "Set tls_ca_path to enable broker certificate verification "
                     "and prevent man-in-the-middle attacks (GAP-017/CWE-295).",
                     kMqttTlsVerifyNoneFallbackLogPrefix,
                     config_.broker_host);
        ctx.set_verify_mode(boost::asio::ssl::verify_none, ec);
        if (ec) { scheduleReconnect(); return; }
    }

    // Mutual TLS: client certificate.
    if (!config_.tls_cert_path.empty()) {
        ctx.use_certificate_file(config_.tls_cert_path,
                                 boost::asio::ssl::context::pem, ec);
        if (ec) { scheduleReconnect(); return; }
    }

    // Mutual TLS: client private key.
    if (!config_.tls_key_path.empty()) {
        ctx.use_private_key_file(config_.tls_key_path,
                                 boost::asio::ssl::context::pem, ec);
        if (ec) { scheduleReconnect(); return; }
    }

    // Move the connected plain socket into the SSL stream.
    asio_->ssl_stream =
        std::make_unique<boost::asio::ssl::stream<asio::ip::tcp::socket>>(
            std::move(asio_->socket), ctx);
    // Restore a fresh socket so future reconnects work correctly.
    asio_->socket = asio::ip::tcp::socket{asio_->io_ctx};

    // SNI: let the broker present the correct certificate for virtual hosting.
    if (!config_.broker_host.empty()) {
        SSL_set_tlsext_host_name(asio_->ssl_stream->native_handle(),
                                 config_.broker_host.c_str());
    }

    // Perform the async TLS handshake.
    asio_->ssl_stream->async_handshake(
        boost::asio::ssl::stream_base::client,
        [this](boost::system::error_code hec) {
            if (hec) {
                boost::system::error_code ce;
                if (asio_->ssl_stream)
                    asio_->ssl_stream->lowest_layer().close(ce);
                asio_->ssl_stream.reset();
                scheduleReconnect();
                return;
            }
            asio_->tls_ready = true;
            sendMqttConnect();
        });
}
#endif // THEMIS_ENABLE_MQTT_TLS

// ── MqttCDCTransport ──────────────────────────────────────────────────────────

MqttCDCTransport::MqttCDCTransport(MqttClientService& service)
    : service_(service)
    , topic_prefix_(service.getConfig().cdc_topic_prefix)
    , qos_(service.getConfig().cdc_qos) {}

bool MqttCDCTransport::start() {
    service_.start();
    return true;
}

void MqttCDCTransport::stop() {
    service_.stop();
}

bool MqttCDCTransport::publish([[maybe_unused]] const Changefeed::ChangeEvent& event) {
    try {
        nlohmann::json j = event.toJson();
        std::string    payload = j.dump();
        std::string    topic   = topicForEvent([[maybe_unused]] event);
        return service_.publish(topic, payload, qos_, false);
    } catch (...) {
        THEMIS_WARN("mqtt_client_service::service_: unhandled exception caught");
        return false;
    }
}

std::string MqttCDCTransport::topicForEvent(
        const Changefeed::ChangeEvent& event) const {
    using ET = Changefeed::ChangeEventType;
    std::string type_str = {};
    switch ([[maybe_unused]] event.type) {
    case ET::EVENT_PUT:                  type_str = "PUT";                  break;
    case ET::EVENT_DELETE:               type_str = "DELETE";               break;
    case ET::EVENT_TRANSACTION_COMMIT:   type_str = "TRANSACTION_COMMIT";   break;
    case ET::EVENT_TRANSACTION_ROLLBACK: type_str = "TRANSACTION_ROLLBACK"; break;
    default:                             type_str = "UNKNOWN";              break;
    }

    // Extract collection name from metadata (key "collection") or use "default"
    std::string collection = "default";
    if (event.metadata.contains("collection") &&
        event.metadata["collection"].is_string()) {
        collection = event.metadata["collection"].get<std::string>();
    }

    return topic_prefix_ + collection + "/" + type_str;
}

void MqttCDCTransport::setTopicPrefix(const std::string& prefix) {
    topic_prefix_ = prefix;
}

void MqttCDCTransport::setQos([[maybe_unused]] uint8_t qos) {
    qos_ = qos;
}

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MQTT

