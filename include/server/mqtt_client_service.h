/**
 * @file mqtt_client_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "server/mqtt_session.h" // MqttRetryConfig, MqttMetrics, MqttProperties

#ifdef THEMIS_ENABLE_MQTT

#include "cdc/icdc_transport.h"
#include "cdc/changefeed.h"
#include "plugins/rpc_plugin_interface.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace boost { namespace asio { class io_context; } }

namespace themis {
namespace server {

inline constexpr std::string_view kMqttTlsVerifyNoneFallbackLogPrefix =
    "[SECURITY][TLS] MQTT TLS verify_none fallback active";

// ── Forward declarations ──────────────────────────────────────────────────────

class MqttClientService;

// ── Configuration ─────────────────────────────────────────────────────────────

struct MqttClientConfig {
    std::string broker_host{"localhost"};

    uint16_t broker_port{1883};

    std::string client_id{};

    std::string username{};

    std::string password{};

    bool clean_session{true};

    uint16_t keepalive_seconds{60};

    uint8_t default_qos{1};

    // TLS
    bool tls_enabled{false};           ///< Enable TLS (requires broker_port 8883 typically).
    std::string tls_cert_path{};       ///< Client certificate (PEM) for mutual TLS.
    std::string tls_key_path{};        ///< Client private key (PEM) for mutual TLS.
    std::string tls_ca_path{};         ///< CA certificate bundle for broker verification.

    std::string cdc_topic_prefix{"themis/cdc/"};

    uint8_t cdc_qos{1};

    MqttRetryConfig retry{};

    uint32_t connect_timeout_ms{10000};

    size_t max_outbound_queue{4096};
};

// ── Statistics ────────────────────────────────────────────────────────────────

struct MqttClientStats {
    std::atomic<uint64_t> messages_published{0};  ///< Successful outbound publishes.
    std::atomic<uint64_t> messages_received{0};   ///< Inbound messages dispatched to handler.
    std::atomic<uint64_t> bytes_sent{0};           ///< Total bytes written to the broker.
    std::atomic<uint64_t> bytes_received{0};       ///< Total bytes read from the broker.
    std::atomic<uint64_t> connect_count{0};        ///< Successful TCP+CONNACK sequences.
    std::atomic<uint64_t> reconnect_count{0};      ///< Automatic reconnection attempts.
    std::atomic<uint64_t> publish_errors{0};       ///< Failed publish calls (not connected or queue full).
    std::atomic<uint64_t> subscribe_count{0};      ///< SUBSCRIBE packets successfully sent.
    std::atomic<bool>     is_connected{false};     ///< True while CONNACK was received and connection is live.

    MqttClientStats() = default;

    MqttClientStats(const MqttClientStats&) = delete;
    MqttClientStats& operator=(const MqttClientStats&) = delete;

    void reset() noexcept {
        messages_published = 0;
        messages_received  = 0;
        bytes_sent         = 0;
        bytes_received     = 0;
        connect_count      = 0;
        reconnect_count    = 0;
        publish_errors     = 0;
        subscribe_count    = 0;
        is_connected       = false;
    }
};

// ── Message handler interface ─────────────────────────────────────────────────

class IMqttMessageHandler {
public:
    /**
     * @brief IMqtt Message Handler.
     * @return Return value.
     */
    virtual ~IMqttMessageHandler() = default;

    /**
     * @brief On Message.
     * @param[in] topic Input parameter.
     * @param[in] payload Input parameter.
     * @param[in] qos Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void onMessage(const std::string& topic,
                           const std::string& payload,
                           uint8_t            qos) noexcept = 0;

    virtual void onConnected([[maybe_unused]] const std::string&) noexcept {}

    virtual void onDisconnected([[maybe_unused]] const std::string&) noexcept {}
};

// ── CDC → MQTT transport ──────────────────────────────────────────────────────

class MqttCDCTransport : public cdc::ICDCTransport {
public:
    /**
     * @brief Mqtt CDCTransport.
     * @param[in,out] service Input/output parameter.
     * @return Return value.
     */
    explicit MqttCDCTransport(MqttClientService& service);

    // ICDCTransport
    bool start() override;
    void stop()  override;

    bool publish(const Changefeed::ChangeEvent& event) override;

    /**
     * @brief Topic For Event.
     * @param[in] event Input parameter.
     * @return Return value.
     */
    std::string topicForEvent(const Changefeed::ChangeEvent& event) const;

    /**
     * @brief Set Topic Prefix.
     * @param[in] prefix Input parameter.
     */
    void setTopicPrefix(const std::string& prefix);

    /**
     * @brief Set Qos.
     * @param[in] qos Input parameter.
     */
    void setQos(uint8_t qos);

    const std::string& topicPrefix() const noexcept { return topic_prefix_; }
    uint8_t qos() const noexcept { return qos_; }

private:
    MqttClientService& service_;
    std::string topic_prefix_;
    uint8_t     qos_;
};

// ── Client service ────────────────────────────────────────────────────────────

class MqttClientService {
public:
    explicit MqttClientService(MqttClientConfig config = {});
    ~MqttClientService();

    // Non-copyable, non-movable.
    MqttClientService(const MqttClientService&) = delete;
    MqttClientService& operator=(const MqttClientService&) = delete;


    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Stop.
     */
    void stop();

    /**
     * @brief Is Connected.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isConnected() const noexcept;

    // ── Publish ────────────────────────────────────────────────────────────

    bool publish(const std::string& topic,
                 const std::string& payload,
                 uint8_t            qos    = 1,
                 bool               retain = false);

    // ── Subscribe / unsubscribe ────────────────────────────────────────────

    bool subscribe(const std::string& topic_filter, uint8_t qos = 1);

    /**
     * @brief Unsubscribe.
     * @param[in] topic_filter Input parameter.
     * @return True when the operation succeeds.
     */
    bool unsubscribe(const std::string& topic_filter);


    /**
     * @brief Set Message Handler.
     * @param[in] handler Input parameter.
     */
    void setMessageHandler(std::shared_ptr<IMqttMessageHandler> handler);

    // ── CDC transport ──────────────────────────────────────────────────────

    MqttCDCTransport& cdcTransport() noexcept { return cdc_transport_; }

    // ── Statistics ─────────────────────────────────────────────────────────

    const MqttClientStats& getStats() const noexcept { return stats_; }

    void resetStats() noexcept { stats_.reset(); }

    // ── Service registry ───────────────────────────────────────────────────

    void registerWithServiceRegistry(
        const std::string& service_name = "mqtt_client");

    void unregisterFromServiceRegistry(
        const std::string& service_name = "mqtt_client");

    // ── Configuration ──────────────────────────────────────────────────────

    const MqttClientConfig& getConfig() const noexcept { return config_; }

    const std::string& clientId() const noexcept { return effective_client_id_; }

private:

    /**
     * @brief Io Thread Entry.
     */
    void ioThreadEntry();
    /**
     * @brief Do Connect.
     */
    void doConnect();
    /**
     * @brief Send Mqtt Connect.
     */
    void sendMqttConnect();
    /**
     * @brief Do Read.
     */
    void doRead();
    /**
     * @brief Do Write.
     */
    void doWrite();
    /**
     * @brief On Conn Ack.
     * @param[in] flags Input parameter.
     * @param[in] return_code Input parameter.
     */
    void onConnAck(uint8_t flags, uint8_t return_code);
    /**
     * @brief On Publish Received.
     * @param[in] topic Input parameter.
     * @param[in] payload Input parameter.
     * @param[in] qos Input parameter.
     */
    void onPublishReceived(const std::string& topic,
                           const std::string& payload,
                           uint8_t            qos);
    /**
     * @brief Process Buffer.
     */
    void processBuffer();
    /**
     * @brief Start Keepalive.
     */
    void startKeepalive();
    /**
     * @brief Schedule Reconnect.
     */
    void scheduleReconnect();
    /**
     * @brief Handle Disconnect.
     * @param[in] reason Input parameter.
     */
    void handleDisconnect(const std::string& reason);
    /**
     * @brief Enqueue Packet.
     * @param[in] packet Input parameter.
     */
    void enqueuePacket(std::vector<uint8_t> packet);
    /**
     * @brief Send Subscriptions.
     */
    void sendSubscriptions();
#ifdef THEMIS_ENABLE_MQTT_TLS
    /**
     * @brief Do Handshake.
     */
    void doHandshake();
#endif

    /**
     * @brief Generate Client Id.
     * @return Return value.
     */
    static std::string generateClientId();

    // ── Members ────────────────────────────────────────────────────────────

    MqttClientConfig   config_;
    std::string        effective_client_id_;

    // io_context_ must be declared before socket_ and timers.
    struct AsioImpl;
    std::unique_ptr<AsioImpl> asio_;

    std::thread io_thread_ = {};
    std::atomic<bool> running_{false};

    // Pending subscriptions: topic_filter → qos  (access from io_thread_ only)
    std::map<std::string, uint8_t> subscriptions_;

    // Outbound packet queue (protected by outbound_mutex_)
    std::deque<std::vector<uint8_t>> outbound_queue_;
    std::mutex                        outbound_mutex_;
    bool                              writing_{false};

    // Read buffer
    std::vector<uint8_t> read_buf_;     ///< Raw bytes from async_read_some
    std::vector<uint8_t> packet_buf_;   ///< Reassembly buffer for partial packets

    // Packet ID counter (io_thread_ only)
    uint16_t next_packet_id_{1};

    // Reconnect state
    uint32_t reconnect_attempt_{0};

    // Handler
    std::shared_ptr<IMqttMessageHandler> handler_;
    std::mutex                            handler_mutex_;

    MqttClientStats  stats_;
    MqttCDCTransport cdc_transport_;

    std::string registered_service_name_;
};

} // namespace server
} // namespace themis

#else // !THEMIS_ENABLE_MQTT — no-op stubs ─────────────────────────────────────

#include "cdc/icdc_transport.h"
#include "cdc/changefeed.h"
#include <string>
#include <memory>

namespace themis {
namespace server {

struct MqttClientConfig {
    std::string broker_host{"localhost"};
    uint16_t    broker_port{1883};
    std::string client_id{};
    std::string username{};
    std::string password{};
    bool        clean_session{true};
    uint16_t    keepalive_seconds{60};
    uint8_t     default_qos{1};
    bool        tls_enabled{false};
    std::string tls_cert_path{};
    std::string tls_key_path{};
    std::string tls_ca_path{};
    std::string cdc_topic_prefix{"themis/cdc/"};
    uint8_t     cdc_qos{1};
    uint32_t    connect_timeout_ms{10000};
    size_t      max_outbound_queue{4096};
};

class IMqttMessageHandler {
public:
    /**
     * @brief IMqtt Message Handler.
     * @return Return value.
     */
    virtual ~IMqttMessageHandler() = default;
    /**
     * @brief On Message.
     * @param[in] param Input parameter.
     * @param[in] param Input parameter.
     * @param[in] uint8_t Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void onMessage(const std::string&, const std::string&, uint8_t) noexcept = 0;
    virtual void onConnected(const std::string&) noexcept {}
    virtual void onDisconnected(const std::string&) noexcept {}
};

class MqttClientService;

class MqttCDCTransport : public cdc::ICDCTransport {
public:
    explicit MqttCDCTransport(MqttClientService& s) : service_(s) {}
    bool start() override { return false; }
    void stop()  override {}
    bool publish(const Changefeed::ChangeEvent&) override { return false; }
    std::string topicForEvent(const Changefeed::ChangeEvent&) const { return {}; }
    /**
     * @brief Set Topic Prefix.
     * @param[in] param Input parameter.
     * @details Implements setTopicPrefix without additional internal calls.
     */
    void setTopicPrefix(const std::string&) {}
    /**
     * @brief Set Qos.
     * @param[in] uint8_t Input parameter.
     * @details Implements setQos without additional internal calls.
     */
    void setQos(uint8_t) {}
    const std::string& topicPrefix() const noexcept { static std::string s; return s; }
    uint8_t qos() const noexcept { return 0; }
private:
    MqttClientService& service_;
};

class MqttClientService {
public:
    explicit MqttClientService(MqttClientConfig = {}) : cdc_transport_(*this) {}
    ~MqttClientService() = default;
    MqttClientService(const MqttClientService&)            = delete;
    MqttClientService& operator=(const MqttClientService&) = delete;
    /**
     * @brief Start.
     * @details Implements start without additional internal calls.
     */
    void start()  {}
    /**
     * @brief Stop.
     * @details Implements stop without additional internal calls.
     */
    void stop()   {}
    bool isConnected() const noexcept { return false; }
    bool publish(const std::string&, const std::string&, uint8_t = 1, bool = false) { return false; }
    bool subscribe(const std::string&, uint8_t = 1) { return false; }
    /**
     * @brief Unsubscribe.
     * @param[in] param Input parameter.
     * @return True when the operation succeeds.
     * @details Implements unsubscribe without additional internal calls.
     */
    bool unsubscribe(const std::string&) { return false; }
    /**
     * @brief Set Message Handler.
     * @param[in] param Input parameter.
     * @details Implements setMessageHandler without additional internal calls.
     */
    void setMessageHandler(std::shared_ptr<IMqttMessageHandler>) {}
    MqttCDCTransport& cdcTransport() noexcept { return cdc_transport_; }
    void registerWithServiceRegistry(const std::string& = "mqtt_client") {}
    void unregisterFromServiceRegistry(const std::string& = "mqtt_client") {}
    const MqttClientConfig& getConfig() const noexcept { return config_; }
    const std::string& clientId() const noexcept { return effective_client_id_; }
private:
    MqttClientConfig config_;
    std::string effective_client_id_;
    MqttCDCTransport cdc_transport_;
};

} // namespace server
} // namespace themis

#endif // THEMIS_ENABLE_MQTT
