/*
 * ThemisDB | File: network_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen network module API contracts for the active v1.x major line.
 */

/**
 * @file network_api_contract.h
 * @brief Frozen network module API contracts for the active v1.x major line.
 *
 * This header defines the normative, binding contract for the ThemisDB network
 * module covering:
 *   - Connection lifecycle (accept → auth → serve → drain → close ordering)
 *   - Frame validation (malformed frame handling — fail-closed)
 *   - Auth/session guards (every privileged opcode requires a valid session)
 *   - Transport fallback (TCP → QUIC semantics, no silent data loss)
 *   - Rate limiting (per-connection and per-client with explicit 429)
 *   - Canonical error taxonomy
 *
 * ## Contract Scope
 *
 * These contracts are binding for all v1.x implementations:
 *   - Wire protocol servers (WireProtocolServer, WireProtocolWebSocket)
 *   - UDP and QUIC transports (UdpServer, QuicServer, QuicTransport)
 *   - gRPC transport adapters (GrpcTransport)
 *   - Connection pool / session managers
 *   - Rate limiters and backpressure controllers (QosManager)
 *   - Retry policy implementations (WireRetryPolicy)
 *
 * ## Versioning
 *
 * Stable within v1.x.  Breaking changes require v2.0 with migration notes.
 *
 * @see src/network/ROADMAP.md — Phase 1 frozen contract items
 * @see include/network/wire_protocol.h        — Frame framing constants
 * @see include/network/wire_protocol_server.h — Server lifecycle types
 * @see include/network/qos_manager.h          — Rate-limiting interface
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace themis {
namespace network {

// ============================================================================
// § 1  Connection Lifecycle Contract
//
// Every connection MUST traverse the following states in order:
//
//   ACCEPTING → AUTHENTICATING → SERVING → DRAINING → CLOSED
//
// Invariants:
//   a) No application messages may be dispatched before AUTHENTICATING
//      completes successfully.
//   b) No new messages are accepted once DRAINING begins.
//   c) CLOSED is terminal: no data may be written after entering CLOSED.
//   d) Connections that do not complete AUTHENTICATING within
//      kAuthHandshakeTimeout are forcibly CLOSED.
// ============================================================================

/// Maximum time allowed for the authentication handshake after TCP accept.
/// Connections not authenticated within this window are aborted.
inline constexpr std::chrono::seconds kAuthHandshakeTimeout{15};

/// Maximum idle time in SERVING state before the server initiates DRAINING.
inline constexpr std::chrono::minutes kConnectionIdleTimeout{30};

/// Maximum time the server waits for in-flight requests to complete during
/// DRAINING before forcibly closing the connection.
inline constexpr std::chrono::seconds kDrainGracePeriod{30};

/// Maximum number of concurrent connections per server instance.
/// Connections exceeding this limit receive BACKPRESSURE_EXCEEDED immediately.
inline constexpr std::size_t kMaxConcurrentConnections = 65536;

// ============================================================================
// § 2  Frame Validation Contract
//
// All inbound frames MUST be validated before dispatch:
//   a) A frame with an invalid magic number → connection is CLOSED immediately
//      with FRAME_INVALID (not silently dropped).
//   b) A frame whose declared payload length exceeds kMaxFramePayloadBytes →
//      FRAME_OVERSIZED and connection is CLOSED.
//   c) A frame with an unrecognised opcode in SERVING state →
//      OPCODE_UNKNOWN response; connection remains open.
//   d) Validation failures are always logged to the network audit channel
//      before the connection is closed.
//
// Silent drop of malformed frames is NOT permitted.
// ============================================================================

/// Magic bytes that open every valid ThemisDB wire frame.
inline constexpr std::uint8_t kFrameMagic0 = 0xDB;
inline constexpr std::uint8_t kFrameMagic1 = 0x01;

/// Maximum payload size per frame.  Frames exceeding this are FRAME_OVERSIZED.
inline constexpr std::size_t kMaxFramePayloadBytes = 64 * 1024 * 1024;  ///< 64 MiB.

/// Minimum valid frame size (magic + type + length field).
inline constexpr std::size_t kMinFrameBytes = 7;

// ============================================================================
// § 3  Auth/Session Guard Contract
//
// Every privileged opcode MUST:
//   a) Carry a valid, non-expired session token in the frame header.
//   b) Be rejected with AUTH_REQUIRED if no token is present.
//   c) Be rejected with SESSION_EXPIRED if the token's expiry is in the past.
//   d) Be rejected with SESSION_REVOKED if the session appears in the
//      revocation index at evaluation time.
//
// The session guard is evaluated BEFORE any handler dispatch.  Handlers must
// not be reachable without a passing guard check.
// ============================================================================

/// Maximum session token size accepted in frame metadata.
inline constexpr std::size_t kMaxSessionTokenBytes = 4096;

/// Maximum session lifetime issued by the network layer.
inline constexpr std::chrono::hours kMaxNetworkSessionLifetime{24};

/// Clock-skew tolerance applied to session token expiry checks.
inline constexpr std::chrono::seconds kSessionClockSkew{30};

// ============================================================================
// § 4  Transport Fallback Contract (TCP → QUIC)
//
// When transport fallback is enabled:
//   a) The fallback triggers only when the primary transport fails to complete
//      the handshake within kTransportFallbackDelay.
//   b) No data written to the primary transport may be silently dropped;
//      pending writes are either replayed on the fallback transport or the
//      caller receives an explicit TRANSPORT_CLOSED error.
//   c) The fallback sequence is: TCP → QUIC; no further automatic fallback.
//   d) Both transports use the same session and authentication context.
// ============================================================================

/// Delay after which the network layer initiates transport fallback.
inline constexpr std::chrono::milliseconds kTransportFallbackDelay{500};

// ============================================================================
// § 5  Rate-Limiting Contract
//
// Two independent rate-limit dimensions are enforced:
//   - Per-connection: kDefaultPerConnRequestsPerSec requests per second.
//   - Per-client (by remote IP): kDefaultPerClientRequestsPerSec.
//
// When a limit is exceeded:
//   a) The server returns RATE_LIMITED with a Retry-After indication.
//   b) The current request is NOT processed.
//   c) The connection is NOT closed on the first violation; it is closed after
//      kRateLimitBanThreshold consecutive violations within kRateLimitBanWindow.
// ============================================================================

/// Default per-connection request rate limit (requests/second).
inline constexpr int kDefaultPerConnRequestsPerSec = 1000;

/// Default per-client (IP) request rate limit (requests/second).
inline constexpr int kDefaultPerClientRequestsPerSec = 5000;

/// Number of consecutive RATE_LIMITED events within kRateLimitBanWindow that
/// triggers a temporary connection ban.
inline constexpr int kRateLimitBanThreshold = 100;

/// Sliding window used for the ban-threshold accumulator.
inline constexpr std::chrono::seconds kRateLimitBanWindow{60};

/// Duration of a temporary ban imposed after kRateLimitBanThreshold violations.
inline constexpr std::chrono::minutes kRateLimitBanDuration{5};

// ============================================================================
// § 6  Error Taxonomy
//
// Codes < 100: frame/protocol; 100–199: auth/session; 200–299: transport;
// 300–399: rate-limit/backpressure; 400–499: quorum/cluster; 9xxx: internal.
// ============================================================================

/**
 * @brief Canonical error codes for the ThemisDB network module.
 *
 * All network operation failures MUST map to one of these codes before being
 * returned to callers or emitted in metrics/audit events.
 */
enum class NetworkErrorCode : int {
    // ── Frame / Protocol ─────────────────────────────────────────────────────
    /// Frame is structurally invalid (bad magic, corrupt header).
    FRAME_INVALID               = 10,
    /// Frame payload exceeds kMaxFramePayloadBytes.
    FRAME_OVERSIZED             = 11,
    /// Frame opcode is not recognised in the current protocol version.
    OPCODE_UNKNOWN              = 12,
    /// Frame checksum or integrity tag does not match.
    FRAME_CHECKSUM_FAILED       = 13,

    // ── Auth / Session ────────────────────────────────────────────────────────
    /// Privileged opcode received without a session token.
    AUTH_REQUIRED               = 100,
    /// Session token has expired (exp check failed).
    SESSION_EXPIRED             = 101,
    /// Session has been explicitly revoked.
    SESSION_REVOKED             = 102,
    /// Session token is malformed or unparseable.
    SESSION_MALFORMED           = 103,
    /// Authentication handshake did not complete within kAuthHandshakeTimeout.
    AUTH_TIMEOUT                = 104,

    // ── Transport ─────────────────────────────────────────────────────────────
    /// Connection has been closed; no further writes are permitted.
    TRANSPORT_CLOSED            = 200,
    /// Transport fallback was initiated and the primary transport is unavailable.
    TRANSPORT_FALLBACK_ACTIVE   = 201,
    /// Neither primary nor fallback transport is available.
    TRANSPORT_UNAVAILABLE       = 202,
    /// Write to transport failed; data may not have been delivered.
    TRANSPORT_WRITE_ERROR       = 203,

    // ── Rate Limiting / Backpressure ─────────────────────────────────────────
    /// Request rate limit exceeded; caller should back off.
    RATE_LIMITED                = 300,
    /// Server backpressure limit reached; no new requests accepted.
    BACKPRESSURE_EXCEEDED       = 301,
    /// Client has been temporarily banned due to repeated rate violations.
    CLIENT_BANNED               = 302,

    // ── Quorum / Cluster ──────────────────────────────────────────────────────
    /// Cluster quorum is degraded; operation may not be consistent.
    QUORUM_DEGRADED             = 400,
    /// Routing table is unavailable; request cannot be forwarded.
    ROUTING_UNAVAILABLE         = 401,

    // ── Connection Lifecycle ──────────────────────────────────────────────────
    /// Connection limit kMaxConcurrentConnections has been reached.
    CONNECTION_LIMIT_REACHED    = 500,
    /// Connection was rejected during the drain/shutdown phase.
    SERVER_DRAINING             = 501,

    // ── Generic ───────────────────────────────────────────────────────────────
    /// Operation succeeded.
    OK                          = 0,
    /// Unclassified internal network error; always fail-closed.
    INTERNAL_ERROR              = 9999,
};

// ============================================================================
// § 7  Fail-Closed Classification Helpers
// ============================================================================

/**
 * @brief Returns true when @p code mandates immediate connection closure.
 *
 * Fail-closed (connection-closing) codes: FRAME_INVALID, FRAME_OVERSIZED,
 * AUTH_TIMEOUT, TRANSPORT_CLOSED, TRANSPORT_UNAVAILABLE, INTERNAL_ERROR.
 */
[[nodiscard]] inline constexpr bool isConnectionClosingError(NetworkErrorCode code) noexcept {
    switch (code) {
        case NetworkErrorCode::FRAME_INVALID:
        case NetworkErrorCode::FRAME_OVERSIZED:
        case NetworkErrorCode::AUTH_TIMEOUT:
        case NetworkErrorCode::TRANSPORT_CLOSED:
        case NetworkErrorCode::TRANSPORT_UNAVAILABLE:
        case NetworkErrorCode::INTERNAL_ERROR:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Returns true when @p code is a transient rate-limit code that the
 *        caller MAY retry after a Retry-After interval.
 */
[[nodiscard]] inline constexpr bool isRateLimitTransient(NetworkErrorCode code) noexcept {
    return code == NetworkErrorCode::RATE_LIMITED
        || code == NetworkErrorCode::BACKPRESSURE_EXCEEDED;
}

// ============================================================================
// § 8  Contract Conformance Notes
//
// All network module components MUST:
//   1. Close connections (not silently drop) on FRAME_INVALID or FRAME_OVERSIZED.
//   2. Evaluate the session guard before invoking any privileged handler.
//   3. Emit an audit event for AUTH_REQUIRED, SESSION_EXPIRED, RATE_LIMITED.
//   4. Never write application data after the connection enters DRAINING/CLOSED.
//   5. Surface NetworkErrorCode values (or Expected<T, NetworkErrorCode>).
// ============================================================================

}  // namespace network
}  // namespace themis
