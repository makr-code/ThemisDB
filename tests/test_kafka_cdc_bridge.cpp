/**
 * @file test_kafka_cdc_bridge.cpp
 * @brief Unit tests for KafkaCDCProducer StartFn/PublishFn bridge (STUB #98).
 *
 * Verifies that the start/publish fn injection works correctly in
 * non-Kafka builds (THEMIS_ENABLE_KAFKA not defined):
 *   CDC-KFK-01  No fn set              → start()/publish() return false.
 *   CDC-KFK-02  Fns return true        → start()/publish() return true.
 *   CDC-KFK-03  Fns throw exception    → fail-closed; return false.
 *
 * Tests run in builds WITHOUT THEMIS_ENABLE_KAFKA.  In Kafka builds the
 * injected fns are unreachable and tests are skipped.
 */

#include <gtest/gtest.h>
#include "cdc/kafka_cdc_producer.h"

#include <stdexcept>

using namespace themis::cdc;

class KafkaCdcBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        KafkaCDCProducer::setStartFn({});    // restore clean state
        KafkaCDCProducer::setPublishFn({});
    }
};

// ── CDC-KFK-01 ───────────────────────────────────────────────────────────────
// With no fn registered, start() and publish() return false (no-op fallback).
TEST_F(KafkaCdcBridgeTest, NoFnReturnsFalse) {
#ifdef THEMIS_ENABLE_KAFKA
    GTEST_SKIP() << "THEMIS_ENABLE_KAFKA is ON — real Kafka path active; skip.";
#endif
    KafkaCDCProducer::setStartFn({});
    KafkaCDCProducer::setPublishFn({});

    KafkaCDCProducer producer(nullptr);

    EXPECT_FALSE(producer.start())
        << "start() must return false when no StartFn is registered";

    themis::Changefeed::ChangeEvent evt;
    evt.key  = "doc1";
    evt.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
    EXPECT_FALSE(producer.publish(evt))
        << "publish() must return false when no PublishFn is registered";
}

// ── CDC-KFK-02 ───────────────────────────────────────────────────────────────
// Injected fns returning true propagate that result from start()/publish().
TEST_F(KafkaCdcBridgeTest, SuccessFnsReturnTrue) {
#ifdef THEMIS_ENABLE_KAFKA
    GTEST_SKIP() << "THEMIS_ENABLE_KAFKA is ON — real Kafka path active; skip.";
#endif

    bool start_called   = false;
    bool publish_called = false;
    std::string received_key = {};

    KafkaCDCProducer::setStartFn([&]() -> bool {
        start_called = true;
        return true;
    });
    KafkaCDCProducer::setPublishFn([&](const themis::Changefeed::ChangeEvent& e) -> bool {
        publish_called = true;
        received_key   = e.key;
        return true;
    });

    KafkaCDCProducer producer(nullptr);

    EXPECT_TRUE(producer.start())   << "start() must return true from injected fn";
    EXPECT_TRUE(start_called)       << "StartFn must have been invoked";

    themis::Changefeed::ChangeEvent evt;
    evt.key  = "order-42";
    evt.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
    EXPECT_TRUE(producer.publish(evt)) << "publish() must return true from injected fn";
    EXPECT_TRUE(publish_called)         << "PublishFn must have been invoked";
    EXPECT_EQ(received_key, "order-42") << "PublishFn must receive the correct event";
}

// ── CDC-KFK-03 ───────────────────────────────────────────────────────────────
// Throwing fns are fail-closed: no exception escapes, start()/publish() return false.
TEST_F(KafkaCdcBridgeTest, ThrowingFnsAreFailClosed) {
#ifdef THEMIS_ENABLE_KAFKA
    GTEST_SKIP() << "THEMIS_ENABLE_KAFKA is ON — real Kafka path active; skip.";
#endif

    KafkaCDCProducer::setStartFn([]() -> bool {
        throw std::runtime_error("Kafka broker unavailable");
    });
    KafkaCDCProducer::setPublishFn([](const themis::Changefeed::ChangeEvent&) -> bool {
        throw std::runtime_error("Kafka publish failed");
    });

    KafkaCDCProducer producer(nullptr);

    EXPECT_NO_THROW({
        bool ok = producer.start();
        EXPECT_FALSE(ok) << "start() must return false when fn throws";
    });

    themis::Changefeed::ChangeEvent evt;
    evt.key = "k1";
    evt.type = themis::Changefeed::ChangeEventType::EVENT_PUT;
    EXPECT_NO_THROW({
        bool ok = producer.publish(evt);
        EXPECT_FALSE(ok) << "publish() must return false when fn throws";
    });
}
