/**
 * @file iconnection_policy.h
 * @brief Abstract connection-limit policy for HTTP/2 streams, SSE, and total
 *        server connections in ThemisDB.
 *
 * Governs four connection-admission axes: concurrent HTTP/2 streams per
 * physical connection, total SSE long-poll connections, total simultaneous
 * server connections, and SSE event emission rate.  Sits in Tier 2 of the
 * four-tier resource-governance chain:
 *
 * @code
 *   compile-time constexpr (edition.h)   ← absolute ceiling, never overridable
 *   RuntimeLicenseGate                   ← edition-tier ceiling
 *   IConnectionPolicy   (this file)      ← signed-plugin fine-tuning
 *   ConnectionConfig                     ← per-deployment operational tuning
 * @endcode
 *
 * All implementations must be individually thread-safe.
 *
 * @note This interface is part of the edition-policy plugin contract
 *       (IEditionPolicyPlugin::createConnectionPolicy).  Claimed limits are
 *       validated against the compile-time ceilings in edition.h
 *       (CONNECTION_MAX_HTTP2_STREAMS, CONNECTION_MAX_SSE_CONNECTIONS,
 *        CONNECTION_MAX_TOTAL, CONNECTION_MAX_SSE_EVENTS_PER_SEC) before
 *       EditionManager accepts the policy.
 */

#pragma once

#include <cstdint>

namespace themis {
namespace network {

/**
 * @brief Abstract server connection-limit policy.
 *
 * Controls four connection-admission axes:
 *  - **HTTP/2 streams** — maximum concurrent streams per connection.
 *  - **SSE connections** — maximum total server-sent-event subscriptions.
 *  - **Total connections** — maximum simultaneous server connections.
 *  - **SSE event rate** — maximum events emitted per second (throttle).
 *
 * Implementations are installed into EditionManager via
 * `EditionManager::installConnectionPolicy()` and consulted by the protocol
 * session layer when admitting new connections or streams.
 */
class IConnectionPolicy {
public:
    virtual ~IConnectionPolicy() = default;

    // Non-copyable, non-movable by default.
    IConnectionPolicy(const IConnectionPolicy&)            = delete;
    IConnectionPolicy& operator=(const IConnectionPolicy&) = delete;

    // -------------------------------------------------------------------------
    // HTTP/2 stream limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff opening one more HTTP/2 stream on the current
     *        connection is permitted.
     *
     * Does not modify accounting state — call onStreamOpened() only after the
     * stream has been accepted.
     *
     * @param current_streams  Number of streams currently open on this connection.
     * @return true when another stream may be opened.
     */
    [[nodiscard]] virtual bool canOpenStream(uint32_t current_streams) const = 0;

    /**
     * @brief Maximum concurrent HTTP/2 streams per connection; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxHttp2StreamsPerConnection() const noexcept = 0;

    // -------------------------------------------------------------------------
    // SSE connection limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff accepting one more SSE subscription is permitted.
     *
     * Does not modify accounting state — call onSSEConnectionOpened() only
     * after the subscription has been accepted.
     *
     * @return true when another SSE connection may be opened.
     */
    [[nodiscard]] virtual bool canOpenSSEConnection() const = 0;

    /**
     * @brief Notify the policy that an SSE connection has been opened.
     *
     * Updates accounting state.  Thread-safe.
     */
    virtual void onSSEConnectionOpened() = 0;

    /**
     * @brief Notify the policy that an SSE connection has been closed.
     *
     * Releases the SSE slot.  Implementations must clamp to zero on
     * mismatched calls.  Thread-safe.
     */
    virtual void onSSEConnectionClosed() = 0;

    /**
     * @brief Maximum total SSE connections; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxSSEConnections() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Total connection limit
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff accepting one more server connection is permitted.
     *
     * Does not modify accounting state — call onConnectionAccepted() only
     * after the connection has been admitted.
     *
     * @return true when another connection may be accepted.
     */
    [[nodiscard]] virtual bool canAcceptConnection() const = 0;

    /**
     * @brief Notify the policy that a new connection has been accepted.
     *
     * Updates total-connection accounting.  Thread-safe.
     */
    virtual void onConnectionAccepted() = 0;

    /**
     * @brief Notify the policy that an existing connection has been closed.
     *
     * Releases the connection slot.  Implementations must clamp to zero on
     * mismatched calls.  Thread-safe.
     */
    virtual void onConnectionClosed() = 0;

    /**
     * @brief Return the number of connections currently tracked as open.
     */
    [[nodiscard]] virtual uint32_t activeConnections() const = 0;

    /**
     * @brief Maximum total simultaneous server connections; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxTotalConnections() const noexcept = 0;

    // -------------------------------------------------------------------------
    // SSE event emission rate
    // -------------------------------------------------------------------------

    /**
     * @brief Return true iff emitting one more SSE event right now is permitted.
     *
     * Uses token-bucket semantics.  May consume a token — callers must not
     * emit the event if this method returns false.
     *
     * @return true when the event emission is within the allowed rate.
     */
    [[nodiscard]] virtual bool allowSSEEvent() = 0;

    /**
     * @brief Maximum SSE events emitted per second; 0 = unlimited.
     */
    [[nodiscard]] virtual uint32_t maxSSEEventsPerSecond() const noexcept = 0;

    // -------------------------------------------------------------------------
    // Status
    // -------------------------------------------------------------------------

    /**
     * @brief Return true when any connection-limit enforcement is active.
     *
     * Implementations should return false when all limits are 0 (unlimited)
     * so that callers may bypass the admission check on hot paths.
     */
    [[nodiscard]] virtual bool isEnforced() const noexcept = 0;

protected:
    IConnectionPolicy() = default;
    IConnectionPolicy(IConnectionPolicy&&) = default;
    IConnectionPolicy& operator=(IConnectionPolicy&&) = default;
};

} // namespace network
} // namespace themis
