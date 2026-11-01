// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "network/wire_protocol_server.h"
#include "query/query_engine.h"

#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using themis::network::AuditEventType;
using themis::network::NetworkAuditLog;
using themis::network::WireProtocolServer;
using themis::network::tcp;

uint16_t reservePort() {
    boost::asio::io_context io_context;
    tcp::acceptor acceptor(
        io_context, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    return acceptor.local_endpoint().port();
}

template <typename Predicate>
bool waitFor(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

std::vector<uint8_t> makeAuthRequest(const std::string& token) {
    const std::string payload =
        nlohmann::json{{"token", token}, {"username", "audit-test"}}.dump();
    const auto size = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> frame{
        'T', 'M', 'D', 'B', 1, 0x04, 0x00, 0x04,
        static_cast<uint8_t>((size >> 24) & 0xFF),
        static_cast<uint8_t>((size >> 16) & 0xFF),
        static_cast<uint8_t>((size >> 8) & 0xFF),
        static_cast<uint8_t>(size & 0xFF),
    };
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

class GeoQueryBridgeReset {
public:
    ~GeoQueryBridgeReset() {
        themis::network::setNetworkGeoQueryFn(nullptr);
    }
};

class ClientCloseGuard {
public:
    ClientCloseGuard(tcp::socket& socket, NetworkAuditLog& audit_log)
        : socket_(socket), audit_log_(audit_log) {}

    ~ClientCloseGuard() {
        boost::system::error_code ignored;
        socket_.close(ignored);
        waitFor([this] {
            return audit_log_.getEventsByType(
                       AuditEventType::CONNECTION_CLOSE).size() == 1;
        });
    }

private:
    tcp::socket& socket_;
    NetworkAuditLog& audit_log_;
};

} // namespace

TEST(NetworkAuditLogIntegration, CallbackFailureDoesNotEscapeRecording) {
    NetworkAuditLog audit_log;
    audit_log.setEventCallback([](const themis::network::AuditEvent&) {
        throw std::runtime_error("sink unavailable");
    });

    EXPECT_NO_THROW(audit_log.recordConnectionOpen("127.0.0.1", 42));
    EXPECT_EQ(audit_log.getStats().connection_opens, 1u);
}

TEST(NetworkAuditLogIntegration, WireServerRecordsLifecycleAuthAndRateLimit) {
    GeoQueryBridgeReset geo_query_bridge_reset;
    themis::network::setNetworkGeoQueryFn(
        [](const std::string&, double, double, double, int) {
            return nlohmann::json::array();
        });

    WireProtocolServer::Config config;
    config.host = "127.0.0.1";
    config.port = reservePort();
    config.num_io_threads = 1;
    config.num_worker_threads = 1;
    config.auth_token = "expected-audit-test-token";
    config.max_requests_per_second = 1;
    config.max_requests_per_minute = 10;

    auto query_engine = themis::query::QueryEngine::createDefault();
    WireProtocolServer server(config, nullptr, nullptr, nullptr, nullptr,
                              nullptr, nullptr, nullptr, nullptr, query_engine);
    auto& audit_log = server.getAuditLog();

    server.start();
    ASSERT_TRUE(waitFor([&server] { return server.isRunning(); }));

    boost::asio::io_context client_io;
    tcp::socket accepted_client(client_io);
    accepted_client.connect(
        tcp::endpoint(boost::asio::ip::address_v4::loopback(), config.port));
    ClientCloseGuard accepted_client_close_guard(accepted_client, audit_log);
    ASSERT_TRUE(waitFor([&audit_log] {
        return audit_log.getEventsByType(AuditEventType::CONNECTION_OPEN).size() == 1;
    }));

    std::vector<tcp::socket> rate_limited_clients;
    for (int attempt = 0; attempt < 3; ++attempt) {
        rate_limited_clients.emplace_back(client_io);
        boost::system::error_code ec;
        rate_limited_clients.back().connect(
            tcp::endpoint(boost::asio::ip::address_v4::loopback(), config.port),
            ec);
        if (ec) {
            rate_limited_clients.pop_back();
        }
    }
    ASSERT_TRUE(waitFor([&audit_log] {
        return audit_log.getEventsByType(AuditEventType::RATE_LIMITED).size() >= 1;
    }));

    const auto auth_frame = makeAuthRequest("incorrect-audit-test-token");
    boost::asio::write(accepted_client, boost::asio::buffer(auth_frame));
    ASSERT_TRUE(waitFor([&audit_log] {
        return audit_log.getEventsByType(AuditEventType::AUTH_FAILURE).size() == 1;
    }));

    const auto auth_events =
        audit_log.getEventsByType(AuditEventType::AUTH_FAILURE);
    ASSERT_EQ(auth_events.size(), 1);
    EXPECT_EQ(auth_events.front().principal, "");
    EXPECT_EQ(auth_events.front().token_hash,
              themis::network::NetworkAuditLog::truncatedSha256Hex(
                  "incorrect-audit-test-token"));

    const auto successful_auth_frame =
        makeAuthRequest("expected-audit-test-token");
    boost::asio::write(
        accepted_client, boost::asio::buffer(successful_auth_frame));
    ASSERT_TRUE(waitFor([&audit_log] {
        return audit_log.getEventsByType(AuditEventType::AUTH_SUCCESS).size() == 1;
    }));
    const auto success_events =
        audit_log.getEventsByType(AuditEventType::AUTH_SUCCESS);
    ASSERT_EQ(success_events.size(), 1);
    EXPECT_EQ(success_events.front().principal, "audit-test");
    EXPECT_EQ(success_events.front().token_hash,
              NetworkAuditLog::truncatedSha256Hex(
                  "expected-audit-test-token"));

    boost::system::error_code ignored;
    accepted_client.close(ignored);
    ASSERT_TRUE(waitFor([&audit_log] {
        return audit_log.getEventsByType(AuditEventType::CONNECTION_CLOSE).size() == 1;
    }));

    server.stop();
}
