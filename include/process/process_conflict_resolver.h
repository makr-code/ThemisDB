/**
 * @file process_conflict_resolver.h
 * @brief Multi-model conflict detection and resolution orchestrator.
 * @version 2.1.0
 * @date 2026-08-06
 */

#ifndef THEMISDB_INCLUDE_PROCESS_PROCESS_CONFLICT_RESOLVER_H
#define THEMISDB_INCLUDE_PROCESS_PROCESS_CONFLICT_RESOLVER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>

namespace themis {
namespace process {

// Forward declarations
class ProcessConflictResolverImpl;
class ProcessConflictResolverCallback;

/**
 * @brief Configuration for conflict resolver.
 */
struct ConflictResolverConfig {
  std::string strategy = "LWW";  // LWW, FWW, or custom
  uint32_t callback_timeout_ms = 5000;
  uint32_t batch_size = 100;
};

/**
 * @brief Conflict information.
 */
struct ConflictInfo {
  std::string model_id;
  std::vector<std::string> version_ids;
  uint64_t detected_at_ms = 0;
};

/**
 * @brief Version information for conflict detection.
 */
struct VersionInfo {
  std::string model_id;
  std::string version_id;
};

/**
 * @brief Conflict resolver statistics.
 */
struct ConflictResolverStats {
  uint64_t conflicts_detected = 0;
  uint64_t conflicts_resolved = 0;
  uint64_t avg_resolution_time_ms = 0;
};

/**
 * @class ProcessConflictResolver
 * @brief Multi-model conflict detection and resolution engine.
 *
 * Detects concurrent updates on the same model version and invokes
 * application-provided callbacks or LWW fallback for deterministic resolution.
 */
class ProcessConflictResolver {
 public:
  /**
   * @brief Factory method to create conflict resolver.
   */
  static std::unique_ptr<ProcessConflictResolver> Create(
      const ConflictResolverConfig& config);

  /**
   * @brief Constructor.
   */
  explicit ProcessConflictResolver(
      std::unique_ptr<ProcessConflictResolverImpl> impl);

  /**
   * @brief Destructor.
   */
  ~ProcessConflictResolver();

  /**
   * @brief Detect and resolve conflict between two versions.
   */
  std::string ResolveConflict(const std::string& model_id,
                              const std::string& v1_id,
                              uint64_t v1_timestamp,
                              const std::string& v1_sender,
                              const std::string& v2_id,
                              uint64_t v2_timestamp,
                              const std::string& v2_sender);

  /**
   * @brief Register application-provided resolver callback.
   */
  void RegisterResolver(std::shared_ptr<ProcessConflictResolverCallback> resolver);

  /**
   * @brief Detect conflicts in a batch of model versions.
   */
  std::vector<ConflictInfo> DetectConflictsBatch(
      const std::map<std::string, std::vector<std::string>>& versions);

  /**
   * @brief Get conflict resolver statistics.
   */
  ConflictResolverStats GetStats() const;

 private:
  std::unique_ptr<ProcessConflictResolverImpl> impl_;
};

}  // namespace process
}  // namespace themis

#endif  // THEMISDB_INCLUDE_PROCESS_PROCESS_CONFLICT_RESOLVER_H
