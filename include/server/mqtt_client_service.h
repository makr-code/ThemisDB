/**
 * @file mqtt_client_service.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Configuration for MqttClientService.
 *
 * Credentials (username / password) must be loaded from
 * config/security/ paths via ConfigPathResolver::resolve();
 * they are never logged even at DEBUG level.
 */
struct MqttClientConfig {
    /// External broker hostname or IP address.
    std::string broker_host{"localhost"};

    /// External broker TCP port (default plain-text: 1883, TLS: 8883).
    uint16_t broker_port{1883};

    /**
     * @brief MQTT client identifier.
     *
     * Must be unique per broker.  When empty, MqttClientService generates a
     * random identifier of the form "themisdb-<random-8-hex>".
     */
    std::string client_id{};

    /// MQTT username (empty = anonymous access).
    std::string username{};

    /// MQTT password — load from config/security/; never log.
    std::string password{};

    /// When true, the broker discards any persistent session state on connect.
    bool clean_session{true};

    /// MQTT keepalive interval in seconds.  0 disables keepalive.
    uint16_t keepalive_seconds{60};

    /// Default QoS for publish() calls that do not specify an explicit value.
    uint8_t default_qos{1};

    // TLS
    bool tls_enabled{false};           ///< Enable TLS (requires broker_port 8883 typically).
    std::string tls_cert_path{};       ///< Client certificate (PEM) for mutual TLS.
    std::string tls_key_path{};        ///< Client private key (PEM) for mutual TLS.
    std::string tls_ca_path{};         ///< CA certificate bundle for broker verification.

    /// Topic prefix prepended to all topics emitted by MqttCDCTransport.
    /// Example: "themis/cdc/" produces "themis/cdc/orders/PUT".
    std::string cdc_topic_prefix{"themis/cdc/"};

    /// QoS used by MqttCDCTransport for CDC event publishing.
    uint8_t cdc_qos{1};

    /// Reconnection policy (initial delay, max delay, backoff multiplier).
    MqttRetryConfig retry{};

    /// TCP connection timeout in milliseconds.
    uint32_t connect_timeout_ms{10000};

    /// Maximum size (bytes) of the internal outbound packet queue.
    /// Packets are dropped (and publish_errors incremented) when the queue is full.
    size_t max_outbound_queue{4096};
};

// ── Statistics ────────────────────────────────────────────────────────────────

/**
 * @brief Atomic performance counters for MqttClientService.
 *
 * All fields are std::atomic<> — safe to read from any thread.
 * Intentionally non-copyable; access via const reference.
 */
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

/**
 * @brief Callback interface for inbound MQTT messages.
 *
 * Implementations must be thread-safe: all callbacks are invoked from the
 * internal I/O thread of MqttClientService.
 *
 * All methods are noexcept; exceptions thrown from an implementation are
 * silently caught and ignored.
 */
class IMqttMessageHandler {
public:
    virtual ~IMqttMessageHandler() = default;

    /**
     * @brief Called for every PUBLISH packet received from the broker.
     * @param topic    Fully-qualified topic string.
     * @param payload  Raw message payload (UTF-8 or binary).
     * @param qos      QoS level of the received message (0, 1, or 2).
     */
    virtual void onMessage(const std::string& topic,
                           const std::string& payload,
                           uint8_t            qos) noexcept = 0;

    /**
     * @brief Called once the CONNACK is received and the session is live.
     * @param client_id  The effective client ID used for this session.
     */
    virtual void onConnected([[maybe_unused]] const std::string&) noexcept {}

    /**
     * @brief Called when the connection is lost or cleanly closed.
     * @param reason  Human-readable reason string.
     */
    virtual void onDisconnected([[maybe_unused]] const std::string&) noexcept {}
};

// ── CDC → MQTT transport ──────────────────────────────────────────────────────

/**
 * @brief CDC transport that forwards Changefeed events to an MQTT broker.
 *
 * Implements ICDCTransport: serialises each ChangeEvent to JSON and publishes
 * it to a topic derived from the event type and collection:
 *
 *   {cdc_topic_prefix}{collection}/{EVENT_TYPE}
 *
 * Examples (prefix = "themis/cdc/"):
 *   - PUT event on "orders"   → "themis/cdc/orders/PUT"
 *   - DELETE event on "users" → "themis/cdc/users/DELETE"
 *   - TRANSACTION_COMMIT      → "themis/cdc/$system/TRANSACTION_COMMIT"
 *
 * Lifecycle is delegated to the owning MqttClientService.
 */
class MqttCDCTransport : public cdc::ICDCTransport {
public:
    /**
     * @brief Construct; the service must outlive this object.
     * @param service  Owning MqttClientService.
     */
    explicit MqttCDCTransport(MqttClientService& service);

    // ICDCTransport
    bool start() override;
    void stop()  override;

    /**
     * @brief Serialise and publish one ChangeEvent to the MQTT broker.
     *
     * Derives the target topic via topicForEvent() and calls
     * MqttClientService::publish() with the configured CDC QoS.
     *
     * @return true if the event was accepted for delivery; false when the
     *         service is not connected or the outbound queue is full.
     */
    bool publish(const Changefeed::ChangeEvent& event) override;

    /**
     * @brief Compute the MQTT topic for a given ChangeEvent.
     *
     * Format: {cdc_topic_prefix}{collection}/{EVENT_TYPE}
     * where EVENT_TYPE is one of PUT, DELETE, TRANSACTION_COMMIT,
     * TRANSACTION_ROLLBACK, or UNKNOWN.
     */
    std::string topicForEvent(const Changefeed::ChangeEvent& event) const;

    /** @brief Override the CDC topic prefix (default from MqttClientConfig). */
    void setTopicPrefix(const std::string& prefix);

    /** @brief Override the CDC publish QoS (default from MqttClientConfig). */
    void setQos(uint8_t qos);

    const std::string& topicPrefix() const noexcept { return topic_prefix_; }
    uint8_t qos() const noexcept { return qos_; }

private:
    MqttClientService& service_;
    std::string topic_prefix_;
    uint8_t     qos_;
};

// ── Client service ────────────────────────────────────────────────────────────

/**
 * @brief Bidirectional MQTT client service.
 *
 * Manages a persistent connection to an external MQTT broker:
 *   - outbound: publish() + MqttCDCTransport for CDC event forwarding,
 *   - inbound:  topic subscriptions dispatched to IMqttMessageHandler,
 *   - lifecycle: automatic reconnect with exponential back-off,
 *   - discovery: registers itself with RPCServiceRegistry.
 *
 * Thread-safety:
 *   - start(), stop(), publish(), subscribe(), unsubscribe() are safe to call
 *     from any thread; they post work to the internal asio::io_context.
 *   - setMessageHandler() is safe to call before start().  Replacing the
 *     handler while the service is running is permitted but the old handler
 *     may still receive a last in-flight callback.
 *   - getStats() / resetStats() are always thread-safe (atomic fields).
 */
class MqttClientService {
public:
    /**
     * @brief Construct with the given configuration.
     *
     * Does not establish a network connection; call start() to do so.
     */
    explicit MqttClientService(MqttClientConfig config = {});
    ~MqttClientService();

    // Non-copyable, non-movable.
    MqttClientService(const MqttClientService&) = delete;
    MqttClientService& operator=(const MqttClientService&) = delete;

    // ── Lifecycle ──────────────────────────────────────────────────────────

    /**
     * @brief Start the service and initiate the broker connection.
     *
     * Spawns a background I/O thread and asynchronously connects to the
     * configured broker.  Returns immediately; connection success is signalled
     * via IMqttMessageHandler::onConnected().
     *
     * Calling start() on an already-running service is a no-op.
     */
    void start();

    /**
     * @brief Stop the service and disconnect from the broker.
     *
     * Sends a DISCONNECT packet, closes the TCP connection, and joins the
     * background thread.  Calling stop() on a stopped service is a no-op.
     */
    void stop();

    /** @return true while the CONNACK has been received and the link is live. */
    bool isConnected() const noexcept;

    // ── Publish ────────────────────────────────────────────────────────────

    /**
     * @brief Publish a message to the broker.
     *
     * Thread-safe; posts the packet to the internal I/O thread.
     *
     * @param topic    Target topic string.
     * @param payload  Message payload (UTF-8 or binary).
     * @param qos      QoS level (0, 1).  QoS 2 is not supported in v1.9.0.
     * @param retain   When true, the broker stores this message as the
     *                 retained value for the topic.
     * @return false if the service is not connected or the outbound queue is
     *         full; increments stats_.publish_errors in that case.
     */
    bool publish(const std::string& topic,
                 const std::string& payload,
                 uint8_t            qos    = 1,
                 bool               retain = false);

    // ── Subscribe / unsubscribe ────────────────────────────────────────────

    /**
     * @brief Subscribe to a topic filter.
     *
     * If the service is already connected, the SUBSCRIBE packet is sent
     * immediately.  Otherwise the subscription is queued and sent once the
     * connection is established (including after reconnects).
     *
     * @param topic_filter  MQTT topic filter (supports '+' and '#' wildcards).
     * @param qos           Maximum QoS for delivered messages (0 or 1).
     * @return Always true (the subscription is registered for delivery).
     */
    bool subscribe(const std::string& topic_filter, uint8_t qos = 1);

    /**
     * @brief Unsubscribe from a topic filter.
     *
     * Sends UNSUBSCRIBE when connected; removes the filter from the pending
     * subscription list when not yet connected.
     */
    bool unsubscribe(const std::string& topic_filter);

    // ── Handler ────────────────────────────────────────────────────────────

    /**
     * @brief Register an inbound message handler.
     *
     * The handler is invoked from the internal I/O thread for every inbound
     * PUBLISH message.  Replaces any previously registered handler.
     */
    void setMessageHandler(std::shared_ptr<IMqttMessageHandler> handler);

    // ── CDC transport ──────────────────────────────────────────────────────

    /** @return Reference to the CDC→MQTT transport owned by this service. */
    MqttCDCTransport& cdcTransport() noexcept { return cdc_transport_; }

    // ── Statistics ─────────────────────────────────────────────────────────

    /** @return Read-only reference to the atomic stats counters. */
    const MqttClientStats& getStats() const noexcept { return stats_; }

    /** @brief Reset all stats counters to zero. */
    void resetStats() noexcept { stats_.reset(); }

    // ── Service registry ───────────────────────────────────────────────────

    /**
     * @brief Register this service with the global RPCServiceRegistry.
     *
     * After registration, other components can retrieve the service via
     *   plugins::rpc::RPCServiceRegistry::getService(service_name)
     * and cast the returned void* to MqttClientService*.
     *
     * @param service_name  Registry key (default: "mqtt_client").
     */
    void registerWithServiceRegistry(
        const std::string& service_name = "mqtt_client");

    /**
     * @brief Remove the service registration from the RPCServiceRegistry.
     * @param service_name  Registry key used in registerWithServiceRegistry().
     */
    void unregisterFromServiceRegistry(
        const std::string& service_name = "mqtt_client");

    // ── Configuration ──────────────────────────────────────────────────────

    /** @return The configuration snapshot used to construct this service. */
    const MqttClientConfig& getConfig() const noexcept { return config_; }

    /** @return The effective client ID (auto-generated when config was empty). */
    const std::string& clientId() const noexcept { return effective_client_id_; }

private:
    // ── Internal helpers ───────────────────────────────────────────────────

    void ioThreadEntry();
    void doConnect();
    void sendMqttConnect();
    void doRead();
    void doWrite();
    void onConnAck(uint8_t flags, uint8_t return_code);
    void onPublishReceived(const std::string& topic,
                           const std::string& payload,
                           uint8_t            qos);
    void processBuffer();
    void startKeepalive();
    void scheduleReconnect();
    void handleDisconnect(const std::string& reason);
    void enqueuePacket(std::vector<uint8_t> packet);
    void sendSubscriptions();
#ifdef THEMIS_ENABLE_MQTT_TLS
    void doHandshake();
#endif

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

/** @brief I mqtt message event handler. */
class IMqttMessageHandler {
public:
    virtual ~IMqttMessageHandler() = default;
    virtual void onMessage(const std::string&, const std::string&, uint8_t) noexcept = 0;
    virtual void onConnected(const std::string&) noexcept {}
    virtual void onDisconnected(const std::string&) noexcept {}
};

class MqttClientService;

/** @brief Mqtt cdc transport. */
class MqttCDCTransport : public cdc::ICDCTransport {
public:
    explicit MqttCDCTransport(MqttClientService& s) : service_(s) {}
    bool start() override { return false; }
    void stop()  override {}
    bool publish(const Changefeed::ChangeEvent&) override { return false; }
    std::string topicForEvent(const Changefeed::ChangeEvent&) const { return {}; }
    void setTopicPrefix(const std::string&) {}
    void setQos(uint8_t) {}
    const std::string& topicPrefix() const noexcept { static std::string s; return s; }
    uint8_t qos() const noexcept { return 0; }
private:
    MqttClientService& service_;
};

/** @brief Mqtt client service component. */
class MqttClientService {
public:
    explicit MqttClientService(MqttClientConfig = {}) : cdc_transport_(*this) {}
    ~MqttClientService() = default;
    MqttClientService(const MqttClientService&)            = delete;
    MqttClientService& operator=(const MqttClientService&) = delete;
    void start()  {}
    void stop()   {}
    bool isConnected() const noexcept { return false; }
    bool publish(const std::string&, const std::string&, uint8_t = 1, bool = false) { return false; }
    bool subscribe(const std::string&, uint8_t = 1) { return false; }
    bool unsubscribe(const std::string&) { return false; }
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
