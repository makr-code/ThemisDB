/**
 * @file metrics_stream_server.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/metrics_stream_server.h"

#include <chrono>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace themis {
namespace observability {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

MetricsStreamServer::MetricsStreamServer() = default;
MetricsStreamServer::~MetricsStreamServer() = default;

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void MetricsStreamServer::setDeliveryCallback([[maybe_unused]] SendFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    send_fn_ = std::move(fn);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MetricsStreamServer::start(const std::string& bind_address, uint16_t port) {
    if (bind_address.empty()) {
        throw std::runtime_error("MetricsStreamServer::start: bind_address must not be empty");
    }
    if (port == 0) {
        throw std::runtime_error("MetricsStreamServer::start: port must not be 0");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    bind_address_ = bind_address;
    port_ = port;
    running_.store(true, std::memory_order_release);
}

void MetricsStreamServer::stop() {
    running_.store(false, std::memory_order_release);
}

bool MetricsStreamServer::isRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

std::string MetricsStreamServer::bindAddress() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bind_address_;
}

uint16_t MetricsStreamServer::port() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return port_;
}

// ---------------------------------------------------------------------------
// Subscription management
// ---------------------------------------------------------------------------

void MetricsStreamServer::subscribe(const StreamSubscription& subscription) {
    if (subscription.client_id.empty()) {
        throw std::invalid_argument(
            "MetricsStreamServer::subscribe: client_id must not be empty");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    SubscriptionState state;
    state.subscription = subscription;
    // last_delivery defaults to the epoch; first update will always pass the
    // interval check regardless of the configured update_interval.
    subscriptions_[subscription.client_id] = std::move(state);
}

void MetricsStreamServer::unsubscribe(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_.erase(client_id);
}

size_t MetricsStreamServer::subscriptionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.size();
}

bool MetricsStreamServer::hasSubscription(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.count(client_id) > 0;
}

// ---------------------------------------------------------------------------
// Metric dispatch
// ---------------------------------------------------------------------------

void MetricsStreamServer::pushMetrics(const MetricUpdate& update) {
    total_updates_pushed_.fetch_add(1, std::memory_order_relaxed);

    if (!running_.load(std::memory_order_acquire)) {
        return;
    }

    const std::string payload = formatWebSocketMessage(update);
    const auto now = std::chrono::steady_clock::now();

    // Collect eligible client IDs while holding the lock, then release the
    // lock before invoking callbacks to avoid both iterator invalidation and
    // potential deadlocks if a callback calls back into the server.
    // Also copy send_fn_ under the mutex to avoid a data race with
    // setDeliveryCallback() running concurrently.
    std::vector<std::string> to_deliver;
    SendFn send_fn_copy;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        send_fn_copy = send_fn_;

        for (auto& [client_id, state] : subscriptions_) {
            const StreamSubscription& sub = state.subscription;

            // --- Name filter ---
            if (!matchesMetricNames(sub, update)) {
                filtered_deliveries_.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            // --- Label filters ---
            if (!matchesFilters(sub, update)) {
                filtered_deliveries_.fetch_add(1, std::memory_order_relaxed);
                continue;
            }

            // --- Rate limit ---
            if (sub.update_interval.count() > 0) {
                const auto elapsed =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - state.last_delivery);
                if (elapsed < sub.update_interval) {
                    throttled_deliveries_.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }
            }

            state.last_delivery = now;
            total_deliveries_.fetch_add(1, std::memory_order_relaxed);
            to_deliver.push_back(client_id);
        }
    } // lock released here

    // Invoke callbacks without holding the mutex.
    if (send_fn_copy) {
        for (const auto& cid : to_deliver) {
            send_fn_copy(cid, payload);
        }
    }
}

// ---------------------------------------------------------------------------
// Serialisation helpers
// ---------------------------------------------------------------------------

// Escape a string for use as a JSON string value (handles \, ", and control
// characters).  This avoids introducing a heavy JSON library dependency for
// the small set of string fields we serialise here.
static std::string jsonEscapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 2 + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    // Encode other control characters as \uXXXX.
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out;
}

// static
std::string MetricsStreamServer::labelsToJson(
    const std::map<std::string, std::string>& labels) {
    if (labels.empty()) return "{}";
    std::ostringstream oss;
    oss << '{';
    bool first = true;
    for (const auto& [k, v] : labels) {
        if (!first) {
          oss << ',';
        }
        oss << '"' << jsonEscapeString(k) << "\":\"" << jsonEscapeString(v) << '"';
        first = false;
    }
    oss << '}';
    return oss.str();
}

// static
std::string MetricsStreamServer::formatWebSocketMessage(const MetricUpdate& update) {
    // Emit timestamp as milliseconds since Unix epoch.
    const int64_t ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                              update.timestamp.time_since_epoch())
                              .count();

    std::ostringstream oss;
    oss << R"({"type":"metric_update","metric_name":")"
        << jsonEscapeString(update.metric_name)
        << R"(","value":)" << update.value
        << R"(,"labels":)" << labelsToJson(update.labels)
        << R"(,"timestamp_ms":)" << ts_ms
        << '}';
    return oss.str();
}

// static
std::string MetricsStreamServer::formatSseMessage(const MetricUpdate& update) {
    return "data: " + formatWebSocketMessage(update) + "\n\n";
}

// ---------------------------------------------------------------------------
// Observability
// ---------------------------------------------------------------------------

MetricsStreamServer::Stats MetricsStreamServer::getStats() const {
    Stats s;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        s.active_subscriptions = subscriptions_.size();
    }
    s.total_updates_pushed  = total_updates_pushed_.load(std::memory_order_relaxed);
    s.total_deliveries      = total_deliveries_.load(std::memory_order_relaxed);
    s.throttled_deliveries  = throttled_deliveries_.load(std::memory_order_relaxed);
    s.filtered_deliveries   = filtered_deliveries_.load(std::memory_order_relaxed);
    return s;
}

void MetricsStreamServer::resetStats() {
    total_updates_pushed_.store(0, std::memory_order_relaxed);
    total_deliveries_.store(0, std::memory_order_relaxed);
    throttled_deliveries_.store(0, std::memory_order_relaxed);
    filtered_deliveries_.store(0, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// static
bool MetricsStreamServer::matchesMetricNames(
    const StreamSubscription& sub,
    const MetricUpdate& update) noexcept {
    if (sub.metric_names.empty()) {
        return true; // empty list → subscribe to all
    }
    for (const auto& name : sub.metric_names) {
        if (name == update.metric_name) {
          return true;
        }
    }
    return false;
}

// static
bool MetricsStreamServer::matchesFilters(
    const StreamSubscription& sub,
    const MetricUpdate& update) noexcept {
    for (const auto& f : sub.filters) {
        auto it = update.labels.find(f.label);
        if (it == update.labels.end()) {
            // The label is not present in the update — filter fails.
            return false;
        }
        // Empty value means wildcard; any value is accepted.
        if (!f.value.empty() && it->second != f.value) {
            return false;
        }
    }
    return true;
}

} // namespace observability
} // namespace themis
