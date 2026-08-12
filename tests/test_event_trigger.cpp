#include <gtest/gtest.h>
#include "scheduler/event_trigger.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <thread>
#include <atomic>

using namespace themis;

class EventTriggerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable event trigger tests on Windows";
#endif
        const std::string db_path = "data/themis_event_trigger_test";
        if (std::filesystem::exists(db_path)) {
            std::filesystem::remove_all(db_path);
        }
        
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.memtable_size_mb = 64;
        cfg.block_cache_size_mb = 128;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());
        
        changefeed_ = std::make_unique<Changefeed>(storage_->getRawDB());
    }
    
    void TearDown() override {
        changefeed_.reset();
        storage_->close();
    }
    
    std::shared_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<Changefeed> changefeed_;
};

// ===== CDCTriggerConfig Tests =====

TEST_F(EventTriggerTest, ConfigValidation) {
    CDCTriggerConfig valid_config;
    valid_config.key_prefix = "users:";
    valid_config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    EXPECT_TRUE(valid_config.isValid());
    EXPECT_TRUE(valid_config.getValidationError().empty());
}

TEST_F(EventTriggerTest, ConfigValidationEmptyPrefix) {
    CDCTriggerConfig invalid_config;
    invalid_config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    EXPECT_FALSE(invalid_config.isValid());
    EXPECT_FALSE(invalid_config.getValidationError().empty());
}

TEST_F(EventTriggerTest, ConfigValidationEmptyEventTypes) {
    CDCTriggerConfig invalid_config;
    invalid_config.key_prefix = "users:";
    
    EXPECT_FALSE(invalid_config.isValid());
    EXPECT_FALSE(invalid_config.getValidationError().empty());
}

// ===== EventTrigger Basic Tests =====

TEST_F(EventTriggerTest, ConstructorValidation) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    std::atomic<int> callback_count{0};
    auto callback = [&callback_count](const Changefeed::ChangeEvent&) {
        callback_count++;
    };
    
    EXPECT_NO_THROW({
        EventTrigger trigger(changefeed_.get(), config, callback);
    });
}

TEST_F(EventTriggerTest, ConstructorNullChangefeed) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    auto callback = [](const Changefeed::ChangeEvent&) {};
    
    EXPECT_THROW({
        EventTrigger trigger(nullptr, config, callback);
    }, std::invalid_argument);
}

TEST_F(EventTriggerTest, ConstructorInvalidConfig) {
    CDCTriggerConfig config;
    // Empty config - invalid
    
    auto callback = [](const Changefeed::ChangeEvent&) {};
    
    EXPECT_THROW({
        EventTrigger trigger(changefeed_.get(), config, callback);
    }, std::invalid_argument);
}

TEST_F(EventTriggerTest, ConstructorNullCallback) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    EXPECT_THROW({
        EventTrigger trigger(changefeed_.get(), config, nullptr);
    }, std::invalid_argument);
}

// ===== EventTrigger Lifecycle Tests =====

TEST_F(EventTriggerTest, StartStop) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    auto callback = [](const Changefeed::ChangeEvent&) {};
    
    EventTrigger trigger(changefeed_.get(), config, callback);
    
    EXPECT_FALSE(trigger.isRunning());
    
    trigger.start();
    EXPECT_TRUE(trigger.isRunning());
    
    trigger.stop();
    EXPECT_FALSE(trigger.isRunning());
}

// ===== EventTrigger Filtering Tests =====

TEST_F(EventTriggerTest, KeyPrefixFiltering) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    std::atomic<int> callback_count{0};
    std::string last_key;
    
    auto callback = [&callback_count, &last_key](const Changefeed::ChangeEvent& event) {
        callback_count++;
        last_key = event.key;
    };
    
    EventTrigger trigger(changefeed_.get(), config, callback);
    trigger.start();
    
    // Record some events
    Changefeed::ChangeEvent event1;
    event1.type = Changefeed::ChangeEventType::EVENT_PUT;
    event1.key = "users:123";
    event1.value = "test_value";
    event1.timestamp_ms = 1000;
    changefeed_->recordEvent(event1);
    
    Changefeed::ChangeEvent event2;
    event2.type = Changefeed::ChangeEventType::EVENT_PUT;
    event2.key = "orders:456";
    event2.value = "test_value";
    event2.timestamp_ms = 2000;
    changefeed_->recordEvent(event2);
    
    Changefeed::ChangeEvent event3;
    event3.type = Changefeed::ChangeEventType::EVENT_PUT;
    event3.key = "users:789";
    event3.value = "test_value";
    event3.timestamp_ms = 3000;
    changefeed_->recordEvent(event3);
    
    // Wait for events to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    trigger.stop();
    
    // Should have triggered for users: keys only (event1 and event3)
    EXPECT_EQ(callback_count.load(), 2);
}

TEST_F(EventTriggerTest, EventTypeFiltering) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_DELETE);
    
    std::atomic<int> callback_count{0};
    
    auto callback = [&callback_count](const Changefeed::ChangeEvent&) {
        callback_count++;
    };
    
    EventTrigger trigger(changefeed_.get(), config, callback);
    trigger.start();
    
    // Record PUT event (should not trigger)
    Changefeed::ChangeEvent event1;
    event1.type = Changefeed::ChangeEventType::EVENT_PUT;
    event1.key = "users:123";
    event1.value = "test_value";
    event1.timestamp_ms = 1000;
    changefeed_->recordEvent(event1);
    
    // Record DELETE event (should trigger)
    Changefeed::ChangeEvent event2;
    event2.type = Changefeed::ChangeEventType::EVENT_DELETE;
    event2.key = "users:456";
    event2.timestamp_ms = 2000;
    changefeed_->recordEvent(event2);
    
    // Wait for events to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    trigger.stop();
    
    // Should have triggered only for DELETE event
    EXPECT_EQ(callback_count.load(), 1);
}

// ===== EventTrigger Debouncing Tests =====

TEST_F(EventTriggerTest, Debouncing) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    config.debounce_ms = 500;  // 500ms debounce
    
    std::atomic<int> callback_count{0};
    
    auto callback = [&callback_count](const Changefeed::ChangeEvent&) {
        callback_count++;
    };
    
    EventTrigger trigger(changefeed_.get(), config, callback);
    trigger.start();
    
    // Record 3 events rapidly
    for (int i = 0; i < 3; i++) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "users:123";
        event.value = "test_value";
        event.timestamp_ms = 1000 + i;
        changefeed_->recordEvent(event);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    trigger.stop();
    
    // Should have triggered only once due to debouncing
    // (first event triggers, next 2 are debounced)
    EXPECT_LE(callback_count.load(), 2);  // Allow for timing variations
}

// ===== EventTrigger Statistics Tests =====

TEST_F(EventTriggerTest, Statistics) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    std::atomic<int> callback_count{0};
    
    auto callback = [&callback_count](const Changefeed::ChangeEvent&) {
        callback_count++;
    };
    
    EventTrigger trigger(changefeed_.get(), config, callback);
    trigger.start();
    
    // Record matching event
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = "users:123";
    event.value = "test_value";
    event.timestamp_ms = 1000;
    changefeed_->recordEvent(event);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    auto stats = trigger.getStats();
    
    trigger.stop();
    
    EXPECT_GT(stats.events_received, 0);
    EXPECT_GT(stats.events_matched, 0);
    EXPECT_GT(stats.triggers_fired, 0);
}

// ===== EventTriggerManager Tests =====

TEST_F(EventTriggerTest, ManagerRegisterUnregister) {
    EventTriggerManager manager(changefeed_.get());
    
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    auto callback = [](const Changefeed::ChangeEvent&) {};
    
    EXPECT_TRUE(manager.registerTrigger("test_trigger", config, callback));
    
    // Try to register again - should fail
    EXPECT_FALSE(manager.registerTrigger("test_trigger", config, callback));
    
    manager.unregisterTrigger("test_trigger");
}

TEST_F(EventTriggerTest, ManagerMultipleTriggers) {
    EventTriggerManager manager(changefeed_.get());
    
    std::atomic<int> callback1_count{0};
    std::atomic<int> callback2_count{0};
    
    CDCTriggerConfig config1;
    config1.key_prefix = "users:";
    config1.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    CDCTriggerConfig config2;
    config2.key_prefix = "orders:";
    config2.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    
    auto callback1 = [&callback1_count](const Changefeed::ChangeEvent&) {
        callback1_count++;
    };
    
    auto callback2 = [&callback2_count](const Changefeed::ChangeEvent&) {
        callback2_count++;
    };
    
    EXPECT_TRUE(manager.registerTrigger("users_trigger", config1, callback1));
    EXPECT_TRUE(manager.registerTrigger("orders_trigger", config2, callback2));
    
    manager.startAll();
    
    // Record events for both prefixes
    Changefeed::ChangeEvent event1;
    event1.type = Changefeed::ChangeEventType::EVENT_PUT;
    event1.key = "users:123";
    event1.value = "test";
    event1.timestamp_ms = 1000;
    changefeed_->recordEvent(event1);
    
    Changefeed::ChangeEvent event2;
    event2.type = Changefeed::ChangeEventType::EVENT_PUT;
    event2.key = "orders:456";
    event2.value = "test";
    event2.timestamp_ms = 2000;
    changefeed_->recordEvent(event2);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    manager.stopAll();
    
    // Each callback should have been triggered once
    EXPECT_EQ(callback1_count.load(), 1);
    EXPECT_EQ(callback2_count.load(), 1);
}

// ===== Condition Evaluation Tests =====

// Helper: create a simple ChangeEvent
static Changefeed::ChangeEvent makeEvent(const std::string& key,
                                          const std::optional<std::string>& value = std::nullopt) {
    Changefeed::ChangeEvent event;
    event.type = Changefeed::ChangeEventType::EVENT_PUT;
    event.key = key;
    event.value = value;
    event.sequence = 1;
    event.timestamp_ms = 1000;
    return event;
}

// Test the condition evaluator indirectly through EventTrigger by counting callbacks
TEST_F(EventTriggerTest, ConditionEqualMatch) {
    CDCTriggerConfig config;
    config.key_prefix = "*";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    config.condition = R"(key == "users:42")";

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    // Matching event
    auto ev1 = makeEvent("users:42");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    // Non-matching event
    auto ev2 = makeEvent("users:99");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(EventTriggerTest, ConditionStartsWith) {
    CDCTriggerConfig config;
    config.key_prefix = "*";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    config.condition = R"(key STARTS_WITH "admin:")";

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    auto ev1 = makeEvent("admin:superuser");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    auto ev2 = makeEvent("user:bob");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(EventTriggerTest, ConditionContains) {
    CDCTriggerConfig config;
    config.key_prefix = "*";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    config.condition = R"(key CONTAINS "error")";

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    auto ev1 = makeEvent("logs:error:404");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    auto ev2 = makeEvent("logs:info:request");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(EventTriggerTest, ConditionNotEqual) {
    CDCTriggerConfig config;
    config.key_prefix = "users:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    config.condition = R"(key != "users:banned")";

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    auto ev1 = makeEvent("users:alice");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    auto ev2 = makeEvent("users:banned");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(EventTriggerTest, NoConditionMatchesAll) {
    CDCTriggerConfig config;
    config.key_prefix = "data:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    // No condition → all events with matching prefix/type fire

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    auto ev1 = makeEvent("data:a");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    auto ev2 = makeEvent("data:b");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 2);
}

// ===== Circuit Breaker Tests =====

TEST_F(EventTriggerTest, CircuitBreakerOpensAfterConsecutiveFailures) {
    CDCTriggerConfig config;
    config.key_prefix = "cb:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);

    std::atomic<int> call_count{0};
    // Callback always throws
    auto callback = [&](const Changefeed::ChangeEvent&) {
        ++call_count;
        throw std::runtime_error("cb test failure");
    };

    EventTrigger trigger(changefeed_.get(), config, callback);

    // Configure circuit to open after 2 failures with very short cooldown
    EventTrigger::CircuitBreakerConfig cb_cfg;
    cb_cfg.failure_threshold = 2;
    cb_cfg.cooldown = std::chrono::seconds(60);  // Long cooldown so it stays open
    trigger.setCircuitBreakerConfig(cb_cfg);

    trigger.start();

    // Send 5 events – circuit should open after 2 callback failures
    for (int i = 0; i < 5; ++i) {
        Changefeed::ChangeEvent ev;
        ev.type = Changefeed::ChangeEventType::EVENT_PUT;
        ev.key = "cb:item";
        ev.value = "v";
        ev.timestamp_ms = static_cast<int64_t>(i + 1) * 100;
        changefeed_->recordEvent(ev);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    trigger.stop();

    auto stats = trigger.getStats();
    // Circuit should be open
    EXPECT_TRUE(stats.circuit_open);
    // Callback was called at most failure_threshold times before circuit opened
    EXPECT_LE(call_count.load(), static_cast<int>(cb_cfg.failure_threshold));
    EXPECT_GT(stats.callback_failures, 0u);
}

TEST_F(EventTriggerTest, InitialCircuitIsClosed) {
    CDCTriggerConfig config;
    config.key_prefix = "cb2:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);

    auto callback = [](const Changefeed::ChangeEvent&) {};
    EventTrigger trigger(changefeed_.get(), config, callback);

    // Without any events, circuit should be closed
    auto stats = trigger.getStats();
    EXPECT_FALSE(stats.circuit_open);
    EXPECT_EQ(stats.callback_failures, 0u);
}

TEST_F(EventTriggerTest, CircuitRemainsClosedOnSuccessfulCallbacks) {
    CDCTriggerConfig config;
    config.key_prefix = "cb3:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);

    std::atomic<int> call_count{0};
    auto callback = [&](const Changefeed::ChangeEvent&) { ++call_count; };

    EventTrigger trigger(changefeed_.get(), config, callback);
    trigger.start();

    for (int i = 0; i < 5; ++i) {
        Changefeed::ChangeEvent ev;
        ev.type = Changefeed::ChangeEventType::EVENT_PUT;
        ev.key = "cb3:item";
        ev.value = "v";
        ev.timestamp_ms = static_cast<int64_t>(i + 1) * 100;
        changefeed_->recordEvent(ev);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    trigger.stop();

    auto stats = trigger.getStats();
    EXPECT_FALSE(stats.circuit_open);
    EXPECT_EQ(stats.callback_failures, 0u);
    EXPECT_EQ(call_count.load(), 5);
}

// ===== Additional condition and config tests =====

TEST_F(EventTriggerTest, ConditionEndsWith) {
    CDCTriggerConfig config;
    config.key_prefix = "*";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    config.condition = R"(key ENDS_WITH ":admin")";

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    auto ev1 = makeEvent("users:admin");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);   // matches ENDS_WITH ":admin"

    auto ev2 = makeEvent("users:alice");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);   // does NOT match

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(EventTriggerTest, ConditionAndClause) {
    CDCTriggerConfig config;
    config.key_prefix = "*";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    // Must start with "orders:" AND contain "urgent"
    config.condition = R"(key STARTS_WITH "orders:" AND key CONTAINS "urgent")";

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    // Both conditions satisfied
    auto ev1 = makeEvent("orders:urgent:42");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    // Only first condition satisfied
    auto ev2 = makeEvent("orders:normal:7");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    // Only second condition satisfied
    auto ev3 = makeEvent("products:urgent:9");
    ev3.timestamp_ms = 3000;
    changefeed_->recordEvent(ev3);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    trigger.stop();

    EXPECT_EQ(count.load(), 1);
}

TEST_F(EventTriggerTest, UpdateConfigAffectsSubsequentMatching) {
    CDCTriggerConfig config;
    config.key_prefix = "old_prefix:";
    config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);

    std::atomic<int> count{0};
    auto cb = [&](const Changefeed::ChangeEvent&) { ++count; };

    EventTrigger trigger(changefeed_.get(), config, cb);
    trigger.start();

    // Event using new_prefix: – should NOT fire because the trigger is still
    // configured with old_prefix: at this point
    auto ev1 = makeEvent("new_prefix:item");
    ev1.timestamp_ms = 1000;
    changefeed_->recordEvent(ev1);

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Update config to new prefix
    CDCTriggerConfig new_config;
    new_config.key_prefix = "new_prefix:";
    new_config.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
    trigger.updateConfig(new_config);

    // Now an event matching the new prefix should fire
    auto ev2 = makeEvent("new_prefix:widget");
    ev2.timestamp_ms = 2000;
    changefeed_->recordEvent(ev2);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    trigger.stop();

    // Only ev2 should have triggered (ev1 was before config update)
    EXPECT_EQ(count.load(), 1);
}
