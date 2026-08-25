/**
 * @file network_observability.h
 * @brief Unified per-connection trace and metric emission interface for ThemisDB network module.
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Interface-Stable
 */

// ThemisDB – Network Observability Interface
//
// Provides a unified, low-overhead interface for emitting per-connection
// telemetry (traces, metrics, and structured events) across all network
// front doors: TCP wire protocol, WebSocket, UDP fast-path, QUIC/HTTP3,
// and gRPC.
//
// Design goals:
//   - Zero-copy event assembly: events are built on the stack and forwarded
//     to a registered sink; no heap allocation on the hot path.
//   - Sink-agnostic: the default no-op sink discards all events; callers
//     register their own sink (e.g., OpenTelemetry, Prometheus, syslog).
//   - Thread-safe: the sink pointer is set once at startup; per-connection
//     scoped spans are stack-allocated and require no global synchronisation.
//   - Composable: each component (WireProtocolServer, QuicTransport, etc.)
//     constructs a NetworkSpan scoped to its logical operation, then calls
//     emit() on completion.
//
// Usage pattern:
// @code
//   auto& obs = NetworkObservabilityRegistry::instance();
//   NetworkSpan span(obs, NetworkSpanKind::FRAME_DISPATCH);
//   span.setConnectionId(conn_id);
//   span.setOpcode(opcode);
//   // ... process request ...
//   span.setResultCode(NetworkResultCode::OK);
//   span.emit(); // forwards to registered sink
// @endcode

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

namespace themis {
namespace network {

// =============================================================================
// Span kinds and result codes
// =============================================================================

/**
 * @brief Classification of the network operation being observed.
 */
enum class NetworkSpanKind : uint8_t {
    FRAME_DISPATCH,       ///< A complete wire-protocol frame was dispatched
    FRAME_RECEIVE,        ///< A wire-protocol frame was received and parsed
    AUTH_CHECK,           ///< Authentication or session guard check
    RATE_LIMIT_CHECK,     ///< Rate-limit guard evaluation
    CONNECTION_ACCEPT,    ///< New TCP/QUIC/WS connection accepted
    CONNECTION_CLOSE,     ///< Connection gracefully closed or timed out
    TRANSPORT_FALLBACK,   ///< Transport fallback was triggered (e.g., QUIC → TCP)
    ROUTING_DECISION,     ///< A load-balancer or topology routing decision
    CIRCUIT_BREAKER,      ///< Circuit breaker state checked or transitioned
    BATCH_SEND,           ///< A batch of frames was submitted (io_uring / batch)
    TLS_HANDSHAKE,        ///< TLS/QUIC handshake completed or failed
    GRPC_CALL,            ///< A gRPC RPC call started or completed
    CUSTOM,               ///< Custom span defined by the emitting component
};

/**
 * @brief Normalised result codes for network operations.
 */
enum class NetworkResultCode : uint8_t {
    OK,
    ERROR_FRAME_INVALID,
    ERROR_AUTH_REQUIRED,
    ERROR_SESSION_EXPIRED,
    ERROR_RATE_LIMITED,
    ERROR_BACKPRESSURE,
    ERROR_TRANSPORT_CLOSED,
    ERROR_QUORUM_DEGRADED,
    ERROR_TIMEOUT,
    ERROR_TLS_FAILURE,
    ERROR_CIRCUIT_OPEN,
    ERROR_ROUTING_FAILED,
    ERROR_INTERNAL,
    PENDING,              ///< Span still in progress (not yet complete)
};

// =============================================================================
// Metric counters (lock-free per-component aggregation)
// =============================================================================

/**
 * @brief Lightweight per-component network metric aggregator.
 *
 * All counters are atomic and updated with relaxed memory order (suitable
 * for monotonic counters where strict ordering is not required).
 * Call @c snapshot() to obtain a consistent read (acquires all atomics).
 */
struct NetworkMetrics {
    std::atomic<uint64_t> frames_dispatched{0};
    std::atomic<uint64_t> frames_received{0};
    std::atomic<uint64_t> auth_checks_passed{0};
    std::atomic<uint64_t> auth_checks_failed{0};
    std::atomic<uint64_t> rate_limit_allows{0};
    std::atomic<uint64_t> rate_limit_drops{0};
    std::atomic<uint64_t> connections_accepted{0};
    std::atomic<uint64_t> connections_closed{0};
    std::atomic<uint64_t> transport_fallbacks{0};
    std::atomic<uint64_t> circuit_breaker_trips{0};
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_received{0};
    std::atomic<uint64_t> errors_total{0};

    NetworkMetrics() = default;

    // Non-copyable (atomics), non-movable.
    NetworkMetrics(const NetworkMetrics&)            = delete;
    NetworkMetrics& operator=(const NetworkMetrics&) = delete;

    /**
     * @brief Snapshot all counters into a plain (non-atomic) struct.
     */
    struct Snapshot {
        uint64_t frames_dispatched     = 0;
        uint64_t frames_received       = 0;
        uint64_t auth_checks_passed    = 0;
        uint64_t auth_checks_failed    = 0;
        uint64_t rate_limit_allows     = 0;
        uint64_t rate_limit_drops      = 0;
        uint64_t connections_accepted  = 0;
        uint64_t connections_closed    = 0;
        uint64_t transport_fallbacks   = 0;
        uint64_t circuit_breaker_trips = 0;
        uint64_t bytes_sent            = 0;
        uint64_t bytes_received        = 0;
        uint64_t errors_total          = 0;
    };

    /// Acquire-load all counters and return a consistent snapshot.
    Snapshot snapshot() const noexcept {
        Snapshot s;
        s.frames_dispatched     = frames_dispatched.load(std::memory_order_acquire);
        s.frames_received       = frames_received.load(std::memory_order_acquire);
        s.auth_checks_passed    = auth_checks_passed.load(std::memory_order_acquire);
        s.auth_checks_failed    = auth_checks_failed.load(std::memory_order_acquire);
        s.rate_limit_allows     = rate_limit_allows.load(std::memory_order_acquire);
        s.rate_limit_drops      = rate_limit_drops.load(std::memory_order_acquire);
        s.connections_accepted  = connections_accepted.load(std::memory_order_acquire);
        s.connections_closed    = connections_closed.load(std::memory_order_acquire);
        s.transport_fallbacks   = transport_fallbacks.load(std::memory_order_acquire);
        s.circuit_breaker_trips = circuit_breaker_trips.load(std::memory_order_acquire);
        s.bytes_sent            = bytes_sent.load(std::memory_order_acquire);
        s.bytes_received        = bytes_received.load(std::memory_order_acquire);
        s.errors_total          = errors_total.load(std::memory_order_acquire);
        return s;
    }
};

// =============================================================================
// Span event (stack-allocated, forwarded to sink on emit())
// =============================================================================

/**
 * @brief Structured event representing a single observed network operation.
 *
 * Built on the stack by the emitting component.  No heap allocation occurs
 * until the sink's @c onSpan() implementation decides to store the event.
 */
struct NetworkSpanEvent {
    NetworkSpanKind   kind              = NetworkSpanKind::CUSTOM;
    NetworkResultCode result            = NetworkResultCode::PENDING;

    uint64_t          connection_id     = 0;    ///< Opaque connection identifier
    uint32_t          session_id        = 0;    ///< Optional session ID (0 = none)
    uint16_t          opcode            = 0;    ///< Wire-protocol opcode (if applicable)
    uint16_t          transport_tag     = 0;    ///< Transport type tag (TCP=1,WS=2,UDP=3,QUIC=4,gRPC=5)

    uint64_t          payload_bytes     = 0;    ///< Payload size in bytes
    uint32_t          latency_us        = 0;    ///< Operation latency in microseconds

    /// Wall-clock timestamp at the start of the operation.
    std::chrono::system_clock::time_point timestamp;

    /// Optional free-form label (e.g., route name, error class, span name).
    /// Points into caller-owned storage; valid only during @c onSpan().
    std::string_view  label;

    /// Optional error message (empty string = no error detail).
    std::string_view  error_detail;
};

// =============================================================================
// Observability sink interface
// =============================================================================

/**
 * @brief Abstract sink for network observability events.
 *
 * Register a concrete implementation via
 * @c NetworkObservabilityRegistry::setSink(). The default (null) sink
 * discards all events.
 *
 * Implementations MUST be thread-safe: @c onSpan() may be called from
 * multiple threads concurrently.
 */
class INetworkObservabilitySink {
public:
    virtual ~INetworkObservabilitySink();

    /**
     * @brief Called once per completed span with the event descriptor.
     *
     * @param event  Stack-allocated event; do not store a pointer to it
     *               beyond the duration of this call.
     */
    virtual void onSpan(const NetworkSpanEvent& event) noexcept = 0;

    /**
     * @brief Called periodically (e.g., every 10 s) with aggregated metrics.
     *
     * The default implementation is a no-op.
     *
     * @param metrics  Read-only metrics snapshot from @c NetworkMetrics::snapshot().
     */
    virtual void onMetricsFlush([[maybe_unused]] const NetworkMetrics::Snapshot& metrics) noexcept {
        (void)metrics;
    }
};

// =============================================================================
// Global observability registry (singleton)
// =============================================================================

/**
 * @brief Process-wide registry for the network observability sink and metrics.
 *
 * Thread safety: @c setSink() must be called before I/O threads start.
 * @c getSink() and @c metrics() are safe to call from any thread.
 */
class NetworkObservabilityRegistry {
public:
    /// Return the process-wide singleton instance.
    static NetworkObservabilityRegistry& instance() noexcept {
        static NetworkObservabilityRegistry registry;
        return registry;
    }

    // Non-copyable, non-movable singleton.
    NetworkObservabilityRegistry(const NetworkObservabilityRegistry&)            = delete;
    NetworkObservabilityRegistry& operator=(const NetworkObservabilityRegistry&) = delete;

    /**
     * @brief Register the observability sink.
     *
     * @note Call before starting any network I/O threads.  The registry does
     *       NOT take ownership; the sink must outlive the registry.
     *
     * @param sink  Pointer to the sink implementation, or nullptr to reset
     *              to the null (discard) sink.
     */
    void setSink(INetworkObservabilitySink* sink) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        sink_ = sink;
    }

    /**
     * @brief Return the currently registered sink (may be nullptr).
     */
    INetworkObservabilitySink* getSink() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return sink_;
    }

    /**
     * @brief Return a reference to the global per-process metrics aggregator.
     */
    NetworkMetrics& metrics() noexcept { return metrics_; }
    const NetworkMetrics& metrics() const noexcept { return metrics_; }

    /**
     * @brief Forward a span event to the registered sink (no-op if no sink).
     *
     * This is the hot-path emit call. It is inline to avoid a function-call
     * overhead when no sink is registered.
     */
    void emit(const NetworkSpanEvent& event) noexcept {
        INetworkObservabilitySink* s = getSink();
        if (s) {
            s->onSpan(event);
        }
    }

private:
    NetworkObservabilityRegistry() = default;

    mutable std::mutex         mutex_;
    INetworkObservabilitySink* sink_{nullptr};
    NetworkMetrics             metrics_;
};

// =============================================================================
// RAII span helper
// =============================================================================

/**
 * @brief RAII helper that auto-times and auto-emits a @c NetworkSpanEvent.
 *
 * Constructs a @c NetworkSpanEvent on the stack, starts a steady_clock timer
 * on construction, and calls @c NetworkObservabilityRegistry::instance().emit()
 * on destruction.  Callers set fields via fluent setters before the span goes
 * out of scope.
 *
 * Usage:
 * @code
 *   {
 *       NetworkSpan span(NetworkSpanKind::AUTH_CHECK);
 *       span.setConnectionId(conn_id).setResultCode(NetworkResultCode::OK);
 *       // ... perform auth check ...
 *   } // auto-emits on scope exit
 * @endcode
 */
class NetworkSpan {
public:
    explicit NetworkSpan(NetworkSpanKind kind) noexcept
        : start_(std::chrono::steady_clock::now())
    {
        event_.kind      = kind;
        event_.timestamp = std::chrono::system_clock::now();
    }

    ~NetworkSpan() noexcept { emit(); }

    // Non-copyable, non-movable (timer semantics).
    NetworkSpan(const NetworkSpan&)            = delete;
    NetworkSpan& operator=(const NetworkSpan&) = delete;

    NetworkSpan& setConnectionId(uint64_t id) noexcept {
        event_.connection_id = id;
        return *this;
    }

    NetworkSpan& setSessionId(uint32_t id) noexcept {
        event_.session_id = id;
        return *this;
    }

    NetworkSpan& setOpcode(uint16_t opcode) noexcept {
        event_.opcode = opcode;
        return *this;
    }

    NetworkSpan& setTransportTag(uint16_t tag) noexcept {
        event_.transport_tag = tag;
        return *this;
    }

    NetworkSpan& setPayloadBytes(uint64_t bytes) noexcept {
        event_.payload_bytes = bytes;
        return *this;
    }

    NetworkSpan& setResultCode(NetworkResultCode code) noexcept {
        event_.result = code;
        return *this;
    }

    NetworkSpan& setLabel(std::string_view label) noexcept {
        event_.label = label;
        return *this;
    }

    NetworkSpan& setErrorDetail(std::string_view detail) noexcept {
        event_.error_detail = detail;
        return *this;
    }

    /// Manually emit the span before scope exit (idempotent).
    void emit() noexcept {
        if (emitted_) return;
        emitted_ = true;
        const auto now = std::chrono::steady_clock::now();
        const auto us  = std::chrono::duration_cast<std::chrono::microseconds>(
                             now - start_).count();
        event_.latency_us = static_cast<uint32_t>(
            us > UINT32_MAX ? UINT32_MAX : us);
        NetworkObservabilityRegistry::instance().emit(event_);
    }

private:
    std::chrono::steady_clock::time_point start_;
    NetworkSpanEvent                       event_{};
    bool                                   emitted_{false};
};

} // namespace network
} // namespace themis
