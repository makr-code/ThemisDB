/**
 * @file notification_webhook.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/notification_webhook.h"
#include "updates/batch5_safety_helpers.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

#include <nlohmann/json.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace updates {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Default HTTP sender (libcurl)
// ---------------------------------------------------------------------------

namespace {

#ifdef THEMIS_ENABLE_CURL
/// libcurl write callback – discards the response body.
static std::size_t curlNullSink(char* /*buf*/, std::size_t /*size*/,
                                 std::size_t nmemb, void* /*userp*/) {
    return nmemb;
}
#endif // THEMIS_ENABLE_CURL

/**
 * @brief Post @p body as JSON to @p url.
 *
 * When THEMIS_ENABLE_CURL is defined this uses libcurl; otherwise it
 * logs a warning and returns false.
 */
bool defaultHttpPost(const std::string& url, const std::string& body) {
#ifdef THEMIS_ENABLE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("NotificationWebhook: curl_easy_init() failed");
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(body.size()));  // 7511 Fix: Explicit cast avoids implicit conversion warning

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlNullSink);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    CURLcode rc = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        LOG_ERROR("NotificationWebhook: curl error: {}",
                  curl_easy_strerror(rc));
        return false;
    }
    if (http_code < 200 || http_code >= 300) {
        LOG_ERROR("NotificationWebhook: HTTP {} from {}", http_code, url);
        return false;
    }
    return true;
#else
    static_cast<void>(body);
    LOG_WARN("NotificationWebhook: THEMIS_ENABLE_CURL is not defined; "
             "HTTP POST skipped (url={})", url);
    return false;
#endif // THEMIS_ENABLE_CURL
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// NotificationWebhook – construction
// ---------------------------------------------------------------------------

NotificationWebhook::NotificationWebhook()
    : http_sender_(defaultHttpPost) {}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void NotificationWebhook::setSlackConfig(const SlackConfig& cfg) {
    if (cfg.webhook_url.empty()) {
        LOG_WARN("NotificationWebhook::setSlackConfig: webhook_url is empty "
                 "– Slack channel will NOT be activated");
        slack_enabled_ = false;
        return;
    }
    slack_cfg_     = cfg;
    slack_enabled_ = true;
}

void NotificationWebhook::setPagerDutyConfig(const PagerDutyConfig& cfg) {
    if (cfg.routing_key.empty()) {
        LOG_WARN("NotificationWebhook::setPagerDutyConfig: routing_key is "
                 "empty – PagerDuty channel will NOT be activated");
        pagerduty_enabled_ = false;
        return;
    }
    pagerduty_cfg_     = cfg;
    pagerduty_enabled_ = true;
}

void NotificationWebhook::setHttpSender(HttpSendFunc fn) {
    if (!fn) {
        throw std::invalid_argument(
            "NotificationWebhook::setHttpSender: fn must not be null");
    }
    http_sender_ = std::move(fn);
}

// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

bool NotificationWebhook::notify([[maybe_unused]] const UpdateEventPayload& payload) {
    if (!slack_enabled_ && !pagerduty_enabled_) {
        LOG_WARN("NotificationWebhook::notify: no channels configured");
        return false;
    }

    bool all_ok = true;
    if (slack_enabled_) {
        if (!sendSlack(payload)) {
            all_ok = false;
        }
    }
    if (pagerduty_enabled_) {
        if (!sendPagerDuty(payload)) {
            all_ok = false;
        }
    }
    return all_ok;
}

// ---------------------------------------------------------------------------
// Slack
// ---------------------------------------------------------------------------

std::string NotificationWebhook::buildSlackPayload(
    const UpdateEventPayload& payload) const
{
    const std::string color  = slackColor([[maybe_unused]] payload.event);
    const std::string label  = eventLabel([[maybe_unused]] payload.event);

    json fields = json::array();
    fields.push_back({{"title", "Version"}, {"value", payload.version},
                      {"short", true}});
    if (!payload.from_version.empty()) {
        fields.push_back({{"title", "From"}, {"value", payload.from_version},
                          {"short", true}});
    }
    if (!payload.error_message.empty()) {
        fields.push_back({{"title", "Error"}, {"value", payload.error_message},
                          {"short", false}});
    }
    if (!payload.rollback_id.empty()) {
        fields.push_back({{"title", "Rollback ID"},
                          {"value", payload.rollback_id},
                          {"short", true}});
    }
    if (!payload.files_updated.empty()) {
        // Use ostringstream for efficient string concatenation (Error Code: 7471)
        std::ostringstream files_stream = {};
        bool first = true;
        for (const auto& f : payload.files_updated) {
            if (!first) {
              files_stream << "\n";
            }
            files_stream << f;
            first = false;
        }
        std::string files_str = files_stream.str();
        fields.push_back({{"title", "Files Updated"},
                          {"value", files_str},
                          {"short", false}});
    }

    json attachment = {
        {"color",  color},
        {"title",  label},
        {"ts",     std::chrono::duration_cast<std::chrono::seconds>(
                       payload.timestamp.time_since_epoch()).count()},
        {"fields", fields},
        {"footer", "ThemisDB Update System"},
    };

    json body;
    body["username"] = slack_cfg_.username;
    body["icon_emoji"] = slack_cfg_.icon_emoji;
    if (!slack_cfg_.channel.empty()) {
        body["channel"] = slack_cfg_.channel;
    }
    body["text"] = label + " – version " + payload.version;
    body["attachments"] = json::array({attachment});

    return body.dump();
}

bool NotificationWebhook::sendSlack([[maybe_unused]] const UpdateEventPayload& payload) {
    const std::string body = buildSlackPayload(payload);
    LOG_DEBUG("NotificationWebhook: sending Slack notification (event={}, "
              "version={})", eventLabel([[maybe_unused]] payload.event), payload.version);
    if (!http_sender_(slack_cfg_.webhook_url, body)) {
        LOG_ERROR("NotificationWebhook: Slack POST failed");
        return false;
    }
    LOG_INFO("NotificationWebhook: Slack notification sent (version={})",
             payload.version);
    return true;
}

// ---------------------------------------------------------------------------
// PagerDuty
// ---------------------------------------------------------------------------

std::string NotificationWebhook::buildPagerDutyPayload(
    const UpdateEventPayload& payload) const
{
    const std::string action   = pagerDutyAction([[maybe_unused]] payload.event);
    const std::string severity = pagerDutySeverity([[maybe_unused]] payload.event);
    const std::string label    = eventLabel([[maybe_unused]] payload.event);
    const std::string ts_str   = toISO8601(payload.timestamp);
    // Stable dedup key so that a "resolve" event closes the matching alert.
    const std::string dedup_key =
        "themisdb-update-" + payload.version;

    json custom_details;
    custom_details["version"] = payload.version;
    if (!payload.from_version.empty()) {
        custom_details["from_version"] = payload.from_version;
    }
    if (!payload.error_message.empty()) {
        custom_details["error_message"] = payload.error_message;
    }
    if (!payload.rollback_id.empty()) {
        custom_details["rollback_id"] = payload.rollback_id;
    }
    if (!payload.files_updated.empty()) {
        custom_details["files_updated"] = payload.files_updated;
    }

    json pd_payload = {
        {"summary",        label + ": " + pagerduty_cfg_.service_name +
                           " version " + payload.version},
        {"severity",       severity},
        {"source",         pagerduty_cfg_.service_name},
        {"timestamp",      ts_str},
        {"custom_details", custom_details},
    };

    json body = {
        {"routing_key",   pagerduty_cfg_.routing_key},
        {"event_action",  action},
        {"dedup_key",     dedup_key},
        {"payload",       pd_payload},
    };

    return body.dump();
}

bool NotificationWebhook::sendPagerDuty([[maybe_unused]] const UpdateEventPayload& payload) {
    const std::string body = buildPagerDutyPayload(payload);
    LOG_DEBUG("NotificationWebhook: sending PagerDuty notification (event={}, "
              "version={})", eventLabel([[maybe_unused]] payload.event), payload.version);
    if (!http_sender_(pagerduty_cfg_.events_api_url, body)) {
        LOG_ERROR("NotificationWebhook: PagerDuty POST failed");
        return false;
    }
    LOG_INFO("NotificationWebhook: PagerDuty notification sent (version={})",
             payload.version);
    return true;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

std::string NotificationWebhook::eventLabel([[maybe_unused]] UpdateEvent event) const {
    switch ([[maybe_unused]] event) {
        case UpdateEvent::UPDATE_SUCCESS:   return "Update Successful";
        case UpdateEvent::UPDATE_FAILED:    return "Update Failed";
        case UpdateEvent::ROLLBACK_SUCCESS: return "Rollback Successful";
        case UpdateEvent::ROLLBACK_FAILED:  return "Rollback Failed";
        default: break;
    }
    return "Unknown Event";
}

std::string NotificationWebhook::slackColor([[maybe_unused]] UpdateEvent event) const {
    switch ([[maybe_unused]] event) {
        case UpdateEvent::UPDATE_SUCCESS:   return "good";
        case UpdateEvent::UPDATE_FAILED:    return "danger";
        case UpdateEvent::ROLLBACK_SUCCESS: return "warning";
        case UpdateEvent::ROLLBACK_FAILED:  return "danger";
        default: break;
    }
    return "warning";
}

std::string NotificationWebhook::pagerDutyAction([[maybe_unused]] UpdateEvent event) const {
    switch ([[maybe_unused]] event) {
        case UpdateEvent::UPDATE_SUCCESS:   return "resolve";
        case UpdateEvent::ROLLBACK_SUCCESS: return "resolve";
        case UpdateEvent::UPDATE_FAILED:    return "trigger";
        case UpdateEvent::ROLLBACK_FAILED:  return "trigger";
        default: break;
    }
    return "trigger";
}

std::string NotificationWebhook::pagerDutySeverity([[maybe_unused]] UpdateEvent event) const {
    switch ([[maybe_unused]] event) {
        case UpdateEvent::UPDATE_SUCCESS:   return "info";
        case UpdateEvent::UPDATE_FAILED:    return "critical";
        case UpdateEvent::ROLLBACK_SUCCESS: return "warning";
        case UpdateEvent::ROLLBACK_FAILED:  return "critical";
        default: break;
    }
    return "error";
}

/*static*/
std::string NotificationWebhook::toISO8601(
    std::chrono::system_clock::time_point tp)
{
    const std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::ostringstream oss = {};
    oss << std::put_time(std::gmtime(&tt), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace updates
} // namespace themis
