#pragma once

#ifdef THEMIS_ENABLE_MQTT

#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace asio = boost::asio;

class MqttSession : public std::enable_shared_from_this<MqttSession> {
public:
    explicit MqttSession(asio::ip::tcp::socket socket);
    ~MqttSession();

    void start();
    void stop();

    // MQTT packet handlers
    void handleConnect();
    void handlePublish(const std::string& topic, const std::string& payload);
    void handleSubscribe(const std::string& topic);
    void handleUnsubscribe(const std::string& topic);
    void handlePingReq();
    void handleDisconnect();

    // Send MQTT packets
    void sendConnAck(bool sessionPresent, uint8_t returnCode);
    void sendPublish(const std::string& topic, const std::string& payload, uint8_t qos);
    void sendSubAck(uint16_t packetId, const std::vector<uint8_t>& returnCodes);
    void sendPingResp();

private:
    void doRead();
    void doWrite();
    
    asio::ip::tcp::socket socket_;
    std::array<char, 8192> buffer_;
    std::string clientId_;
    bool isConnected_;
};

class MqttBroker {
public:
    static MqttBroker& getInstance();
    
    void subscribe(const std::string& topic, std::shared_ptr<MqttSession> session);
    void unsubscribe(const std::string& topic, std::shared_ptr<MqttSession> session);
    void publish(const std::string& topic, const std::string& payload, uint8_t qos);
    
private:
    MqttBroker() = default;
    std::map<std::string, std::vector<std::weak_ptr<MqttSession>>> subscriptions_;
    std::mutex mutex_;
};

#endif // THEMIS_ENABLE_MQTT
