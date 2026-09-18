/**
 * @file mqtt_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_MQTT

#include <boost/asio.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <deque>
#include <array>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace asio = boost::asio;
namespace websocket = boost::beast::websocket;

// MQTT 5.0 Property types
struct MqttProperties {
    std::string contentType;
    std::string responseTopic;
    std::vector<std::pair<std::string, std::string>> userProperties;
    uint16_t topicAlias = 0;
    uint32_t messageExpiryInterval = 0;
};

// MQTT Metrics for monitoring
struct MqttMetrics {
    std::atomic<uint64_t> messagesReceived{0};
    std::atomic<uint64_t> messagesSent{0};
    std::atomic<uint64_t> bytesReceived{0};
    std::atomic<uint64_t> bytesSent{0};
    std::atomic<uint64_t> connectCount{0};
    std::atomic<uint64_t> disconnectCount{0};
    std::atomic<uint64_t> subscribeCount{0};
    std::atomic<uint64_t> publishCount{0};
    std::atomic<uint64_t> qos0Messages{0};
    std::atomic<uint64_t> qos1Messages{0};
    std::atomic<uint64_t> qos2Messages{0};
    std::atomic<uint64_t> rateLimitedMessages{0};
    std::chrono::steady_clock::time_point startTime;
    
    MqttMetrics() : startTime(std::chrono::steady_clock::now()) {}

    MqttMetrics(const MqttMetrics& other)
        : messagesReceived(other.messagesReceived.load())
        , messagesSent(other.messagesSent.load())
        , bytesReceived(other.bytesReceived.load())
        , bytesSent(other.bytesSent.load())
        , connectCount(other.connectCount.load())
        , disconnectCount(other.disconnectCount.load())
        , subscribeCount(other.subscribeCount.load())
        , publishCount(other.publishCount.load())
        , qos0Messages(other.qos0Messages.load())
        , qos1Messages(other.qos1Messages.load())
        , qos2Messages(other.qos2Messages.load())
        , rateLimitedMessages(other.rateLimitedMessages.load())
        , startTime(other.startTime) {}

    MqttMetrics& operator=(const MqttMetrics& other) {
        if (this == &other) {
          return *this;
        }
        messagesReceived.store(other.messagesReceived.load());
        messagesSent.store(other.messagesSent.load());
        bytesReceived.store(other.bytesReceived.load());
        bytesSent.store(other.bytesSent.load());
        connectCount.store(other.connectCount.load());
        disconnectCount.store(other.disconnectCount.load());
        subscribeCount.store(other.subscribeCount.load());
        publishCount.store(other.publishCount.load());
        qos0Messages.store(other.qos0Messages.load());
        qos1Messages.store(other.qos1Messages.load());
        qos2Messages.store(other.qos2Messages.load());
        rateLimitedMessages.store(other.rateLimitedMessages.load());
        startTime = other.startTime;
        return *this;
    }
    
    /**
     * @brief Reset the modification detection flag.
     * @details Calls: std::chrono::steady_clock::now().
     */
    void reset() {
        messagesReceived = 0;
        messagesSent = 0;
        bytesReceived = 0;
        bytesSent = 0;
        connectCount = 0;
        disconnectCount = 0;
        subscribeCount = 0;
        publishCount = 0;
        qos0Messages = 0;
        qos1Messages = 0;
        qos2Messages = 0;
        rateLimitedMessages = 0;
        startTime = std::chrono::steady_clock::now();
    }
};

// Rate limiter configuration
struct MqttRateLimitConfig {
    uint32_t maxMessagesPerSecond = 1000;  // Per client
    uint32_t maxBytesPerSecond = 1048576;   // 1MB per second per client
    uint32_t burstSize = 100;                // Allow bursts
    bool enabled = true;
};

// Connection retry configuration
struct MqttRetryConfig {
    uint32_t maxRetries = 3;
    uint32_t initialRetryDelayMs = 1000;
    uint32_t maxRetryDelayMs = 60000;
    float backoffMultiplier = 2.0f;
    bool exponentialBackoff = true;
};

// QoS 2 state tracking
enum class Qos2State {
    WaitingForPubRec,
    WaitingForPubComp
};

struct Qos2Message {
    uint16_t packetId;
    std::string topic = {};
    std::string payload;
    Qos2State state;
    std::chrono::steady_clock::time_point timestamp;
};

// Session state for persistent sessions
struct MqttSessionState {
    std::string clientId;
    std::map<std::string, uint8_t> subscriptions; // topic -> QoS
    std::map<uint16_t, Qos2Message> qos2Messages;
    bool cleanSession = true;
    std::string willTopic;
    std::string willMessage;
    uint8_t willQos = 0;
    bool willRetain = false;
};

class MqttSession : public std::enable_shared_from_this<MqttSession> {
public:
    enum class TransportType {
        TCP,
        WebSocket
    };
    
    explicit MqttSession(asio::ip::tcp::socket socket, uint8_t protocolVersion = 4, 
                        TransportType transport = TransportType::TCP);
    ~MqttSession() noexcept;

    /**
     * @brief Start.
     */
    void start();
    /**
     * @brief Stop.
     */
    void stop();
    
    // WebSocket support
    /**
     * @brief Set Web Socket.
     * @param[in] ws Input parameter.
     */
    void setWebSocket(std::shared_ptr<websocket::stream<asio::ip::tcp::socket>> ws);
    bool isWebSocketTransport() const { return transportType_ == TransportType::WebSocket; }

    // MQTT packet handlers
    /**
     * @brief Handle Connect.
     */
    void handleConnect();
    /**
     * @brief Handle Publish.
     * @param[in] topic Input parameter.
     * @param[in] payload Input parameter.
     * @param[in] qos Input parameter.
     * @param[in] packetId Input parameter.
     */
    void handlePublish(const std::string& topic, const std::string& payload, uint8_t qos, uint16_t packetId);
    /**
     * @brief Handle Pub Rec.
     * @param[in] packetId Input parameter.
     */
    void handlePubRec(uint16_t packetId);
    /**
     * @brief Handle Pub Rel.
     * @param[in] packetId Input parameter.
     */
    void handlePubRel(uint16_t packetId);
    /**
     * @brief Handle Pub Comp.
     * @param[in] packetId Input parameter.
     */
    void handlePubComp(uint16_t packetId);
    /**
     * @brief Handle Subscribe.
     * @param[in] topic Input parameter.
     * @param[in] qos Input parameter.
     * @param[in] packetId Input parameter.
     */
    void handleSubscribe(const std::string& topic, uint8_t qos, uint16_t packetId);
    /**
     * @brief Handle Unsubscribe.
     * @param[in] topic Input parameter.
     */
    void handleUnsubscribe(const std::string& topic);
    /**
     * @brief Handle Ping Req.
     */
    void handlePingReq();
    /**
     * @brief Handle Disconnect.
     */
    void handleDisconnect();

    // Send MQTT packets
    /**
     * @brief Send Conn Ack.
     * @param[in] sessionPresent Input parameter.
     * @param[in] returnCode Input parameter.
     */
    void sendConnAck(bool sessionPresent, uint8_t returnCode);
    void sendPublish(const std::string& topic, const std::string& payload, uint8_t qos, bool retain = false);
    /**
     * @brief Send Pub Ack.
     * @param[in] packetId Input parameter.
     */
    void sendPubAck(uint16_t packetId);
    /**
     * @brief Send Pub Rec.
     * @param[in] packetId Input parameter.
     */
    void sendPubRec(uint16_t packetId);
    /**
     * @brief Send Pub Rel.
     * @param[in] packetId Input parameter.
     */
    void sendPubRel(uint16_t packetId);
    /**
     * @brief Send Pub Comp.
     * @param[in] packetId Input parameter.
     */
    void sendPubComp(uint16_t packetId);
    /**
     * @brief Send Sub Ack.
     * @param[in] packetId Input parameter.
     * @param[in] returnCodes Input parameter.
     */
    void sendSubAck(uint16_t packetId, const std::vector<uint8_t>& returnCodes);
    /**
     * @brief Send Ping Resp.
     */
    void sendPingResp();

    /**
     * @brief Set Properties.
     * @param[in] props Input parameter.
     * @details Implements setProperties without additional internal calls.
     */
    void setProperties(const MqttProperties& props) { properties_ = props; }
    const MqttProperties& getProperties() const { return properties_; }
    
    // Session management
    std::string getClientId() const { return sessionState_.clientId; }
    /**
     * @brief Restore Session.
     * @param[in] state Input parameter.
     */
    void restoreSession(const MqttSessionState& state);
    MqttSessionState getSessionState() const { return sessionState_; }
    
    // Rate limiting
    /**
     * @brief Set Rate Limit Config.
     * @param[in] config Input parameter.
     * @details Implements setRateLimitConfig without additional internal calls.
     */
    void setRateLimitConfig(const MqttRateLimitConfig& config) { rateLimitConfig_ = config; }
    /**
     * @brief Check whether a user exceeds the current rate limit.
     * @param[in] messageSize Input parameter.
     * @return True when the user remains within the configured limit.
     */
    bool checkRateLimit(size_t messageSize);
    
    // Metrics
    const MqttMetrics& getMetrics() const { return metrics_; }
    /**
     * @brief Reset Metrics.
     * @details Calls: reset().
     */
    void resetMetrics() { metrics_.reset(); }

private:
    /**
     * @brief Do Read.
     */
    void doRead();
    /**
     * @brief Do Write.
     */
    void doWrite();
    /**
     * @brief Do Web Socket Read.
     */
    void doWebSocketRead();
    /**
     * @brief Do Web Socket Write.
     */
    void doWebSocketWrite();
    /**
     * @brief Process Qos2 Timeouts.
     */
    void processQos2Timeouts();
    /**
     * @brief Trigger Will Message.
     */
    void triggerWillMessage() const;
    /**
     * @brief Update Rate Limiter.
     */
    void updateRateLimiter();
    
    asio::ip::tcp::socket socket_;
    std::shared_ptr<websocket::stream<asio::ip::tcp::socket>> wsStream_;
    boost::beast::flat_buffer wsReadBuffer_;
    TransportType transportType_;
    std::array<char, 8192> buffer_;
    bool isConnected_;
    uint16_t packetIdCounter_;
    std::deque<std::vector<uint8_t>> writeQueue_;
    uint8_t protocolVersion_; // 4 for MQTT 3.1.1, 5 for MQTT 5.0
    
    // Session state
    MqttSessionState sessionState_;
    
    // MQTT 5.0 properties
    MqttProperties properties_;
    
    // QoS 2 message tracking
    std::map<uint16_t, Qos2Message> outgoingQos2_;
    std::map<uint16_t, Qos2Message> incomingQos2_;
    
    // Keepalive timer
    asio::steady_timer keepaliveTimer_;
    std::chrono::seconds keepaliveInterval_;
    
    // Rate limiting
    MqttRateLimitConfig rateLimitConfig_;
    std::chrono::steady_clock::time_point lastRateLimitReset_;
    uint32_t messagesThisSecond_;
    uint64_t bytesThisSecond_;
    
    // Metrics
    MqttMetrics metrics_;
};

// Retained message storage
struct RetainedMessage {
    std::string topic;
    std::string payload;
    uint8_t qos;
    std::chrono::steady_clock::time_point timestamp;
};

class MqttBroker {
public:
    /**
     * @brief Get Instance.
     * @return Return value.
     */
    static MqttBroker& getInstance();
    
    /**
     * @brief Subscribe.
     * @param[in] topic Input parameter.
     * @param[in] session Input parameter.
     * @param[in] qos Input parameter.
     */
    void subscribe(const std::string& topic, std::shared_ptr<MqttSession> session, uint8_t qos);
    /**
     * @brief Unsubscribe.
     * @param[in] topic Input parameter.
     * @param[in] session Input parameter.
     */
    void unsubscribe(const std::string& topic, std::shared_ptr<MqttSession> session);
    void publish(const std::string& topic, const std::string& payload, uint8_t qos, bool retain = false);
    
    /**
     * @brief Subscribe Shared.
     * @param[in] shareName Input parameter.
     * @param[in] topic Input parameter.
     * @param[in] session Input parameter.
     * @param[in] qos Input parameter.
     */
    void subscribeShared(const std::string& shareName, const std::string& topic, 
                        std::shared_ptr<MqttSession> session, uint8_t qos);
    
    // Session persistence
    /**
     * @brief Save Session.
     * @param[in] clientId Input parameter.
     * @param[in] state Input parameter.
     */
    void saveSession(const std::string& clientId, const MqttSessionState& state);
    /**
     * @brief Load Session.
     * @param[in] clientId Input parameter.
     * @param[in,out] state Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool loadSession(const std::string& clientId, MqttSessionState& state);
    /**
     * @brief Delete Session.
     * @param[in] clientId Input parameter.
     */
    void deleteSession(const std::string& clientId);
    
    // Retained messages
    /**
     * @brief Set Retained Message.
     * @param[in] topic Input parameter.
     * @param[in] payload Input parameter.
     * @param[in] qos Input parameter.
     */
    void setRetainedMessage(const std::string& topic, const std::string& payload, uint8_t qos);
    /**
     * @brief Get Retained Messages.
     * @param[in] topicFilter Input parameter.
     * @return Return value.
     */
    std::vector<RetainedMessage> getRetainedMessages(const std::string& topicFilter);
    /**
     * @brief Clear Retained Message.
     * @param[in] topic Input parameter.
     */
    void clearRetainedMessage(const std::string& topic);
    
    // Metrics & monitoring
    /**
     * @brief Get Aggregated Metrics.
     * @return Return value.
     */
    MqttMetrics getAggregatedMetrics();
    /**
     * @brief Set Rate Limit Config.
     * @param[in] config Input parameter.
     * @details Implements setRateLimitConfig without additional internal calls.
     */
    void setRateLimitConfig(const MqttRateLimitConfig& config) { rateLimitConfig_ = config; }
    const MqttRateLimitConfig& getRateLimitConfig() const { return rateLimitConfig_; }
    
    // Connection retry
    /**
     * @brief Set Retry Config.
     * @param[in] config Input parameter.
     * @details Implements setRetryConfig without additional internal calls.
     */
    void setRetryConfig(const MqttRetryConfig& config) { retryConfig_ = config; }
    const MqttRetryConfig& getRetryConfig() const { return retryConfig_; }

    /**
     * @brief Register Active Session.
     * @param[in] session Input parameter.
     */
    void registerActiveSession(std::weak_ptr<MqttSession> session);
    /**
     * @brief Unregister Active Session.
     * @param[in,out] raw_ptr Input/output parameter.
     */
    void unregisterActiveSession(MqttSession* raw_ptr);
    
private:
    MqttBroker() = default;
    /**
     * @brief Topic Matches.
     * @param[in] filter Input parameter.
     * @param[in] topic Input parameter.
     * @return True when the operation succeeds.
     */
    bool topicMatches(const std::string& filter, const std::string& topic);
    
    std::map<std::string, std::vector<std::weak_ptr<MqttSession>>> subscriptions_;
    std::map<std::string, std::map<std::string, std::vector<std::weak_ptr<MqttSession>>>> sharedSubscriptions_; // shareName -> topic -> sessions
    std::map<std::string, MqttSessionState> persistentSessions_;
    std::map<std::string, RetainedMessage> retainedMessages_;
    MqttRateLimitConfig rateLimitConfig_;
    MqttRetryConfig retryConfig_;
    std::mutex mutex_;
    // Thread-safe round-robin index for shared subscriptions
    std::atomic<size_t> sharedSubscriptionRoundRobin_{0};
    // All currently-connected sessions (weak refs; expired entries are cleaned up lazily)
    std::vector<std::weak_ptr<MqttSession>> activeSessions_;
};

#endif // THEMIS_ENABLE_MQTT
