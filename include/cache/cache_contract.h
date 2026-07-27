/*
 * ThemisDB | File: cache_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen cache-contract semantics for the active v1.x major line.
 */

/**
 * @file cache_contract.h
 * @brief Frozen cache-module contract semantics for the active v1.x line.
 *
 * This header defines the normative contract for all cache-module components:
 * - Adaptive query cache (AdaptiveQueryCache, BoundedLRUCache, …)
 * - Embedding and semantic caches (EmbeddingCache, SemanticCache)
 * - Cache coordination and replication surfaces
 *   (ICacheCoordinator, CacheReplicationCoordinator, RedisCacheCoordinator, …)
 * - Admin and observability paths
 *   (CacheAdminApiHandler, CacheHitRateSloMonitor, …)
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB cache pipeline:
 *   - get/put/invalidate operations on all tier levels (L1/L2/L3).
 *   - tenant-aware keying and quota-enforcement paths.
 *   - replication event propagation and invalidation delivery.
 *   - coordinator degradation and fail-closed decision paths.
 *   - warmup/prefetch pre-population paths.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry before merge.
 *
 * @see src/cache/ROADMAP.md — Phase 1 item
 * @see src/cache/FUTURE_ENHANCEMENTS.md — Design Constraints section
 * @see include/cache/cache_interfaces.h — concrete interface declarations
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace themis {
namespace cache {

// ============================================================================
// § 1  Cache key, value, and tenant-ID size constraints
//
// These limits are enforced at every inbound boundary (API handlers, TCP
// streams, gRPC metadata, internal API calls).  Inputs exceeding these limits
// are rejected before any serialisation or hashing work, preventing
// memory-exhaustion and algorithmic-complexity attacks.
// ============================================================================

/// Maximum accepted cache key size in bytes (fingerprint + prefix overhead).
/// Keys exceeding this limit are rejected with a MalformedKey failure.
/// Kept in sync with the SHA-256 hex output length (64 bytes) plus tenant
/// prefix; practical effective ceiling for human-readable keys.
inline constexpr std::size_t kMaxCacheKeyBytes = 4'096;

/// Maximum accepted cache value size in bytes per entry (hard cap).
///
/// Entries whose serialised JSON value exceeds this limit are rejected by
/// put() before any tier-selection or compression work.  512 MiB matches
/// the multi-tier design assumption for very large OLAP result sets.
/// In most deployments per-tier limits (l1_max_entry_size, l2_max_entry_size)
/// apply before this global hard cap.
inline constexpr std::size_t kMaxCacheValueBytes = 512ULL * 1024 * 1024;

/// Maximum tenant-ID length in UTF-8 bytes.
/// Tenant IDs exceeding this limit are rejected with a MalformedKey failure.
/// Kept in sync with the URL-path budget in CacheAdminApiHandler route parsing.
inline constexpr std::size_t kMaxTenantIdBytes = 256;

/// Minimum non-empty tenant ID length in bytes.
/// Empty string is reserved for the global (no-tenant) namespace; a non-empty
/// tenant ID that is shorter than this limit is rejected as malformed.
inline constexpr std::size_t kMinTenantIdNonEmptyBytes = 1;

// ============================================================================
// § 2  Temporal contract
//
// All time comparisons use steady_clock for duration measurements and
// system_clock for TTL expiry evaluation.  Clock-skew tolerance is not
// applied across nodes; operators must ensure NTP synchronisation.
// ============================================================================

/// Maximum acceptable L3 retrieval latency before the L3 read path is
/// considered degraded and the circuit breaker may open.
/// Implementations that exceed this p99 threshold must surface the degraded
/// state via ICacheAdminOps::stats() and the SLO monitor.
inline constexpr std::chrono::milliseconds kMaxL3RetrievalLatency{5'000};

/// Maximum time allowed for a coordinator state transition (OPEN → HALF_OPEN
/// → CLOSED).  Transitions taking longer than this value are logged as
/// degraded-coordination incidents and surfaced via diagnostics.
inline constexpr std::chrono::seconds kMaxCoordinatorTransitionTime{30};

/// Maximum acceptable replication delivery delay for a single
/// invalidation/entry event.  Events not delivered within this window are
/// treated as lost; the coordinator must record the failure and not retry
/// indefinitely without bounded backoff.
inline constexpr std::chrono::milliseconds kMaxReplicationDeliveryMs{30'000};

/// Minimum TTL that can be stored in any cache tier (enforces non-zero
/// TTL at ingest time; entries with TTL < kMinCacheTtlSeconds are rejected).
inline constexpr int kMinCacheTtlSeconds = 1;

// ============================================================================
// § 3  Failure classification
//
// All cache components must map their internal error states to one of these
// canonical failure classes.  This enables uniform operator diagnostics and
// consistent fail-closed / fail-open policy enforcement.
//
// The category names map directly to the `category` field in the cache admin
// diagnostic JSON envelope:
//   { "category": "degraded", "component": "redis", "retryable": true, … }
// ============================================================================

/**
 * @brief Canonical failure classes for cache operation errors.
 *
 * Every failure path in the cache pipeline must carry one of these classes so
 * that callers can apply uniform policy regardless of the underlying adapter
 * or tier.
 *
 * Integer values are stable across v1.x; do NOT renumber or remove entries.
 */
enum class CacheFailureClass : int {
    /// Input key is structurally malformed: empty, oversized, invalid encoding,
    /// or fails tenant-ID format validation.
    MalformedKey = 1,

    /// Input value is structurally invalid: JSON primitive (not object/array),
    /// null value, or exceeds the kMaxCacheValueBytes hard cap.
    MalformedValue = 2,

    /// The requested operation is not permitted in the current cache state
    /// (e.g. put on a read-only cache, invalidate on a sealed snapshot).
    InvalidOperation = 3,

    /// A tenant-aware operation was rejected because the caller attempted to
    /// access a key outside their assigned tenant namespace.
    TenantViolation = 4,

    /// A backend tier (L3 / Redis / gRPC coordinator) is unreachable or
    /// returned an unexpected error.
    /// Treated as fail-closed: operations that require the backend are denied
    /// unless a locally-cached positive confirmation is still valid.
    DegradedBackend = 5,

    /// A distributed invalidation or replication event completed partially:
    /// some peers acknowledged, others timed out or rejected.
    /// The local cache operation succeeded; the partial delivery is surfaced
    /// as a non-fatal diagnostic event.
    PartialDelivery = 6,

    /// Unclassified internal error; always fail-closed.
    InternalError = 7,
};

// ============================================================================
// § 4  Fail-closed contract
//
// All cache decision paths MUST default to denial / rejection (fail-closed)
// when:
//   a) The failure class is DegradedBackend or InternalError.
//   b) A mandatory tenant contract check cannot be confirmed.
//   c) The coordinator backend is unreachable and no locally-cached state
//      provides a sufficiently fresh confirmation.
//
// Fail-OPEN behaviour (degraded but still serving) is ONLY allowed for
// explicitly whitelisted paths documented with operator opt-in configuration.
// ============================================================================

/**
 * @brief Returns true when the given failure class mandates fail-closed denial.
 *
 * Use this predicate in catch/error-handling blocks to decide whether to
 * propagate the denial or attempt a graceful fallback:
 *
 * @code
 *   try {
 *       result = coordinator->publishEntry(key, value, ttl, tenant_id);
 *   } catch (const std::exception& ex) {
 *       const auto fc = classifyCacheFailure(ex);
 *       if (themis::cache::isFailClosedClass(fc)) {
 *           throw;  // hard denial — no fallback permitted
 *       }
 *       // partial delivery or format issue — log and continue
 *   }
 * @endcode
 *
 * @param fc The failure class to evaluate.
 * @return true if @p fc mandates fail-closed behaviour.
 */
[[nodiscard]] inline constexpr bool isFailClosedClass(CacheFailureClass fc) noexcept {
    return fc == CacheFailureClass::DegradedBackend
        || fc == CacheFailureClass::InternalError;
}

/**
 * @brief Returns true when the failure is due to a malformed input artifact.
 *
 * Malformed inputs (MalformedKey, MalformedValue) are always rejected
 * immediately without fallback, but for diagnostic purposes they are
 * classified separately from fail-closed backend failures.
 *
 * @param fc The failure class to evaluate.
 * @return true if @p fc indicates a malformed input rejection.
 */
[[nodiscard]] inline constexpr bool isMalformedInputClass(CacheFailureClass fc) noexcept {
    return fc == CacheFailureClass::MalformedKey
        || fc == CacheFailureClass::MalformedValue;
}

// ============================================================================
// § 5  Backend availability contract
//
// Cache tier availability checks MUST satisfy all of the following:
//   - L1/L2 availability checks are O(1) and never block on I/O.
//   - L3 (RocksDB) and coordinator (Redis/gRPC) reachability checks must
//     use the circuit breaker before accessing the backend.
//   - If a backend becomes unavailable, operations requiring that backend
//     return DegradedBackend; they do NOT silently fall back to a higher tier
//     for write or invalidation operations.
//   - Warmup/prefetch operations tolerate backend unavailability by skipping
//     individual entries and recording them as skipped in WarmupStats.
// ============================================================================

/// Maximum staleness of a locally-cached coordinator confirmation that is
/// still considered authoritative when the primary coordinator is unreachable.
/// After this window expires, the fail-closed predicate applies.
inline constexpr std::chrono::seconds kCoordinatorCacheMaxStaleness{120};

/// Maximum number of consecutive L3 or coordinator failures before the
/// circuit breaker opens.  Implementations MUST use this constant (or a
/// configurable override bounded by this value) as the default threshold.
inline constexpr uint32_t kCircuitBreakerDefaultFailureThreshold = 5;

/// Minimum backoff between coordinator retry attempts.  Implementations MUST
/// wait at least this long between successive attempts when the circuit breaker
/// is in HALF_OPEN state.
inline constexpr std::chrono::milliseconds kCoordinatorMinRetryBackoff{100};

// ============================================================================
// § 6  Coordinator capability contract
//
// Distributed coordinator adapters MUST declare their capability requirement
// before performing any outbound call.  If the capability is not available at
// runtime, the adapter must surface a DegradedBackend failure class rather
/// than blocking or timing out silently.
// ============================================================================

/**
 * @brief Capability flags that a coordinator adapter may require at runtime.
 *
 * Flags are OR-combinable with operator|.  Declare composite requirements by
 * combining flags:
 * @code
 *   auto needed = CoordinatorCapability::Redis | CoordinatorCapability::TLS;
 *   if (!hasCoordinatorCapability(runtime_caps, needed)) { ... }
 * @endcode
 */
enum class CoordinatorCapability : unsigned int {
    None      = 0u,
    /// Adapter requires a reachable Redis server with valid pub/sub channels.
    Redis     = 1u << 0,
    /// Adapter requires active gRPC connections to all configured peer nodes.
    gRPC      = 1u << 1,
    /// Adapter uses the in-process local bus (always available; no network).
    LocalBus  = 1u << 2,
    /// Adapter requires TLS/mTLS for all outbound coordinator connections.
    TLS       = 1u << 3,
};

/// Bitwise-OR for CoordinatorCapability flags.
[[nodiscard]] inline constexpr CoordinatorCapability operator|(
        CoordinatorCapability a, CoordinatorCapability b) noexcept {
    return static_cast<CoordinatorCapability>(
        static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

/// Bitwise-AND for CoordinatorCapability flags (intersection).
[[nodiscard]] inline constexpr CoordinatorCapability operator&(
        CoordinatorCapability a, CoordinatorCapability b) noexcept {
    return static_cast<CoordinatorCapability>(
        static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

/**
 * @brief Returns true when @p set includes every bit in @p required.
 *
 * @param set      Bitmask of capabilities available at runtime.
 * @param required Bitmask of capabilities required by the adapter.
 * @return true if all bits in @p required are set in @p set.
 */
[[nodiscard]] inline constexpr bool hasCoordinatorCapability(
        CoordinatorCapability set, CoordinatorCapability required) noexcept {
    return (static_cast<unsigned int>(set) & static_cast<unsigned int>(required))
        == static_cast<unsigned int>(required);
}

// ============================================================================
// § 7  Async replication and invalidation consistency contract
//
// Async replication paths (CacheReplicationCoordinator, RedisCacheCoordinator)
// MUST obey all of the following:
//   - Exceptions from peer delivery are caught per-peer and never propagate
//     to the local cache operation (local operation always completes).
//   - A per-event timeout (bounded by kMaxReplicationDeliveryMs) fires before
//     abandoning a delivery attempt.
//   - After timeout: the peer is marked unhealthy; the failure is surfaced in
//     diagnostics but does NOT roll back the local cache operation.
//   - Partial delivery (some peers acked, others timed out) is classified as
//     PartialDelivery, NOT DegradedBackend; the operator diagnostic must
//     enumerate which peers failed and which succeeded.
//   - Invalidation events are processed in sequence-number order on each peer
//     to preserve monotonic invalidation semantics; out-of-order events are
//     discarded (not re-applied).
// ============================================================================

/// Default per-event replication timeout.  Used by CacheReplicationCoordinator
/// when no explicit timeout is configured.
inline constexpr std::chrono::milliseconds kAsyncReplicationDefaultTimeout{30'000};

/// Hard maximum allowed per-event replication timeout.
/// Operator-configurable timeout values MUST be bounded by this constant.
inline constexpr std::chrono::milliseconds kAsyncReplicationMaxTimeout{120'000};

/// Sequence number used to indicate an unsequenced (legacy) replication event.
/// New implementations MUST assign monotonically increasing sequence numbers;
/// events with this value are accepted but cannot participate in ordering.
inline constexpr uint64_t kUnsequencedReplicationEvent = 0;

} // namespace cache
} // namespace themis
