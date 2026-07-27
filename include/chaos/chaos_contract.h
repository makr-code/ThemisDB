/*
 * ThemisDB | File: chaos_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen fault descriptor and scheduler contract semantics for the active v1.x major line.
 */

/**
 * @file chaos_contract.h
 * @brief Frozen fault descriptor and scheduler contract semantics for the active v1.x line.
 *
 * This header defines the normative contract for chaos fault injection and scheduler
 * behaviour that all chaos module components must honour in the current major release line.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB chaos simulation pipeline:
 *   - FaultInjector (in-process fault registry)
 *   - ChaosScheduler (time-driven fault scheduler)
 *   - EventCallback consumers (lifecycle event observers)
 *   - Test fixtures that query isFaultActive() or pendingCount()
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/chaos/ROADMAP.md — Phase 1 item
 * @see include/chaos/chaos_framework.h — implementation header
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string_view>

namespace themis {
namespace chaos {

// ============================================================================
// § 1  Fault descriptor constraints
//
// Current v1.x runtime guarantees:
//   - injectFault() rejects empty node_id and probability outside [0.0, 1.0].
//   - schedule()/scheduleIn() accept entries without additional range checks.
//
// Additional constants in this section are frozen interoperability guardrails
// for callers/tests and future hardening work; they are not yet uniformly
// enforced by chaos_framework.cpp.
// ============================================================================

/// Maximum recommended node identifier length in UTF-8 bytes (not currently
/// enforced by the v1.x FaultInjector runtime).
inline constexpr std::size_t kMaxNodeIdBytes = 512;

/// Maximum recommended fault description length in UTF-8 bytes (not currently
/// enforced or truncated by the v1.x FaultSpec/FaultInjector runtime).
inline constexpr std::size_t kMaxDescriptionBytes = 4096;

/// Valid probability range for RANDOM_FAILURE faults: [0.0, 1.0].
/// Values outside this range are rejected by injectFault() with a false return.
inline constexpr double kMinProbability = 0.0;
inline constexpr double kMaxProbability = 1.0;

/// An empty node_id is always rejected; the minimum accepted length is 1 byte.
inline constexpr std::size_t kMinNodeIdBytes = 1;

// ============================================================================
// § 2  Temporal contract
//
// All time comparisons use std::chrono::steady_clock.  The scheduler fires
// faults as soon as their trigger_at time has elapsed within the tick window.
// ============================================================================

/// Maximum recommended fault duration for integration scenarios.
/// Current v1.x runtime does not clamp durations above this bound.
inline constexpr std::chrono::hours kMaxFaultDuration{24 * 7};  ///< 7 days

/// Maximum recommended scheduler tick_interval for ChaosSchedulerConfig.
/// Current v1.x runtime does not enforce this upper bound.
inline constexpr std::chrono::milliseconds kMaxTickInterval{60'000};  ///< 60 s

/// Minimum recommended scheduler tick_interval to prevent busy-spin.
/// Current v1.x runtime does not enforce this lower bound.
inline constexpr std::chrono::milliseconds kMinTickInterval{1};  ///< 1 ms

/// Maximum recommended number of pending entries in the ChaosScheduler queue.
/// Current v1.x runtime does not enforce queue-cap drops at this limit.
inline constexpr std::size_t kMaxPendingEntries = 4096;

// ============================================================================
// § 3  Failure classification
//
// All chaos components must map their internal error states to one of these
// canonical failure classes so that callers can apply uniform diagnostics.
// ============================================================================

/**
 * @brief Canonical failure classes for chaos module errors.
 *
 * Every failure detectable from outside the chaos module maps to one of these
 * classes, enabling uniform operator diagnostics regardless of the triggering
 * component (FaultInjector vs ChaosScheduler).
 */
enum class ChaosFailureClass : int {
    /// Input is structurally malformed (empty node_id, probability out of range,
    /// tick_interval out of bounds, null injector pointer, …).
    MalformedDescriptor = 1,

    /// The operation was attempted on a component that is not in a valid state
    /// (e.g., scheduling a fault to a stopped scheduler, recovering from an
    /// empty registry).
    InvalidState        = 2,

    /// A time-related constraint was violated (duration exceeds kMaxFaultDuration,
    /// tick_interval out of [kMinTickInterval, kMaxTickInterval]).
    TemporalViolation   = 3,

    /// The fault registry or scheduler queue has reached its capacity limit
    /// (kMaxPendingEntries exceeded, active fault registry full).
    CapacityExceeded    = 4,

    /// An unexpected internal error occurred; always results in a no-op return.
    InternalError       = 5,
};

// ============================================================================
// § 4  Fail-closed contract
//
// Current v1.x fail-closed behavior is limited to:
//   a) injectFault(): return false on empty node_id or probability out of range.
//   b) recoverFault(): return false if no matching active fault exists.
//   c) schedule()/scheduleIn(): always queue; entries fire only while RUNNING.
//
// Callers MUST NOT rely on undefined behaviour when inputs are out of range.
// ============================================================================

/**
 * @brief Returns true when the given failure class mandates a fail-closed no-op.
 *
 * @param fc  The failure class to classify.
 * @return    true if the class requires silent rejection (no state mutation).
 *
 * @note All classes currently mandate fail-closed semantics; this helper exists
 *       to allow future fine-grained recovery for non-critical classes.
 */
[[nodiscard]] inline constexpr bool isFailClosedClass(ChaosFailureClass fc) noexcept {
    switch (fc) {
        case ChaosFailureClass::MalformedDescriptor: return true;
        case ChaosFailureClass::InvalidState:        return true;
        case ChaosFailureClass::TemporalViolation:   return true;
        case ChaosFailureClass::CapacityExceeded:    return true;
        case ChaosFailureClass::InternalError:       return true;
    }
    return true;  // default to fail-closed for unknown future values
}

// ============================================================================
// § 5  Callback / event semantics
//
// FaultInjector::registerEventCallback() consumers must honour the following
// invariants to remain correct under concurrent use:
//   1. Callbacks are invoked synchronously on the thread that calls
//      injectFault() or recoverFault().
//   2. recoverFault() currently dispatches while fault_mutex_ is held; callbacks
//      should not re-enter the same FaultInjector from that call path.
//      injectFault() dispatches after registry mutation is complete.
//   3. Callbacks are invoked in registration order (FIFO).
//   4. Callback exceptions are not caught by the framework.
//   5. Callback arguments are passed as (const FaultSpec&, bool injected).
// ============================================================================

/// Maximum recommended number of event callbacks for a single FaultInjector.
/// Current v1.x runtime does not enforce a hard registration cap.
inline constexpr std::size_t kMaxEventCallbacks = 64;

// ============================================================================
// § 6  Scheduler state contract
//
// ChaosScheduler state transitions must obey the following finite-state machine:
//
//   STOPPED ──start()──► RUNNING ──stop()──► STOPPED
//
// Additional invariants:
//   - start() on a RUNNING scheduler is idempotent (no-op, no exception).
//   - stop() on a STOPPED scheduler is idempotent (no-op, no exception).
//   - schedule() / scheduleIn() called on a STOPPED scheduler MAY queue the
//     entry but the entry will NOT be fired until start() is called again.
//   - clearPending() is safe to call from any state (STOPPED or RUNNING).
//   - The scheduler thread MUST exit within kSchedulerStopTimeoutMs after
//     stop() is called; callers may observe isRunning() transitioning to false.
// ============================================================================

/// Maximum expected latency (in milliseconds) from stop() call to isRunning()
/// returning false.  Tests that poll isRunning() after stop() should budget at
/// least this window before reporting failure.
inline constexpr std::chrono::milliseconds kSchedulerStopTimeout{500};

// ============================================================================
// § 7  Process-local blast-radius contract
//
// The chaos module is strictly in-process.  The following invariants hold
// unconditionally regardless of active fault mix or scheduler state:
//   1. No real network packet is suppressed or delayed by FaultInjector.
//   2. No real disk I/O is blocked by FaultInjector.
//   3. isFaultActive() never blocks (returns under lock in bounded time).
//   4. injectFault() / recoverFault() never perform I/O or cross-process IPC.
//   5. FaultInjector state is not persisted across process restarts.
//   6. ChaosScheduler uses a single background thread; its CPU usage is bounded
//      by the tick_interval and the number of pending entries.
// ============================================================================

}  // namespace chaos
}  // namespace themis
