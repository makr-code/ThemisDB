// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_notification_webhook.cpp
 * @brief Unit tests for NotificationWebhook (Phase 3 – Slack/PagerDuty).
 *
 * All tests use an injectable HttpSendFunc so no real network calls are made.
 * Tests verify JSON payload structure for both Slack and PagerDuty, channel
 * selection logic, error handling, and the notify() dispatch path.
 */

#include <gtest/gtest.h>

#include "updates/notification_webhook.h"

#include <atomic>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace themis::updates;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Capture URL + body of the most-recent HTTP POST.
struct CapturedPost {
    std::string url;
    std::string body;
    bool        called{false};
};

/// Returns a sender that records the last call into @p cap and always succeeds.
static NotificationWebhook::HttpSendFunc makeSender(CapturedPost& cap,
                                                     bool succeed = true) {
    return [&cap, succeed](const std::string& url,
                            const std::string& body) -> bool {
        cap.url    = url;
        cap.body   = body;
        cap.called = true;
        return succeed;
    };
}

/// Build a minimal success payload.
static UpdateEventPayload makeSuccessPayload(const std::string& version = "1.6.0") {
    UpdateEventPayload p;
    p.event        = UpdateEvent::UPDATE_SUCCESS;
    p.version      = version;
    p.from_version = "1.5.0";
    p.files_updated = {"bin/themis_server", "lib/libthemis.so"};
    return p;
}

/// Build a failure payload.
static UpdateEventPayload makeFailPayload(const std::string& version = "1.6.0") {
    UpdateEventPayload p;
    p.event         = UpdateEvent::UPDATE_FAILED;
    p.version       = version;
    p.error_message = "checksum mismatch";
    p.rollback_id   = "rb-20260301-001";
    return p;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

class NotificationWebhookConstructTest : public ::testing::Test {};

TEST_F(NotificationWebhookConstructTest, DefaultConstruct_DoesNotThrow) {
    EXPECT_NO_THROW(NotificationWebhook w);
}

TEST_F(NotificationWebhookConstructTest, SetNullSender_Throws) {
    NotificationWebhook w;
    EXPECT_THROW(w.setHttpSender(nullptr), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// No channels configured
// ---------------------------------------------------------------------------

class NoChannelTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;
    CapturedPost        cap_;
    void SetUp() override {
        webhook_.setHttpSender(makeSender(cap_));
    }
};

TEST_F(NoChannelTest, Notify_ReturnsFalse_WhenNoChannelsConfigured) {
    EXPECT_FALSE(webhook_.notify(makeSuccessPayload()));
}

TEST_F(NoChannelTest, Notify_NoHttpCall_WhenNoChannelsConfigured) {
    webhook_.notify(makeSuccessPayload());
    EXPECT_FALSE(cap_.called);
}

// ---------------------------------------------------------------------------
// SlackConfig validation
// ---------------------------------------------------------------------------

class SlackConfigTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;
    CapturedPost        cap_;
    void SetUp() override {
        webhook_.setHttpSender(makeSender(cap_));
    }
};

TEST_F(SlackConfigTest, EmptyWebhookUrl_ChannelNotActivated) {
    webhook_.setSlackConfig(SlackConfig{/* webhook_url = "" */});
    EXPECT_FALSE(webhook_.notify(makeSuccessPayload()));
    EXPECT_FALSE(cap_.called);
}

TEST_F(SlackConfigTest, ValidConfig_ActivatesChannel) {
    webhook_.setSlackConfig({"https://hooks.slack.com/services/TEST"});
    EXPECT_TRUE(webhook_.notify(makeSuccessPayload()));
    EXPECT_TRUE(cap_.called);
}

TEST_F(SlackConfigTest, PostTargetsConfiguredUrl) {
    const std::string url = "https://hooks.slack.com/services/MY_HOOK";
    webhook_.setSlackConfig({url});
    webhook_.notify(makeSuccessPayload());
    EXPECT_EQ(cap_.url, url);
}

// ---------------------------------------------------------------------------
// PagerDutyConfig validation
// ---------------------------------------------------------------------------

class PagerDutyConfigTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;
    CapturedPost        cap_;
    void SetUp() override {
        webhook_.setHttpSender(makeSender(cap_));
    }
};

TEST_F(PagerDutyConfigTest, EmptyRoutingKey_ChannelNotActivated) {
    webhook_.setPagerDutyConfig(PagerDutyConfig{/* routing_key = "" */});
    EXPECT_FALSE(webhook_.notify(makeSuccessPayload()));
    EXPECT_FALSE(cap_.called);
}

TEST_F(PagerDutyConfigTest, ValidConfig_ActivatesChannel) {
    webhook_.setPagerDutyConfig({"routingkey123abc"});
    EXPECT_TRUE(webhook_.notify(makeSuccessPayload()));
    EXPECT_TRUE(cap_.called);
}

TEST_F(PagerDutyConfigTest, PostTargetsDefaultEventsApiUrl) {
    webhook_.setPagerDutyConfig({"routingkey123abc"});
    webhook_.notify(makeSuccessPayload());
    EXPECT_EQ(cap_.url, "https://events.pagerduty.com/v2/enqueue");
}

TEST_F(PagerDutyConfigTest, PostTargetsCustomEventsApiUrl) {
    PagerDutyConfig cfg;
    cfg.routing_key   = "routingkey123abc";
    cfg.events_api_url = "https://my-proxy/v2/enqueue";
    webhook_.setPagerDutyConfig(cfg);
    webhook_.notify(makeSuccessPayload());
    EXPECT_EQ(cap_.url, "https://my-proxy/v2/enqueue");
}

// ---------------------------------------------------------------------------
// Slack payload structure
// ---------------------------------------------------------------------------

class SlackPayloadTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;

    void SetUp() override {
        CapturedPost dummy;
        webhook_.setHttpSender(makeSender(dummy));
        webhook_.setSlackConfig({"https://hooks.slack.com/services/TEST"});
    }
};

TEST_F(SlackPayloadTest, SuccessPayload_IsValidJson) {
    const std::string body = webhook_.buildSlackPayload(makeSuccessPayload());
    EXPECT_NO_THROW({
        auto parsed = json::parse(body);
        static_cast<void>(parsed);
    });
}

TEST_F(SlackPayloadTest, SuccessPayload_ContainsVersion) {
    const json j = json::parse(
        webhook_.buildSlackPayload(makeSuccessPayload("2.0.0")));
    const std::string text = j.value("text", "");
    EXPECT_NE(text.find("2.0.0"), std::string::npos);
}

TEST_F(SlackPayloadTest, SuccessPayload_ColorIsGood) {
    const json j = json::parse(
        webhook_.buildSlackPayload(makeSuccessPayload()));
    const std::string color =
        j.at("attachments").at(0).value("color", "");
    EXPECT_EQ(color, "good");
}

TEST_F(SlackPayloadTest, FailurePayload_ColorIsDanger) {
    const json j = json::parse(
        webhook_.buildSlackPayload(makeFailPayload()));
    const std::string color =
        j.at("attachments").at(0).value("color", "");
    EXPECT_EQ(color, "danger");
}

TEST_F(SlackPayloadTest, RollbackSuccess_ColorIsWarning) {
    UpdateEventPayload p;
    p.event   = UpdateEvent::ROLLBACK_SUCCESS;
    p.version = "1.5.0";
    const json j = json::parse(webhook_.buildSlackPayload(p));
    EXPECT_EQ(j.at("attachments").at(0).value("color", ""), "warning");
}

TEST_F(SlackPayloadTest, UsernameInPayload) {
    SlackConfig cfg;
    cfg.webhook_url = "https://hooks.slack.com/services/TEST";
    cfg.username    = "MyBot";
    webhook_.setSlackConfig(cfg);
    const json j = json::parse(webhook_.buildSlackPayload(makeSuccessPayload()));
    EXPECT_EQ(j.value("username", ""), "MyBot");
}

TEST_F(SlackPayloadTest, ChannelIncludedWhenSet) {
    SlackConfig cfg;
    cfg.webhook_url = "https://hooks.slack.com/services/TEST";
    cfg.channel     = "#ops-alerts";
    webhook_.setSlackConfig(cfg);
    const json j = json::parse(webhook_.buildSlackPayload(makeSuccessPayload()));
    EXPECT_EQ(j.value("channel", ""), "#ops-alerts");
}

TEST_F(SlackPayloadTest, ChannelAbsentWhenEmpty) {
    const json j = json::parse(webhook_.buildSlackPayload(makeSuccessPayload()));
    EXPECT_FALSE(j.contains("channel"));
}

TEST_F(SlackPayloadTest, ErrorMessageAppearsInFields_OnFailure) {
    const json j = json::parse(webhook_.buildSlackPayload(makeFailPayload()));
    const auto& fields = j.at("attachments").at(0).at("fields");
    bool found = false;
    for (const auto& f : fields) {
        if (f.value("value", "").find("checksum mismatch") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(SlackPayloadTest, FilesUpdatedAppearsInFields_OnSuccess) {
    const json j = json::parse(webhook_.buildSlackPayload(makeSuccessPayload()));
    const auto& fields = j.at("attachments").at(0).at("fields");
    bool found = false;
    for (const auto& f : fields) {
        if (f.value("title", "") == "Files Updated") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ---------------------------------------------------------------------------
// PagerDuty payload structure
// ---------------------------------------------------------------------------

class PagerDutyPayloadTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;

    void SetUp() override {
        CapturedPost dummy;
        webhook_.setHttpSender(makeSender(dummy));
        webhook_.setPagerDutyConfig({"routingkey123abc"});
    }
};

TEST_F(PagerDutyPayloadTest, SuccessPayload_IsValidJson) {
    const std::string body =
        webhook_.buildPagerDutyPayload(makeSuccessPayload());
    EXPECT_NO_THROW({
        auto parsed = json::parse(body);
        static_cast<void>(parsed);
    });
}

TEST_F(PagerDutyPayloadTest, RoutingKeyInPayload) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeSuccessPayload()));
    EXPECT_EQ(j.value("routing_key", ""), "routingkey123abc");
}

TEST_F(PagerDutyPayloadTest, SuccessEvent_ActionIsResolve) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeSuccessPayload()));
    EXPECT_EQ(j.value("event_action", ""), "resolve");
}

TEST_F(PagerDutyPayloadTest, FailureEvent_ActionIsTrigger) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeFailPayload()));
    EXPECT_EQ(j.value("event_action", ""), "trigger");
}

TEST_F(PagerDutyPayloadTest, RollbackSuccess_ActionIsResolve) {
    UpdateEventPayload p;
    p.event   = UpdateEvent::ROLLBACK_SUCCESS;
    p.version = "1.5.0";
    const json j = json::parse(webhook_.buildPagerDutyPayload(p));
    EXPECT_EQ(j.value("event_action", ""), "resolve");
}

TEST_F(PagerDutyPayloadTest, RollbackFailed_ActionIsTrigger) {
    UpdateEventPayload p;
    p.event   = UpdateEvent::ROLLBACK_FAILED;
    p.version = "1.5.0";
    const json j = json::parse(webhook_.buildPagerDutyPayload(p));
    EXPECT_EQ(j.value("event_action", ""), "trigger");
}

TEST_F(PagerDutyPayloadTest, DedupKeyContainsVersion) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeSuccessPayload("3.0.0")));
    const std::string dk = j.value("dedup_key", "");
    EXPECT_NE(dk.find("3.0.0"), std::string::npos);
}

TEST_F(PagerDutyPayloadTest, FailureSeverityIsCritical) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeFailPayload()));
    EXPECT_EQ(j.at("payload").value("severity", ""), "critical");
}

TEST_F(PagerDutyPayloadTest, SuccessSeverityIsInfo) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeSuccessPayload()));
    EXPECT_EQ(j.at("payload").value("severity", ""), "info");
}

TEST_F(PagerDutyPayloadTest, VersionInCustomDetails) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeSuccessPayload("4.0.0")));
    EXPECT_EQ(j.at("payload").at("custom_details").value("version", ""),
              "4.0.0");
}

TEST_F(PagerDutyPayloadTest, ErrorMessageInCustomDetails_OnFailure) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeFailPayload()));
    EXPECT_EQ(j.at("payload").at("custom_details")
                  .value("error_message", ""),
              "checksum mismatch");
}

TEST_F(PagerDutyPayloadTest, TimestampInPayload_IsISO8601) {
    const json j = json::parse(
        webhook_.buildPagerDutyPayload(makeSuccessPayload()));
    const std::string ts = j.at("payload").value("timestamp", "");
    // ISO-8601 format ends with Z
    EXPECT_FALSE(ts.empty());
    EXPECT_EQ(ts.back(), 'Z');
}

// ---------------------------------------------------------------------------
// notify() with both channels
// ---------------------------------------------------------------------------

class BothChannelsTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;

    std::vector<std::string> called_urls_;

    void SetUp() override {
        webhook_.setHttpSender(
            [this](const std::string& url, const std::string& /*body*/) {
                called_urls_.push_back(url);
                return true;
            });
        webhook_.setSlackConfig({"https://hooks.slack.com/services/TEST"});
        webhook_.setPagerDutyConfig({"routingkey123abc"});
    }
};

TEST_F(BothChannelsTest, NotifyCallsBothChannels) {
    webhook_.notify(makeSuccessPayload());
    EXPECT_EQ(called_urls_.size(), 2u);
}

TEST_F(BothChannelsTest, BothChannelsSucceed_NotifyReturnsTrue) {
    EXPECT_TRUE(webhook_.notify(makeSuccessPayload()));
}

// ---------------------------------------------------------------------------
// HTTP failure propagation
// ---------------------------------------------------------------------------

class HttpFailureTest : public ::testing::Test {};

TEST_F(HttpFailureTest, SlackFailure_NotifyReturnsFalse) {
    NotificationWebhook webhook;
    CapturedPost cap;
    webhook.setHttpSender(makeSender(cap, /*succeed=*/false));
    webhook.setSlackConfig({"https://hooks.slack.com/services/TEST"});
    EXPECT_FALSE(webhook.notify(makeSuccessPayload()));
}

TEST_F(HttpFailureTest, PagerDutyFailure_NotifyReturnsFalse) {
    NotificationWebhook webhook;
    CapturedPost cap;
    webhook.setHttpSender(makeSender(cap, /*succeed=*/false));
    webhook.setPagerDutyConfig({"routingkey123abc"});
    EXPECT_FALSE(webhook.notify(makeSuccessPayload()));
}

TEST_F(HttpFailureTest, SlackFailure_DoesNotPreventPagerDutyAttempt) {
    NotificationWebhook webhook;
    std::vector<std::string> urls;
    webhook.setHttpSender([&urls](const std::string& url,
                                   const std::string&) -> bool {
        urls.push_back(url);
        return false;  // always fail
    });
    webhook.setSlackConfig({"https://hooks.slack.com/services/TEST"});
    webhook.setPagerDutyConfig({"routingkey123abc"});
    webhook.notify(makeSuccessPayload());
    // Both channels should have been attempted
    EXPECT_EQ(urls.size(), 2u);
}

// ---------------------------------------------------------------------------
// Payload without optional fields
// ---------------------------------------------------------------------------

class MinimalPayloadTest : public ::testing::Test {
protected:
    NotificationWebhook webhook_;
    CapturedPost        cap_;
    void SetUp() override {
        webhook_.setHttpSender(makeSender(cap_));
        webhook_.setSlackConfig({"https://hooks.slack.com/services/TEST"});
        webhook_.setPagerDutyConfig({"routingkey123abc"});
    }
};

TEST_F(MinimalPayloadTest, MinimalPayload_SlackPayloadValid) {
    UpdateEventPayload p;
    p.event   = UpdateEvent::UPDATE_SUCCESS;
    p.version = "1.0.0";
    EXPECT_NO_THROW({
        auto parsed = json::parse(webhook_.buildSlackPayload(p));
        static_cast<void>(parsed);
    });
}

TEST_F(MinimalPayloadTest, MinimalPayload_PagerDutyPayloadValid) {
    UpdateEventPayload p;
    p.event   = UpdateEvent::UPDATE_FAILED;
    p.version = "1.0.0";
    EXPECT_NO_THROW({
        auto parsed = json::parse(webhook_.buildPagerDutyPayload(p));
        static_cast<void>(parsed);
    });
}
