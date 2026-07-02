/// @file shard_placement.h
/// @brief Factorization-aware shard placement strategy for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02
///
/// This header defines placement strategies for distributed tensor artifacts
/// with support for factorization-aware distribution.
///
/// ## Design Philosophy
///
/// Tensor artifacts have different structural characteristics that affect
/// optimal placement strategies:
///
/// - **TT cores**: Can be placed independently; partial loading benefits from
///   factorization-aware placement
/// - **Factor matrices**: Benefit from co-location or strategic distribution
/// - **Hierarchical tensor components**: Require hierarchical placement logic
/// - **Ephemeral/derived artifacts**: Can use simpler placement strategies
///
/// This header provides:
/// - Artifact suitability assessment for factorization-aware placement
/// - Placement strategy interfaces and implementations
/// - Cost models for placement decisions
/// - Recovery and rebalancing support
///
/// ## References
/// - DISTRIBUTED_TENSOR_SHARDING.md § 6.3 Factorization-aware Distribution
/// - docs/EPIC3_SHARD_PLACEMENT.md: Placement strategy planning and roadmap
/// - src/distributed_tensor/include/artifact_manifest.h: Manifest structure
/// - src/distributed_tensor/include/tensor_artifact_classes.h: Artifact classification

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include <stdexcept>

#include "src/distributed_tensor/include/artifact_manifest.h"
#include "src/distributed_tensor/include/tensor_artifact_classes.h"

namespace themis {
namespace distributed_tensor {

// Forward declarations
struct PlacementConfig;
struct PlacementResult;
class PlacementStrategy;
class PlacementCostModel;

/// @brief Represents a shard in the distributed system.
///
/// Tracks physical and logical properties relevant to artifact placement.
struct ShardDescriptor {
  /// Unique shard identifier.
  std::string shard_id;

  /// Total storage capacity in bytes.
  uint64_t capacity_bytes = 0;

  /// Currently used storage in bytes.
  uint64_t used_bytes = 0;

  /// Network latency to this shard in milliseconds (from reference point).
  uint32_t latency_ms = 0;

  /// Reliability score [0.0, 1.0] (1.0 = highly reliable).
  float reliability_score = 1.0f;

  /// Hardware tier: "CPU", "GPU", "TPU", "NVRAM", "DISK".
  std::string tier;

  /// Number of replicas already placed on this shard.
  uint32_t replica_count = 0;

  /// Is shard currently healthy? False = degraded or recovering.
  bool is_healthy = true;

  /// @brief Compute available capacity.
  uint64_t available_bytes() const {
    return (capacity_bytes > used_bytes) ? (capacity_bytes - used_bytes) : 0;
  }

  /// @brief Compute utilization percentage [0, 100].
  float utilization_percent() const {
    if (capacity_bytes == 0) return 0.0f;
    return (100.0f * used_bytes) / capacity_bytes;
  }
};

/// @brief Hardware profile describing network and storage characteristics.
///
/// Used to inform placement decisions based on system topology.
struct HardwareProfile {
  /// List of all available shards.
  std::vector<ShardDescriptor> shards;

  /// Network bandwidth between shards in MB/s (symmetric assumption).
  float inter_shard_bandwidth_mbps = 100.0f;

  /// Network latency baseline in milliseconds (lowest observed).
  uint32_t latency_baseline_ms = 1;

  /// Replication factor target (minimum replicas per artifact).
  uint32_t replication_factor = 2;

  /// Maximum acceptable utilization percentage [0, 100].
  float max_utilization_percent = 80.0f;

  /// Enable factorization-aware placement (if applicable to artifact).
  bool enable_factorization_aware = true;

  /// @brief Find shard by ID.
  /// @return Pointer to shard descriptor or nullptr if not found.
  ShardDescriptor* find_shard(const std::string& shard_id);
  const ShardDescriptor* find_shard(const std::string& shard_id) const;

  /// @brief Find shard with most available capacity.
  /// @return Shard ID or empty string if no suitable shard found.
  std::string find_shard_with_most_capacity() const;

  /// @brief Find shard with lowest latency.
  /// @return Shard ID or empty string.
  std::string find_shard_with_lowest_latency() const;
};

/// @brief Enumeration of placement strategy types.
enum class PlacementStrategyType : uint8_t {
  /// Round-robin across shards (baseline, non-factorization-aware).
  ROUND_ROBIN = 0,

  /// Factorization-aware placement for TT cores and factor matrices.
  /// Considers tensor structure for partial loading efficiency.
  FACTORIZED = 1,

  /// Hierarchical placement for composite tensor structures.
  /// Groups related components for efficient reconstruction.
  HIERARCHICAL = 2,

  /// Cost-aware placement balancing latency, capacity, and reliability.
  COST_AWARE = 3,

  /// Balanced strategy mixing multiple objectives.
  BALANCED = 4,
};

/// @brief Configuration for placement operations.
struct PlacementConfig {
  /// Strategy to use for placement decisions.
  PlacementStrategyType strategy = PlacementStrategyType::BALANCED;

  /// Target replication factor.
  uint32_t target_replication_factor = 2;

  /// Whether to allow placement on shards already holding replicas.
  bool allow_co_placement = false;

  /// Prefer high-reliability shards even if farther away.
  bool prefer_reliability = true;

  /// Weight for latency in cost model [0.0, 1.0].
  float latency_weight = 0.3f;

  /// Weight for capacity utilization in cost model [0.0, 1.0].
  float capacity_weight = 0.3f;

  /// Weight for reliability in cost model [0.0, 1.0].
  float reliability_weight = 0.4f;

  /// Consider factorization structure if available.
  bool use_factorization_hints = true;

  /// Maximum time in milliseconds for placement decision.
  uint32_t timeout_ms = 1000;
};

/// @brief Result of a placement decision.
struct PlacementResult {
  /// Whether placement was successful.
  bool success = false;

  /// Error message if placement failed.
  std::string error_message;

  /// Assigned shard placements (in priority order).
  /// Index 0 = primary, 1+ = replicas.
  std::vector<std::string> shard_placements;

  /// Estimated cost of this placement (lower is better).
  float placement_cost = 0.0f;

  /// Estimated retrieval latency in milliseconds.
  float estimated_latency_ms = 0.0f;

  /// Rationale for placement decision.
  std::string rationale;

  /// @brief Check if placement is valid (at least one shard).
  bool is_valid() const { return success && !shard_placements.empty(); }
};

/// @brief Describes factorization structure of a tensor artifact.
///
/// Provides hints to placement strategies about tensor decomposition.
struct FactorizationHint {
  /// Type of factorization: "TT" (tensor train), "HT" (hierarchical Tucker),
  /// "CP" (CANDECOMP/PARAFAC), "LOW_RANK", "UNSTRUCTURED".
  std::string factorization_type;

  /// Number of factors or cores.
  uint32_t num_factors = 0;

  /// Estimated sizes of each factor in bytes (if available).
  std::vector<uint64_t> factor_sizes;

  /// Interdependencies between factors (0 = independent, 1 = fully dependent).
  float interdependency_score = 0.0f;

  /// Is partial loading beneficial for this tensor?
  bool supports_partial_loading = false;

  /// Recommended co-location groups (if any).
  /// Each group is a set of factor indices that should be co-located.
  std::vector<std::vector<uint32_t>> co_location_hints;
};

/// @brief Interface for artifact-suitability assessment.
///
/// Determines whether an artifact is suitable for factorization-aware placement
/// and what strategy to use.
class ArtifactPlacementAnalyzer {
 public:
  virtual ~ArtifactPlacementAnalyzer() = default;

  /// @brief Assess whether artifact benefits from factorization-aware placement.
  /// @param manifest Artifact manifest to analyze.
  /// @return true if factorization-aware placement is recommended.
  virtual bool should_use_factorized_placement(const ArtifactManifest& manifest) const;

  /// @brief Extract factorization hints from artifact metadata.
  /// @param manifest Artifact manifest.
  /// @return Factorization hint or nullopt if not available.
  virtual std::optional<FactorizationHint> extract_factorization_hint(
      const ArtifactManifest& manifest) const;

  /// @brief Validate placement feasibility for artifact.
  /// @param manifest Artifact manifest.
  /// @param hardware Hardware profile available.
  /// @return Error message if placement is not feasible, empty string if OK.
  virtual std::string validate_placement_feasibility(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware) const;
};

/// @brief Interface for placement cost modeling.
class PlacementCostModel {
 public:
  virtual ~PlacementCostModel() = default;

  /// @brief Compute placement cost for assigning artifact to shards.
  /// @param manifest Artifact manifest.
  /// @param shards Candidate shards for placement.
  /// @param hardware Hardware profile.
  /// @param config Placement configuration.
  /// @return Placement cost (lower is better).
  virtual float compute_cost(
      const ArtifactManifest& manifest,
      const std::vector<ShardDescriptor>& shards,
      const HardwareProfile& hardware,
      const PlacementConfig& config) const;

  /// @brief Estimate retrieval latency for placement.
  /// @param shards Shards where artifact is placed.
  /// @param hardware Hardware profile.
  /// @return Estimated latency in milliseconds.
  virtual float estimate_latency(
      const std::vector<ShardDescriptor>& shards,
      const HardwareProfile& hardware) const;
};

/// @brief Interface for placement strategies.
///
/// Implementations of this interface define specific algorithms
/// for selecting shards for artifact placement.
class PlacementStrategy {
 public:
  virtual ~PlacementStrategy() = default;

  /// @brief Get strategy type identifier.
  virtual PlacementStrategyType type() const = 0;

  /// @brief Get human-readable strategy name.
  virtual std::string name() const = 0;

  /// @brief Execute placement decision.
  /// @param manifest Artifact manifest to place.
  /// @param hardware Hardware profile with available shards.
  /// @param config Placement configuration.
  /// @param analyzer Optional artifact analyzer for factorization hints.
  /// @return Placement result with shard assignments.
  virtual PlacementResult place(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config,
      const std::shared_ptr<ArtifactPlacementAnalyzer>& analyzer = nullptr) const = 0;

  /// @brief Check if strategy can handle the artifact type.
  /// @param artifact_class Artifact class to check.
  /// @return true if strategy supports this artifact class.
  virtual bool supports_artifact_class(ArtifactClass artifact_class) const = 0;
};

/// @brief Factory for creating placement strategies.
class PlacementStrategyFactory {
 public:
  /// @brief Create a placement strategy by type.
  /// @param type Strategy type identifier.
  /// @return Shared pointer to strategy instance, or nullptr if type unsupported.
  static std::shared_ptr<PlacementStrategy> create_strategy(
      PlacementStrategyType type);

  /// @brief Create all available strategies.
  /// @return Map of strategy type to strategy instance.
  static std::map<PlacementStrategyType, std::shared_ptr<PlacementStrategy>>
  create_all_strategies();

  /// @brief Get default strategy based on hardware profile.
  /// @param hardware Hardware profile to analyze.
  /// @return Recommended strategy type for the hardware.
  static PlacementStrategyType recommend_strategy(const HardwareProfile& hardware);
};

/// @brief Coordinator for shard placement operations.
///
/// High-level interface for managing artifact placement across shards.
class ShardPlacementCoordinator {
 public:
  explicit ShardPlacementCoordinator(const HardwareProfile& hardware);
  ~ShardPlacementCoordinator() = default;

  /// @brief Place an artifact across shards.
  /// @param manifest Artifact manifest to place.
  /// @param config Placement configuration (uses default if nullptr).
  /// @return Placement result with shard assignments.
  PlacementResult place_artifact(
      const ArtifactManifest& manifest,
      const std::shared_ptr<PlacementConfig>& config = nullptr);

  /// @brief Rebalance existing placements.
  /// @param manifests Vector of artifact manifests to rebalance.
  /// @param config Placement configuration.
  /// @return Vector of rebalancing results.
  std::vector<PlacementResult> rebalance_placements(
      const std::vector<ArtifactManifest>& manifests,
      const std::shared_ptr<PlacementConfig>& config = nullptr);

  /// @brief Validate current placements.
  /// @param manifests Vector of artifact manifests with current placements.
  /// @return Map of artifact ID to validation errors (empty if valid).
  std::map<std::string, std::string> validate_placements(
      const std::vector<ArtifactManifest>& manifests) const;

  /// @brief Get current hardware profile.
  const HardwareProfile& hardware() const { return hardware_; }

  /// @brief Update hardware profile.
  void set_hardware(const HardwareProfile& hardware);

  /// @brief Set placement strategy.
  void set_strategy(std::shared_ptr<PlacementStrategy> strategy);

  /// @brief Get current placement strategy.
  const std::shared_ptr<PlacementStrategy>& get_strategy() const;

  /// @brief Set artifact analyzer for factorization hints.
  void set_analyzer(std::shared_ptr<ArtifactPlacementAnalyzer> analyzer);

  /// @brief Set cost model for placement decisions.
  void set_cost_model(std::shared_ptr<PlacementCostModel> cost_model);

 private:
  HardwareProfile hardware_;
  std::shared_ptr<PlacementStrategy> strategy_;
  std::shared_ptr<ArtifactPlacementAnalyzer> analyzer_;
  std::shared_ptr<PlacementCostModel> cost_model_;
};

/// @brief Get singleton instance of the artifact placement analyzer.
ArtifactPlacementAnalyzer& get_default_analyzer();

/// @brief Get singleton instance of the placement cost model.
PlacementCostModel& get_default_cost_model();

}  // namespace distributed_tensor
}  // namespace themis
