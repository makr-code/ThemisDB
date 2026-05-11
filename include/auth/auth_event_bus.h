/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_event_bus.h                                   ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 INTERFACE-ONLY (Q3 2026)                     ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 📋 Interface Header — Implementation Target Q4 2026         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file auth_event_bus.h
 * @brief Auth event streaming interface for external SIEM integration.
 *
 * IAuthEventBus decouples event producers (authenticators, session manager,
 * policy engine) from consumers (Splunk HEC, Elastic ECS, Kafka, webhook).
 *
 * Compliance: SOC 2 CC7.2, ISO 27001 A.12.4, NIST SP 800-92.
 */

#pragma once

#include <chrono>
#include <cstddef>
#include &lt;map&gt;
#include <memory>
#include <string>

namespace themis {
namespace auth {

// ---------------------------------------------------------------------------
// AuthEventType — strongly-typed auth event taxonomy
// ---------------------------------------------------------------------------

/**
 * @brief Enumeration of security-relevant authentication and authorisation events.
 *
 * SIEM consumers use this to route events to the correct index/stream.
 */
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

/**
 * @brief Structured record representing a single auth security event.
 *
 * `correlation_id` links related events across subsystems (e.g., a login that
 * triggers MFA which produces a token — all share one correlation_id).
 * `metadata` carries type-specific fields (e.g., credential_id for passkey
 * events, policy_id for POLICY_UPDATED).
 */
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

/**
 * @brief Pure-virtual subscriber interface for auth event consumers.
 *
 * `onAuthEvent()` is called synchronously on the publishing thread; implementations
 * must not block.  Use an async queue internally if heavy processing is needed.
 */
class IAuthEventSubscriber {
public:
    virtual ~IAuthEventSubscriber() = default;

    /**
     * @brief Invoked for every published AuthEvent.
     *
     * Must complete quickly (< 1 ms); no I/O or heavy computation inline.
     */
    virtual void onAuthEvent(const AuthEvent& event) = 0;

    /// Unique subscriber identifier used for (de)registration.
    [[nodiscard]] virtual std::string subscriberId() const = 0;
};

// ---------------------------------------------------------------------------
// IAuthEventBus — publish/subscribe bus for auth events
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual auth event publish/subscribe bus.
 *
 * ### Thread safety
 * All methods must be safe to call concurrently from multiple threads.
 *
 * ### Delivery guarantee
 * Events are delivered synchronously to all subscribers registered at the
 * time of `publish()`.  No persistence or replay is provided at this layer;
 * SIEM forwarding durability is the subscriber's responsibility.
 */
class IAuthEventBus {
public:
    virtual ~IAuthEventBus() = default;

    /**
     * @brief Publish an auth event to all registered subscribers.
     *
     * Subscribers are notified in registration order.
     */
    virtual void publish(const AuthEvent& event) = 0;

    /**
     * @brief Register a subscriber.
     *
     * @return `false` if a subscriber with the same `subscriberId()` is already registered.
     */
    [[nodiscard]] virtual bool subscribe(std::shared_ptr<IAuthEventSubscriber> subscriber) = 0;

    /**
     * @brief Unregister a subscriber by ID.
     *
     * @return `false` if no subscriber with @p subscriber_id was found.
     */
    [[nodiscard]] virtual bool unsubscribe(const std::string& subscriber_id) = 0;

    /// Return the current number of registered subscribers.
    [[nodiscard]] virtual size_t subscriberCount() const = 0;
};

} // namespace auth
} // namespace themis
