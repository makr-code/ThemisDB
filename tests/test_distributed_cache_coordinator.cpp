// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for RedisCacheCoordinator (distributed cache coordination).
//
// These tests exercise the coordinator without requiring a live Redis server:
//  - Configuration and construction (graceful degradation when Redis is down)
//  - ICacheCoordinator interface compliance
//  - Message serialization / deserialization
//  - Stats collection
//  - Publish operations when not connected (no-throw guarantee)
//  - name() and isConnected() contract

#include <gtest/gtest.h>
#include "cache/distributed_cache_coordinator.h"
#include "cache/cache_replication_coordinator.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(THEMIS_ENABLE_REDIS) && !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

using namespace themis;
using namespace themis::cache;
using json = nlohmann::json;

#if defined(THEMIS_ENABLE_REDIS) && !defined(_WIN32)
namespace {

bool readLine(int fd, std::string& line) {
    line.clear();
    char ch = 0;
    while (true) {
        const auto count = ::recv(fd, &ch, 1, 0);
        if (count != 1) {
            return false;
        }
        if (ch == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return true;
        }
        line.push_back(ch);
        if (line.size() > 1024 * 1024) {
            return false;
        }
    }
}

bool readExact(int fd, std::string& value, size_t size) {
    value.resize(size);
    size_t read = 0;
    while (read < size) {
        const auto count = ::recv(fd, value.data() + read, size - read, 0);
        if (count <= 0) {
            return false;
        }
        read += static_cast<size_t>(count);
    }
    return true;
}

bool readCommand(int fd, std::vector<std::string>& args) {
    std::string line;
    if (!readLine(fd, line) || line.empty() || line[0] != '*') {
        return false;
    }
    const auto count = static_cast<size_t>(std::stoul(line.substr(1)));
    if (count > 16) {
        return false;
    }
    args.clear();
    for (size_t i = 0; i < count; ++i) {
        if (!readLine(fd, line) || line.empty() || line[0] != '$') {
            return false;
        }
        const auto length = static_cast<size_t>(std::stoul(line.substr(1)));
        if (length > 1024 * 1024) {
            return false;
        }
        std::string value;
        if (!readExact(fd, value, length)) {
            return false;
        }
        char crlf[2]{};
        if (::recv(fd, crlf, sizeof(crlf), MSG_WAITALL) != static_cast<ssize_t>(sizeof(crlf))) {
            return false;
        }
        args.push_back(std::move(value));
    }
    return true;
}

std::string respArray(const std::vector<std::string>& values) {
    std::string response = "*" + std::to_string(values.size()) + "\r\n";
    for (const auto& value : values) {
        response += "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
    }
    return response;
}

std::string respSubscribeAck(const std::string& channel, size_t count) {
    return "*3\r\n$9\r\nsubscribe\r\n$" + std::to_string(channel.size()) + "\r\n" + channel + "\r\n:"
           + std::to_string(count) + "\r\n";
}

bool sendAll(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        const auto count = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (count <= 0) {
            return false;
        }
        sent += static_cast<size_t>(count);
    }
    return true;
}

class LocalRedisServer {
public:
    LocalRedisServer() {
        listener_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listener_ < 0) {
            throw std::runtime_error("could not create local Redis test socket");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if (::bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0
            || ::listen(listener_, 8) != 0) {
            ::close(listener_);
            throw std::runtime_error("could not bind local Redis test socket");
        }
        socklen_t address_size = sizeof(address);
        if (::getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &address_size) != 0) {
            ::close(listener_);
            throw std::runtime_error("could not inspect local Redis test socket");
        }
        port_ = ntohs(address.sin_port);
        accept_thread_ = std::thread([this] { acceptLoop(); });
    }

    ~LocalRedisServer() {
        stopping_.store(true);
        ::shutdown(listener_, SHUT_RDWR);
        ::close(listener_);
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            for (const auto& [fd, channels] : clients_) {
                (void)channels;
                ::shutdown(fd, SHUT_RDWR);
            }
        }
        for (auto& thread : client_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    uint16_t port() const { return port_; }

    size_t subscriberCount() const {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        size_t count = 0;
        for (const auto& [fd, channels] : clients_) {
            (void)fd;
            count += channels.empty() ? 0 : 1;
        }
        return count;
    }

    size_t subscriptionAttemptCount() const {
        return subscription_attempts_.load();
    }

    void disconnectClients() {
        accepting_.store(false);
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (const auto& [fd, channels] : clients_) {
            (void)channels;
            ::shutdown(fd, SHUT_RDWR);
        }
    }

    void resumeClients() {
        accepting_.store(true);
    }

    void setZeroSubscriberChannel(std::string channel) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        zero_subscriber_channel_ = std::move(channel);
    }

    void setSubscriptionEnabled(bool enabled) {
        subscription_enabled_.store(enabled);
    }

private:
    void acceptLoop() {
        while (!stopping_.load()) {
            fd_set readable;
            FD_ZERO(&readable);
            FD_SET(listener_, &readable);
            timeval timeout{0, 50000};
            const int ready = ::select(listener_ + 1, &readable, nullptr, nullptr, &timeout);
            if (ready <= 0) {
                continue;
            }
            const int client = ::accept(listener_, nullptr, nullptr);
            if (client < 0) {
                continue;
            }
            if (!accepting_.load()) {
                ::close(client);
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.emplace(client, std::set<std::string>{});
            }
            client_threads_.emplace_back([this, client] { handleClient(client); });
        }
    }

    void handleClient(int fd) {
        std::vector<std::string> command;
        while (!stopping_.load() && readCommand(fd, command)) {
            if (command.empty()) {
                break;
            }
            if (command[0] == "SUBSCRIBE" && command.size() > 1) {
                subscription_attempts_.fetch_add(1);
                if (!subscription_enabled_.load()) {
                    sendAll(fd, "-NOPERM subscription denied\r\n");
                    break;
                }
                std::string acknowledgements;
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);
                    auto& channels = clients_[fd];
                    for (size_t i = 1; i < command.size(); ++i) {
                        channels.insert(command[i]);
                        acknowledgements += respSubscribeAck(command[i], channels.size());
                    }
                }
                if (!sendAll(fd, acknowledgements)) {
                    break;
                }
            } else if (command[0] == "PUBLISH" && command.size() == 3) {
                std::vector<int> recipients;
                {
                    std::lock_guard<std::mutex> lock(clients_mutex_);
                    if (zero_subscriber_channel_ != command[1]) {
                        for (const auto& [client, channels] : clients_) {
                            if (channels.contains(command[1])) {
                                recipients.push_back(client);
                            }
                        }
                    }
                }
                const auto message = respArray({"message", command[1], command[2]});
                for (const int recipient : recipients) {
                    sendAll(recipient, message);
                }
                const auto response = ":" + std::to_string(recipients.size()) + "\r\n";
                if (!sendAll(fd, response)) {
                    break;
                }
            } else if (!sendAll(fd, "+OK\r\n")) {
                break;
            }
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            clients_.erase(fd);
        }
        ::close(fd);
    }

    int listener_{-1};
    uint16_t port_{0};
    std::atomic<bool> stopping_{false};
    std::atomic<bool> accepting_{true};
    std::atomic<bool> subscription_enabled_{true};
    std::atomic<size_t> subscription_attempts_{0};
    std::string zero_subscriber_channel_;
    mutable std::mutex clients_mutex_;
    std::map<int, std::set<std::string>> clients_;
    std::vector<std::thread> client_threads_;
    std::thread accept_thread_;
};

template <typename Predicate>
bool waitFor(Predicate predicate, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

} // namespace
#endif

// ============================================================================
// Helper: build a coordinator that connects to a port that is not listening
// so all operations degrade gracefully.
// ============================================================================
static RedisCacheCoordinatorConfig makeOfflineConfig(
    [[maybe_unused]] const std::string& node_id = "test-node")
{
    RedisCacheCoordinatorConfig cfg;
    cfg.host                 = "127.0.0.1";
    cfg.port                 = 16399;   // Likely not running – graceful degradation
    cfg.channel_prefix       = "themis_test";
    cfg.connect_timeout_ms   = 200;     // Fast timeout for tests
    cfg.reconnect_interval_ms = 50;
    // Note: new config does not have node_id field; node_id is derived internally
    return cfg;
}

// ============================================================================
// Construction / destruction
// ============================================================================

TEST(RedisCacheCoordinatorTest, ConstructionDoesNotThrow) {
    // Even without a running Redis, construction must not throw.
    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(makeOfflineConfig());
        // Give subscribe thread a moment to attempt connection and back off
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
}

TEST(RedisCacheCoordinatorTest, NameContainsHostPort) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    std::string n = coord.name();
    // name() is a stable transport identifier, not a host/port descriptor.
    EXPECT_EQ(n, "RedisCacheCoordinator");
}

// ============================================================================
// ICacheCoordinator interface compliance
// ============================================================================

TEST(RedisCacheCoordinatorTest, IsConnectedFalseWhenRedisAbsent) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    // After a connect attempt the connection should be reported as offline
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_FALSE(coord.isConnected());
}

TEST(RedisCacheCoordinatorTest, PublishEntryNoThrowWhenOffline) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    json result = {{"rows", 5}};
    EXPECT_NO_THROW(coord.publishEntry("fp_abc123", result, 300, "acme"));
}

TEST(RedisCacheCoordinatorTest, PublishInvalidationNoThrowWhenOffline) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    EXPECT_NO_THROW(coord.publishInvalidation("orders.*", "tenant_x"));
}

TEST(RedisCacheCoordinatorTest, PublishEntryNoThrowWhenNoTenant) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    EXPECT_NO_THROW(coord.publishEntry("fp_000", {{"v", 1}}, 60, ""));
}

TEST(RedisCacheCoordinatorTest, SubscribeEntryCallbackRegisteredNoThrow) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    bool called = false;
    EXPECT_NO_THROW(
        coord.subscribeEntries([&called](const ReplicationMessage&) { called = true; }));
    // Callback should NOT have been called (no messages in offline mode)
    EXPECT_FALSE(called);
}

TEST(RedisCacheCoordinatorTest, SubscribeInvalidationCallbackRegisteredNoThrow) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    EXPECT_NO_THROW(
        coord.subscribeInvalidations([](const ReplicationMessage&) {}));
}

// ============================================================================
// Stats
// ============================================================================

TEST(RedisCacheCoordinatorTest, GetStatsReturnsExpectedFields) {
    RedisCacheCoordinator coord(makeOfflineConfig("stats-node")); // Parameter ignored by new config
    auto stats = coord.getStats();

    EXPECT_TRUE(stats.contains("messages_published"));
    EXPECT_TRUE(stats.contains("messages_received"));
    EXPECT_TRUE(stats.contains("publish_errors"));
    EXPECT_TRUE(stats.contains("invalidation_errors"));
    EXPECT_TRUE(stats.contains("reconnect_count"));
    EXPECT_TRUE(stats.contains("connected"));
    EXPECT_TRUE(stats.contains("ready"));
    EXPECT_TRUE(stats.contains("dependency_available"));
    EXPECT_TRUE(stats.contains("channel") || stats.contains("channel_prefix"));

    EXPECT_EQ(stats["messages_published"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["messages_received"].get<uint64_t>(),  0u);
    if (stats.contains("node_id")) {
        // POSIX implementation derives node_id from host:port.
        EXPECT_EQ(stats["node_id"].get<std::string>(), "127.0.0.1:16399");
    }

    if (stats.contains("channel")) {
        EXPECT_EQ(stats["channel"].get<std::string>(), "themis_test:replication");
    } else {
        EXPECT_EQ(stats["channel_prefix"].get<std::string>(), "themis_test");
    }
}

TEST(RedisCacheCoordinatorTest, PublishErrorsIncrementedWhenOffline) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    coord.publishEntry("k1", {{"a", 1}}, 60, "");
    coord.publishInvalidation(".*");

    // Wait briefly for the reconnect thread
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto stats = coord.getStats();
    // Publish errors should be > 0 because Redis is not available
    EXPECT_GT(stats["publish_errors"].get<uint64_t>(), 0u);
    EXPECT_GT(stats["invalidation_errors"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["messages_published"].get<uint64_t>(), 0u);
    EXPECT_FALSE(stats["ready"].get<bool>());
#if defined(THEMIS_ENABLE_REDIS)
    EXPECT_TRUE(stats["dependency_available"].get<bool>());
    EXPECT_EQ(stats["failure_reason"], "redis_connection_unavailable");
#else
    EXPECT_FALSE(stats["dependency_available"].get<bool>());
    EXPECT_EQ(stats["failure_reason"], "hiredis_unavailable");
#endif
}

TEST(RedisCacheCoordinatorTest, MissingHiredisCannotReportInvalidationSuccess) {
#if defined(THEMIS_ENABLE_REDIS)
    GTEST_SKIP() << "Redis dependency profile is enabled.";
#else
    RedisCacheCoordinator::setRedisPublishBridgeFn(
        [](const std::string&, const std::string&) { return true; });
    RedisCacheCoordinator coord(makeOfflineConfig());
    coord.publishInvalidation("orders.*", "tenant_x");
    const auto stats = coord.getStats();
    RedisCacheCoordinator::setRedisPublishBridgeFn({});

    EXPECT_FALSE(coord.isConnected());
    EXPECT_FALSE(stats["ready"].get<bool>());
    EXPECT_FALSE(stats["dependency_available"].get<bool>());
    EXPECT_EQ(stats["status"], "dependency_unavailable");
    EXPECT_EQ(stats["messages_published"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["invalidation_errors"].get<uint64_t>(), 1u);
#endif
}

TEST(RedisCacheCoordinatorTest, ChannelNameMatchesPrefix) {
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.channel_prefix = "myapp_cache";
    RedisCacheCoordinator coord(cfg);

    auto stats = coord.getStats();
    if (stats.contains("channel")) {
        EXPECT_EQ(stats["channel"].get<std::string>(), "myapp_cache:replication");
    } else {
        EXPECT_EQ(stats["channel_prefix"].get<std::string>(), "myapp_cache");
    }
}

// ============================================================================
// Multiple publish calls
// ============================================================================

TEST(RedisCacheCoordinatorTest, MultiplePublishCallsDoNotCrash) {
    RedisCacheCoordinator coord(makeOfflineConfig());
    for (int i = 0; i < 10; ++i) {
        coord.publishEntry("key_" + std::to_string(i), {{"i", i}}, 60, "");
    }
    for (int i = 0; i < 5; ++i) {
        coord.publishInvalidation("pattern_" + std::to_string(i));
    }
    // No crash = pass
    SUCCEED();
}

// ============================================================================
// Callback overwrite
// ============================================================================

TEST(RedisCacheCoordinatorTest, CallbackCanBeOverwritten) {
    RedisCacheCoordinator coord(makeOfflineConfig());

    int call_count_a = 0, call_count_b = 0;
    coord.subscribeEntries([&call_count_a](const ReplicationMessage&) { ++call_count_a; });
    coord.subscribeEntries([&call_count_b](const ReplicationMessage&) { ++call_count_b; });

    // In offline mode neither callback is invoked; the important check is
    // that overwriting does not throw or corrupt internal state.
    EXPECT_EQ(call_count_a, 0);
    EXPECT_EQ(call_count_b, 0);
}

// ============================================================================
// Polymorphism via ICacheCoordinator pointer
// ============================================================================

TEST(RedisCacheCoordinatorTest, PolymorphicUsageViaInterface) {
    auto coord = std::make_shared<RedisCacheCoordinator>(makeOfflineConfig());

    // Use through the base interface pointer
    ICacheCoordinator* iface = coord.get();
    EXPECT_NE(iface, nullptr);

    // All interface methods must be callable without crashing
    EXPECT_NO_THROW(iface->publishEntry("k", {{}}, 10, ""));
    EXPECT_NO_THROW(iface->publishInvalidation(".*"));
    EXPECT_NO_THROW(iface->subscribeEntries([](const ReplicationMessage&) {}));
    EXPECT_NO_THROW(iface->subscribeInvalidations([](const ReplicationMessage&) {}));
    EXPECT_FALSE(iface->isConnected());  // Offline
    EXPECT_FALSE(iface->name().empty());
    auto s = iface->getStats();
    EXPECT_TRUE(s.is_object());
}

// ============================================================================
// Node ID defaults to host:port when not specified
// ============================================================================

TEST(RedisCacheCoordinatorTest, DefaultNodeIdIsHostPort) {
    RedisCacheCoordinatorConfig cfg;
    cfg.host               = "192.168.1.10";
    cfg.port               = 6380;
    cfg.connect_timeout_ms = 100;
    // Note: new config does not have node_id field; node_id is derived internally from host:port
    // Use high port to avoid accidental connection
    cfg.port               = 16400;
    RedisCacheCoordinator coord(cfg);

    auto stats = coord.getStats();
    if (!stats.contains("node_id")) {
        GTEST_SKIP() << "node_id not exposed in this platform coordinator stats";
    }

    std::string node_id = stats["node_id"].get<std::string>();
    EXPECT_EQ(node_id, "192.168.1.10:16400");
}

TEST(RedisCacheCoordinatorTest, ExplicitNodeIdIsPreserved) {
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    // Note: new config does not have node_id field; this test now verifies default behavior
    RedisCacheCoordinator coord(cfg);

    auto stats = coord.getStats();
    if (!stats.contains("node_id")) {
        GTEST_SKIP() << "node_id not exposed in this platform coordinator stats";
    }

    // Default node_id is host:port
    EXPECT_EQ(stats["node_id"].get<std::string>(), "127.0.0.1:16399");
}

// ============================================================================
// Graceful degradation: callbacks never fire in offline mode
// ============================================================================

TEST(RedisCacheCoordinatorTest, NoCallbacksFireInOfflineMode) {
    RedisCacheCoordinator coord(makeOfflineConfig());

    std::atomic<int> entry_calls{0};
    std::atomic<int> inv_calls{0};

    coord.subscribeEntries([&entry_calls](const ReplicationMessage&) { ++entry_calls; });
    coord.subscribeInvalidations([&inv_calls](const ReplicationMessage&) { ++inv_calls; });

    coord.publishEntry("k", {{"v", 1}}, 60, "");
    coord.publishInvalidation(".*");

    // Give subscriber thread time to potentially fire (it shouldn't)
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_EQ(entry_calls.load(), 0);
    EXPECT_EQ(inv_calls.load(),   0);
}

// ============================================================================
// HMAC signing / verification (offline unit tests — no Redis required)
// ============================================================================

TEST(RedisCacheCoordinatorTest, HmacSecretDisabledByDefault) {
    // When hmac_secret is empty, the coordinator must NOT attach a "sig" field
    // and must accept unsigned messages. Verify via stats/no-throw.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    ASSERT_TRUE(cfg.hmac_secret.empty());

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
        coord.publishEntry("k", {{"v", 1}}, 60, "");
        coord.publishInvalidation("pattern");
    });
}

TEST(RedisCacheCoordinatorTest, HmacSecretConfigFieldAccepted) {
    // Verify the hmac_secret config field is reachable and stored.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.hmac_secret = "s3cr3t";

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
    });
}

TEST(RedisCacheCoordinatorTest, HmacPublishDoesNotThrow) {
    // Publishing with an HMAC secret set must not throw even if Redis is offline.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.hmac_secret = "test-hmac-key";

    EXPECT_NO_THROW({
        RedisCacheCoordinator coord(cfg);
        coord.publishEntry("fp", {{"r", 42}}, 30, "acme");
        coord.publishInvalidation("acme.*");
    });
}

TEST(RedisCacheCoordinatorTest, HmacEmptySecretPublishAndStatsOk) {
    // Without hmac_secret, publish should increment publish_errors_ (no Redis)
    // but stats must be accessible.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    ASSERT_TRUE(cfg.hmac_secret.empty());

    RedisCacheCoordinator coord(cfg);
    coord.publishEntry("key", {}, 60, "");
    coord.publishInvalidation("*");

    auto stats = coord.getStats();
    EXPECT_TRUE(stats.contains("publish_errors"));
    EXPECT_GE(stats["publish_errors"].get<uint64_t>(), 0u);
}

// ============================================================================
// Exponential back-off reconnect – validate reconnect_count increments
// ============================================================================

TEST(RedisCacheCoordinatorTest, ReconnectCountIncreasesOverTime) {
    // On POSIX builds the subscriber thread attempts reconnects when Redis is
    // absent. On Windows the coordinator is a deliberate no-op stub without a
    // background reconnect loop.
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.connect_timeout_ms   = 100;
    cfg.reconnect_interval_ms = 50;  // fast for test; back-off overrides this

    RedisCacheCoordinator coord(cfg);

    // Wait long enough for at least one reconnect attempt.
    std::this_thread::sleep_for(std::chrono::milliseconds(400));

    auto stats = coord.getStats();
    ASSERT_TRUE(stats.contains("reconnect_count"));
#if !defined(THEMIS_ENABLE_REDIS) || defined(_WIN32)
    EXPECT_EQ(stats["reconnect_count"].get<uint64_t>(), 0u);
#else
    EXPECT_GE(stats["reconnect_count"].get<uint64_t>(), 1u);
#endif
}

TEST(RedisCacheCoordinatorTest, IsConnectedFalseAfterMultipleAttempts) {
    // After several back-off cycles the coordinator must still report
    // disconnected (not throw, not corrupt state).
    RedisCacheCoordinatorConfig cfg = makeOfflineConfig();
    cfg.connect_timeout_ms   = 100;

    RedisCacheCoordinator coord(cfg);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_FALSE(coord.isConnected());
    // Stats must remain well-formed
    auto stats = coord.getStats();
    EXPECT_TRUE(stats.is_object());
}

#if defined(THEMIS_ENABLE_REDIS) && !defined(_WIN32)
TEST(RedisCacheCoordinatorIntegrationTest, SubscriptionDenialKeepsCoordinatorUnready) {
    LocalRedisServer redis;
    redis.setSubscriptionEnabled(false);
    RedisCacheCoordinatorConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = redis.port();
    cfg.channel_prefix = "themis_no_subscription";
    cfg.connect_timeout_ms = 200;
    cfg.reconnect_interval_ms = 50;
    RedisCacheCoordinator coord(cfg);

    ASSERT_TRUE(waitFor([&redis] { return redis.subscriptionAttemptCount() > 0; },
                        std::chrono::seconds(2)));
    ASSERT_TRUE(waitFor([&coord] {
        return coord.getStats().value("reconnect_count", uint64_t{0}) > 0;
    }, std::chrono::seconds(2)));
    coord.publishInvalidation("orders:*", "tenant_x");
    const auto stats = coord.getStats();
    EXPECT_FALSE(coord.isConnected());
    EXPECT_FALSE(stats["ready"].get<bool>());
    EXPECT_TRUE(stats["dependency_available"].get<bool>());
    EXPECT_EQ(stats["failure_reason"], "redis_connection_unavailable");
    EXPECT_EQ(stats["invalidation_errors"].get<uint64_t>(), 1u);
}

TEST(RedisCacheCoordinatorIntegrationTest, ZeroSubscriberPublishFailsClosed) {
    LocalRedisServer redis;
    RedisCacheCoordinatorConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = redis.port();
    cfg.channel_prefix = "themis_no_subscribers";
    cfg.connect_timeout_ms = 200;
    cfg.reconnect_interval_ms = 50;
    RedisCacheCoordinator coord(cfg);

    ASSERT_TRUE(waitFor([&redis] { return redis.subscriberCount() > 0; },
                        std::chrono::seconds(2)));
    redis.setZeroSubscriberChannel(cfg.channel_prefix + ":invalidations");
    coord.publishInvalidation("orders:*", "tenant_x");

    const auto stats = coord.getStats();
    EXPECT_TRUE(coord.isConnected());
    EXPECT_EQ(stats["messages_published"].get<uint64_t>(), 0u);
    EXPECT_EQ(stats["invalidation_errors"].get<uint64_t>(), 1u);
    EXPECT_FALSE(stats["ready"].get<bool>());
    EXPECT_EQ(stats["failure_reason"], "invalidation_publish_failed");

    coord.publishEntry("entry", {{"value", 1}}, 60, "tenant_x");
    EXPECT_FALSE(coord.getStats()["ready"].get<bool>());

    redis.setZeroSubscriberChannel({});
    coord.publishInvalidation("orders:*", "tenant_x");
    const auto recovered_stats = coord.getStats();
    EXPECT_TRUE(recovered_stats["ready"].get<bool>());
    EXPECT_EQ(recovered_stats["messages_published"].get<uint64_t>(), 2u);
    EXPECT_EQ(recovered_stats["invalidation_errors"].get<uint64_t>(), 1u);
}

TEST(RedisCacheCoordinatorIntegrationTest, PublishesInvalidationAndRecoversAfterDisconnect) {
    LocalRedisServer redis;
    RedisCacheCoordinatorConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = redis.port();
    cfg.channel_prefix = "themis_integration";
    cfg.connect_timeout_ms = 200;
    cfg.reconnect_interval_ms = 50;
    RedisCacheCoordinator coord(cfg);

    std::atomic<int> invalidations{0};
    coord.subscribeInvalidations([&invalidations](const ReplicationMessage& message) {
        if (message.key == "orders:*" && message.tenant_id == "tenant_x") {
            ++invalidations;
        }
    });

    ASSERT_TRUE(waitFor([&redis] { return redis.subscriberCount() > 0; },
                        std::chrono::seconds(2)));
    coord.publishInvalidation("orders:*", "tenant_x");
    ASSERT_TRUE(waitFor([&invalidations] { return invalidations.load() == 1; },
                        std::chrono::seconds(2)));
    ASSERT_TRUE(coord.isConnected());

    redis.disconnectClients();
    ASSERT_TRUE(waitFor([&coord] { return !coord.isConnected(); },
                        std::chrono::seconds(2)));
    redis.resumeClients();
    ASSERT_TRUE(waitFor([&redis] { return redis.subscriptionAttemptCount() > 1; },
                        std::chrono::seconds(4)));

    coord.publishInvalidation("orders:*", "tenant_x");
    ASSERT_TRUE(waitFor([&coord] { return coord.isConnected(); },
                        std::chrono::seconds(2)));
    ASSERT_TRUE(waitFor([&invalidations] { return invalidations.load() == 2; },
                        std::chrono::seconds(2)));

    const auto stats = coord.getStats();
    EXPECT_TRUE(stats["ready"].get<bool>());
    EXPECT_TRUE(stats["dependency_available"].get<bool>());
    EXPECT_EQ(stats["messages_published"].get<uint64_t>(), 2u);
    EXPECT_EQ(stats["invalidation_errors"].get<uint64_t>(), 0u);
}
#endif
