/**
 * @file notification_webhook.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace updates {

/**
 * @brief Type of update lifecycle event triggering a notification.
 */
enum class UpdateEvent {
    UPDATE_SUCCESS,    ///< A new version was applied successfully.
    UPDATE_FAILED,     ///< Applying a new version failed.
    ROLLBACK_SUCCESS,  ///< A rollback completed successfully.
    ROLLBACK_FAILED,   ///< A rollback attempt failed.
};

/**
 * @brief Data describing an update event sent to notification endpoints.
 */
struct UpdateEventPayload {
    /// Event type.
    UpdateEvent event{UpdateEvent::UPDATE_SUCCESS};

    /// Version being applied / rolled back to.
    std::string version;

    /// Previous version (may be empty when not known).
    std::string from_version;

    /// Non-empty only on failure events.
    std::string error_message;

    /// Rollback ID – non-empty when a rollback is involved.
    std::string rollback_id;

    /// Files touched by the update (may be empty on failure).
    std::vector<std::string> files_updated;

    /// When the event occurred; defaults to the time of construction.
    std::chrono::system_clock::time_point timestamp{
        std::chrono::system_clock::now()};
};

// ---------------------------------------------------------------------------
// Slack
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for a Slack Incoming Webhook endpoint.
 *
 * Obtain the webhook URL from your Slack workspace's "Incoming Webhooks" app.
 */
struct SlackConfig {
    /// Incoming Webhook URL, e.g. "https://hooks.slack.com/services/T.../B.../…"
    std::string webhook_url;

    /// Override the default channel set for the webhook (optional).
    std::string channel;

    /// Display name for the bot message.
    std::string username{"ThemisDB"};

    /// Emoji icon for the bot message.
    std::string icon_emoji{":database:"};
};

// ---------------------------------------------------------------------------
// PagerDuty
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the PagerDuty Events API v2.
 *
 * Obtain the routing_key (integration key) from your PagerDuty service's
 * "Integrations" tab when using the "Events API v2" integration type.
 */
struct PagerDutyConfig {
    /// Integration / routing key (32-character hex string).
    std::string routing_key;

    /// Human-readable service name used in alert summaries.
    std::string service_name{"ThemisDB"};

    /// PagerDuty Events API v2 endpoint (override for testing / proxies).
    std::string events_api_url{
        "https://events.pagerduty.com/v2/enqueue"};
};

// ---------------------------------------------------------------------------
// NotificationWebhook
// ---------------------------------------------------------------------------

/**
 * @brief Dispatches structured update-lifecycle notifications to Slack
 *        and/or PagerDuty.
 *
 * HTTP transport is injectable via setHttpSender() so unit tests can
 * run without network access or mocking libcurl.  When no sender is
 * registered the class falls back to libcurl when THEMIS_ENABLE_CURL
 * is defined, and returns @c false otherwise.
 *
 * Usage example:
 * @code
 *   NotificationWebhook webhook;
 *   webhook.setSlackConfig({"https://hooks.slack.com/services/…"});
 *   webhook.setPagerDutyConfig({"abc123routingkey"});
 *
 *   UpdateEventPayload payload;
 *   payload.event   = UpdateEvent::UPDATE_SUCCESS;
 *   payload.version = "1.6.0";
 *   webhook.notify(payload);
 * @endcode
 */
class NotificationWebhook {
public:
    /**
     * @brief Signature for the injected HTTP POST sender.
     *
     * @param url   Destination URL.
     * @param body  JSON body (UTF-8).
     * @return @c true on HTTP 2xx; @c false on any error.
     */
    using HttpSendFunc =
        std::function<bool(const std::string& url,
                           const std::string& body)>;

    NotificationWebhook();
    ~NotificationWebhook() = default;

    // Non-copyable (contains a std::function member and stateful configs).
    NotificationWebhook(const NotificationWebhook&) = delete;
    NotificationWebhook& operator=(const NotificationWebhook&) = delete;

    // ---- Configuration ----

    /**
     * @brief Enable and configure the Slack notification channel.
     *
     * @param cfg  Slack webhook configuration.  The webhook_url must be
     *             non-empty for the channel to be activated.
     */
    void setSlackConfig(const SlackConfig& cfg);

    /**
     * @brief Enable and configure the PagerDuty notification channel.
     *
     * @param cfg  PagerDuty configuration.  The routing_key must be
     *             non-empty for the channel to be activated.
     */
    void setPagerDutyConfig(const PagerDutyConfig& cfg);

    /**
     * @brief Override the HTTP POST sender (primarily for unit tests).
     *
     * When not set the default implementation uses libcurl (when
     * THEMIS_ENABLE_CURL is defined) or logs a warning and returns
     * @c false.
     */
    void setHttpSender(HttpSendFunc fn);

    // ---- Dispatch ----

    /**
     * @brief Send notifications for @p payload to all configured channels.
     *
     * Individual channel failures are logged but do not throw.
     *
     * @param payload  Event payload.
     * @return @c true if every configured channel accepted the notification;
     *         @c false if any channel failed or no channel is configured.
     */
    bool notify(const UpdateEventPayload& payload);

    // ---- Payload builders (exposed for testing / inspection) ----

    /**
     * @brief Build the JSON body for a Slack Incoming Webhook message.
     */
    std::string buildSlackPayload(const UpdateEventPayload& payload) const;

    /**
     * @brief Build the JSON body for a PagerDuty Events API v2 request.
     */
    std::string buildPagerDutyPayload(
        const UpdateEventPayload& payload) const;

private:
    bool sendSlack(const UpdateEventPayload& payload);
    bool sendPagerDuty(const UpdateEventPayload& payload);

    /// Human-readable label for an event (e.g. "Update Successful").
    std::string eventLabel(UpdateEvent event) const;

    /// Slack attachment colour string: "good", "warning", or "danger".
    std::string slackColor(UpdateEvent event) const;

    /// PagerDuty event_action: "trigger" or "resolve".
    std::string pagerDutyAction(UpdateEvent event) const;

    /// PagerDuty severity: "critical", "error", "warning", or "info".
    std::string pagerDutySeverity(UpdateEvent event) const;

    /// ISO-8601 timestamp string from a time_point.
    static std::string toISO8601(
        std::chrono::system_clock::time_point tp);

    SlackConfig      slack_cfg_;
    PagerDutyConfig  pagerduty_cfg_;
    bool             slack_enabled_{false};
    bool             pagerduty_enabled_{false};
    HttpSendFunc     http_sender_;
};

} // namespace updates
} // namespace themis
