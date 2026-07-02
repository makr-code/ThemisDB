/// @file shard_placement.cc
/// @brief Implementation of factorization-aware shard placement strategies
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02

#include "src/distributed_tensor/include/shard_placement.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <numeric>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// HardwareProfile Implementation
// ============================================================================

ShardDescriptor* HardwareProfile::find_shard(const std::string& shard_id) {
  for (auto& shard : shards) {
    if (shard.shard_id == shard_id) {
      return &shard;
    }
  }
  return nullptr;
}

const ShardDescriptor* HardwareProfile::find_shard(
    const std::string& shard_id) const {
  for (const auto& shard : shards) {
    if (shard.shard_id == shard_id) {
      return &shard;
    }
  }
  return nullptr;
}

std::string HardwareProfile::find_shard_with_most_capacity() const {
  if (shards.empty()) return "";

  auto max_it = std::max_element(
      shards.begin(), shards.end(),
      [](const ShardDescriptor& a, const ShardDescriptor& b) {
        return a.available_bytes() < b.available_bytes();
      });

  return max_it != shards.end() ? max_it->shard_id : "";
}

std::string HardwareProfile::find_shard_with_lowest_latency() const {
  if (shards.empty()) return "";

  auto min_it = std::min_element(
      shards.begin(), shards.end(),
      [](const ShardDescriptor& a, const ShardDescriptor& b) {
        return a.latency_ms < b.latency_ms;
      });

  return min_it != shards.end() ? min_it->shard_id : "";
}

// ============================================================================
// Helper Functions
// ============================================================================

/// @brief Estimate artifact size from manifest metadata
static uint64_t estimate_artifact_size(const ArtifactManifest& manifest) {
  // Base estimate
  uint64_t estimated_size = 100 * 1024 * 1024;  // 100 MB default

  // Use rank information for refinement
  if (manifest.rank_cap > 0) {
    // Rough estimate: 10 MB per rank unit
    estimated_size = manifest.rank_cap * 10 * 1024 * 1024;
  }

  // Check for explicit size hint in custom_attributes
  const auto size_it = manifest.custom_attributes.find("estimated_size_bytes");
  if (size_it != manifest.custom_attributes.end()) {
    try {
      estimated_size = std::stoull(size_it->second);
    } catch (...) {
      // Keep the default
    }
  }

  return estimated_size;
}

// ============================================================================
// ArtifactPlacementAnalyzer Implementation
// ============================================================================

class DefaultArtifactPlacementAnalyzer : public ArtifactPlacementAnalyzer {
 public:
  bool should_use_factorized_placement(const ArtifactManifest& manifest) const override {
    // Factorization-aware placement benefits these artifact classes:
    // - PRIMARY artifacts (often durable, integrity-critical)
    // - Certain DERIVED artifacts (if explicitly marked)
    // But NOT Ephemeral or AdvisoryOnly

    if (manifest.artifact_class == ArtifactClass::EPHEMERAL ||
        manifest.artifact_class == ArtifactClass::ADVISORY_ONLY) {
      return false;
    }

    // Check for factorization hints in custom_attributes
    const auto& attrs = manifest.custom_attributes;
    const auto it = attrs.find("factorization_type");
    if (it != attrs.end() && !it->second.empty()) {
      // Metadata explicitly states factorization structure is present
      return true;
    }

    // Check for semantic hints in custom_attributes
    const auto sem_it = attrs.find("semantic_hint");
    if (sem_it != attrs.end()) {
      const auto& hint = sem_it->second;
      return hint.find("TT_CORE") != std::string::npos ||
             hint.find("FACTOR_MATRIX") != std::string::npos ||
             hint.find("HT_COMPONENT") != std::string::npos ||
             hint.find("LOW_RANK") != std::string::npos;
    }

    // Check if artifact has rank information (indicates factorization)
    return manifest.rank_cap > 0;
  }

  std::optional<FactorizationHint> extract_factorization_hint(
      const ArtifactManifest& manifest) const override {
    const auto& attrs = manifest.custom_attributes;
    
    // Check for factorization type
    auto type_it = attrs.find("factorization_type");
    if (type_it == attrs.end() || type_it->second.empty()) {
      return std::nullopt;  // No factorization information
    }

    FactorizationHint hint;
    hint.factorization_type = type_it->second;

    // Extract number of factors
    auto num_factors_it = attrs.find("num_factors");
    if (num_factors_it != attrs.end()) {
      try {
        hint.num_factors = std::stoi(num_factors_it->second);
      } catch (...) {
        hint.num_factors = 0;
      }
    } else {
      // Use rank info as fallback
      hint.num_factors = manifest.rank_cap;
    }

    // Extract interdependency score
    auto interdep_it = attrs.find("interdependency_score");
    if (interdep_it != attrs.end()) {
      try {
        hint.interdependency_score = std::stof(interdep_it->second);
      } catch (...) {
        hint.interdependency_score = 0.5f;  // Default: moderate interdependency
      }
    }

    // Check for partial loading support
    auto partial_it = attrs.find("supports_partial_loading");
    hint.supports_partial_loading = 
        partial_it != attrs.end() && partial_it->second == "true";

    return hint;
  }

  std::string validate_placement_feasibility(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware) const override {
    // Check if total shard capacity is sufficient
    // Estimate artifact size from manifest
    uint64_t estimated_artifact_size = estimate_artifact_size(manifest);

    uint64_t total_capacity = 0;
    for (const auto& shard : hardware.shards) {
      total_capacity += shard.capacity_bytes;
    }

    if (estimated_artifact_size > total_capacity) {
      std::ostringstream oss;
      oss << "Estimated artifact size (" << estimated_artifact_size
          << " bytes) exceeds total shard capacity (" << total_capacity << " bytes)";
      return oss.str();
    }

    // Check if replication factor is achievable
    uint32_t healthy_shard_count = 0;
    for (const auto& shard : hardware.shards) {
      if (shard.is_healthy) {
        healthy_shard_count++;
      }
    }

    if (manifest.replication_factor > healthy_shard_count) {
      std::ostringstream oss;
      oss << "Replication factor (" << manifest.replication_factor
          << ") exceeds healthy shards (" << healthy_shard_count << ")";
      return oss.str();
    }

    return "";  // OK
  }
};

// ============================================================================
// PlacementCostModel Implementation
// ============================================================================

class DefaultPlacementCostModel : public PlacementCostModel {
 public:
  float compute_cost(
      const ArtifactManifest& manifest,
      const std::vector<ShardDescriptor>& shards,
      const HardwareProfile& hardware,
      const PlacementConfig& config) const override {
    if (shards.empty()) {
      return 1e6f;  // Very high cost for empty placement
    }

    float latency_cost = 0.0f;
    float capacity_cost = 0.0f;
    float reliability_cost = 0.0f;

    // Compute average latency across replicas
    float total_latency = 0.0f;
    for (const auto& shard : shards) {
      total_latency += shard.latency_ms;
    }
    latency_cost = total_latency / shards.size();

    // Normalize to [0, 1] range
    if (hardware.latency_baseline_ms > 0) {
      latency_cost /= (hardware.latency_baseline_ms + 50);
    }

    // Compute capacity cost (penalty for high utilization)
    float max_utilization = 0.0f;
    for (const auto& shard : shards) {
      float util = shard.utilization_percent();
      if (util > max_utilization) {
        max_utilization = util;
      }
    }
    // Linear penalty for utilization
    capacity_cost = std::max(0.0f, (max_utilization - 50.0f) / 50.0f);

    // Compute reliability cost (1 - avg reliability score)
    float total_reliability = 0.0f;
    for (const auto& shard : shards) {
      total_reliability += shard.reliability_score;
    }
    reliability_cost = 1.0f - (total_reliability / shards.size());

    // Normalize weights
    float total_weight = config.latency_weight + config.capacity_weight +
                        config.reliability_weight;
    if (total_weight < 1e-6f) {
      total_weight = 1.0f;
    }

    float normalized_latency = config.latency_weight / total_weight;
    float normalized_capacity = config.capacity_weight / total_weight;
    float normalized_reliability = config.reliability_weight / total_weight;

    // Weighted cost
    float total_cost = (normalized_latency * latency_cost) +
                       (normalized_capacity * capacity_cost) +
                       (normalized_reliability * reliability_cost);

    return total_cost;
  }

  float estimate_latency(
      const std::vector<ShardDescriptor>& shards,
      const HardwareProfile& hardware) const override {
    if (shards.empty()) {
      return 0.0f;
    }

    // Use minimum latency for primary access, average for replicas
    float min_latency = shards[0].latency_ms;
    for (const auto& shard : shards) {
      if (shard.latency_ms < min_latency) {
        min_latency = shard.latency_ms;
      }
    }

    // Bias towards primary (index 0) if available
    if (shards.size() == 1) {
      return shards[0].latency_ms;
    }

    return min_latency;
  }
};

// ============================================================================
// Concrete Placement Strategy Implementations
// ============================================================================

class RoundRobinPlacementStrategy : public PlacementStrategy {
 public:
  PlacementStrategyType type() const override {
    return PlacementStrategyType::ROUND_ROBIN;
  }

  std::string name() const override { return "RoundRobin"; }

  bool supports_artifact_class(ArtifactClass artifact_class) const override {
    return true;  // Supports all artifact classes
  }

  PlacementResult place(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config,
      const std::shared_ptr<ArtifactPlacementAnalyzer>& analyzer) const override {
    PlacementResult result;

    if (hardware.shards.empty()) {
      result.success = false;
      result.error_message = "No available shards";
      return result;
    }

    // Simple round-robin across shards
    std::vector<std::string> placements;
    uint32_t target_replicas =
        std::min(config.target_replication_factor,
                 static_cast<uint32_t>(hardware.shards.size()));

    uint64_t estimated_size = estimate_artifact_size(manifest);

    for (uint32_t i = 0; i < target_replicas; ++i) {
      const auto& shard = hardware.shards[i % hardware.shards.size()];
      if (shard.available_bytes() >= estimated_size) {
        placements.push_back(shard.shard_id);
      }
    }

    if (placements.empty()) {
      result.success = false;
      result.error_message = "No shards have sufficient capacity";
      return result;
    }

    result.success = true;
    result.shard_placements = placements;
    result.rationale = "RoundRobin placement across " + std::to_string(placements.size()) +
                       " shards";

    return result;
  }
};

class FactorizedPlacementStrategy : public PlacementStrategy {
 public:
  PlacementStrategyType type() const override {
    return PlacementStrategyType::FACTORIZED;
  }

  std::string name() const override { return "Factorized"; }

  bool supports_artifact_class(ArtifactClass artifact_class) const override {
    // Best for PRIMARY and DERIVED artifacts
    return artifact_class == ArtifactClass::PRIMARY ||
           artifact_class == ArtifactClass::DERIVED;
  }

  PlacementResult place(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config,
      const std::shared_ptr<ArtifactPlacementAnalyzer>& analyzer) const override {
    PlacementResult result;

    if (hardware.shards.empty()) {
      result.success = false;
      result.error_message = "No available shards";
      return result;
    }

    // Extract factorization hints if available
    std::optional<FactorizationHint> hint;
    if (analyzer) {
      hint = analyzer->extract_factorization_hint(manifest);
    }

    // Placement strategy based on factorization
    std::vector<std::string> placements;

    if (hint && hint->num_factors > 0) {
      // Factor-aware placement: try to co-locate related factors
      placements = place_factorized_components(manifest, hardware, config, hint.value());
    } else {
      // Fall back to capacity-based placement
      placements = place_by_capacity(manifest, hardware, config);
    }

    if (placements.empty()) {
      result.success = false;
      result.error_message = "No suitable placement found for factorized artifact";
      return result;
    }

    result.success = true;
    result.shard_placements = placements;
    result.rationale = "Factorized placement: " + (hint ? hint->factorization_type : "unknown") +
                       " with " + std::to_string(placements.size()) + " replicas";

    return result;
  }

 private:
  std::vector<std::string> place_factorized_components(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config,
      const FactorizationHint& hint) const {
    std::vector<std::string> placements;

    // Strategy: For TT cores with low interdependency, place factors
    // on different shards to enable partial loading.
    // For high interdependency, co-locate factors.

    uint32_t target_replicas =
        std::min(config.target_replication_factor,
                 static_cast<uint32_t>(hardware.shards.size()));

    uint64_t estimated_size = estimate_artifact_size(manifest);

    if (hint.interdependency_score < 0.3f && hint.supports_partial_loading) {
      // Low interdependency: distribute factors across shards
      std::vector<size_t> shard_indices;
      for (size_t i = 0; i < hardware.shards.size(); ++i) {
        shard_indices.push_back(i);
      }

      // Sort by available capacity (descending)
      std::sort(
          shard_indices.begin(), shard_indices.end(),
          [&hardware](size_t a, size_t b) {
            return hardware.shards[a].available_bytes() >
                   hardware.shards[b].available_bytes();
          });

      // Select first target_replicas shards with sufficient capacity
      for (size_t i = 0; i < target_replicas && i < shard_indices.size(); ++i) {
        const auto& shard = hardware.shards[shard_indices[i]];
        if (shard.available_bytes() >= estimated_size) {
          placements.push_back(shard.shard_id);
        }
      }
    } else {
      // High interdependency or unknown: use co-location strategy
      placements = place_by_capacity(manifest, hardware, config);
    }

    return placements;
  }

  std::vector<std::string> place_by_capacity(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config) const {
    std::vector<std::string> placements;
    uint32_t target_replicas =
        std::min(config.target_replication_factor,
                 static_cast<uint32_t>(hardware.shards.size()));

    uint64_t estimated_size = estimate_artifact_size(manifest);

    // Create a sorted list of shards by available capacity
    std::vector<size_t> shard_indices;
    for (size_t i = 0; i < hardware.shards.size(); ++i) {
      shard_indices.push_back(i);
    }

    std::sort(
        shard_indices.begin(), shard_indices.end(),
        [&hardware](size_t a, size_t b) {
          return hardware.shards[a].available_bytes() >
                 hardware.shards[b].available_bytes();
        });

    // Select shards with sufficient capacity
    for (size_t i = 0; i < target_replicas && i < shard_indices.size(); ++i) {
      const auto& shard = hardware.shards[shard_indices[i]];
      if (shard.available_bytes() >= estimated_size && shard.is_healthy) {
        placements.push_back(shard.shard_id);
      }
    }

    return placements;
  }
};

class CostAwarePlacementStrategy : public PlacementStrategy {
 public:
  explicit CostAwarePlacementStrategy(
      std::shared_ptr<PlacementCostModel> cost_model = nullptr)
      : cost_model_(cost_model ? cost_model
                               : std::make_shared<DefaultPlacementCostModel>()) {}

  PlacementStrategyType type() const override {
    return PlacementStrategyType::COST_AWARE;
  }

  std::string name() const override { return "CostAware"; }

  bool supports_artifact_class(ArtifactClass artifact_class) const override {
    return artifact_class != ArtifactClass::EPHEMERAL;
  }

  PlacementResult place(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config,
      const std::shared_ptr<ArtifactPlacementAnalyzer>& analyzer) const override {
    PlacementResult result;

    if (hardware.shards.empty()) {
      result.success = false;
      result.error_message = "No available shards";
      return result;
    }

    uint32_t target_replicas =
        std::min(config.target_replication_factor,
                 static_cast<uint32_t>(hardware.shards.size()));

    uint64_t estimated_size = estimate_artifact_size(manifest);

    // Find best combination of shards by cost
    std::vector<std::string> best_placement;
    float best_cost = 1e9f;

    // Try all combinations of target_replicas shards
    std::vector<size_t> indices;
    for (size_t i = 0; i < hardware.shards.size(); ++i) {
      indices.push_back(i);
    }

    // Greedy selection: pick best shard iteratively
    std::vector<bool> selected(hardware.shards.size(), false);
    for (uint32_t i = 0; i < target_replicas; ++i) {
      float best_shard_cost = 1e9f;
      size_t best_shard_idx = 0;

      for (size_t j = 0; j < hardware.shards.size(); ++j) {
        if (selected[j]) continue;  // Already selected
        const auto& shard = hardware.shards[j];
        if (shard.available_bytes() < estimated_size ||
            !shard.is_healthy) {
          continue;  // Not suitable
        }

        // Compute cost for this shard
        std::vector<ShardDescriptor> candidate_shards;
        for (size_t k = 0; k < j; ++k) {
          if (selected[k]) {
            candidate_shards.push_back(hardware.shards[k]);
          }
        }
        candidate_shards.push_back(shard);

        float cost = cost_model_->compute_cost(manifest, candidate_shards, hardware, config);
        if (cost < best_shard_cost) {
          best_shard_cost = cost;
          best_shard_idx = j;
        }
      }

      selected[best_shard_idx] = true;
      best_placement.push_back(hardware.shards[best_shard_idx].shard_id);
    }

    if (best_placement.empty()) {
      result.success = false;
      result.error_message = "No suitable shards for cost-aware placement";
      return result;
    }

    // Compute final placement cost
    std::vector<ShardDescriptor> final_shards;
    for (const auto& shard_id : best_placement) {
      const auto* shard = hardware.find_shard(shard_id);
      if (shard) {
        final_shards.push_back(*shard);
      }
    }

    result.success = true;
    result.shard_placements = best_placement;
    result.placement_cost = cost_model_->compute_cost(manifest, final_shards, hardware, config);
    result.estimated_latency_ms = cost_model_->estimate_latency(final_shards, hardware);
    result.rationale = "Cost-aware placement: cost=" +
                       std::to_string(result.placement_cost) +
                       ", latency=" + std::to_string(result.estimated_latency_ms) + "ms";

    return result;
  }

 private:
  std::shared_ptr<PlacementCostModel> cost_model_;
};

class BalancedPlacementStrategy : public PlacementStrategy {
 public:
  PlacementStrategyType type() const override {
    return PlacementStrategyType::BALANCED;
  }

  std::string name() const override { return "Balanced"; }

  bool supports_artifact_class(ArtifactClass artifact_class) const override {
    return true;
  }

  PlacementResult place(
      const ArtifactManifest& manifest,
      const HardwareProfile& hardware,
      const PlacementConfig& config,
      const std::shared_ptr<ArtifactPlacementAnalyzer>& analyzer) const override {
    // Balanced strategy: use factorized if applicable, otherwise cost-aware
    bool use_factorized = config.use_factorization_hints && analyzer &&
                          analyzer->should_use_factorized_placement(manifest);

    if (use_factorized) {
      FactorizedPlacementStrategy factorized;
      return factorized.place(manifest, hardware, config, analyzer);
    } else {
      CostAwarePlacementStrategy cost_aware;
      return cost_aware.place(manifest, hardware, config, analyzer);
    }
  }
};

// ============================================================================
// PlacementStrategyFactory Implementation
// ============================================================================

std::shared_ptr<PlacementStrategy> PlacementStrategyFactory::create_strategy(
    PlacementStrategyType type) {
  switch (type) {
    case PlacementStrategyType::ROUND_ROBIN:
      return std::make_shared<RoundRobinPlacementStrategy>();
    case PlacementStrategyType::FACTORIZED:
      return std::make_shared<FactorizedPlacementStrategy>();
    case PlacementStrategyType::COST_AWARE:
      return std::make_shared<CostAwarePlacementStrategy>();
    case PlacementStrategyType::BALANCED:
      return std::make_shared<BalancedPlacementStrategy>();
    default:
      return nullptr;
  }
}

std::map<PlacementStrategyType, std::shared_ptr<PlacementStrategy>>
PlacementStrategyFactory::create_all_strategies() {
  std::map<PlacementStrategyType, std::shared_ptr<PlacementStrategy>> strategies;
  strategies[PlacementStrategyType::ROUND_ROBIN] =
      std::make_shared<RoundRobinPlacementStrategy>();
  strategies[PlacementStrategyType::FACTORIZED] =
      std::make_shared<FactorizedPlacementStrategy>();
  strategies[PlacementStrategyType::COST_AWARE] =
      std::make_shared<CostAwarePlacementStrategy>();
  strategies[PlacementStrategyType::BALANCED] =
      std::make_shared<BalancedPlacementStrategy>();
  return strategies;
}

PlacementStrategyType PlacementStrategyFactory::recommend_strategy(
    const HardwareProfile& hardware) {
  // Recommend based on hardware characteristics
  if (hardware.shards.size() > 4 && hardware.enable_factorization_aware) {
    return PlacementStrategyType::BALANCED;  // Default for diverse environments
  } else if (hardware.shards.size() <= 2) {
    return PlacementStrategyType::ROUND_ROBIN;  // Simple strategy for small clusters
  } else {
    return PlacementStrategyType::COST_AWARE;  // Balance efficiency
  }
}

// ============================================================================
// ShardPlacementCoordinator Implementation
// ============================================================================

ShardPlacementCoordinator::ShardPlacementCoordinator(
    const HardwareProfile& hardware)
    : hardware_(hardware),
      analyzer_(std::make_shared<DefaultArtifactPlacementAnalyzer>()),
      cost_model_(std::make_shared<DefaultPlacementCostModel>()) {
  // Initialize with recommended strategy
  auto recommended_type = PlacementStrategyFactory::recommend_strategy(hardware);
  strategy_ = PlacementStrategyFactory::create_strategy(recommended_type);
}

PlacementResult ShardPlacementCoordinator::place_artifact(
    const ArtifactManifest& manifest,
    const std::shared_ptr<PlacementConfig>& config) {
  // Use provided config or create default
  PlacementConfig placement_config = config ? *config : PlacementConfig();

  // Validate placement feasibility first
  std::string feasibility_error = analyzer_->validate_placement_feasibility(manifest, hardware_);
  if (!feasibility_error.empty()) {
    return PlacementResult{false, feasibility_error};
  }

  // Perform placement
  if (!strategy_) {
    return PlacementResult{false, "No placement strategy configured"};
  }

  return strategy_->place(manifest, hardware_, placement_config, analyzer_);
}

std::vector<PlacementResult> ShardPlacementCoordinator::rebalance_placements(
    const std::vector<ArtifactManifest>& manifests,
    const std::shared_ptr<PlacementConfig>& config) {
  std::vector<PlacementResult> results;
  results.reserve(manifests.size());

  for (const auto& manifest : manifests) {
    results.push_back(place_artifact(manifest, config));
  }

  return results;
}

std::map<std::string, std::string> ShardPlacementCoordinator::validate_placements(
    const std::vector<ArtifactManifest>& manifests) const {
  std::map<std::string, std::string> validation_errors;

  for (const auto& manifest : manifests) {
    std::string error = analyzer_->validate_placement_feasibility(manifest, hardware_);
    if (!error.empty()) {
      validation_errors[manifest.artifact_id] = error;
    }

    // Additional checks: verify shard placements exist
    for (const auto& shard_id : manifest.shard_placements) {
      if (!hardware_.find_shard(shard_id)) {
        validation_errors[manifest.artifact_id] = "Invalid shard placement: " + shard_id;
        break;
      }
    }
  }

  return validation_errors;
}

void ShardPlacementCoordinator::set_hardware(const HardwareProfile& hardware) {
  hardware_ = hardware;
}

void ShardPlacementCoordinator::set_strategy(
    std::shared_ptr<PlacementStrategy> strategy) {
  strategy_ = strategy;
}

const std::shared_ptr<PlacementStrategy>& ShardPlacementCoordinator::get_strategy()
    const {
  return strategy_;
}

void ShardPlacementCoordinator::set_analyzer(
    std::shared_ptr<ArtifactPlacementAnalyzer> analyzer) {
  analyzer_ = analyzer;
}

void ShardPlacementCoordinator::set_cost_model(
    std::shared_ptr<PlacementCostModel> cost_model) {
  cost_model_ = cost_model;
}

// ============================================================================
// Singleton Instances
// ============================================================================

ArtifactPlacementAnalyzer& get_default_analyzer() {
  static DefaultArtifactPlacementAnalyzer analyzer;
  return analyzer;
}

PlacementCostModel& get_default_cost_model() {
  static DefaultPlacementCostModel cost_model;
  return cost_model;
}

}  // namespace distributed_tensor
}  // namespace themis
