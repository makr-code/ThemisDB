#pragma once

#ifdef THEMIS_ENABLE_MQTT

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <deque>
#include <array>
#include <unordered_map>
#include <chrono>

namespace asio = boost::asio;

// MQTT 5.0 Property types
struct MqttProperties {
    std::string contentType;
    std::string responseTopic;
    std::vector<std::pair<std::string, std::string>> userProperties;
    uint16_t topicAlias = 0;
    uint32_t messageExpiryInterval = 0;
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
    explicit MqttSession(asio::ip::tcp::socket socket, uint8_t protocolVersion = 4);
    ~MqttSession();

    void start();
    void stop();

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

private:
    void doRead();
    void doWrite();
    void processQos2Timeouts();
    void triggerWillMessage();
    
    asio::ip::tcp::socket socket_;
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
    
private:
    MqttBroker() = default;
    bool topicMatches(const std::string& filter, const std::string& topic);
    
    std::map<std::string, std::vector<std::weak_ptr<MqttSession>>> subscriptions_;
    std::map<std::string, std::map<std::string, std::vector<std::weak_ptr<MqttSession>>>> sharedSubscriptions_; // shareName -> topic -> sessions
    std::map<std::string, MqttSessionState> persistentSessions_;
    std::map<std::string, RetainedMessage> retainedMessages_;
    std::mutex mutex_;
};

#endif // THEMIS_ENABLE_MQTT
