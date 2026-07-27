// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file cdc_delivery_contract.h
 * @brief Frozen CDC delivery, transport, and replay contract semantics for the
 *        active v1.x major release line.
 *
 * This header defines the normative contract that all CDC module components
 * must honour.  It is the single authoritative source for:
 *   - Event sequence and ordering guarantees.
 *   - At-least-once delivery semantics and acknowledgement rules.
 *   - Replay session scope and state-machine contract.
 *   - Transport degradation classification and fail-closed policy.
 *   - Consumer-group partition consistency rules.
 *   - Diagnostic observability obligations.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * accompanied by migration notes and a CHANGELOG entry.
 *
 * @see src/cdc/ROADMAP.md — Phase 1 contract freeze
 * @see include/cdc/delivery_tracker.h — §2 at-least-once implementation
 * @see include/cdc/icdc_replay_controller.h — §3 replay state machine
 * @see include/cdc/consumer_group.h — §5 partition consistency
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace themis {
namespace cdc {

// ============================================================================
// § 1  Event sequence constraints
//
// All change events carry a monotonically increasing, gap-free sequence number
// assigned by the Changefeed.  Consumers MUST use the sequence as the canonical
// resume cursor.
// ============================================================================

/// Maximum sequence number value.  Overflow is not expected in production;
/// if the sequence approaches this value the operator must take a manual
/// snapshot and reset the stream.
inline constexpr uint64_t kMaxEventSequence = UINT64_MAX - 1;

/// Sequence value reserved to mean "no sequence / not yet assigned".
inline constexpr uint64_t kNoSequence = 0;

/// Maximum accepted event key size in bytes.  Keys exceeding this limit are
/// rejected at recordEvent() time with an appropriate CDCException.
inline constexpr std::size_t kMaxEventKeyBytes = 4096;

/// Maximum accepted event value payload size in bytes.
inline constexpr std::size_t kMaxEventValueBytes = 64 * 1024 * 1024; // 64 MiB

// ============================================================================
// § 2  At-least-once delivery semantics
//
// The DeliveryTracker implements at-least-once delivery.  All components that
// wrap or compose DeliveryTracker MUST preserve these guarantees.
// ============================================================================

/// Default acknowledgement timeout: an event unacknowledged within this window
/// is eligible for redelivery.  Callers MUST NOT assume a longer window
/// without explicit per-request override via DeliveryTrackerConfig or the
/// timeout_override parameter.
inline constexpr std::chrono::milliseconds kDefaultAckTimeout{30'000};

/// Maximum allowed per-request ack-timeout override.  Values above this are
/// clamped or rejected by the delivery layer.
inline constexpr std::chrono::milliseconds kMaxAckTimeoutOverride{300'000};

/// Maximum in-flight (unacknowledged) events tracked per consumer by default.
/// Once reached, new trackDelivery() calls MUST return false to apply back-
/// pressure; the caller MUST NOT silently drop events.
inline constexpr std::size_t kDefaultMaxPendingPerConsumer = 10'000;

/// An event acknowledged after expiry (max_redelivery_attempts exceeded) MUST
/// be reported via ConsumerDeliveryStats::total_expired, NOT silently discarded.
/// This rule ensures diagnostics are always accurate regardless of late ACKs.

// ============================================================================
// § 3  Replay session contract
//
// ICDCReplayController and IReplaySession implementations must obey the
// following state-machine transitions.  Deviations are defects.
//
//   Active  → Done      (all events in window delivered or window empty)
//   Active  → Cancelled (cancel() called before drain complete)
//   Done    → Done      (terminal; nextBatch() always returns {})
//   Cancelled → Cancelled (terminal; nextBatch() always returns {})
//
// done() MUST return true for both Done and Cancelled states.
// ============================================================================

/// Maximum events per replay session when no explicit limit is set.
/// Implementations MUST enforce this to avoid unbounded memory consumption.
inline constexpr std::size_t kDefaultMaxReplayEventsPerSession = 1'000'000;

/// Default replay batch size: number of events returned per nextBatch() call.
inline constexpr std::size_t kDefaultReplayBatchSize = 100;

// ============================================================================
// § 4  Transport degradation failure classes
//
// Every CDC transport failure MUST be classified into one of these canonical
// classes so that operators receive consistent, actionable diagnostics.
// ============================================================================

/**
 * @brief Canonical transport failure classes for CDC component diagnostics.
 *
 * All ICDCTransport implementations and transport-dependent CDC components must
 * map their internal error states to one of these classes.
 */
enum class CdcTransportFailureClass : int {
    /// Transient connectivity loss; automatic retry is appropriate.
    Transient           = 1,

    /// Target backend is fully unreachable; retry with backoff.
    BackendUnreachable  = 2,

    /// Authentication or authorisation to the transport endpoint failed;
    /// operator intervention required.
    AuthFailure         = 3,

    /// Event payload is malformed or exceeds size limits; must be dead-lettered.
    PayloadInvalid      = 4,

    /// Transport capacity is exhausted (queue full, rate-limited);
    /// caller MUST apply back-pressure.
    BackpressureRequired = 5,

    /// Duplicate event detected by the transport; silently discard.
    DuplicateEvent      = 6,

    /// Unclassified internal transport error; always fail-closed.
    InternalError       = 7,
};

// ============================================================================
// § 5  Fail-closed contract for transport degradation
//
// When a transport failure prevents a deterministic delivery decision, all CDC
// components MUST default to fail-closed:
//
//   a) Event is enqueued in the DeadLetterQueue (not silently dropped).
//   b) Diagnostics MUST record the failure class and stream context.
//   c) Consumer delivery state is NOT advanced until explicit ACK is received.
//
// Fail-open behaviour is ONLY allowed for CdcTransportFailureClass::Transient
// with documented retry policy and operator opt-in configuration.
// ============================================================================

/**
 * @brief Returns true when the transport failure class mandates fail-closed
 *        behaviour (dead-letter or halt, no automatic drop or advance).
 *
 * @param fc  Transport failure class to evaluate.
 * @return true if the event must be dead-lettered or delivery must halt.
 */
[[nodiscard]] inline constexpr bool isCdcFailClosedClass(
        CdcTransportFailureClass fc) noexcept {
    return fc == CdcTransportFailureClass::BackendUnreachable
        || fc == CdcTransportFailureClass::AuthFailure
        || fc == CdcTransportFailureClass::InternalError
        || fc == CdcTransportFailureClass::BackpressureRequired
        || fc == CdcTransportFailureClass::PayloadInvalid;
}

// ============================================================================
// § 6  Consumer-group partition consistency rules
//
// ConsumerGroupManager MUST honour all of the following:
//   a) Partition assignment for a given (key, consumer_count) pair is
//      deterministic and stateless: same inputs always produce the same output.
//   b) Two distinct consumer IDs in the same group MUST receive disjoint key
//      subsets (no overlapping partitions for valid configurations).
//   c) Committed offsets MUST be persisted to RocksDB before acknowledgeEvents()
//      returns to the caller; memory-only state is not sufficient.
//   d) fetchEventsAtLeastOnce() MUST resume from the last committed offset when
//      the consumer reconnects; it must not re-scan from sequence 0.
// ============================================================================

/// Maximum consumer count per group.  Groups larger than this value are
/// rejected at createGroup() time with a CDCException.
inline constexpr uint32_t kMaxConsumerGroupSize = 1024;

/// Maximum number of concurrent groups per Changefeed instance.
inline constexpr uint32_t kMaxConsumerGroupsPerChangefeed = 256;

// ============================================================================
// § 7  Diagnostic observability obligations
//
// All CDC components that handle events MUST emit actionable diagnostics for:
//   - Redelivery events: consumer_id, sequence, attempt count.
//   - DLQ promotions: failure reason, event key, timestamp.
//   - Lag growth: consumer_id, current lag, threshold.
//   - Transport failures: failure class, stream context, retry count.
//
// Diagnostics MUST include both stream-id and consumer-group context where
// applicable so operators can correlate across components.
// ============================================================================

/// Lag threshold above which a consumer is considered lagging and diagnostics
/// MUST be emitted.  Implementations SHOULD emit a structured warning when
/// the pending count for a consumer exceeds this value.
inline constexpr std::size_t kConsumerLagWarningThreshold = 1'000;

/// Maximum redelivery attempts before diagnostics escalate to ERROR severity.
inline constexpr uint32_t kRedeliveryEscalationThreshold = 3;

} // namespace cdc
} // namespace themis
