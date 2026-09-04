/**
 * Focused unit tests for MqttClientService (v1.9.0).
 *
 * Covered acceptance criteria:
 *
 * AC-CFG  MqttClientConfig defaults and custom values
 * AC-STA  MqttClientStats initial state, increment semantics, reset
 * AC-HDL  IMqttMessageHandler virtual interface & default callbacks
 * AC-SVC  MqttClientService construction, lifecycle (start/stop idempotent)
 * AC-PUB  publish() when disconnected returns false + increments error counter
 * AC-SUB  subscribe() / unsubscribe() state management
 * AC-REG  registerWithServiceRegistry / unregisterFromServiceRegistry
 * AC-CDC  MqttCDCTransport: topicForEvent (all 4 event types), custom prefix/QoS
 * AC-CDC2 MqttCDCTransport: publish() not connected returns false
 * AC-CDC3 MqttCDCTransport: start() / stop() delegation
 * AC-ID   clientId() is non-empty and stable
 * AC-CFG2 getConfig() returns the original configuration
 * AC-RST  resetStats() zeroes all counters
 * AC-MCK  Recording handler: onMessage, onConnected, onDisconnected callbacks
 */

#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#ifndef THEMIS_ENABLE_MQTT

TEST(MqttClientServiceFocusedTests, SkippedMqttDisabled) {
    GTEST_SKIP() << "MQTT is disabled. "
                    "Build with -DTHEMIS_ENABLE_MQTT=ON to enable these tests.";
}

#else // THEMIS_ENABLE_MQTT

#include "server/mqtt_client_service.h"

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace themis::server;

// ── Recording handler ─────────────────────────────────────────────────────────

class RecordingHandler : public IMqttMessageHandler {
public:
    struct Record {
        std::string topic = {};
        std::string payload = {};
        uint8_t     qos{0};
    };

    void onMessage(const std::string& topic,
                   const std::string& payload,
                   uint8_t            qos) noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        messages.push_back({topic, payload, qos});
        ++message_count;
    }

    void onConnected(const std::string& client_id) noexcept override {
        connected_client_id = client_id;
        ++connected_count;
    }

    void onDisconnected(const std::string& reason) noexcept override {
        disconnect_reason = reason;
        ++disconnected_count;
    }

    std::vector<Record>  messages;
    std::string          connected_client_id;
    std::string          disconnect_reason;
    std::atomic<int>     message_count{0};
    std::atomic<int>     connected_count{0};
    std::atomic<int>     disconnected_count{0};

private:
    mutable std::mutex mu_;
};

class ScopedThreadJoiner {
public:
    explicit ScopedThreadJoiner(std::thread& t) : thread_(t) {}
    ~ScopedThreadJoiner() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    std::thread& thread_;
};

// ── AC-CFG: MqttClientConfig defaults ────────────────────────────────────────

TEST(MqttClientConfigFocusedTests, DefaultBrokerHost) {
    MqttClientConfig cfg;
    EXPECT_EQ(cfg.broker_host, "localhost");
}

TEST(MqttClientConfigFocusedTests, DefaultBrokerPort) {
    MqttClientConfig cfg;
    EXPECT_EQ(cfg.broker_port, uint16_t{1883});
}

TEST(MqttClientConfigFocusedTests, DefaultClientIdEmpty) {
    MqttClientConfig cfg;
    EXPECT_TRUE(cfg.client_id.empty());
}

TEST(MqttClientConfigFocusedTests, DefaultCleanSession) {
    MqttClientConfig cfg;
    EXPECT_TRUE(cfg.clean_session);
}

TEST(MqttClientConfigFocusedTests, DefaultKeepalive) {
    MqttClientConfig cfg;
    EXPECT_EQ(cfg.keepalive_seconds, uint16_t{60});
}

TEST(MqttClientConfigFocusedTests, DefaultQos) {
    MqttClientConfig cfg;
    EXPECT_EQ(cfg.default_qos, uint8_t{1});
}

TEST(MqttClientConfigFocusedTests, DefaultTlsDisabled) {
    MqttClientConfig cfg;
    EXPECT_FALSE(cfg.tls_enabled);
}

TEST(MqttClientConfigFocusedTests, DefaultCdcTopicPrefix) {
    MqttClientConfig cfg;
    EXPECT_EQ(cfg.cdc_topic_prefix, "themis/cdc/");
}

TEST(MqttClientConfigFocusedTests, DefaultCdcQos) {
    MqttClientConfig cfg;
    EXPECT_EQ(cfg.cdc_qos, uint8_t{1});
}

TEST(MqttClientConfigFocusedTests, CustomValues) {
    MqttClientConfig cfg;
    cfg.broker_host        = "mqtt.example.com";
    cfg.broker_port        = 8883;
    cfg.client_id          = "myservice-001";
    cfg.username           = "user";
    cfg.password           = "secret";
    cfg.clean_session      = false;
    cfg.keepalive_seconds  = 30;
    cfg.default_qos        = 0;
    cfg.tls_enabled        = true;
    cfg.cdc_topic_prefix   = "prod/cdc/";
    cfg.cdc_qos            = 2;
    cfg.connect_timeout_ms = 5000;

    EXPECT_EQ(cfg.broker_host, "mqtt.example.com");
    EXPECT_EQ(cfg.broker_port, uint16_t{8883});
    EXPECT_EQ(cfg.client_id,   "myservice-001");
    EXPECT_FALSE(cfg.clean_session);
    EXPECT_EQ(cfg.keepalive_seconds, uint16_t{30});
    EXPECT_TRUE(cfg.tls_enabled);
    EXPECT_EQ(cfg.cdc_topic_prefix, "prod/cdc/");
    EXPECT_EQ(cfg.cdc_qos, uint8_t{2});
    EXPECT_EQ(cfg.connect_timeout_ms, uint32_t{5000});
}

// ── AC-STA: MqttClientStats ───────────────────────────────────────────────────

TEST(MqttClientStatsFocusedTests, InitialCountersAreZero) {
    MqttClientStats s;
    EXPECT_EQ(s.messages_published.load(), 0u);
    EXPECT_EQ(s.messages_received.load(),  0u);
    EXPECT_EQ(s.bytes_sent.load(),         0u);
    EXPECT_EQ(s.bytes_received.load(),     0u);
    EXPECT_EQ(s.connect_count.load(),      0u);
    EXPECT_EQ(s.reconnect_count.load(),    0u);
    EXPECT_EQ(s.publish_errors.load(),     0u);
    EXPECT_EQ(s.subscribe_count.load(),    0u);
    EXPECT_FALSE(s.is_connected.load());
}

TEST(MqttClientStatsFocusedTests, ResetClearsAll) {
    MqttClientStats s;
    ++s.messages_published;
    ++s.messages_received;
    ++s.bytes_sent;
    s.is_connected = true;

    s.reset();

    EXPECT_EQ(s.messages_published.load(), 0u);
    EXPECT_EQ(s.messages_received.load(),  0u);
    EXPECT_EQ(s.bytes_sent.load(),         0u);
    EXPECT_FALSE(s.is_connected.load());
}

TEST(MqttClientStatsFocusedTests, AtomicIncrementIsThreadSafe) {
    MqttClientStats s;
    constexpr int N = 1000;
    std::vector<std::thread> threads;
    threads.reserve(N);
    for (int i = 0; i < N; ++i)
        threads.emplace_back([&s] { ++s.messages_published; });
    for (auto& t : threads) {
      t.join();
    }
    EXPECT_EQ(s.messages_published.load(), static_cast<uint64_t>(N));
}

// ── AC-HDL: IMqttMessageHandler ───────────────────────────────────────────────

TEST(IMqttMessageHandlerFocusedTests, IsAbstract) {
    // IMqttMessageHandler must be abstract (cannot instantiate directly)
    EXPECT_TRUE(std::is_abstract_v<IMqttMessageHandler>);
}

TEST(IMqttMessageHandlerFocusedTests, DefaultOnConnectedIsNoOp) {
    // RecordingHandler overrides onConnected — verify default is benign
    // by calling it directly via the base interface default implementation.
    class MinimalHandler : public IMqttMessageHandler {
    public:
        void onMessage(const std::string&, const std::string&, uint8_t) noexcept override {}
    };
    MinimalHandler h;
    EXPECT_NO_THROW(h.onConnected("test-id"));
    EXPECT_NO_THROW(h.onDisconnected("reason"));
}

TEST(IMqttMessageHandlerFocusedTests, RecordingHandlerCaptures) {
    auto h = std::make_shared<RecordingHandler>();
    h->onMessage("topic/a", "hello", 1);
    h->onMessage("topic/b", "world", 0);

    ASSERT_EQ(h->messages.size(), 2u);
    EXPECT_EQ(h->messages[0].topic,   "topic/a");
    EXPECT_EQ(h->messages[0].payload, "hello");
    EXPECT_EQ(h->messages[0].qos,     uint8_t{1});
    EXPECT_EQ(h->messages[1].topic,   "topic/b");
    EXPECT_EQ(h->messages[1].payload, "world");
}

// ── AC-SVC: MqttClientService construction ────────────────────────────────────

TEST(MqttClientServiceFocusedTests, ConstructsWithDefaults) {
    MqttClientService svc;
    EXPECT_FALSE(svc.isConnected());
    EXPECT_EQ(svc.getConfig().broker_host, "localhost");
}

TEST(MqttClientServiceFocusedTests, ConstructsWithCustomConfig) {
    MqttClientConfig cfg;
    cfg.broker_host = "broker.internal";
    cfg.broker_port = 1884;
    MqttClientService svc(cfg);
    EXPECT_EQ(svc.getConfig().broker_host, "broker.internal");
    EXPECT_EQ(svc.getConfig().broker_port, uint16_t{1884});
}

TEST(MqttClientServiceFocusedTests, InitiallyDisconnected) {
    MqttClientService svc;
    EXPECT_FALSE(svc.isConnected());
}

TEST(MqttClientServiceFocusedTests, StopBeforeStartIsNoop) {
    MqttClientService svc;
    EXPECT_NO_THROW(svc.stop()); // must not crash or hang
}

TEST(MqttClientServiceFocusedTests, StartAndStopAreIdempotent) {
    // With no real broker, the service starts its I/O thread and immediately
    // schedules a reconnect when the TCP connect fails.  start()/stop() must
    // be callable multiple times without UB.
    MqttClientConfig cfg;
    cfg.broker_host        = "127.0.0.1";
    cfg.broker_port        = 19999; // assume nothing listens here
    cfg.connect_timeout_ms = 200;
    cfg.retry.maxRetries   = 0;     // do not retry; fail fast

    MqttClientService svc(cfg);
    svc.start();
    svc.start(); // second start() must be no-op
    svc.stop();
    svc.stop(); // second stop() must be no-op
}

// ── AC-ID: clientId() ─────────────────────────────────────────────────────────

TEST(MqttClientServiceFocusedTests, ClientIdIsNonEmptyWhenConfigEmpty) {
    MqttClientConfig cfg;
    cfg.client_id = "";
    MqttClientService svc(cfg);
    EXPECT_FALSE(svc.clientId().empty());
}

TEST(MqttClientServiceFocusedTests, ClientIdUsesConfigWhenSet) {
    MqttClientConfig cfg;
    cfg.client_id = "my-custom-id";
    MqttClientService svc(cfg);
    EXPECT_EQ(svc.clientId(), "my-custom-id");
}

TEST(MqttClientServiceFocusedTests, AutoGeneratedClientIdStartsWithPrefix) {
    MqttClientConfig cfg;
    // leave client_id empty → auto-generate
    MqttClientService svc(cfg);
    const std::string& id = svc.clientId();
    EXPECT_GT(id.size(), 0u);
    // Convention: "themisdb-" prefix
    EXPECT_EQ(id.substr(0, 9), "themisdb-");
}

// ── AC-PUB: publish() when disconnected ──────────────────────────────────────

TEST(MqttClientServiceFocusedTests, PublishReturnsFalseWhenDisconnected) {
    MqttClientService svc;
    bool ok = svc.publish("test/topic", "payload");
    EXPECT_FALSE(ok);
}

TEST(MqttClientServiceFocusedTests, PublishIncrementsErrorCountWhenDisconnected) {
    MqttClientService svc;
    svc.publish("test/topic", "payload");
    EXPECT_EQ(svc.getStats().publish_errors.load(), 1u);
}

TEST(MqttClientServiceFocusedTests, MultiplePublishErrorsAccumulate) {
    MqttClientService svc;
    for (int i = 0; i < 5; ++i)
        svc.publish("t", "p");
    EXPECT_EQ(svc.getStats().publish_errors.load(), 5u);
}

TEST(MqttClientServiceFocusedTests, PublishDoesNotIncrementBytesSentWhenDisconnected) {
    // bytes_sent must NOT be incremented on a failed publish (not connected).
    MqttClientService svc;
    svc.publish("test/topic", "hello");
    EXPECT_EQ(svc.getStats().bytes_sent.load(), 0u);
}

TEST(MqttClientServiceFocusedTests, MessagesPublishedNotIncrementedOnError) {
    // messages_published counts only successful enqueues, not failed ones.
    MqttClientService svc;
    svc.publish("test/topic", "payload"); // fails: not connected
    EXPECT_EQ(svc.getStats().messages_published.load(), 0u);
}

// ── AC-SUB: subscribe() / unsubscribe() ──────────────────────────────────────

TEST(MqttClientServiceFocusedTests, SubscribeReturnsTrue) {
    MqttClientService svc;
    EXPECT_TRUE(svc.subscribe("sensor/#"));
}

TEST(MqttClientServiceFocusedTests, UnsubscribeReturnsTrue) {
    MqttClientService svc;
    svc.subscribe("sensor/#");
    EXPECT_TRUE(svc.unsubscribe("sensor/#"));
}

TEST(MqttClientServiceFocusedTests, MultipleSubscriptionsAccepted) {
    MqttClientService svc;
    EXPECT_TRUE(svc.subscribe("a/+/status", 0));
    EXPECT_TRUE(svc.subscribe("b/#",         1));
    EXPECT_TRUE(svc.subscribe("c/exact",     0));
}

// ── AC-REG: service registry ──────────────────────────────────────────────────

TEST(MqttClientServiceFocusedTests, RegisterWithDefaultName) {
    MqttClientService svc;
    svc.registerWithServiceRegistry();

    using namespace themis::plugins::rpc;
    void* ptr = RPCServiceRegistry::getService("mqtt_client");
    EXPECT_EQ(ptr, static_cast<void*>(&svc));

    svc.unregisterFromServiceRegistry();
    EXPECT_EQ(RPCServiceRegistry::getService("mqtt_client"), nullptr);
}

TEST(MqttClientServiceFocusedTests, RegisterWithCustomName) {
    MqttClientService svc;
    svc.registerWithServiceRegistry("my_mqtt_svc");

    using namespace themis::plugins::rpc;
    void* ptr = RPCServiceRegistry::getService("my_mqtt_svc");
    EXPECT_EQ(ptr, static_cast<void*>(&svc));

    svc.unregisterFromServiceRegistry("my_mqtt_svc");
    EXPECT_EQ(RPCServiceRegistry::getService("my_mqtt_svc"), nullptr);
}

// ── AC-RST: resetStats() ──────────────────────────────────────────────────────

TEST(MqttClientServiceFocusedTests, ResetStatsClearsErrors) {
    MqttClientService svc;
    svc.publish("t", "p"); // increments publish_errors
    EXPECT_EQ(svc.getStats().publish_errors.load(), 1u);
    svc.resetStats();
    EXPECT_EQ(svc.getStats().publish_errors.load(), 0u);
}

// ── AC-CDC: MqttCDCTransport::topicForEvent ───────────────────────────────────

namespace {
::themis::Changefeed::ChangeEvent makeEvent(
        ::themis::Changefeed::ChangeEventType type,
        const std::string& collection = "orders") {
    ::themis::Changefeed::ChangeEvent ev;
    ev.sequence     = 1;
    ev.type         = type;
    ev.key          = "key-1";
    ev.timestamp_ms = 0;
    ev.metadata["collection"] = collection;
    return ev;
}
} // namespace

TEST(MqttCDCTransportFocusedTests, TopicForPutEvent) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_PUT, "orders");
    EXPECT_EQ(tr.topicForEvent(ev), "themis/cdc/orders/PUT");
}

TEST(MqttCDCTransportFocusedTests, TopicForDeleteEvent) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_DELETE, "users");
    EXPECT_EQ(tr.topicForEvent(ev), "themis/cdc/users/DELETE");
}

TEST(MqttCDCTransportFocusedTests, TopicForTransactionCommit) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT,
                        "$system");
    EXPECT_EQ(tr.topicForEvent(ev), "themis/cdc/$system/TRANSACTION_COMMIT");
}

TEST(MqttCDCTransportFocusedTests, TopicForTransactionRollback) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK,
                        "$system");
    EXPECT_EQ(tr.topicForEvent(ev), "themis/cdc/$system/TRANSACTION_ROLLBACK");
}

TEST(MqttCDCTransportFocusedTests, DefaultCollectionWhenMetadataMissing) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    ::themis::Changefeed::ChangeEvent ev;
    ev.type = ::themis::Changefeed::ChangeEventType::EVENT_PUT;
    // no "collection" in metadata
    std::string topic = tr.topicForEvent(ev);
    EXPECT_EQ(topic, "themis/cdc/default/PUT");
}

TEST(MqttCDCTransportFocusedTests, CustomPrefixApplied) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    tr.setTopicPrefix("iot/events/");
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_PUT, "sensors");
    EXPECT_EQ(tr.topicForEvent(ev), "iot/events/sensors/PUT");
}

TEST(MqttCDCTransportFocusedTests, CustomQosApplied) {
    MqttClientService svc;
    auto& tr = svc.cdcTransport();
    tr.setQos(0);
    EXPECT_EQ(tr.qos(), uint8_t{0});
    tr.setQos(2);
    EXPECT_EQ(tr.qos(), uint8_t{2});
}

TEST(MqttCDCTransportFocusedTests, TopicPrefixReflectsConfig) {
    MqttClientConfig cfg;
    cfg.cdc_topic_prefix = "custom/prefix/";
    MqttClientService svc(cfg);
    EXPECT_EQ(svc.cdcTransport().topicPrefix(), "custom/prefix/");
}

// ── AC-CDC2: MqttCDCTransport::publish() when not connected ──────────────────

TEST(MqttCDCTransportFocusedTests, PublishNotConnectedReturnsFalse) {
    MqttClientService svc;
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_PUT, "items");
    bool ok = svc.cdcTransport().publish(ev);
    EXPECT_FALSE(ok);
}

TEST(MqttCDCTransportFocusedTests, PublishNotConnectedIncrementsErrors) {
    MqttClientService svc;
    auto ev = makeEvent(::themis::Changefeed::ChangeEventType::EVENT_DELETE, "items");
    svc.cdcTransport().publish(ev);
    EXPECT_GE(svc.getStats().publish_errors.load(), 1u);
}

// ── AC-CDC3: MqttCDCTransport start/stop delegation ──────────────────────────

TEST(MqttCDCTransportFocusedTests, StartDelegatesToService) {
    MqttClientConfig cfg;
    cfg.broker_host        = "127.0.0.1";
    cfg.broker_port        = 19998;
    cfg.connect_timeout_ms = 100;
    cfg.retry.maxRetries   = 0;
    MqttClientService svc(cfg);
    // start() via cdcTransport must not throw
    EXPECT_NO_THROW(svc.cdcTransport().start());
    EXPECT_NO_THROW(svc.cdcTransport().stop());
}

// ── AC-MCK: RecordingHandler invocation ──────────────────────────────────────

TEST(MqttClientServiceFocusedTests, SetMessageHandlerAccepted) {
    MqttClientService svc;
    auto h = std::make_shared<RecordingHandler>();
    EXPECT_NO_THROW(svc.setMessageHandler(h));
}

TEST(MqttClientServiceFocusedTests, SetMessageHandlerCanBeReplaced) {
    MqttClientService svc;
    auto h1 = std::make_shared<RecordingHandler>();
    auto h2 = std::make_shared<RecordingHandler>();
    svc.setMessageHandler(h1);
    svc.setMessageHandler(h2); // should not throw
    svc.setMessageHandler(nullptr); // reset to null is safe
}

TEST(MqttClientServiceFocusedTests, CdcTransportAccessibleFromService) {
    MqttClientService svc;
    // Ensure cdcTransport() returns a valid MqttCDCTransport&
    MqttCDCTransport& tr = svc.cdcTransport();
    EXPECT_EQ(&tr.topicPrefix(), &svc.cdcTransport().topicPrefix());
}

// ── MqttRetryConfig defaults (from mqtt_session.h) ───────────────────────────

TEST(MqttRetryConfigFocusedTests, DefaultMaxRetries) {
    MqttRetryConfig r;
    EXPECT_EQ(r.maxRetries, 3u);
}

TEST(MqttRetryConfigFocusedTests, DefaultInitialDelayMs) {
    MqttRetryConfig r;
    EXPECT_EQ(r.initialRetryDelayMs, 1000u);
}

TEST(MqttRetryConfigFocusedTests, DefaultMaxDelayMs) {
    MqttRetryConfig r;
    EXPECT_EQ(r.maxRetryDelayMs, 60000u);
}

TEST(MqttRetryConfigFocusedTests, DefaultExponentialBackoff) {
    MqttRetryConfig r;
    EXPECT_TRUE(r.exponentialBackoff);
}

// ── TLS configuration (AC-TLS) ───────────────────────────────────────────────
// These tests validate the TLS fields of MqttClientConfig.  They run in all
// builds regardless of THEMIS_ENABLE_MQTT_TLS because the TLS fields are
// always present in MqttClientConfig (the config struct is not gated).

TEST(MqttClientTlsConfigTests, TlsEnabledDefaultsFalse) {
    MqttClientConfig cfg;
    EXPECT_FALSE(cfg.tls_enabled);
}

TEST(MqttClientTlsConfigTests, TlsCertPathDefaultsEmpty) {
    MqttClientConfig cfg;
    EXPECT_TRUE(cfg.tls_cert_path.empty());
}

TEST(MqttClientTlsConfigTests, TlsKeyPathDefaultsEmpty) {
    MqttClientConfig cfg;
    EXPECT_TRUE(cfg.tls_key_path.empty());
}

TEST(MqttClientTlsConfigTests, TlsCaPathDefaultsEmpty) {
    MqttClientConfig cfg;
    EXPECT_TRUE(cfg.tls_ca_path.empty());
}

TEST(MqttClientTlsConfigTests, TlsEnabledCanBeSetTrue) {
    MqttClientConfig cfg;
    cfg.tls_enabled = true;
    EXPECT_TRUE(cfg.tls_enabled);
}

TEST(MqttClientTlsConfigTests, TlsCertPathCanBeSet) {
    MqttClientConfig cfg;
    cfg.tls_cert_path = "/etc/certs/client.pem";
    EXPECT_EQ(cfg.tls_cert_path, "/etc/certs/client.pem");
}

TEST(MqttClientTlsConfigTests, TlsKeyPathCanBeSet) {
    MqttClientConfig cfg;
    cfg.tls_key_path = "/etc/certs/client.key";
    EXPECT_EQ(cfg.tls_key_path, "/etc/certs/client.key");
}

TEST(MqttClientTlsConfigTests, TlsCaPathCanBeSet) {
    MqttClientConfig cfg;
    cfg.tls_ca_path = "/etc/certs/ca-bundle.pem";
    EXPECT_EQ(cfg.tls_ca_path, "/etc/certs/ca-bundle.pem");
}

TEST(MqttClientTlsConfigTests, TlsPortConventionIs8883) {
    // RFC convention: plain MQTT = 1883, MQTT over TLS = 8883
    MqttClientConfig cfg;
    cfg.tls_enabled = true;
    cfg.broker_port  = 8883;
    EXPECT_EQ(cfg.broker_port, uint16_t{8883});
    EXPECT_TRUE(cfg.tls_enabled);
}

TEST(MqttClientTlsConfigTests, TlsConfigPreservedInService) {
    MqttClientConfig cfg;
    cfg.tls_enabled   = true;
    cfg.broker_port   = 8883;
    cfg.tls_cert_path = "/certs/c.pem";
    cfg.tls_key_path  = "/certs/c.key";
    cfg.tls_ca_path   = "/certs/ca.pem";

    MqttClientService svc(cfg);
    const auto& stored = svc.getConfig();
    EXPECT_TRUE(stored.tls_enabled);
    EXPECT_EQ(stored.broker_port, uint16_t{8883});
    EXPECT_EQ(stored.tls_cert_path, "/certs/c.pem");
    EXPECT_EQ(stored.tls_key_path, "/certs/c.key");
    EXPECT_EQ(stored.tls_ca_path, "/certs/ca.pem");
}

TEST(MqttClientTlsConfigTests, TlsDisabledDoesNotChangePort) {
    // Leaving tls_enabled=false should keep the default plain-text port.
    MqttClientConfig cfg;
    EXPECT_FALSE(cfg.tls_enabled);
    EXPECT_EQ(cfg.broker_port, uint16_t{1883});
}

TEST(MqttClientTlsConfigTests, MutualTlsBothPathsSet) {
    // A valid mutual-TLS config must supply both cert and key paths.
    MqttClientConfig cfg;
    cfg.tls_enabled   = true;
    cfg.tls_cert_path = "/certs/client.crt";
    cfg.tls_key_path  = "/certs/client.key";
    // No CA path → server cert will not be verified (verify_none).
    EXPECT_FALSE(cfg.tls_cert_path.empty());
    EXPECT_FALSE(cfg.tls_key_path.empty());
    EXPECT_TRUE(cfg.tls_ca_path.empty());
}

TEST(MqttClientTlsConfigTests, CaOnlyConfigEnablesPeerVerification) {
    // CA path alone enables server-cert verification without mutual TLS.
    MqttClientConfig cfg;
    cfg.tls_enabled = true;
    cfg.tls_ca_path = "/certs/ca.pem";
    EXPECT_FALSE(cfg.tls_ca_path.empty());
    EXPECT_TRUE(cfg.tls_cert_path.empty());
    EXPECT_TRUE(cfg.tls_key_path.empty());
}

#ifdef THEMIS_ENABLE_MQTT_TLS
// When compiled with TLS support, verify that starting a service with
// tls_enabled=true and a non-existent CA path causes a graceful failure
// (scheduleReconnect) rather than a crash or exception propagation.
TEST(MqttClientTlsRuntimeTests, StartWithBadCaPathDoesNotCrash) {
    MqttClientConfig cfg;
    cfg.broker_host  = "127.0.0.1";
    cfg.broker_port  = 8883;
    cfg.tls_enabled  = true;
    cfg.tls_ca_path  = "/nonexistent/path/ca.pem";
    cfg.retry.maxRetries = 0; // do not retry so the test terminates quickly

    MqttClientService svc(cfg);
    // start() is non-blocking and must not throw
    EXPECT_NO_THROW(svc.start());
    // Brief wait so the I/O thread has time to attempt a connection
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    svc.stop();
    // The service must not be in a connected state
    EXPECT_FALSE(svc.isConnected());
}

TEST(MqttClientTlsRuntimeTests, StartWithTlsDisabledIsUnaffectedByTlsPaths) {
    // Even if tls_* paths are set, they are ignored when tls_enabled=false.
    MqttClientConfig cfg;
    cfg.broker_host   = "127.0.0.1";
    cfg.broker_port   = 1883;
    cfg.tls_enabled   = false;
    cfg.tls_ca_path   = "/nonexistent/ca.pem"; // should be ignored
    cfg.retry.maxRetries = 0;

    MqttClientService svc(cfg);
    EXPECT_NO_THROW(svc.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    svc.stop();
    EXPECT_FALSE(svc.isConnected());
}

TEST(MqttClientTlsRuntimeTests, EmptyTlsCaPathLogsVerifyNoneFallback) {
    auto previous_logger = spdlog::default_logger();
    auto previous_level = spdlog::get_level();

    std::ostringstream capture = {};
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(capture);
    auto logger = std::make_shared<spdlog::logger>("mqtt_tls_verify_none_test", sink);
    logger->set_pattern("%v");
    logger->set_level(spdlog::level::warn);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::warn);

    boost::asio::io_context io_ctx;
    boost::asio::ip::tcp::acceptor acceptor(
        io_ctx,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const auto port = acceptor.local_endpoint().port();

    std::thread accept_thread([&acceptor]() {
        boost::system::error_code ec;
        boost::asio::ip::tcp::socket socket(acceptor.get_executor());
        acceptor.accept(socket, ec);
        if (!ec) {
            // Keep the accepted TCP connection alive briefly so TLS handshake
            // setup reaches the verify_none warning path deterministically.
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    });
    ScopedThreadJoiner accept_thread_joiner(accept_thread);

    MqttClientConfig cfg;
    cfg.broker_host = "127.0.0.1";
    cfg.broker_port = port;
    cfg.tls_enabled = true;
    cfg.tls_ca_path = "";
    cfg.retry.maxRetries = 0;

    MqttClientService svc(cfg);
    EXPECT_NO_THROW(svc.start());

    // doHandshake() runs on the internal I/O thread after async TCP connect.
    // Poll briefly for the fallback warning to appear in captured logs.
    // Total wait budget: 20 * 50ms = 1000ms.
    constexpr int max_log_poll_attempts = 20;
    constexpr auto log_poll_interval = std::chrono::milliseconds(50);
    bool saw_fallback_log = false;
    for (int i = 0; i < max_log_poll_attempts; ++i) {
        std::this_thread::sleep_for(log_poll_interval);
        const auto current_logs = capture.str();
        if (current_logs.find(kMqttTlsVerifyNoneFallbackLogPrefix) != std::string::npos) {
            saw_fallback_log = true;
            break;
        }
    }

    svc.stop();
    boost::system::error_code cleanup_ec;
    acceptor.close(cleanup_ec);

    const auto logs = capture.str();
    EXPECT_TRUE(saw_fallback_log) << logs;
    EXPECT_NE(logs.find("tls_ca_path is empty"), std::string::npos);

    spdlog::set_default_logger(previous_logger);
    spdlog::set_level(previous_level);
}
#endif // THEMIS_ENABLE_MQTT_TLS

#endif // THEMIS_ENABLE_MQTT
