/**
 * @file auth_event_bus.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <string>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// AuthEventType — strongly-typed auth event taxonomy
// ---------------------------------------------------------------------------

enum class AuthEventType {
    LOGIN_SUCCESS,
    LOGIN_FAILED,
    LOGOUT,
    MFA_REQUIRED,
    MFA_SUCCESS,
    MFA_FAILED,
    TOKEN_ISSUED,
    TOKEN_REVOKED,
    TOKEN_EXPIRED,
    PERMISSION_DENIED,
    PRIVILEGE_ESCALATION,
    PASSKEY_REGISTERED,
    PASSKEY_REMOVED,
    PASSKEY_AUTH_SUCCESS,
    PASSKEY_AUTH_FAILED,
    POLICY_UPDATED,
    ANOMALY_DETECTED,
};

// ---------------------------------------------------------------------------
// AuthEvent — structured auth event record
// ---------------------------------------------------------------------------

struct AuthEvent {
    std::string   event_id;
    AuthEventType type;
    std::string   user_id;
    std::string   session_id;
    std::string   client_ip;
    std::string   user_agent;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
    std::string   correlation_id;
};

// ---------------------------------------------------------------------------
// IAuthEventSubscriber — consumer interface for auth events
// ---------------------------------------------------------------------------

class IAuthEventSubscriber {
public:
    /**
     * @brief IAuth Event Subscriber.
     * @return Return value.
     */
    virtual ~IAuthEventSubscriber() = default;

    /**
     * @brief On Auth Event.
     * @param[in] event Input parameter.
     */
    virtual void onAuthEvent(const AuthEvent& event) = 0;

    [[nodiscard]] virtual std::string subscriberId() const = 0;
};

// ---------------------------------------------------------------------------
// IAuthEventBus — publish/subscribe bus for auth events
// ---------------------------------------------------------------------------

class IAuthEventBus {
public:
    /**
     * @brief IAuth Event Bus.
     * @return Return value.
     */
    virtual ~IAuthEventBus() = default;

    /**
     * @brief Publish.
     * @param[in] event Input parameter.
     */
    virtual void publish(const AuthEvent& event) = 0;

    [[nodiscard]] virtual bool subscribe(std::shared_ptr<IAuthEventSubscriber> subscriber) = 0;

    [[nodiscard]] virtual bool unsubscribe(const std::string& subscriber_id) = 0;

    [[nodiscard]] virtual size_t subscriberCount() const = 0;
};

} // namespace auth
} // namespace themis
