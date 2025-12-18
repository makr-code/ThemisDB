#ifdef THEMIS_ENABLE_MQTT

#include "server/mqtt_session.h"
#include <boost/beast/core.hpp>
#include <iostream>
#include <algorithm>

MqttSession::MqttSession(asio::ip::tcp::socket socket)
    : socket_(std::move(socket))
    , isConnected_(false)
    , packetIdCounter_(1) {
}

MqttSession::~MqttSession() {
    stop();
}

void MqttSession::start() {
    doRead();
}

void MqttSession::stop() {
    if (isConnected_) {
        boost::beast::error_code ec;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
        isConnected_ = false;
    }
}

void MqttSession::handleConnect() {
    // MQTT CONNECT packet handler - base implementation
    // Parse CONNECT packet for protocol name, version, flags
    
    isConnected_ = true;
    sendConnAck(false, 0); // Session not present, connection accepted
}

void MqttSession::handlePublish(const std::string& topic, const std::string& payload) {
    // MQTT PUBLISH packet handler - base implementation
    // Validate topic and publish to broker
    
    if (!isConnected_) return;
    
    MqttBroker::getInstance().publish(topic, payload, 0);
}

void MqttSession::handleSubscribe(const std::string& topic) {
    // MQTT SUBSCRIBE packet handler - base implementation
    if (!isConnected_) return;
    
    MqttBroker::getInstance().subscribe(topic, shared_from_this());
    
    // Send SUBACK
    std::vector<uint8_t> returnCodes = {0}; // QoS 0 granted
    sendSubAck(packetIdCounter_++, returnCodes);
}

void MqttSession::handleUnsubscribe(const std::string& topic) {
    // MQTT UNSUBSCRIBE packet handler
    if (!isConnected_) return;
    
    MqttBroker::getInstance().unsubscribe(topic, shared_from_this());
}

void MqttSession::handlePingReq() {
    // MQTT PINGREQ packet handler
    sendPingResp();
}

void MqttSession::handleDisconnect() {
    // MQTT DISCONNECT packet handler
    stop();
}

void MqttSession::sendConnAck(bool sessionPresent, uint8_t returnCode) {
    // Build MQTT CONNACK packet
    // Format: [Type(0x20), RemainingLength(2), SessionPresent, ReturnCode]
    std::vector<uint8_t> packet;
    packet.push_back(0x20); // CONNACK packet type
    packet.push_back(2);    // Remaining length
    packet.push_back(sessionPresent ? 1 : 0);
    packet.push_back(returnCode);
    
    writeQueue_.push_back(std::move(packet));
    doWrite();
}

void MqttSession::sendPublish(const std::string& topic, const std::string& payload, uint8_t qos) {
    // Build MQTT PUBLISH packet
    // Format: [Type(0x30 + flags), RemainingLength, TopicLength(2), Topic, Payload]
    std::vector<uint8_t> packet;
    packet.push_back(0x30 | (qos << 1)); // PUBLISH with QoS
    
    // Calculate remaining length
    uint32_t remainingLength = 2 + topic.size() + payload.size();
    if (qos > 0) remainingLength += 2; // Packet ID for QoS 1 or 2
    
    // Encode remaining length (variable length)
    do {
        uint8_t encodedByte = remainingLength % 128;
        remainingLength = remainingLength / 128;
        if (remainingLength > 0) {
            encodedByte = encodedByte | 128;
        }
        packet.push_back(encodedByte);
    } while (remainingLength > 0);
    
    // Topic length (2 bytes, big-endian)
    uint16_t topicLen = topic.size();
    packet.push_back((topicLen >> 8) & 0xFF);
    packet.push_back(topicLen & 0xFF);
    
    // Topic
    packet.insert(packet.end(), topic.begin(), topic.end());
    
    // Packet ID for QoS > 0
    if (qos > 0) {
        uint16_t packetId = packetIdCounter_++;
        packet.push_back((packetId >> 8) & 0xFF);
        packet.push_back(packetId & 0xFF);
    }
    
    // Payload
    packet.insert(packet.end(), payload.begin(), payload.end());
    
    writeQueue_.push_back(std::move(packet));
    doWrite();
}

void MqttSession::sendSubAck(uint16_t packetId, const std::vector<uint8_t>& returnCodes) {
    // Build MQTT SUBACK packet
    // Format: [Type(0x90), RemainingLength, PacketId(2), ReturnCodes...]
    std::vector<uint8_t> packet;
    packet.push_back(0x90); // SUBACK packet type
    packet.push_back(2 + returnCodes.size()); // Remaining length
    
    // Packet ID (2 bytes, big-endian)
    packet.push_back((packetId >> 8) & 0xFF);
    packet.push_back(packetId & 0xFF);
    
    // Return codes
    packet.insert(packet.end(), returnCodes.begin(), returnCodes.end());
    
    writeQueue_.push_back(std::move(packet));
    doWrite();
}

void MqttSession::sendPingResp() {
    // Build MQTT PINGRESP packet
    // Format: [Type(0xD0), Length(0)]
    std::vector<uint8_t> packet = {0xD0, 0x00};
    
    writeQueue_.push_back(std::move(packet));
    doWrite();
}

void MqttSession::doRead() {
    auto self = shared_from_this();
    
    // Read MQTT fixed header (minimum 2 bytes: packet type + remaining length byte)
    socket_.async_read_some(asio::buffer(buffer_),
        [this, self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                stop();
                return;
            }
            
            if (bytes_transferred < 2) {
                doRead(); // Need more data
                return;
            }
            
            // Parse fixed header
            uint8_t packetType = buffer_[0];
            uint8_t messageType = (packetType >> 4) & 0x0F;
            
            // Decode remaining length (variable length encoding)
            size_t multiplier = 1;
            size_t remainingLength = 0;
            size_t headerSize = 1;
            
            for (size_t i = 1; i < bytes_transferred && i < 5; ++i) {
                headerSize++;
                uint8_t encodedByte = buffer_[i];
                remainingLength += (encodedByte & 127) * multiplier;
                multiplier *= 128;
                
                if ((encodedByte & 128) == 0) {
                    break; // Last byte of remaining length
                }
            }
            
            // Dispatch based on message type
            switch (messageType) {
                case 1: // CONNECT
                    handleConnect();
                    break;
                case 3: // PUBLISH
                    if (bytes_transferred >= headerSize + 2) {
                        // Extract topic name
                        uint16_t topicLen = (buffer_[headerSize] << 8) | buffer_[headerSize + 1];
                        std::string topic(buffer_.data() + headerSize + 2, topicLen);
                        std::string payload(buffer_.data() + headerSize + 2 + topicLen, 
                                          remainingLength - 2 - topicLen);
                        handlePublish(topic, payload);
                    }
                    break;
                case 8: // SUBSCRIBE
                    if (bytes_transferred >= headerSize + 4) {
                        // Skip packet ID (2 bytes)
                        uint16_t topicLen = (buffer_[headerSize + 2] << 8) | buffer_[headerSize + 3];
                        std::string topic(buffer_.data() + headerSize + 4, topicLen);
                        handleSubscribe(topic);
                    }
                    break;
                case 10: // UNSUBSCRIBE
                    if (bytes_transferred >= headerSize + 4) {
                        uint16_t topicLen = (buffer_[headerSize + 2] << 8) | buffer_[headerSize + 3];
                        std::string topic(buffer_.data() + headerSize + 4, topicLen);
                        handleUnsubscribe(topic);
                    }
                    break;
                case 12: // PINGREQ
                    handlePingReq();
                    break;
                case 14: // DISCONNECT
                    handleDisconnect();
                    return;
                default:
                    break;
            }
            
            doRead(); // Continue reading
        });
}

void MqttSession::doWrite() {
    if (writeQueue_.empty()) {
        return;
    }
    
    auto self = shared_from_this();
    
    asio::async_write(socket_, asio::buffer(writeQueue_.front()),
        [this, self](boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
            if (!ec) {
                writeQueue_.pop_front();
                if (!writeQueue_.empty()) {
                    doWrite();
                }
            } else {
                stop();
            }
        });
}

// MqttBroker implementation

MqttBroker& MqttBroker::getInstance() {
    static MqttBroker instance;
    return instance;
}

void MqttBroker::subscribe(const std::string& topic, std::shared_ptr<MqttSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[topic].push_back(session);
}

void MqttBroker::unsubscribe(const std::string& topic, std::shared_ptr<MqttSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscriptions_.find(topic);
    if (it != subscriptions_.end()) {
        auto& sessions = it->second;
        sessions.erase(
            std::remove_if(sessions.begin(), sessions.end(),
                [](const std::weak_ptr<MqttSession>& wp) { return wp.expired(); }),
            sessions.end()
        );
    }
}

bool MqttBroker::topicMatches(const std::string& filter, const std::string& topic) {
    // MQTT topic matching with wildcards
    // + matches single level, # matches multiple levels
    
    size_t filterPos = 0, topicPos = 0;
    
    while (filterPos < filter.size() && topicPos < topic.size()) {
        if (filter[filterPos] == '+') {
            // Single-level wildcard: match until next '/' or end
            while (topicPos < topic.size() && topic[topicPos] != '/') {
                topicPos++;
            }
            filterPos++;
        } else if (filter[filterPos] == '#') {
            // Multi-level wildcard: matches rest of topic
            return filterPos == filter.size() - 1; // # must be last char
        } else if (filter[filterPos] == topic[topicPos]) {
            filterPos++;
            topicPos++;
        } else {
            return false;
        }
    }
    
    return filterPos == filter.size() && topicPos == topic.size();
}

void MqttBroker::publish(const std::string& topic, const std::string& payload, uint8_t qos) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Match topic against all subscription filters
    for (const auto& [filter, sessions] : subscriptions_) {
        if (topicMatches(filter, topic)) {
            for (auto& sessionWeak : sessions) {
                if (auto session = sessionWeak.lock()) {
                    session->sendPublish(topic, payload, qos);
                }
            }
        }
    }
}

#endif // THEMIS_ENABLE_MQTT
