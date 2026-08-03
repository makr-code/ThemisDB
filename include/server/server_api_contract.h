/*
 * ThemisDB | File: server_api_contract.h | Version: 1.0.0
 * Author: ThemisDB Contributors | Maturity: 🟢 PRODUCTION-READY
 * Status: Phase 1 — Frozen Contract
 * Purpose: Frozen server API contract semantics for the active v1.x major line.
 */

/**
 * @file server_api_contract.h
 * @brief Frozen server module API contracts for the active v1.x line.
 *
 * @note **Header-Only Contract**: This file defines frozen semantics and invariants.
 *       No .cpp implementation needed. Consumers link to implementations of the contracts
 *       (e.g., HttpServer, Http2Session, WebSocketSession, ApiGateway, etc.).
 *
 * This header defines the normative contract for all server module components
 * including HTTP/gRPC/WebSocket/MQTT handler registration, auth gate enforcement,
 * retry/timeout/backpressure semantics, graceful shutdown ordering, error taxonomy,
 * and threading guarantees.
 *
 * @section scope Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB server pipeline:
 *   - HTTP/1.1, HTTP/2, HTTP/3 request handlers (HttpServer, Http2Session, Http3Session)
 *   - gRPC service implementations (RpcServiceImpl, ThemisCoreGrpcService)
 *   - WebSocket sessions (WebSocketSession)
 *   - MQTT sessions (MqttSession, MqttClientService)
 *   - API gateway (ApiGateway, DistributedGateway)
 *   - Auth middleware (AuthMiddleware)
 *   - Rate limiting middleware (RateLimitingMiddleware, AdaptiveRateLimiter)
 *   - Graceful shutdown manager (HttpShutdownManager)
 *
 * @section versioning Versioning
 *
 * This contract is stable within v1.x. Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/server/ROADMAP.md — Phase 1 item
 * @see tests/server/test_server_contract_hardening_focused.cpp — SCH-01..SCH-20
 * @see benchmarks/server/bench_server_hotpaths.cpp — SVR-01..SVR-08
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace themis {
namespace server {

// ============================================================================
// § 1  Handler Registration Constraints
//
// All handler registrations MUST satisfy the following before the first request
// is accepted. Violations cause a hard startup assertion (fail-fast).
// ============================================================================

/// Maximum allowed HTTP route path length in bytes.
inline constexpr std::size_t kMaxRoutePathBytes = 4096;

/// Maximum number of simultaneously registered routes per protocol.
inline constexpr std::size_t kMaxRegisteredRoutes = 4096;

/// Maximum allowed request body size (16 MiB) enforced at the transport layer.
/// Requests exceeding this limit are rejected with 413 before handler dispatch.
inline constexpr std::size_t kMaxRequestBodyBytes = 16u * 1024u * 1024u;

/// Maximum gRPC metadata value size in bytes.
inline constexpr std::size_t kMaxGrpcMetadataValueBytes = 8192;

/// Maximum MQTT payload size in bytes.
inline constexpr std::size_t kMaxMqttPayloadBytes = 256u * 1024u;

// ============================================================================
// § 2  Auth Gate Contract
//
// Every inbound request MUST pass through the auth gate before handler
// dispatch. The gate is applied in the following order:
//   1. Transport-layer TLS verification (if applicable)
//   2. JWT/API-key token extraction and structural validation
//   3. Token signature and expiry verification
//   4. Scope/permission gate for the target route
//
// A request that fails at any gate step MUST receive AUTH_GATE_DENIED (HTTP 401
// or gRPC UNAUTHENTICATED). Handler code MUST NOT execute on a denied request.
//
// Routes explicitly marked as public (e.g., /health, /metrics/prometheus) are
// exempt from JWT validation but MUST still pass rate-limit checks.
// ============================================================================

/// Default clock-skew tolerance for JWT exp/nbf comparisons.
inline constexpr std::chrono::seconds kAuthClockSkewTolerance{30};

/// Maximum accepted JWT token size at the server transport boundary.
inline constexpr std::size_t kServerMaxJwtTokenBytes = 16u * 1024u;

// ============================================================================
// § 3  Retry / Backoff Contract
//
// Retry behaviour is governed by the following invariants:
//   - Only TRANSIENT_ERROR class failures are eligible for retry.
//   - FATAL_ERROR and INVALID_INPUT failures must fail-fast (no retry).
//   - The exponential backoff formula: delay = base * 2^(attempt-1) + jitter
//   - The global retry budget (wall-clock) caps the total retry window.
//   - After budget exhaustion the caller receives RETRY_BUDGET_EXHAUSTED.
//   - Idempotency: retried requests MUST carry the same idempotency key.
// ============================================================================

/// Default maximum number of retry attempts per request.
inline constexpr int kDefaultMaxRetries = 3;

/// Default base delay for exponential backoff.
inline constexpr std::chrono::milliseconds kRetryBaseDelay{100};

/// Default global retry budget (wall-clock window for all retry attempts).
inline constexpr std::chrono::seconds kRetryGlobalBudget{30};

/// Maximum allowed retry base delay (operator-configurable upper bound).
inline constexpr std::chrono::milliseconds kRetryMaxBaseDelay{5000};

// ============================================================================
// § 4  Timeout Contract
//
// Request handler timeouts are enforced at the server layer before handler code
// can run indefinitely. The semantics are:
//   - Per-request deadline is set at request admission time.
//   - If the handler does not complete within the deadline, the server calls
//     the handler's cancellation path and returns TRANSPORT_TIMEOUT to the caller.
//   - Graceful shutdown MUST drain all in-flight requests within
//     kShutdownDrainTimeout before forcing connection close.
//   - Keepalive idle timeout triggers connection recycling (no error response).
// ============================================================================

/// Default per-request handler timeout.
inline constexpr std::chrono::seconds kDefaultRequestTimeout{30};

/// Maximum allowed per-request handler timeout (operator-configurable).
inline constexpr std::chrono::seconds kMaxRequestTimeout{300};

/// Default graceful shutdown drain timeout.
inline constexpr std::chrono::seconds kShutdownDrainTimeout{60};

/// Hard maximum for graceful shutdown drain (after which connections are forced closed).
inline constexpr std::chrono::seconds kShutdownDrainMaxTimeout{120};

/// Keepalive idle timeout before connection recycling.
inline constexpr std::chrono::seconds kKeepaliveIdleTimeout{90};

// ============================================================================
// § 5  Backpressure Contract
//
// The server applies backpressure to prevent resource exhaustion:
//   - When the in-flight request queue reaches kBackpressureQueueLimit, new
//     requests are rejected with BACKPRESSURE_SHED (HTTP 503 / gRPC UNAVAILABLE).
//   - WebSocket frame backpressure: if the write buffer exceeds
//     kWebSocketWriteBufferLimit, the session MUST pause reading from the peer
//     until the buffer drains below kWebSocketWriteBufferResumeThreshold.
//   - MQTT session backpressure follows the same write-buffer contract.
// ============================================================================

/// In-flight request queue depth that triggers backpressure shedding.
inline constexpr std::size_t kBackpressureQueueLimit = 8192;

/// Per-session WebSocket write buffer limit in bytes before read-pause.
inline constexpr std::size_t kWebSocketWriteBufferLimit = 1u * 1024u * 1024u;

/// WebSocket write buffer drain threshold before reads resume.
inline constexpr std::size_t kWebSocketWriteBufferResumeThreshold = 256u * 1024u;

// ============================================================================
// § 6  Error Taxonomy
//
// All server components MUST map internal error states to one of these canonical
// error classes. This enables uniform operator diagnostics and consistent
// fail-closed/fail-open enforcement across transports.
// ============================================================================

/**
 * @brief Canonical server error classes.
 *
 * Values are stable across v1.x. Any addition of a new class requires a
 * CHANGELOG entry. Removal or renumbering requires a v2.0 major bump.
 */
enum class ServerErrorClass : int {
    /// Request succeeded.
    OK                     =  0,
    /// Auth gate denied the request (invalid/missing/expired credential).
    AUTH_GATE_DENIED       =  1,
    /// Transport-layer timeout: handler did not complete within deadline.
    TRANSPORT_TIMEOUT      =  2,
    /// Graceful shutdown drain exceeded kShutdownDrainMaxTimeout.
    SHUTDOWN_DRAIN_TIMEOUT =  3,
    /// Per-client or global rate limit exceeded.
    RATE_LIMIT_EXCEEDED    =  4,
    /// Malformed protocol frame or violates structural wire contract.
    PROTOCOL_VIOLATION     =  5,
    /// Request body or parameter fails schema/JSON validation.
    INPUT_VALIDATION_ERROR =  6,
    /// All retry attempts exhausted within the global retry budget.
    RETRY_BUDGET_EXHAUSTED =  7,
    /// Request shed due to backpressure queue limit.
    BACKPRESSURE_SHED      =  8,
    /// gRPC/raft quorum unavailable (≥ n/2+1 nodes unreachable).
    QUORUM_UNAVAILABLE     =  9,
    /// API version not supported or version contract mismatch.
    VERSION_MISMATCH       = 10,
    /// Underlying transport is in the process of draining/shutting down.
    SERVER_DRAINING        = 11,
    /// Unclassified internal error; always results in 500 / INTERNAL.
    INTERNAL_ERROR         = 12,
};

// ============================================================================
// § 7  Lifecycle and Ownership
//
// Request handler ownership:
//   - The server owns the request object until handler completion or timeout.
//   - Handlers MUST NOT retain references to the request object beyond their
//     synchronous call frame; async continuations must copy required data.
//   - Response ownership transfers to the server on handler return.
//
// Connection lifecycle:
//   - Connections are owned exclusively by the session layer.
//   - Handlers access connections through a stable reference counted handle;
//     the handle becomes invalid after session close.
//   - Sessions are destroyed asynchronously; handlers MUST check validity
//     before any post-handler access.
// ============================================================================

// ============================================================================
// § 8  Threading Guarantees
//
// Middleware stack thread-safety:
//   - The auth middleware is thread-safe: concurrent calls from different
//     request threads are safe. Internal token cache uses reader-writer locking.
//   - Rate limit middleware is thread-safe for in-memory backends. Distributed
//     state backends add network latency but remain safe under concurrent access.
//   - Route registry lookup (OpenApiRouteRegistry) is read-only after server
//     start and is lock-free in the hot read path.
//   - Handler dispatch is per-request concurrent; handlers MUST NOT share
//     mutable state without explicit synchronisation.
//   - Shutdown sequencing (HttpShutdownManager) is serialised via an internal
//     state machine; concurrent shutdown calls are idempotent.
// ============================================================================

/// Returns true when the given error class requires fail-closed behaviour
/// (i.e., the server MUST deny rather than attempting any fallback).
[[nodiscard]] inline constexpr bool isServerFailClosedClass(ServerErrorClass ec) noexcept {
    return ec == ServerErrorClass::AUTH_GATE_DENIED
        || ec == ServerErrorClass::QUORUM_UNAVAILABLE
        || ec == ServerErrorClass::INTERNAL_ERROR
        || ec == ServerErrorClass::SHUTDOWN_DRAIN_TIMEOUT;
}

// ============================================================================
// § 9  Graceful Shutdown Ordering Contract
//
// The shutdown sequence MUST follow this strict ordering:
//   1. Stop accepting new connections (kRunning → kDraining).
//   2. Drain existing in-flight requests (up to kShutdownDrainTimeout).
//   3. Run pre-shutdown health checks and notify dependent services.
//   4. Force-close any remaining connections after timeout.
//   5. Transition to kStopped state.
//
// No handler dispatch is permitted in kDraining or kStopped states.
// Health probes to /health MAY return 503 during kDraining.
// ============================================================================

/**
 * @brief Server lifecycle state machine.
 *
 * Transitions: kIdle → kRunning → kDraining → kStopped.
 * No reverse transitions are permitted.
 */
enum class ServerState : int {
    kIdle     = 0, ///< Server constructed but not yet started.
    kRunning  = 1, ///< Accepting and processing requests.
    kDraining = 2, ///< Draining in-flight requests; no new connections accepted.
    kStopped  = 3, ///< Fully stopped; no I/O activity.
};

// ============================================================================
// § 10  Rate Limit State Contract
//
// Rate limit enforcement semantics:
//   - In-memory (single-node) rate limits are evaluated atomically per client key.
//   - Distributed state (multi-node) rate limits use eventual consistency with
//     a configurable synchronisation interval (kRateLimitSyncInterval).
//   - On distributed backend failure the rate limiter MUST fail-closed:
//     all requests that cannot be evaluated are rejected with RATE_LIMIT_EXCEEDED.
//   - Per-client limit resets are based on sliding window (default 60s).
// ============================================================================

/// Default per-client rate limit window.
inline constexpr std::chrono::seconds kRateLimitWindow{60};

/// Default synchronisation interval for distributed rate limit state.
inline constexpr std::chrono::milliseconds kRateLimitSyncInterval{500};

} // namespace server
} // namespace themis
