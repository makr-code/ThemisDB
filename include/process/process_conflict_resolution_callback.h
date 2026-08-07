/**
 * @file process_conflict_resolution_callback.h
 * @brief Application-level conflict resolution callback interface.
 *
 * Defines callback contracts for custom multi-model conflict resolution strategies
 * beyond Last-Write-Wins, allowing applications to implement domain-specific conflict policies.
 *
 * @version 2.1.0
 * @date 2026-08-06
 * @status PHASE_1_DESIGN (Q1 2027)
 *
 * ## Overview
 *
 * When two replicas detect a concurrent update to the same model version, the Process Module
 * can invoke application-provided callbacks to determine the winning version. This enables
 * conflict resolution strategies beyond LWW:
 *
 * - **Last-Write-Wins (LWW):** Default; tie-breaking by node ID
 * - **First-Write-Wins (FWW):** Earliest timestamp wins
 * - **Application-Custom:** Callback determines winner based on domain logic
 *
 * ## Callback Contract
 *
 * ### Synchronous Execution
 * - Callback invoked during consensus commit phase (blocking)
 * - Must return within 5s timeout; otherwise exception caught, LWW fallback applied
 * - Called once per conflict; result cached for subsequent apply phases
 *
 * ### Exception Handling
 * - All exceptions caught; logged with incident (CALLBACK_FAILED)
 * - LWW fallback applied if callback fails
 * - Operation proceeds with fallback result; no crash
 *
 * ### Callback Context
 * - All metadata passed: versions, timestamps, senders, operation details
 * - Sufficient to make informed decision without external lookups
 * - Context immutable during callback execution
 *
 * ## Usage Pattern
 *
 * @code
 * class MyProcessConflictResolver : public ProcessConflictResolverCallback {
 *   std::string Resolve(const ConflictMetadata& metadata) override {
 *     // Custom logic: choose based on business rules
 *     if (metadata.v1_timestamp > metadata.v2_timestamp) return metadata.v1_id;
 *     else return metadata.v2_id;
 *   }
 * };
 *
 * auto resolver = std::make_shared<MyProcessConflictResolver>();
 * conflict_mgr->RegisterResolver(resolver);
 * @endcode
 *
 * @see process_federation_contract.h – Federated concurrency model
 * @see ROADMAP_FEDERATION.md – Phase 1-6 implementation plan
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_CONFLICT_RESOLUTION_CALLBACK_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_CONFLICT_RESOLUTION_CALLBACK_H

#include <string>
#include <cstdint>
#include <chrono>
#include <memory>

namespace themis::process {

// ============================================================================
// CONFLICT METADATA
// ============================================================================

/**
 * @brief Metadata for conflict resolution decision.
 *
 * Passed to callback during conflict detection. Contains all information
 * needed to make resolution decision without external lookups.
 */
struct ConflictMetadata {
  /// First model version ID
  std::string v1_id;

  /// Second model version ID
  std::string v2_id;

  /// Version 1 timestamp (from version clock)
  std::chrono::system_clock::time_point v1_timestamp;

  /// Version 2 timestamp (from version clock)
  std::chrono::system_clock::time_point v2_timestamp;

  /// Version 1 source node ID (for deterministic tie-breaking)
  std::string v1_sender_node_id;

  /// Version 2 source node ID (for deterministic tie-breaking)
  std::string v2_sender_node_id;

  /// Model ID that experienced conflict
  std::string model_id;

  /// Operation type that caused conflict (insert, update, delete)
  std::string operation_type;

  /// Model size (bytes) for both versions
  std::uint64_t v1_size_bytes = 0;
  std::uint64_t v2_size_bytes = 0;

  /// Confidence in conflict detection (0.0-1.0)
  float detection_confidence = 1.0f;

  /// Additional context as JSON string
  std::string context_json;
};

// ============================================================================
// CONFLICT RESOLUTION STRATEGIES
// ============================================================================

/**
 * @brief Conflict resolution strategy enumeration.
 */
enum class ConflictResolutionStrategy : std::uint8_t {
  /// Last-Write-Wins with tie-breaking by node ID
  kLastWriteWins = 0,

  /// First-Write-Wins (earliest timestamp)
  kFirstWriteWins = 1,

  /// Application-provided callback determines winner
  kApplicationCallback = 2,

  /// Error: no strategy configured (default to LWW)
  kUnconfigured = 3,
};

// ============================================================================
// CALLBACK INTERFACE
// ============================================================================

/**
 * @brief Abstract interface for application-provided conflict resolution.
 *
 * Applications implement this interface to provide domain-specific conflict
 * resolution logic beyond LWW.
 */
class ProcessConflictResolverCallback {
 public:
  /**
   * @brief Resolve conflict between two model versions.
   *
   * @param metadata Conflict metadata (versions, timestamps, senders, etc.)
   * @return ID of winning version (must be either metadata.v1_id or metadata.v2_id)
   *
   * @throws std::exception If resolution fails; caught by consensus layer (LWW fallback)
   * @note Must complete within 5 seconds; timeout triggers LWW fallback
   * @note Result must be deterministic (same metadata → same result)
   * @invariant Returned ID must equal either metadata.v1_id or metadata.v2_id
   */
  virtual std::string Resolve(const ConflictMetadata& metadata) = 0;

  /**
   * @brief Optional: Get resolver name for logging/diagnostics.
   * @return Resolver name (e.g., "MyApplicationResolver")
   */
  virtual std::string GetName() const { return "ProcessConflictResolverCallback"; }

  virtual ~ProcessConflictResolverCallback() = default;
};

// ============================================================================
// CALLBACK MANAGEMENT
// ============================================================================

/**
 * @brief Conflict resolution callback manager.
 *
 * Manages resolver registration, invocation, error handling, and fallback.
 *
 * @note Actual implementation in src/process/process_conflict_resolver.cpp
 */
class ConflictResolverManager {
 public:
  /**
   * @brief Register application-provided conflict resolver.
   *
   * @param resolver Resolver implementation
   * @note Only one resolver active at a time; new registration replaces old
   */
  virtual void RegisterResolver(
      std::shared_ptr<ProcessConflictResolverCallback> resolver) = 0;

  /**
   * @brief Resolve conflict using registered resolver or LWW fallback.
   *
   * @param metadata Conflict metadata
   * @return ID of winning version
   *
   * @note If no resolver registered, uses LWW fallback
   * @note If resolver throws exception, uses LWW fallback (incident emitted)
   * @note If resolver timeout (5s), uses LWW fallback (incident emitted)
   */
  virtual std::string ResolveConflict(const ConflictMetadata& metadata) = 0;

  /**
   * @brief Get current resolution strategy.
   * @return Active strategy (LWW, FWW, ApplicationCallback, Unconfigured)
   */
  virtual ConflictResolutionStrategy GetCurrentStrategy() const = 0;

  /**
   * @brief Unregister resolver; fall back to LWW.
   */
  virtual void UnregisterResolver() = 0;

  virtual ~ConflictResolverManager() = default;
};

// ============================================================================
// CALLBACK RESULT & DIAGNOSTICS
// ============================================================================

/**
 * @brief Result of conflict resolution.
 *
 * Contains outcome, metadata, and diagnostics for auditing.
 */
struct ConflictResolutionResult {
  /// Winning version ID
  std::string winner_id;

  /// Strategy used (callback, LWW fallback, error)
  ConflictResolutionStrategy strategy_used;

  /// Was callback invoked?
  bool callback_invoked = false;

  /// Callback execution time (milliseconds)
  std::uint64_t callback_execution_ms = 0;

  /// Did callback throw exception (caught & fallback)?
  bool callback_exception = false;

  /// Exception message (if callback_exception == true)
  std::string exception_message;

  /// Deterministic outcome (same metadata → same result)?
  bool is_deterministic = true;

  /// Timestamp of resolution
  std::chrono::system_clock::time_point resolution_time;
};

// ============================================================================
// BUILT-IN STRATEGIES (Implementation in Phase 2)
// ============================================================================

/**
 * @brief Last-Write-Wins resolver (default).
 *
 * Selects version with later timestamp; tie-breaking by node ID (lexicographic).
 */
class LastWriteWinsResolver : public ProcessConflictResolverCallback {
 public:
  std::string Resolve(const ConflictMetadata& metadata) override;
  std::string GetName() const override { return "LastWriteWins"; }
};

/**
 * @brief First-Write-Wins resolver.
 *
 * Selects version with earlier timestamp; tie-breaking by node ID (lexicographic).
 */
class FirstWriteWinsResolver : public ProcessConflictResolverCallback {
 public:
  std::string Resolve(const ConflictMetadata& metadata) override;
  std::string GetName() const override { return "FirstWriteWins"; }
};

// ============================================================================
// DOCUMENTATION SECTIONS (Design-Phase Specifications)
// ============================================================================

/**
 * @section callback_requirements Requirements for Application Resolvers
 *
 * 1. **Determinism:** Callback must return same result for same input
 *    - Forbidden: Random choice, external lookups, time-based logic
 *    - Allowed: Deterministic math, version comparison, metadata-based logic
 *
 * 2. **Timeout Compliance:** Must complete within 5 seconds
 *    - Exceeded timeout → exception caught → LWW fallback applied
 *    - Timeout enforced by consensus layer (not application responsibility)
 *
 * 3. **Exception Safety:** All exceptions caught; LWW fallback applied
 *    - Callback crash does not crash consensus (fail-closed)
 *    - Exception logged with CALLBACK_FAILED diagnostic incident
 *
 * 4. **Metadata Sufficiency:** All needed info in ConflictMetadata
 *    - No external database lookups
 *    - No RPC calls (prohibited; timeout risk)
 *    - Decision made solely from provided metadata
 *
 * @section callback_testing Test Scenarios (Phase 4)
 *
 * | Test | Scenario | Expected |
 * |------|----------|----------|
 * | FCR-01 | Custom callback invoked | Callback result used |
 * | FCR-02 | Callback timeout (5s+) | LWW fallback applied |
 * | FCR-03 | Callback exception | LWW fallback applied |
 * | FCR-05 | Callback determinism | Same input → same output |
 * | FCR-07 | Callback context | All metadata passed |
 * | FCR-11 | No callback registered | LWW fallback used |
 *
 * @section performance_expectations Performance Expectations
 *
 * | Operation | P95 | P99 | Budget |
 * |-----------|-----|-----|--------|
 * | Callback latency | 1-2ms | 5-10ms | ≤10ms |
 * | LWW fallback | <1ms | <1ms | ≤5ms |
 * | Total conflict resolution | 5-25ms | 20-100ms | ≤50ms |
 */

}  // namespace themis::process

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_CONFLICT_RESOLUTION_CALLBACK_H
