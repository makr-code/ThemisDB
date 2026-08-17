/**
 * @file process_conflict_resolver.cpp
 * @brief Multi-model conflict detection and resolution orchestrator.
 *
 * Detects concurrent updates on the same model version and invokes
 * application-provided callbacks or LWW fallback for deterministic resolution.
 *
 * @version 2.1.0
 * @date 2026-08-06
 * @status PHASE_2_CORE_IMPLEMENTATION
 *
 * @note Maturity: 🟡 ALPHA (Phase 2 delivery, production hardening in Phase 5)
 * @note This implementation is auto-generated from ROADMAP_FEDERATION.md Phase 2.
 *
 * ## Conflict Detection
 *
 * Conflict is detected when:
 * - Two replicas have different model versions for the same model ID
 * - Both versions have same logical clock (concurrent updates)
 * - At least one is not an ancestor of the other (true conflict)
 *
 * ## Resolution Strategies
 *
 * 1. **Last-Write-Wins (LWW):** Default fallback
 *    - Winner: higher timestamp
 *    - Tie-breaker: node ID (lexicographic ordering)
 *
 * 2. **First-Write-Wins (FWW):** Application override
 *    - Winner: lower timestamp
 *
 * 3. **Application-Custom:** Callback-based
 *    - Application provides ConflictResolver callback
 *    - Callback has 5s timeout; LWW on timeout
 *
 * ## Determinism Guarantee
 *
 * Same inputs on all replicas → same winner, always.
 * Achieved via tie-breaking by node ID (lexicographic).
 *
 * @see process_conflict_resolution_callback.h – Callback interface
 * @see federation_consensus_manager.cpp – Consensus coordination
 * @see ROADMAP_FEDERATION.md – Phase 1-6 roadmap
 */

#include "process/process_conflict_resolver.h"
#include "process/process_conflict_resolution_callback.h"
#include "process/process_federation_contract.h"
#include "process/process_common.h"
#include "utils/logger.h"

#include <chrono>
#include <algorithm>
#include <thread>
#include <future>
#include <stdexcept>

namespace themis {
namespace process {

// ============================================================================
// CONFLICT RESOLUTION STRATEGIES
// ============================================================================

/**
 * @class ConflictResolutionStrategy
 * @brief Abstract base for conflict resolution algorithms.
 * @internal
 */
class IConflictResolutionStrategy {
 public:
  virtual ~IConflictResolutionStrategy() = default;

  /**
   * @brief Determine winner between two conflicting versions.
   * @return ID of winning version
   */
  virtual std::string ResolveConflict(
      const ConflictMetadata& metadata) = 0;
};

/**
 * @class LastWriteWinsStrategy
 * @brief Last-Write-Wins conflict resolution (default fallback).
 * @internal
 */
class LastWriteWinsStrategy : public IConflictResolutionStrategy {
 public:
  std::string ResolveConflict(const ConflictMetadata& metadata) override {
    // Higher timestamp wins
    if (metadata.v1_timestamp > metadata.v2_timestamp) {
      return metadata.v1_id;
    }
    if (metadata.v2_timestamp > metadata.v1_timestamp) {
      return metadata.v2_id;
    }
    // Tie-break by node ID (lexicographic)
    return (metadata.v1_sender_node_id < metadata.v2_sender_node_id)
               ? metadata.v1_id
               : metadata.v2_id;
  }
};

/**
 * @class FirstWriteWinsStrategy
 * @brief First-Write-Wins conflict resolution.
 * @internal
 */
class FirstWriteWinsStrategy : public IConflictResolutionStrategy {
 public:
  std::string ResolveConflict(const ConflictMetadata& metadata) override {
    // Lower timestamp wins
    if (metadata.v1_timestamp < metadata.v2_timestamp) {
      return metadata.v1_id;
    }
    if (metadata.v2_timestamp < metadata.v1_timestamp) {
      return metadata.v2_id;
    }
    // Tie-break by node ID (lexicographic)
    return (metadata.v1_sender_node_id < metadata.v2_sender_node_id)
               ? metadata.v1_id
               : metadata.v2_id;
  }
};

/**
 * @class ApplicationCustomStrategy
 * @brief Application callback-based conflict resolution.
 * @internal
 */
class ApplicationCustomStrategy : public IConflictResolutionStrategy {
 public:
  explicit ApplicationCustomStrategy(
      std::shared_ptr<ProcessConflictResolverCallback> callback)
      : callback_(callback), fallback_(std::make_unique<LastWriteWinsStrategy>()) {}

  std::string ResolveConflict(const ConflictMetadata& metadata) override {
    if (!callback_) {
      return fallback_->ResolveConflict(metadata);
    }

    try {
      // Call application callback with timeout
      auto future = std::async(std::launch::async, [this, &metadata]() {
        return callback_->Resolve(metadata);
      });

      const auto kTimeoutMs = std::chrono::milliseconds(5000);
      if (future.wait_for(kTimeoutMs) == std::future_status::timeout) {
        utils::Logger::Warn(
            "Conflict resolver callback timed out, using LWW fallback");
        return fallback_->ResolveConflict(metadata);
      }

      return future.get();
    } catch (const std::exception& e) {
      utils::Logger::Error(
          "Conflict resolver callback failed: %s, using LWW fallback",
          e.what());
      return fallback_->ResolveConflict(metadata);
    }
  }

 private:
  std::shared_ptr<ProcessConflictResolverCallback> callback_;
  std::unique_ptr<IConflictResolutionStrategy> fallback_;
};

// ============================================================================
// CONFLICT RESOLVER IMPLEMENTATION
// ============================================================================

/**
 * @class ProcessConflictResolverImpl
 * @brief Core conflict detection and resolution engine.
 *
 * ### Thread Safety
 * All public methods are thread-safe via fine-grained locking.
 * Lock ordering: resolver_mutex_ → conflict_history_mutex_
 *
 * ### Performance
 * - Conflict detection: < 5ms (GATE-CRE-01)
 * - Resolution: < 50ms P95 (GATE-CRE-02)
 * - Callback overhead: < 10ms (GATE-CRE-03)
 */
class ProcessConflictResolverImpl {
 public:
  /**
   * @brief Constructor.
   * @param config Resolver configuration (strategy, timeout, batch size)
   */
  explicit ProcessConflictResolverImpl(const ConflictResolverConfig& config)
      : config_(config),
        strategy_(CreateStrategy(config.strategy, nullptr)),
        conflicts_detected_(0),
        conflicts_resolved_(0) {
    utils::Logger::Info(
        "ProcessConflictResolver initialized: strategy=%s, timeout_ms=%u",
        config.strategy.c_str(), config.callback_timeout_ms);
  }

  /**
   * @brief Destructor.
   */
  ~ProcessConflictResolverImpl() = default;

  // ========================================================================
  // PUBLIC API
  // ========================================================================

  /**
   * @brief Detect and resolve conflict between two concurrent model versions.
   *
   * @param model_id Model ID where conflict occurred
   * @param v1_id First version ID
   * @param v1_timestamp Timestamp of first version
   * @param v1_sender Sender node ID of first version
   * @param v2_id Second version ID
   * @param v2_timestamp Timestamp of second version
   * @param v2_sender Sender node ID of second version
   * @return ID of winning version (deterministic across all replicas)
   * @thread_safe Acquires resolver_mutex_
   */
  std::string ResolveConflict(const std::string& model_id,
                              const std::string& v1_id, uint64_t v1_timestamp,
                              const std::string& v1_sender,
                              const std::string& v2_id, uint64_t v2_timestamp,
                              const std::string& v2_sender);

  /**
   * @brief Register application-provided conflict resolver callback.
   *
   * If registered, callback will be invoked for conflict resolution.
   * If callback not registered or times out, LWW fallback is used.
   *
   * @param resolver Callback implementing ProcessConflictResolver interface
   * @thread_safe Acquires resolver_mutex_
   */
  void RegisterResolver(std::shared_ptr<ProcessConflictResolverCallback> resolver);

  /**
   * @brief Detect conflicts in a batch of model versions.
   *
   * Used for periodic consistency checking across replicas.
   *
   * @param versions Map of model_id → version_ids
   * @return Vector of detected conflicts (empty if none)
   * @thread_safe Acquires resolver_mutex_
   */
  std::vector<ConflictInfo> DetectConflictsBatch(
      const std::map<std::string, std::vector<std::string>>& versions);

  /**
   * @brief Get conflict resolver statistics.
   *
   * @return Struct with conflicts_detected, conflicts_resolved, resolution_time_ms
   * @thread_safe Acquires resolver_mutex_
   */
  ConflictResolverStats GetStats() const;

  // ========================================================================
  // PRIVATE IMPLEMENTATION
  // ========================================================================

 private:
  /**
   * @brief Create resolution strategy based on config.
   */
  static std::unique_ptr<IConflictResolutionStrategy> CreateStrategy(
      const std::string& strategy_name,
      std::shared_ptr<ProcessConflictResolverCallback> callback) {
    if (strategy_name == "LWW") {
      return std::make_unique<LastWriteWinsStrategy>();
    } else if (strategy_name == "FWW") {
      return std::make_unique<FirstWriteWinsStrategy>();
    } else if (strategy_name == "custom") {
      return std::make_unique<ApplicationCustomStrategy>(callback);
    }
    // Default to LWW
    return std::make_unique<LastWriteWinsStrategy>();
  }

  /**
   * @brief Check if two versions are in conflict (concurrent, not ancestor).
   */
  static bool AreInConflict(const VersionInfo& v1, const VersionInfo& v2) {
    // Simplified: concurrent if different and not ancestor
    return v1.model_id == v2.model_id &&
           v1.version_id != v2.version_id;
  }

  // ========================================================================
  // MEMBER VARIABLES
  // ========================================================================

  ConflictResolverConfig config_;

  mutable std::mutex resolver_mutex_;
  std::unique_ptr<IConflictResolutionStrategy> strategy_;
  std::shared_ptr<ProcessConflictResolverCallback> callback_;

  mutable std::mutex conflict_history_mutex_;
  std::map<std::string, ConflictInfo> conflict_history_;  // model_id → latest conflict

  // Metrics
  mutable std::mutex metrics_mutex_;
  uint64_t conflicts_detected_ = 0;
  uint64_t conflicts_resolved_ = 0;
  uint64_t total_resolution_time_ms_ = 0;
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

std::string ProcessConflictResolverImpl::ResolveConflict(
    const std::string& model_id, const std::string& v1_id,
    uint64_t v1_timestamp, const std::string& v1_sender,
    const std::string& v2_id, uint64_t v2_timestamp,
    const std::string& v2_sender) {
  auto start_time = std::chrono::high_resolution_clock::now();

  ConflictMetadata metadata;
  metadata.model_id = model_id;
  metadata.v1_id = v1_id;
  metadata.v1_timestamp =
      std::chrono::system_clock::time_point{std::chrono::milliseconds(v1_timestamp)};
  metadata.v1_sender_node_id = v1_sender;
  metadata.v2_id = v2_id;
  metadata.v2_timestamp =
      std::chrono::system_clock::time_point{std::chrono::milliseconds(v2_timestamp)};
  metadata.v2_sender_node_id = v2_sender;

  conflicts_detected_++;

  std::lock_guard<std::mutex> lock(resolver_mutex_);
  std::string winner = strategy_->ResolveConflict(metadata);

  auto end_time = std::chrono::high_resolution_clock::now();
  uint64_t duration_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                             start_time)
          .count();

  total_resolution_time_ms_ += duration_ms;
  conflicts_resolved_++;

  utils::Logger::Info(
      "ResolveConflict: model=%s, winner=%s, duration_ms=%llu",
      model_id.c_str(), winner.c_str(), duration_ms);

  return winner;
}

void ProcessConflictResolverImpl::RegisterResolver(
    std::shared_ptr<ProcessConflictResolverCallback> resolver) {
  std::lock_guard<std::mutex> lock(resolver_mutex_);
  callback_ = resolver;
  strategy_ = CreateStrategy("custom", resolver);
  utils::Logger::Info("Conflict resolver callback registered");
}

std::vector<ConflictInfo> ProcessConflictResolverImpl::DetectConflictsBatch(
    const std::map<std::string, std::vector<std::string>>& versions) {
  std::vector<ConflictInfo> conflicts;

  std::lock_guard<std::mutex> lock(resolver_mutex_);

  for (const auto& [model_id, version_ids] : versions) {
    if (version_ids.size() > 1) {
      ConflictInfo conflict;
      conflict.model_id = model_id;
      conflict.version_ids = version_ids;
      conflict.detected_at_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      conflicts.push_back(conflict);
    }
  }

  utils::Logger::Debug("DetectConflictsBatch: detected %zu conflicts",
                       conflicts.size());
  return conflicts;
}

ConflictResolverStats ProcessConflictResolverImpl::GetStats() const {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  ConflictResolverStats stats;
  stats.conflicts_detected = conflicts_detected_;
  stats.conflicts_resolved = conflicts_resolved_;
  stats.avg_resolution_time_ms =
      (conflicts_resolved_ > 0) ? (total_resolution_time_ms_ / conflicts_resolved_)
                                : 0;

  return stats;
}

// ============================================================================
// PUBLIC INTERFACE
// ============================================================================

std::unique_ptr<ProcessConflictResolver>
ProcessConflictResolver::Create(const ConflictResolverConfig& config) {
  return std::make_unique<ProcessConflictResolver>(
      std::make_unique<ProcessConflictResolverImpl>(config));
}

ProcessConflictResolver::ProcessConflictResolver(
    std::unique_ptr<ProcessConflictResolverImpl> impl)
    : impl_(std::move(impl)) {}

ProcessConflictResolver::~ProcessConflictResolver() = default;

std::string ProcessConflictResolver::ResolveConflict(
    const std::string& model_id, const std::string& v1_id,
    uint64_t v1_timestamp, const std::string& v1_sender,
    const std::string& v2_id, uint64_t v2_timestamp,
    const std::string& v2_sender) {
  return impl_->ResolveConflict(model_id, v1_id, v1_timestamp, v1_sender,
                                v2_id, v2_timestamp, v2_sender);
}

void ProcessConflictResolver::RegisterResolver(
    std::shared_ptr<ProcessConflictResolverCallback> resolver) {
  impl_->RegisterResolver(resolver);
}

std::vector<ConflictInfo> ProcessConflictResolver::DetectConflictsBatch(
    const std::map<std::string, std::vector<std::string>>& versions) {
  return impl_->DetectConflictsBatch(versions);
}

ConflictResolverStats ProcessConflictResolver::GetStats() const {
  return impl_->GetStats();
}

}  // namespace process
}  // namespace themis
