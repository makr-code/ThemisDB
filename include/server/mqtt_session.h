/**
 * @file mqtt_session.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
        if (this == &other) return *this;
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
    std::string topic;
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

    void start();
    void stop();
    
    // WebSocket support
    void setWebSocket(std::shared_ptr<websocket::stream<asio::ip::tcp::socket>> ws);
    bool isWebSocketTransport() const { return transportType_ == TransportType::WebSocket; }

    // MQTT packet handlers
    void handleConnect();
    void handlePublish(const std::string& topic, const std::string& payload, uint8_t qos, uint16_t packetId);
    void handlePubRec(uint16_t packetId);
    void handlePubRel(uint16_t packetId);
    void handlePubComp(uint16_t packetId);
    void handleSubscribe(const std::string& topic, uint8_t qos, uint16_t packetId);
    void handleUnsubscribe(const std::string& topic);
    void handlePingReq();
    void handleDisconnect();

    // Send MQTT packets
    void sendConnAck(bool sessionPresent, uint8_t returnCode);
    void sendPublish(const std::string& topic, const std::string& payload, uint8_t qos, bool retain = false);
    void sendPubAck(uint16_t packetId);
    void sendPubRec(uint16_t packetId);
    void sendPubRel(uint16_t packetId);
    void sendPubComp(uint16_t packetId);
    void sendSubAck(uint16_t packetId, const std::vector<uint8_t>& returnCodes);
    void sendPingResp();

    // MQTT 5.0 features
    void setProperties(const MqttProperties& props) { properties_ = props; }
    const MqttProperties& getProperties() const { return properties_; }
    
    // Session management
    std::string getClientId() const { return sessionState_.clientId; }
    void restoreSession(const MqttSessionState& state);
    MqttSessionState getSessionState() const { return sessionState_; }
    
    // Rate limiting
    void setRateLimitConfig(const MqttRateLimitConfig& config) { rateLimitConfig_ = config; }
    bool checkRateLimit(size_t messageSize);
    
    // Metrics
    const MqttMetrics& getMetrics() const { return metrics_; }
    void resetMetrics() { metrics_.reset(); }

private:
    void doRead();
    void doWrite();
    void doWebSocketRead();
    void doWebSocketWrite();
    void processQos2Timeouts();
    void triggerWillMessage() const;
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
    static MqttBroker& getInstance();
    
    void subscribe(const std::string& topic, std::shared_ptr<MqttSession> session, uint8_t qos);
    void unsubscribe(const std::string& topic, std::shared_ptr<MqttSession> session);
    void publish(const std::string& topic, const std::string& payload, uint8_t qos, bool retain = false);
    
    // Shared subscriptions (MQTT 5.0)
    void subscribeShared(const std::string& shareName, const std::string& topic, 
                        std::shared_ptr<MqttSession> session, uint8_t qos);
    
    // Session persistence
    void saveSession(const std::string& clientId, const MqttSessionState& state);
    bool loadSession(const std::string& clientId, MqttSessionState& state);
    void deleteSession(const std::string& clientId);
    
    // Retained messages
    void setRetainedMessage(const std::string& topic, const std::string& payload, uint8_t qos);
    std::vector<RetainedMessage> getRetainedMessages(const std::string& topicFilter);
    void clearRetainedMessage(const std::string& topic);
    
    // Metrics & monitoring
    MqttMetrics getAggregatedMetrics();
    void setRateLimitConfig(const MqttRateLimitConfig& config) { rateLimitConfig_ = config; }
    const MqttRateLimitConfig& getRateLimitConfig() const { return rateLimitConfig_; }
    
    // Connection retry
    void setRetryConfig(const MqttRetryConfig& config) { retryConfig_ = config; }
    const MqttRetryConfig& getRetryConfig() const { return retryConfig_; }

    // Active-session registry — called by MqttSession on connect/disconnect
    void registerActiveSession(std::weak_ptr<MqttSession> session);
    void unregisterActiveSession(MqttSession* raw_ptr);
    
private:
    MqttBroker() = default;
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
